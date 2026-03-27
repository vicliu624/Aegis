
# Aegis Host API

## 1. Purpose

The Host API is the formal boundary between an Aegis app and the resident system.

It exists to answer a critical architectural requirement:

> Apps must be native and powerful,  
> but they must not directly own the machine.

The Host API is how apps gain access to the world without taking sovereignty over it.

It is not just a convenience wrapper.  
It is the principal governance boundary of Aegis.

---

## 2. Why Host API exists

Without a Host API, a native app would tend to:

- grab raw device handles
- hold global pointers
- create uncontrolled timers
- bind directly to board-specific services
- manipulate UI objects outside runtime tracking
- leak resources beyond session teardown

That would destroy unload safety, lifecycle clarity, and system authority.

The Host API prevents this by ensuring that apps access system capabilities only through deliberate, controlled, trackable interfaces.

---

## 3. Design goals

The Host API must be:

### 3.1 Explicit
No hidden global reach.  
An app should know that it is calling the system.

### 3.2 Small
The API surface must stay compact and disciplined.

### 3.3 Stable
It is part of the app ABI and should evolve conservatively.

### 3.4 Capability-oriented
It should expose system capabilities, not raw board internals.

### 3.5 Trackable
The runtime must be able to understand what resources an app has created or borrowed.

### 3.6 Portable across devices
The Host API must not expose board-specific type names or device identities.

---

## 4. Core principle

Apps use **Host API**.  
Host API uses **Service Gateway**.  
Service Gateway uses **Device Adaptation Layer** and **Zephyr Base**.

Apps must not bypass this chain.

```text
App
  ↓
Host API
  ↓
Service Gateway
  ↓
Device Adaptation Layer
  ↓
Zephyr / Drivers / Hardware
````

This is the core sovereignty structure of Aegis.

---

## 5. API shape

The Host API should be exposed as a small, stable, versioned interface block.

Apps may be written in C++, but the cross-boundary API should be conservative and ABI-stable.

This strongly suggests a C-style function table or equivalent stable ABI structure.

Example concept:

```cpp
typedef struct AegisHostApi {
    uint32_t abi_version;

    void (*log_write)(int level, const char* tag, const char* msg);

    void* (*mem_alloc)(size_t size, size_t align);
    void  (*mem_free)(void* ptr);

    void* (*ui_create_root)(void);
    void  (*ui_destroy_root)(void* root);

    int   (*timer_create)(uint32_t ms, bool repeat, void* user_ctx);
    void  (*timer_cancel)(int timer_id);

    int   (*settings_get)(const char* key, void* out_buf, size_t* inout_len);
    int   (*settings_set)(const char* key, const void* data, size_t len);

    int   (*service_call)(uint32_t service_id,
                          uint32_t op_id,
                          const void* in_buf,
                          size_t in_len,
                          void* out_buf,
                          size_t* inout_out_len);

    const CapabilitySet* (*get_capability_set)(void);
} AegisHostApi;
```

This example is illustrative, not final.
The important point is the architecture, not the exact C syntax.

---

## 6. Host API domains

The Host API should be organized into clear domains.

---

## 6.1 Logging

Purpose:

* allow apps to write structured logs
* support debugging and diagnostics
* avoid uncontrolled stdout-style behavior

Typical functions:

* `log_write(...)`

Rules:

* logging is system-owned
* apps do not control logging backends directly

---

## 6.2 Memory

Purpose:

* allow app runtime allocations under system policy
* keep allocation within runtime awareness
* support future ownership tracking and quotas

Typical functions:

* `mem_alloc(...)`
* `mem_free(...)`

Rules:

* cross-boundary allocation ownership must be clear
* avoid mixed alloc/free responsibility confusion

---

## 6.3 UI

Purpose:

* allow an app to create and destroy its UI root
* allow the runtime to track app-owned UI structures
* preserve teardown safety
* allow a foreground app to describe page intent without taking shell ownership

Typical functions:

* `ui_create_root(...)`
* `ui_destroy_root(...)`
* future UI helpers as needed

Rules:

* apps do not directly attach themselves to global shell structures
* app UI ownership must remain session-bounded
* shell-owned affordances such as softkeys remain system-governed even when an app owns the foreground

Current UI-facing operations support:

* foreground page declaration through the UI service
* preferred softkey binding declaration for page-owned commands
* page state token updates for stale-event rejection
* semantic routed event delivery back to the app through UI polling

---

## 6.4 Timers

Purpose:

* allow apps to use runtime-managed timers
* prevent uncontrolled timer lifetimes
* make timer cleanup enforceable during teardown

Typical functions:

* `timer_create(...)`
* `timer_cancel(...)`

Rules:

* timers created by an app belong to that app session
* timer cleanup must be automatic on session stop

---

## 6.5 Settings

Purpose:

* allow apps to read and write app-visible settings through system policy
* preserve system control over persistence and namespace discipline

Typical functions:

* `settings_get(...)`
* `settings_set(...)`

Rules:

* settings namespace policy should be system-defined
* apps do not directly own raw persistence backends

---

## 6.6 Service access

Purpose:

* give apps structured access to system-owned services
* avoid leaking concrete backend implementations

Typical functions:

* `service_call(...)`
* or future typed service handles where justified

Possible service domains:

* radio
* gps
* audio
* notification
* storage
* power
* network
* hostlink

Current storage-facing expectations include:

* status query, including mount root and SD-card presence
* governed directory listing through the storage service ABI
* no shell-private file model dependencies

Rules:

* app-facing contracts stay generic
* board-specific service types must not appear here
* runtime permission policy may still deny individual service domains even after an app was admitted

---

## 6.7 Capability queries

Purpose:

* allow apps to inspect what the current device/runtime can provide
* avoid board-specific branching
* enable graceful degradation

Typical functions:

* `get_capability_set()`
* or capability-specific query helpers

Rules:

* capability truth comes from the system
* apps do not infer device features from unrelated facts

---

## 7. Relationship to AppSession

Host API use must be tied to app session ownership.

This means:

* the Host API instance is only valid in the context of a running app session
* runtime-created objects must be attributable to that session
* when the session stops, all session-owned resources must be reclaimable

The Host API must therefore cooperate with:

* `AppSession`
* `ResourceOwnershipTable`
* lifecycle state transitions

Without this relationship, the API would lose governance value.

For foreground UI, any page descriptor, command registration, or softkey binding
owned by the app must also be attributable to the current session and revoked at
session stop.

---

## 8. Relationship to ResourceOwnershipTable

Every resource created through Host API should be either:

* directly reclaimable by runtime,
* or explicitly registered under the current app session.

Examples:

* UI roots
* timers
* subscriptions
* service handles
* notifications
* text-input focus grants
* foreground page command registrations
* future async work handles

This is essential for clean exit and unload.

The runtime must always be able to answer:

* what does this app currently own?
* what must be revoked during teardown?

---

## 9. Relationship to capabilities

The Host API and the capability model are closely linked, but they are not identical.

### Capability model

Describes what the system can provide.

### Host API

Provides the callable path to use that functionality.

For example:

* capability says `gps_fix = degraded`
* Host API still exposes GPS query functions
* app decides whether degraded support is sufficient

This keeps runtime truth and service access aligned.

The Host API must also remain subordinate to runtime permission policy.

Examples:

* an app may be able to inspect text-input state as a generic service query
* but claiming session text-input focus should require an explicit manifest permission such as `text_input_focus`
* an app may be statically compatible with a device that has radio or gps hardware
* but access to `radio`, `gps`, `audio`, or `hostlink` service domains should still be checked at the Host API boundary against manifest-requested permissions

---

## 10. Relationship to device adaptation

The Host API must remain device-agnostic.

This means:

* it must not expose board identity
* it must not expose board-specific type names
* it must not encode board-specific input paths directly
* it must not require app code to know concrete backends

The device adaptation layer is below Host API, not inside it.

This separation is non-negotiable.

---

## 11. ABI considerations

The Host API is part of the app/runtime ABI boundary.

Because apps are independently built native binaries, the Host API must be treated as ABI-sensitive infrastructure.

This implies:

* strict versioning
* conservative evolution
* stable field ordering where applicable
* careful compatibility policy
* avoiding fragile C++ ABI exposure at the boundary

Apps may be implemented internally in C++, but the cross-boundary contract should remain conservative.

---

## 12. Error handling policy

The Host API should return explicit status codes for fallible operations.

Avoid:

* throwing exceptions across the app/system boundary
* implicit failure semantics
* hidden global error state

Preferred approach:

* integer status codes
* explicit out-parameter lengths
* null checks
* validation at boundary points

The boundary should remain easy to audit and predictable under constrained runtime conditions.

---

## 13. Non-goals

The Host API is not intended to be:

* a full POSIX-like system call surface
* a dump of every internal runtime function
* a board SDK
* a way for apps to escape system authority
* an invitation to expose raw driver handles

Its purpose is narrower and more disciplined.

---

## 14. Non-negotiable rules

### Rule 1

Apps must not bypass Host API to reach system-owned capabilities.

### Rule 2

Host API must remain board-agnostic.

### Rule 3

Resources created through Host API must remain trackable.

### Rule 4

Cross-boundary ownership must be explicit.

### Rule 5

Host API stability is part of runtime ABI discipline.

### Rule 6

The API should expose capabilities and services, not raw hardware sovereignty.

---

## 15. Minimal first implementation

The first useful Host API in Aegis should include only a small set:

* logging
* memory allocation
* UI root creation/destruction
* timer creation/cancel
* settings get/set
* capability query
* one generic service call path

As typed service domains mature, the service call path may additionally expose focused contracts such as:

* text-input state
* text-input focus state
* session-scoped focus request/release

That is enough to prove:

* apps can start
* apps can query the device world
* apps can build UI through the runtime
* apps can allocate and clean up resources
* apps can remain guest workloads

More can be added later.

---

## 16. Summary

The Host API is the formal language through which Aegis says to apps:

> you may act in this world,
> but you do not own it.

That is why the Host API is one of the central architectural boundaries in Aegis.

---

## 17. See also

The Host API document defines the service boundary itself.
For the runtime rules that decide how Host API data must be owned, copied, and
revoked in practice, read:

- [Aegis App Runtime Memory Model](./app-runtime-memory-model.md)
- [Aegis LLEXT App Constraints](./llext-app-constraints.md)
- [Aegis Foreground Page Contract](./foreground-page-contract.md)
