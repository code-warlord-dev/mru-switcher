// Fuzz harness for the external-overlay parse path (REQ-O-005 / REQ-O-008).
//
// The parser (overlay_protocol::parse_command) runs on the compositor thread
// and consumes bytes from an untrusted local peer, so it must NEVER crash or
// throw, and must honour the one-line-drop contract: any line that is not an
// exact known message -> std::nullopt, session state untouched.
//
// Two build modes in a single translation unit:
//   * normal / CI build (this is the mode registered with ctest): a standalone
//     main() drives a bounded, DETERMINISTIC set of generated + mutated inputs
//     and asserts the nullopt contract. Runs under every CI job, including the
//     ASan/UBSan `sanitize` job. No libFuzzer dependency.
//   * future libFuzzer build: compile with -DMRU_FUZZ_NO_MAIN so this TU links
//     against the fuzzer's own main() and uses the LLVMFuzzerTestOneInput
//     entry point (libFuzzer-compatible, future-proof).
//
// T-FUZZ-01 (docs/SPEC.md §9, docs/REQ-TRACE.md).

#include "overlay_protocol.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

using mru::plugin::overlay_protocol::kMaxLineBytes; // REQ-O-005 line cap (64 KiB)
using mru::plugin::overlay_protocol::parse_command;

// Deterministic: fixed seed + fixed corpus -> byte-for-byte reproducible on
// every CI run and platform (runtime well under 3 s).
inline constexpr std::uint32_t kSeed = 0x5EED0;
inline constexpr std::size_t kIterations = 4000;

// Real protocol corpus: the exact message shapes emitted/accepted by the
// reference peer (tools/overlay_stub.py) and observed on the live nest
// (docs/agent-state/reports/2026-09-19-m5-s3-nest-smoke.md). Feed them
// through the parser even though outbound events (session_start/selection/
// session_end) are not peer commands: a hostile peer is allowed to send any
// bytes, so these must parse without crashing and are legitimate nullopt cases.
const std::vector<std::string> kCorpus = {
    "{\"v\":1,\"type\":\"apply\"}",
    "{\"v\":1,\"type\":\"cancel\"}",
    "{\"v\":1,\"type\":\"select\",\"index\":0}",
    "{\"v\":1,\"type\":\"select\",\"index\":7}",
    "  {\"v\": 1, \"type\": \"cancel\"}\r\n",
    "{\"v\":1,\"type\":\"session_start\","
    "\"windows\":[{\"addr\":\"0x5d8b1991bde0\",\"title\":\"term\",\"class\":\"foot\"},"
    "{\"addr\":\"0x0\",\"title\":\"a\\\"b\\\\c\\nd\",\"class\":\"\"}],\"index\":1}",
    "{\"v\":1,\"type\":\"selection\",\"index\":2}",
    "{\"v\":1,\"type\":\"session_end\",\"reason\":\"applied\"}",
};

// Structured adversarial inputs that MUST yield nullopt (REQ-O-005). Asserted
// in the standalone driver; also used as mutation bases below.
const std::vector<std::string> kMustReject = {
    "",                                                             // empty
    "{",                                                            // single brace (size < 2)
    "{]",                                                           // balanced braces required
    "not json",                                                     // no braces at all
    "{\"v\":1,",                                                    // truncated object before `type`
    "{}",                                                           // no version
    "{\"type\":\"apply\"}",                                         // no version
    "{\"v\":2,\"type\":\"apply\"}",                                 // unknown version
    "{\"v\":1,\"type\":\"focus\"}",                                 // unknown type
    "{\"v\":1,\"type\":\"appl\\q\"}",                               // unsupported JSON escape
    "{\"v\":1,\"type\":\"select\"}",                                // missing index
    "{\"v\":1,\"type\":\"select\",\"index\":-1}",                   // negative index
    "{\"v\":1,\"type\":\"select\",\"index\":\"5\"}",                // string index
    "{\"v\":1,\"type\":\"select\",\"index\":18446744073709551616}", // int64 overflow
};

void run_input(const std::uint8_t *data, std::size_t size) {
    // Constructing the string_view copies nothing; the parser must not throw
    // or touch memory outside [data, data + size) for ANY input.
    std::string_view line(reinterpret_cast<const char *>(data), size);
    (void)parse_command(line);
}

const std::vector<std::string> &mutation_pool() {
    // Real messages + adversarial shapes are the mutation base set; random and
    // raw inputs are generated separately.
    static const std::vector<std::string> pool = [] {
        std::vector<std::string> p = kCorpus;
        p.insert(p.end(), kMustReject.begin(), kMustReject.end());
        return p;
    }();
    return pool;
}

struct Mutator {
    explicit Mutator(std::uint32_t seed) : rng(seed) {}

    std::mt19937 rng; // std::mt19937 is deterministic across platforms

    // 1..6 byte-level edits drawn from the Goblins-style mutation set.
    std::string mutate(const std::string &base) {
        std::string out = base;
        const int ops = 1 + static_cast<int>(rng() % 6);
        for (int op = 0; op < ops; ++op) {
            switch (rng() % 8) {
            case 0: // single byte flip
                if (!out.empty())
                    out[rng() % out.size()] ^= static_cast<char>(1u << (rng() % 8));
                break;
            case 1: // insert random byte
                out.insert(out.begin() + static_cast<std::ptrdiff_t>(rng() % (out.size() + 1)),
                           static_cast<char>(rng() & 0xFF));
                break;
            case 2: // delete a small chunk
                if (out.size() > 1) {
                    const std::size_t at = rng() % out.size();
                    const std::size_t len = 1 + rng() % std::min<std::size_t>(out.size() - at, 8);
                    out.erase(at, len);
                }
                break;
            case 3: // truncate (often leaves the JSON unterminated)
                if (!out.empty())
                    out.resize(rng() % (out.size() + 1));
                break;
            case 4: // append garbage (oversize / trailing junk)
                out.append(1 + rng() % 8, static_cast<char>(rng() & 0xFF));
                break;
            case 5: // control bytes (incl. \n and NUL breaking naive framing)
                out.insert(out.begin() + static_cast<std::ptrdiff_t>(rng() % (out.size() + 1)),
                           static_cast<char>(rng() % 32));
                break;
            case 6: // duplicate a slice
                if (out.size() > 2) {
                    const std::size_t at = rng() % (out.size() - 1);
                    const std::size_t len = 1 + rng() % std::min<std::size_t>(out.size() - at, 16);
                    out.insert(out.begin() + static_cast<std::ptrdiff_t>(rng() % (out.size() + 1)),
                               out.begin() + static_cast<std::ptrdiff_t>(at),
                               out.begin() + static_cast<std::ptrdiff_t>(at + len));
                }
                break;
            case 7: // overwrite a range with a repeating character
                if (out.size() > 1) {
                    const std::size_t at = rng() % (out.size() - 1);
                    const std::size_t len = 1 + rng() % std::min<std::size_t>(out.size() - at, 16);
                    std::fill_n(out.begin() + static_cast<std::ptrdiff_t>(at), len, 'x');
                }
                break;
            default:
                break;
            }
        }
        return out;
    }

    // Raw generated bytes: random lengths selected to cover the size-capped
    // path (REQ-O-005): 0, tiny, mid, and > kMaxLineBytes.
    std::string random_line() {
        constexpr std::size_t kLens[] = {0, 1, 2, 3, 8, 32, 128, 512, kMaxLineBytes, kMaxLineBytes + 8};
        const std::size_t len = kLens[rng() % (sizeof(kLens) / sizeof(kLens[0]))];
        std::string out;
        out.reserve(len);
        for (std::size_t i = 0; i < len; ++i)
            out.push_back(static_cast<char>(rng() & 0xFF));
        return out;
    }
};

bool unexpectedly_parsed(const std::string &line) {
    return parse_command(line).has_value();
}

int standalone_driver() {
    Mutator mut{kSeed};

    // 1. Bounded deterministic iteration over generated + mutated inputs.
    for (std::size_t i = 0; i < kIterations; ++i) {
        std::string input;
        if (mut.rng() % 4 == 0) {
            input = mut.random_line();
        } else {
            const std::vector<std::string> &pool = mutation_pool();
            input = mut.mutate(pool[mut.rng() % pool.size()]);
        }
        try {
            run_input(reinterpret_cast<const std::uint8_t *>(input.data()), input.size());
        } catch (const std::exception &e) {
            std::printf("[FAIL] fuzz_overlay_protocol: iteration %zu threw: %s\n", i, e.what());
            return 1;
        } catch (...) {
            std::printf("[FAIL] fuzz_overlay_protocol: iteration %zu threw (unknown exception)\n", i);
            return 1;
        }
    }

    // 2. The real corpus must never blow up the parser.
    for (const std::string &line : kCorpus) {
        try {
            run_input(reinterpret_cast<const std::uint8_t *>(line.data()), line.size());
        } catch (...) {
            std::printf("[FAIL] fuzz_overlay_protocol: corpus input threw\n");
            return 1;
        }
    }

    // 3. REQ-O-005 one-line-drop contract: every structured bad line -> nullopt.
    for (const std::string &line : kMustReject) {
        if (unexpectedly_parsed(line)) {
            std::printf("[FAIL] fuzz_overlay_protocol: must-reject line parsed: %s\n", line.c_str());
            return 1;
        }
    }

    // 4. Oversized / unterminated variants of a would-be-valid command -> nullopt.
    std::string oversized = "{\"v\":1,\"type\":\"apply\",\"pad\":\"";
    oversized.append(kMaxLineBytes, 'x');
    oversized += "\"}";
    const std::string unterminated = oversized.substr(0, kMaxLineBytes + 1);
    const std::string *const bad_inputs[] = {&oversized, &unterminated};
    for (const std::string *bad : bad_inputs) {
        run_input(reinterpret_cast<const std::uint8_t *>(bad->data()), bad->size());
        if (unexpectedly_parsed(*bad)) {
            std::printf("[FAIL] fuzz_overlay_protocol: oversized/unterminated line parsed\n");
            return 1;
        }
    }

    std::printf("[PASS] fuzz_overlay_protocol: %zu iterations + corpus + reject contract\n", kIterations);
    return 0;
}

} // namespace

// libFuzzer entry (future -fsanitize=fuzzer builds). CI uses the deterministic
// standalone main() below instead; MRU_FUZZ_NO_MAIN selects the fuzzer's main.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    try {
        run_input(data, size);
    } catch (...) {
        // A throw from the parser is a bug in the same sense as a crash.
        std::fputs("fuzz_overlay_protocol: LLVMFuzzerTestOneInput threw\n", stderr);
        std::abort();
    }
    return 0;
}

#ifndef MRU_FUZZ_NO_MAIN
int main() {
    return standalone_driver();
}
#endif