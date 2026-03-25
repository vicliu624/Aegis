#!/usr/bin/env python3

from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import sys


def run_check(label: str, command: list[str]) -> tuple[bool, str]:
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        output = (result.stdout or result.stderr).strip()
        return True, output.splitlines()[0] if output else "ok"
    except FileNotFoundError:
        return False, "not found"
    except PermissionError as exc:
        return False, f"permission denied: {exc.filename}"
    except subprocess.CalledProcessError as exc:
        details = (exc.stderr or exc.stdout or "").strip()
        return False, details.splitlines()[0] if details else f"exit {exc.returncode}"


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parents[1]
    print("[aegis-linux-check] repo=" + str(repo_root))

    failures = 0

    if sys.platform != "linux":
        print("[aegis-linux-check] warning: this helper is intended for Linux/WSL hosts")

    checks = [
        ("python3", ["python3", "--version"]),
        ("python3-pip", ["python3", "-m", "pip", "--version"]),
        ("cmake", ["cmake", "--version"]),
        ("ninja", ["ninja", "--version"]),
        ("west", ["west", "--version"]),
        ("esptool", ["python3", "-m", "esptool", "version"]),
    ]

    for label, command in checks:
        ok, detail = run_check(label, command)
        status = "ok" if ok else "missing"
        print(f"[aegis-linux-check] {label}: {status} - {detail}")
        if not ok:
            failures += 1

    zephyr_base = os.environ.get("ZEPHYR_BASE", "").strip()
    if zephyr_base:
        exists = pathlib.Path(zephyr_base).exists()
        status = "ok" if exists else "missing"
        print(f"[aegis-linux-check] ZEPHYR_BASE: {status} - {zephyr_base}")
        if not exists:
            failures += 1
    else:
        print("[aegis-linux-check] ZEPHYR_BASE: missing - environment variable is not set")
        failures += 1

    mklittlefs_path = shutil.which("mklittlefs")
    if mklittlefs_path:
        print(f"[aegis-linux-check] mklittlefs: ok - {mklittlefs_path}")
    else:
        print("[aegis-linux-check] mklittlefs: optional - not found, python fallback may still work")

    if failures:
        print(
            "[aegis-linux-check] result: not ready - fix the missing required items above before "
            "running the Zephyr build flow"
        )
        return 1

    print("[aegis-linux-check] result: ready for repository-level Zephyr configure checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
