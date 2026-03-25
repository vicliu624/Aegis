# Zephyr Quick Start

## 1. Purpose

This document is the shortest working path for a new machine.

Read this first if your goal is simple:

- get the current Aegis Zephyr target to configure
- build firmware plus appfs
- flash the board
- know what to check first when the path fails

If you need full environment preparation, read [zephyr-onboarding.md](./zephyr-onboarding.md).

If you need the full build model, artifact layout, or target explanations, read
[building-zephyr.md](./building-zephyr.md).

---

## 2. What "Success" Means

For a brand-new machine, there are three useful milestones:

### 2.1 First milestone: configure works

```bash
python scripts/build_zephyr.py configure
```

If that succeeds, the machine is no longer "totally unprepared". It means:

- Python is callable
- CMake is callable
- the repository build entrypoint works
- `ZEPHYR_BASE` is visible
- the Zephyr toolchain can be discovered
- the repository's local Xtensa patch checks are passing

### 2.2 Second milestone: build-all works

```bash
python scripts/build_zephyr.py build-all --parallel 8
```

If that succeeds, both of these are working:

- resident firmware build
- appfs packaging

### 2.3 Third milestone: flash-all works

```bash
python scripts/build_zephyr.py flash-all --port <your-port> --parallel 8
```

If that succeeds, the current end-to-end host build and flash path is working.

---

## 3. Current Default Target

The repository quick-start path assumes the current Pager bring-up target:

- Zephyr board: `esp32s3_devkitc/esp32s3/procpu`
- overlay: `boards/lilygo_tlora_pager_sx1262.overlay`

You do not need to pass these values in the normal quick-start flow because
`scripts/build_zephyr.py` already defaults to them.

---

## 4. Pick the Right Entry Path

Use this rule:

- Windows host: start at [Windows Quick Start](#5-windows-quick-start)
- Linux host: start at [Linux Quick Start](#6-linux-quick-start)
- WSL host: start at [WSL Quick Start](#7-wsl-quick-start)
- macOS host: start at [macOS Quick Start](#8-macos-quick-start)

If you are unsure whether the machine is fully onboarded, do not start with `flash-all`.
Start with the first check for your platform.

---

## 5. Windows Quick Start

### 5.1 First check

From the repository root:

```powershell
python scripts/build_zephyr.py check-host-env
```

This is the safest first step because it does not build anything. It verifies the host environment
that the repository depends on.

If that passes, you can optionally inspect the resolved build commands:

```powershell
python scripts/build_zephyr.py print-plan
```

The host check verifies the items most likely to break a first Windows bring-up:

- Python can run
- `pip` can run in the active Python environment
- CMake is callable
- Ninja is callable
- `west` is callable
- `esptool` is callable
- `ZEPHYR_BASE` is set

### 5.2 First real check

```powershell
python scripts/build_zephyr.py configure
```

If this fails immediately, stop and read [zephyr-onboarding.md](./zephyr-onboarding.md).

Do not move on to `build-all` until `configure` succeeds.

### 5.3 Build firmware plus appfs

```powershell
python scripts/build_zephyr.py build-all --parallel 8
```

### 5.4 Flash everything

Replace `COM7` with the actual serial port:

```powershell
python scripts/build_zephyr.py flash-all --port COM7 --parallel 8
```

### 5.5 If Windows fails very early

Check these first:

- `python --version`
- `cmake --version`
- `ninja --version`
- `west --version`
- `python -m esptool version`
- `echo $env:ZEPHYR_BASE`

The most common Windows-specific mistake is a missing `Ninja` install, which can push CMake toward
the wrong generator choice.

---

## 6. Linux Quick Start

### 6.1 First check

From the repository root:

```bash
python3 scripts/build_zephyr.py check-host-env
```

This is the fastest way to catch the most common Linux onboarding misses before a full Zephyr
configure.

If you want the older Linux-specific compatibility path, this still exists:

```bash
python3 scripts/build_zephyr.py check-host-env
```

### 6.2 If the check reports missing tools

Follow the suggested repair actions printed by the command.

Typical missing items are:

- `python3-pip`
- `cmake`
- `ninja`
- `west`
- `python3 -m esptool`
- `ZEPHYR_BASE`

### 6.3 After the check passes

```bash
python3 scripts/build_zephyr.py configure
python3 scripts/build_zephyr.py build-all --parallel 8
```

### 6.4 Flash everything

Replace `/dev/ttyACM0` with the actual device path:

```bash
python3 scripts/build_zephyr.py flash-all --port /dev/ttyACM0 --parallel 8
```

### 6.5 Minimal Linux self-check commands

If you want the simplest manual sanity pass before the repository script:

```bash
python3 --version
python3 -m pip --version
cmake --version
ninja --version
west --version
python3 -m esptool version
echo "$ZEPHYR_BASE"
```

If any one of those fails, the machine is not ready yet.

---

## 7. WSL Quick Start

Treat WSL like Linux, but keep one rule in mind:

- use Linux tools inside WSL
- do not assume Windows Python and WSL Python are interchangeable

### 7.1 First check inside WSL

From the repository root inside the WSL distro shell:

```bash
python3 scripts/build_zephyr.py check-linux-env
```

### 7.2 Most common WSL fixes

If `python3-pip` is missing:

```bash
sudo apt update && sudo apt install -y python3-pip
```

If `cmake` is missing:

```bash
sudo apt update && sudo apt install -y cmake
```

If `ninja` is missing:

```bash
sudo apt update && sudo apt install -y ninja-build
```

If `west` is missing:

```bash
python3 -m pip install west
```

If `esptool` is missing:

```bash
python3 -m pip install esptool
```

If `ZEPHYR_BASE` is missing:

```bash
export ZEPHYR_BASE=/path/to/zephyr
```

If a tool shows `permission denied`, check whether WSL is seeing a broken Windows shim instead of a
real Linux executable:

```bash
command -v cmake
command -v ninja
command -v west
```

### 7.3 After the check passes

```bash
python3 scripts/build_zephyr.py configure
python3 scripts/build_zephyr.py build-all --parallel 8
```

### 7.4 Flash everything

```bash
python3 scripts/build_zephyr.py flash-all --port /dev/ttyACM0 --parallel 8
```

---

## 8. macOS Quick Start

macOS uses the same repository entrypoint as Linux.

### 8.1 First check

```bash
python3 scripts/build_zephyr.py check-host-env
```

This gives macOS the same top-level self-check entrypoint as Windows and Linux.

### 8.2 Configure and build

```bash
python3 scripts/build_zephyr.py configure
python3 scripts/build_zephyr.py build-all --parallel 8
```

### 8.3 Flash everything

Replace the serial port with the real device path:

```bash
python3 scripts/build_zephyr.py flash-all --port /dev/cu.usbmodem101 --parallel 8
```

If your macOS Python environment does not expose `pip`, `west`, or `esptool`, go to
[zephyr-onboarding.md](./zephyr-onboarding.md) and complete the environment setup first.

---

## 9. Shortest Happy Path

### Windows

```powershell
python scripts/build_zephyr.py check-host-env
python scripts/build_zephyr.py configure
python scripts/build_zephyr.py build-all --parallel 8
python scripts/build_zephyr.py flash-all --port COM7 --parallel 8
```

### Linux

```bash
python3 scripts/build_zephyr.py check-host-env
python3 scripts/build_zephyr.py configure
python3 scripts/build_zephyr.py build-all --parallel 8
python3 scripts/build_zephyr.py flash-all --port /dev/ttyACM0 --parallel 8
```

### WSL

```bash
python3 scripts/build_zephyr.py check-host-env
python3 scripts/build_zephyr.py configure
python3 scripts/build_zephyr.py build-all --parallel 8
python3 scripts/build_zephyr.py flash-all --port /dev/ttyACM0 --parallel 8
```

### macOS

```bash
python3 scripts/build_zephyr.py check-host-env
python3 scripts/build_zephyr.py configure
python3 scripts/build_zephyr.py build-all --parallel 8
python3 scripts/build_zephyr.py flash-all --port /dev/cu.usbmodem101 --parallel 8
```

---

## 10. Failure Triage

Use this simple rule:

### 10.1 Failure before `configure`

Read [zephyr-onboarding.md](./zephyr-onboarding.md).

This is almost always an environment problem, not a repository build-graph problem.

### 10.2 Failure during `configure`

Check:

- `ZEPHYR_BASE`
- active Zephyr toolchain visibility
- generator mismatch inside the chosen build directory
- local Zephyr Xtensa patch validation

Then read:

- [zephyr-onboarding.md](./zephyr-onboarding.md)
- [building-zephyr.md](./building-zephyr.md)

### 10.3 Failure during `build-all`

Check whether the failure happened in:

- resident firmware build
- app module build
- appfs packaging

If the failure mentions LittleFS image creation, check:

- `mklittlefs`
- `littlefs-python`
- working `pip`

### 10.4 Failure during flash

Check:

- the serial port value
- whether another tool is holding the port open
- whether the board is in the expected flash state
- whether you already built the current build directory successfully

---

## 11. Recommended Reading Order

If you are new to this repository, the safest reading order is:

1. [quick-start-zephyr.md](./quick-start-zephyr.md)
2. [zephyr-onboarding.md](./zephyr-onboarding.md)
3. [building-zephyr.md](./building-zephyr.md)

That order intentionally goes from:

- shortest path
- to environment preparation
- to full build model

---

## 12. One-Line Summary

For a new machine, the single most important command is:

```bash
python scripts/build_zephyr.py configure
```

Or on Linux, WSL, and most macOS setups:

```bash
python3 scripts/build_zephyr.py configure
```

That is the first meaningful proof that the machine is actually ready to participate in the Aegis
Zephyr build flow.
