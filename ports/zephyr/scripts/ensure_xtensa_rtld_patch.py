#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import sys

SUPPORTED_VERSION = (4, 3, 0)
VERSION_FILE_NAME = "VERSION"

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
    parser.add_argument(
        "--allow-unsupported-version",
        action="store_true",
        help="Permit check/apply against a Zephyr version other than the validated one.",
    )
    return parser.parse_args()


def parse_version(zephyr_base: pathlib.Path) -> tuple[int, int, int]:
    version_file = zephyr_base / VERSION_FILE_NAME
    if not version_file.exists():
        raise FileNotFoundError(version_file)

    values: dict[str, int] = {}
    for raw_line in version_file.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        if key in {"VERSION_MAJOR", "VERSION_MINOR", "PATCHLEVEL"}:
            values[key] = int(value or "0")

    return (
        values.get("VERSION_MAJOR", -1),
        values.get("VERSION_MINOR", -1),
        values.get("PATCHLEVEL", -1),
    )


def format_version(version: tuple[int, int, int]) -> str:
    return ".".join(str(part) for part in version)


def main() -> int:
    args = parse_args()
    zephyr_base = pathlib.Path(args.zephyr_base)
    target = zephyr_base / "arch" / "xtensa" / "core" / "elf.c"

    try:
        version = parse_version(zephyr_base)
    except FileNotFoundError as exc:
        print(f"[aegis] zephyr version file not found: {exc}", file=sys.stderr)
        return 4
    except ValueError as exc:
        print(f"[aegis] unable to parse zephyr version: {exc}", file=sys.stderr)
        return 5

    if version != SUPPORTED_VERSION and not args.allow_unsupported_version:
        print(
            "[aegis] xtensa RTLD patch helper is only validated against Zephyr "
            f"{format_version(SUPPORTED_VERSION)}, but found {format_version(version)} in {zephyr_base}. "
            "Re-run with --allow-unsupported-version only after re-validating the loader source layout.",
            file=sys.stderr,
        )
        return 6

    if not target.exists():
        print(f"[aegis] xtensa patch target not found: {target}", file=sys.stderr)
        return 2

    original = target.read_text(encoding="utf-8")

    if PATCH_SENTINEL in original:
        print(
            "[aegis] xtensa RTLD patch already present in "
            f"{target} for Zephyr {format_version(version)}"
        )
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
    print(
        "[aegis] applied xtensa RTLD no-op patch to "
        f"{target} for Zephyr {format_version(version)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
