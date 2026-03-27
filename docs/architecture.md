# Aegis Architecture

## 1. System definition

Aegis is a **resident device runtime** built for MCU systems.

It is not just a launcher, and not just a plugin mechanism.  
It is a governed runtime where:

- hardware belongs to the system,
- settings belong to the system,
- services belong to the system,
- apps are native loadable guests,
- and lifecycle is explicitly controlled by the runtime.

Aegis uses **Zephyr** as the lower OS and hardware governance layer, and builds a native app platform on top of it.

---

## 2. Layered architecture

```text
┌──────────────────────────────────────────────────────────────┐
│                         User / Operator                      │
└──────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                    Aegis Shell / System UI                   │
│  Home / Launcher / Settings / Status / Notifications         │
└──────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                     Aegis App Platform                       │
│  App Registry / Manifest / Icon Catalog / Session Mgmt       │
│  ABI Check / Permission Check / Crash Return Paths           │
└──────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                    Aegis Native Runtime                      │
│  Loader / Host API Binding / Resource Ownership / Teardown   │
│  LLEXT Adapter / Symbol Policy / Session Bringup             │
└──────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                  Aegis Service Gateway                       │
│  UI / Storage / Radio / GPS / Audio / Net / Settings         │
│  Timers / Logging / Notifications / Power                    │
└──────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                         Zephyr Base                          │
│  Kernel / Threads / Workqueue / Filesystem / Drivers         │
│  Settings / Timing / Sync / Memory / Power / LLEXT           │
└──────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                       Hardware / Board                       │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. Layer responsibilities

### 3.1 Zephyr Base

Zephyr provides the lower substrate:

- kernel and scheduling
- threads and workqueues
- synchronization
- filesystem
- settings subsystem
- device drivers
- timing and memory primitives
- power management
- LLEXT support

This layer is the **substrate**, not the user-facing platform.

### 3.2 Aegis Core

Aegis Core is the resident system authority.

It is responsible for:

- boot orchestration
- storage mount and app scan
- system settings coordination
- service initialization
- runtime policy
- app registry construction
- crash return behavior
- shell coordination

### 3.3 Aegis Service Gateway

This is one of the most important boundaries in the system.

Apps do not directly control drivers or raw hardware objects.  
Instead, they use system-owned capabilities exposed through controlled interfaces.

Typical service domains include:

- UI
- storage
- radio
- GPS
- audio
- network
- notifications
- timers
- logging
- settings
- power status

This layer preserves **hardware sovereignty** for the system.

### 3.4 Aegis Native Runtime

The runtime bridges native apps to the system.

It is responsible for:

- loading native app binaries
- binding the Host API
- managing symbol/export policy
- creating app sessions
- tracking resource ownership
- controlling teardown and unload order

The runtime is not merely “load code and jump”.  
It turns loadable native code into a **governed app session**.

### 3.5 Aegis Shell

The shell is the visible surface of the system.

It includes:

- home
- launcher
- settings
- app entry surfaces
- status bar
- notifications
- return paths after exit or failure

The shell is how users perceive the governed world of Aegis.

The shell should expose a renderer-independent presentation contract that
includes page data, page commands, and softkey declarations.
Renderers consume this contract.
They do not infer softkey meaning from page type.

---

## 4. System principle: hardware belongs to the system

This is the defining rule of Aegis.

Apps do not directly own:

- radio
- GPS
- power state
- driver handles
- raw global UI state
- long-lived timers outside runtime governance

The system owns them.

Apps can only use hardware-related capabilities through the Host API and service gateway.

This is what makes unload, teardown, and recovery possible.

---

## 5. Host API philosophy

The Host API is not only a technical interface.  
It is also a governance boundary.

The Host API should be:

- explicit
- small
- stable
- capability-oriented
- versioned

Typical host-facing domains include:

- logging
- runtime-managed allocation
- UI root creation
- timers
- storage access
- settings get/set
- service calls
- notifications
- publish/subscribe where appropriate

Its purpose is to ensure the system always knows:

- what resources an app owns,
- what needs cleanup on exit,
- what an app is allowed to do,
- and how the system can evolve without giving away hardware authority.

The same rule applies to foreground UI:
apps may describe page intent, but they should not directly own shell chrome.

---

## 6. Boot model

A typical boot sequence looks like this:

1. Zephyr Base boots
2. drivers, filesystem, settings, and runtime substrate initialize
3. Aegis Core starts
4. storage is mounted
5. `/apps` is scanned
6. manifests and icons are read
7. app registry is built
8. launcher is shown

At this point, apps are **present**, but not yet **running**.

Presence is not execution.

---

## 7. App start model

When a user selects an app:

1. Aegis validates metadata and ABI compatibility
2. runtime budget and policy checks run
3. an `AppSession` and `AppContext` are created
4. the native binary is loaded
5. Host API is bound
6. runtime bringup occurs
7. the app gains a foreground session
8. the shell transfers control to the app surface

---

## 8. Exit and recovery model

When an app exits or fails:

1. active session ownership is revoked
2. timers are canceled
3. subscriptions and callbacks are removed
4. UI roots are destroyed
5. runtime teardown runs
6. the binary is unloaded
7. the app context is reclaimed
8. control returns to shell / launcher

This explicit recovery path is one of the main reasons Aegis exists.

---

## 9. Summary

Aegis is best understood as:

- a resident device runtime,
- a governed native app platform,
- a system-owned hardware world,
- and a clear separation between **system authority** and **app workload**.

---

## 10. Related design references

The architecture document gives the system-wide map.
The detailed runtime constraints that repeatedly matter during app debugging are
documented separately in:

- [Aegis App Runtime Memory Model](./app-runtime-memory-model.md)
- [Aegis LLEXT App Constraints](./llext-app-constraints.md)
- [Aegis Foreground Page Contract](./foreground-page-contract.md)
- [Aegis App Asset and Icon Contract](./app-assets-and-icons.md)

These documents should be treated as implementation-facing companions to the
main architecture, especially when investigating foreground app failures,
unload safety, or package ownership mistakes.
