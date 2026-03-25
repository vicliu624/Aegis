#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import sys


PATCH_ANCHOR = """\tcase R_XTENSA_32:
\t\t/* Used for both LOCAL and GLOBAL bindings */
\t\t*got_entry += addr;
\t\tbreak;
\tcase R_XTENSA_SLOT0_OP:
"""

PATCH_REPLACEMENT = """\tcase R_XTENSA_32:
\t\t/* Used for both LOCAL and GLOBAL bindings */
\t\t*got_entry += addr;
\t\tbreak;
\tcase R_XTENSA_RTLD:
\t\t/*
\t\t * These entries are emitted in shared Xtensa objects alongside
\t\t * the PLT veneer and do not require an additional relocation
\t\t * write at load time here. Treat them as an intentional no-op
\t\t * instead of reporting them as unsupported.
\t\t */
\t\tbreak;
\tcase R_XTENSA_SLOT0_OP:
"""

PATCH_SENTINEL = "case R_XTENSA_RTLD:"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Verify or apply the local Zephyr Xtensa R_XTENSA_RTLD no-op patch."
    )
    parser.add_argument("--zephyr-base", required=True, help="Path to ZEPHYR_BASE")
    parser.add_argument(
        "--mode",
        choices=("check", "apply"),
        default="check",
        help="Only verify the patch or apply it if missing.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    zephyr_base = pathlib.Path(args.zephyr_base)
    target = zephyr_base / "arch" / "xtensa" / "core" / "elf.c"

    if not target.exists():
        print(f"[aegis] xtensa patch target not found: {target}", file=sys.stderr)
        return 2

    original = target.read_text(encoding="utf-8")

    if PATCH_SENTINEL in original:
        print(f"[aegis] xtensa RTLD patch already present in {target}")
        return 0

    if PATCH_ANCHOR not in original:
        print(
            "[aegis] unable to find expected Xtensa relocation block in "
            f"{target}; upstream layout may have changed",
            file=sys.stderr,
        )
        return 3

    if args.mode == "check":
        print(
            "[aegis] xtensa RTLD patch missing in "
            f"{target}; reconfigure with auto-apply enabled or run the helper in apply mode",
            file=sys.stderr,
        )
        return 1

    updated = original.replace(PATCH_ANCHOR, PATCH_REPLACEMENT, 1)
    target.write_text(updated, encoding="utf-8")
    print(f"[aegis] applied xtensa RTLD no-op patch to {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
