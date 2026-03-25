# Zephyr Environment Onboarding

## 1. Purpose

This document is for a fresh machine that does not yet have the Aegis Zephyr build environment
working.

It explains how to prepare the host so the repository build flow can run successfully, including:

- Python
- CMake
- Ninja
- `west`
- Zephyr SDK and toolchain setup
- `ZEPHYR_BASE`
- `esptool`
- `mklittlefs`

This is an environment preparation guide.

For the actual repository build flow after the environment is ready, use
[building-zephyr.md](./building-zephyr.md).

---

## 2. What "Ready" Means

A machine is considered onboarding-complete for the current Aegis Zephyr target when all of the
following are true:

- `python --version` works
- `cmake --version` works
- `west --version` works
- `ninja --version` works
- `ZEPHYR_BASE` points at a usable Zephyr tree
- the Zephyr SDK/toolchain is available to that Zephyr tree
- `python -m esptool version` works
- the repository command below succeeds:

```bash
python scripts/build_zephyr.py configure
```

That command is the practical end of onboarding because it exercises:

- Python
- CMake
- the CMake generator choice
- Zephyr discovery
- toolchain discovery
- devicetree generation
- the local Xtensa patch check path

---

## 3. Host tools overview

The current Aegis Zephyr bring-up depends on a small set of host-side tools.

### 3.1 Python

Used for:

- repository helper scripts
- Zephyr build support
- appfs packaging helpers
- `esptool`

### 3.2 CMake

Used as the primary build generator and target runner.

### 3.3 Ninja

Recommended as the CMake generator, especially on Windows.

For the current Xtensa Zephyr target, Ninja is the safest default because it avoids CMake silently
falling back to a Visual Studio host generator that is inappropriate for this cross-toolchain path.

### 3.4 west

Used as the Zephyr workspace and tooling front-end.

Even when Aegis uses direct `cmake` calls in its own build driver, `west` is still part of a healthy
Zephyr environment and is a useful verification signal.

### 3.5 Zephyr SDK / Zephyr toolchain

Provides the actual cross-compilers, linker, and related binaries for the target architecture.

### 3.6 esptool

Used for ESP flashing, especially appfs flashing through the repository helper path.

### 3.7 mklittlefs

Used to build `appfs.bin` when the packaging flow relies on the LittleFS image tool rather than the
Python fallback path.

### 3.8 littlefs-python

Used as the cross-platform fallback backend for appfs image generation when `mklittlefs` is not
available.

---

## 4. Cross-platform notes

The repository build script is cross-platform, but onboarding still depends on the host OS.

### Windows

Current behavior is best validated on Windows because that is the path used during the present Pager
bring-up.

Recommendations:

- install Python 3
- install CMake
- install Ninja
- install `west`
- install the Zephyr SDK/toolchain
- ensure `esptool` is available through Python

The repository build driver automatically prefers the `Ninja` generator on Windows when it is
available.

### Linux

Linux should work well with the same build driver as long as:

- Python, CMake, Ninja, and `west` are installed
- the Zephyr SDK/toolchain is installed and visible
- `esptool` is installed
- `mklittlefs` is installed if your flow requires it

### macOS

macOS follows the same pattern as Linux:

- Python, CMake, Ninja, and `west`
- Zephyr SDK/toolchain
- `esptool`
- `mklittlefs` if needed

The repository build script itself does not assume PowerShell, batch files, or shell-specific command
syntax.

---

## 5. Install order

The easiest onboarding order is:

1. Install Python.
2. Install CMake.
3. Install Ninja.
4. Install `west`.
5. Install the Zephyr SDK/toolchain.
6. Make sure `ZEPHYR_BASE` points to a Zephyr tree.
7. Install `esptool`.
8. Install `mklittlefs` if your host path needs it.
9. Run repository verification commands.

This order matters because later steps assume earlier ones are already working.

---

## 6. Python setup

Install a modern Python 3 release and confirm:

```bash
python --version
```

Or, depending on host setup:

```bash
python3 --version
```

The current repository scripts are invoked with `python`, so it is best if `python` resolves cleanly
on your machine.

After Python is available, install `west` and `esptool` into that same Python environment:

```bash
python -m pip install west esptool
```

Verify:

```bash
west --version
python -m esptool version
```

---

## 7. CMake and Ninja setup

Install both tools and verify:

```bash
cmake --version
ninja --version
```

Ninja is especially important on Windows because the repository build driver will prefer it
automatically when found.

If Ninja is not installed on Windows, CMake may fall back to a Visual Studio generator, which is not
what you want for the current Xtensa Zephyr target.

---

## 8. Zephyr setup

You need two things:

- a Zephyr tree
- the Zephyr SDK/toolchain used by that tree

### 8.1 `ZEPHYR_BASE`

Set `ZEPHYR_BASE` to the root of your Zephyr checkout or installed Zephyr tree.

The repository currently expects that variable to exist before configuring the Zephyr target.

Verify:

Windows PowerShell:

```powershell
echo $env:ZEPHYR_BASE
```

Linux/macOS shell:

```bash
echo "$ZEPHYR_BASE"
```

### 8.2 Verify the Zephyr environment

From any shell where onboarding should work, confirm:

```bash
west --version
```

And, if you want an additional sanity check:

```bash
cmake -E environment
```

Then confirm `ZEPHYR_BASE` appears in the environment and points at the expected tree.

### 8.3 Current Zephyr version expectation

The repository currently validates its local Xtensa loader patch path against Zephyr `4.3.0`.

That does not mean newer Zephyr versions are impossible, but it does mean:

- the local patch helper is version-gated
- if you move to a different Zephyr version, you may need to re-validate the loader source layout

Relevant references:

- [ports/zephyr/patches/README.md](../ports/zephyr/patches/README.md)
- [ports/zephyr/patches/xtensa_rtld_noop.patch](../ports/zephyr/patches/xtensa_rtld_noop.patch)

---

## 9. esptool setup

Install through Python:

```bash
python -m pip install esptool
```

Verify:

```bash
python -m esptool version
```

This matters even if your normal firmware flash flow sometimes uses other Zephyr mechanisms, because
the repository appfs flash path currently relies on the ESP flashing toolchain model.

---

## 10. mklittlefs setup

`mklittlefs` is used when building the LittleFS `appfs.bin` image.

### Windows

The current repository appfs helper can use either:

- `mklittlefs`
- or the Python `littlefs-python` backend

With auto-fetch enabled, the Python backend can be installed into the tool cache automatically.

### Linux and macOS

Linux and macOS now follow the same general pattern:

- if `mklittlefs` is already installed, the helper can use it
- otherwise it can fall back to `littlefs-python`
- with auto-fetch enabled, that Python backend can be installed into the tool cache automatically

That means Linux/macOS are no longer forced onto a separate “must already have `mklittlefs`”
experience just to package appfs.

Verify:

```bash
mklittlefs --help
```

If `mklittlefs` is not globally installed, you can still point the repository packaging flow at an
explicit tool path through the lower-level helper configuration path documented in
[building-zephyr.md](./building-zephyr.md).

If you prefer to verify the Python fallback path directly, you can also confirm that `littlefs-python`
is importable:

```bash
python -c "import littlefs"
```

---

## 11. First repository verification steps

Once the host tools are installed, verify from the repository root.

### 11.1 Inspect the planned build commands

```bash
python scripts/build_zephyr.py print-plan
```

This should print the configured `cmake` and `cmake --build` commands without executing them.

### 11.2 Run a real configure

```bash
python scripts/build_zephyr.py configure
```

This is the most important onboarding check.

If it succeeds, your machine has at least:

- Python working
- CMake working
- a usable generator
- Zephyr discovery working
- toolchain discovery working
- repository-specific Zephyr patch checks working

### 11.3 Optional full build verification

```bash
python scripts/build_zephyr.py build-all --parallel 8
```

That confirms not only configuration, but also resident firmware and appfs packaging.

---

## 12. Serial port notes

You only need a port when flashing.

Examples:

- Windows: `COM7`
- Linux: `/dev/ttyUSB0`
- Linux: `/dev/ttyACM0`
- macOS: `/dev/cu.usbmodem101`

Examples:

```bash
python scripts/build_zephyr.py flash-firmware --port COM7
```

```bash
python scripts/build_zephyr.py flash-appfs --port /dev/ttyACM0
```

---

## 13. Common onboarding failures

### `python` works, but `west` does not

That usually means:

- `west` was not installed into the active Python environment
- or the Python scripts directory is not on your `PATH`

### `cmake` exists, but configure picks the wrong generator on Windows

That usually means:

- Ninja is missing
- or the build directory was previously configured with a different generator

The current repository build driver prefers Ninja on Windows, but CMake will not let you switch
generators inside an already-configured build directory.

### `ZEPHYR_BASE` is set, but configure still fails

That may mean:

- the Zephyr tree is incomplete
- the SDK/toolchain is not correctly installed
- the shell session does not have the same environment you thought it had

### The local Xtensa patch check fails

That usually means one of two things:

- the current Zephyr tree is missing the required local loader adjustment
- or you are using a Zephyr version newer than the version currently validated in this repository

### `esptool` is missing during flash

Install it into the same Python environment used to run the repository scripts:

```bash
python -m pip install esptool
```

### `mklittlefs` is missing during appfs packaging

Install it on the host, let the helper fall back to `littlefs-python`, or provide the tool through the appfs helper path described in
[building-zephyr.md](./building-zephyr.md).

---

## 14. Recommended handoff checklist

If you are preparing a new machine for someone else, hand off only after all of these pass:

- `python --version`
- `cmake --version`
- `ninja --version`
- `west --version`
- `python -m esptool version`
- `echo $ZEPHYR_BASE` or the platform equivalent
- `python scripts/build_zephyr.py print-plan`
- `python scripts/build_zephyr.py configure`

If the machine will also be used for real packaging and flashing, also verify:

- `python scripts/build_zephyr.py build-all --parallel 8`

---

## 15. Summary

The fastest way to think about onboarding is:

- get the host tools installed
- get Zephyr and the toolchain visible
- set `ZEPHYR_BASE`
- verify `west`, `esptool`, and Ninja
- run `python scripts/build_zephyr.py configure`

Once that succeeds, move on to the normal repository build flow described in
[building-zephyr.md](./building-zephyr.md).
