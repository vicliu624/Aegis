# Aegis Visual Specification

## 1. Purpose

This document defines the visual token system for the Aegis shell.

It exists to make the shell visually implementable rather than only conceptually described.

This document specifies:

- palette baselines
- semantic color tokens
- component color mappings
- typography roles
- spacing and border tokens
- monochrome fallback rules

It complements:

- [Aegis System UI Design](./ui-system.md)
- [Aegis UI Patterns and Interaction Spec](./ui-patterns.md)
- [Aegis Shell Wireframes and Screen Contracts](./ui-wireframes.md)

---

## 2. Visual intent

The Aegis shell should feel:

- quiet
- disciplined
- tactile
- system-owned
- low-noise
- readable on constrained displays

The default visual baseline is:

> warm light utility surfaces, restrained executive accents, strong borders, and compact readable text

This is not a neon UI and not a glossy smartphone UI.

---

## 3. Theme families

Aegis defines two first-class theme families:

- `Color Executive`
- `Monochrome Utility`

`Color Executive` is the default color-capable shell baseline.
`Monochrome Utility` is the required fallback for monochrome or low-render-budget devices.

Both themes must preserve:

- page structure
- focus behavior
- information hierarchy
- state semantics

---

## 4. Color Executive base palette

This is the default shell palette for color displays.

### 4.1 Neutral surfaces

| Token | Hex | Usage |
|---|---|---|
| `bg_root` | `#ECE6D8` | app-wide background, shell root |
| `bg_panel` | `#F6F1E6` | panels, list bodies, dialogs |
| `bg_panel_alt` | `#E4DDCF` | alternating rows, secondary blocks |
| `bg_status_bar` | `#D8CFBE` | top status bar |
| `bg_softkey_bar` | `#D4CAB8` | bottom softkey or action hint row |
| `bg_field` | `#FBF7EF` | input-like or inspector value cells |

### 4.2 Borders and structure

| Token | Hex | Usage |
|---|---|---|
| `line_strong` | `#5A554D` | outer borders, focus box outlines, key separators |
| `line_default` | `#7B746A` | normal dividers, section separators |
| `line_muted` | `#A79E90` | subtle separators, disabled boundaries |

### 4.3 Text colors

| Token | Hex | Usage |
|---|---|---|
| `text_primary` | `#26231F` | main labels, titles, values |
| `text_secondary` | `#4A453D` | secondary copy, metadata |
| `text_muted` | `#6B655C` | low-priority notes, hints |
| `text_inverse` | `#F8F4EC` | text on dark focus or active backgrounds |
| `text_disabled` | `#8A8378` | disabled labels |

### 4.4 Accent colors

| Token | Hex | Usage |
|---|---|---|
| `accent_focus` | `#2F5B6A` | primary focus fill or highlight band |
| `accent_focus_alt` | `#466F7B` | secondary focus or hover-equivalent |
| `accent_active` | `#405E45` | active/running state |
| `accent_info` | `#5E6C8D` | informational badge or detail emphasis |
| `accent_selection` | `#7A6A44` | selection marker, pinned or chosen item |

### 4.5 Semantic state colors

| Token | Hex | Usage |
|---|---|---|
| `state_ready` | `#4E6B50` | ready, mounted, healthy |
| `state_busy` | `#8A6A2B` | busy, pending, working |
| `state_warning` | `#A45A2A` | degraded, partial issue |
| `state_error` | `#8C3B32` | blocked, failed, critical |
| `state_unavailable` | `#70685D` | unavailable, unsupported |
| `state_policy` | `#6C557A` | policy-limited, permission-limited |

### 4.6 Notification severity accents

| Token | Hex | Usage |
|---|---|---|
| `notice_info` | `#5E6C8D` | informational notices |
| `notice_success` | `#54714E` | success notices |
| `notice_warn` | `#A7672E` | warnings |
| `notice_error` | `#934238` | errors |

---

## 5. Monochrome Utility palette

This palette is the baseline for monochrome-capable devices or low budget render paths.

### 5.1 Primary monochrome values

| Token | Hex | Usage |
|---|---|---|
| `mono_bg_root` | `#F2F0EA` | root background |
| `mono_bg_panel` | `#FCFBF8` | panels |
| `mono_bg_alt` | `#E3DED4` | alternate blocks |
| `mono_line_strong` | `#2E2A26` | major borders |
| `mono_line_default` | `#5A544D` | normal borders |
| `mono_text_primary` | `#1F1B18` | primary text |
| `mono_text_secondary` | `#4A443E` | secondary text |
| `mono_focus_fill` | `#2C2925` | focused row fill |
| `mono_text_inverse` | `#F7F4EE` | text on focused row |

### 5.2 Monochrome state strategy

Monochrome mode must not try to simulate color states through weak grays alone.

State differentiation should rely on:

- text label
- icon or prefix marker
- border weight
- fill inversion
- optional pattern or glyph

Recommended glyph pairings:

- ready: `OK`
- busy: `...`
- warning: `!`
- blocked: `X`
- policy-limited: `P`

---

## 6. Semantic token set

These are the shell-level semantic tokens.
Implementation code should map components to these tokens rather than directly hardcoding palette values everywhere.

### 6.1 Surface tokens

| Semantic token | Color Executive | Monochrome Utility |
|---|---|---|
| `surface.root` | `#ECE6D8` | `#F2F0EA` |
| `surface.panel` | `#F6F1E6` | `#FCFBF8` |
| `surface.panel_alt` | `#E4DDCF` | `#E3DED4` |
| `surface.status_bar` | `#D8CFBE` | `#E3DED4` |
| `surface.softkey_bar` | `#D4CAB8` | `#E3DED4` |
| `surface.field` | `#FBF7EF` | `#FCFBF8` |

### 6.2 Text tokens

| Semantic token | Color Executive | Monochrome Utility |
|---|---|---|
| `text.primary` | `#26231F` | `#1F1B18` |
| `text.secondary` | `#4A453D` | `#4A443E` |
| `text.muted` | `#6B655C` | `#5A544D` |
| `text.inverse` | `#F8F4EC` | `#F7F4EE` |
| `text.disabled` | `#8A8378` | `#8B857B` |

### 6.3 Border tokens

| Semantic token | Color Executive | Monochrome Utility |
|---|---|---|
| `border.strong` | `#5A554D` | `#2E2A26` |
| `border.default` | `#7B746A` | `#5A544D` |
| `border.muted` | `#A79E90` | `#A29A8D` |

### 6.4 Focus and interaction tokens

| Semantic token | Color Executive | Monochrome Utility |
|---|---|---|
| `focus.fill` | `#2F5B6A` | `#2C2925` |
| `focus.text` | `#F8F4EC` | `#F7F4EE` |
| `focus.border` | `#21424D` | `#1F1B18` |
| `selection.fill` | `#7A6A44` | `#5A544D` |
| `selection.text` | `#F8F4EC` | `#F7F4EE` |

### 6.5 State tokens

| Semantic token | Color Executive | Monochrome Utility fallback |
|---|---|---|
| `state.ready` | `#4E6B50` | `border.default + label "Ready"` |
| `state.active` | `#405E45` | `focus-style emphasis + label "Active"` |
| `state.busy` | `#8A6A2B` | `label "Busy" + subtle marker` |
| `state.warning` | `#A45A2A` | `label "Warning" + ! marker` |
| `state.error` | `#8C3B32` | `label "Blocked" or "Failed" + X marker` |
| `state.unavailable` | `#70685D` | `label "Unavailable"` |
| `state.policy` | `#6C557A` | `label "Policy-limited" + P marker` |

---

## 7. Component color mapping

### 7.1 Shell root

- background: `surface.root`
- text: `text.primary`

### 7.2 Status bar

- background: `surface.status_bar`
- title text: `text.primary`
- supporting icons: `text.secondary`
- warning icon: `state.warning`

### 7.3 Softkey bar

- background: `surface.softkey_bar`
- labels: `text.primary`
- inactive hint text: `text.secondary`

### 7.4 Panels and lists

- panel background: `surface.panel`
- row separator: `border.default`
- section separator: `border.strong`
- alternate block: `surface.panel_alt`

### 7.5 Focused list row

- background: `focus.fill`
- text: `focus.text`
- leading marker or border: `focus.border`

### 7.6 Buttons

#### Primary button

- fill: `accent_focus`
- text: `text.inverse`
- border: `focus.border`

#### Secondary button

- fill: `surface.panel_alt`
- text: `text.primary`
- border: `border.default`

#### Destructive button

- fill: `state_error`
- text: `text.inverse`
- border: `#6A2B24`

#### Disabled button

- fill: `#D8D0C2`
- text: `text.disabled`
- border: `border.muted`

### 7.7 Dialog

- background: `surface.panel`
- outer border: `border.strong`
- body text: `text.primary`
- supporting text: `text.secondary`

### 7.8 Input-like field or inspector value field

- background: `surface.field`
- border: `border.default`
- text: `text.primary`

### 7.9 Notices

- info left marker: `notice_info`
- success left marker: `notice_success`
- warning left marker: `notice_warn`
- error left marker: `notice_error`

---

## 8. Status expression rules

Semantic states should not rely on color alone.

### 8.1 Required representation order

Every important status should be readable in this order:

1. explicit text label
2. optional icon or marker
3. color if available

### 8.2 Forbidden representation

Do not represent critical state only by:

- a color dot
- a weak tinted background
- an icon without text

---

## 9. Typography specification

Typography should feel compact, narrow enough to fit constrained displays, and highly readable at small sizes.

### 9.1 Typography principles

- readability first
- few sizes
- short labels
- avoid decorative weights
- avoid wide, playful forms

### 9.2 Role scale

| Role | Relative size | Weight | Usage |
|---|---|---|---|
| `type.title` | 1.25x body | bold | page title, dialog title |
| `type.section` | 1.1x body | bold | section headers |
| `type.body` | 1.0x | regular | normal row labels and values |
| `type.status` | 0.95x | medium | status labels, metadata |
| `type.hint` | 0.9x | regular | softkey hints, secondary help |

### 9.3 Font direction

The exact font may vary by render stack, but the shell should prefer:

- compact sans-serif
- low ambiguity at small sizes
- good digit readability
- narrow enough for status bars and compact lists

If a custom font is later selected, it must be documented as a shell asset rather than chosen ad hoc per board.

---

## 10. Spacing and geometry tokens

### 10.1 Spacing scale

| Token | Value | Usage |
|---|---|---|
| `space.1` | `2 px` | tight icon/text gaps |
| `space.2` | `4 px` | compact internal padding |
| `space.3` | `6 px` | normal row padding |
| `space.4` | `8 px` | section spacing |
| `space.5` | `12 px` | large group separation |

### 10.2 Border thickness

| Token | Value | Usage |
|---|---|---|
| `border.thin` | `1 px` | normal separators |
| `border.strong` | `2 px` | outer panels, focused box, dialogs |

### 10.3 Corner radius

| Token | Value | Usage |
|---|---|---|
| `radius.none` | `0 px` | monochrome or ultra-compact mode |
| `radius.sm` | `2 px` | most panels and buttons |
| `radius.md` | `4 px` | dialogs on richer color devices |

Do not exceed `4 px` in the shell baseline.

---

## 11. Focus rendering specification

Focus is one of the most important visual states in Aegis.

### 11.1 Default focused row

- fill: `focus.fill`
- text: `focus.text`
- border or left band: `focus.border`

### 11.2 Alternative focus for very low-color or monochrome environments

- dark fill
- inverse text
- 2 px outer border if needed

### 11.3 Focus must remain stable

Do not animate focus continuously.

Allowed:

- instant switch
- very short settle transition if hardware allows

Avoid:

- pulsing
- breathing glow
- color cycling

---

## 12. Button styling specification

### 12.1 Primary button

- background: `accent_focus`
- text: `text.inverse`
- border: `#21424D`
- radius: `2 px`
- padding: `4 px` vertical, `6 px` horizontal

### 12.2 Secondary button

- background: `bg_panel_alt`
- text: `text.primary`
- border: `border.default`
- radius: `2 px`

### 12.3 Passive text-button style

For very small layouts, visible text-button bars may replace filled buttons:

- background: transparent
- text: `text.primary`
- focused state: `focus.fill` plus `focus.text`

---

## 13. Icon styling specification

### 13.1 Size guidance

Recommended shell icon sizes:

- `16 px` for compact rows and status indicators
- `24 px` for launcher items

### 13.2 Visual style

- simple strokes
- little or no inner detail
- consistent stroke weight
- limited fill use

### 13.3 Color use

Default icon color:

- normal: `text.primary`
- muted: `text.secondary`
- focused: `text.inverse`

State icons may use semantic state colors, but only when text remains present.

---

## 14. Page-level style mapping

### 14.1 Home

- background: `surface.root`
- grid items: `surface.panel`
- summary line: `text.secondary`
- focused tile: `focus.fill`

### 14.2 Apps

- catalog background: `surface.root`
- tile or row base: `surface.panel`
- state marker: relevant `state.*` token
- count summary: `text.secondary`

### 14.3 Running

- current foreground row should use `state.active` accent in addition to focus when selected

### 14.4 Services

- status text should use `state.ready`, `state.warning`, `state.error`, or `state.unavailable`
- list row background remains quiet

### 14.5 Control Panel

- prefer neutral surfaces
- do not over-accent settings lists

### 14.6 Device and capability inspection

- values: `text.primary`
- section labels: `text.secondary`
- unavailable or policy-limited items: `state.unavailable` or `state.policy`

---

## 15. Contrast guidance

The shell should maintain strong contrast in these pairings:

- `text.primary` on `surface.panel`
- `text.primary` on `surface.root`
- `focus.text` on `focus.fill`
- `text.inverse` on `state_error`

If a later palette revision reduces contrast in any of these pairs, it should be rejected.

---

## 16. Animation guidance

Animation should be restrained.

Allowed:

- page replace
- dialog appear
- focus move if extremely short

Recommended timing:

- focus change: `0 ms` to `60 ms`
- dialog appear: `80 ms` to `120 ms`
- page transition: `100 ms` to `160 ms`

Avoid:

- spring motion
- bounce
- blur transitions
- long fades

---

## 17. Example LVGL-style token export

The exact implementation may differ, but the shell should eventually expose tokens in a form similar to:

```c
#define AEGIS_COLOR_BG_ROOT        0xECE6D8
#define AEGIS_COLOR_BG_PANEL       0xF6F1E6
#define AEGIS_COLOR_TEXT_PRIMARY   0x26231F
#define AEGIS_COLOR_TEXT_SECONDARY 0x4A453D
#define AEGIS_COLOR_FOCUS_FILL     0x2F5B6A
#define AEGIS_COLOR_FOCUS_TEXT     0xF8F4EC
#define AEGIS_COLOR_READY          0x4E6B50
#define AEGIS_COLOR_WARNING        0xA45A2A
#define AEGIS_COLOR_ERROR          0x8C3B32
```

This is an implementation pattern example, not a required file format.

---

## 18. Non-negotiable visual rules

1. No large-radius mobile-card look.
2. No glow or neon focus.
3. No dependence on color alone for state.
4. No decorative gradients as the shell identity.
5. No high-saturation app-store icon style.
6. Borders and typography carry structure more than shadow does.

---

## 19. Summary

The Aegis shell visual system should be concrete enough that two implementers can build the same UI language without inventing different palettes or component treatments.

That is the purpose of this document.
