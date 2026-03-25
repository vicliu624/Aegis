# Aegis

> A native app runtime and governed device platform for MCU systems.

## What is Aegis?

**Aegis** is a native app runtime and device platform for MCU-based systems.

It is built on a simple principle:

- the **system** owns the hardware,
- the **system** governs settings, services, and lifecycle,
- and **apps** are independently built native extensions that are discovered, loaded, run, and unloaded at runtime.

Aegis is not a traditional desktop operating system.  
It is not a simple launcher either.

It is a controlled runtime for constrained devices.

---

## Why Aegis exists

Most MCU firmware is built as a single monolithic image.

Over time, every screen, tool, experiment, protocol path, and utility gets compiled into the same binary. The result is familiar:

- firmware grows without clear boundaries,
- system code and app code become entangled,
- hardware access leaks everywhere,
- lifecycle becomes hard to reason about,
- and adding one more feature means touching the whole world again.

Aegis takes a different path.

It treats the device as a **small governed world**:

- a **resident system** remains in control,
- hardware is managed centrally,
- services are exposed through explicit interfaces,
- and apps exist as **native loadable units** stored in the filesystem.

At boot, Aegis scans the app directory, builds an app catalog, and presents apps to the user.  
When an app is selected, Aegis loads it, binds it to the host API, creates its session, and gives it a controlled place to run.  
When the app exits, Aegis tears it down and reclaims its resources.

This is not “firmware with many pages”.  
This is a **device runtime with native apps**.

---

## Core ideas

### Hardware belongs to the system

Drivers, board resources, and device coordination remain under system authority.  
Apps consume capabilities, not raw ownership.

### Apps are native, but not sovereign

Apps are independently built native binaries, but they are still guests inside the runtime.  
They do not define the machine.  
They run within it.

### Runtime loading is a first-class concept

Apps are discovered from storage, not hardwired into the main firmware menu.  
Installation and presence are represented by files and metadata.

### Lifecycle is explicit

Loading, startup, active session, exit, teardown, unload, and recovery are not incidental behaviors.  
They are part of the architecture.

### System and app must remain distinct

The system is the long-lived authority.  
Apps are replaceable workloads.

---

## High-level structure

Aegis is organized around these major parts:

- **Zephyr Base**: OS substrate, drivers, filesystem, settings, scheduling, memory, power
- **Aegis Core**: resident system authority
- **Aegis Services**: controlled gateway for system-owned capabilities
- **Aegis Runtime**: native app loading and session runtime
- **Aegis Shell**: launcher, home, settings, status surfaces

---

## Repository layout

```text
/core        Resident system core
/runtime     Native app runtime and loader integration
/services    System-owned service gateway
/shell       Launcher, settings, home, visible system surfaces
/sdk         Headers and build support for Aegis apps
/apps        Example or built-in development apps
/docs        Architecture notes and design documents
```

---

## Status

Aegis is in active design and early implementation.

The architecture is being shaped around one central idea:

> the device is not a bag of pages;  
> it is a governed runtime, and apps are guests within it.

---

## Documents

- [Architecture](./docs/architecture.md)
- [App Model](./docs/app-model.md)
- [Zephyr Build Guide](./docs/building-zephyr.md)

---

## License

MIT
