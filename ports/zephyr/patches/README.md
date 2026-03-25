# Local Zephyr Patches

This directory tracks Zephyr-source adjustments that are required for the current Aegis bring-up but
live outside the main repository checkout because Zephyr is consumed through `ZEPHYR_BASE`.

Current patch set:

- Xtensa LLEXT `R_XTENSA_RTLD` relocation handling:
  Aegis requires the local Xtensa loader path to treat `R_XTENSA_RTLD` as an intentional no-op for
  the current writable-buffer LLEXT bring-up flow on ESP32-S3.

The repository does not vendor Zephyr itself, so these adjustments are applied through helper scripts
at configure time and verified as part of the Zephyr build entrypoint.
