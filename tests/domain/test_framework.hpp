#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace mru::test {

struct Case {
  const char* name;
  void (*fn)();
};

inline std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}

inline int& failures() {
  static int count = 0;
  return count;
}

template <typename T>
std::string fmt(const T& v) {
  if constexpr (std::is_arithmetic_v<T>)
    return std::to_string(v);
  else
    return "<value>";
}

int run_all() {
  int total = 0;
  for (const auto& c : registry()) {
    c.fn();
    if (failures() > total) {
      total = failures();
      std::printf("[FAIL] %s\n", c.name);
    } else {
      std::printf("[PASS] %s\n", c.name);
    }
  }
  return failures();
}

} // namespace mru::test

#define TEST(name)                                          \
  static void name();                                       \
  static const bool name##_registered = [] {                \
    mru::test::registry().push_back({#name, &name});        \
    return true;                                            \
  }();                                                      \
  static void name()

#define CHECK(expr)                                        \
  do {                                                     \
    if (!(expr)) {                                         \
      std::printf("[FAIL] %s:%d CHECK(%s)\n", __FILE__,    \
                  __LINE__, #expr);                        \
      ++mru::test::failures();                             \
    }                                                      \
  } while (0)

#define EQ(a, b)                                           \
  do {                                                     \
    const auto& va__ = (a);                                \
    const auto& vb__ = (b);                                \
    if (!(va__ == vb__)) {                                 \
      std::printf("[FAIL] %s:%d EQ(%s, %s): %s != %s\n",   \
                  __FILE__, __LINE__, #a, #b,              \
                  mru::test::fmt(va__).c_str(),            \
                  mru::test::fmt(vb__).c_str());           \
      ++mru::test::failures();                             \
    }                                                      \
  } while (0)
