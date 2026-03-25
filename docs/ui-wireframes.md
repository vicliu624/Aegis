# Aegis Shell Wireframes and Screen Contracts

## 1. Purpose

This document provides implementation-oriented wireframes and screen contracts for the first serious Aegis shell.

It is not pixel-accurate artwork.
It defines:

- screen purpose
- required content
- required actions
- state expectations
- transition expectations

---

## 2. Home

### 2.1 Purpose

- stable default landing page
- immediate entry into common system actions
- low-noise device summary

### 2.2 Required content

- top status bar
- 6 to 9 primary entries
- short readiness summary
- bottom action hints if space allows

### 2.3 Required actions

- select focused entry
- open menu
- return from child surfaces back to Home

### 2.4 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Home                           10:42     |
+--------------------------------------------------+
| [Apps ] [Files] [Radio]                           |
| [Tools] [Prefs] [Device]                          |
|                                                   |
| Ready. 8 apps. SD mounted.                        |
+--------------------------------------------------+
| Open              Menu                   Back     |
+--------------------------------------------------+
```

### 2.5 Notes

- keep the page calm
- do not turn Home into a dense diagnostics board

---

## 3. Apps

### 3.1 Purpose

- show the full app catalog
- surface app governance state before launch

### 3.2 Required content

- app grid or list depending on device profile
- app icon or symbol
- short app name
- state marker
- catalog summary

### 3.3 Required actions

- launch app
- inspect app details
- open app context menu

### 3.4 Grid wireframe

```text
+--------------------------------------------------+
| AEGIS / Apps                           10:42     |
+--------------------------------------------------+
| [Demo ] [Files] [Radio]                           |
| [Info ] [Tools] [Host ]                           |
| [More ] [Test ] [Diag ]                           |
|                                                   |
| 8 installed, 1 incompatible                       |
+--------------------------------------------------+
| Launch            Details                Back     |
+--------------------------------------------------+
```

### 3.5 List wireframe

```text
+--------------------------------------------------+
| AEGIS / Apps                           10:42     |
+--------------------------------------------------+
| > Demo Info                     Ready            |
|   Demo Hostlink                 Blocked          |
|   Demo Background               Unsupported      |
|   Files                         Ready            |
+--------------------------------------------------+
| Launch            Details                Back     |
+--------------------------------------------------+
```

---

## 4. App Details

### 4.1 Purpose

- explain app identity, compatibility, and policy state

### 4.2 Required content

- app title
- manifest summary
- compatibility state
- capability summary
- permission summary

### 4.3 Required actions

- launch
- open secondary actions
- return

### 4.4 Wireframe

```text
+--------------------------------------------------+
| AEGIS / App Details                    10:42     |
+--------------------------------------------------+
| Demo Info                                          |
| Native extension                                   |
|                                                    |
| Status              Ready                          |
| ABI                 Compatible                     |
| Required caps       display, storage               |
| Optional caps       hostlink                       |
| Permission state    hostlink denied                |
+--------------------------------------------------+
| Launch            Menu                   Back     |
+--------------------------------------------------+
```

---

## 5. Running

### 5.1 Purpose

- expose runtime session state as a system concept

### 5.2 Required content

- current foreground session
- resumable or suspended sessions if supported
- recent failure or stop reason where relevant

### 5.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Running                        10:42     |
+--------------------------------------------------+
| > Demo Info                     Foreground       |
|   Files                         Suspended        |
|   Radio Tool                    Last exit clean  |
+--------------------------------------------------+
| Resume            Details                Back     |
+--------------------------------------------------+
```

---

## 6. Services

### 6.1 Purpose

- expose system-owned service health and availability

### 6.2 Required content

- service list
- service state
- short summary

### 6.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Services                       10:42     |
+--------------------------------------------------+
| > Radio                         Ready            |
|   Storage                       Mounted          |
|   Audio                         Degraded         |
|   Location                      Ready            |
|   Notifications                 Ready            |
+--------------------------------------------------+
| Open              Details                Back    |
+--------------------------------------------------+
```

---

## 7. Service Detail

### 7.1 Purpose

- provide a structured explanation without dumping raw low-level data on the list screen

### 7.2 Required content

- service name
- status
- user-facing explanation
- optional diagnostics section

### 7.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Service Detail                 10:42     |
+--------------------------------------------------+
| Audio                                               |
|                                                    |
| Status              Degraded                        |
| Output              Ready                           |
| Input               Limited                         |
| Policy              System-managed                  |
| Note                Microphone path unavailable     |
+--------------------------------------------------+
| Test              More                     Back     |
+--------------------------------------------------+
```

---

## 8. Control Panel

### 8.1 Purpose

- entry surface for system settings groups

### 8.2 Required content

- settings group list
- brief group summary where helpful

### 8.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Control Panel                  10:42     |
+--------------------------------------------------+
| > Display                                          |
|   Input                                            |
|   Audio                                            |
|   Radio and Connectivity                           |
|   Storage                                          |
|   Runtime                                          |
+--------------------------------------------------+
| Open              Details                Back      |
+--------------------------------------------------+
```

---

## 9. Setting Group

### 9.1 Purpose

- expose a compact system-owned setting list

### 9.2 Required content

- setting label
- current value summary
- optional group notes

### 9.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Display                        10:42     |
+--------------------------------------------------+
| > Theme                         Color Executive  |
|   Brightness                    70%              |
|   Contrast                      Normal           |
|   Status bar                    Shown            |
|   Reduced motion                On               |
+--------------------------------------------------+
| Change            Details                Back     |
+--------------------------------------------------+
```

---

## 10. Device

### 10.1 Purpose

- present device identity and system-governed capability knowledge

### 10.2 Required content

- device name or family
- firmware version
- capability summary
- storage summary
- policy notes

### 10.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Device                         10:42     |
+--------------------------------------------------+
| T-LoRa Pager SX1262                                 |
|                                                    |
| Firmware            0.1.x                          |
| Storage             Mounted /lfs                   |
| Capabilities        11 available                   |
| Input               Rotary + center select         |
| Display             Color, non-touch               |
+--------------------------------------------------+
| Inspect            Diagnostics            Back     |
+--------------------------------------------------+
```

---

## 11. Capabilities

### 11.1 Purpose

- make capability governance explicit

### 11.2 Required content

- capability list
- availability level
- degraded or unavailable reason where possible

### 11.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Capabilities                   10:42     |
+--------------------------------------------------+
| > Display                       Full             |
|   Keyboard input                Full             |
|   Pointer input                 Absent           |
|   Audio output                  Full             |
|   Microphone input              Degraded         |
|   USB hostlink                  Policy-limited   |
+--------------------------------------------------+
| Inspect            Refresh                Back    |
+--------------------------------------------------+
```

---

## 12. Notification Surface

### 12.1 Purpose

- present shell-owned transient and historical notices

### 12.2 Required content

- short title or source
- one-line summary
- severity or state marker

### 12.3 Wireframe

```text
+--------------------------------------------------+
| AEGIS / Notices                        10:42     |
+--------------------------------------------------+
| > Demo Info              Host API path exercised |
|   Runtime                Returned to shell       |
|   Storage                SD mounted              |
|   Audio                  Input unavailable       |
+--------------------------------------------------+
| Open               Clear                  Back   |
+--------------------------------------------------+
```

---

## 13. Confirmation Dialog

### 13.1 Purpose

- force a small, explicit decision

### 13.2 Required content

- short title
- short explanation
- safe cancel path

### 13.3 Wireframe

```text
+--------------------------------------+
| Remove this app?                     |
|                                      |
| Demo Info will no longer appear      |
| in the launcher.                     |
|                                      |
| [Remove]                  [Cancel]   |
+--------------------------------------+
```

---

## 14. Launch Failure Dialog

### 14.1 Purpose

- explain governed failure without dumping internal jargon

### 14.2 Required content

- failure summary
- concise reason
- next action

### 14.3 Wireframe

```text
+--------------------------------------+
| App cannot start                     |
|                                      |
| Required capability is unavailable.  |
|                                      |
| [Details]                 [Back]     |
+--------------------------------------+
```

---

## 15. Transition contracts

The shell should preserve a small, predictable set of transitions.

### 15.1 Top-level transitions

- Home -> Apps
- Home -> Running
- Home -> Services
- Home -> Control Panel
- Home -> Device

### 15.2 Catalog transitions

- Apps -> App Details
- Apps -> App Launch
- App Launch -> App Foreground
- App Exit -> prior shell recovery target

### 15.3 Settings transitions

- Control Panel -> Setting Group
- Setting Group -> Setting Editor or Toggle
- Setting save -> same group with persisted focus

### 15.4 Device transitions

- Device -> Capabilities
- Device -> Diagnostics

---

## 16. Screen contract checklist

Every shell screen should define:

1. purpose
2. required content
3. focus landing target
4. primary action
5. secondary action
6. back behavior
7. degraded-state behavior

If a proposed screen cannot answer these clearly, it should not be added yet.

---

## 17. Summary

These wireframes should be treated as contracts for the first substantial shell implementation.

They intentionally keep:

- few templates
- short labels
- stable action semantics
- visible system governance

That is the core of the Aegis shell identity.
