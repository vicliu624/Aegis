# Device Adaptation in Aegis

## 1. Purpose

Aegis is not firmware for a single board.

It is a **resident device runtime** and **native app platform** intended to run across multiple hardware devices.  
This means device differences must be absorbed by architecture, not leaked into app code.

The purpose of the device adaptation layer is to make this possible.

It exists to answer a simple question:

> How can the same Aegis Core and App Runtime run on different hardware devices without turning the codebase into scattered board-specific conditionals?

The answer is:

- define a formal device adaptation layer,
- model devices as profiles,
- model exposed functionality as capabilities,
- bind generic services to concrete hardware implementations,
- and isolate board-specific orchestration inside board packages.

---

## 2. Design goals

The device adaptation layer must satisfy the following goals:

### 2.1 Keep hardware sovereignty in the system
Apps must not directly manipulate board-specific hardware objects.

### 2.2 Keep app runtime generic
App loading, lifecycle, registry, session management, and shell-level orchestration should not depend on board names.

### 2.3 Support multiple devices explicitly
Different devices may vary in:

- display size and topology
- touch / keyboard / joystick / button input
- LoRa / radio availability
- GPS availability
- audio input/output
- battery and power management
- storage options
- vibration / sensors / USB roles

These differences must become first-class system concepts.

### 2.4 Avoid scattered board-specific logic
Aegis must not devolve into:
- random `#ifdef BOARD_X`
- board-specific branches inside generic runtime
- app code checking board identity
- shell code tightly coupled to one device family

### 2.5 Make capabilities queryable
Device features must be represented as runtime-queryable system data, not just comments or build-time assumptions.

---

## 3. Architectural position

The device adaptation layer sits between generic system services and the actual hardware/board implementations.

```text
Aegis Shell
Aegis App Platform
Aegis Native Runtime
Aegis Service Gateway
Aegis Device Adaptation Layer
Zephyr Base
Hardware / Board
````

Its role is not to replace drivers.
Its role is to translate **board reality** into **system-understandable structure**.

---

## 4. Core concepts

The device adaptation layer is built around five core concepts:

* `DeviceProfile`
* `CapabilitySet`
* `BoardPackage`
* `ServiceBinding`
* `DeviceRegistry`

These concepts must exist as real code-level architecture objects.

---

## 5. DeviceProfile

## 5.1 What it is

A `DeviceProfile` is the system-facing description of a device.

It is not just a board name.
It is not just a devicetree node dump.
It is not just a build target.

It is the structured description of what kind of device Aegis is currently running on.

## 5.2 What it should describe

A `DeviceProfile` should express at least:

* device id
* product/family name
* display topology
* input topology
* storage topology
* power topology
* communication topology
* optional peripheral presence
* shell layout hints
* runtime constraints

## 5.3 Example fields

```cpp
struct DeviceProfile {
    const char* device_id;
    const char* family_name;

    DisplayTopology display;
    InputTopology input;
    StorageTopology storage;
    PowerTopology power;
    CommTopology comm;

    CapabilitySet capabilities;

    ShellHints shell_hints;
    RuntimeLimits runtime_limits;
};
```

## 5.4 Why it matters

The shell may adapt itself based on `DeviceProfile`.
The service gateway may bind implementations based on `DeviceProfile`.
The runtime may enforce limits based on `DeviceProfile`.

But apps should not directly branch on `DeviceProfile`.

---

## 6. CapabilitySet

`CapabilitySet` answers the question:

> What can this device provide to apps and system surfaces?

This is a formal system object, not documentation prose.

Capabilities must be queryable and versionable.

Examples:

* display
* keyboard_input
* pointer_input
* joystick_input
* radio_messaging
* gps_fix
* audio_output
* microphone_input
* battery_status
* removable_storage
* vibration
* usb_hostlink

Capabilities may also carry quality/state information rather than simple booleans.

For example:

* absent
* degraded
* full

This is important because device support is often partial, not binary.

---

## 7. BoardPackage

## 7.1 What it is

A `BoardPackage` is the board-specific integration unit for Aegis.

It is where board-level realities are gathered and normalized.

## 7.2 Responsibilities

A `BoardPackage` is responsible for:

* board-specific init sequence
* peripheral bringup ordering
* display wiring and rotation rules
* input matrix / touch / key source hookup
* radio backend hookup
* GPS backend hookup
* audio backend hookup
* power management sequencing
* creation of service backends
* production of the final `DeviceProfile`

## 7.3 What it must not do

It must not:

* directly implement app logic
* leak board-specific names into app-facing APIs
* override runtime lifecycle semantics
* act as a substitute for generic service interfaces

## 7.4 Why it matters

Without `BoardPackage`, board-specific complexity will leak into every layer.
With it, board-specific orchestration is explicitly contained.

---

## 8. ServiceBinding

## 8.1 What it is

A `ServiceBinding` connects a generic service interface to a concrete implementation for the current device.

Examples:

* `RadioService` → LoRa backend on Device A
* `RadioService` → absent backend on Device B
* `InputService` → joystick backend on Device A
* `InputService` → keyboard backend on Device B

## 8.2 Why it matters

This allows the rest of Aegis to depend on generic services instead of concrete board implementations.

The service contract remains stable, while the hardware binding varies by device.

## 8.3 Example

```cpp
class ServiceBindingRegistry {
public:
    void bind_radio(IRadioService* svc);
    void bind_gps(IGpsService* svc);
    void bind_input(IInputService* svc);
    void bind_display(IDisplayService* svc);

    IRadioService* radio() const;
    IGpsService* gps() const;
    IInputService* input() const;
    IDisplayService* display() const;
};
```

---

## 9. DeviceRegistry

A `DeviceRegistry` is the system’s authority for device identity and available profiles.

It should answer questions like:

* what device am I running on?
* what profile is active?
* what capabilities are exposed?
* what board package instantiated this profile?

This avoids spreading device identity knowledge across unrelated modules.

---

## 10. Input and display adaptation

Two of the most visible areas of hardware divergence are input and display.

### 10.1 Display divergence

Different devices may vary in:

* resolution
* aspect ratio
* orientation
* color depth
* touch presence
* screen count
* rendering performance

The shell may use profile hints to adapt layout.
Apps should preferably work through generic UI services and layout contracts.

### 10.2 Input divergence

Different devices may have:

* touchscreen
* full keyboard
* 5-way joystick
* a few physical buttons
* rotary encoder
* mixed input modes

This must not be hidden as a trivial detail.
Input topology is a core device characteristic.

The shell may adapt interaction style based on it.
Apps should access generic input semantics, not raw board key matrices.

---

## 11. Power and lifecycle implications

Some device differences are not merely UI differences.

Examples:

* aggressive low-power device vs always-on device
* removable storage vs internal-only storage
* battery reporting quality
* suspend/resume support
* radio coexistence constraints

The device adaptation layer must expose this to the system in structured form.

This is especially important because app runtime policy may depend on device constraints.

---

## 12. Shell adaptation vs app adaptation

Aegis must distinguish carefully between these two:

### Shell adaptation

Allowed:

* launcher layout changes for screen size
* status bar differences
* settings pages that reflect device-specific features
* interaction style changes based on input topology

### App adaptation

Restricted:

* apps may query capabilities
* apps may adapt behavior to capability presence or quality
* apps must not directly depend on board names

This rule is critical.

Apps adapt to **capability**, not to **board identity**.

---

## 13. Non-negotiable rules

### Rule 1

No app may directly branch on board name.

### Rule 2

No generic app-facing service interface may expose board-specific type names.

### Rule 3

No board-specific initialization sequence may be scattered across generic layers.

### Rule 4

Capability information must exist as a real runtime object.

### Rule 5

Device adaptation is a formal layer, not a collection of `#ifdef`s.

### Rule 6

The same Aegis Core should be able to run on multiple `DeviceProfile`s.

---

## 14. Minimal implementation target

The first useful implementation of the device adaptation layer should prove:

* at least two distinct `DeviceProfile`s exist
* both can bind generic services differently
* shell behavior can differ by profile
* apps can query capabilities
* apps do not know the board identity directly

For example:

### Profile A

* display
* joystick input
* radio messaging
* GPS
* no full keyboard

### Profile B

* display
* keyboard input
* no radio
* no GPS
* USB hostlink

If Aegis Core and the same app runtime can operate on both profiles with only `BoardPackage` and `ServiceBinding` changes, the architecture is working.

---

## 15. Summary

The device adaptation layer exists to preserve a fundamental truth:

> Aegis must be portable across devices,
> but that portability must not come from pretending hardware differences do not matter.

Instead, hardware differences must be:

* recognized,
* modeled,
* contained,
* and translated into capability-oriented system structure.

That is the role of device adaptation in Aegis.
