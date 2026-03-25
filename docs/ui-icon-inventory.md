# Aegis UI Icon and Image Inventory

## 1. Purpose

This document defines the image and icon resources needed to implement the Aegis shell.

It covers:

- built-in system entry icons
- status bar icons
- capability and service icons
- common interaction icons
- state and feedback icons
- brand and placeholder resources

This document exists to prevent icon work from becoming ad hoc or stylistically inconsistent.

---

## 2. Design principles

The Aegis icon system should match the shell design language.

Icons should be:

- compact
- clear at small sizes
- monochrome-capable
- consistent in stroke weight
- symbolic rather than illustrative

Icons should not be:

- glossy
- high-saturation by default
- app-store style mini illustrations
- dependent on gradients or shadows
- so detailed that they collapse at `16 px`

---

## 3. Delivery format

The preferred source format is:

- `SVG`

Recommended export sizes:

- `16x16`
- `20x20`
- `24x24`
- `32x32`

The icon system should be designed so that:

- monochrome rendering remains readable
- color-capable rendering can add restrained semantic emphasis
- the same icon silhouette works across device classes

---

## 3.1 Prompting baseline

If icons are generated through an image model or used as concept drafts for later manual cleanup, prompts should preserve a single visual language.

Use this baseline in every icon prompt:

> Create a compact handheld-system UI icon for Aegis.  
> Palm-inspired, utility-first, quiet, disciplined, monochrome-friendly.  
> Simple vector icon, centered, isolated on plain light background, no text, no logo, no mockup, no shadow, no gradient, no glossy effect, no 3D, no photorealism.  
> Strong silhouette, consistent stroke weight, readable at 16px and 24px, suitable for SVG cleanup.

Recommended negative prompt:

> no photo, no realistic object, no app-store illustration, no colorful mascot, no glassmorphism, no skeuomorphic shading, no perspective mockup, no background scene, no typography, no watermark

When preparing production assets, generated outputs should still be normalized and redrawn into a clean SVG family.

## 4. Resource groups

The shell image system should be organized into these groups:

1. built-in entry icons
2. status and indicator icons
3. capability and service icons
4. common interaction icons
5. state and feedback icons
6. brand and placeholder assets

---

## 5. Built-in entry icons

These icons are required for the built-in system entries defined in:

- [Aegis Built-in System Entries](./system-apps.md)

### 5.1 Required built-in icons

| Icon id | Entry | Recommended semantic shape | Prompt suffix |
|---|---|---|---|
| `apps` | Apps | grid or tiled launcher | `Depict a tidy launcher grid made of small square tiles, balanced and geometric.` |
| `settings` | Settings | gear | `Depict a compact gear icon with clear teeth and a stable mechanical feel.` |
| `files` | Files | folder | `Depict a simple folder icon with a short tab, clean and readable.` |
| `device` | Device | handheld device or device panel | `Depict a compact handheld device front view or system device panel outline.` |
| `services` | Services | stacked modules or service blocks | `Depict stacked service modules or linked system blocks, structured and technical.` |
| `running` | Running | play arrow, active stack, or running task marker | `Depict an active running-task symbol, such as a play mark with stacked layers.` |
| `notifications` | Notifications | bell or alert marker | `Depict a clean notification bell or alert marker, restrained and system-like.` |
| `power` | Power | battery with power symbol | `Depict a battery or power-state icon with a compact power glyph.` |
| `connectivity` | Connectivity | wireless waves plus node | `Depict wireless signal waves with a central node or link point.` |
| `diagnostics` | Diagnostics | wrench, stethoscope line, or device check symbol | `Depict a maintenance or device-check symbol, such as a wrench with a diagnostic line.` |
| `store` | Store | package, catalog tray, or install box | `Depict a governed software catalog icon, like a package box or install tray, not a shopping bag.` |

### 5.2 Built-in icon prompts

Each built-in icon can use:

`baseline prompt + prompt suffix`

Example:

> Create a compact handheld-system UI icon for Aegis. Palm-inspired, utility-first, quiet, disciplined, monochrome-friendly. Simple vector icon, centered, isolated on plain light background, no text, no logo, no mockup, no shadow, no gradient, no glossy effect, no 3D, no photorealism. Strong silhouette, consistent stroke weight, readable at 16px and 24px, suitable for SVG cleanup. Depict a tidy launcher grid made of small square tiles, balanced and geometric.

### 5.3 Guidance

The built-in icon set must feel like one family.

It should not mix:

- some icons that are purely line-based
- some icons that are filled pictograms
- some icons that look like modern mobile app icons

---

## 6. Status and indicator icons

These icons are used in the shell status bar, service detail views, notices, and device summary surfaces.

### 6.1 Battery icons

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `battery_full` | full battery | `Depict a battery icon showing full charge with a strong, clear fill level.` |
| `battery_75` | high battery | `Depict a battery icon showing about three quarters charge.` |
| `battery_50` | medium battery | `Depict a battery icon showing about half charge.` |
| `battery_25` | low battery | `Depict a battery icon showing about one quarter charge.` |
| `battery_low` | critical battery | `Depict a battery icon in critical low state, still simple and readable.` |
| `battery_charging` | charging battery | `Depict a battery icon with a charging bolt or charging indicator.` |
| `battery_unknown` | battery status unknown | `Depict a battery icon with an unknown-state indicator, such as a subtle question mark or uncertain mark.` |

### 6.2 Wireless and connection icons

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `wifi_off` | wifi disabled | `Depict a Wi-Fi symbol in an off or disabled state.` |
| `wifi_on` | wifi available | `Depict a simple Wi-Fi symbol with clear arcs.` |
| `wifi_connected` | wifi connected | `Depict a Wi-Fi symbol in connected state with a subtle confirmation cue.` |
| `wifi_limited` | wifi limited | `Depict a Wi-Fi symbol with a restrained limited or warning cue.` |
| `bluetooth_off` | bluetooth disabled | `Depict a Bluetooth symbol in an off or disabled state.` |
| `bluetooth_on` | bluetooth available | `Depict a clean Bluetooth symbol in normal available state.` |
| `bluetooth_connected` | bluetooth connected | `Depict a Bluetooth symbol with a subtle connection confirmation cue.` |
| `gps_off` | gps disabled | `Depict a GPS or location marker in an off or unavailable state.` |
| `gps_searching` | gps searching | `Depict a GPS or location marker with a searching or acquisition cue.` |
| `gps_locked` | gps fix locked | `Depict a GPS or location marker with a clear locked-fix cue.` |
| `radio_off` | radio disabled | `Depict a radio communication symbol in an off state.` |
| `radio_idle` | radio available but idle | `Depict a radio communication symbol in quiet idle state.` |
| `radio_active` | radio active | `Depict a radio communication symbol actively transmitting or receiving.` |
| `radio_error` | radio error | `Depict a radio communication symbol with an error or fault cue.` |
| `signal_0` | no signal | `Depict a signal-strength indicator with zero bars.` |
| `signal_1` | weak signal | `Depict a signal-strength indicator with one bar.` |
| `signal_2` | low-medium signal | `Depict a signal-strength indicator with two bars.` |
| `signal_3` | medium-high signal | `Depict a signal-strength indicator with three bars.` |
| `signal_4` | strong signal | `Depict a signal-strength indicator with four bars.` |

### 6.3 Storage and link icons

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `sd_absent` | SD not present | `Depict an SD card icon in absent or removed state.` |
| `sd_present` | SD present | `Depict a compact SD card icon in present state.` |
| `sd_busy` | SD busy | `Depict an SD card icon with a subtle busy or activity cue.` |
| `sd_error` | SD error | `Depict an SD card icon with a restrained error cue.` |
| `usb_disconnected` | USB absent | `Depict a USB connector icon in disconnected state.` |
| `usb_connected` | USB present | `Depict a USB connector icon in connected state.` |
| `usb_hostlink` | host link active | `Depict a USB or cable link icon representing an active host link or bridge.` |

### 6.4 Notice severity icons

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `info` | informational notice | `Depict a compact informational symbol, calm and system-like.` |
| `success` | success result | `Depict a success symbol with restrained confirmation, not celebratory.` |
| `warning` | warning result | `Depict a warning symbol, clear and serious.` |
| `error` | failure result | `Depict an error symbol, compact and unmistakable.` |
| `notification_dot` | pending notice marker | `Depict a very small pending-notice marker suitable for status overlays.` |

---

## 7. Capability and service icons

These icons are used in:

- Device
- Services
- capability inspection
- app detail and permission views
- diagnostics surfaces

### 7.1 Recommended capability icon set

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `display` | display capability | `Depict a simple display or screen panel icon.` |
| `keyboard` | keyboard input | `Depict a compact keyboard icon.` |
| `pointer` | pointer input | `Depict a pointer or cursor icon in a system style.` |
| `joystick` | joystick input | `Depict a joystick or directional control icon.` |
| `touch` | touch input | `Depict a finger-touch or touchpoint icon, minimal and technical.` |
| `speaker` | audio output | `Depict a speaker icon for audio output.` |
| `microphone` | audio input | `Depict a microphone icon, compact and clean.` |
| `storage` | storage capability | `Depict a generic storage device icon.` |
| `removable_storage` | removable storage | `Depict removable media or storage card icon.` |
| `radio` | radio messaging capability | `Depict a communication radio symbol, not a retro broadcast radio.` |
| `gps` | gps capability | `Depict a location or GPS fix icon.` |
| `network` | networking capability | `Depict network nodes or a simple network link symbol.` |
| `bluetooth` | bluetooth capability | `Depict a clean Bluetooth symbol.` |
| `usb` | usb capability | `Depict a USB connector icon.` |
| `hostlink` | hostlink capability | `Depict a bridge or host-link connection symbol.` |
| `battery` | battery reporting capability | `Depict a generic battery status icon.` |
| `power_sleep` | sleep and power state | `Depict a sleep or low-power symbol, such as crescent plus power cue.` |
| `vibration` | vibration capability | `Depict a vibration or haptic pulse symbol.` |
| `notification` | notification capability | `Depict a system notice or bell symbol.` |
| `security` | secure/system protection | `Depict a shield or protection symbol, disciplined and plain.` |
| `permission` | permission and access control | `Depict a controlled-access symbol, such as shield plus keyhole or lock cue.` |
| `update` | update capability | `Depict an update symbol with an arrow or cycle cue.` |
| `logs` | logs and records | `Depict a document or record log icon.` |

### 7.2 Guidance

These icons should remain more system-like than consumer-facing.

For example:

- `radio` should read as a communication device symbol, not a retro broadcast radio
- `hostlink` should read as link or bridge, not a decorative cable
- `permission` should read as controlled access, not a playful badge

---

## 8. Common interaction icons

These icons support shell navigation and actions.

### 8.1 Required action icons

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `back` | go back | `Depict a back arrow, compact and unambiguous.` |
| `forward` | go forward | `Depict a forward arrow, compact and unambiguous.` |
| `up` | move up | `Depict a simple upward arrow for navigation.` |
| `down` | move down | `Depict a simple downward arrow for navigation.` |
| `left` | move left | `Depict a simple left arrow for navigation.` |
| `right` | move right | `Depict a simple right arrow for navigation.` |
| `select` | select or confirm | `Depict a select or confirm icon, such as a centered dot in a frame or a check cue.` |
| `menu` | open menu | `Depict a compact menu icon with stacked horizontal lines.` |
| `more` | more actions | `Depict a more-actions icon, such as three dots, balanced and simple.` |
| `search` | search | `Depict a magnifying glass icon, clean and compact.` |
| `filter` | filter | `Depict a filter or funnel icon, simple and technical.` |
| `sort` | sort | `Depict a sort icon with ordered lines or arrows.` |
| `refresh` | refresh | `Depict a refresh icon with circular arrows.` |
| `open` | open item | `Depict an open or launch symbol, such as arrow leaving a frame.` |
| `close` | close item | `Depict a close symbol, simple and clear.` |
| `check` | confirmed or done | `Depict a compact checkmark symbol.` |
| `add` | add item | `Depict a plus symbol, strong and balanced.` |
| `remove` | remove item | `Depict a minus or remove symbol.` |
| `edit` | edit item | `Depict a pencil or edit mark, compact and clean.` |
| `pin` | pin item | `Depict a pin or fixed marker icon.` |
| `unpin` | unpin item | `Depict an unpin or released-pin icon.` |
| `lock` | locked | `Depict a simple lock symbol.` |
| `unlock` | unlocked | `Depict an unlocked padlock symbol.` |
| `expand` | expand section | `Depict an expand icon, such as outward arrows or chevron open state.` |
| `collapse` | collapse section | `Depict a collapse icon, such as inward arrows or chevron closed state.` |

### 8.2 Guidance

These icons should be especially legible at `16 px`.

They are functional UI glyphs, not decorative symbols.

---

## 9. State and feedback icons

These icons complement text-based state labels.

They must not replace text entirely for important system meaning.

### 9.1 Recommended state icon set

| Icon id | Meaning | Prompt suffix |
|---|---|---|
| `ready` | ready state | `Depict a ready-state symbol, compact and calm.` |
| `active` | active state | `Depict an active-state symbol with restrained energy.` |
| `busy` | busy state | `Depict a busy-state symbol, such as a clock or progress cue.` |
| `paused` | paused state | `Depict a paused-state symbol using clear pause bars.` |
| `warning` | warning state | `Depict a warning symbol, serious and legible.` |
| `blocked` | blocked or denied state | `Depict a blocked or denied symbol, compact and unmistakable.` |
| `unavailable` | unavailable state | `Depict an unavailable-state symbol, muted but clear.` |
| `incompatible` | incompatible state | `Depict an incompatibility symbol, such as mismatch or split shape.` |
| `policy_limited` | policy-limited state | `Depict a policy-restricted symbol, such as shield with restriction cue.` |
| `degraded` | degraded state | `Depict a degraded-state symbol between ready and warning.` |
| `installed` | installed state | `Depict an installed-state symbol, such as package with check cue.` |
| `not_installed` | not installed | `Depict a not-installed package symbol, simple and clear.` |
| `update_available` | update available | `Depict an update-available symbol, such as package with upward or refresh cue.` |

### 9.2 Rule

Important states should still be expressed through:

1. text
2. optional icon
3. optional color

Do not reduce critical meaning to a color-only dot or icon-only signal.

---

## 10. Brand and placeholder assets

These assets help the shell remain coherent even when third-party packages lack polished artwork.

### 10.1 Recommended brand assets

| Asset id | Meaning | Prompt suffix |
|---|---|---|
| `aegis_mark_small` | small symbol mark | `Create a restrained Aegis brand mark for small UI use, geometric and system-like.` |
| `aegis_mark_large` | large symbol mark | `Create a restrained Aegis brand mark for splash and larger UI use, geometric and calm.` |
| `boot_logo` | boot identity asset | `Create a boot logo for Aegis, centered, quiet, disciplined, system-owned, not flashy.` |
| `shell_wordmark` | shell text mark concept | `Create a clean Aegis wordmark concept for a handheld system shell, restrained and technical.` |

### 10.2 Required placeholder assets

| Asset id | Meaning | Prompt suffix |
|---|---|---|
| `placeholder_app_icon` | fallback app icon | `Create a placeholder application icon for Aegis, neutral, intentional, and system-owned.` |
| `placeholder_store_item_icon` | fallback store item icon | `Create a placeholder software catalog icon for Aegis store items, neutral and orderly.` |

### 10.3 Guidance

Placeholder assets should feel intentional and system-owned.

They should not look like:

- broken image icons
- generic web placeholders
- temporary developer markers

---

## 11. First-priority asset pack

If only one first asset pack is prepared initially, it should contain these resources.

### 11.1 Built-in entries

- `apps`
- `settings`
- `files`
- `device`
- `services`
- `running`
- `notifications`
- `power`
- `connectivity`
- `diagnostics`
- `store`

### 11.2 Status core

- `battery_full`
- `battery_50`
- `battery_low`
- `battery_charging`
- `wifi_on`
- `wifi_connected`
- `bluetooth_on`
- `bluetooth_connected`
- `gps_locked`
- `radio_active`
- `sd_present`
- `usb_connected`
- `warning`
- `error`
- `info`

### 11.3 Common actions

- `back`
- `menu`
- `more`
- `check`
- `refresh`

This first pack is enough to build:

- Home
- system status bar
- built-in entry launcher
- Device summary
- Services summary
- basic notice surfaces

---

## 12. Directory structure

The recommended asset directory layout is:

```text
assets/ui/icons/
  builtins/
    apps.svg
    settings.svg
    files.svg
    device.svg
    services.svg
    running.svg
    notifications.svg
    power.svg
    connectivity.svg
    diagnostics.svg
    store.svg

  status/
    battery_full.svg
    battery_50.svg
    battery_low.svg
    battery_charging.svg
    wifi_on.svg
    wifi_connected.svg
    bluetooth_on.svg
    bluetooth_connected.svg
    gps_locked.svg
    radio_active.svg
    sd_present.svg
    usb_connected.svg
    warning.svg
    error.svg
    info.svg

  capabilities/
    display.svg
    keyboard.svg
    touch.svg
    speaker.svg
    microphone.svg
    storage.svg
    radio.svg
    gps.svg
    network.svg
    bluetooth.svg
    usb.svg
    hostlink.svg
    permission.svg
    update.svg
    logs.svg

  actions/
    back.svg
    up.svg
    down.svg
    left.svg
    right.svg
    menu.svg
    more.svg
    search.svg
    filter.svg
    sort.svg
    refresh.svg
    check.svg
    add.svg
    remove.svg
    edit.svg

  brand/
    aegis_mark_small.svg
    aegis_mark_large.svg
    boot_logo.svg
    placeholder_app_icon.svg
    placeholder_store_item_icon.svg
```

---

## 13. Monochrome compatibility rule

The icon system should be authored from a single source family.

Recommended approach:

- maintain one unified SVG source set
- require every icon to remain readable in monochrome
- allow restrained color accents only as an optional layer

Do not create two unrelated icon families for:

- color devices
- monochrome devices

The shape language must stay the same.

---

## 14. Implementation guidance

When the shell implementation begins, the code should not directly depend on arbitrary filenames spread across the codebase.

Instead, it should eventually use a stable icon registry or asset mapping layer such as:

- built-in entry id -> icon id
- service state -> icon id
- capability id -> icon id
- status state -> icon id

This prevents board-specific or page-specific duplication of icon naming.

---

## 15. Summary

The Aegis shell needs a disciplined icon and image inventory because the visual system depends on small, reusable, readable symbols rather than decorative artwork.

This inventory defines the baseline resource set needed to implement the interface consistently across devices.
