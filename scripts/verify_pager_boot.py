#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Capture pager boot logs and verify expected board-coordination markers."
    )
    parser.add_argument("--port", required=True, help="Serial port such as COM3 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud. Default: 115200")
    parser.add_argument("--duration", type=float, default=20.0, help="Capture duration in seconds. Default: 20")
    args = parser.parse_args()

    try:
        import serial  # type: ignore
    except Exception as exc:  # pragma: no cover - environment specific
        print(f"[aegis-verify] error: pyserial import failed: {exc}", file=sys.stderr)
        return 2

    expected = [
        "coordination acquire coordinator=pager-board-control owner=expander",
        "coordination op coordinator=pager-board-control owner=expander op=expander.enable_lines rc=0",
        "coordination acquire coordinator=pager-board-control owner=storage-detect",
        "coordination op coordinator=pager-board-control owner=keyboard op=keyboard.probe rc=0",
        "pager runtime complete devices=ready expander=ready keyboard-probe=ready transfer-coordination=ready board-control=ready",
    ]
    found = {needle: False for needle in expected}

    print(f"[aegis-verify] opening {args.port} at {args.baud}", flush=True)
    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        ser.dtr = False
        ser.rts = True
        time.sleep(0.25)
        ser.reset_input_buffer()
        ser.dtr = True
        ser.rts = False

        deadline = time.time() + args.duration
        while time.time() < deadline:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").rstrip()
            for needle in expected:
                if needle in line:
                    found[needle] = True
                    print(f"[aegis-verify] matched: {needle}", flush=True)

    missing = [needle for needle, ok in found.items() if not ok]
    if missing:
        print("[aegis-verify] boot verification failed; missing markers:", file=sys.stderr)
        for needle in missing:
            print(f"  - {needle}", file=sys.stderr)
        return 1

    print("[aegis-verify] pager boot verification passed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
