# Aegis Phase 0-1 Bootstrap

## 1. Repository skeleton

```text
/
  CMakeLists.txt
  main.cpp
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
      app_manifest.hpp
      app_manifest.cpp
      app_registry.hpp
      app_registry.cpp
    /app_session
      app_session.hpp
      app_session.cpp
  /runtime
    /loader
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
```

## 2. Two orthogonal axes

### App runtime axis

This axis owns:

- discovery of app packages from `/apps`
- manifest parsing into `AppManifest`
- registry construction in `AppRegistry`
- explicit session object creation in `AppSession`
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
- `AppRegistry`: scans filesystem app packages and performs capability admission checks
- `AppSession`: explicit lifecycle carrier for a running app instance

### Runtime

- `RuntimeLoader`: governed loader boundary with explicit `load`, `bringup`, `teardown`, and `unload` phases
- `HostApi`: system-owned capability access surface presented to apps
- `ResourceOwnershipTable`: tracks session-owned UI roots and notifications for teardown
- `lifecycle.hpp`: canonical lifecycle states from discovered to returned-to-shell

### Services

- generic interfaces remain app-facing and board-agnostic
- concrete service backends are bound by the current board package
- early phase uses console or in-memory stubs to prove architecture before real Zephyr/LLEXT integration

### Device

- `CapabilitySet`: first-class capability truth with `absent/degraded/full`
- `DeviceProfile`: system-facing device image including display/input/storage/power/comm topology
- `BoardPackage`: board-specific initialization and service binding convergence point
- mock packages prove that the same core can run on at least two different profiles

### Shell

- `AegisShell`: adapts launcher and visible settings surfaces from `DeviceProfile`
- `LauncherModel`: shell-owned app list for entry selection
- `SettingsModel`: device-sensitive settings page visibility model

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

The current Phase 1 stub proves the following path:

1. `main.cpp` chooses a board package id such as `mock_device_a` or `mock_device_b`.
2. `BootCoordinator` resolves the package, runs board-specific init, creates a `DeviceProfile`, and binds generic services.
3. `AppRegistry` scans `/apps`, parses `manifest.json`, and builds the launcher-visible registry.
4. `AegisShell` configures visible behavior from the chosen profile instead of hardcoded board branches.
5. `AegisCore` picks the first capability-compatible app and creates an `AppSession`.
6. `RuntimeLoader` moves that session through `load -> bringup -> teardown -> unload`.
7. The selected app runs through `HostApi`, queries `CapabilitySet`, and never checks board name.
8. `ResourceOwnershipTable` reclaims runtime-owned resources before the session reaches `returned to shell`.

## 6. Next phase, not implemented yet

- replace stub app entry resolution with real LLEXT-backed binary loading
- formalize manifest schema validation and ABI/version negotiation
- add typed service gateway contracts instead of descriptive strings
- introduce permission/admission policy separate from registry scanning
- add richer shell navigation and explicit launcher selection instead of auto-run-first-app
- connect settings, storage, timers, and UI ownership to real Zephyr subsystems
