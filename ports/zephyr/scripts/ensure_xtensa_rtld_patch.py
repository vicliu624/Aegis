#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import sys

SUPPORTED_VERSION = (4, 3, 0)
VERSION_FILE_NAME = "VERSION"

RTLD_PATCH_ANCHOR = """\tcase R_XTENSA_32:
\t\t/* Used for both LOCAL and GLOBAL bindings */
\t\t*got_entry += addr;
\t\tbreak;
\tcase R_XTENSA_SLOT0_OP:
"""

RTLD_PATCH_REPLACEMENT = """\tcase R_XTENSA_32:
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

RTLD_PATCH_SENTINEL = "case R_XTENSA_RTLD:"

FETCHABILITY_PATCH_ANCHOR = """#elif CONFIG_HARVARD && !CONFIG_ARC
/* Unknown if section / region is in instruction memory; warn or compensate */
#define INSTR_FETCHABLE(base_addr, alloc) false
#else /* all non-Harvard architectures */
#define INSTR_FETCHABLE(base_addr, alloc) true
#endif
"""

FETCHABILITY_PATCH_REPLACEMENT = """#elif CONFIG_HARVARD && !CONFIG_ARC
/* Unknown if section / region is in instruction memory; warn or compensate */
#define INSTR_FETCHABLE(base_addr, alloc) false
#elif defined(CONFIG_XTENSA)
/*
 * Xtensa on ESP32-class parts exposes external mapped memory through separate
 * data and instruction windows. A writable ELF buffer that lives in the
 * 0x3c... data window must not be treated as directly fetchable code, or the
 * loader will bind text to the wrong address space and the module will fault
 * on entry. Only true instruction windows may be reused in place.
 */
#define XTENSA_RANGE_FETCHABLE(node_id, base_addr, alloc)                                         \\
\t(((uintptr_t)(base_addr) >= DT_REG_ADDR(node_id)) &&                                        \\
\t ((uintptr_t)(base_addr) + (alloc) <= DT_REG_ADDR(node_id) + DT_REG_SIZE(node_id)))
#define INSTR_FETCHABLE(base_addr, alloc)                                                         \\
\t(XTENSA_RANGE_FETCHABLE(DT_NODELABEL(sram0), base_addr, alloc) ||                          \\
\t XTENSA_RANGE_FETCHABLE(DT_NODELABEL(icache0), base_addr, alloc))
#else /* all non-Harvard architectures */
#define INSTR_FETCHABLE(base_addr, alloc) true
#endif
"""

FETCHABILITY_PATCH_SENTINEL = "XTENSA_RANGE_FETCHABLE"

ALLOC_REUSE_PATCH_ANCHOR = """\t\tif (region->sh_type != SHT_NOBITS) {
\t\t\t/* Region has data in the file, check if peek() is supported */
\t\t\text->mem[mem_idx] = llext_peek(ldr, region->sh_offset);
\t\t\tif (ext->mem[mem_idx]) {
"""

ALLOC_REUSE_PATCH_REPLACEMENT = """\t\tif (region->sh_type != SHT_NOBITS) {
\t\t\t/*
\t\t\t * Xtensa shared-library modules loaded from writable buffers should not
\t\t\t * bind SHF_ALLOC regions directly to the source ELF image. Keeping some
\t\t\t * alloc sections in the 0x3c... data-mapped window while others are
\t\t\t * copied into internal RAM breaks relocation assumptions during link.
\t\t\t * Force a real copy for all alloc regions so the module sees one
\t\t\t * consistent loaded-memory layout.
\t\t\t */
\t\t\tif (IS_ENABLED(CONFIG_XTENSA) && (region->sh_flags & SHF_ALLOC)) {
\t\t\t\text->mem[mem_idx] = NULL;
\t\t\t} else {
\t\t\t\t/* Region has data in the file, check if peek() is supported */
\t\t\t\text->mem[mem_idx] = llext_peek(ldr, region->sh_offset);
\t\t\t}
\t\t\tif (ext->mem[mem_idx]) {
"""

ALLOC_REUSE_PATCH_SENTINEL = "Xtensa shared-library modules loaded from writable buffers should not"


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


def apply_text_patch(
    path: pathlib.Path,
    *,
    sentinel: str,
    anchor: str,
    replacement: str,
    mode: str,
    missing_message: str,
    present_message: str,
    applied_message: str,
) -> int:
    if not path.exists():
        print(f"[aegis] patch target not found: {path}", file=sys.stderr)
        return 2

    original = path.read_text(encoding="utf-8")

    if sentinel in original:
        print(present_message)
        return 0

    if anchor not in original:
        print(missing_message, file=sys.stderr)
        return 3

    if mode == "check":
        print(
            f"[aegis] patch missing in {path}; reconfigure with auto-apply enabled or run the helper in apply mode",
            file=sys.stderr,
        )
        return 1

    updated = original.replace(anchor, replacement, 1)
    path.write_text(updated, encoding="utf-8")
    print(applied_message)
    return 0


def main() -> int:
    args = parse_args()
    zephyr_base = pathlib.Path(args.zephyr_base)
    rtld_target = zephyr_base / "arch" / "xtensa" / "core" / "elf.c"
    fetchability_target = zephyr_base / "subsys" / "llext" / "llext_priv.h"
    alloc_reuse_target = zephyr_base / "subsys" / "llext" / "llext_mem.c"

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

    rtld_result = apply_text_patch(
        rtld_target,
        sentinel=RTLD_PATCH_SENTINEL,
        anchor=RTLD_PATCH_ANCHOR,
        replacement=RTLD_PATCH_REPLACEMENT,
        mode=args.mode,
        missing_message=(
            "[aegis] unable to find expected Xtensa relocation block in "
            f"{rtld_target}; upstream layout may have changed"
        ),
        present_message=(
            "[aegis] xtensa RTLD patch already present in "
            f"{rtld_target} for Zephyr {format_version(version)}"
        ),
        applied_message=(
            "[aegis] applied xtensa RTLD no-op patch to "
            f"{rtld_target} for Zephyr {format_version(version)}"
        ),
    )
    if rtld_result != 0:
        return rtld_result

    fetchability_result = apply_text_patch(
        fetchability_target,
        sentinel=FETCHABILITY_PATCH_SENTINEL,
        anchor=FETCHABILITY_PATCH_ANCHOR,
        replacement=FETCHABILITY_PATCH_REPLACEMENT,
        mode=args.mode,
        missing_message=(
            "[aegis] unable to find expected Xtensa llext fetchability block in "
            f"{fetchability_target}; upstream layout may have changed"
        ),
        present_message=(
            "[aegis] xtensa llext fetchability patch already present in "
            f"{fetchability_target} for Zephyr {format_version(version)}"
        ),
        applied_message=(
            "[aegis] applied xtensa llext fetchability patch to "
            f"{fetchability_target} for Zephyr {format_version(version)}"
        ),
    )
    if fetchability_result != 0:
        return fetchability_result

    alloc_reuse_result = apply_text_patch(
        alloc_reuse_target,
        sentinel=ALLOC_REUSE_PATCH_SENTINEL,
        anchor=ALLOC_REUSE_PATCH_ANCHOR,
        replacement=ALLOC_REUSE_PATCH_REPLACEMENT,
        mode=args.mode,
        missing_message=(
            "[aegis] unable to find expected Xtensa llext alloc-reuse block in "
            f"{alloc_reuse_target}; upstream layout may have changed"
        ),
        present_message=(
            "[aegis] xtensa llext alloc-reuse patch already present in "
            f"{alloc_reuse_target} for Zephyr {format_version(version)}"
        ),
        applied_message=(
            "[aegis] applied xtensa llext alloc-reuse patch to "
            f"{alloc_reuse_target} for Zephyr {format_version(version)}"
        ),
    )
    if alloc_reuse_result != 0:
        return alloc_reuse_result

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
