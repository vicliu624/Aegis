# Building Aegis on Zephyr

## 1. Purpose

This document explains the real build flow for the current Aegis Zephyr target, not just a list of
commands.

If you are starting from a new machine and want the shortest working path first, read
[quick-start-zephyr.md](./quick-start-zephyr.md) before this document.

It covers:

- what gets built
- why the build is split into multiple stages
- how to use the repository build script on Windows, Linux, and macOS
- what each generated artifact means
- how firmware flashing and appfs flashing differ

The current target described here is the LilyGo T-LoRa Pager bring-up path built on Zephyr's
`esp32s3_devkitc/esp32s3/procpu` board target plus the repository overlay
`boards/lilygo_tlora_pager_sx1262.overlay`.

---

## 2. Build model

Aegis on Zephyr is not a single build product.

There are two distinct outputs:

### 2.1 Resident firmware image

This is the long-lived Zephyr image that contains:

- Zephyr itself
- Aegis core
- the shell
- board adaptation
- runtime loader
- service backends

This image is built by the normal Zephyr application under [ports/zephyr](../ports/zephyr).

### 2.2 App filesystem image

This is a LittleFS image containing discovered app packages under `/lfs/apps`, for example:

- `manifest.json`
- `app.llext`
- optional icon assets

This is packaged separately because Aegis treats resident system firmware and loadable app content as
different deployment units.

That separation is intentional and matches the architecture:

- the resident firmware is the governed system
- the appfs image is the installable app catalog for that system

---

## 3. What the build stages do

The full bring-up flow has five practical stages.

### 3.1 Configure

Configuration tells CMake and Zephyr:

- which CMake generator to use
- which Zephyr board target to use
- which devicetree overlay to apply
- whether compiled app fallback is enabled
- whether standalone LLEXT app modules are enabled
- what appfs flash defaults to use
- whether local Zephyr source patch helpers may auto-apply required fixes

It also generates the Zephyr build tree, including:

- `zephyr.dts`
- `.config`
- generated headers
- target graph for firmware and app modules

### 3.2 Build resident firmware

This compiles the Aegis system image itself.

The result is the Zephyr firmware output for the configured build directory.

### 3.3 Build app modules and package appfs

This builds each app module as a Zephyr LLEXT binary, stages it into a deploy tree, and then packs
the deploy tree into `appfs.bin`.

The important internal steps are:

1. compile each app entry as an LLEXT module
2. stage package files under `deploy/lfs/apps/<app-id>/`
3. inspect `zephyr.dts` to discover the `storage_partition` size and offset
4. write `appfs-layout.json`
5. build the LittleFS image `appfs.bin`

### 3.4 Flash resident firmware

This writes the main Zephyr/Aegis image to the normal firmware partitions.

### 3.5 Flash appfs

This writes `appfs.bin` specifically to the `storage_partition` offset discovered from the generated
devicetree.

This is separate from firmware flashing because appfs is not just another source file inside the main
binary. It is a filesystem image that the resident runtime mounts later at `/lfs`.

---

## 4. Repository build script

The repository now includes a cross-platform Python build driver:

- [build_zephyr.py](../scripts/build_zephyr.py)

It runs on Windows, Linux, and macOS because it only orchestrates:

- `cmake -S ... -B ...`
- `cmake --build ... --target ...`

The script does not replace Zephyr's build system. It standardizes how this repository invokes it.

### 4.1 Why this script exists

Before this script, the build flow was spread across:

- ad hoc `cmake` configure commands
- manual `cmake --build` target selection
- separate appfs-specific helper scripts
- platform-specific command examples

That made the flow easy to forget and hard to document consistently.

The driver script fixes that by giving the project one stable user-facing entrypoint for the common
Zephyr bring-up path.

For the shortest first-run command sequence, see [quick-start-zephyr.md](./quick-start-zephyr.md).

---

## 5. Script prerequisites

The script assumes you already have a working Zephyr toolchain environment.

At minimum:

- Python 3
- CMake
- Zephyr SDK/toolchain
- `west`
- `ZEPHYR_BASE` set to an installed Zephyr tree

For ESP flashing you also need:

- `esptool` available through `python -m esptool` or a separately provided executable

For appfs packaging:

- the helper prefers `mklittlefs` when it is available
- if `mklittlefs` is not available, the helper can fall back to `littlefs-python`
- with auto-fetch enabled, that Python fallback can be installed into the tool cache on all supported host platforms

The script does not hide these dependencies. It makes the repository workflow repeatable once the
environment exists.

On Windows, this target should normally be configured with the `Ninja` generator. The build driver
will choose `Ninja` automatically when it is available so CMake does not fall back to a Visual Studio
host generator that is not appropriate for the Xtensa Zephyr toolchain path.

---

## 6. Common script usage

All commands are run from the repository root.

### 6.1 Print the resolved command plan without running it

```bash
python scripts/build_zephyr.py print-plan
```

This is useful when you want to inspect the exact `cmake` invocations first.

### 6.2 Configure only

```bash
python scripts/build_zephyr.py configure
```

### 6.3 Build resident firmware only

```bash
python scripts/build_zephyr.py build-firmware
```

### 6.4 Build appfs only

```bash
python scripts/build_zephyr.py build-appfs
```

### 6.5 Configure, build firmware, and package appfs

```bash
python scripts/build_zephyr.py build-all
```

### 6.6 Flash main firmware

```bash
python scripts/build_zephyr.py flash-firmware --port COM7
```

Linux or macOS example:

```bash
python scripts/build_zephyr.py flash-firmware --port /dev/ttyACM0
```

### 6.7 Flash appfs

```bash
python scripts/build_zephyr.py flash-appfs --port COM7
```

### 6.8 Full configure/build/flash path

```bash
python scripts/build_zephyr.py flash-all --port COM7
```

---

## 7. Important script options

### `--build-dir`

Defaults to `build-zephyr`.

Use this when you want to keep multiple build trees around:

```bash
python scripts/build_zephyr.py build-all --build-dir build-zephyr-clean
```

### `--generator`

Lets you choose the CMake generator explicitly.

Examples:

```bash
python scripts/build_zephyr.py configure --generator Ninja
```

```bash
python scripts/build_zephyr.py configure --generator "Unix Makefiles"
```

On Windows, `Ninja` is the recommended generator for this target.

### `--board`

Defaults to `esp32s3_devkitc/esp32s3/procpu`.

This is the Zephyr base board target, not the Aegis board identity itself.

### `--overlay`

Defaults to `boards/lilygo_tlora_pager_sx1262.overlay`.

This overlay is what maps the generic Zephyr board target onto the concrete T-LoRa Pager wiring used
by the current Aegis bring-up.

### `--compiled-fallback`

Defaults to `off`.

Use `on` only if you intentionally want compiled fallback app contracts to remain enabled for
bootstrap/debug work.

### `--app-modules`

Defaults to `on`.

This should stay on for the real LLEXT path because it is what builds and stages the standalone app
packages.

### `--selftest-app`

Defaults to `demo_hello`.

Set it to an empty string if you want no auto-launched boot validation app:

```bash
python scripts/build_zephyr.py configure --selftest-app ""
```

### `--port`

Used by flash-related commands.

Examples:

- Windows: `COM7`
- Linux: `/dev/ttyUSB0`
- macOS: `/dev/cu.usbmodem101`

### `--parallel`

Passes through to `cmake --build --parallel`.

Example:

```bash
python scripts/build_zephyr.py build-all --parallel 8
```

### `--auto-apply-local-patches`

Defaults to `on`.

This controls whether configure may auto-apply the required local Xtensa Zephyr patch for the current
LLEXT bring-up path.

### `--allow-unsupported-local-patch-version`

Defaults to `off`.

This should remain off unless you have explicitly re-validated the local patch path against a newer
Zephyr source version.

---

## 8. Generated artifacts

The most important outputs live under the chosen build directory.

### Resident firmware tree

Important files include the normal Zephyr build outputs plus generated metadata such as:

- `zephyr/zephyr.dts`
- `zephyr/.config`

### Staged app packages

Under:

- `<build-dir>/deploy/lfs/apps/<app-id>/`

Each package may contain:

- `manifest.json`
- `app.llext`
- `icon.bin`

### Appfs metadata

Under:

- `<build-dir>/deploy/appfs-layout.json`

This records:

- filesystem type
- mount point
- apps root
- storage partition size and offset
- packaged app list

### Appfs image

Under:

- `<build-dir>/deploy/appfs.bin`

This is the image written into the board's `storage_partition`.

---

## 9. Build principles behind the script

The script intentionally follows a few rules.

### 9.1 It keeps Zephyr as the build authority

The script does not invent a second build graph.

Instead, it drives the existing CMake/Zephyr targets:

- `app`
- `flash`
- `aegis_zephyr_appfs_image`
- `aegis_zephyr_flash_appfs`

### 9.2 It keeps packaging and resident firmware distinct

This matters because Aegis is architected around a resident system plus loadable app content.

If the build flow hid that distinction, it would work against the actual runtime model.

### 9.3 It remains cross-platform by staying simple

The driver is written in Python because Python is already a build dependency for Zephyr and the
existing appfs helpers.

That makes one script usable across:

- Windows
- Linux
- macOS

without requiring separate `.bat`, `.ps1`, and `.sh` wrappers for the core flow.

---

## 10. Relation to the lower-level appfs helpers

The repository still contains lower-level helpers in
[ports/zephyr/scripts](../ports/zephyr/scripts):

- [build_appfs_image.py](../ports/zephyr/scripts/build_appfs_image.py)
- [flash_appfs_image.py](../ports/zephyr/scripts/flash_appfs_image.py)
- [ensure_xtensa_rtld_patch.py](../ports/zephyr/scripts/ensure_xtensa_rtld_patch.py)

Their roles are:

- `build_appfs_image.py`: package the deploy tree into `appfs.bin`
- `flash_appfs_image.py`: write `appfs.bin` to the correct storage offset
- `ensure_xtensa_rtld_patch.py`: verify or apply the required local Xtensa loader fix in the active Zephyr tree

The top-level build driver does not replace them. It orchestrates the CMake targets that already use
them.

For appfs packaging specifically, `build_appfs_image.py` now supports three backend modes:

- `auto`: prefer `mklittlefs`, otherwise fall back to `littlefs-python`
- `mklittlefs`: require the native tool path
- `python`: require the Python LittleFS backend

The repository CMake path currently uses the helper's default `auto` mode.

---

## 11. Typical bring-up sequence

For a clean full cycle:

1. Configure the Zephyr build tree.
2. Build the resident firmware image.
3. Package the app modules into `appfs.bin`.
4. Flash the resident firmware.
5. Flash the appfs image.
6. Boot the device and inspect serial logs plus display output.

With the build driver, that becomes:

```bash
python scripts/build_zephyr.py flash-all --port COM7 --parallel 8
```

Or on Linux/macOS:

```bash
python scripts/build_zephyr.py flash-all --port /dev/ttyACM0 --parallel 8
```

---

## 12. Troubleshooting

### Configure fails before Zephyr starts

Check:

- `ZEPHYR_BASE` is set
- CMake is installed
- Python can run the helper scripts
- `west` is installed and your Zephyr toolchain environment is active
- the selected build directory was not previously configured with a different CMake generator such as Visual Studio instead of Ninja

### Configure fails on local Xtensa patch validation

That means the active Zephyr tree is missing the required local loader adjustment or you upgraded
Zephyr beyond the validated version gate.

Start with:

- [README.md](../ports/zephyr/patches/README.md)
- [xtensa_rtld_noop.patch](../ports/zephyr/patches/xtensa_rtld_noop.patch)

### Appfs packaging fails

Check:

- staged app packages exist under `<build-dir>/deploy/lfs/apps`
- either `mklittlefs` or `littlefs-python` is available for your host
- the generated deploy tree fits inside `storage_partition`

### Flashing appfs fails

Check:

- the serial port is correct
- no other tool is holding the serial port open
- `appfs.bin` exists
- `zephyr.dts` was generated for the same build tree

---

## 13. Summary

The Aegis Zephyr build is intentionally split because the runtime itself is split:

- resident system firmware
- separately packaged app content

The repository build driver exists to make that flow repeatable across operating systems without
hiding the real architecture. It standardizes the commands, but it still exposes the actual stages,
artifacts, and control points that matter during bring-up.
