# Local Zephyr Patches

This directory tracks Zephyr-source adjustments that are required for the current Aegis bring-up but
live outside the main repository checkout because Zephyr is consumed through `ZEPHYR_BASE`.

Current patch set:

- Xtensa LLEXT `R_XTENSA_RTLD` relocation handling:
  Aegis requires the local Xtensa loader path to treat `R_XTENSA_RTLD` as an intentional no-op for
  the current writable-buffer LLEXT bring-up flow on ESP32-S3.
- Xtensa LLEXT instruction fetchability gating:
  Aegis requires the local LLEXT memory classifier to reject writable ELF buffers that sit in the
  `0x3c...` data-mapped external window on ESP32-S3. Without this, Zephyr may bind `.text` directly
  to a non-executable alias and resident-packaged apps fault on launch.
- Xtensa LLEXT alloc-section reuse gating:
  Aegis requires the local LLEXT loader to stop reusing `SHF_ALLOC` regions in place when the source
  ELF lives in a writable external buffer. Xtensa shared-library relocation on ESP32-S3 proved
  unstable when `.text` was copied but other alloc sections were still left in the source image.

Validated against:

- Zephyr `4.3.0`
- target file: `arch/xtensa/core/elf.c`
- expected source delta: [xtensa_rtld_noop.patch](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/patches/xtensa_rtld_noop.patch)
- target file: `subsys/llext/llext_priv.h`
- expected source delta: [xtensa_llext_fetchability.patch](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/patches/xtensa_llext_fetchability.patch)
- target file: `subsys/llext/llext_mem.c`
- expected source delta: [xtensa_llext_alloc_reuse.patch](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/patches/xtensa_llext_alloc_reuse.patch)

The repository does not vendor Zephyr itself, so these adjustments are applied through helper scripts
at configure time and verified as part of the Zephyr build entrypoint.
