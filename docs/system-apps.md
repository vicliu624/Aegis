# Aegis Built-in System Entries

## 1. Purpose

This document defines the built-in system-visible entries that ship with Aegis.

These entries are part of the product shape of Aegis.
They are not optional examples and they are not ordinary third-party apps.

This document exists to answer four questions clearly:

1. which entries are built into Aegis
2. what each built-in entry is responsible for
3. how built-in entries differ from normal apps
4. how the shell should present them to users

---

## 2. Design intent

The Aegis shell must not appear as a random list of plugins.

For ordinary users, Aegis should present a stable set of understandable system entry points.

These entries should feel like:

- native parts of the device
- stable and always available
- easy to understand without platform knowledge

They should not feel like:

- developer-only internals
- temporary bring-up surfaces
- random demo apps elevated into permanent system identity

---

## 3. Built-in entries vs normal apps

Built-in system entries are not the same thing as ordinary third-party products.
However, when one of these entries is implemented as an Aegis app package, it
should still use the same governed app mechanisms as any other app.

### 3.1 Built-in system entries

Built-in system entries are:

- system-owned
- product-defining
- expected to exist on every user-facing device build unless a device profile explicitly excludes one
- part of shell information architecture
- allowed to ship as standard app packages when that creates cleaner lifecycle and packaging boundaries

### 3.2 Normal apps

Normal apps are:

- discovered from storage
- admitted by manifest and runtime policy
- installable and removable
- not required to define the core identity of the system

### 3.3 Presentation rule

The shell may visually present built-in system entries in the same launcher language as apps if that improves user clarity.

However, semantically they remain system entries rather than ordinary guest workloads.

Implementation rule:

- being built-in does not justify bypassing manifest, app package, runtime admission, Host API, or service ABI paths
- if a built-in entry is implemented as an app, it should exist as a real app package in the repository and in deployed storage

---

## 4. Non-negotiable rules

### Rule 1

The following entries are part of the Aegis product baseline and should be treated as built-in:

- Apps
- Settings
- Files
- Device
- Services
- Running
- Notifications
- Power
- Connectivity
- Diagnostics
- Store

### Rule 2

Built-in entries are system-owned and are not uninstallable by ordinary app removal flows.

### Rule 3

Built-in entries may be visually hidden from Home on some device profiles, but they still remain system features unless explicitly removed by product policy.

### Rule 4

Built-in entries should use short, user-readable names.

### Rule 5

Built-in entries must not be confused with demo apps, diagnostics samples, or development tooling that is only present during bring-up.

---

## 5. Built-in entry list

The built-in user-facing system entry set is:

| Entry | Category | Primary user meaning |
|---|---|---|
| `Apps` | Catalog | See installed applications |
| `Settings` | System control | Change device and system behavior |
| `Files` | Storage | Browse files and storage |
| `Device` | Device info | See device identity and system summary |
| `Services` | System state | Inspect system service status |
| `Running` | Runtime | See current and recoverable sessions |
| `Notifications` | System events | View alerts, notices, and results |
| `Power` | Device control | Battery, charging, sleep, and power state |
| `Connectivity` | Communications | Wireless and connection state |
| `Diagnostics` | Maintenance | Self-check, logs, and fault diagnosis |
| `Store` | Content acquisition | Browse and install software |

---

## 6. Built-in entry specifications

### 6.1 Apps

Purpose:

- present the complete installed application catalog
- provide entry into app launch and app detail flows

It is not:

- the app store
- a software recommendation page
- a random favorites view

Primary responsibilities:

- show installed apps
- show app compatibility state
- show policy-limited or incompatible status
- open app detail
- launch apps

Product policy:

- built-in
- not uninstallable
- should usually be reachable from Home

### 6.2 Settings

Purpose:

- expose system settings and control panel behavior

It is not:

- app-local preferences
- a hidden developer-only tuning page

Primary responsibilities:

- display settings
- input settings
- audio settings
- connectivity settings
- storage settings
- runtime or notification behavior

Product policy:

- built-in
- always present
- not uninstallable
- should be treated as a permanent shell entry

### 6.3 Files

Purpose:

- provide file and storage browsing

Primary responsibilities:

- browse local storage
- browse removable storage where present
- inspect files and folders
- support import, export, or package discovery flows if product policy allows

Product policy:

- built-in
- not uninstallable
- high-frequency user tool

Implementation notes:

- `Files` should ship as a standard Aegis app package with `manifest.json`, `icon.bin`, and `app.llext`
- it should launch through the normal runtime loader path
- storage browsing should use the storage service ABI rather than shell-private models
- the shell may expose a direct `Files` tile, but that tile should resolve to an app launch request
- app-owned UI resources such as launcher icon, file glyphs, and page icons should live in the `Files` package rather than in shell or port-private asset buckets
- the repository package itself should remain structurally complete for discovery and host-side tooling, which means the `Files` directory should carry the same package-level files expected of other app packages even when Zephyr deploy builds later replace the placeholder `app.llext` with a real module image

### 6.4 Device

Purpose:

- explain what device the user is holding and what state it is in

Primary responsibilities:

- show product identity
- show firmware version
- show storage summary
- show device capability summary
- show high-level device health and policy notes

Product policy:

- built-in
- not uninstallable

### 6.5 Services

Purpose:

- expose system-owned service state in user-readable language

Primary responsibilities:

- show readiness and health of major services
- show degraded or unavailable subsystems
- provide entry into service detail views

Typical service domains:

- radio
- storage
- audio
- location
- networking
- notifications

Product policy:

- built-in
- not uninstallable
- may be second-level on simpler products, but must remain accessible

### 6.6 Running

Purpose:

- expose runtime-managed app session state

Primary responsibilities:

- show current foreground app
- show suspended or recoverable sessions if supported
- show recent exit or failure state where useful

This entry is important because it makes Aegis feel like a governed runtime rather than a static launcher.

Product policy:

- built-in
- not uninstallable

### 6.7 Notifications

Purpose:

- present shell-owned notices, alerts, reminders, and task results

Primary responsibilities:

- show recent system notices
- show app-generated notices presented by system policy
- show warning and completion results

It is distinct from messaging.

`Notifications` means:

- system events
- alerts
- reminders
- results

It does not mean:

- user conversations
- message threads

Product policy:

- built-in
- not uninstallable
- should remain easy to reach

### 6.8 Power

Purpose:

- provide battery and power-state visibility

Primary responsibilities:

- show battery state
- show charging state
- show low-power status
- expose sleep or power behavior controls where supported

Product policy:

- built-in
- not uninstallable

### 6.9 Connectivity

Purpose:

- unify major communications and connection surfaces under one user-readable entry

Primary responsibilities:

- show connection state
- show wireless state
- expose radio or network control paths at a user level
- provide entry into communication-specific details

This entry may aggregate:

- radio
- bluetooth
- network
- host link if product-facing

Product policy:

- built-in
- not uninstallable

### 6.10 Diagnostics

Purpose:

- provide self-check, logs, health inspection, and failure diagnosis

Primary responsibilities:

- run self-checks
- show hardware and subsystem health
- expose logs or diagnostic summaries
- support fault investigation

This is a system maintenance surface.
It should not be confused with ordinary daily-use tools.

Product policy:

- built-in
- not uninstallable
- usually not a first-screen Home entry

### 6.11 Store

Purpose:

- provide a governed software acquisition surface

Primary responsibilities:

- browse available software
- inspect compatibility before install
- inspect version and package source
- begin install flows

Store in Aegis should not imitate a modern mobile app market.

It should act as:

> a system-governed software catalog

It must make visible:

- compatibility
- source trust
- install size
- policy limitations where relevant

Product policy:

- built-in
- not uninstallable
- distinct from `Apps`

---

## 7. Built-in entry behavior matrix

| Entry | Built-in | Uninstallable | Usually visible in Home | Always visible somewhere in shell |
|---|---|---|---|---|
| `Apps` | yes | no | yes | yes |
| `Settings` | yes | no | yes | yes |
| `Files` | yes | no | yes | yes |
| `Device` | yes | no | maybe | yes |
| `Services` | yes | no | maybe | yes |
| `Running` | yes | no | maybe | yes |
| `Notifications` | yes | no | maybe | yes |
| `Power` | yes | no | maybe | yes |
| `Connectivity` | yes | no | maybe | yes |
| `Diagnostics` | yes | no | usually no | yes |
| `Store` | yes | no | yes or maybe | yes |

---

## 8. Home placement guidance

Not every built-in entry should appear on the first Home screen.

For low cognitive load, first-screen Home should generally prioritize high-frequency entries such as:

- Apps
- Files
- Store
- Settings

Device-profile-specific products may additionally prioritize:

- Connectivity
- Notifications
- Power

Second-level or secondary Home pages may contain:

- Device
- Services
- Running
- Diagnostics

The exact first-screen arrangement may vary by product, but the built-in list itself should remain stable.

---

## 9. Relationship to app registry

Built-in system entries remain system-owned product features, but some of them
may and should be implemented as ordinary Aegis app packages when that creates
cleaner lifecycle, packaging, and resource boundaries.

That means the shell may keep ownership of:

- information architecture
- ordering and visibility policy
- uninstall policy
- launcher prominence

while still delegating execution to:

- manifest-discovered app packages
- normal runtime admission
- Host API and service ABI paths

This preserves the distinction between:

- shell/system authority over product structure
- app/runtime governance over executable workloads

---

## 10. Naming guidance

The built-in entry names should remain short and ordinary-user-readable:

- Apps
- Settings
- Files
- Device
- Services
- Running
- Notifications
- Power
- Connectivity
- Diagnostics
- Store

Do not rename these casually into more technical titles such as:

- Runtime Sessions
- Service Gateway
- Capability Inspector
- Board Control

Those may appear inside detail views, but not as primary user-facing entry names.

---

## 11. Summary

The built-in Aegis system entry set is part of the product definition of Aegis.

It gives the user a stable, understandable system surface while preserving the deeper architectural rule:

> the system owns the world,  
> and apps live inside that world rather than defining it.
