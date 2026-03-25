#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Capture T-Deck boot logs and verify expected board bring-up markers."
    )
    parser.add_argument("--port", required=True, help="Serial port such as COM7 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud. Default: 115200")
    parser.add_argument("--duration", type=float, default=20.0, help="Capture duration in seconds. Default: 20")
    args = parser.parse_args()

    try:
        import serial  # type: ignore
    except Exception as exc:  # pragma: no cover - environment specific
        print(f"[aegis-verify] error: pyserial import failed: {exc}", file=sys.stderr)
        return 2

    expected = [
        "initialize zephyr_tdeck_sx1262",
        "tdeck runtime complete devices=ready",
        "keyboard=ready",
        "touch=ready",
        "battery=ready",
        "tdeck input backend trackball=ready keyboard=ready",
        "board package runtime ready package=zephyr_tdeck_sx1262",
        "interactive input enabled",
        "AEGIS HEARTBEAT 1",
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
                if needle in line and not found[needle]:
                    found[needle] = True
                    print(f"[aegis-verify] matched: {needle}", flush=True)

    missing = [needle for needle, ok in found.items() if not ok]
    if missing:
        print("[aegis-verify] tdeck boot verification failed; missing markers:", file=sys.stderr)
        for needle in missing:
            print(f"  - {needle}", file=sys.stderr)
        return 1

    print("[aegis-verify] tdeck boot verification passed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
