# Aegis App Lifecycle

## 1. Purpose

In Aegis, lifecycle is not an incidental detail.

An app is not simply “opened” and “closed”.  
It is a runtime-governed native workload with explicit admission, execution, teardown, and recovery semantics.

The purpose of the lifecycle model is to make the following questions answerable in a strict and predictable way:

- when does an app officially exist in the system?
- when is an app only present, but not running?
- when is an app allowed to consume resources?
- when does the system revoke app authority?
- what must happen before a binary can be unloaded?
- what does recovery mean after app failure?

Lifecycle exists to preserve system authority and unload safety.

---

## 2. Why lifecycle must be explicit

Without an explicit lifecycle model, MCU systems usually drift into one of these failures:

- “enter page” is treated as “app started”
- “leave page” is treated as “app exited”
- timers outlive the UI that created them
- callbacks survive after app teardown
- resource ownership becomes impossible to audit
- unload becomes unsafe or purely aspirational
- app failure has no defined return path

Aegis rejects this ambiguity.

Lifecycle must be a first-class runtime concept.

---

## 3. Lifecycle objects

The lifecycle model is centered around these key objects:

- `AppDescriptor`
- `AppSession`
- `RuntimeLoader`
- `ResourceOwnershipTable`
- `AegisShell`

### 3.1 AppDescriptor
Represents a discovered and validated app package known to the system.

### 3.2 AppSession
Represents a runtime-recognized app instance that has been granted execution rights.

### 3.3 RuntimeLoader
Responsible for the loading, bringup, teardown, and unload of the native binary.

### 3.4 ResourceOwnershipTable
Tracks app-owned runtime resources that must be reclaimed during stop/teardown.

### 3.5 AegisShell
Provides the visible system return surface after exit or failure.

---

## 4. Lifecycle phases

Aegis should define the app lifecycle explicitly as the following phases.

### 4.1 Discovered

The app exists in storage and has been found during app scan.

Properties:
- app directory exists
- manifest file exists
- package is known to registry
- no admission decision yet
- binary not loaded

This phase means:
> the system knows the app exists.

---

### 4.2 Validated

The app has passed static validation.

Checks may include:
- manifest syntax valid
- required manifest fields present
- referenced files exist
- package structure acceptable

Properties:
- app may appear in launcher
- app still not running
- no runtime memory allocated yet

This phase means:
> the app is structurally valid enough to be considered for execution later.

---

### 4.3 Load Requested

The user or system has requested that this app be started.

Typical triggers:
- user selects app in launcher
- system action requests app start
- future automation or deep-link path

Properties:
- runtime admission checks begin
- capability checks occur
- ABI checks occur
- singleton policy checks occur

This phase means:
> execution is being considered, but not yet granted.

---

### 4.4 Loaded

The native binary has been loaded into runtime memory.

Properties:
- binary file opened
- runtime loader engaged
- binary sections resolved/bound
- entry symbol resolved
- Host API prepared
- app still not fully running until bringup succeeds

This phase means:
> code is now present in the runtime, but startup is not yet complete.

---

### 4.5 Started

The runtime has invoked app bringup/entry and startup completed successfully.

Properties:
- Host API is active
- app context is initialized
- initial runtime-owned structures may exist
- app can begin building foreground behavior

This phase means:
> the app has become a live runtime participant.

---

### 4.6 Running

The app has an active runtime session.

Properties:
- app session exists
- app may own UI root(s)
- app may own timers
- app may hold service-backed handles
- app may receive input
- resource ownership must be tracked

This phase means:
> the app is the active guest workload in the foreground or recognized active context.

---

### 4.7 Stopping

The system has initiated app stop.

Typical triggers:
- user exits app
- runtime policy forces stop
- app failure requires controlled unwind
- app replacement / restart
- system shutdown path

Properties:
- foreground ownership is revoked
- new app-side activity should stop being admitted
- timers/subscriptions/input should begin detaching
- teardown sequence begins

This phase means:
> the system is reclaiming authority over all app-owned activity.

---

### 4.8 Torn Down

Runtime cleanup has completed.

Properties:
- UI roots destroyed
- timers canceled
- subscriptions removed
- service handles revoked
- teardown callbacks completed
- app-specific session state no longer active

This phase means:
> the app has no remaining live runtime presence except possibly the loaded binary image.

---

### 4.9 Unloaded

The native binary has been detached from runtime memory.

Properties:
- loader state released
- symbol linkage removed
- app code no longer resident
- session object eligible for destruction

This phase means:
> the app is no longer present as executable code in the runtime.

---

### 4.10 Returned to Shell

Control has returned to a stable system surface.

Typical targets:
- launcher
- home
- fallback shell page
- error/recovery view

Properties:
- app session fully inactive
- shell owns visible interaction again
- runtime is ready for next app request

This phase means:
> the system has fully recovered control of the foreground world.

---

## 5. State diagram

```text
Discovered
   ↓
Validated
   ↓
Load Requested
   ↓
Loaded
   ↓
Started
   ↓
Running
   ↓
Stopping
   ↓
Torn Down
   ↓
Unloaded
   ↓
Returned to Shell
````

Not every failure path reaches every phase in the same way.
For example, a load failure may return to shell without reaching `Started` or `Running`.

---

## 6. Failure paths

Lifecycle must define failure behavior explicitly.

### 6.1 Validation failure

If static validation fails:

* app remains excluded from runnable state
* launcher may hide it or mark it invalid
* no load attempt occurs

### 6.2 Admission failure

If capability or ABI checks fail at startup request:

* app does not move to `Loaded`
* shell should remain stable
* runtime should produce clear failure reason

### 6.3 Load failure

If binary load fails:

* app does not move to `Started`
* partial loader allocations must be reclaimed
* control returns to shell

### 6.4 Bringup failure

If startup entry fails:

* runtime must transition toward stop/cleanup
* no partial running state may be left behind
* control returns to shell or recovery surface

### 6.5 Runtime failure

If app crashes or violates runtime expectations:

* foreground ownership must be revoked
* runtime must attempt controlled stop
* cleanup must reclaim as much ownership as possible
* shell regains control

Aegis must always prefer “return to a stable shell state” over undefined foreground behavior.

---

## 7. Relationship to AppSession

`AppSession` is the runtime’s formal representation of app existence.

An app may be known by the registry without having a session.
Only once execution is admitted should an `AppSession` exist.

This distinction is critical.

### Registry existence

Means:

* the system knows about the app

### Session existence

Means:

* the system has granted app runtime participation

These must never be conflated.

---

## 8. Relationship to ResourceOwnershipTable

Lifecycle is only meaningful if the runtime knows what the app owns.

At minimum, the ownership table should track:

* UI roots
* timers
* subscriptions
* service-backed handles
* future async work handles

The stop/teardown path must use this table to reclaim resources predictably.

Without ownership tracking, `Stopping` and `Torn Down` are not real states.

---

## 9. Foreground ownership

Aegis should treat foreground presence as an explicit grant, not an incidental page stack fact.

When an app enters `Running`:

* it becomes the active foreground owner
* shell delegates the visible interaction surface
* app input routing is enabled
* session-scoped text-input focus may be granted by the runtime

When an app enters `Stopping`:

* foreground ownership is revoked
* shell becomes the recovery/return authority again
* session-scoped text-input focus must be revoked before final return

This matters because shell and app must never be confused about who controls the visible world.

---

## 10. Lifecycle transitions

Transitions must be explicit and validated.

Suggested examples:

* `Discovered -> Validated`
* `Validated -> LoadRequested`
* `LoadRequested -> Loaded`
* `Loaded -> Started`
* `Started -> Running`
* `Running -> Stopping`
* `Stopping -> TornDown`
* `TornDown -> Unloaded`
* `Unloaded -> ReturnedToShell`

Illegal or skipped transitions should be treated as runtime errors.

For example:

* jumping directly from `Running` to `Unloaded` without teardown
* allowing new timers during `Stopping`
* reusing stale session resources after `TornDown`

The lifecycle model is a policy, not just an enum.

---

## 11. Suggested state enum

Example:

```cpp
enum class AppLifecycleState {
    Discovered,
    Validated,
    LoadRequested,
    Loaded,
    Started,
    Running,
    Stopping,
    TornDown,
    Unloaded,
    ReturnedToShell
};
```

This is illustrative only.
The important part is the state semantics and legal transition discipline.

---

## 12. Lifecycle hooks

The runtime may later support explicit lifecycle hooks for apps.

Possible examples:

* `on_start`
* `on_foreground`
* `on_stop`
* `on_teardown`

But even if app-level hooks exist, system lifecycle authority remains above them.

App hooks may participate in lifecycle.
They do not define lifecycle.

---

## 13. Non-negotiable rules

### Rule 1

Lifecycle must not be approximated by page navigation alone.

### Rule 2

An app may not be considered running without an `AppSession`.

### Rule 3

Stopping must revoke app authority before unload.

### Rule 4

Teardown must happen before unload.

### Rule 5

Shell must be the final recovery authority after failure.

### Rule 6

Resource ownership must be reclaimable at lifecycle boundaries.

---

## 14. Minimal first implementation

The first useful Aegis lifecycle implementation should prove:

* app can be discovered and validated
* launcher can request app start
* runtime can create a session
* app can enter running state
* user can exit app
* runtime can stop, tear down, unload
* shell regains control afterward

Even if the first implementation is simple, these boundaries must already be real.

---

## 15. Summary

The Aegis lifecycle model exists to make one truth enforceable:

> an app in Aegis does not merely appear and disappear;
> it is admitted, granted execution, governed, revoked, torn down, and released by the system.

That is what makes Aegis a runtime, not just a menu system.
