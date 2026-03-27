# Aegis Foreground Page Contract

## 1. Purpose

This document defines the contract between:

- a foreground app that wants to present a page
- the resident Aegis runtime that stores page state
- the shell renderer that presents the approved result

It exists because "app has foreground" is not the same thing as "the shell now
renders arbitrary app memory directly".

Foreground presentation in Aegis is a governed data contract.

---

## 2. Core idea

A foreground page is not a live shared widget tree.
It is a structured page declaration published by the app and retained by the
runtime under shell governance.

The flow is:

1. app prepares page data
2. app submits page data through Host API / UI service
3. runtime validates and stores a runtime-owned page snapshot
4. shell renderer presents that stored snapshot
5. input is routed back as semantic events tied to a page token

This keeps shell authority visible while still allowing app-defined content.

---

## 3. What "foreground acquired" means

When logs say the app acquired foreground, that means only that:

- the app session is the current foreground owner
- foreground input may be routed to that session
- the app is allowed to publish foreground page data

It does not guarantee that:

- a complete page payload has already been declared
- the renderer has accepted all app-provided content
- stale placeholder content has been replaced
- command routing is already valid

This distinction is important during debugging.

---

## 4. Contract layers

The foreground page contract has three layers.

### 4.1 App declaration layer

The app declares:

- page id
- title or headline text
- context text
- visible rows or items
- command ids
- preferred softkey bindings
- page state token

### 4.2 Runtime governance layer

The runtime:

- validates counts, lengths, and ids
- copies retained data into runtime-owned memory
- stores the current approved snapshot
- associates it with the active session
- rejects malformed or stale submissions

### 4.3 Shell presentation layer

The shell:

- renders the approved page snapshot
- maps declared commands into visible softkeys under policy
- routes user actions back as semantic events
- preserves a predictable system escape path

---

## 5. Required page identity fields

Every foreground page should be identifiable without pointer identity.

Recommended minimum identity:

- app id or owning session
- page id
- page token
- optional logical context string such as current path

The page token must change when the currently routable page state changes in a
way that could make old events unsafe.

---

## 6. Page token semantics

The token is not decoration.
It exists to reject stale routed events.

The token should change when:

- selection changes and command targets change
- the page contents are replaced
- the page id changes
- a command table changes
- the foreground session is recreated

The token need not change for purely cosmetic redraws that do not affect event
meaning, but conservative updates are preferred over stale routing.

---

## 7. Snapshot rule

Foreground page declarations should be complete snapshots of the current
presentable state.

That means a page update should contain everything the renderer needs for the
current frame of meaning, rather than relying on hidden previous state inside the
renderer.

Benefits:

- simpler stale-event rejection
- clearer ownership
- easier logging and diagnostics
- easier shell fallback when a submission is invalid

---

## 8. Content model

The exact ABI structure may evolve, but the contract should support these kinds
of content safely:

- a headline
- a root or context label
- a bounded list of visible rows
- optional row icon references
- selection index or focus position
- command declarations
- softkey preferences

The contract should avoid open-ended object graphs and instead favor bounded
arrays and explicit lengths.

---

## 9. Command model

Commands are page-declared but shell-routed.

The page may declare:

- command id
- label
- semantic role if defined
- enabled or disabled state
- preferred softkey slot

The shell should decide how those commands are finally surfaced while preserving
system policy.

This prevents page logic from being rewritten into renderer hard code.

---

## 10. Softkey governance

Softkeys belong to the page contract, but not exclusively to the app.

The governing rules are:

- the page may request labels and bindings
- the runtime stores the approved declarations
- the shell may normalize, hide, or reject unsafe declarations
- the right-most escape path remains system-governed by default

Apps should therefore think of softkeys as requests under policy, not as direct
control over shell chrome.

---

## 11. Placeholder behavior

The shell may display a placeholder foreground page when:

- foreground has been granted but no page is declared yet
- the last declaration was rejected
- the app is between transitions
- the runtime deliberately falls back to a minimal foreground frame

Examples of placeholder information:

- app name
- app id
- root id
- basic status such as "foreground acquired"

If the placeholder remains visible while input routing still changes page tokens,
that usually indicates the app session is alive but the actual page payload is
not replacing the placeholder snapshot successfully.

That is a page-contract debugging clue, not automatically a launcher bug.

---

## 12. Copy and lifetime rules

The runtime must assume that incoming page data is app-owned and potentially
temporary.

Therefore:

- strings needed after the call should be copied
- command tables needed after the call should be rebuilt in runtime memory
- any nested pointers should be treated as unsafe unless explicitly copied
- renderer-visible page state should be runtime-owned

The shell should never rely on app stack or module pointers for retained page
rendering.

---

## 13. Icon and asset references in pages

Pages may reference icons or assets, but the contract must keep ownership clear.

Preferred model:

- the app provides an asset reference or path string
- the runtime validates and stores the reference
- the shell resolves or decodes it through runtime-managed services

Avoid:

- renderer retaining raw pointers into module-private asset tables
- page item structs containing host-retained pointers to app-owned icon metadata

If an asset reference is needed after the declaration call returns, it must be
retained in runtime-owned form.

---

## 14. Routed input events

Input should be routed back to the foreground app as semantic events, not raw UI
backend implementation details.

Examples:

- move next
- move previous
- activate current item
- invoke left softkey
- invoke center softkey
- invoke right softkey

Each routed event should be associated with:

- current session identity
- current page token
- command id or semantic action

If the current token no longer matches, the event should be dropped.

---

## 15. Failure handling

If a page declaration is malformed or unsafe, the runtime should prefer a clear
fallback to half-valid rendering.

Safe fallback behaviors include:

- keep the last valid runtime-owned snapshot
- show a minimal placeholder foreground frame
- log why the declaration was rejected
- reject routed events that target stale state

Unsafe behavior is to partially retain malformed app-owned structures and hope
the renderer can cope.

---

## 16. Debugging guidance

When the visible symptom is:

- app loads successfully
- foreground is acquired
- input continues to route
- but real page content never appears

check the foreground page contract first:

1. Was a page declaration actually submitted?
2. Was it accepted and copied by the runtime?
3. Did the page token advance as expected?
4. Were command tables or item arrays rejected or truncated?
5. Is the shell still rendering a placeholder snapshot?
6. Is any retained field relying on app-owned temporary memory?

This class of failure often looks like a shell rendering issue while actually
being a page snapshot ownership issue.

---

## 17. Minimal expectations for every app page

Every well-formed foreground page should, at minimum:

- identify itself
- declare bounded visible content
- declare routable commands
- provide a page token
- survive replacement and unload cleanly

If a page cannot satisfy those requirements, it should not be treated as a valid
foreground page publication.

---

## 18. Relationship to other documents

This document should be read together with:

- [Aegis Shell](./shell.md)
- [Aegis App ABI](./app-abi.md)
- [Aegis Host API](./host-api.md)
- [Aegis App Runtime Memory Model](./app-runtime-memory-model.md)
- [Aegis App Asset and Icon Contract](./app-assets-and-icons.md)

The shell document explains authority.
This document explains the exact data-sharing discipline that allows foreground
apps to present pages without taking over shell-owned rendering policy.
