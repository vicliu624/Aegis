#!/usr/bin/env python3

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Flash Aegis appfs.bin into storage_partition.")
    parser.add_argument("--zephyr-dts", required=True)
    parser.add_argument("--image", required=True)
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--chip", default="esp32s3")
    parser.add_argument("--flash-tool", default="")
    parser.add_argument("--before", default="default_reset")
    parser.add_argument("--after", default="hard_reset")
    return parser.parse_args()


def parse_storage_partition(zephyr_dts: pathlib.Path) -> tuple[int, int]:
    text = zephyr_dts.read_text(encoding="utf-8")
    match = re.search(
        r"storage_partition:\s*partition@[^\{]+\{(?:(?!\};).)*?reg\s*=\s*<\s*(0x[0-9a-fA-F]+|\d+)\s+(0x[0-9a-fA-F]+|\d+)\s*>;",
        text,
        re.DOTALL,
    )
    if not match:
        raise RuntimeError("failed to locate storage_partition offset/size in zephyr.dts")
    return int(match.group(1), 0), int(match.group(2), 0)


def resolve_flash_command(args: argparse.Namespace) -> list[str]:
    if args.flash_tool:
        return [args.flash_tool]
    return [sys.executable, "-m", "esptool"]


def main() -> int:
    args = parse_args()
    if not args.port:
        raise RuntimeError("flash port is required. Set AEGIS_ZEPHYR_FLASH_PORT before invoking appfs flash.")
    image_path = pathlib.Path(args.image)
    if not image_path.exists():
        raise RuntimeError(f"appfs image does not exist: {image_path}")

    offset, size = parse_storage_partition(pathlib.Path(args.zephyr_dts))
    image_size = image_path.stat().st_size
    if image_size > size:
        raise RuntimeError(
            f"appfs image is too large for storage_partition: image={image_size} partition={size}"
        )

    command = resolve_flash_command(args) + [
        "--chip",
        args.chip,
        "--port",
        args.port,
        "--baud",
        str(args.baud),
        "--before",
        args.before,
        "--after",
        args.after,
        "write_flash",
        hex(offset),
        str(image_path),
    ]
    subprocess.run(command, check=True)
    print(
        "[aegis-appfs] flashed "
        f"image={image_path} offset={hex(offset)} size={image_size} port={args.port}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
