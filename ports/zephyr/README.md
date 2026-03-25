# Zephyr Target Skeleton

This directory is the formal Zephyr app entry for Aegis.

Current status:

- Zephyr provides the process entry and logging sink.
- `AegisCore` is instantiated through Zephyr-facing platform adapters.
- Zephyr now bootstraps a formal LittleFS-backed `appfs` mount at `/lfs`, ensures `/lfs/apps` exists, and app package discovery probes `/apps` first and then `/lfs/apps`.
- Zephyr-backed service adapters now exist for settings, timer, notification, storage, power, time, radio, GPS, audio, and hostlink.
- A typed text-input service contract now exists above the Zephyr keyboard path, so apps can query interpreted text-entry state without seeing board-specific keypad details.
- Zephyr UI binding is no longer forced through `Mock*Service` names in the board package path; display/input status is now carried by typed UI service data.
- The Zephyr target now includes a concrete `zephyr_tlora_pager_sx1262` board package/profile derived from the LilyGo T-LoRa-Pager hardware map.
- T-LoRa-Pager board adaptation now declares a real ST7796S display node on a Zephyr MIPI-DBI SPI controller, a real `gpio-qdec` rotary input node, a real `gpio-keys` center button, and a real TCA8418 keypad input driver path reported through the Zephyr input subsystem.
- Shell presentation and control are now hooked to Zephyr-backed adapters: shell surface changes drive a real display adapter, rotary input events come from Zephyr input devices, and TCA8418 keypad events now flow through a board-backed Zephyr input driver plus `input-keymap`.
- The Zephyr target now binds keyboard hardware twice in the right layers: raw matrix/key reporting through the Zephyr input subsystem, and interpreted modifier/text state through the Aegis text-input service contract.
- Compiled app fallback is now an explicit bootstrap option, and app-side ABI entry sources are split from compiled fallback contracts so `.llext`-oriented module builds can advance independently.
- Zephyr-native `add_llext_target(...)` packaging is now used for app modules, and staged `.llext` app packages are emitted under `build-zephyr/aegis/apps/<app-id>/`.
- A formal deploy-tree target now exists: `aegis_zephyr_deploy_app_tree` assembles those staged packages under `build-zephyr/deploy/lfs/apps/` so the runtime-facing appfs layout is explicit and no longer implicit build-directory knowledge.
- A formal `aegis_zephyr_appfs_image` target now exists. It derives the `storage_partition` size and offset from `zephyr.dts`, writes an `appfs-layout.json`, prefers `mklittlefs` when available, and now has a cross-platform `littlefs-python` fallback path that can be cached automatically when the native tool is absent.
- The T-LoRa-Pager overlay now reclaims the real 8MB flash space instead of stopping at the inherited 4MB partition profile, so `storage_partition` can hold a real multi-app `appfs.bin`.
- A formal `aegis_zephyr_flash_appfs` target now exists. It writes the generated `appfs.bin` directly to the resolved `storage_partition` offset through `esptool`, with the serial port supplied through `AEGIS_ZEPHYR_FLASH_PORT`.
- The Zephyr entrypoint now verifies the local Xtensa `R_XTENSA_RTLD` LLEXT no-op patch against the active `ZEPHYR_BASE` during configure, and by default auto-applies it so Pager bring-up does not silently depend on an untracked manual edit under `C:\\ProgramData\\zephyrproject\\zephyr`.
- That helper is intentionally version-gated to the Zephyr source layout validated during bring-up, so an upstream Zephyr upgrade fails loudly until the patch path is re-checked instead of silently mutating an unknown loader implementation.

Expected next steps:

1. Extend the current text-input service from latest-key snapshot semantics toward session-scoped text buffers, focus routing, and shell/app editor ownership rules.
2. Extend the display adapter from color-surface presentation to richer text/scene composition once the shell UI stack is formalized above the raw panel.
3. Continue pushing the loader path toward symbol-contract-first native execution so compiled fallback becomes purely a governance/debug escape hatch.
4. Replace the remaining device-name status adapters for radio / GPS / audio / hostlink with full board-backed backend implementations.

Suggested build flow for the current bringup path:

```text
west build -b esp32s3_devkitc/esp32s3/procpu ports/zephyr ^
  -- -DDTC_OVERLAY_FILE=boards/lilygo_tlora_pager_sx1262.overlay ^
     -DAEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK=OFF ^
     -DAEGIS_ZEPHYR_ENABLE_APP_MODULES=ON
```

This uses the ESP32-S3 devkit SoC target as the Zephyr base while binding Aegis device adaptation to the concrete T-LoRa-Pager wiring described in the overlay and `zephyr_tlora_pager_sx1262` board package.

Local Zephyr patch behavior:

- configure will verify the required Xtensa loader patch inside the active `ZEPHYR_BASE`
- by default Aegis auto-applies the patch if it is missing
- set `-DAEGIS_ZEPHYR_AUTO_APPLY_LOCAL_PATCHES=OFF` if you want configure to fail instead of mutating the local Zephyr tree
- the helper is version-gated by default; use `-DAEGIS_ZEPHYR_ALLOW_UNSUPPORTED_LOCAL_PATCH_VERSION=ON` only after re-validating the upstream Xtensa loader layout
- the helper logic lives in [ensure_xtensa_rtld_patch.py](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/scripts/ensure_xtensa_rtld_patch.py), the tracked rationale lives in [README.md](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/patches/README.md), and the expected source delta is mirrored in [xtensa_rtld_noop.patch](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/patches/xtensa_rtld_noop.patch)

Current appfs workflow:

```text
cmake --build build-zephyr --target aegis_zephyr_appfs_image
cmake -S ports/zephyr -B build-zephyr ... -DAEGIS_ZEPHYR_FLASH_PORT=COM7
cmake --build build-zephyr --target aegis_zephyr_flash_appfs
```

`aegis_zephyr_appfs_image` produces:

- `deploy/lfs/apps/...` staged app packages
- `deploy/appfs-layout.json`
- `deploy/appfs.bin`

`aegis_zephyr_flash_appfs` then flashes `appfs.bin` into the board `storage_partition`.

Bringup-Ready checklist for LilyGo T-LoRa-Pager:

1. Build the resident Zephyr image with the pager overlay and app modules enabled.
2. Flash the resident Zephyr image with the normal Zephyr `flash` target or `west flash`.
3. Build `aegis_zephyr_appfs_image`.
4. Flash `appfs.bin` with `aegis_zephyr_flash_appfs`.
5. Boot the device and confirm the panel shows readable Aegis text, not only a flat color fill.

Expected first-screen sequence on a healthy bringup:

- `AEGIS`
- `HOME`
- `BOOT`
- `APPFS MOUNTED APPS=<n> ROOT=/LFS/APPS`

Then, once shell bootstrap completes, the screen should continue updating with shell-owned frames such as:

- `LAUNCHER`
- `HOME`
- `SETTINGS`
- `NOTIFICATIONS`

If the screen only changes color but never shows text, the resident system is not yet considered bringup-ready even if the panel driver is probing successfully.
