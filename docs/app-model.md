# Aegis App Model

## 1. What is an Aegis app?

An Aegis app is a **native application unit** that:

- is built independently from the resident system,
- lives in its own app directory,
- carries metadata and icon assets,
- is discovered at runtime,
- is loaded only when needed,
- runs through Aegis Host APIs,
- and is torn down and unloaded when it exits.

To the user, it is an **app**.  
To the runtime, it is a **loadable native extension**.  
To the system, it is a **guest workload**.

---

## 2. App directory layout

A typical app package may look like this:

```text
/apps/
  aprs_tool/
    manifest.json
    icon.bin
    app.llext
  node_info/
    manifest.json
    icon.bin
    app.llext
  spectrum/
    manifest.json
    icon.bin
    app.llext
```

The app binary is not loaded just because it exists.  
Presence in storage is not execution.

Execution begins only when the runtime grants the app a session.

---

## 3. Manifest role

Each app should carry a manifest that describes its identity and requirements.

Typical fields may include:

- app id
- display name
- version
- ABI version
- entry symbol
- permissions or capability requests
- icon path
- memory requirements
- description

The manifest is how the system recognizes an app as a valid member of the app world.

It is not just metadata for display.  
It is part of runtime admission.

---

## 4. Lifecycle

Aegis treats lifecycle as a first-class system concern.

Typical lifecycle phases:

1. **Discovered**  
   The app exists in storage and is recognized by the registry.

2. **Validated**  
   Metadata, ABI, and compatibility checks pass.

3. **Load Requested**  
   The user selects the app.

4. **Loaded**  
   The app binary is brought into runtime memory.

5. **Started**  
   The entrypoint initializes state and receives Host API bindings.

6. **Running**  
   The app owns an active foreground session.

7. **Stopping**  
   The runtime revokes active session ownership and begins cleanup.

8. **Torn Down**  
   UI roots, timers, subscriptions, and handles owned by the app are reclaimed.

9. **Unloaded**  
   The native binary is detached from the runtime.

10. **Returned to Shell**  
    Control returns to launcher or another system surface.

This explicit lifecycle is central to the design of Aegis.

---

## 5. Resource ownership

An app may allocate local state and request services, but the runtime must remain able to answer:

- what UI roots belong to this app?
- what timers belong to this app?
- what subscriptions belong to this app?
- what handles must be revoked when it exits?
- what cleanup must happen before unload?

Without explicit ownership tracking, unload is only a slogan.

Aegis therefore treats resource ownership as a runtime responsibility.

---

## 6. Host API boundary

Apps do not directly manipulate hardware drivers or raw global system state.

Instead, they interact with the runtime through the Host API.

Examples include:

- `ui_*`
- `timer_*`
- `storage_*`
- `radio_*`
- `gps_*`
- `settings_*`
- `notify_*`
- `log_*`

This boundary exists to preserve:

- unload safety
- lifecycle clarity
- service governance
- hardware authority in the system

The boundary should remain active during runtime, not only at startup.

Examples:

- manifest permission may allow or deny admission
- Host API may still reject specific service-domain calls such as radio, gps, audio, hostlink, notification posting, or text-input focus control if the current session lacks the declared permission

---

## 7. App session model

An app does not merely “run”.  
It is granted a **session**.

An `AppSession` represents the runtime-recognized existence of an app instance in the foreground or active context.

The runtime should be able to create, stop, destroy, and audit a session explicitly.

This is a better model for MCU devices than the vague idea that “a page was entered”.

---

## 8. Exit discipline

When an app exits, the system must be able to reclaim everything that belongs to it.

A correct exit path should include at least:

- foreground ownership revoked
- input detached
- timers canceled
- subscriptions removed
- UI subtree destroyed
- app-owned runtime handles released
- teardown called
- binary unloaded
- control returned to shell

An app that cannot be cleanly exited is not well integrated with Aegis.

---

## 9. What makes an app valid in Aegis

An app is valid in Aegis when it is:

- independently buildable,
- discoverable from storage,
- admissible by manifest and ABI policy,
- loadable by the runtime,
- bounded by Host API access,
- explicitly session-managed,
- and cleanly unloadable.

That is the difference between “some native code” and “an Aegis app”.

---

## 10. Summary

The Aegis app model exists to make one statement real:

> apps may be native, independent, and powerful,  
> but they are still guests in a system-owned world.
