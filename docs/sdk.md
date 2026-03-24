可以，继续往前补最合适的两份：

* `docs/sdk.md`
* `docs/app-abi.md`

这两份的作用是把前面的架构文档，进一步落到“**app 开发者如何接入**”和“**二进制边界到底怎么定义**”上。

---

# `docs/sdk.md`

````markdown
# Aegis SDK

## 1. Purpose

The Aegis SDK exists to make native app development for Aegis possible without giving app code direct ownership of the system.

Its purpose is to provide:

- the app-facing headers
- the stable Host API contract
- the app entry contract
- manifest generation conventions
- capability query interfaces
- build templates
- example app structures

The SDK is not a board SDK.  
It is not a driver SDK.  
It is not a direct hardware access kit.

It is the disciplined app-facing development surface for Aegis.

---

## 2. Design goals

The SDK must satisfy the following goals:

### 2.1 Make app development possible
A developer must be able to build a native Aegis app without touching system-internal code.

### 2.2 Preserve system authority
The SDK must not become a back door through which apps bypass Host API and reach raw hardware.

### 2.3 Remain device-agnostic
SDK consumers should build against Aegis contracts, not specific board implementations.

### 2.4 Preserve ABI discipline
The SDK must align with the runtime ABI and expose only the supported boundary.

### 2.5 Be small and explicit
The SDK should expose the minimum app-facing surface necessary for apps to exist as proper Aegis apps.

---

## 3. What the SDK contains

A minimal Aegis SDK should contain:

- public app-facing headers
- Host API definitions
- capability model definitions
- app entrypoint definitions
- ABI version constants
- manifest schema guidance
- example app templates
- build integration helpers

It may also later contain:
- manifest generation tools
- validation helpers
- packaging helpers
- CI checks for app compatibility

---

## 4. What the SDK does not contain

The SDK must not expose:

- raw board headers as the primary app API
- raw device drivers
- direct Zephyr driver ownership paths
- system-internal shell control internals
- unrestricted access to global runtime state
- privileged system-only service paths

Apps are guests.  
The SDK must preserve that truth.

---

## 5. SDK directory structure

A recommended SDK layout:

```text
/sdk
  /include
    aegis/
      app.h
      host_api.h
      capability.h
      abi.h
      types.h
      status.h
  /templates
    minimal_app/
    tool_app/
  /examples
    hello_app/
    info_app/
````

This structure should clearly separate:

* stable public headers
* starter templates
* reference examples

---

## 6. Core public headers

The SDK should begin with a very small set of public headers.

### 6.1 `aegis/app.h`

Defines:

* app entrypoint contract
* app lifecycle callback structure if applicable
* app-visible context types

### 6.2 `aegis/host_api.h`

Defines:

* Host API structure
* app-facing service call conventions
* runtime-provided access surface

### 6.3 `aegis/capability.h`

Defines:

* capability identifiers
* capability levels
* capability descriptors
* capability query helpers

### 6.4 `aegis/abi.h`

Defines:

* app ABI version constants
* compatibility macros
* runtime contract versioning support

### 6.5 `aegis/types.h`

Defines:

* shared small cross-boundary types
* opaque handles
* fixed-width type aliases if needed

### 6.6 `aegis/status.h`

Defines:

* common status/result codes
* load/start/teardown result helpers

---

## 7. App developer mental model

The SDK should teach app developers the correct Aegis mental model:

* your app is a native extension
* your app is loaded by the runtime
* your app receives Host API from the system
* your app does not own the device
* your app should adapt to capabilities, not boards
* your app must behave well under session teardown
* your app must not assume unrestricted persistence or hardware access

The SDK should make the right thing easy and the wrong thing awkward.

---

## 8. App entry contract

The SDK must define the canonical app entry contract.

A minimal conceptual model may be:

* runtime loads binary
* runtime resolves entry symbol
* runtime passes Host API and app context
* app performs startup
* app enters running session
* runtime later calls teardown path if needed

The SDK must make this contract explicit and unambiguous.

Example conceptual shape:

```cpp
typedef struct AegisAppContext AegisAppContext;
typedef struct AegisHostApi AegisHostApi;

typedef int (*AegisAppEntryFn)(const AegisHostApi* host,
                               AegisAppContext* app_ctx);
```

This is illustrative only.
The exact final form may evolve, but the contract must be stable and documented.

---

## 9. Capability usage in the SDK

The SDK should make capability-aware app development the default.

An app should be able to:

* query capability set
* check whether a capability exists
* inspect whether support is absent / degraded / full
* adapt behavior accordingly

It must not need to:

* identify a board name
* include a board-specific header
* inspect board package internals

This is one of the key ways the SDK preserves multi-device portability.

---

## 10. Host API exposure in the SDK

The SDK should expose Host API as a stable public contract.

This includes at minimum:

* logging
* allocation helpers
* UI root creation/destruction
* timer management
* settings access
* capability queries
* service call gateway

But the SDK should not expose:

* internal runtime implementation classes
* service backend implementations
* device adaptation objects
* shell-private control paths

Public SDK does not mean “everything in the system is public”.

---

## 11. Build integration

The SDK should support independent app builds.

App authors should be able to build apps:

* outside the main Aegis source tree if desired
* against exported headers and ABI definitions
* using packaging conventions that produce:

  * binary
  * manifest
  * icon
  * app package directory structure

The SDK therefore needs not only headers, but also:

* build templates
* package layout conventions
* manifest integration guidance

This is especially important because Aegis apps are not merely static source modules in the main firmware tree.

---

## 12. Example app template

The SDK should provide at least one minimal app template.

The template should demonstrate:

* app entrypoint
* Host API usage
* capability query
* simple UI creation
* proper exit discipline
* manifest structure

This is important because architecture is easier to preserve when the first examples are correct.

---

## 13. Result codes and error handling

The SDK should define a common status model.

Reasons:

* apps need predictable boundary behavior
* runtime needs consistent result interpretation
* exceptions across the system boundary are undesirable

Examples:

* `AEGIS_OK`
* `AEGIS_ERR_INVALID_ARGUMENT`
* `AEGIS_ERR_UNSUPPORTED`
* `AEGIS_ERR_CAPABILITY_MISSING`
* `AEGIS_ERR_RUNTIME_FAILURE`

This keeps app/runtime interaction auditable and predictable.

---

## 14. Opaque handles

The SDK should prefer opaque handles across the system boundary where object identity must cross the boundary.

Examples:

* UI handles
* timer handles
* service handles
* future subscription handles

Reasons:

* avoid ABI fragility
* avoid leaking internal object layout
* preserve runtime control over ownership

Opaque handles are often preferable to shared C++ object ownership at this boundary.

---

## 15. Versioning

The SDK must be tightly aligned with Aegis ABI discipline.

This implies:

* ABI version constants in public headers
* clear compatibility expectations
* conservative evolution of public structures
* documentation of breaking vs non-breaking changes

The SDK is part of the runtime contract, not just convenience documentation.

---

## 16. Security and authority implications

The SDK is also part of the system’s authority structure.

A badly designed SDK would accidentally:

* expose too much runtime internals
* make unload unsafe
* let apps depend on hidden global state
* make device portability impossible

So the SDK must be intentionally narrow.

Its job is to help apps participate, not to help apps dominate.

---

## 17. Non-negotiable rules

### Rule 1

The SDK must remain app-facing, not board-facing.

### Rule 2

The SDK must expose Host API, not raw hardware ownership.

### Rule 3

Capability use must be first-class in the SDK.

### Rule 4

Public headers must be ABI-conscious.

### Rule 5

Examples and templates must model correct Aegis architecture.

### Rule 6

The SDK must make independent app packaging possible.

---

## 18. Minimal first implementation

The first useful SDK should include:

* `app.h`
* `host_api.h`
* `capability.h`
* `abi.h`
* `status.h`
* one minimal example app
* one minimal app template

That is enough to let:

* an app compile,
* a runtime bind Host API,
* an app query capabilities,
* and a minimal app package be produced.

---

## 19. Summary

The Aegis SDK exists to make one thing practical:

> apps should be easy to build for Aegis,
> but difficult to build in ways that violate Aegis architecture.

That is the role of the SDK.