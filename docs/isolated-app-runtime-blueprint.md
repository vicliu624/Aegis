# Aegis Strongly Isolated App Runtime Blueprint

## 1. Purpose

This document defines the final-form runtime model for Aegis apps when
isolation, containment, and system sovereignty are treated as non-negotiable.

It replaces the weaker mental model of:

- "native extension"
- "hosted plugin"
- "loadable app that behaves if it follows the rules"

with a stronger and more accurate model:

- "isolated application guest"
- "runtime-managed native workload"
- "resource-governed and fault-contained app instance"

The goal is not to patch the current runtime with more guardrails.
The goal is to define the complete target architecture that removes whole
classes of historical problems:

- shared-fate crashes
- ambiguous ownership
- pointer leakage
- service bypass
- unbounded resource consumption
- UI takeover
- unsafe unload behavior

---

## 2. Final system statement

An Aegis app must execute as a strongly isolated managed guest.

That statement has four hard consequences:

1. an app may access only memory explicitly mapped into its domain
2. an app may request system capabilities only through one governed syscall boundary
3. an app failure must terminate only that app instance, not the resident system
4. every material resource consumed by an app must be explicitly metered, bounded, and revocable

If any of the four consequences is not true, the runtime is still a governed
extension system, not yet a strongly isolated app platform.

---

## 3. Non-goals

This blueprint does not optimize for:

- minimum implementation effort
- backwards compatibility with every existing runtime shortcut
- preserving current host pointer patterns
- preserving direct LVGL object ownership by apps
- keeping the current "load and jump" architecture intact

The document is intentionally written as a final-form target.

---

## 4. Top-level architecture

The final app runtime must contain these first-class subsystems:

- App Admission Authority
- App Supervisor
- App Memory Domain Manager
- Syscall Gateway
- Service Broker
- UI Compositor
- Quota and Budget Engine
- Fault and Recovery Engine
- Binary Policy Loader
- Audit and Telemetry Log

The high-level runtime path becomes:

```text
App Package
  -> Admission Authority
  -> Binary Policy Loader
  -> App Memory Domain Manager
  -> App Supervisor
  -> Managed App Instance
  -> Syscall Gateway
  -> Service Broker
  -> Resident Services
```

The current design language of "Host API" remains useful, but in the final
system it is only the app-facing ABI surface.
The real authority belongs to the supervisor, gateway, and broker layers.

---

## 5. Core runtime object: AppInstance

Every launched app must become an `AppInstance`.

`AppInstance` is the only valid runtime unit of execution.
It is not enough to know that an app image is loaded.
The runtime must know which instance is consuming which resources under which
limits at which moment.

Each `AppInstance` must own or reference all of the following state:

- app identity
- package identity
- binary identity and ABI identity
- lifecycle state
- memory domain descriptors
- stack allocation and guard metadata
- heap arena metadata
- executable and read-only segment metadata
- handle table
- permission view
- capability view
- quota ledger
- timer table
- UI scene root
- event queue
- watchdog state
- fault state
- quarantine state
- audit correlation id

An app image may exist on storage forever.
An app instance exists only while admitted by the runtime.

---

## 6. Lifecycle model

The final lifecycle model is:

1. discovered
2. parsed
3. admitted
4. loaded
5. mapped
6. instantiated
7. running
8. suspended
9. resumed
10. faulted
11. killed
12. reclaimed
13. quarantined

Rules:

- discovery does not imply admission
- loading does not imply execution
- execution does not imply foreground control
- fault does not imply system failure
- kill does not imply leak
- reclaim must be deterministic
- quarantine blocks automatic relaunch until policy permits it

---

## 7. Memory isolation model

### 7.1 Memory domains are mandatory

Each app instance must execute inside its own memory domain.

The runtime must never rely on "well-behaved pointers" as the primary safety
mechanism.
Ownership documentation is necessary but not sufficient.

### 7.2 Required memory regions

Each app instance must have separately described regions for:

- text
- rodata
- data
- bss
- heap
- stack
- syscall shared buffers
- UI scene memory
- optional media decode arena

### 7.3 Region permissions

Required access policy:

- `text`: read + execute
- `rodata`: read only
- `data`: read + write
- `bss`: read + write
- `heap`: read + write
- `stack`: read + write with guard region
- `shared syscall buffers`: explicitly directional and size-bounded
- `UI scene memory`: writable only through UI gateway semantics

Writable executable memory is forbidden.
Executable writable memory is forbidden.
The runtime must enforce W^X.

### 7.4 Shared memory policy

Apps must not hold arbitrary host pointers.
All cross-boundary exchange must use one of:

- value copy
- bounded shared buffer
- opaque handle

If lifetime or aliasing is unclear, the interface must not expose raw pointers.

### 7.5 Stack safety

Each app instance stack must have:

- dedicated allocation
- overflow guard region
- stack high-water telemetry
- hard fault mapping to fault reason

Stack corruption must never silently damage resident shell state.

### 7.6 Heap safety

Each app heap must be arena-scoped to that app instance.
Heap accounting must be exact enough to answer:

- current bytes in use
- peak bytes in use
- failed allocations
- suspicious fragmentation
- outstanding allocation count

App heaps must not share a generic allocator pool with resident runtime state
unless the pool itself provides hard per-domain accounting and revocation.

### 7.7 Hardware-backed enforcement

Where supported, MPU or MMU policy must enforce domain boundaries.
Where full hardware isolation is not available, the runtime must still model:

- non-overlapping arenas
- faultable guard regions
- copy-in and copy-out gateway buffers
- prohibition of raw host pointer export
- loader-time segment policy

The absence of full MMU support is not a reason to collapse back into shared
ownership semantics.

---

## 8. Syscall boundary model

### 8.1 One legal path

An app may interact with the resident system only through the syscall gateway.

No direct linkage to resident service objects is permitted.
No bypass channel is permitted.
No app-visible header may expose resident object layouts.

### 8.2 Syscall requirements

Every syscall must be:

- versioned
- length checked
- structure checked
- enum checked
- permission checked
- capability checked
- quota checked
- state checked
- auditable
- revocable

### 8.3 Default deny

Syscall policy is default deny.

The gateway must refuse a request if:

- the app lacks the requested permission
- the platform lacks the required capability
- the app is in the wrong lifecycle state
- the handle is invalid
- the handle belongs to another app instance
- the payload is malformed
- the rate limit is exceeded
- the quota is exhausted
- the app is quarantined or terminating

### 8.4 ABI contract

The app-facing ABI must expose only:

- stable scalar types
- explicit versioned structs
- opaque handles
- bounded slices and buffers
- explicit result codes

The ABI must not expose:

- resident C++ object types
- driver object references
- unmanaged callback ownership
- resident allocator internals
- renderer internals

---

## 9. Service broker model

The service broker exists so that an app never owns the service.
It owns a revocable right to request an operation from the service.

The broker must mediate all service domains, including:

- UI
- input
- storage
- settings
- radio
- gps
- audio
- notification
- power
- time
- hostlink
- text input

Each brokered operation must be attributable to an app instance and a handle.

The broker must support:

- admission checks
- permission checks
- handle ownership checks
- capability degradation handling
- per-app quotas
- rate limiting
- cancellation
- forced revoke on app kill

---

## 10. Permission and capability model

Permissions and capabilities serve different purposes and both are required.

- permissions answer "is this app allowed to ask?"
- capabilities answer "can this device satisfy the request at the required level?"

The final runtime must compute an app execution view that is the intersection of:

- manifest-declared requests
- device capabilities
- resident policy
- runtime supervision state

The execution view is frozen at admission time and may be narrowed later.
It must never silently widen during execution.

---

## 11. UI isolation and compositor model

### 11.1 Apps do not own the display tree

An app must never receive direct ownership of the resident display hierarchy.
It may own only a scene subtree or a declarative scene description under
resident compositor control.

### 11.2 Required UI architecture

The final model must contain:

- one resident compositor
- one app scene root per running app instance
- compositor-controlled focus
- compositor-controlled foreground and background transitions
- compositor-owned render submission
- compositor-owned scene destruction

### 11.3 UI resource policy

Each app scene must have bounded:

- object count
- text bytes
- image decode bytes
- canvas bytes
- style count
- animation count
- frame submission frequency

### 11.4 UI fault containment

If an app scene becomes invalid, too expensive, or unresponsive, the compositor
must be able to:

- detach the scene
- destroy the scene
- return to a resident safe page
- preserve shell responsiveness

The app must not be able to pin the whole display system in an unrecoverable
state.

---

## 12. Event, timer, and scheduling containment

Each app instance must have:

- a bounded event queue
- a bounded timer table
- bounded deferred work registrations
- bounded foreground processing time
- watchdog-observed responsiveness

Timer abuse must not become a practical path to starve resident work.
Event floods must not create unbounded queue growth in shell or compositor
subsystems.

The runtime must track:

- timer count
- minimum timer interval
- event backlog depth
- longest event handling latency
- watchdog strikes

---

## 13. Quota and budget engine

### 13.1 Quotas are runtime-enforced, not declarative only

Manifest budgets are advisory until the runtime binds them into enforced quotas.

Every app instance must have an active ledger with limits for at least:

- stack bytes
- heap bytes
- UI bytes
- image decode bytes
- event queue depth
- timer count
- service call rate
- storage handle count
- open file count
- notification count
- text input sessions
- log bandwidth
- CPU execution window
- uninterrupted foreground occupancy

### 13.2 Enforcement behavior

On quota breach the runtime must choose from a defined policy set:

- deny current operation
- record strike
- degrade service
- revoke handle
- kill app instance
- quarantine app instance identity

Quota enforcement must be deterministic and visible in audit logs.

---

## 14. Fault model

The runtime must classify faults.
A fault is not just "app exited badly".

Required fault classes include:

- invalid memory access
- stack overflow
- heap corruption
- illegal syscall payload
- invalid handle use
- permission violation
- quota exhaustion
- watchdog timeout
- UI compositor violation
- binary policy violation
- loader integrity failure

Each fault record must include:

- app id
- package version
- instance id
- lifecycle state
- fault class
- fault timestamp
- last successful syscall
- recent quota state
- current foreground or background status

---

## 15. Recovery and crash containment

When an app faults, the runtime must recover in the following order:

1. stop further app execution
2. detach app scene from compositor
3. revoke timers and event registrations
4. revoke all service handles
5. release text input ownership
6. close storage and notification sessions
7. release heap and stack arenas
8. release image and UI decode arenas
9. record audit event
10. increment crash and strike counters
11. choose relaunch, safe return, or quarantine policy
12. restore a resident safe surface

At no point should fault recovery require trusting app code to cooperate.

---

## 16. Quarantine policy

Quarantine exists to stop repeated failure loops.

The runtime must support quarantine at least by:

- app id
- package version
- binary hash

Quarantine triggers include:

- repeated crash loops
- repeated quota violations
- repeated invalid syscall patterns
- loader integrity failures
- explicit resident policy decision

Quarantined apps:

- must not auto-launch
- must not reclaim previous live handles
- must surface clear diagnostic status to system tooling

---

## 17. Binary admission and loader policy

The binary loader must evolve from "loadable image support" into policy-backed
admission control.

Admission must verify:

- package manifest integrity
- ABI version compatibility
- entry symbol validity
- section layout validity
- relocation policy compliance
- permitted import table
- segment permission policy
- binary size budget
- declared memory budget compatibility

The loader must refuse binaries that:

- import unauthorized resident symbols
- require forbidden relocation patterns
- violate section policy
- request executable writable regions
- exceed policy limits

The app must bind only to stable runtime ABI exports.
Ad hoc resident symbol resolution is forbidden.

---

## 18. Audit and observability

The isolated runtime must be observable enough to answer:

- why an app was admitted
- what resources it owns right now
- why a syscall was denied
- why a foreground scene was killed
- why an app was quarantined
- whether recovery fully reclaimed its resources

Required audit stream categories:

- admission
- load and map
- quota decisions
- syscall denials
- service broker actions
- UI scene lifecycle
- fault and recovery
- quarantine decisions

Audit records must be lightweight enough for embedded use but structured enough
to support field diagnostics.

---

## 19. Development model implications

The SDK contract must change to fit the isolated runtime.

App authors must program against:

- stable syscall ABI
- opaque handles
- copy-based payloads
- runtime-owned UI scene contracts
- explicit resource lifetime APIs

App authors must not program against:

- resident LVGL internals
- resident filesystem objects
- resident scheduler internals
- shared allocator assumptions
- hidden cross-module symbol access

The programming model becomes more constrained on purpose.
That constraint is what creates platform trustworthiness.

---

## 20. Required repository shape

The repository should eventually reflect the isolated architecture with clear
subsystems such as:

```text
runtime/
  admission/
  supervisor/
  memory_domain/
  syscall/
  broker/
  quota/
  fault/
  recovery/
  quarantine/
  audit/
  loader/
  ui_compositor/
sdk/
  include/aegis/
    syscall_abi.h
    handles.h
    app_instance_contract.h
```

This is not a demand for file renaming today.
It is a statement that the final runtime has more than one responsibility and
must not hide them inside a single "host api" abstraction.

---

## 21. Relationship to the current runtime

The current Aegis runtime is a governed native extension runtime.
It is a meaningful step forward from monolithic firmware and from hard-coded
system pages pretending to be apps.

However, it is still weaker than the target defined here because it still
leans on:

- extension-style trust assumptions
- shared-fate failure modes
- incomplete resource hardening
- UI path coupling
- insufficiently hard memory boundaries

That is not a criticism of the current work.
It is a precise architectural distinction.

---

## 22. Definition of done for the final model

Aegis should consider the isolated runtime complete only when all of the
following statements are true:

- an app cannot directly access resident memory it was not mapped
- an app cannot directly call resident services outside the syscall boundary
- an app cannot crash the shell merely by crashing itself
- an app cannot keep resident UI hostage after being killed
- an app cannot exceed its memory budget without deterministic enforcement
- an app cannot retain service handles after supervisor kill
- an app cannot bypass permission and capability policy through linkage tricks
- an app cannot create infinite timer or event pressure without revocation
- an app crash loop results in quarantine, not endless relaunch churn
- the runtime can prove that recovery reclaimed all instance-owned resources

Anything less should be described honestly as an intermediate runtime, not the
final isolated platform.

---

## 23. Architectural position

The final Aegis app platform should be described as:

> A strongly isolated managed native runtime for constrained devices.

That phrase is more accurate than:

- plugin platform
- extension loader
- dynamic app shell
- native module runtime

Those older descriptions were useful for bring-up.
They are not sufficient for the final system you want to build.
