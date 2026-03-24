# Aegis App Manifest Specification

## 1. Purpose

In Aegis, an app is not recognized merely because a binary exists in the filesystem.

An app becomes a valid Aegis app only when the system can:

- identify it,
- understand its metadata,
- check its compatibility,
- understand its runtime expectations,
- and decide whether it may be loaded.

The manifest is the formal document that makes this possible.

It is the admission contract between an app package and the Aegis runtime.

---

## 2. Role of the manifest

The manifest serves four purposes:

### 2.1 Identity
It tells the system what the app is.

### 2.2 Compatibility
It tells the runtime whether the app is compatible with the current system ABI and runtime version.

### 2.3 Admission policy
It tells the runtime what capabilities the app requires, what capabilities are optional, and what minimum conditions must be satisfied before startup.

### 2.4 Presentation
It tells the shell how to present the app in the launcher and related system surfaces.

The manifest is not a decorative metadata file.  
It is part of runtime governance.

---

## 3. Location

Each app lives in its own app directory.

Example:

```text
/apps/
  aprs_tool/
    manifest.json
    icon.bin
    app.llext
````

The manifest must live at:

```text
/apps/<app_id>/manifest.json
```

The runtime must treat the manifest as the authoritative metadata source for that app package.

---

## 4. Format

The initial Aegis manifest format should be JSON.

Reasons:

* easy to inspect manually
* easy to generate from build tools
* easy to parse on constrained systems with predictable structure
* good enough for early platform development

The format may evolve later, but the first implementation should stay simple and explicit.

---

## 5. Required fields

The following fields should be required in v0.1.

### 5.1 `app_id`

Stable unique identifier of the app.

Rules:

* lowercase
* ASCII
* no spaces
* use `_` or `-` if needed
* should match the logical package identity, not merely display name

Example:

```json
"app_id": "aprs_tool"
```

### 5.2 `display_name`

Human-readable app name shown in the shell.

Example:

```json
"display_name": "APRS Tool"
```

### 5.3 `version`

The app’s own version string.

Example:

```json
"version": "0.1.0"
```

### 5.4 `abi_version`

The Aegis app ABI version that this app targets.

This field is critical.

The runtime must reject apps whose ABI version is incompatible with the current runtime ABI.

Example:

```json
"abi_version": 1
```

### 5.5 `binary`

Relative path to the native app binary within the app package directory.

Example:

```json
"binary": "app.llext"
```

### 5.6 `entry_symbol`

Name of the entry symbol the runtime should resolve after loading.

Example:

```json
"entry_symbol": "aegis_app_entry"
```

### 5.7 `icon`

Relative path to the app icon asset.

Example:

```json
"icon": "icon.bin"
```

### 5.8 `description`

Human-readable short description.

Example:

```json
"description": "Field APRS tools and utilities."
```

---

## 6. Capability declaration fields

Aegis apps must declare what capabilities they need or prefer.

This is one of the core mechanisms that allows Aegis to remain multi-device without leaking board identity into app code.

### 6.1 `required_capabilities`

Capabilities that must be available for the app to start.

If any required capability is absent or below required level, the runtime must reject startup.

Example:

```json
"required_capabilities": [
  { "id": "display", "min_level": "full" }
]
```

### 6.2 `optional_capabilities`

Capabilities that the app can use if available, but does not require.

Example:

```json
"optional_capabilities": [
  { "id": "gps_fix", "min_level": "degraded" },
  { "id": "radio_messaging", "min_level": "degraded" }
]
```

### 6.3 `preferred_capabilities`

Capabilities that improve the experience, but are not required for startup.

These can be used for shell hints or compatibility messaging.

Example:

```json
"preferred_capabilities": [
  { "id": "keyboard_input", "min_level": "full" }
]
```

---

## 7. Runtime requirement fields

These fields help the runtime decide whether the app may be admitted.

### 7.1 `memory_budget`

A hint describing expected runtime memory use.

Initial v0.1 can keep this simple.

Example:

```json
"memory_budget": {
  "heap_bytes": 32768,
  "ui_bytes": 16384
}
```

This does not need to become perfect immediately.
But the concept must exist.

### 7.2 `singleton`

Indicates whether only one instance/session of the app may exist at a time.

Example:

```json
"singleton": true
```

### 7.3 `allow_background`

Whether the app may continue in background in future runtime models.

In v0.1, even if background execution is not implemented, the field may still exist for forward compatibility.

Example:

```json
"allow_background": false
```

---

## 8. Shell presentation fields

These fields help the shell present the app consistently.

### 8.1 `category`

Optional category string.

Example:

```json
"category": "tools"
```

### 8.2 `order_hint`

Optional numeric sort hint for launcher ordering.

Example:

```json
"order_hint": 100
```

### 8.3 `hidden`

Whether the app should be hidden from normal launcher display.

Useful for test apps, internal tools, or system-only apps.

Example:

```json
"hidden": false
```

---

## 9. Example manifest

```json
{
  "app_id": "aprs_tool",
  "display_name": "APRS Tool",
  "version": "0.1.0",
  "abi_version": 1,
  "binary": "app.llext",
  "entry_symbol": "aegis_app_entry",
  "icon": "icon.bin",
  "description": "Field APRS tools and utilities.",
  "required_capabilities": [
    { "id": "display", "min_level": "full" }
  ],
  "optional_capabilities": [
    { "id": "gps_fix", "min_level": "degraded" },
    { "id": "radio_messaging", "min_level": "degraded" }
  ],
  "preferred_capabilities": [
    { "id": "keyboard_input", "min_level": "full" }
  ],
  "memory_budget": {
    "heap_bytes": 32768,
    "ui_bytes": 16384
  },
  "singleton": true,
  "allow_background": false,
  "category": "tools",
  "order_hint": 100,
  "hidden": false
}
```

---

## 10. Admission rules

The runtime should use the manifest for admission control.

At minimum, startup admission should check:

1. manifest parse success
2. required fields present
3. `abi_version` compatibility
4. binary file exists
5. entry symbol exists
6. required capabilities satisfied
7. requested permissions are allowed by current runtime policy
8. memory budget does not violate current device/runtime policy

Permission requests should remain explicit in manifest data.

Examples:

* an app that wants to post notifications should request `notification_post`
* an app that wants to claim text-entry focus should request `text_input_focus`
* an app that wants to access radio, gps, audio, or hostlink service domains should request the matching `radio_use`, `gps_use`, `audio_use`, or `hostlink_use` permission

These permissions should influence both:

* startup admission
* Host API enforcement while the app session is running

Only after these checks pass may the app move from **Validated** to **Load Requested** / **Loaded**.

---

## 11. Validation policy

Manifest validation should be split into two stages.

### 11.1 Static validation

Done when scanning app packages.

Checks:

* JSON syntax
* required field presence
* field type correctness
* runtime-critical file references exist
* presentation resources are recorded as presentation warnings when absent or degraded

### 11.2 Runtime admission validation

Done when app startup is requested.

Checks:

* ABI compatibility
* capability satisfaction
* runtime memory policy
* singleton conflict
* other runtime policy checks

This separation matters because presence in registry is not the same as runtime admission.

Presentation metadata such as launcher icon assets must remain formal manifest concepts, but they must
not be confused with runtime-fatal admission requirements.

Examples:

* missing `binary` is a fatal validation error
* missing `icon` is a presentation warning; the shell should degrade to placeholder art
* malformed `icon` path should be surfaced to shell/catalog diagnostics without blocking app launch

---

## 12. Non-negotiable rules

### Rule 1

The runtime must never treat the binary alone as sufficient app identity.

### Rule 2

The manifest must be the single authoritative metadata source for app admission and presentation.

### Rule 3

Apps must declare capabilities, not board names.

### Rule 4

Manifest compatibility must be checked before loading binary code.

### Rule 5

Presentation fields must not be confused with admission fields.

### Rule 6

The manifest format should evolve conservatively; it is a runtime contract.

---

## 13. Future evolution

The manifest may later include:

* digital signature / integrity fields
* publisher metadata
* localization support
* migration hooks
* state restoration hints
* install-time dependency declarations
* app-specific settings schema references

These are not required in the first implementation.

The first implementation should stay focused on:

* identity
* compatibility
* capability declaration
* admission policy
* launcher presentation

---

## 14. Summary

The Aegis manifest exists to make one thing explicit:

> An app is not merely code in storage.
> An app is a runtime-admitted unit with identity, compatibility, declared expectations, and system-visible presentation.

That is why the manifest is foundational to Aegis.
