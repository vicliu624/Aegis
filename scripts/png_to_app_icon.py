#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

from PIL import Image


LV_COLOR_FORMAT_RGB565A8 = 0x14
APP_ICON_MAGIC = 0x314E4349  # ICN1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a PNG into an Aegis app icon binary (ICN1 / RGB565A8)."
    )
    parser.add_argument("--input", required=True, help="Path to source PNG")
    parser.add_argument("--output", required=True, help="Path to output icon.bin")
    parser.add_argument(
        "--size",
        type=int,
        default=48,
        help="Square output size in pixels. Defaults to 48.",
    )
    return parser.parse_args()


def rgb888_to_rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def main() -> int:
    args = parse_args()
    input_path = pathlib.Path(args.input)
    output_path = pathlib.Path(args.output)
    size = args.size

    if size <= 0:
        raise ValueError("size must be positive")

    image = Image.open(input_path).convert("RGBA")
    if image.size != (size, size):
        image = image.resize((size, size), Image.LANCZOS)

    pixels = list(image.getdata())
    rgb565 = bytearray()
    alpha = bytearray()
    for red, green, blue, alpha_value in pixels:
        rgb565_value = rgb888_to_rgb565(red, green, blue)
        rgb565.extend(struct.pack("<H", rgb565_value))
        alpha.append(alpha_value)

    payload = bytes(rgb565 + alpha)
    stride = size * 2
    header = struct.pack(
        "<IHHHHI",
        APP_ICON_MAGIC,
        size,
        size,
        LV_COLOR_FORMAT_RGB565A8,
        stride,
        len(payload),
    )
    output_path.write_bytes(header + payload)
    print(
        f"packed {input_path} -> {output_path} "
        f"({size}x{size}, rgb565={len(rgb565)} bytes, alpha={len(alpha)} bytes)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
