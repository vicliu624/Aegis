# Aegis Phase 0-1 Bootstrap

## 1. Repository skeleton

```text
/
  CMakeLists.txt
  /core
    /aegis_core
      aegis_core.hpp
      aegis_core.cpp
    /boot
      boot_coordinator.hpp
      boot_coordinator.cpp
    /device_registry
      device_registry.hpp
      device_registry.cpp
    /app_registry
      app_admission_policy.hpp
      app_admission_policy.cpp
      app_catalog.hpp
      app_catalog.cpp
      app_catalog_builder.hpp
      app_catalog_builder.cpp
      app_compatibility_evaluator.hpp
      app_compatibility_evaluator.cpp
      app_runtime_policy.hpp
      app_manifest.hpp
      app_manifest.cpp
      app_registry.hpp
      app_registry.cpp
    /app_session
      app_session.hpp
      app_session.cpp
  /runtime
    /loader
      llext_adapter.hpp
      stub_llext_adapter.hpp
      stub_llext_adapter.cpp
      runtime_loader.hpp
      runtime_loader.cpp
    /host_api
      host_api.hpp
      host_api.cpp
    /ownership
      resource_ownership_table.hpp
      resource_ownership_table.cpp
    /lifecycle
      lifecycle.hpp
  /services
    /common
      service_interfaces.hpp
    /logging
      console_logging_service.hpp
      console_logging_service.cpp
    /settings
      in_memory_settings_service.hpp
      in_memory_settings_service.cpp
    /notification
      console_notification_service.hpp
      console_notification_service.cpp
    /ui
    /storage
    /timer
    /radio
    /gps
    /audio
    /power
    /device
      /common
        /capability
          capability_set.hpp
          capability_set.cpp
        /profile
          device_profile.hpp
          shell_surface_profile.hpp
        /binding
          service_binding_registry.hpp
          service_binding_registry.cpp
          mock_services.hpp
        mock_services.cpp
    /boards
      /board_x
      /board_y
    /packages
      board_package.hpp
      mock_device_packages.hpp
      mock_device_packages.cpp
  /shell
    aegis_shell.hpp
    aegis_shell.cpp
    /control
      shell_controller.hpp
      shell_controller.cpp
      shell_input_model.hpp
      shell_input_model.cpp
      shell_navigation_state.hpp
      shell_navigation_state.cpp
    /home
    /launcher
      launcher_model.hpp
      launcher_model.cpp
    /settings
      settings_model.hpp
      settings_model.cpp
    /status
  /sdk
    /include
      /aegis
        app_contract.hpp
    /templates
    /examples
  /apps
    /demo_hello
      manifest.json
      app.llext
      demo_hello_app.cpp
    /demo_info
      manifest.json
      app.llext
      demo_info_app.cpp
  /docs
    architecture.md
    app-model.md
    manifest.md
    host-api.md
    lifecycle.md
    device-adaptation.md
    capability-model.md
    bootstrap-phase-1.md
  /ports
    /host
      host_main.cpp
      host_logger.hpp
      host_logger.cpp
      filesystem_app_package_source.hpp
      filesystem_app_package_source.cpp
    /zephyr
      CMakeLists.txt
      prj.conf
      zephyr_main.cpp
      zephyr_logger.hpp
      zephyr_logger.cpp
      zephyr_app_package_source.hpp
      zephyr_app_package_source.cpp
      zephyr_device_packages.hpp
      zephyr_board_packages.hpp
      zephyr_board_packages.cpp
```

## 2. Two orthogonal axes

### App runtime axis

This axis owns:

- discovery of app packages from `/apps`
- manifest parsing into `AppManifest`
- registry construction in `AppRegistry`
- explicit session object creation in `AppSession`
- lifecycle history tracking and recovery-to-shell modeling in `AppSession`
- load/bringup/teardown/unload in `RuntimeLoader`
- host boundary and ownership tracking through `HostApi` and `ResourceOwnershipTable`

This axis stays device-agnostic. It only reasons about capability requirements, never board identity.

### Device adaptation axis

This axis owns:

- package selection through `DeviceRegistry`
- board-specific orchestration through `BoardPackage`
- system-facing device description through `DeviceProfile`
- runtime-queryable exposed functionality through `CapabilitySet`
- binding of generic services to concrete implementations through `ServiceBindingRegistry`

This axis absorbs board differences and produces generic service truth for shell and apps.

## 3. Module responsibilities

### Core

- `AegisCore`: resident system authority coordinating boot, launcher, app start, teardown, and return to shell
- `BootCoordinator`: chooses a device package, initializes board integration, binds services, and scans apps
- `DeviceRegistry`: authoritative registry of available `BoardPackage`s
- `AppRegistry`: scans filesystem app packages and performs static package validation
- `AppAdmissionPolicy`: applies runtime admission policy separately from registry scanning
- `AppCompatibilityEvaluator`: computes compatibility notes for shell-facing app presentation
- `AppRuntimePolicy`: carries runtime-governed policy truth such as background support
- `ManifestParser`: parses JSON manifest structure and emits explicit schema validation issues
- `AppCatalogBuilder`: shapes registry + admission truth into a shell-consumable app catalog
- `AppSession`: explicit lifecycle carrier for a running app instance

### Runtime

- `RuntimeLoader`: governed loader boundary with explicit `prepare`, `load`, `bringup`, `teardown`, and `unload` phases
- `LlextAdapter`: keeps binary loading substrate details below Aegis runtime semantics
- `HostApi`: system-owned capability access surface presented to apps
- `ResourceOwnershipTable`: tracks session-owned UI roots, timers, allocations, and other reclaimable runtime handles for teardown
- `lifecycle.hpp`: canonical lifecycle states from discovered to returned-to-shell

### Services

- generic interfaces remain app-facing and board-agnostic
- typed service contracts now exist for UI, text input, radio, GPS, audio, settings, notification, storage, power, time, and hostlink
- concrete service backends are bound by the current board package
- early phase uses console or in-memory stubs to prove architecture before real Zephyr/LLEXT integration
- Zephyr port now has explicit Zephyr-backed implementations for settings, timer, notification, storage, time, and power service domains
- Zephyr port now also binds formal Zephyr device-model adapters for radio, GPS, audio, and hostlink service status paths

### Device

- `CapabilitySet`: first-class capability truth with `absent/degraded/full`
- `DeviceProfile`: system-facing device image including display/input/storage/power/comm topology
- `ShellSurfaceProfile`: device-adaptation-owned shell extension truth for settings/status/notifications
- `BoardPackage`: board-specific initialization, service binding, shell extension convergence point, and device-specific input/navigation truth source
- mock packages prove that the same core can run on at least two different profiles

### Shell

- `AegisShell`: adapts launcher, settings, status, and notification surfaces from `DeviceProfile`
- `ShellController`: owns shell navigation/control flow across home, launcher, app foreground, and recovery
- `ShellInputModel`: device-shaped abstract shell navigation actions such as next/select/back/settings
- `ShellNavigationState`: explicit shell-owned foreground/navigation state
- `LauncherModel`: shell-owned app list with ordering, visibility state, and focus selection
- `SettingsModel`: device-sensitive settings page visibility model
- `StatusModel` / `NotificationModel`: shell-owned system surfaces distinct from app UI
- host bootstrap can now drive shell navigation through abstract shell actions instead of only direct app-id launching

## 4. Device profile examples

### Mock Device A

- display: `320x240`, compact
- input: `5-way joystick`
- radio: `full`
- GPS: `full`
- keyboard: `absent`
- shell hint: card-grid launcher

### Mock Device B

- display: `480x320`, wide
- input: `hardware keyboard`
- radio: `absent`
- GPS: `absent`
- removable storage: `full`
- USB hostlink: `full`
- shell hint: dense list launcher

## 5. Minimal runnable mainline

The current Phase 1 host + Zephyr-port bootstrap proves the following path:

1. `ports/host/host_main.cpp` chooses a board package id such as `mock_device_a` or `mock_device_b`.
2. `BootCoordinator` resolves the package, runs board-specific init, creates a `DeviceProfile`, and binds generic services.
3. `AppRegistry` scans `/apps`, parses `manifest.json`, validates field types / capability ids / permission ids, rejects duplicate `app_id`s, and builds the launcher-visible registry.
4. `BoardPackage` now provides a `ShellSurfaceProfile`, so device-specific settings/status/notification surfaces come from device adaptation instead of being hardcoded inside the shell.
5. `AppCatalogBuilder` shapes app registry truth into a shell-consumable catalog instead of letting `AegisCore` hand-assemble launcher entries inline.
6. `AegisShell` configures home, settings, status, notification, and launcher behavior from `DeviceProfile + ShellSurfaceProfile` instead of hardcoded board branches.
7. `ShellInputModel` now exposes abstract navigation actions derived from device adaptation instead of assuming a single input style.
8. `ShellController` now owns shell navigation through `home -> launcher -> app foreground -> recovery -> launcher`.
9. `AppAdmissionPolicy` admits an app only if static validation, ABI, capability, heap budget, UI budget, singleton policy, and entry-symbol policy all pass.
10. `AppCompatibilityEvaluator` computes launcher-visible compatibility notes for optional/preferred capability gaps and runtime-policy limitations.
11. `RuntimeLoader` moves an `AppSession` through `prepare -> load -> bringup -> teardown -> unload`.
12. A stub `LlextAdapter` already sits under the loader so the runtime/service/session architecture is separated from the future real Zephyr LLEXT backend.
13. The selected app runs through `HostApi`, queries `CapabilitySet`, and never checks board name.
14. `ResourceOwnershipTable` reclaims only still-owned runtime resources; app-side active cleanup can now release UI roots, timers, and allocations before teardown.
15. Typed service gateway contracts now cover UI, radio, GPS, audio, settings, notification, storage, power, time, and hostlink.
16. Runtime policy now governs explicit permission requests, and `demo_hostlink` proves that denied permissions remain visible in the launcher catalog without being admitted into execution.
17. Host API permission enforcement now also remains active during runtime service calls, so service domains such as radio, GPS, audio, hostlink, notification posting, storage access, settings writes, and text-input focus are not governed only by admission-time checks.
18. `ports/zephyr/` now assembles explicit Zephyr-specific board packages so Zephyr-backed service binding can diverge from host/mock composition without changing the higher platform layers.
19. A Zephyr-specific `LlextAdapter` skeleton now sits behind the same loader boundary, so the Zephyr port can advance toward real LLEXT integration without collapsing runtime semantics into port glue.
20. Manifest validation now emits structured issue lists, and invalid app entries can surface schema diagnostics through the launcher catalog instead of collapsing all failures into opaque parse errors.
21. `AppSession` now records lifecycle history and explicitly models recovery back to shell on pre-load failure paths.
22. Runtime policy now carries explicit permission grant entries with human-readable denial reasons, instead of only a flat allow-list.
23. Background execution requests are now formal runtime-governance decisions, and `demo_background` proves that unsupported background policy is surfaced as an admission failure.
24. Launcher focus can now move through visible entries and request app launch through abstract shell actions such as `select`, `move_next`, and `move_previous`.
25. Zephyr target now uses an adapter-backed `LlextLoaderBackend` rather than wiring Zephyr through the host stub loader path.
26. `ZephyrLlextAdapter` now attempts real `llext` + `fs_loader` integration first. Compiled contract fallback is no longer implicit; it is controlled through `AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK`, so the bootstrap path can be pushed back as real `.llext` app modules come online.
27. App sources are now split into ABI-entry module units and compiled fallback contract units. This keeps the app-facing build path aligned with `app.llext` packaging instead of forcing the resident core to own app code forever.
28. Zephyr board packages now use formal Zephyr service adapters for radio/GPS/audio/hostlink status binding instead of continuing to expose `Mock*Service` names at the Zephyr adaptation layer.
29. The Zephyr port now carries a concrete `zephyr_tlora_pager_sx1262` device package plus a T-LoRa-Pager bringup overlay. This is still an intermediate board port, but it moves device adaptation from generic alias probing toward an explicit real-target configuration surface.
30. Zephyr boot now includes a formal `appfs` bootstrap that mounts LittleFS on the board `storage_partition`, ensures `/lfs/apps` exists, and points runtime app discovery at a system-declared mount path instead of an implicit host build directory.
31. Zephyr app-module packaging now also emits a deploy-shaped tree under `build-zephyr/deploy/lfs/apps`, so the runtime-facing `/lfs/apps/<app-id>/` package layout is explicit in the build graph even before the board-flash population step is finished.

## 6. Next phase, not implemented yet

- replace Zephyr adapter placeholder image bookkeeping with real LLEXT-backed binary loading and symbol resolution
- formalize entry-symbol resolution and image metadata checking against real Zephyr/LLEXT loader state
- replace Zephyr bootstrap device-name adapters for radio, GPS, audio, and hostlink with final board-backed integrations for real targets
- add richer shell navigation and explicit launcher selection instead of auto-run-first-app
- connect settings, storage, timers, and UI ownership to real Zephyr subsystems
