# Aegis App Runtime Memory Model

This document describes the current runtime memory ownership model and the rules
needed to stay safe in the present native-runtime implementation.

It should not be mistaken for the final isolation model.
The final target is stronger: per-app memory domains, fault-attributable
regions, bounded shared buffers, and runtime-enforced access boundaries.
See [Strongly Isolated App Runtime Blueprint](./isolated-app-runtime-blueprint.md).

## 1. Purpose

This document defines the runtime memory model for Aegis native apps.

It exists to answer the practical questions that the ABI document only names at a
high level:

- which side owns memory at any given boundary
- which pointers are safe to keep
- how long foreground page data remains valid
- what must happen at app stop and unload
- which patterns are especially risky for Zephyr LLEXT apps on Xtensa targets

Without these rules, an app may appear to "work" while still violating unload
safety, foreground presentation safety, or cross-boundary ownership discipline.

---

## 2. Scope

This document applies to all Aegis native apps, including:

- built-in apps shipped with the product
- system-governed apps such as `Files`
- externally packaged apps
- Zephyr LLEXT-loaded apps

It applies whether an app is visually prominent or not.
Being "built in" does not grant a separate memory model.

---

## 3. Core model

Aegis has three different ownership worlds that must not be blurred together:

1. resident system memory owned by Aegis
2. session-scoped runtime state owned by a running app session
3. app image memory owned by the loaded module and revoked at unload

The most important rule is:

> A pointer may only be retained by the side that owns the backing storage for
> the whole lifetime in which that pointer is used.

If lifetime cannot be proven, the data must be copied or represented as an
opaque handle.

---

## 4. Memory regions and lifetimes

### 4.1 Resident system memory

Resident system memory belongs to the always-alive Aegis runtime.

Examples:

- shell models
- service backends
- runtime registries
- device adaptation state
- resident caches that outlive any one app session

Apps may interact with these systems only through Host API and ABI contracts.
They must not retain raw pointers into resident internals unless the API
explicitly documents a borrowed lifetime.

### 4.2 Session memory

Session memory is runtime state created on behalf of one running app session.

Examples:

- tracked UI roots
- timer registrations
- page command tables
- routed event queues
- app-scoped allocations made through Host API

Session memory must be reclaimable in teardown even if the app fails to clean up.

### 4.3 Module image memory

Module image memory is the memory backing the loaded app binary and its data.

Examples:

- module `.text`
- module `.rodata`
- module `.data`
- module `.bss`
- static objects inside the app image

This memory ceases to be valid when the module unloads.
The host must never retain pointers into it after unload.

### 4.4 Stack memory

Stack memory is valid only for the dynamic extent of the call frame or thread
that owns it.

It must never be published across the ABI boundary unless the host consumes it
synchronously before the call returns and the contract explicitly allows that.

---

## 5. Ownership vocabulary

Every cross-boundary API should be describable using one of these terms:

- `owned by app`: the app allocates it and the app frees or invalidates it
- `owned by runtime`: the host allocates it and the runtime frees or revokes it
- `borrowed for call`: valid only during the current synchronous call
- `borrowed until next replace`: valid until the app publishes a newer snapshot
- `session-bound handle`: no pointer lifetime is exposed; the runtime tracks it

If an interaction cannot be described cleanly with this vocabulary, the design
is too implicit and should be revised.

---

## 6. Non-negotiable ownership rules

### Rule 1

No cross-boundary pointer may have an undocumented lifetime.

### Rule 2

The runtime must not retain pointers into app stack memory.

### Rule 3

The runtime must not retain pointers into app-owned temporary buffers unless the
contract explicitly says the data is copied before the call returns.

### Rule 4

The app must not retain pointers into resident shell or service internals.

### Rule 5

Anything that must survive app stop or unload must live in runtime-owned memory,
not in module-owned storage.

### Rule 6

Anything that is expensive or dangerous to revoke should be represented as a
tracked handle rather than as a shared object layout.

---

## 7. Preferred boundary patterns

The safest patterns across the app/runtime boundary are:

- fixed-size value structs
- explicit length-delimited buffers
- runtime-owned copies of page declarations
- opaque handles for live resources
- polling or message delivery through runtime-owned queues
- replace-whole-snapshot publication instead of incremental pointer graphs

The least safe patterns are:

- raw object pointers
- pointers to stack data
- pointers to STL-managed storage
- callbacks that assume resident C++ object identity across unload
- hidden sharing of static app buffers

---

## 8. Publication model for app-owned data

Foreground page data, command descriptors, labels, and similar app-declared UI
state should be treated as publications, not as shared mutable memory.

That means:

1. the app prepares a complete snapshot
2. the app submits it through Host API / ABI
3. the runtime validates and copies what it needs
4. the runtime renders from its own retained representation

The host should not render directly from module-owned strings or arrays after the
submission call has returned.

This is especially important for:

- page headline text
- context strings
- line arrays
- command ids
- softkey labels
- icon paths

---

## 9. Foreground page lifetime rules

Foreground page publication has caused confusion in practice, so the lifetime
rules are stated explicitly here.

### 9.1 What the app may do

The app may assemble a page declaration from:

- local temporary buffers
- static module strings
- generated arrays
- manifest-derived runtime strings

### 9.2 What the runtime must assume

The runtime must assume those buffers are not safe to retain by pointer.

The runtime should therefore:

- validate lengths and ids
- copy scalar fields directly
- copy required strings into runtime-owned storage
- rebuild command tables in runtime-owned memory
- reject oversized or malformed submissions

### 9.3 What must not happen

The shell renderer must not depend on app-owned buffers staying stable after the
page declaration call returns.

If a page update requires new content, the app should publish a new snapshot.

---

## 10. Routed events and stale tokens

Page state tokens exist because memory lifetime and event lifetime are different.

An input event may be generated after:

- focus changed
- a page was replaced
- a command table was rebuilt
- a foreground app stopped

Therefore event routing must be keyed by runtime-owned page state, not by raw
pointer identity.

The runtime should reject any event whose token no longer matches the current
foreground page snapshot.

This prevents stale events from targeting:

- already replaced command tables
- invalid page indices
- app state that has already been torn down

---

## 11. Strings and path buffers

Strings are one of the easiest ways to violate the memory model accidentally.

### 11.1 Safe patterns

- fixed-size ABI fields with explicit truncation rules
- runtime-owned copies of incoming strings
- app-owned buffers used only for the duration of the call
- path construction into a local buffer followed by immediate Host API call

### 11.2 Risky patterns

- host retaining `const char*` from module-owned storage
- path pointers derived from stack-local builders and reused later
- assuming a string literal in a module is equivalent to resident immutable
  storage
- returning pointers to hidden temporary buffers

### 11.3 Practical rule

If a string will be used later by the shell, service gateway, or renderer, it
must be copied into runtime-owned memory before the call returns.

---

## 12. Allocation discipline

Apps should allocate through Host API when the runtime needs visibility or
ownership tracking.

Examples:

- session-bound buffers retained by runtime-managed objects
- service request payloads whose lifetime extends beyond the call
- UI-related retained state

Apps may still use ordinary module-local allocation internally, but that memory
must remain entirely app-private and must not become an implicit runtime
dependency.

Mixed ownership patterns such as "app allocates, runtime silently frees later"
must be avoided unless the API explicitly defines them.

---

## 13. Stop, teardown, and unload

Memory safety in Aegis is not only about startup. It is mainly about revocation.

At app stop:

- no new page publications should be admitted
- no new timers should be created
- no new routed events should be queued for that session

At teardown:

- session-owned UI state is destroyed
- session-owned command tables are discarded
- tracked timers and subscriptions are revoked
- retained copies of app-published foreground data are released

At unload:

- module `.text`, `.rodata`, `.data`, and `.bss` become invalid
- any remaining host pointer into module memory is a bug

The runtime should be able to complete these phases without asking the app for
the final authority on what still exists.

---

## 14. Xtensa and Zephyr LLEXT implications

On Zephyr LLEXT targets, especially Xtensa-class boards, apparently harmless C++
patterns may become much less robust than they look in a monolithic firmware.

The runtime memory model should therefore assume higher risk for:

- pointer-heavy static metadata graphs
- complex static initialization
- relocation-sensitive string and table references
- callback designs that depend on stable resident addresses
- implementation techniques that rely on compiler-specific layout behavior

This does not mean such code is forbidden everywhere.
It means the app/runtime boundary must be designed so that the host does not
depend on those details for retained correctness.

---

## 15. Patterns that have already proven risky

The following patterns deserve explicit suspicion in Aegis app work:

- foreground page data published from structures whose nested pointers are not
  copied by the host
- static metadata tables where labels and resource paths are retained by pointer
- stack-local descriptors passed into a retained runtime table
- shell logic that keeps app-owned strings for later icon decode or command
  dispatch
- runtime code that uses module pointer identity as if it were resident object
  identity

When debugging a page that "looks loaded but never fully renders", memory
lifetime bugs should be considered before assuming the renderer is broken.

---

## 16. Checklist for new app-facing APIs

Before introducing a new app-facing ABI or Host API shape, check all of the
following:

1. Who allocates the data?
2. Who frees or revokes it?
3. Is the data copied or borrowed?
4. If borrowed, until exactly when?
5. Can the runtime tear it down without asking the app?
6. Does unload invalidate any retained pointer?
7. Should this be a handle instead of a pointer-bearing struct?

If any answer is vague, the API is not ready.

---

## 17. Relationship to other documents

This document complements, not replaces:

- [Aegis App ABI](./app-abi.md)
- [Aegis Host API](./host-api.md)
- [Aegis Foreground Page Contract](./foreground-page-contract.md)
- [Aegis LLEXT App Constraints](./llext-app-constraints.md)

The ABI document defines the public binary contract.
This document defines the ownership and lifetime discipline required to make that
contract safe in practice.
