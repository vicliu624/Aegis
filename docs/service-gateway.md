# Aegis Service Gateway

## 1. Purpose

The Service Gateway is the system-owned boundary through which Aegis exposes capabilities to apps and higher-level runtime components.

Its purpose is to ensure that:

- hardware remains under system authority,
- apps interact with capabilities rather than raw device ownership,
- services remain generic across devices,
- and runtime lifecycle can govern resource usage and cleanup.

In short:

> apps do not own the machine;  
> they borrow structured capabilities from the system.

The Service Gateway is the formal mechanism that makes this true.

---

## 2. Why the Service Gateway exists

Without a Service Gateway, native apps would tend to couple themselves directly to:

- raw drivers
- board-specific peripherals
- global singletons
- device handles
- ad-hoc helper modules
- subsystem internals

That would create several architectural failures:

- hardware sovereignty leaks out of the system
- board-specific details spread across the codebase
- unload and teardown become unsafe
- app portability collapses
- runtime policy becomes unenforceable
- system evolution becomes painful

Aegis introduces the Service Gateway to prevent this.

---

## 3. Architectural position

The Service Gateway sits between:

- the app/runtime-facing Host API
- and the device adaptation / Zephyr-backed service implementations

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

This placement is intentional.

The Service Gateway is not the driver layer.
It is not the board package.
It is not the Host API itself.

It is the generic, system-facing service layer that translates runtime requests into controlled system operations.

---

## 4. Core responsibilities

The Service Gateway is responsible for:

### 4.1 Exposing generic service interfaces

Apps and runtime components should interact with services such as:

* UI
* text input
* storage
* settings
* logging
* timer
* radio
* GPS
* audio
* notifications
* power
* network
* hostlink

These services must be expressed in system-generic terms.

### 4.2 Preserving system ownership

The service gateway must ensure that service access never implies raw hardware ownership transfer.

### 4.3 Abstracting device differences

Different devices may implement the same service differently.
The service gateway must keep the app-facing contract stable.

### 4.4 Supporting lifecycle governance

Resources created or acquired through services must remain attributable to the current app session.

### 4.5 Supporting capability-aware behavior

Service access must align with current device capability truth.

---

## 5. Service Gateway vs Host API

These two are closely related, but they are not the same.

### Host API

The formal app/runtime boundary.

### Service Gateway

The system-owned internal service layer behind Host API.

Example:

* Host API exposes `service_call(...)`
* Service Gateway receives that call
* Service Gateway dispatches to `RadioService`, `GpsService`, `SettingsService`, etc.
* Those services may be bound to different concrete implementations per device

This separation is important.

The Host API is an ABI boundary.
The Service Gateway is a system architecture boundary.

---

## 6. Service Gateway vs Device Adaptation Layer

The Service Gateway is also distinct from the Device Adaptation Layer.

### Device Adaptation Layer

* knows board/package/profile/capability reality
* binds services to concrete implementations
* absorbs board-specific orchestration

### Service Gateway

* presents stable generic services to the rest of Aegis
* routes requests without leaking board identity
* remains above device-specific detail

The Device Adaptation Layer creates the concrete service world.
The Service Gateway presents it in a stable way.

---

## 7. Service domains

Aegis should organize services into clear domains.

---

## 7.1 UI Service

Purpose:

* create app-owned UI roots
* expose app-safe UI attachment points
* maintain runtime ownership discipline

Possible responsibilities:

* create root container
* destroy root container
* obtain screen metrics
* expose layout hints
* post UI tasks through runtime-safe paths

Non-goal:

* apps directly mutating shell-owned global UI structures

---

## 7.2 Storage Service

Purpose:

* provide app access to system-governed storage
* preserve namespace and path policy
* abstract storage backend differences

Possible responsibilities:

* open/read/write app-scoped files
* enumerate app data directory
* import/export helpers
* report storage availability

Non-goal:

* exposing raw filesystem internals as app-owned territory

---

## 7.3 Text Input Service

Purpose:

* expose text-entry state through a stable, device-agnostic contract
* separate raw keyboard hardware events from interpreted text semantics
* keep modifier/layer policy under system control

Possible responsibilities:

* report text-entry availability
* report current modifier/layer state
* expose latest interpreted text input snapshot
* expose current text-focus owner and routing state
* allow runtime-governed focus transfer between shell and the current app session

Non-goal:

* exposing board-specific keypad controller details to apps
* forcing apps to decode matrix coordinates or driver-private scan codes
* allowing an app to impersonate another session when claiming focus
---

## 7.4 Settings Service

Purpose:

* provide app-visible persistent settings under system governance
* keep settings namespace structured
* allow shell and apps to share coherent configuration behavior

Possible responsibilities:

* get/set values
* namespace policy
* defaults and migration helpers
* device-specific visibility rules

Non-goal:

* allowing apps to directly control low-level persistence backends

---

## 7.5 Logging Service

Purpose:

* provide system-owned logging paths
* support diagnostics and runtime auditing
* preserve unified logging format and control

Possible responsibilities:

* structured log write
* levels and tags
* optional persistence
* future forwarding or export

Non-goal:

* unmanaged app-local output chaos

---

## 7.6 Timer Service

Purpose:

* provide runtime-governed timers
* ensure timer ownership is session-bound
* support teardown cleanup

Possible responsibilities:

* create/cancel timers
* session ownership tagging
* policy around repeat timers
* future deadline/task helpers

Non-goal:

* unmanaged long-lived timer objects outside runtime tracking

---

## 7.7 Radio Service

Purpose:

* provide messaging/transport capabilities without exposing raw radio ownership
* let devices with different radio hardware expose a unified service contract

Possible responsibilities:

* send/receive messaging payloads
* capability query
* status reporting
* transport mode selection where policy allows

Non-goal:

* apps directly controlling board-specific radio drivers

---

## 7.8 GPS Service

Purpose:

* expose position/fix/time data through a stable service contract
* avoid apps binding directly to GPS hardware details

Possible responsibilities:

* query current fix
* inspect availability/quality
* subscribe to updates
* access time/location state through system policy

Non-goal:

* app access to raw device driver state

---

## 7.9 Audio Service

Purpose:

* expose audio output/input related capabilities in a controlled way

Possible responsibilities:

* play tones
* audio path state
* future voice path controls
* query microphone/audio availability

Non-goal:

* apps directly orchestrating codec or peripheral drivers

---

## 7.10 Notification Service

Purpose:

* allow apps and system components to post user-visible notices through shell governance

Possible responsibilities:

* transient notifications
* alerts
* badge or state integration
* shell-surface mediation

Non-goal:

* apps drawing directly into status or notification surfaces without system approval

---

## 7.11 Power Service

Purpose:

* expose battery, charging, power-state, and policy-aware power information

Possible responsibilities:

* battery status
* charging status
* low-power state information
* device-specific power hints

Non-goal:

* apps directly taking over device-wide power policy

---

## 7.12 Hostlink / Network / External Connectivity Services

Purpose:

* expose external communication pathways in generic terms
* allow device-specific backends without leaking transport details

Possible responsibilities:

* USB hostlink
* BLE transport
* network transport
* bridge status query

Non-goal:

* making apps own the underlying transport stack

---

## 8. Service interface design rules

All service interfaces should follow these rules:

### 8.1 Generic naming

No board-specific names in app-facing or runtime-facing service contracts.

### 8.2 Capability-oriented semantics

The service contract should express what the system can do, not how a specific board wires it.

### 8.3 Ownership clarity

Any object or handle obtained through a service must have clear ownership semantics.

Text-input focus is one example:

* shell-owned focus is system authority
* app-owned focus is a session-bounded grant
* teardown must revoke that grant automatically
* a foreground app should only be able to claim that grant if runtime policy and manifest permission both allow it

### 8.4 Session awareness

Where relevant, service-created resources must be attributable to the current app session.

### 8.5 Conservative surface area

Do not dump every internal subsystem function into service interfaces.

---

## 9. Service binding

The Service Gateway should not instantiate device-specific implementations itself.

Instead:

* Device Adaptation Layer / BoardPackage creates the backend implementation
* ServiceBindingRegistry binds the implementation
* Service Gateway exposes the generic contract

This separation matters because it keeps:

* board/package logic below
* app/runtime logic above
* the gateway itself stable and device-agnostic

---

## 10. Capability truth vs service availability

A service interface existing in code does not imply the device truly supports that function fully.

Examples:

* a GPS service may exist, but current device capability may be `absent`
* a radio service may exist, but be backed by a null implementation
* a display service may exist, but only support degraded UI behavior

Therefore:

* service contracts and capability truth must both exist
* apps and runtime should consult capabilities for admission/adaptation
* service calls should remain valid and well-defined even when capability is absent or degraded
* permission-aware service domains should still be enforced at runtime, not only during initial admission

This prevents the API from collapsing into board checks.

---

## 11. Access patterns

The Service Gateway may be accessed in one of two broad ways:

### 11.1 Through generic typed services

Example:

* `ISettingsService`
* `IRadioService`
* `IGpsService`

### 11.2 Through a generic service dispatch path

Example:

* `service_call(service_id, op_id, ...)`

Aegis may begin with a smaller, generic dispatch-oriented shape and later introduce typed service layers where appropriate.

The exact shape can evolve, but the architectural boundary must remain.

---

## 12. Relationship to ResourceOwnershipTable

The Service Gateway must cooperate with runtime ownership discipline.

If an app acquires:

* a timer
* a UI root
* a subscription
* a session-scoped service handle

the runtime must be able to reclaim it during teardown.

That means services must either:

* directly register ownership,
* or return objects whose ownership is visible to the runtime.

Without this, service access and unload safety are incompatible.

---

## 13. Relationship to shell

The shell also uses services, but as a privileged system surface rather than as a guest app.

This distinction matters:

* shell may use more system-internal paths
* app must use Host API mediated paths
* shared underlying services may exist, but not identical authority levels

The shell is system-owned; apps are guests.
The service model must preserve that distinction.

---

## 14. Non-negotiable rules

### Rule 1

No app-facing service contract may expose board-specific details.

### Rule 2

No service may silently transfer hardware sovereignty to apps.

### Rule 3

Service access must remain compatible with runtime teardown and unload.

### Rule 4

Service contracts must align with capability truth.

### Rule 5

Service Gateway must stay above device adaptation, not replace it.

### Rule 6

Board-specific binding must not leak upward into runtime or app logic.

---

## 15. Minimal first implementation

The first useful Service Gateway should include only a small, high-value set:

* settings
* logging
* timer
* UI
* capability query support
* one stub communication service
* one stub positioning service

This is enough to prove:

* system-owned services exist
* apps can use them through Host API
* ownership can remain session-aware
* device-specific implementations can vary below the gateway

---

## 16. Summary

The Service Gateway exists to make one architectural truth real:

> the system may lend capabilities to apps,
> but it must never surrender ownership of the device world.

That is the role of the Service Gateway in Aegis.
