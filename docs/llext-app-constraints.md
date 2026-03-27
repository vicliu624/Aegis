# Aegis LLEXT App Constraints

This document describes the constraints that apply to the current Zephyr LLEXT
app path.

It should not be read as the final platform model.
LLEXT is a substrate for current bringup, not the final security boundary.
The final target is a strongly isolated managed native runtime that may still
use LLEXT beneath a stricter loader and isolation backend.
See [Strongly Isolated App Runtime Blueprint](./isolated-app-runtime-blueprint.md)
and [Runtime Refactor Slices](./runtime-refactor-slices.md).

## 1. Purpose

This document captures the implementation constraints for Aegis apps that are
loaded through Zephyr LLEXT.

It is not a generic C++ style guide.
It documents the patterns that are safe, risky, or forbidden specifically
because Aegis apps are:

- independently built
- admitted through a governed runtime
- loaded as native extensions
- expected to stop, tear down, and unload cleanly

---

## 2. Why this document exists

Many implementation patterns work acceptably in a monolithic firmware image but
become fragile once the app is a runtime-loaded module.

Symptoms often look misleading:

- the app loads successfully but the foreground page is incomplete
- input continues to work but the visible page stays on a shell placeholder
- strings, asset paths, or labels appear corrupted
- a module works on one build and fails after a seemingly harmless refactor

The root cause is often not the visible feature itself.
It is that the app relied on assumptions that are unsafe in a loadable-module
environment.

---

## 3. Design stance

LLEXT apps should be written as conservative native plugins.

That means preferring:

- simple C-compatible boundary structs
- explicit snapshots over retained pointer graphs
- runtime-owned copies for retained data
- simple control flow
- predictable storage duration

It means being cautious with:

- complex static initialization
- pointer-rich metadata tables
- boundary-visible C++ object graphs
- implementation techniques that depend on compiler or relocation behavior

---

## 4. Non-negotiable rules

### Rule 1

The app must behave correctly when loaded and unloaded multiple times.

### Rule 2

No host-retained correctness may depend on app stack memory.

### Rule 3

No host-retained correctness may depend on module-local pointers remaining valid
after teardown or unload.

### Rule 4

All foreground UI declarations must be safe under copy-in / revoke-out
semantics.

### Rule 5

Built-in apps do not get exceptions to these rules.

---

## 5. Safe coding patterns

The following patterns are preferred for LLEXT apps:

- `extern "C"` entrypoints for runtime-visible symbols
- flat POD-style descriptor structs across the boundary
- explicit integer ids instead of host-visible callback object graphs
- page publication as complete snapshots
- path construction into local buffers followed by immediate submission
- small helper functions with straightforward control flow
- runtime-side copying of retained strings and labels

These patterns are easier to reason about under relocation, unload, and lifetime
pressure.

---

## 6. High-risk patterns

The following patterns are not always wrong, but they are risky enough that they
should trigger review before landing:

- metadata tables with nested pointers that the host later traverses
- static arrays of descriptors that reference many other static objects
- heavy reliance on C++ global constructors
- hidden singleton state that survives conceptually across unloads
- stack-local descriptor trees published to retained host state
- switch-heavy control tables whose correctness depends on opaque generated jump
  layout
- assuming string literals or static `const char*` are safe retained ABI data
- board-facing direct IO from app code instead of Host API services

If one of these patterns is used, the code should justify why it remains safe in
the LLEXT environment.

---

## 7. Forbidden boundary patterns

The following boundary patterns should be treated as forbidden:

- host storing raw pointers to app-owned mutable state for later use
- app storing raw pointers to shell-owned mutable internals
- exceptions crossing the app/runtime boundary
- STL containers crossing the ABI boundary
- host-visible virtual object layouts used as ABI
- app code depending on shell-private fallback surfaces or special-case routes

These patterns are incompatible with clean unload and governed runtime behavior.

---

## 8. C++ guidance

Apps may be implemented in C++, but the exposed runtime boundary should stay
conservative.

Prefer:

- plain structs
- free functions
- explicit init and teardown
- internal C++ usage hidden behind a stable boundary

Use extra caution with:

- global constructors and destructors
- thread-local storage
- complex static object graphs
- type-erased callback frameworks
- ABI-sensitive template-heavy types near the boundary

The issue is not "C++ is forbidden".
The issue is that the runtime boundary must remain explainable and unload-safe.

---

## 9. Strings, labels, and asset paths

Strings deserve special caution in LLEXT apps.

### 9.1 Preferred approach

- build the string in app-local storage
- pass it through an ABI field or explicit buffer
- let the runtime copy it if the runtime needs to retain it

### 9.2 Avoid assuming

- module string literals are effectively resident forever
- `const char*` metadata tables are safe to retain by pointer
- asset paths built from static fragments are immune to module-boundary issues

When a string drives shell behavior after the call returns, it must be treated as
runtime-owned copied data.

---

## 10. Foreground page publication constraints

Foreground page publication is the most important LLEXT-facing UI rule.

Apps should publish foreground pages as:

- complete current-page snapshots
- fixed-size or bounded strings
- command ids, not host callbacks
- explicit item counts and bounds

Apps should not publish:

- open-ended pointer graphs
- host-retained pointers to nested app-owned text buffers
- page structures whose validity depends on the current stack frame

If a page changes, publish a new snapshot.
Do not assume the runtime is observing or sharing the app's internal state live.

---

## 11. Input and board access constraints

Foreground apps must not directly take over board-level input devices just because
they are in the foreground.

In particular, avoid:

- direct I2C polling of shared keyboard devices from app code
- direct GPIO scanning that duplicates board runtime responsibility
- parallel ownership of the same board input controller from shell and app paths

Input should flow through board/runtime providers and Host API semantics.
This is both a lifecycle rule and a stability rule.

---

## 12. Memory pressure constraints

On constrained boards, two different problems often get conflated:

1. link-time DRAM exhaustion
2. runtime instability caused by changing execution models carelessly

The safer sequence is usually:

1. reduce resident DRAM use
2. relocate suitable buffers or stacks to the correct memory region
3. keep proven execution models unless there is a strong reason to change them

Do not assume that replacing a dedicated thread with a work queue is a harmless
memory optimization.
On timing-sensitive board paths it can create a new correctness problem while
appearing to solve a memory one.

---

## 13. Package completeness constraints

A valid Aegis LLEXT app package should be self-contained enough for discovery,
loading, and basic presentation.

That typically means the package owns:

- `manifest.json`
- `app.llext`
- package icon asset
- any app-specific assets

Avoid designs where the shell must supply missing pieces from private locations
for a supposedly normal app package.

---

## 14. Built-in apps are still normal apps

`Files`, `Settings`, `Device`, `Diagnostics`, and similar product entries may be
prominent in the shell, but they should still follow the same LLEXT and runtime
constraints as any other Aegis app package.

The shell may decide:

- where the entry appears
- which label it uses in menus
- whether it is promoted on the first page

The shell should not decide:

- private lifecycle shortcuts
- alternate ownership rules
- private hard-coded UI contracts
- private icon fallback paths for one specific app

If a built-in app needs special behavior, that behavior should be expressed as a
documented platform rule, not as an undocumented exception.

---

## 15. Review checklist

When reviewing an LLEXT app change, ask:

1. Does the host retain any pointer into app memory?
2. Could this page or command declaration outlive the stack frame that built it?
3. Is any runtime-visible string assumed to stay valid without a copy?
4. Does this code introduce board-level direct access that bypasses Host API?
5. Does this app still unload cleanly if stopped immediately after bringup?
6. Would this design still make sense if the app were external rather than
   built in?

If the answer to any of these is uncomfortable, the design likely needs to be
simplified.

---

## 16. Relationship to other documents

This document should be read together with:

- [Aegis App Runtime Memory Model](./app-runtime-memory-model.md)
- [Aegis App ABI](./app-abi.md)
- [Aegis Host API](./host-api.md)
- [Aegis Foreground Page Contract](./foreground-page-contract.md)

The ABI document defines what the boundary is.
This document defines the coding discipline needed to make that boundary reliable
on Zephyr LLEXT targets.
