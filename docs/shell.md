# Aegis Shell

## 1. Purpose

The Aegis Shell is the visible system surface through which users encounter the device world.

It is not merely a launcher page.  
It is the system-owned foreground environment that provides:

- home
- launcher
- settings
- status surfaces
- notifications
- app entry
- app return paths
- recovery surfaces after app failure

The shell exists to answer a simple question:

> When no app is currently in control of the foreground, what world does the user inhabit?

The answer is: the Aegis Shell.

---

## 2. Why the shell matters

In a monolithic firmware, “system” and “app” are often visually and structurally mixed together.

Aegis rejects that confusion.

Because Aegis distinguishes:
- resident system authority
- guest app workload

it must also distinguish:
- system-owned visible surfaces
- app-owned foreground surfaces

The shell is the visible expression of system authority.

Without it, return paths, recovery, launcher behavior, and settings governance would all become unclear.

---

## 3. Shell responsibilities

The shell is responsible for:

### 3.1 Home / launcher surface
Show available apps, system entry points, and main navigation.

### 3.2 Settings surface
Provide access to system settings and device-dependent settings surfaces.

### 3.3 Status surface
Expose system state such as:
- battery
- connectivity
- notifications
- app activity indicators
- device-specific status hints

### 3.4 Notification surface
Provide system-owned ways to present messages, alerts, or transient status to the user.

### 3.5 Foreground authority recovery
When an app exits or fails, the shell must retake visible control.

### 3.6 System identity
The shell is where the device presents itself as a governed runtime rather than a collection of pages.

---

## 4. Shell vs app

This distinction must remain sharp.

### Shell
- system-owned
- long-lived
- recovery authority
- may persist across app sessions
- may adapt to device profile
- may use privileged system paths

### App
- guest workload
- session-bound
- runtime-admitted
- revocable
- must return control to shell when stopped

The shell is not “just another app”.  
It is the resident visible authority of Aegis.

---

## 5. Shell layers

Aegis Shell should itself be internally layered.

### 5.1 Shell model layer
Responsible for:
- launcher list model
- settings model
- status model
- notification model
- shell navigation state

### 5.2 Shell presentation layer
Responsible for:
- rendering system surfaces
- device-aware layout adaptation
- user interaction routing while shell owns foreground

### 5.3 Shell control layer
Responsible for:
- switching between home / launcher / settings / notifications
- entering app sessions
- regaining control after exit/failure

This helps prevent shell code from degenerating into a UI-only tangle.

---

## 6. Core shell surfaces

The first Aegis shell should define at least these surfaces.

---

## 6.1 Home

Purpose:
- provide the main stable system landing surface
- offer access to launcher, settings, status, and possibly recent state

The exact visual form may vary by device.

The conceptual role does not.

---

## 6.2 Launcher

Purpose:
- present the app catalog
- show app icon, name, description, or key metadata
- allow user to request app start

The launcher should be driven by:
- `AppRegistry`
- `LauncherModel`
- app metadata from manifest

The launcher does not load apps itself.  
It requests that the runtime platform do so.

---

## 6.3 Settings

Purpose:
- present system-owned settings
- expose device-specific options through system governance
- keep settings distinct from app-local settings

Settings may include:
- runtime/system behavior
- device feature toggles
- connectivity/radio configuration
- display/input preferences
- storage/system information
- app-visible but system-governed settings

The settings surface belongs to the shell, not to any app.

---

## 6.4 Status surface

Purpose:
- expose live system indicators
- keep system state visible even while the device is app-centric

Examples:
- battery
- charging
- radio state
- GPS state
- notification indicators
- connection state
- current app identity or focus state

The exact shape may vary by device profile.

---

## 6.5 Notification surface

Purpose:
- give the system a controlled way to present transient or important information

Notifications may come from:
- system runtime
- services
- apps through the notification service

But presentation remains shell-owned.

Apps may request notifications.  
They do not own the notification surface.

---

## 6.6 Recovery / fallback surface

Purpose:
- provide a safe return point after app exit or failure
- ensure the foreground never falls into undefined visual state

This may be:
- launcher
- home
- a minimal recovery screen
- an error/fallback panel

The exact design may vary, but the role is non-optional.

---

## 7. Relationship to app lifecycle

The shell is deeply tied to lifecycle, but should not collapse into it.

### Before app start
The shell owns the foreground.

### During app running
The shell may still own system-level overlays or status functions, depending on policy, but foreground authority belongs to the app session.

### During stopping / teardown
The shell becomes the destination authority again.

### After exit or failure
The shell must provide the stable visible world to which the system returns.

This relationship is why shell and lifecycle must be designed together.

---

## 8. Relationship to device adaptation

The shell is one of the main consumers of `DeviceProfile` and `CapabilitySet`.

Why?

Because the shell must adapt to differences in:

- screen size
- aspect ratio
- orientation
- touch vs joystick vs keyboard input
- presence/absence of system features
- device-specific settings visibility

### Important rule
The shell may adapt to device profile.  
Apps should primarily adapt to capability.

This is a major architectural distinction.

The shell is allowed to be device-aware.  
Apps are expected to be capability-aware.

---

## 9. Input model

The shell must not assume a single input style.

Different devices may provide:

- touch
- keyboard
- joystick
- buttons
- rotary input
- mixed input systems

Therefore the shell should work from an abstract input/navigation model rather than raw board event assumptions.

Examples:
- move focus
- select
- back
- open menu
- open settings
- page navigation

When text entry is available, the shell should also be treated as an explicit focus owner:

- while shell owns foreground, shell owns text-input focus
- when a foreground app session is admitted, text-input focus may be granted to that session
- when the app stops or fails, focus must return to shell

This abstraction allows shell behavior to remain coherent across device classes.

---

## 10. Launcher model

The launcher should be driven by a dedicated model object.

A `LauncherModel` should represent:

- visible apps
- order/sorting
- hidden/internal apps
- invalid/incompatible apps
- current selection/focus
- category/grouping if used

The launcher should not directly scan the filesystem or parse manifests itself.

That work belongs to app registry/platform layers.

The shell consumes prepared app catalog data.

---

## 11. Settings model

The shell should also avoid hardcoding settings presentation as a scattered set of ad-hoc pages.

A `SettingsModel` should represent:

- system settings entries
- device-specific settings extensions
- visibility rules
- grouping and ordering
- read/write routes through settings service

This is important because settings are one of the places where system identity and device adaptation meet.

---

## 12. Shell authority boundaries

The shell must not:

- directly implement app runtime loading logic
- bypass lifecycle discipline
- become a giant mixed manager for all system concerns
- allow apps to directly take over shell-owned global surfaces
- collapse settings, status, launcher, and runtime into one inseparable blob

The shell is a major system layer, but it is not the whole system.

---

## 13. Shell and overlays

Aegis may later support shell-owned overlays that remain system-visible while an app is running.

Possible examples:
- status bar
- low battery warnings
- notifications
- system modal alerts

If implemented, these overlays must remain clearly shell-owned.

Apps may coexist with them.  
Apps do not own them.

This preserves visible system authority even during app execution.

---

## 14. Non-negotiable rules

### Rule 1
The shell is not just another app.

### Rule 2
Launcher does not own runtime loading; it only requests app entry.

### Rule 3
Settings remain system-owned, even if apps have their own local settings later.

### Rule 4
The shell must be the default recovery destination after app exit or failure.

### Rule 5
The shell may adapt to device profile.

### Rule 6
The shell must not collapse into a giant unstructured UI manager.

---

## 15. Minimal first implementation

The first useful shell should prove:

- the system can boot into a shell-owned surface
- the launcher can display app entries from registry
- the user can request app start
- the shell can surrender foreground to an app session
- the shell can retake foreground after app exit
- the settings surface can exist as a distinct system-owned path

Even if visually simple, these roles must already be real.

---

## 16. Summary

The shell exists to make one thing visible:

> Aegis is not only a runtime behind the scenes;  
> it is also a system with a foreground world of its own.

That world is the Aegis Shell.

---

## 17. UI design references

The shell document defines architectural role and authority boundaries.

The detailed UI design for the shell is specified in:

- [Aegis System UI Design](./ui-system.md)
- [Aegis Built-in System Entries](./system-apps.md)
- [Aegis Visual Specification](./ui-visual-spec.md)
- [Aegis UI Icon and Image Inventory](./ui-icon-inventory.md)
- [Aegis UI Patterns and Interaction Spec](./ui-patterns.md)
- [Aegis Shell Wireframes and Screen Contracts](./ui-wireframes.md)

These documents define:

- visual direction
- information architecture
- navigation rules
- page templates
- wireframes and screen contracts

They should be treated as the implementation-facing shell design reference set.
