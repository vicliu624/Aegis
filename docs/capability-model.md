
# Capability Model in Aegis

## 1. Purpose

Aegis is designed to run on multiple hardware devices.  
Those devices will not share identical hardware features.

Some devices may have:

- LoRa radio
- GPS
- keyboard input
- joystick input
- touch
- audio output
- microphone input
- removable storage
- USB hostlink
- strong power telemetry

Others may only provide a subset.

Because of this, Aegis cannot let apps depend on board identity.  
Instead, Aegis must let apps depend on **capabilities**.

The purpose of the capability model is to define how system-visible functionality is represented, queried, and used across devices.

---

## 2. Core principle

Aegis apps must adapt to **what the system can provide**, not to **which board is underneath**.

This means:

- apps do not ask “am I on device X?”
- apps ask “is capability Y available?”
- apps do not assume full support
- apps can react to absent / degraded / full support states

The capability model is the formal language of what a device can offer.

---

## 3. Why capability modeling is necessary

Without a capability model, multi-device support usually degenerates into one of these failures:

### 3.1 Board-specific app logic
Apps start branching on board identity.

### 3.2 Hidden assumptions
Apps assume keyboard, GPS, radio, or certain display geometry always exist.

### 3.3 Leaky services
Generic interfaces become contaminated with board names and special cases.

### 3.4 Shell/app confusion
It becomes unclear whether behavior differences are due to device constraints or sloppy architecture.

The capability model prevents this by making device-provided functionality explicit.

---

## 4. What a capability is

A capability is a system-recognized statement that a certain class of function is available to the shell and/or apps.

A capability is not:

- a driver name
- a GPIO mapping
- a board identity string
- an implementation detail

A capability is a higher-level statement such as:

- display available
- keyboard input available
- joystick input available
- radio messaging available
- GPS fix service available
- microphone input available
- battery status available

Capabilities exist above drivers and below app logic.

---

## 5. Capability categories

Aegis should organize capabilities into broad domains.

## 5.1 UI capabilities
Examples:

- display
- touch_input
- keyboard_input
- joystick_input
- pointer_input
- status_surface
- notification_surface
- vibration_feedback

## 5.2 Communication capabilities
Examples:

- radio_messaging
- mesh_transport
- usb_hostlink
- ble_transport
- network_transport

## 5.3 Positioning and sensing capabilities
Examples:

- gps_fix
- motion_sensor
- compass
- environmental_sensor

## 5.4 Storage capabilities
Examples:

- persistent_storage
- removable_storage
- filesystem_browse
- import_export

## 5.5 Power and system capabilities
Examples:

- battery_status
- charging_status
- suspend_resume
- low_power_mode
- system_time

## 5.6 Audio capabilities
Examples:

- audio_output
- microphone_input
- codec2_voice_path
- tone_playback

---

## 6. Capability state model

A capability should not be modeled as a naive boolean unless the domain truly is binary.

Aegis should support at least three states:

- `absent`
- `degraded`
- `full`

This matters because many embedded features are partial.

### Example: display
- absent → no screen
- degraded → very small or limited rendering capability
- full → intended display support

### Example: input
- absent → no usable input path
- degraded → limited buttons only
- full → full keyboard or rich interaction path

### Example: positioning
- absent → no GPS
- degraded → coarse or intermittent location source
- full → robust GPS fix capability

A capability state is therefore richer than feature presence.

---

## 7. Capability descriptor

Each capability should be representable as structured runtime data.

Example concept:

```cpp
enum class CapabilityLevel {
    Absent,
    Degraded,
    Full
};

struct CapabilityDescriptor {
    CapabilityId id;
    CapabilityLevel level;
    uint32_t flags;
};
````

This should be part of a wider `CapabilitySet`.

---

## 8. CapabilitySet

A `CapabilitySet` is the system’s structured summary of what is available on the current device.

Example sketch:

```cpp
struct CapabilitySet {
    CapabilityDescriptor display;
    CapabilityDescriptor keyboard_input;
    CapabilityDescriptor joystick_input;
    CapabilityDescriptor radio_messaging;
    CapabilityDescriptor gps_fix;
    CapabilityDescriptor audio_output;
    CapabilityDescriptor microphone_input;
    CapabilityDescriptor battery_status;
    CapabilityDescriptor removable_storage;
    CapabilityDescriptor usb_hostlink;
};
```

This object should be:

* created by the device adaptation layer,
* owned by the system,
* queryable by shell and apps,
* stable enough to support policy checks.

---

## 9. Relationship to services

Capabilities and services are related, but not identical.

### Capability

Says whether a class of function is available.

### Service

Provides the actual callable interface.

For example:

* capability: `gps_fix = full`
* service: `IGpsService`

Or:

* capability: `radio_messaging = absent`
* service: `INullRadioService`

This distinction matters because a service may still exist in API shape even when the device cannot actually provide the feature.

The capability tells the truth about availability.
The service provides the operational contract.

---

## 10. Relationship to DeviceProfile

`DeviceProfile` describes the device as a whole.
`CapabilitySet` describes what functionality is exposed.

A `DeviceProfile` contains a `CapabilitySet`, but they are not interchangeable.

### DeviceProfile answers:

* what kind of device is this?
* what input/display/storage/power topology does it have?

### CapabilitySet answers:

* what can shell and apps rely on here?

This distinction should remain sharp.

---

## 11. Who uses capabilities?

### 11.1 Shell

The shell may use capabilities to decide:

* which launcher interactions are appropriate
* which settings pages to show
* which status indicators to expose
* whether to show radio/GPS/audio controls

### 11.2 App runtime

The app runtime may use capabilities for:

* admission checks
* compatibility warnings
* resource policy
* disabling unsupported app entry

### 11.3 Apps

Apps may use capabilities to adapt behavior:

* offer a reduced UI path
* disable location features if GPS is absent
* hide voice tools if no microphone/audio path exists
* change interaction hints based on input capability

But apps must not branch on board identity.

---

## 12. Capability queries

Aegis should support runtime capability queries through a generic interface.

Example ideas:

```cpp
CapabilityLevel host_get_capability(CapabilityId id);
bool host_has_capability(CapabilityId id);
```

Or through a structured object returned by Host API:

```cpp
const CapabilitySet* host_get_capability_set();
```

The important thing is not the exact API shape, but the architectural rule:

> apps query capabilities through the system, not through board-specific knowledge.

---

## 13. Capability-based app design

An Aegis app should be written to:

1. declare what capabilities it prefers or requires
2. validate those expectations when starting
3. adapt when possible
4. fail clearly when impossible

This suggests app manifest support such as:

* required capabilities
* optional capabilities
* preferred capabilities

Example:

* required: display
* optional: gps_fix
* optional: radio_messaging
* preferred: keyboard_input

This gives the runtime a principled way to decide whether an app can start.

---

## 14. Capability compatibility

Capability compatibility should not be “all or nothing”.

The runtime should be able to reason about cases such as:

* app can start, but with reduced function
* app can start, but should warn user
* app cannot start because required capability is absent
* app can start because degraded capability still satisfies minimum requirement

This is where capability level becomes useful.

---

## 15. Non-negotiable rules

### Rule 1

Apps must not branch on board identity.

### Rule 2

Capabilities must be first-class runtime objects.

### Rule 3

Capabilities must be generic and board-agnostic.

### Rule 4

Capability truth must come from the system, not app assumptions.

### Rule 5

Capability checks belong to runtime policy and app startup logic, not scattered guesses.

### Rule 6

A service being present in code does not imply the capability is truly available.

---

## 16. Minimal first implementation

The first useful capability model should include a small but meaningful set, for example:

* display
* keyboard_input
* joystick_input
* radio_messaging
* gps_fix
* audio_output
* microphone_input
* battery_status
* removable_storage
* usb_hostlink

The first implementation should prove:

* two different devices produce different `CapabilitySet`s
* shell changes behavior accordingly
* at least one demo app queries capabilities
* the app behaves differently without knowing board identity

---

## 17. Summary

The capability model is one of the key ideas that makes Aegis a real multi-device platform.

It allows Aegis to say:

> device differences are real,
> but apps should see them as capability structure,
> not as board-specific chaos.

That is how Aegis keeps hardware variation contained while preserving a unified app world.