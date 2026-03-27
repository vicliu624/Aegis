# Aegis Runtime Refactor Slices

## 1. Purpose

This document translates the final isolated runtime blueprint into concrete
repository refactor slices.

It answers a practical question:

> If Aegis is going to become a strongly isolated managed native runtime,
> which parts of the current codebase must be split, renamed, narrowed, or moved
> into dedicated runtime subsystems?

This is not a phase plan.
It is a code-structure decomposition checklist for the final architecture.

The target blueprint is defined in
[Strongly Isolated App Runtime Blueprint](./isolated-app-runtime-blueprint.md).

---

## 2. How to read this document

Each slice below defines:

- target subsystem responsibility
- current code that partially covers it
- current coupling or ambiguity
- required end-state split

The end-state split is the important part.
The current code references exist so the repository can be systematically
reworked without losing track of where responsibilities are hiding today.

---

## 3. Target runtime subsystem map

The final runtime should be decomposed into these code-facing subsystems:

- admission
- package registry
- binary policy loader
- memory domain manager
- app supervisor
- syscall gateway
- service broker
- quota engine
- fault engine
- recovery engine
- quarantine registry
- audit log
- UI compositor bridge
- instance model and handle tables
- SDK syscall ABI
- Zephyr isolation backend

---

## 4. Refactor slice checklist

### Slice 1: Admission authority

Target responsibility:

- parse package metadata
- compute execution eligibility
- freeze permission and capability view
- reject unsupported or unsafe app packages before runtime load

Current code:

- [app_admission_policy.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_admission_policy.cpp)
- [app_compatibility_evaluator.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_compatibility_evaluator.cpp)
- [app_runtime_policy.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_runtime_policy.cpp)
- [app_manifest.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_manifest.cpp)
- [runtime_loader.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/loader/runtime_loader.cpp)

Current problem:

- admission logic is spread across registry and loader concerns
- manifest acceptance and runtime execution policy are not yet clearly separated
- loader still carries some implicit admission meaning

Required end-state split:

- `runtime/admission/`
- static package validation stays outside execution bringup
- admission produces an immutable `AdmittedAppPolicy`
- later runtime stages consume policy, not re-derive it ad hoc

Checklist:

- extract admission result object from app registry helpers
- separate "discoverable" from "launchable"
- define loader input as admitted policy plus package descriptor
- remove permission and capability re-interpretation from downstream runtime code

---

### Slice 2: Package registry and catalog

Target responsibility:

- discover packages
- parse package metadata
- build launcher-visible catalog
- remain separate from execution-state ownership

Current code:

- [app_catalog.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_catalog.cpp)
- [app_catalog_builder.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_catalog_builder.cpp)
- [app_registry.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_registry.cpp)
- [app_package_source.hpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_package_source.hpp)
- [zephyr_app_package_source.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_app_package_source.cpp)

Current problem:

- package discovery and runtime execution identity still sit conceptually too close together
- package cataloging is not yet cleanly separated from supervisor-owned running instances

Required end-state split:

- `runtime/package_registry/`
- registry owns package descriptors only
- registry never owns running app resources

Checklist:

- ensure catalog models do not retain runtime-owned pointers
- split package hash, package identity, and instance identity
- keep storage discovery code free of lifecycle state

---

### Slice 3: Binary policy loader

Target responsibility:

- perform binary admission checks
- enforce relocation and import policy
- map segments according to runtime memory rules
- bind only stable ABI exports

Current code:

- [runtime_loader.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/loader/runtime_loader.cpp)
- [llext_loader_backend.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/loader/llext_loader_backend.cpp)
- [llext_adapter.hpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/loader/llext_adapter.hpp)
- [zephyr_llext_adapter.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_llext_adapter.cpp)
- [compiled_app_contract_registry.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/loader/compiled_app_contract_registry.cpp)

Current problem:

- loader currently mixes image loading, contract resolution, and runtime activation
- LLEXT substrate details are too close to app runtime semantics
- symbol and relocation policy are not yet elevated into a dedicated admission layer

Required end-state split:

- `runtime/loader/` for runtime-facing policy
- `ports/zephyr/isolation_backend/` for Zephyr-specific load mechanics
- explicit binary policy checks before instance activation

Checklist:

- split binary validation from instance startup
- define authorized import table and relocation policy artifacts
- isolate Zephyr LLEXT-specific substrate behind a narrow backend interface
- forbid non-ABI resident symbol reach-through

---

### Slice 4: Memory domain manager

Target responsibility:

- allocate and track app text, rodata, data, bss, heap, stack, shared buffers, and UI arenas
- enforce per-app memory boundaries
- integrate guard regions and fault attribution

Current code:

- [host_api.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/host_api/host_api.cpp)
- [resource_ownership_table.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/ownership/resource_ownership_table.cpp)
- [app_session.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_session/app_session.cpp)
- Zephyr loader mapping paths in [zephyr_llext_adapter.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_llext_adapter.cpp)

Current problem:

- app allocation, ownership tracking, and binary residency are still described through several unrelated abstractions
- memory governance is not yet modeled as a first-class runtime subsystem

Required end-state split:

- `runtime/memory_domain/`
- one `AppMemoryDomain` per `AppInstance`
- no app-visible host raw allocation semantics

Checklist:

- replace generic host allocation framing with domain allocation framing
- define explicit region descriptors and ownership boundaries
- separate app arena accounting from generic resource ownership bookkeeping
- attach faults to domain regions rather than generic runtime events

---

### Slice 5: App supervisor

Target responsibility:

- own `AppInstance`
- drive lifecycle state machine
- track watchdog, foreground state, suspension, kill, reclaim, and quarantine transitions

Current code:

- [app_session.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_session/app_session.cpp)
- [lifecycle.hpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/lifecycle/lifecycle.hpp)
- parts of [runtime_loader.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/loader/runtime_loader.cpp)
- shell coordination currently spread into shell and runtime bringup code

Current problem:

- app session is still narrower than the final `AppInstance` object
- lifecycle authority is not yet centered in one supervisor abstraction
- teardown, return-to-shell, and resource reclaim logic remain too distributed

Required end-state split:

- `runtime/supervisor/`
- `AppInstance`, `AppSupervisor`, `AppLifecycleState`, `WatchdogState`

Checklist:

- promote app session into richer app instance model
- move lifecycle transitions behind supervisor API
- centralize foreground attach, suspend, kill, reclaim, and quarantine decisions
- stop using loader as de facto lifecycle authority

---

### Slice 6: Syscall gateway

Target responsibility:

- serve as the single legal app-to-system call boundary
- validate ABI payloads
- enforce permission, capability, handle, state, and quota checks before broker dispatch

Current code:

- [host_api.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/host_api/host_api.cpp)
- [host_api.hpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/host_api/host_api.hpp)
- [host_api_abi.h](C:/Users/VicLi/Documents/Projects/aegis/sdk/include/aegis/host_api_abi.h)

Current problem:

- "host api" currently carries both ABI surface and system-policy meaning
- it is not yet described as the only legal syscall gate
- host allocation and service dispatch concerns are mixed with policy checks

Required end-state split:

- `runtime/syscall/`
- `sdk/include/aegis/syscall_abi.h`
- host-facing C++ wrapper remains a client convenience layer, not the system boundary itself

Checklist:

- redefine host api as syscall ABI plus runtime gateway
- ensure every operation can be validated without trusting app memory beyond bounded payloads
- remove any hidden direct service reach-through

---

### Slice 7: Service broker

Target responsibility:

- mediate all system-owned capabilities through revocable handles
- maintain per-app service state and usage accounting

Current code:

- [service_gateway_dispatch.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/host_api/service_gateway_dispatch.cpp)
- service ABIs under [sdk/include/aegis/services](C:/Users/VicLi/Documents/Projects/aegis/sdk/include/aegis/services)

Current problem:

- dispatch currently reads as a helper beneath host api rather than a runtime-critical broker layer
- handle ownership, revoke behavior, and per-app rate limiting are not yet primary concepts

Required end-state split:

- `runtime/broker/`
- brokers per service domain or one broker core plus domain adapters

Checklist:

- formalize brokered handle ownership
- unify revoke-on-kill behavior
- put per-domain quotas and rate limits at broker layer
- separate broker policy from device adaptation logic

---

### Slice 8: Handle tables and instance-owned resources

Target responsibility:

- track every revocable cross-boundary resource by handle
- ensure handle ownership is per-app-instance, not global and informal

Current code:

- [resource_ownership_table.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/ownership/resource_ownership_table.cpp)
- scattered ownership relationships in host api and service dispatch

Current problem:

- ownership tracking exists, but not yet as a complete per-instance handle model
- ownership is broader and looser than the future runtime needs

Required end-state split:

- `runtime/handles/`
- typed handle tables plus revocation semantics

Checklist:

- split opaque resource ownership from typed app handle tables
- make handles the only long-lived app-visible reference form
- ensure kill path can revoke by app instance in one place

---

### Slice 9: Quota engine

Target responsibility:

- meter and enforce budgets for memory, UI, timers, queue depth, service rate, logs, and execution windows

Current code:

- manifest memory budget fields in [app_manifest.cpp](C:/Users/VicLi/Documents/Projects/aegis/core/app_registry/app_manifest.cpp)
- partial runtime checks currently spread across host api, UI, and runtime behavior

Current problem:

- budgets are partly declarative and not yet centered in a single runtime ledger
- no dedicated engine currently owns strikes, breach policy, and escalation

Required end-state split:

- `runtime/quota/`
- one quota ledger per app instance

Checklist:

- define quota categories as first-class runtime types
- separate declaration parsing from active enforcement
- define deterministic breach actions
- wire quota engine into syscall gateway, broker, UI, and supervisor

---

### Slice 10: Fault engine

Target responsibility:

- classify faults
- attribute faults to app instance, region, and service state
- feed recovery and quarantine policy

Current code:

- crash return and error handling are currently distributed
- loader, shell, and runtime each carry pieces of failure semantics

Current problem:

- no dedicated fault taxonomy subsystem
- fault handling is still too close to ad hoc error propagation

Required end-state split:

- `runtime/fault/`
- shared fault taxonomy and fault record types

Checklist:

- define fault classes
- attach faults to app instance and memory domain
- normalize fatal error handling paths into structured fault records

---

### Slice 11: Recovery engine

Target responsibility:

- reclaim all app-owned resources after kill or fault
- restore safe resident UI
- prove teardown completion

Current code:

- [resource_ownership_table.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/ownership/resource_ownership_table.cpp)
- shell return paths and runtime teardown code

Current problem:

- reclaim logic exists in fragments rather than as one recovery pipeline
- resource teardown and user-visible fallback are still too loosely coupled

Required end-state split:

- `runtime/recovery/`
- deterministic reclaim pipeline driven by supervisor and fault engine

Checklist:

- make reclaim order explicit
- ensure UI detach precedes resource free
- ensure every broker domain has revoke hooks
- emit completion audit once reclaim is done

---

### Slice 12: Quarantine registry

Target responsibility:

- block repeated failure loops by app id, package version, or binary hash

Current code:

- no dedicated subsystem yet

Current problem:

- repeated failure currently relies on operator observation rather than runtime policy

Required end-state split:

- `runtime/quarantine/`

Checklist:

- define quarantine keys
- persist or cache recent strike and crash windows
- integrate with admission and launcher visibility

---

### Slice 13: Audit and telemetry

Target responsibility:

- make runtime decisions observable without mixing them into informal log strings only

Current code:

- [zephyr_logger.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_logger.cpp)
- runtime and shell debug logs

Current problem:

- logs are useful for bringup but too unstructured for final runtime governance
- no dedicated runtime audit surface yet

Required end-state split:

- `runtime/audit/`

Checklist:

- define structured event categories
- separate audit stream from casual debug logging
- correlate admission, syscall deny, fault, recovery, and quarantine by app instance id

---

### Slice 14: UI compositor bridge

Target responsibility:

- mediate app scene ownership under resident compositor control
- prevent apps from directly owning the resident display tree

Current code:

- [zephyr_lvgl_shell_ui.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_lvgl_shell_ui.cpp)
- [zephyr_shell_display_adapter.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_shell_display_adapter.cpp)
- [host_api.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/host_api/host_api.cpp)
- foreground page contracts in shell-facing layers

Current problem:

- app foreground presentation, shell display adaptation, and LVGL ownership are still closely coupled
- final compositor model is not yet reflected in code structure

Required end-state split:

- `runtime/ui_compositor/`
- `ports/zephyr/ui_backend/`

Checklist:

- separate app scene contracts from renderer backend code
- ensure resident compositor owns scene attach and detach
- enforce per-app UI object and image budgets
- prevent direct app ownership of resident LVGL internals

---

### Slice 15: Input routing and foreground control

Target responsibility:

- ensure input focus belongs to supervisor and compositor, not directly to the app

Current code:

- [zephyr_shell_input_adapter.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_shell_input_adapter.cpp)
- [zephyr_shell_input_backend.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_shell_input_backend.cpp)
- routed event polling in [host_api.cpp](C:/Users/VicLi/Documents/Projects/aegis/runtime/host_api/host_api.cpp)

Current problem:

- input routing is partially app-aware but not yet fully supervisor-owned
- app focus, text input focus, and foreground privileges are not yet modeled as one authority chain

Required end-state split:

- `runtime/input_routing/` or compositor-owned input mediation

Checklist:

- formalize foreground token ownership
- revoke input focus on suspend, fault, and kill
- bound event queue depth per app instance

---

### Slice 16: SDK ABI surface

Target responsibility:

- expose stable isolated-runtime ABI to app authors
- forbid SDK leakage of resident implementation details

Current code:

- [host_api_abi.h](C:/Users/VicLi/Documents/Projects/aegis/sdk/include/aegis/host_api_abi.h)
- [app_module_abi.h](C:/Users/VicLi/Documents/Projects/aegis/sdk/include/aegis/app_module_abi.h)
- service ABI headers under [sdk/include/aegis/services](C:/Users/VicLi/Documents/Projects/aegis/sdk/include/aegis/services)

Current problem:

- SDK surface still reflects the current host-api naming and extension-era assumptions
- future isolated runtime terminology is not yet codified in the SDK layout

Required end-state split:

- `sdk/include/aegis/syscall_abi.h`
- `sdk/include/aegis/handles.h`
- `sdk/include/aegis/app_instance_contract.h`

Checklist:

- make handles and bounded payloads the center of the SDK
- avoid resident object semantics in public headers
- align naming with isolated runtime model rather than plugin model

---

### Slice 17: Zephyr isolation backend

Target responsibility:

- provide board and OS substrate for mapping, protection, threads, watchdog, logging, and revoke mechanics
- remain below runtime policy

Current code:

- many files under [ports/zephyr](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr)
- especially [zephyr_llext_adapter.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_llext_adapter.cpp), [zephyr_main.cpp](C:/Users/VicLi/Documents/Projects/aegis/ports/zephyr/zephyr_main.cpp), display and input backends

Current problem:

- Zephyr-specific loading and runtime policy concepts are still interwoven
- board bringup, app loading, UI rendering, and runtime governance meet too early

Required end-state split:

- `ports/zephyr/isolation_backend/`
- `ports/zephyr/ui_backend/`
- `ports/zephyr/input_backend/`

Checklist:

- narrow the Zephyr backend interface to substrate operations
- keep runtime policy out of board glue code
- treat LLEXT as substrate, not architecture

---

## 5. Cross-cutting renames and model shifts

Some current names are useful for bringup but too weak for the final runtime.

Important terminology shifts:

- `AppSession` -> richer `AppInstance`
- `Host API` -> app-facing syscall ABI plus runtime syscall gateway
- `RuntimeLoader` -> loader plus admission and mapping policy layers
- `ResourceOwnershipTable` -> part of a broader handle and recovery model
- `foreground page` -> one presentation contract under compositor control, not app-owned display state

These are not cosmetic renames.
They reflect a change in authority boundaries.

---

## 6. Repository-level cleanup rules during refactor

As code moves toward the isolated model, each change should preserve these rules:

- do not let loader code regain hidden lifecycle authority
- do not let UI backend code become the policy center
- do not let host api remain a bucket of unrelated concerns
- do not let manifest parsing silently become runtime enforcement
- do not let app-visible pointers outlive domain proof
- do not let Zephyr substrate naming define architecture naming

---

## 7. Definition of repository alignment

The repository will be structurally aligned with the isolated blueprint only when:

- runtime code is decomposed by authority boundary rather than by historical convenience
- docs describe the same authority boundaries as the code
- SDK names match the final managed runtime model
- Zephyr remains the substrate, not the policy center

Until then, the codebase should be treated as a governed native runtime in transition toward a strongly isolated managed runtime.
