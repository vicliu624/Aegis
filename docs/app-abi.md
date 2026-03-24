# Aegis App ABI

## 1. Purpose

The Aegis App ABI defines the binary contract between:

- the Aegis resident runtime
- and independently built native Aegis apps

Because Aegis apps are native binaries loaded at runtime, binary compatibility is not an implementation detail.  
It is a foundational system contract.

The ABI exists to answer:

- how does the runtime recognize a loadable app binary?
- what symbol does it resolve?
- what data structures may safely cross the boundary?
- how is Host API presented to the app?
- what changes are compatible and what changes are breaking?

Without a disciplined ABI, native loading quickly becomes fragile and unsustainable.

---

## 2. Why ABI discipline matters

Aegis apps are not all compiled into one monolithic image.

That means the system must tolerate:
- independent app builds
- binary packaging
- runtime loading
- version mismatch scenarios
- gradual platform evolution

If ABI is not treated explicitly, failure follows quickly:

- entrypoint mismatch
- structure layout mismatch
- broken symbol expectations
- invalid object ownership across boundary
- impossible compatibility diagnosis
- silent loader crashes

The App ABI exists to prevent this.

---

## 3. Scope of the ABI

The App ABI covers:

- entry symbol contract
- Host API layout and calling conventions
- shared public structure layouts
- status/result conventions
- opaque handle usage
- compatibility versioning rules

The App ABI does **not** cover:
- internal runtime implementation classes
- board package internals
- private shell models
- service backend object layouts
- arbitrary C++ implementation details

The ABI is intentionally smaller than the full system architecture.

---

## 4. Core ABI principle

Apps may be written in C++, but the runtime boundary should be treated conservatively.

This strongly suggests:

- stable C-compatible entry contracts
- fixed, versioned public data structures
- opaque handles instead of shared object layouts
- no exceptions crossing the boundary
- no reliance on unstable C++ ABI details for core interoperability

This is not a rejection of C++.  
It is a recognition that the binary boundary must remain governable.

---

## 5. Entry contract

The runtime must know how to locate and invoke an app.

This implies a stable entry symbol contract.

Example conceptual shape:

```cpp
extern "C" int aegis_app_entry(const AegisHostApi* host,
                               AegisAppContext* app_ctx);
````

The exact details may evolve, but the ABI must define:

* symbol name expectations
* argument order and types
* return/result convention
* startup failure semantics

The runtime loader must treat this as a formal contract, not a convention guessed ad hoc.

---

## 6. ABI versioning

Each app package must declare the ABI version it targets.

This version must be checked before runtime load/bringup.

The ABI version should correspond to the public binary boundary contract, not merely to source-level implementation state.

This implies:

* `abi_version` in manifest
* matching runtime ABI constant
* compatibility policy in the loader/platform

Example concept:

```cpp
#define AEGIS_APP_ABI_VERSION 1
```

If an app targets an incompatible ABI version, runtime admission must fail cleanly before bringup.

---

## 7. Compatible vs breaking changes

The App ABI should clearly distinguish between compatible and breaking changes.

### Compatible changes

Examples:

* adding new optional service operation IDs
* adding new optional capabilities
* adding new manifest fields that old runtimes can ignore
* extending functionality through version-aware paths

### Breaking changes

Examples:

* changing entry function signature
* reordering Host API fields without compatibility strategy
* changing shared struct layout incompatibly
* changing opaque handle interpretation incompatibly
* changing required status/result behavior unexpectedly

Breaking changes must result in ABI version changes.

---

## 8. Host API as ABI surface

The Host API is one of the most important parts of the App ABI.

This means:

* Host API layout must be stable
* field order matters if using a function table structure
* new fields must be added with compatibility strategy
* removed/changed fields are ABI-sensitive

The Host API is not just a source-level interface.
It is part of the runtime binary contract.

---

## 9. Public structure rules

Any structure that crosses the app/runtime boundary must follow strict rules.

### Preferred properties

* small
* explicit field types
* fixed-width integers where appropriate
* no hidden ownership semantics
* no raw internal object embedding
* version-aware evolution

### Discouraged properties

* STL containers across the boundary
* exceptions crossing the boundary
* virtual object layouts across the boundary
* compiler-sensitive complex aggregates without discipline

The fewer complex structures cross the boundary, the more stable the ABI becomes.

---

## 10. Opaque handles

Opaque handles should be preferred for runtime-owned resources.

Examples:

* UI handles
* timer handles
* service handles
* subscription handles

Reasons:

* hide internal layout
* preserve runtime ownership
* reduce ABI fragility
* simplify binary compatibility

Opaque handles are one of the key techniques for keeping the ABI small and survivable.

---

## 11. Ownership rules across the ABI

Ownership across the boundary must always be explicit.

The ABI must make it clear:

* who allocates
* who frees
* whether a handle is borrowed or owned
* what becomes invalid at session stop
* what the runtime will revoke during teardown

Undefined ownership is especially dangerous in a native runtime-loaded environment.

This is not optional documentation.
It is part of ABI correctness.

---

## 12. Error/result conventions

The ABI should define a consistent error/result convention.

Preferred characteristics:

* integer or enum status codes
* no exceptions across boundary
* explicit success/failure reporting
* documented meaning for startup/teardown failures

Example conceptual statuses:

* `AEGIS_OK`
* `AEGIS_ERR_INVALID_ARGUMENT`
* `AEGIS_ERR_CAPABILITY_MISSING`
* `AEGIS_ERR_ABI_MISMATCH`
* `AEGIS_ERR_RUNTIME_FAILURE`

Predictable failure semantics are essential for robust runtime loading.

---

## 13. Calling convention discipline

The ABI must assume that apps and runtime are built independently.

Therefore:

* calling conventions must be explicit and conservative
* entry contracts should not rely on toolchain-accidental behavior
* language boundary assumptions must be minimized
* symbol names used by loader must be fixed by contract

Even if the entire system initially uses one toolchain, the ABI should be designed as if change is possible.

---

## 14. C++ use inside apps

Apps may internally use C++ features freely if desired.

But the boundary itself should remain disciplined.

This means:

* C++ inside app: allowed
* C++ ABI exposed as the primary runtime contract: discouraged

Why:

* name mangling
* vtable layout sensitivity
* exception semantics
* compiler and build-mode fragility
* harder compatibility management

So the Aegis App ABI should stay conservative even if app implementations are modern C++ internally.

---

## 15. Manifest and ABI relationship

The manifest and ABI are tightly connected.

The manifest tells the runtime:

* which ABI version the app targets
* which entry symbol should exist
* what capabilities the app expects
* how the app should be admitted

The ABI defines:

* what that entry symbol means
* how Host API is passed
* which binary layouts are valid
* how compatibility is evaluated

The manifest declares intent.
The ABI defines binary truth.

Both are necessary.

---

## 16. Loader and ABI relationship

The runtime loader is the primary enforcer of ABI discipline.

Loader responsibilities include:

* check `abi_version`
* resolve required entry symbol
* reject incompatible binaries before bringup
* bind Host API according to ABI contract
* avoid partially running incompatible binaries

The loader must never treat ABI mismatch as a recoverable nuisance.

It is a hard admission failure.

---

## 17. ABI evolution strategy

The App ABI should evolve conservatively.

A good strategy is:

### Step 1

Keep v0.1 ABI intentionally small.

### Step 2

Prefer extension through:

* new service operations
* optional capability use
* new manifest fields
* new optional Host API functions added compatibly

### Step 3

Only change ABI version when binary compatibility truly breaks.

### Step 4

Document all ABI version changes explicitly.

A small ABI that lasts is better than a large ABI that collapses.

---

## 18. Non-negotiable rules

### Rule 1

The runtime boundary must be treated as an ABI, not just a header inclusion convenience.

### Rule 2

Apps may use C++, but the app/runtime contract must remain conservative.

### Rule 3

Manifest-declared ABI version must be checked before runtime admission.

### Rule 4

Opaque handles are preferred over shared internal object layouts.

### Rule 5

Ownership across the boundary must be explicit.

### Rule 6

Breaking binary contract changes require ABI version changes.

---

## 19. Minimal first implementation

The first useful Aegis App ABI should define at least:

* ABI version constant
* entry symbol name and signature
* Host API binary contract shape
* a small set of shared public status/result types
* opaque handle conventions
* manifest `abi_version` meaning

That is enough to support:

* independent app build
* runtime admission checks
* Host API binding
* safe minimal native loading

---

## 20. Summary

The Aegis App ABI exists to make one thing possible:

> independently built native apps can be loaded into Aegis
> without turning runtime compatibility into guesswork.

That is why ABI discipline is foundational to Aegis.