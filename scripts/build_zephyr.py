#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import pathlib
import shlex
import shutil
import subprocess
import sys
from dataclasses import dataclass
from typing import Sequence


DEFAULT_BOARD = "esp32s3_devkitc/esp32s3/procpu"
DEFAULT_OVERLAY = "boards/lilygo_tlora_pager_sx1262.overlay"
DEFAULT_BUILD_DIR = "build-zephyr"
DEFAULT_FLASH_BAUD = "921600"
DEFAULT_FLASH_CHIP = "esp32s3"


@dataclass(frozen=True)
class Paths:
    repo_root: pathlib.Path
    zephyr_source: pathlib.Path
    linux_check_script: pathlib.Path


def shell_join(command: Sequence[str]) -> str:
    if os.name == "nt":
        return subprocess.list2cmdline(list(command))
    return shlex.join(command)


def run(command: Sequence[str], cwd: pathlib.Path) -> None:
    print(f"[aegis-build] cwd={cwd}", flush=True)
    print(f"[aegis-build] cmd={shell_join(command)}", flush=True)
    subprocess.run(list(command), cwd=str(cwd), check=True)


def run_passthrough(command: Sequence[str], cwd: pathlib.Path) -> int:
    print(f"[aegis-build] cwd={cwd}", flush=True)
    print(f"[aegis-build] cmd={shell_join(command)}", flush=True)
    result = subprocess.run(list(command), cwd=str(cwd), check=False)
    return result.returncode


def resolve_paths() -> Paths:
    repo_root = pathlib.Path(__file__).resolve().parents[1]
    return Paths(
        repo_root=repo_root,
        zephyr_source=repo_root / "ports" / "zephyr",
        linux_check_script=repo_root / "scripts" / "check_linux_env.py",
    )


def default_cmake_generator() -> str:
    if os.name == "nt" and shutil.which("ninja"):
        return "Ninja"
    return ""


def add_common_arguments(parser: argparse.ArgumentParser) -> None:
    default_generator = default_cmake_generator()
    parser.add_argument(
        "--build-dir",
        default=DEFAULT_BUILD_DIR,
        help=f"Build directory relative to the repository root. Default: {DEFAULT_BUILD_DIR}",
    )
    parser.add_argument(
        "--generator",
        default=default_generator,
        help=f"CMake generator. Default: {default_generator or 'cmake default'}",
    )
    parser.add_argument(
        "--board",
        default=DEFAULT_BOARD,
        help=f"Zephyr board target. Default: {DEFAULT_BOARD}",
    )
    parser.add_argument(
        "--overlay",
        default=DEFAULT_OVERLAY,
        help=f"Devicetree overlay relative to ports/zephyr. Default: {DEFAULT_OVERLAY}",
    )
    parser.add_argument(
        "--compiled-fallback",
        choices=("on", "off"),
        default="off",
        help="Enable or disable compiled app fallback. Default: off",
    )
    parser.add_argument(
        "--app-modules",
        choices=("on", "off"),
        default="on",
        help="Enable or disable Zephyr LLEXT app module packaging. Default: on",
    )
    parser.add_argument(
        "--selftest-app",
        default="demo_hello",
        help="Boot self-test app id. Use an empty string to disable. Default: demo_hello",
    )
    parser.add_argument(
        "--port",
        default="",
        help="Serial port for flash targets, such as COM7 or /dev/ttyACM0",
    )
    parser.add_argument(
        "--flash-baud",
        default=DEFAULT_FLASH_BAUD,
        help=f"Flash baud passed into Zephyr appfs helper. Default: {DEFAULT_FLASH_BAUD}",
    )
    parser.add_argument(
        "--flash-chip",
        default=DEFAULT_FLASH_CHIP,
        help=f"ESP flash chip family for appfs helper. Default: {DEFAULT_FLASH_CHIP}",
    )
    parser.add_argument(
        "--auto-apply-local-patches",
        choices=("on", "off"),
        default="on",
        help="Whether configure may auto-apply required local Zephyr source patches. Default: on",
    )
    parser.add_argument(
        "--allow-unsupported-local-patch-version",
        choices=("on", "off"),
        default="off",
        help="Allow patch helpers to run against an unvalidated Zephyr version. Default: off",
    )
    parser.add_argument(
        "--parallel",
        type=int,
        default=0,
        help="Parallel build jobs for cmake --build. Default: tool default",
    )


def bool_to_cmake(value: str) -> str:
    return "ON" if value.lower() == "on" else "OFF"


def absolute_build_dir(paths: Paths, build_dir: str) -> pathlib.Path:
    build_path = pathlib.Path(build_dir)
    if build_path.is_absolute():
        return build_path
    return paths.repo_root / build_path


def configure_command(args: argparse.Namespace, paths: Paths) -> list[str]:
    build_dir = absolute_build_dir(paths, args.build_dir)
    command = [
        "cmake",
        "-S",
        str(paths.zephyr_source),
        "-B",
        str(build_dir),
    ]
    if args.generator:
        command.extend(["-G", args.generator])
    command.extend(
        [
            f"-DBOARD={args.board}",
            f"-DDTC_OVERLAY_FILE={args.overlay}",
            f"-DAEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK={bool_to_cmake(args.compiled_fallback)}",
            f"-DAEGIS_ZEPHYR_ENABLE_APP_MODULES={bool_to_cmake(args.app_modules)}",
            f"-DAEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID={args.selftest_app}",
            f"-DAEGIS_ZEPHYR_AUTO_APPLY_LOCAL_PATCHES={bool_to_cmake(args.auto_apply_local_patches)}",
            (
                "-DAEGIS_ZEPHYR_ALLOW_UNSUPPORTED_LOCAL_PATCH_VERSION="
                f"{bool_to_cmake(args.allow_unsupported_local_patch_version)}"
            ),
            f"-DAEGIS_ZEPHYR_FLASH_BAUD={args.flash_baud}",
            f"-DAEGIS_ZEPHYR_FLASH_CHIP={args.flash_chip}",
        ]
    )
    if args.port:
        command.append(f"-DAEGIS_ZEPHYR_FLASH_PORT={args.port}")
    return command


def build_command(args: argparse.Namespace, paths: Paths, target: str) -> list[str]:
    build_dir = absolute_build_dir(paths, args.build_dir)
    command = ["cmake", "--build", str(build_dir), "--target", target]
    if args.parallel and args.parallel > 0:
        command.extend(["--parallel", str(args.parallel)])
    return command


def ensure_configured(args: argparse.Namespace, paths: Paths) -> None:
    build_dir = absolute_build_dir(paths, args.build_dir)
    cache_path = build_dir / "CMakeCache.txt"
    if cache_path.exists():
        return
    run(configure_command(args, paths), paths.repo_root)


def command_configure(args: argparse.Namespace, paths: Paths) -> None:
    run(configure_command(args, paths), paths.repo_root)


def command_build_firmware(args: argparse.Namespace, paths: Paths) -> None:
    ensure_configured(args, paths)
    run(build_command(args, paths, "app"), paths.repo_root)


def command_flash_firmware(args: argparse.Namespace, paths: Paths) -> None:
    ensure_configured(args, paths)
    run(build_command(args, paths, "flash"), paths.repo_root)


def command_build_appfs(args: argparse.Namespace, paths: Paths) -> None:
    ensure_configured(args, paths)
    run(build_command(args, paths, "aegis_zephyr_appfs_image"), paths.repo_root)


def command_flash_appfs(args: argparse.Namespace, paths: Paths) -> None:
    ensure_configured(args, paths)
    if not args.port:
        raise SystemExit("--port is required for flash-appfs")
    run(build_command(args, paths, "aegis_zephyr_flash_appfs"), paths.repo_root)


def command_build_all(args: argparse.Namespace, paths: Paths) -> None:
    command_configure(args, paths)
    command_build_firmware(args, paths)
    command_build_appfs(args, paths)


def command_flash_all(args: argparse.Namespace, paths: Paths) -> None:
    if not args.port:
        raise SystemExit("--port is required for flash-all")
    command_build_all(args, paths)
    command_flash_firmware(args, paths)
    command_flash_appfs(args, paths)


def command_print_plan(args: argparse.Namespace, paths: Paths) -> None:
    commands = [
        ("configure", configure_command(args, paths)),
        ("build-firmware", build_command(args, paths, "app")),
        ("build-appfs", build_command(args, paths, "aegis_zephyr_appfs_image")),
    ]
    if args.port:
        commands.extend(
            [
                ("flash-firmware", build_command(args, paths, "flash")),
                ("flash-appfs", build_command(args, paths, "aegis_zephyr_flash_appfs")),
            ]
        )

    print("[aegis-build] plan")
    for label, command in commands:
        print(f"{label}: {shell_join(command)}")


def command_check_linux_env(args: argparse.Namespace, paths: Paths) -> None:
    del args
    exit_code = run_passthrough([sys.executable, str(paths.linux_check_script)], paths.repo_root)
    if exit_code != 0:
        raise SystemExit(exit_code)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Cross-platform build and flash driver for the Aegis Zephyr target."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    commands = {
        "check-linux-env": command_check_linux_env,
        "configure": command_configure,
        "build-firmware": command_build_firmware,
        "flash-firmware": command_flash_firmware,
        "build-appfs": command_build_appfs,
        "flash-appfs": command_flash_appfs,
        "build-all": command_build_all,
        "flash-all": command_flash_all,
        "print-plan": command_print_plan,
    }

    for name in commands:
        subparser = subparsers.add_parser(name)
        if name != "check-linux-env":
            add_common_arguments(subparser)
        subparser.set_defaults(handler=commands[name])

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    paths = resolve_paths()
    args.handler(args, paths)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
