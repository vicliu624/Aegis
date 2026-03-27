# Aegis Runtime Loader

This document describes the current runtime loader role.

For the final runtime target, loader responsibilities must be split more
strictly across admission, binary policy, memory mapping, supervisor bringup,
fault handling, and recovery.
See [Strongly Isolated App Runtime Blueprint](./isolated-app-runtime-blueprint.md)
and [Runtime Refactor Slices](./runtime-refactor-slices.md).

## 1. Purpose

The runtime loader is the subsystem that turns a discovered native app package into a loadable and governable runtime participant.

In Aegis, loading is not just:

- open file
- map binary
- jump to symbol

It is a controlled transition from:

> “an app exists in storage”

to

> “the system has admitted this app into the runtime under Host API and lifecycle governance”

The runtime loader exists to make that transition safe, structured, and reversible.

---

## 2. Architectural role

The runtime loader sits inside the Aegis Native Runtime layer.

Its job is to bridge:

- app package metadata
- runtime admission policy
- Host API binding
- binary loading infrastructure
- lifecycle bringup/teardown
- unload discipline

It is not the whole runtime.
It is also not the final authority for lifecycle, isolation, quota, or recovery.
Those responsibilities belong to separate runtime subsystems in the target
architecture.

---

## 3. Relationship to LLEXT

Aegis intends to use Zephyr LLEXT as the underlying native extension loading mechanism.

This means:

- Zephyr provides the lower-level binary extension substrate
- LLEXT handles loading of native extension code
- Aegis wraps this with app-level runtime semantics

This distinction is critical.

### LLEXT provides:
- extension loading
- symbol linkage support
- binary residency management
- bringup / teardown primitives

### Aegis runtime loader adds:
- manifest-driven app identity
- ABI policy checks
- Host API binding
- app session creation
- resource ownership registration
- app lifecycle integration
- return-to-shell recovery semantics

LLEXT is the loading engine.  
Aegis runtime loader is the governed app admission layer built on top of it.

On current Zephyr Xtensa targets, Aegis should not trust raw post-load LLEXT
name tables as the durable app-entry discovery interface. The stable contract is:

- the app manifest declares the expected entry symbol
- the app module explicitly exports that entry with `LL_EXTENSION_SYMBOL(...)`
- the Aegis Zephyr adapter resolves the symbol from the module ELF and maps it
  into the loaded runtime text region under Aegis control

That resolution policy is part of the loader adapter contract, not app logic.

---

## 4. Loader responsibilities

The runtime loader must be responsible for all of the following:

### 4.1 Admission-time preparation
- verify app binary path
- ensure entry symbol policy is satisfied
- confirm ABI expectations
- confirm runtime policy allows load

### 4.2 Binary loading
- engage underlying binary loader
- resolve or bind required runtime interfaces
- prepare app runtime image

### 4.3 Bringup
- invoke app entry/bringup only after loader state is valid
- provide Host API binding
- coordinate with `AppSession`

### 4.4 Failure recovery
- clean up partial allocations
- unwind incomplete bringup
- prevent half-loaded zombie state

### 4.5 Teardown and unload
- ensure app session is no longer active
- cooperate with ownership cleanup
- call teardown path
- unload binary safely

---

## 5. What the loader must not do

The runtime loader must not:

- decide shell presentation order
- own launcher registry logic
- directly implement service backends
- leak board-specific details into app admission
- replace lifecycle governance with ad-hoc “best effort”
- bypass Host API or ownership tracking

Its responsibility is narrow and critical.

---

## 6. Inputs to the loader

A loader operation should depend on these inputs:

- `AppDescriptor`
- validated manifest information
- resolved binary path
- entry symbol name
- current runtime ABI version
- current `CapabilitySet` and runtime policy
- `HostApi` instance or provider
- destination `AppSession`

This ensures the loader is not blind binary machinery.  
It operates within system context.

---

## 7. Outputs of the loader

A successful loader operation should produce at least:

- a loaded runtime image handle
- a resolved app entry handle
- Host API bound to the app context
- loader state associated with the current `AppSession`
- readiness for lifecycle transition to `Started`

A failed loader operation must leave:
- no live partial runtime image
- no stale session-owned executable state
- no orphan loader bookkeeping

---

## 8. Suggested loader interface

Illustrative example:

```cpp
class IRuntimeLoader {
public:
    virtual ~IRuntimeLoader() = default;

    virtual LoadResult load(const AppDescriptor& app,
                            AppSession& session,
                            const HostApi& host_api) = 0;

    virtual BringupResult bringup(AppSession& session) = 0;

    virtual TeardownResult teardown(AppSession& session) = 0;

    virtual UnloadResult unload(AppSession& session) = 0;
};
````

This is not the final required shape.
The key idea is that loading and unloading are explicit phases, not hidden side effects.

---

## 9. Load phases

Aegis should treat loading as a sequence of distinct phases.

### 9.1 Binary preparation

* resolve binary path
* confirm file exists
* confirm manifest entry metadata
* prepare loader backend

### 9.2 Binary load

* load native image through underlying substrate
* resolve required symbols
* prepare runtime image handle

### 9.3 Entry resolution

* resolve entry symbol
* confirm callable contract shape

### 9.4 Host binding

* associate app with Host API
* establish runtime context

### 9.5 Bringup

* invoke app entry / startup
* transition session toward `Started`

If any phase fails, the loader must unwind appropriately.

---

## 10. Unload phases

Unload must be equally explicit.

### 10.1 Stop admission of new app activity

* no new timers
* no new UI roots
* no new subscriptions
* no new service-backed handles

### 10.2 Reclaim runtime-owned resources

* cancel timers
* remove subscriptions
* destroy UI roots
* revoke service handles

### 10.3 App teardown

* call app teardown path if defined
* allow app-local cleanup within runtime policy

### 10.4 Binary unload

* release runtime image
* release loader state
* detach executable presence

Unload is not complete until binary residency is gone and the session can be safely retired.

---

## 11. Relationship to AppSession

The loader must not act on raw app binaries in isolation.

It must always operate in relation to an `AppSession`.

Why:

* a loaded binary must belong to a session
* ownership tracking is session-scoped
* lifecycle transitions are session-scoped
* teardown and unload discipline are session-scoped

No “free-floating loaded app” should exist in Aegis.

---

## 12. Relationship to Host API

The loader is responsible for ensuring the app receives Host API binding in a controlled way.

The app must not:

* discover Host API by global symbol accident
* access host services outside runtime mediation
* run before Host API is valid

The loader is therefore one of the places where system sovereignty is concretely enforced.

---

## 13. Relationship to ResourceOwnershipTable

The loader itself does not own the app’s runtime resources, but it participates in the rules that make ownership possible.

This implies:

* loader bringup should establish session context
* runtime-created resources should become attributable to that session
* teardown/unload should consult ownership tracking before binary release

Without ownership discipline, unloading native apps is unsafe.

---

## 14. Relationship to capability checks

The loader should not be the sole policy engine for capability admission, but it must not ignore capability policy either.

Typical flow:

1. manifest declares required/optional capabilities
2. runtime/platform performs admission checks
3. loader proceeds only if admission succeeded

This keeps loading subordinate to app platform policy.

The loader should assume:

* compatibility already evaluated
* capability admission already decided

But it must still refuse to proceed if preconditions are not met.

---

## 15. LLEXT adapter role

Because Aegis intends to use Zephyr LLEXT, the runtime loader should isolate LLEXT behind an adapter boundary.

Example conceptual layering:

```text
Aegis Runtime Loader
  └─ LLEXT Adapter
       └─ Zephyr LLEXT APIs
```

This provides several benefits:

* keeps Aegis runtime interfaces stable
* keeps LLEXT-specific details from spreading everywhere
* makes stub/mock loaders possible in early phases
* allows controlled testing without full binary loading initially

This is especially useful in the early development stage.

---

## 16. Stub loader strategy

The first implementation does not need full real LLEXT loading immediately.

Aegis should support a `StubRuntimeLoader` during early construction.

Purpose:

* validate architecture
* validate registry → session → shell flow
* exercise lifecycle state transitions
* prove Host API and ownership structure

The stub loader must still respect the real conceptual phases:

* load
* bringup
* teardown
* unload

It must not collapse them into one vague operation.

---

## 17. Error handling policy

Loader errors must be explicit and structured.

Prefer:

* typed result objects
* status enums
* clear failure reasons
* explicit cleanup on partial failure

Avoid:

* silent partial success
* implicit fallback behavior
* binary remaining resident after failed bringup
* exceptions crossing runtime boundaries

A load failure should always leave the system in a stable shell-returnable state.

---

## 18. Non-negotiable rules

### Rule 1

The loader must never load a binary without validated manifest context.

### Rule 2

The loader must never bypass Host API binding.

### Rule 3

The loader must never treat “loaded” as equivalent to “running”.

### Rule 4

The loader must cooperate with teardown before unload.

### Rule 5

The loader must remain device-agnostic.

### Rule 6

Underlying LLEXT details must stay behind a runtime adapter boundary.

---

## 16. Current Zephyr bring-up notes

The current LilyGo T-LoRa Pager bring-up exposed two implementation details that are easy to lose if
they remain only in serial logs.

### 16.1 App module staging must be explicit

When packaging Zephyr LLEXT applications, the final staged `app.llext` should be regenerated from the
current module library as an explicit build step.

Relying on implicit post-package artifacts can leave a stale staged module in `/lfs/apps/<app>/`,
which makes runtime debugging misleading because the loader appears healthy while the device is still
running an older binary.

### 16.2 Xtensa `R_XTENSA_RTLD` is a loader no-op in this path

On the ESP32-S3 Xtensa path, Zephyr LLEXT currently encounters `R_XTENSA_RTLD` relocations during load.
For the Aegis writable-buffer loader path used in bring-up, these relocations are intentionally treated
as a no-op rather than as an unsupported relocation failure.

That behavior belongs inside the Zephyr LLEXT backend implementation, not in Aegis app code or in the
runtime admission layer. If a fresh environment starts logging unsupported relocation warnings again,
the local Zephyr Xtensa relocation handler should be checked first.

---

## 19. Minimal first implementation

The first useful runtime loader implementation should prove:

* a descriptor can be selected from registry
* a session can be created
* a loader can transition the session through load and bringup
* Host API can be provided to the app
* app can be stopped and unloaded
* shell regains control afterward

Even if this first implementation uses a stub backend, the architectural phases must already be real.

---

## 20. Summary

The runtime loader exists to make one thing true in Aegis:

> native app code may enter the system,
> but only through a governed path,
> under Host API, lifecycle, and unload discipline.

That is what turns extension loading into a real app runtime.
