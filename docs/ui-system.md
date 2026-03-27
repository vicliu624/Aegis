# Aegis System UI Design

## 1. Purpose

This document defines the system UI direction for Aegis.

It is not a skin guide and not a collection of isolated screen mockups.
It defines the visible behavior of Aegis as a governed device platform.

The goal is to give the project a UI system that:

- fits constrained MCU hardware
- works well on non-touch devices
- scales across multiple board families
- keeps system authority visible
- gives later implementation work a stable target

---

## 2. Product position

Aegis should not present itself as a modern phone OS.

It is not:

- an iOS-like card launcher
- an Android-style app market shell
- a Linux desktop squeezed onto a small screen
- a decorative retro emulator shell

Aegis should present itself as:

> a governed device workbench

This means the UI must constantly communicate three facts:

1. the user is operating a device platform, not a loose app collection
2. hardware belongs to the system, not directly to apps
3. app lifecycle, device services, and permissions are system-governed

---

## 3. Design lineage

The Aegis UI direction combines ideas from three historical systems.

### 3.1 Palm OS

Take:

- low cognitive load
- short paths
- quiet visuals
- tool-first launcher
- low noise layouts

Do not take:

- nostalgia for its own sake
- skeuomorphic imitation
- weak system structure

### 3.2 BlackBerry OS

Take:

- clear focus-driven interaction
- directional efficiency
- predictable select and back behavior
- stable softkey semantics

Do not take:

- keyboard-specific assumptions that do not generalize
- dense shortcut schemes that break on minimal input devices

### 3.3 Symbian

Take:

- strong system feeling
- layered control surfaces
- explicit settings and service structure
- higher information density where needed

Do not take:

- menu mazes
- deep option nesting
- fragmented interaction vocabulary

---

## 4. Design statement

The design statement for Aegis is:

> Aegis UI is Palm-inspired, quiet, utility-first, and device-governed.  
> It should feel like a serious handheld system, not a modern phone launcher.

This statement must guide:

- shell implementation
- visual design
- page layout
- component behavior
- documentation
- future mockups and assets

---

## 5. Primary UI principles

The system UI should follow these principles.

### 5.1 Utility first

The UI must help the user do useful work quickly.
Visual novelty must never outrank clarity.

### 5.2 Low cognitive load

Users should understand the current screen quickly.
The system should reduce choice pressure, not increase it.

### 5.3 One screen, one main purpose

Each screen should have a dominant task:

- choose an app
- inspect a status
- change a setting
- confirm an action

Avoid mixed dashboards that compete for attention.

### 5.4 Focus must always be obvious

On non-touch devices, a single primary focus target must always be visually clear.

### 5.5 System authority must remain visible

The shell must feel distinct from apps.
Apps run inside Aegis.
They do not replace Aegis as the visible authority.

### 5.6 Structure before decoration

Hierarchy should come from:

- layout
- spacing
- borders
- typography
- selection state

It should not depend on heavy shadows, glass effects, or decorative animation.

### 5.7 Few templates, reused consistently

Aegis should use a small number of page templates and interaction patterns.
This improves learnability and lowers implementation cost.

### 5.8 Graceful degradation

The same design language must degrade across:

- color and monochrome displays
- touch and non-touch devices
- different display sizes
- partial capability presence

---

## 6. UI character

The intended character of the UI is:

- calm
- compact
- disciplined
- tactile
- monochrome-friendly
- device-like

The UI should feel like a reliable handheld tool.
It should not feel like a consumer media surface.

---

## 7. Information architecture

The shell should expose a stable top-level structure.

```text
Aegis Shell
|- Home
|- Apps
|- Running
|- Services
|- Control Panel
`- Device
```

These are shell domains, not app domains.

The built-in user-facing system entry set is defined in:

- [Aegis Built-in System Entries](./system-apps.md)

### 7.1 Home

Purpose:

- default landing surface
- quick access to common actions
- stable device entry point
- brief system readiness summary

Character:

- Palm-like
- shallow
- calm
- sparse

### 7.2 Apps

Purpose:

- full app catalog
- app discovery and launch
- filtering and status visibility

Must express:

- install presence
- compatibility state
- hidden/internal app rules
- manifest-derived metadata

### 7.3 Running

Purpose:

- show active or recoverable app sessions
- expose runtime state as a user-visible system concept

Must make Aegis feel like a managed runtime rather than a static menu firmware.

### 7.4 Services

Purpose:

- show system-owned service status
- expose major subsystems without leaking raw driver details

Examples:

- radio
- storage
- bluetooth
- network
- audio
- location
- update agent

### 7.5 Control Panel

Purpose:

- system settings only
- no confusion with app-local preferences

Examples:

- display settings
- input settings
- audio policy
- connectivity toggles
- runtime policy
- storage actions

### 7.6 Device

Purpose:

- device identity
- capability inspection
- firmware information
- storage health
- compatibility and policy visibility

This is where Aegis most clearly presents itself as a device platform.

### 7.7 Built-in entry baseline

For the current product direction, the built-in user-facing entry set is:

- Apps
- Settings
- Files
- Device
- Services
- Running
- Notifications
- Power
- Connectivity
- Diagnostics
- Store

These entries are built into the system design and should not be treated as ordinary removable apps.

---

## 8. Shell behavior model

The shell is not just a launcher.
Its visible behavior should enforce the system architecture.

### 8.0 Presentation rule

The shell must present pages through a structured presentation contract.

That contract should include:

- page identity
- visible content
- page commands
- softkey declarations
- command enablement state

UI backends render this contract.
They must not guess softkeys from a surface enum or page type.

### 8.1 Before app start

The shell owns the foreground and input focus.

### 8.2 During app running

The app owns the main foreground session, but shell-owned system affordances may remain visible by policy.

Possible shell-visible elements:

- status bar
- urgent alerts
- system notifications
- recovery overlays

### 8.3 After app exit or failure

Foreground must return to a shell-owned recovery destination.

Preferred recovery targets:

- the previous launcher position
- Home
- a recovery panel when launch failed

### 8.4 Lifecycle visibility

The UI should make runtime governance visible where helpful.

Possible user-visible app states:

- available
- incompatible
- permission-limited
- running
- suspended
- failed

---

## 9. Interaction philosophy

The interaction model should default to focus-first, not touch-first.

This must work on devices that may only provide:

- 5-way input
- rotary input
- confirm/select
- back
- a small number of auxiliary buttons

Touch, when present, is additive rather than foundational.

---

## 10. Global navigation rules

The navigation model should be stable across the shell.

### 10.1 Primary directional behavior

- up and down move within lists
- left and right move across grid columns or switch adjacent tabs when the page uses tab-like top sections
- select activates the focused object
- back returns to the previous shell level or exits a transient surface
- menu opens secondary actions for the current page or focused item

### 10.2 Focus rules

- exactly one primary focus target at a time
- focus should persist when returning from a child surface when possible
- focus should land on the most useful default object when entering a screen
- focus must never disappear silently

### 10.3 Back behavior

Back should be predictable:

- dialog -> close dialog
- inspector -> return to previous list or grid
- top-level area -> return to Home
- Home -> no destructive behavior by default

On devices with a visible softkey bar, the right slot should default to a
system-governed back path.
Pages may request alternate wording where policy allows, but they should not
remove the existence of a predictable system escape route.

### 10.4 Long-press behavior

Long-press should be used sparingly and consistently.

Allowed uses:

- open context menu
- open global quick actions
- invoke alternate page action when there is no dedicated key

Long-press must never be required for basic navigation.

---

## 11. Visual system

The visual system should express seriousness, clarity, and discipline.

### 11.1 Visual keywords

- low saturation
- high contrast
- strong edges
- quiet surfaces
- clear states
- restrained color accents

### 11.2 Shapes

Preferred shape language:

- square or lightly rounded panels
- clear borders
- compact controls
- no oversized pills

### 11.3 Shadows

Use little or no heavy shadow.
Hierarchy should come from structure rather than floating cards.

### 11.4 Icons

Icons should be:

- simple
- symbolic
- small
- consistent in stroke weight
- system-tool-like rather than consumer-app-like

### 11.5 Typography

Typography should prioritize:

- compact readability
- stable rhythm
- short labels
- clear distinction between title, body, and status text

The system should use few text scales.

The detailed visual token set is defined in:

- [Aegis Visual Specification](./ui-visual-spec.md)

The required icon and image inventory is defined in:

- [Aegis UI Icon and Image Inventory](./ui-icon-inventory.md)

---

## 12. Theme families

Aegis should define two compatible theme families.

### 12.1 Monochrome Utility

Target:

- low power displays
- monochrome displays
- e-paper
- very limited memory/rendering budgets

Characteristics:

- line-first
- mostly border-defined
- no dependency on color semantics
- strong contrast and selection inversion

### 12.2 Color Executive

Target:

- color displays
- richer shell status expression
- slightly stronger system presence

Characteristics:

- restrained accent colors
- clearer state differentiation
- modest filled icons where useful
- still quiet and compact

### 12.3 Compatibility rule

Color Executive must degrade cleanly into Monochrome Utility without changing:

- navigation logic
- layout structure
- page hierarchy
- semantic meaning

---

## 13. Core page templates

Aegis should rely on four core templates.

### 13.1 Template A: Launcher Grid

Used for:

- Home
- Apps
- top-level tool entry surfaces

Purpose:

- direct entry
- low cognitive load
- Palm-like tool access

Characteristics:

- fixed grid
- short labels
- small item count per page
- direct launch or direct entry

### 13.2 Template B: Simple List

Used for:

- Services
- Control Panel groups
- file-like or record-like system lists
- notifications

Purpose:

- efficient directional browsing
- dense but readable information

Characteristics:

- one focused row
- optional right-aligned status
- optional section headers

### 13.3 Template C: Inspector

Used for:

- app detail
- device detail
- manifest detail
- service detail
- capability inspection

Purpose:

- expose structured information without turning every screen into a form

Characteristics:

- label-value rows
- compact grouped sections
- sparse action set

### 13.4 Template D: Dialog

Used for:

- confirm
- warn
- explain
- choose one of a few actions

Purpose:

- interruption with very low ambiguity

Characteristics:

- small footprint
- short copy
- two or three actions
- obvious default and safe exit path

---

## 14. Global shell chrome

The shell should provide stable framing elements.

### 14.1 Status bar

The top status bar should be fixed and reliable.

It may display:

- current time
- current shell title
- battery status
- storage presence
- radio or connectivity indicators
- warning state

Its job is not decoration.
Its job is persistent system presence.

### 14.2 Softkey bar

On devices with enough space, the shell should support a bottom action bar with three stable semantic slots:

- left: primary page action
- center: select, inspect, confirm, or secondary page action
- right: system back path by default

Example semantics:

- `Launch / Details / Back`
- `Open / Select / Back`
- `Apply / Select / Cancel`

Devices without a visible softkey bar should still preserve the same semantic action mapping.

These slots must be driven by page presentation data rather than renderer-owned
hard-coded strings.

Each slot should have a structured declaration that includes:

- label
- visible or hidden state
- enabled or disabled state
- semantic role
- dispatch target
- either a system action or a page command id

The shell reviews this declaration and renders the approved result.

Foreground apps participate in the same model.
They may declare page commands and preferred softkey bindings, but the shell
retains governance over final presentation and reserved system semantics.

---

## 15. Home screen specification

Home should be the most calm page in the system.

### 15.1 Home goals

- show immediate next actions
- avoid noise
- communicate device readiness
- anchor the system identity

### 15.2 Home content model

Home should include:

- 6 to 9 primary entries at most
- short labels
- a short readiness summary
- limited status detail

Home should not become:

- a dashboard wall
- a notification feed
- a widget canvas
- a service control overload

### 15.3 Example Home entries

- Apps
- Files
- Radio
- Tools
- Control Panel
- Device

---

## 16. Apps screen specification

The Apps surface is the full managed catalog.

### 16.1 Must show

- app name
- icon or symbol
- compatibility state
- permission-limited state where relevant
- total catalog count summary

### 16.2 App metadata states

Examples:

- ready
- incompatible with this device
- hidden by policy
- internal
- permission-limited
- already running

### 16.3 Primary actions

- select: launch or enter app details depending on shell policy
- menu: details, pin, hide, disable, remove, diagnostics

Common actions should be direct.
Secondary actions belong in the menu.

---

## 17. Running screen specification

The Running surface exists to expose runtime management.

It should make visible:

- current foreground session
- suspended or resumable sessions if supported
- recent failure or forced teardown state if useful

This surface is one of the clearest ways to show that Aegis is a governed runtime.

---

## 18. Services screen specification

The Services surface should present system-owned subsystems as inspectable units.

Each row should express:

- service name
- current state
- brief health or availability summary

States may include:

- ready
- disabled
- unavailable
- degraded
- busy
- policy-limited

This page should not dump raw driver internals by default.
It should translate them into system language first.

---

## 19. Control Panel specification

Control Panel is where the user changes system behavior.

It must remain clearly system-owned.

Suggested first groups:

- Display
- Input
- Audio
- Radio and Connectivity
- Storage
- Runtime
- Notifications
- Power

App-local settings should never masquerade as Control Panel entries.

---

## 20. Device screen specification

The Device surface explains what machine the user is holding and what Aegis currently knows about it.

Suggested sections:

- device identity
- board family
- firmware version
- storage summary
- capability set
- permission and runtime policy notes
- diagnostics and health

This page is where capability governance becomes visible to users.

---

## 21. State semantics

The system should standardize a small set of user-visible states.

Recommended shared states:

- ready
- active
- busy
- degraded
- unavailable
- blocked
- incompatible
- warning

These should map consistently across pages, labels, icons, and color usage.

---

## 22. App governance expression

Because Aegis is built on governed native apps, the UI should expose governance where it matters.

Examples:

- an app can be present but incompatible
- an app can be present but denied by policy
- an app can have some capabilities available and others missing
- an app can return to shell after runtime-controlled teardown

The UI should not hide these realities behind phone-like simplicity.
It should explain them in concise system language.

---

## 23. Responsive adaptation

The system UI should adapt by device profile, not by ad hoc board conditionals.

Adaptation inputs include:

- display resolution
- monochrome vs color
- touch availability
- keyboard presence
- rotary or joystick presence
- softkey affordance presence

Adaptation outputs may include:

- grid density
- list density
- label truncation rules
- visible softkey bar presence
- status bar detail density
- inspector section folding

The information architecture should stay stable even when layout density changes.

---

## 24. Things Aegis must avoid

Do not introduce these patterns as the main design direction:

- large floating cards
- decorative dashboards
- heavy gradients or glow
- app-store style icon illustration
- interaction that assumes touch first
- deep nested drawers
- ambiguous focus state
- excessive animation
- system pages that look indistinguishable from app pages

---

## 25. First implementation priorities

The first serious shell implementation should prove:

1. the shell has a stable visual identity
2. the top-level structure is real
3. focus-driven navigation is consistent
4. app governance is visible in the launcher and detail surfaces
5. services and device information are first-class surfaces
6. the design degrades across multiple device classes

---

## 26. Summary

Aegis should look and behave like a quiet, disciplined handheld system.

Its visible world should combine:

- Palm-like directness
- BlackBerry-like directional efficiency
- Symbian-like system layering

But its philosophy remains uniquely Aegis:

> the system owns the machine,  
> and apps are admitted, governed native guests inside that machine.
