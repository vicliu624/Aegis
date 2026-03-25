#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import pathlib
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass


@dataclass(frozen=True)
class CheckResult:
    label: str
    ok: bool
    detail: str
    suggestions: list[str]


@dataclass(frozen=True)
class HostContext:
    platform_name: str
    is_wsl: bool
    python_executable: str
    repo_root: pathlib.Path


def detect_host_context() -> HostContext:
    repo_root = pathlib.Path(__file__).resolve().parents[1]
    is_wsl = sys.platform == "linux" and "microsoft" in platform.release().lower()

    if os.name == "nt":
        platform_name = "windows"
    elif sys.platform == "darwin":
        platform_name = "macos"
    elif is_wsl:
        platform_name = "wsl"
    else:
        platform_name = "linux"

    return HostContext(
        platform_name=platform_name,
        is_wsl=is_wsl,
        python_executable=sys.executable,
        repo_root=repo_root,
    )


def run_check(command: list[str]) -> tuple[bool, str]:
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


def suggestion_commands(context: HostContext, package_name: str) -> list[str]:
    if context.platform_name == "windows":
        if package_name == "python":
            return ["install Python 3 from python.org and ensure `python` is on PATH"]
        if package_name == "cmake":
            return ["install CMake and ensure `cmake` is on PATH"]
        if package_name == "ninja":
            return ["install Ninja and ensure `ninja` is on PATH"]
        if package_name == "west":
            return [f"{context.python_executable} -m pip install west"]
        if package_name == "esptool":
            return [f"{context.python_executable} -m pip install esptool"]
        if package_name == "pip":
            return ["repair or reinstall the active Python environment so `python -m pip` works"]
        return []

    if context.platform_name in {"linux", "wsl"}:
        if package_name == "python":
            return ["sudo apt update && sudo apt install -y python3"]
        if package_name == "pip":
            return ["sudo apt update && sudo apt install -y python3-pip"]
        if package_name == "cmake":
            return ["sudo apt update && sudo apt install -y cmake"]
        if package_name == "ninja":
            return ["sudo apt update && sudo apt install -y ninja-build"]
        if package_name == "west":
            return ["python3 -m pip install west"]
        if package_name == "esptool":
            return ["python3 -m pip install esptool"]
        return []

    if context.platform_name == "macos":
        if package_name == "python":
            return ["install Python 3 and ensure `python3` is available in the active shell"]
        if package_name == "pip":
            return ["repair or reinstall the active Python 3 environment so `python3 -m pip` works"]
        if package_name == "cmake":
            return ["brew install cmake"]
        if package_name == "ninja":
            return ["brew install ninja"]
        if package_name == "west":
            return ["python3 -m pip install west"]
        if package_name == "esptool":
            return ["python3 -m pip install esptool"]
        return []

    return []


def suggestions_for(context: HostContext, label: str, detail: str) -> list[str]:
    suggestions: list[str] = []
    lowered = detail.lower()

    if label == "python":
        suggestions.extend(suggestion_commands(context, "python"))
    elif label == "pip":
        suggestions.extend(suggestion_commands(context, "pip"))
    elif label == "cmake":
        suggestions.extend(suggestion_commands(context, "cmake"))
    elif label == "ninja":
        suggestions.extend(suggestion_commands(context, "ninja"))
    elif label == "west":
        suggestions.extend(suggestion_commands(context, "west"))
    elif label == "esptool":
        suggestions.extend(suggestion_commands(context, "esptool"))
    elif label == "ZEPHYR_BASE":
        if context.platform_name == "windows":
            suggestions.append("set ZEPHYR_BASE to your Zephyr tree before configuring this repository")
            suggestions.append("PowerShell example: $env:ZEPHYR_BASE='C:\\path\\to\\zephyr'")
        else:
            suggestions.append("set ZEPHYR_BASE to your Zephyr tree before configuring this repository")
            suggestions.append("shell example: export ZEPHYR_BASE=/path/to/zephyr")

    if "permission denied" in lowered and context.platform_name in {"linux", "wsl"}:
        if label in {"cmake", "ninja", "west"}:
            suggestions.append("verify the resolved executable is not a broken Windows shim inside WSL")
            suggestions.append(f"check with: command -v {label}")

    return suggestions


def print_check(prefix: str, label: str, ok: bool, detail: str, suggestions: list[str]) -> None:
    status = "ok" if ok else "missing"
    print(f"{prefix} {label}: {status} - {detail}")
    if not ok:
        for suggestion in suggestions:
            print(f"{prefix}   fix: {suggestion}")


def format_next_steps(context: HostContext, failed_results: list[CheckResult]) -> list[str]:
    ordered_steps: list[str] = []
    seen: set[str] = set()

    for result in failed_results:
        for suggestion in result.suggestions:
            if suggestion not in seen:
                seen.add(suggestion)
                ordered_steps.append(suggestion)

    if any(result.label == "ZEPHYR_BASE" for result in failed_results):
        if context.platform_name == "windows":
            suggestion = "after fixing host tools, set ZEPHYR_BASE in the current PowerShell and rerun the host check"
        else:
            suggestion = "after fixing host tools, export ZEPHYR_BASE in the current shell and rerun the host check"
        if suggestion not in seen:
            ordered_steps.append(suggestion)

    rerun_step = "rerun: python scripts/build_zephyr.py check-host-env"
    if context.platform_name in {"linux", "wsl", "macos"}:
        rerun_step = "rerun: python3 scripts/build_zephyr.py check-host-env"
    if rerun_step not in seen:
        ordered_steps.append(rerun_step)

    configure_step = "then run: python scripts/build_zephyr.py configure"
    if context.platform_name in {"linux", "wsl", "macos"}:
        configure_step = "then run: python3 scripts/build_zephyr.py configure"
    if configure_step not in seen:
        ordered_steps.append(configure_step)

    return ordered_steps


def print_summary(prefix: str, context: HostContext, failed_results: list[CheckResult]) -> None:
    if not failed_results:
        ready_next = "python scripts/build_zephyr.py configure"
        if context.platform_name in {"linux", "wsl", "macos"}:
            ready_next = "python3 scripts/build_zephyr.py configure"
        print(f"{prefix} summary:")
        print(f"{prefix}   required checks passed")
        print(f"{prefix}   next: {ready_next}")
        return

    print(f"{prefix} summary:")
    print(f"{prefix}   missing required items: {', '.join(result.label for result in failed_results)}")
    for step in format_next_steps(context, failed_results):
        print(f"{prefix}   next: {step}")


def json_summary(context: HostContext, failed_results: list[CheckResult]) -> dict[str, object]:
    return {
        "host": context.platform_name,
        "python_executable": context.python_executable,
        "ready": not failed_results,
        "missing_required_items": [result.label for result in failed_results],
        "next_steps": format_next_steps(context, failed_results)
        if failed_results
        else [
            "python scripts/build_zephyr.py configure"
            if context.platform_name == "windows"
            else "python3 scripts/build_zephyr.py configure"
        ],
    }


def current_python_version_command(context: HostContext) -> list[str]:
    return [context.python_executable, "--version"]


def main() -> int:
    parser = argparse.ArgumentParser(description="Cross-platform host environment self-check.")
    parser.add_argument(
        "--mode",
        choices=("auto", "linux-like"),
        default="auto",
        help="Use `linux-like` to require Linux/WSL style checks. Default: auto",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit machine-readable JSON instead of the human-oriented report.",
    )
    args = parser.parse_args()

    context = detect_host_context()
    prefix = "[aegis-host-check]"
    if not args.json:
        print(f"{prefix} repo={context.repo_root}")
        print(f"{prefix} host={context.platform_name}")
        print(f"{prefix} python={context.python_executable}")

    all_results: list[CheckResult] = []
    failed_results: list[CheckResult] = []

    if args.mode == "linux-like" and context.platform_name not in {"linux", "wsl"}:
        if not args.json:
            print(f"{prefix} warning: linux-like mode requested on host={context.platform_name}")

    checks = [
        ("python", current_python_version_command(context)),
        ("pip", [context.python_executable, "-m", "pip", "--version"]),
        ("cmake", ["cmake", "--version"]),
        ("ninja", ["ninja", "--version"]),
        ("west", ["west", "--version"]),
        ("esptool", [context.python_executable, "-m", "esptool", "version"]),
    ]

    for label, command in checks:
        ok, detail = run_check(command)
        suggestions = suggestions_for(context, label, detail) if not ok else []
        if not args.json:
            print_check(prefix, label, ok, detail, suggestions)
        result = CheckResult(label=label, ok=ok, detail=detail, suggestions=suggestions)
        all_results.append(result)
        if not ok:
            failed_results.append(result)

    zephyr_base = os.environ.get("ZEPHYR_BASE", "").strip()
    if zephyr_base:
        exists = pathlib.Path(zephyr_base).exists()
        detail = zephyr_base
        if not args.json:
            print_check(
                prefix,
                "ZEPHYR_BASE",
                exists,
                detail,
                suggestions_for(context, "ZEPHYR_BASE", detail),
            )
        if not exists:
            result = CheckResult(
                label="ZEPHYR_BASE",
                ok=False,
                detail=detail,
                suggestions=suggestions_for(context, "ZEPHYR_BASE", detail),
            )
            all_results.append(result)
            failed_results.append(result)
        else:
            all_results.append(
                CheckResult(
                    label="ZEPHYR_BASE",
                    ok=True,
                    detail=detail,
                    suggestions=[],
                )
            )
    else:
        detail = "environment variable is not set"
        suggestions = suggestions_for(context, "ZEPHYR_BASE", detail)
        if not args.json:
            print_check(
                prefix,
                "ZEPHYR_BASE",
                False,
                detail,
                suggestions,
            )
        result = CheckResult(label="ZEPHYR_BASE", ok=False, detail=detail, suggestions=suggestions)
        all_results.append(result)
        failed_results.append(result)

    sdk_dir = os.environ.get("ZEPHYR_SDK_INSTALL_DIR", "").strip()
    if sdk_dir:
        exists = pathlib.Path(sdk_dir).exists()
        if not args.json:
            status = "ok" if exists else "warning"
            print(f"{prefix} ZEPHYR_SDK_INSTALL_DIR: {status} - {sdk_dir}")
            if not exists:
                print(f"{prefix}   note: the path does not exist in this shell")
    else:
        if not args.json:
            print(f"{prefix} ZEPHYR_SDK_INSTALL_DIR: optional - not set")

    mklittlefs_path = shutil.which("mklittlefs")
    if not args.json:
        if mklittlefs_path:
            print(f"{prefix} mklittlefs: ok - {mklittlefs_path}")
        else:
            print(f"{prefix} mklittlefs: optional - not found, python fallback may still work")

    if args.json:
        checks_payload = [
            {
                "label": result.label,
                "ok": result.ok,
                "detail": result.detail,
                "suggestions": result.suggestions,
            }
            for result in all_results
        ]
        payload = {
            "repo_root": str(context.repo_root),
            **json_summary(context, failed_results),
            "checks": checks_payload,
            "zephyr_sdk_install_dir": sdk_dir or None,
            "mklittlefs_path": mklittlefs_path,
        }
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print_summary(prefix, context, failed_results)

    if failed_results:
        if not args.json:
            print(
                f"{prefix} result: not ready - fix the missing required items above before running "
                "the Zephyr build flow"
            )
        return 1

    if not args.json:
        print(f"{prefix} result: ready for repository-level Zephyr configure checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
