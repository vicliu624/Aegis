# Aegis UI Patterns and Interaction Spec

## 1. Purpose

This document defines implementation-facing UI patterns for the Aegis shell.

It is intended to support:

- shell implementation planning
- LVGL or equivalent component design
- visual consistency
- board-adaptive layout decisions
- review of future UI changes

It complements:

- [Aegis Shell](./shell.md)
- [Aegis System UI Design](./ui-system.md)
- [Aegis Visual Specification](./ui-visual-spec.md)
- [Aegis UI Icon and Image Inventory](./ui-icon-inventory.md)

---

## 2. Global page anatomy

Most shell pages should use a common anatomy.

```text
+--------------------------------------------------+
| Status Bar                                       |
+--------------------------------------------------+
| Page Body                                        |
|                                                  |
|                                                  |
|                                                  |
+--------------------------------------------------+
| Softkey Bar or Action Hint Row                   |
+--------------------------------------------------+
```

### 2.1 Status bar

Responsibilities:

- establish current context
- keep system presence visible
- show critical device state

Recommended fields:

- time
- current area title
- battery
- storage indicator
- connectivity indicator
- warning indicator

### 2.2 Page body

Responsibilities:

- present the primary task
- keep focus obvious
- avoid competing sub-layouts

### 2.3 Softkey bar or action hint row

Responsibilities:

- reveal current action mapping
- reduce hidden interaction cost
- make non-touch operation learnable

Implementation rule:

- the row is rendered from page presentation data
- it is not generated from page-type heuristics inside the renderer

---

## 3. Layout density rules

The shell should use a compact but readable density.

### 3.1 Density targets

- avoid oversized touch-mobile spacing
- avoid unreadable terminal-style compression
- prefer stable rows over variable-height cards

### 3.2 Spacing rhythm

Use a small set of spacing steps, for example:

- tight
- normal
- section gap

Avoid many near-identical spacing values.

### 3.3 Borders and separators

Use separators to reveal structure:

- row dividers
- section borders
- group boxes

Do not rely on shadow stacks.

---

## 4. Focus treatment

Focus is a primary system signal.

### 4.1 Focus style hierarchy

The shell should support:

- primary focus
- selected or active state
- disabled or unavailable state

These states must not be confused with one another.

### 4.2 Recommended focus rendering

Preferred treatments:

- inverse fill
- high-contrast border
- solid highlight band
- modest background reversal

Avoid:

- glow
- animated halos
- weak low-contrast outlines

### 4.3 Focus persistence

When returning to a page, focus should restore to the prior useful object when feasible.

---

## 5. Input abstraction

Shell interaction should be defined in semantic input actions, not board-specific keys.

Recommended shell actions:

- `move_up`
- `move_down`
- `move_left`
- `move_right`
- `select`
- `back`
- `menu`
- `home`
- `page_prev`
- `page_next`

Optional extended actions:

- `context`
- `quick_action`
- `toggle`

These actions should be mapped by board packages without changing shell logic.

---

## 6. Softkey semantics

When a softkey bar is shown, semantics should be consistent.

Consistency applies to semantic role and routing policy, not to a renderer-owned
string table.

Pages declare desired bindings.
The shell reviews them.
The renderer displays the reviewed result.

### 6.1 Left slot

Use for the primary task-level action.

Examples:

- Launch
- Open
- Apply
- Start

### 6.2 Center slot

Use for select, inspect, or confirm.

Examples:

- Select
- Details
- Choose

### 6.3 Right slot

Use for navigation escape by default.

Examples:

- Back
- Cancel
- Menu

### 6.4 Consistency rule

Do not swap semantics casually between pages.
A user should be able to predict action intent before reading the labels in detail.

### 6.5 Command binding rule

Each softkey slot should bind to either:

- a system action
- a page command

This keeps touch input, hardware keys, and alternate renderers aligned behind
one interaction model.

### 6.6 Disabled commands

If a page command is unavailable in the current state, the corresponding
softkey should usually remain visible but disabled when that improves
learnability.

Examples:

- `Info` disabled on a directory row
- `Apply` disabled when there are no pending changes

The renderer should display disabled state clearly.
It should not silently replace the command with a different action.

---

## 7. Template A: Launcher Grid

### 7.1 Use cases

- Home
- Apps
- tool entry surfaces

### 7.2 Content rules

- show 6 to 9 primary items per page for small displays
- labels must be short
- icons must remain legible at small sizes
- avoid rich descriptions inside the grid

### 7.3 Item anatomy

Each grid item may include:

- icon
- short label
- small state marker

### 7.4 State markers

State markers may indicate:

- running
- incompatible
- blocked
- hidden by policy

Markers must remain compact and secondary.

### 7.5 Example

```text
+--------------------------------------------------+
| AEGIS / Apps                            10:42    |
+--------------------------------------------------+
| [Msg ] [Map ] [Radio]                             |
| [Files] [Tools] [Info ]                           |
| [Apps ] [Prefs] [More ]                           |
|                                                   |
| 8 installed, 2 hidden                             |
+--------------------------------------------------+
| Launch            Details               Back      |
+--------------------------------------------------+
```

---

## 8. Template B: Simple List

### 8.1 Use cases

- Services
- Control Panel groups
- notification history
- app list variants

### 8.2 Row anatomy

Each row may contain:

- label
- optional subtype or category
- right-aligned status
- optional leading symbol

### 8.3 Selection behavior

- one row focused at a time
- select opens the row's primary meaning
- menu opens secondary actions

### 8.4 Example

```text
+--------------------------------------------------+
| AEGIS / Services                        10:42    |
+--------------------------------------------------+
| > Radio                           Ready          |
|   Storage                         Mounted        |
|   Audio                           Degraded       |
|   Location                        Ready          |
|   Update Agent                    Idle           |
+--------------------------------------------------+
| Open              Details                Back    |
+--------------------------------------------------+
```

---

## 9. Template C: Inspector

### 9.1 Use cases

- app details
- device details
- capability details
- service details

### 9.2 Structure

Preferred structure:

- header identity block
- grouped sections
- label-value rows
- short action row

### 9.3 Content rule

Keep inspector pages concise and scannable.
They are for explanation, not for dense ungrouped dumps.

### 9.4 Example

```text
+--------------------------------------------------+
| AEGIS / App Info                        10:42    |
+--------------------------------------------------+
| Demo Info                                          |
| Native extension                                   |
|                                                    |
| Status              Ready                          |
| ABI                 Compatible                     |
| Permission          Limited                        |
| Required caps       display, storage               |
| Missing caps        hostlink_use                   |
+--------------------------------------------------+
| Launch            More                   Back     |
+--------------------------------------------------+
```

---

## 10. Template D: Dialog

### 10.1 Use cases

- confirmation
- warning
- policy explanation
- small choices

### 10.2 Dialog rules

- short title
- one short explanation block
- two or three actions
- obvious cancel or back path

### 10.3 Example

```text
+--------------------------------------+
| Unload this app?                     |
|                                      |
| Notes will close and return to Home. |
|                                      |
| [Yes]                      [No]      |
+--------------------------------------+
```

---

## 11. Copywriting rules

The shell should use compact, system-like language.

### 11.1 Preferred language qualities

- short
- explicit
- calm
- technical but readable

### 11.2 Avoid

- marketing tone
- chatty phrasing
- ambiguous verbs
- phone-platform vocabulary that hides system state

### 11.3 Preferred examples

- `Ready`
- `Mounted`
- `Incompatible with this device`
- `Permission not granted`
- `Return to shell`

---

## 12. Status vocabulary

Use a shared status vocabulary across the shell.

Recommended terms:

- Ready
- Active
- Busy
- Idle
- Degraded
- Unavailable
- Blocked
- Incompatible
- Warning

If color is used, it must reinforce this vocabulary rather than replace it.

---

## 13. Visual token guidance

This section defines implementation-oriented token guidance.

### 13.1 Color roles

Suggested semantic roles:

- background
- panel
- line
- text
- muted_text
- focus
- active
- warning
- error

### 13.2 Color family direction

Base palette should prefer:

- warm gray or paper-like neutrals
- dark gray or dark brown text
- deep blue, deep green, or restrained slate accents
- low saturation warning orange or red

Avoid neon and high-saturation primaries as default shell identity.

### 13.3 Corner radius

Recommended approach:

- square or lightly rounded major surfaces
- slightly rounded buttons only where needed

### 13.4 Border priority

Borders are more important than shadows in this design system.

### 13.5 Typography roles

Recommended text roles:

- title
- section
- body
- status
- hint

Keep the number of roles small.

The complete palette, semantic colors, button styling, typography roles, geometry tokens, and monochrome fallback rules are defined in:

- [Aegis Visual Specification](./ui-visual-spec.md)

---

## 14. Page-specific behavior guidance

### 14.1 Home

- grid entry should be immediate
- no scrolling if avoidable
- summary line should remain brief

### 14.2 Apps

- focus should stay on last launched or last inspected item when returning
- incompatible state should be visible before launch attempt

### 14.3 Running

- current foreground app should be visually distinct
- destructive runtime actions should require confirmation

### 14.4 Services

- service health should be translated into user-facing summaries
- raw driver detail should move into an inspector, not the list row

### 14.5 Control Panel

- group settings by mental model, not by implementation module
- avoid overly deep nesting

### 14.6 Device

- capability data should be sectioned and scannable
- policy-related limitations should be explicit

---

## 15. Adaptation guidance by device class

### 15.1 Small non-touch color display

Recommended:

- fixed status bar
- compact launcher grid
- visible focus reversal
- visible bottom action hints

### 15.2 Small non-touch monochrome display

Recommended:

- stronger border hierarchy
- monochrome-compatible state markers
- fewer simultaneous indicators

### 15.3 Touch-capable device

Recommended:

- preserve focus logic internally
- allow tap to move focus and activate
- do not replace system structure with gesture-first behavior

---

## 16. Accessibility and readability

Even on constrained devices, the shell should preserve readability.

Rules:

- never depend on color alone for state meaning
- keep labels short enough to avoid frequent truncation
- ensure focused row contrast remains high
- avoid tiny decorative icons that carry required meaning

---

## 17. Review checklist

When reviewing a shell screen or component, ask:

1. Is the primary task obvious?
2. Is the current focus unmistakable?
3. Does this page look shell-owned rather than app-like?
4. Is the language short and system-like?
5. Would this still work on a non-touch device?
6. Would this still make sense in monochrome?
7. Is system governance visible where it matters?

---

## 18. Summary

The Aegis shell should feel consistent because it reuses a small set of structures:

- stable page anatomy
- strong focus treatment
- compact status language
- few page templates
- board-adaptive layout without changing system meaning
