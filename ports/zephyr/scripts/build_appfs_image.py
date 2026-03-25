#!/usr/bin/env python3

from __future__ import annotations

import argparse
import importlib
import json
import os
import pathlib
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build an Aegis appfs image from a deploy tree.")
    parser.add_argument("--zephyr-dts", required=True)
    parser.add_argument("--deploy-root", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--layout-json", required=True)
    parser.add_argument("--backend", choices=("auto", "mklittlefs", "python"), default="auto")
    parser.add_argument("--mklittlefs", default="")
    parser.add_argument("--tool-cache-dir", default="")
    parser.add_argument("--allow-auto-fetch", action="store_true")
    parser.add_argument("--block-size", type=int, default=4096)
    parser.add_argument("--page-size", type=int, default=256)
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


def collect_apps(deploy_root: pathlib.Path) -> list[dict[str, object]]:
    apps: list[dict[str, object]] = []
    if not deploy_root.exists():
        return apps

    for app_dir in sorted(path for path in deploy_root.iterdir() if path.is_dir()):
        files = []
        total_size = 0
        for child in sorted(app_dir.iterdir()):
            if child.is_file():
                size = child.stat().st_size
                total_size += size
                files.append({"name": child.name, "size": size})
        apps.append(
            {
                "app_id": app_dir.name,
                "path": app_dir.as_posix(),
                "files": files,
                "package_size": total_size,
            }
        )
    return apps


def write_layout_json(layout_json: pathlib.Path,
                      image_offset: int,
                      image_size: int,
                      apps: list[dict[str, object]]) -> None:
    payload = {
        "filesystem": "littlefs",
        "mount_point": "/lfs",
        "apps_root": "/lfs/apps",
        "storage_partition_offset": image_offset,
        "image_size": image_size,
        "apps": apps,
    }
    layout_json.parent.mkdir(parents=True, exist_ok=True)
    layout_json.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def detect_release_asset() -> str:
    system = platform.system().lower()
    machine = platform.machine().lower()

    if system != "windows":
        return ""
    if machine in {"amd64", "x86_64"}:
        return "x86_64-w64-mingw32"
    if machine in {"x86", "i386", "i686"}:
        return "i686-w64-mingw32"
    return ""


def fetch_mklittlefs(cache_dir: pathlib.Path) -> str:
    cache_dir.mkdir(parents=True, exist_ok=True)
    tool_path = cache_dir / "mklittlefs.exe"
    if tool_path.exists():
        return str(tool_path)

    asset_prefix = detect_release_asset()
    if not asset_prefix:
        raise RuntimeError(
            "automatic mklittlefs fetch is only implemented for Windows x86/x64 hosts. "
            "Set AEGIS_ZEPHYR_APPFS_IMAGE_TOOL explicitly."
        )

    api_url = "https://api.github.com/repos/earlephilhower/mklittlefs/releases/latest"
    request = urllib.request.Request(
        api_url,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "aegis-zephyr-appfs-bootstrap",
        },
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        release = json.load(response)

    asset = next(
        (
            candidate
            for candidate in release.get("assets", [])
            if candidate.get("name", "").startswith(asset_prefix)
            and candidate.get("name", "").endswith(".zip")
        ),
        None,
    )
    if asset is None:
        raise RuntimeError("failed to locate a matching mklittlefs Windows release asset")

    archive_path = cache_dir / asset["name"]
    if not archive_path.exists():
        with urllib.request.urlopen(asset["browser_download_url"], timeout=60) as response:
            archive_path.write_bytes(response.read())

    with tempfile.TemporaryDirectory(prefix="aegis-mklittlefs-") as temp_dir:
        with zipfile.ZipFile(archive_path) as archive:
            archive.extractall(temp_dir)
        extracted = list(pathlib.Path(temp_dir).rglob("mklittlefs*.exe"))
        if not extracted:
            raise RuntimeError("downloaded archive did not contain mklittlefs.exe")
        shutil.copy2(extracted[0], tool_path)

    return str(tool_path)


def resolve_mklittlefs(args: argparse.Namespace) -> str:
    explicit = args.mklittlefs.strip()
    if explicit:
        return explicit

    discovered = shutil.which("mklittlefs")
    if discovered:
        return discovered

    if args.allow_auto_fetch and args.tool_cache_dir:
        return fetch_mklittlefs(pathlib.Path(args.tool_cache_dir))

    raise RuntimeError(
        "mklittlefs was not found. Set AEGIS_ZEPHYR_APPFS_IMAGE_TOOL to a valid executable "
        "or enable the auto-fetch path."
    )


def try_resolve_mklittlefs(args: argparse.Namespace) -> str | None:
    try:
        return resolve_mklittlefs(args)
    except RuntimeError:
        return None


def install_littlefs_python(cache_dir: pathlib.Path) -> pathlib.Path:
    target_dir = cache_dir / "littlefs_python"
    module_dir = target_dir / "littlefs"
    if module_dir.exists():
        return target_dir

    target_dir.mkdir(parents=True, exist_ok=True)
    command = [
        sys.executable,
        "-m",
        "pip",
        "install",
        "--disable-pip-version-check",
        "--target",
        str(target_dir),
        "littlefs-python",
    ]
    subprocess.run(command, check=True, capture_output=True, text=True)
    if not module_dir.exists():
        raise RuntimeError("littlefs-python installation completed but module was not found")
    return target_dir


def resolve_littlefs_python(args: argparse.Namespace):
    try:
        return importlib.import_module("littlefs")
    except ModuleNotFoundError:
        pass

    if not (args.allow_auto_fetch and args.tool_cache_dir):
        raise RuntimeError(
            "littlefs-python is not available. Enable auto-fetch or install it into the active Python environment."
        )

    cache_root = pathlib.Path(args.tool_cache_dir)
    target_dir = install_littlefs_python(cache_root)
    if str(target_dir) not in sys.path:
        sys.path.insert(0, str(target_dir))
    return importlib.import_module("littlefs")


def try_resolve_littlefs_python(args: argparse.Namespace):
    try:
        return resolve_littlefs_python(args)
    except RuntimeError:
        return None


def iter_fs_entries(fs_root: pathlib.Path):
    if not fs_root.exists():
        return

    for current_root, dirnames, filenames in os.walk(fs_root):
        current_path = pathlib.Path(current_root)
        rel_root = current_path.relative_to(fs_root)
        for dirname in sorted(dirnames):
            directory_path = current_path / dirname
            rel_path = directory_path.relative_to(fs_root)
            yield rel_path.as_posix(), True, directory_path
        for filename in sorted(filenames):
            file_path = current_path / filename
            rel_path = file_path.relative_to(fs_root)
            yield rel_path.as_posix(), False, file_path


def build_image_with_littlefs_python(args: argparse.Namespace,
                                     image_size: int,
                                     littlefs=None) -> str:
    if image_size % args.block_size != 0:
        raise RuntimeError("appfs image size must be a multiple of the LittleFS block size")

    if littlefs is None:
        littlefs = resolve_littlefs_python(args)
    deploy_root = pathlib.Path(args.deploy_root)
    fs_root = deploy_root.parent if deploy_root.name == "apps" else deploy_root
    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    block_count = image_size // args.block_size
    fs = littlefs.LittleFS(block_size=args.block_size, block_count=block_count, mount=False)
    fs.format()
    fs.mount()

    for rel_path, is_directory, source_path in iter_fs_entries(fs_root):
        fs_path = "/" + rel_path.replace("\\", "/").strip("/")
        if not fs_path or fs_path == "/":
            continue
        if is_directory:
            fs.makedirs(fs_path, exist_ok=True)
            continue

        parent = pathlib.PurePosixPath(fs_path).parent.as_posix()
        if parent and parent != "." and parent != "/":
            fs.makedirs(parent, exist_ok=True)
        with fs.open(fs_path, "wb") as destination:
            destination.write(source_path.read_bytes())

    fs.unmount()
    output.write_bytes(bytes(fs.context.buffer))
    return "littlefs-python"


def build_image_with_mklittlefs(args: argparse.Namespace,
                                image_size: int,
                                tool: str | None = None) -> str:
    deploy_root = pathlib.Path(args.deploy_root)
    fs_root = deploy_root.parent if deploy_root.name == "apps" else deploy_root
    if not fs_root.exists():
        raise RuntimeError(f"deploy root does not exist: {fs_root}")

    if tool is None:
        tool = resolve_mklittlefs(args)
    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    command = [
        tool,
        "-c",
        str(fs_root),
        "-b",
        str(args.block_size),
        "-p",
        str(args.page_size),
        "-s",
        str(image_size),
        str(output),
    ]
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    tool_output = "\n".join(part for part in [result.stdout, result.stderr] if part)
    lowered = tool_output.lower()
    if "error" in lowered or "no more free space" in lowered:
        raise RuntimeError(f"mklittlefs reported a packaging failure:\n{tool_output.strip()}")
    return "mklittlefs"


def choose_backend(args: argparse.Namespace) -> tuple[str, object | None]:
    if args.backend == "python":
        littlefs = resolve_littlefs_python(args)
        return "python", littlefs

    if args.backend == "mklittlefs":
        tool = resolve_mklittlefs(args)
        return "mklittlefs", tool

    tool = try_resolve_mklittlefs(args)
    if tool:
        return "mklittlefs", tool

    littlefs = try_resolve_littlefs_python(args)
    if littlefs is not None:
        return "python", littlefs

    raise RuntimeError(
        "no appfs backend is available. Install or provide mklittlefs, install littlefs-python, "
        "or enable auto-fetch with a writable tool cache directory."
    )


def build_image(args: argparse.Namespace, image_size: int) -> None:
    deploy_root = pathlib.Path(args.deploy_root)
    fs_root = deploy_root.parent if deploy_root.name == "apps" else deploy_root
    if not fs_root.exists():
        raise RuntimeError(f"deploy root does not exist: {fs_root}")

    backend, backend_handle = choose_backend(args)
    if backend == "python":
        used_backend = build_image_with_littlefs_python(args, image_size, backend_handle)
        print(f"[aegis-appfs] image backend={used_backend}")
        return

    used_backend = build_image_with_mklittlefs(args, image_size, backend_handle)
    print(f"[aegis-appfs] image backend={used_backend}")


def main() -> int:
    args = parse_args()
    zephyr_dts = pathlib.Path(args.zephyr_dts)
    deploy_root = pathlib.Path(args.deploy_root)
    layout_json = pathlib.Path(args.layout_json)

    image_offset, image_size = parse_storage_partition(zephyr_dts)
    apps = collect_apps(deploy_root)
    write_layout_json(layout_json, image_offset, image_size, apps)

    try:
        build_image(args, image_size)
    except Exception as exc:  # noqa: BLE001
        print(f"[aegis-appfs] {exc}", file=sys.stderr)
        return 2

    print(f"[aegis-appfs] built image_size={image_size} output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
