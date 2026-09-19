#!/usr/bin/env python3
"""Reference external overlay for mru-switcher (SPEC §12 Appendix B, ADR-018).

Connects to the plugin's AF_UNIX stream socket, prints the session/selection
events it receives, and lets you drive the session from the terminal:

    s <n>   select window at index n
    a       apply the current selection
    c       cancel the session
    q       quit

Usage:
    tools/overlay_stub.py [/path/to/socket]

The socket path must match `plugin:mru-switcher:external_socket` and be
non-empty; start this stub, then trigger `mru:cycle` in the compositor.
"""

from __future__ import annotations

import json
import socket
import sys
import threading
import time

VERSION = 1


def encode(msg_type: str, **fields: object) -> bytes:
    payload = {"v": VERSION, "type": msg_type, **fields}
    return (json.dumps(payload, separators=(",", ":")) + "\n").encode()


def humanize(msg: dict) -> str:
    kind = msg.get("type", "?")
    if kind == "session_start":
        lines = [f"session_start: {len(msg.get('windows', []))} window(s), index={msg.get('index')}"]
        for i, window in enumerate(msg.get("windows", [])):
            marker = "*" if i == msg.get("index") else " "
            lines.append(f"  {marker} [{i}] {window.get('class') or '?'}  {window.get('title') or ''}")
        return "\n".join(lines)
    if kind == "selection":
        return f"selection: index={msg.get('index')}"
    if kind == "session_end":
        return f"session_end: reason={msg.get('reason')}"
    return f"unknown: {msg}"


def reader_loop(sock: socket.socket) -> None:
    buffer = b""
    while True:
        try:
            chunk = sock.recv(4096)
        except OSError:
            break
        if not chunk:
            print("[stub] plugin closed the connection")
            break
        buffer += chunk
        while b"\n" in buffer:
            line, _, buffer = buffer.partition(b"\n")
            if not line.strip():
                continue
            try:
                msg = json.loads(line)
            except json.JSONDecodeError:
                print(f"[stub] ignoring malformed line: {line!r}")
                continue
            print(humanize(msg))
            print("[stub] command (s <n> / a / c / q): ", end="", flush=True)


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else "/tmp/mru-switcher-overlay.sock"

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    for attempt in range(50):  # the plugin may not have started the socket yet
        try:
            sock.connect(path)
            break
        except OSError:
            if attempt == 49:
                print(f"[stub] cannot connect to {path}: is ui=external and the path set?", file=sys.stderr)
                return 1
            time.sleep(0.1)

    print(f"[stub] connected to {path}")
    threading.Thread(target=reader_loop, args=(sock,), daemon=True).start()
    print("[stub] command (s <n> / a / c / q): ", end="", flush=True)

    for raw in sys.stdin:
        command = raw.strip().lower()
        if not command:
            continue
        try:
            if command in ("q", "quit", "exit"):
                break
            if command in ("a", "apply"):
                sock.sendall(encode("apply"))
            elif command in ("c", "cancel"):
                sock.sendall(encode("cancel"))
            elif command.startswith("s "):
                index = int(command.split(None, 1)[1])
                if index < 0:
                    raise ValueError("index must be >= 0")
                sock.sendall(encode("select", index=index))
            else:
                print("[stub] unknown command; use s <n>, a, c, q")
        except ValueError as exc:
            print(f"[stub] bad command: {exc}")
        print("[stub] command (s <n> / a / c / q): ", end="", flush=True)

    sock.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
