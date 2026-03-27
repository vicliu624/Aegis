#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
import struct
import sys

LV_COLOR_FORMATS = {
    "LV_COLOR_FORMAT_RGB565A8": 0x14,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Pack an LVGL generated image asset cpp into an Aegis app icon binary."
    )
    parser.add_argument("--asset-cpp", required=True, help="Path to generated LVGL asset cpp")
    parser.add_argument(
        "--symbol",
        help="Optional LVGL image symbol name to extract when the cpp contains multiple images",
    )
    parser.add_argument("--output", required=True, help="Path to output icon.bin")
    return parser.parse_args()


def extract(pattern: str, text: str, label: str) -> str:
    match = re.search(pattern, text, flags=re.MULTILINE | re.DOTALL)
    if not match:
        raise ValueError(f"unable to locate {label}")
    return match.group(1)


def build_symbol_pattern(symbol: str, suffix: str) -> str:
    escaped = re.escape(symbol)
    return rf"{escaped}{suffix}"


def main() -> int:
    args = parse_args()
    asset_cpp = pathlib.Path(args.asset_cpp)
    output = pathlib.Path(args.output)
    text = asset_cpp.read_text(encoding="utf-8")

    if args.symbol:
        data_pattern = build_symbol_pattern(
            args.symbol, r"_data\[\]\s*=\s*\{(.*?)\};"
        )
        image_pattern = build_symbol_pattern(
            args.symbol, r"\s*=\s*\{\s*\.header\s*=\s*\{(.*?)\}\s*,\s*\.data_size\s*=\s*([0-9]+)"
        )
        data_block = extract(
            rf"static const uint8_t\s+{data_pattern}",
            text,
            f"image data for {args.symbol}",
        )
        image_header = extract(
            rf"const lv_image_dsc_t\s+{image_pattern}",
            text,
            f"image descriptor for {args.symbol}",
        )
        data_size_text = extract(
            rf"const lv_image_dsc_t\s+{re.escape(args.symbol)}\s*=\s*\{{.*?\.data_size\s*=\s*([0-9]+)",
            text,
            f"data size for {args.symbol}",
        )
    else:
        data_block = extract(
            r"static const uint8_t .*?\[\]\s*=\s*\{(.*?)\};", text, "image data"
        )
        image_header = extract(
            r"const lv_image_dsc_t .*?\s*=\s*\{\s*\.header\s*=\s*\{(.*?)\}\s*,\s*\.data_size",
            text,
            "image descriptor",
        )
        data_size_text = extract(r"\.data_size\s*=\s*([0-9]+)", text, "data size")

    data = bytes(int(token, 16) for token in re.findall(r"0x([0-9A-Fa-f]{2})", data_block))

    color_format_token = extract(
        r"\.cf\s*=\s*([A-Za-z0-9_]+)", image_header, "color format"
    ).strip()
    if color_format_token in LV_COLOR_FORMATS:
        color_format = LV_COLOR_FORMATS[color_format_token]
    else:
        color_format = int(color_format_token, 0)
    width = int(extract(r"\.w\s*=\s*([0-9]+)", image_header, "width"))
    height = int(extract(r"\.h\s*=\s*([0-9]+)", image_header, "height"))
    stride = int(extract(r"\.stride\s*=\s*([0-9]+)", image_header, "stride"))
    data_size = int(data_size_text)

    if data_size != len(data):
        raise ValueError(
            f"data_size mismatch: header says {data_size}, extracted {len(data)} bytes"
        )

    magic = 0x314E4349  # ICN1
    header = struct.pack("<IHHHHI", magic, width, height, color_format, stride, data_size)
    output.write_bytes(header + data)
    print(f"packed {asset_cpp} -> {output} ({width}x{height}, {data_size} bytes)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
