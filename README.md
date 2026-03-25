# Aegis

[中文说明](./README.zh-CN.md)

> A native app runtime and governed device platform for MCU systems.

## What is Aegis?

**Aegis** is a native app runtime and governed device platform for MCU-based systems.

It is built on a simple principle:

- the **system** owns the hardware,
- the **system** governs settings, services, and lifecycle,
- and **apps** are independently built native extensions that are discovered, loaded, run, and unloaded at runtime.

Aegis is not a traditional desktop operating system.
It is not a simple launcher either.

It is a controlled runtime for constrained devices.

---

## Current bring-up path

The repository currently has one primary bring-up path that new contributors should follow:

- Zephyr-based resident system bring-up
- LilyGo T-LoRa Pager and LilyGo T-Deck hardware adaptation paths
- runtime-loaded native apps packaged into appfs
- top-level build and flash entrypoint through [scripts/build_zephyr.py](./scripts/build_zephyr.py)

If you are entering the repository for the first time, treat the Zephyr path above as the current
authoritative build flow.

---

## Getting started

If your goal is to get the current Zephyr target working on a new machine, use this order:

1. Run the host environment self-check.
2. Read the quick start.
3. Complete onboarding if the self-check reports missing prerequisites.
4. Configure, build, and flash through the top-level build driver.

### First command

Windows:

```powershell
python scripts/build_zephyr.py check-host-env
```

Linux, macOS, or WSL:

```bash
python3 scripts/build_zephyr.py check-host-env
```

If you want machine-readable output for automation:

Windows:

```powershell
python scripts/build_zephyr.py check-host-env --json
```

Linux, macOS, or WSL:

```bash
python3 scripts/build_zephyr.py check-host-env --json
```

### Recommended reading order

Read these in order:

1. [Zephyr Quick Start](./docs/quick-start-zephyr.md)
2. [Zephyr Onboarding](./docs/zephyr-onboarding.md)
3. [Zephyr Build Guide](./docs/building-zephyr.md)
4. [Architecture](./docs/architecture.md)

If your next goal is shell and interface work, continue with:

- [Aegis Shell](./docs/shell.md)
- [Aegis System UI Design](./docs/ui-system.md)
- [Aegis Built-in System Entries](./docs/system-apps.md)
- [Aegis Visual Specification](./docs/ui-visual-spec.md)
- [Aegis UI Icon and Image Inventory](./docs/ui-icon-inventory.md)
- [Aegis UI Patterns and Interaction Spec](./docs/ui-patterns.md)
- [Aegis Shell Wireframes and Screen Contracts](./docs/ui-wireframes.md)

### Primary build entrypoint

The repository's current user-facing Zephyr entrypoint is:

- [scripts/build_zephyr.py](./scripts/build_zephyr.py)

Use it for:

- host checks
- configure
- firmware build
- appfs packaging
- firmware flash
- appfs flash

The lower-level helper scripts still exist, but they are no longer the recommended first entrypoint
for normal bring-up work.

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

This is not "firmware with many pages".
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

## How Aegis compares to existing approaches

Aegis does not appear out of nowhere.
It stands near several existing embedded ideas, but it is not identical to any of them.

### Zephyr + LLEXT

This is the closest technical foundation.

Zephyr LLEXT already provides the low-level ability to load independently built native extensions at runtime.
That means it solves an important part of the problem: **runtime loading of native code**.

But LLEXT alone is still only a mechanism.
It does not by itself define:

- app identity,
- manifest structure,
- catalog discovery,
- capability contracts,
- host API semantics,
- lifecycle governance,
- device adaptation rules,
- or system-shell behavior.

Aegis takes that lower-level mechanism and raises it into a **full device app platform**.

### NuttX loadable modules

NuttX shows another nearby path: embedded systems can support externally loadable program units.

That is similar to Aegis in one important sense:
apps do not have to be permanently fused into the main firmware image.

But the architectural emphasis is different.

NuttX is closer to:

> a small operating system that can run separately linked programs

Aegis is closer to:

> a governed device runtime where apps are admitted as structured guests under system authority

The difference is not just technical.
It is about who owns the device world.

### Tock

Tock is highly relevant as a reference for isolation, authority boundaries, and controlled execution on embedded systems.

It is useful for understanding how the system can remain in charge while applications remain constrained.

But Tock's center of gravity is:

- memory safety,
- protection boundaries,
- kernel/userspace structure,
- and secure isolation.

Aegis has a different center of gravity:

- native runtime loading,
- product-facing app lifecycle,
- filesystem-backed app presence,
- capability-governed services,
- and a resident system shell.

So Tock is philosophically adjacent, but not the same product shape.

### Embedded WebAssembly runtimes

Wasm runtimes for embedded devices are also close in spirit.

They explore:

- portable app modules,
- host-provided APIs,
- execution boundaries,
- and runtime extensibility.

That overlaps with Aegis conceptually.

But Aegis chooses a different primary path:

- **native extensions first**,
- not a virtual instruction format first.

In that sense, Wasm-based platforms are a neighboring route toward extensibility, while Aegis is a native-runtime route.

---

## What is unique about Aegis

Aegis is not unique because "dynamic loading" is new.
Dynamic loading already exists in multiple forms.

What is unique is the **combination** of ideas and the architectural stance behind them.

### 1. Aegis is not just a loader

A loader can bring code into memory.
Aegis does more than that.

It defines a governed runtime around the loaded app:

- discovery,
- app cataloging,
- metadata interpretation,
- host binding,
- session creation,
- controlled execution,
- exit handling,
- teardown,
- unload,
- and recovery.

In Aegis, loading is only one phase of a larger app lifecycle.

### 2. Aegis is not "firmware with plugins"

Many systems use the word plugin, but the plugins still end up directly depending on board details, driver assumptions, and private system internals.

Aegis rejects that model.

Apps should not become hidden extensions of board code.
They should remain apps.

That means the boundary between system and app is not accidental.
It is architectural.

### 3. Aegis keeps hardware ownership in the system

This is one of the most important differences.

In Aegis:

- the system owns hardware,
- the system mediates services,
- the system governs settings,
- and apps consume capabilities instead of grabbing raw ownership.

That changes the meaning of extensibility.

The app is powerful enough to do useful work,
but not sovereign enough to redefine the machine.

### 4. Aegis is designed for hardware diversity

Aegis assumes a practical embedded reality:

- different screens,
- different input methods,
- different radios,
- different sensors,
- different storage layouts,
- different memory budgets,
- different boards.

Most extension systems become fragile exactly here.
Board variance leaks upward and every app becomes a hardware negotiation.

Aegis aims to absorb that variance through:

- device adaptation,
- capability mapping,
- and host-governed interfaces.

This is essential to making a real multi-device MCU app platform.

### 5. Aegis turns runtime extensibility into a product architecture

Many existing mechanisms are technical features.
Aegis tries to make extensibility a **system-level architectural principle**.

That means the device is understood as:

- a resident world,
- with long-lived authority,
- controlled services,
- explicit lifecycle,
- and replaceable native workloads.

This is the deeper shift.

Aegis is not asking,
"Can MCU code be loaded dynamically?"

It is asking,
"What does an embedded device become once runtime-loaded apps are treated as first-class architectural citizens?"

---

## In one sentence

If Zephyr LLEXT provides a low-level loading mechanism,
Aegis aims to provide the **full governed app platform** built on top of that mechanism.

---

## What Aegis is not

Aegis is **not**:

- a desktop operating system for microcontrollers,
- a Linux-style package environment,
- a loose plugin model with unrestricted hardware access,
- a menu system pretending to be an app platform,
- or a monolithic firmware with more pages added over time.

Aegis is a **governed MCU runtime with native apps**.

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
/ports       Platform ports, including Zephyr board-family adaptations
```

For the active Zephyr hardware bring-up path, board-family code now lives under:

- `ports/zephyr/boards/tlora_pager`
- `ports/zephyr/boards/tdeck`

---

## Status

Aegis is in active design and early implementation.

The architecture is being shaped around one central idea:

> the device is not a bag of pages;
> it is a governed runtime, and apps are guests within it.

---

## Documents

- [Zephyr Quick Start](./docs/quick-start-zephyr.md)
- [Architecture](./docs/architecture.md)
- [App Model](./docs/app-model.md)
- [Zephyr Onboarding](./docs/zephyr-onboarding.md)
- [Zephyr Build Guide](./docs/building-zephyr.md)

---

## License

MIT
