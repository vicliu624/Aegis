# Aegis App Asset and Icon Contract

## 1. Purpose

This document defines how Aegis app packages should own, publish, and reference
their assets and icons.

It exists because asset problems are easy to dismiss as "just packaging", while
in practice they are tightly coupled to:

- app identity
- package completeness
- shell presentation
- foreground page publication
- runtime memory ownership

In Aegis, app assets are part of the app contract, not an afterthought.

---

## 2. Core rule

If an asset semantically belongs to an app, the asset should live in that app's
package and be referenced through documented package rules.

This applies to:

- app icon
- page-local icons
- file-type icons owned by the app
- app-specific images
- app-specific generated asset blobs

The shell may present those assets, but it should not become their hidden owner.

---

## 3. Package ownership model

For a standard app package, the package should be complete enough that discovery
and presentation do not depend on shell-private substitutions.

Typical package-owned files include:

- `manifest.json`
- `app.llext`
- package icon file
- `assets/` contents needed by the app
- generated asset outputs if the build produces them

If an app only works because the shell or board layer supplies private fallback
assets, that app is not truly package-complete.

---

## 4. Asset classes

### 4.1 Package icon

This is the icon used to represent the app in catalogs, launcher views, and
other package-level contexts.

It should be owned by the app package and referenced from package metadata or
well-defined package convention.

### 4.2 Page-local icons

These are icons used inside the app's own foreground pages.

Examples:

- folder icon in `Files`
- file-type symbols
- row-leading icons

These should also live in the app package unless they are truly global system
symbols.

### 4.3 Shared system symbols

Some assets may belong to the shell or system UI rather than to any one app.

Examples might include:

- battery symbols
- connectivity indicators
- shell-owned status icons
- generic warning glyphs

Only assets with genuinely shared system meaning should live in shared shell or
system locations.

---

## 5. Location rules

Preferred locations:

- `apps/<app>/icon.bin`
- `apps/<app>/assets/...`
- `apps/<app>/generated/...`

Avoid placing app-owned assets in:

- `ports/zephyr/...`
- board-specific directories
- shell-private asset trees
- unrelated global asset folders

Those locations blur ownership and make future packaging, replacement, and
diagnostics much harder.

---

## 6. Manifest and discovery relationship

The app registry should discover app identity from package data, not from shell
special cases.

That means icon discovery should follow package rules such as:

- manifest-declared icon path
- documented package icon filename convention
- runtime package root plus app-owned relative asset paths

What should not happen:

- shell code hard-coding one app's icon path
- board code deciding which icon a specific app uses
- special-case compiled fallback logic for one package's normal assets

---

## 7. Runtime reference model

When an app references an asset at runtime, the reference should be treated as
data, not as an implicit pointer into app-private structures.

Preferred forms:

- bounded path strings
- validated relative package paths
- runtime-owned copied asset reference strings
- future package-scoped asset ids if introduced

The runtime should not retain:

- raw pointers into app-owned asset tables
- references to stack-built temporary path fragments
- opaque assumptions about one app's private generated arrays

---

## 8. Relative path policy

Asset references should be unambiguous about which package root they resolve
against.

Recommended model:

- package metadata defines the package root
- asset paths used by the app are relative to that root unless explicitly
  documented otherwise
- runtime resolves the final path under package governance

This keeps the contract stable even if the storage backend or staging location
changes later.

---

## 9. Copy and lifetime rule for asset references

If the shell, renderer, or runtime service needs an asset reference after the
app call returns, the reference must be stored in runtime-owned memory.

This is especially important for:

- launcher icon references
- foreground page row icon references
- delayed decode paths
- background refresh or redraw after the initial declaration

Do not assume that a path pointer inside app-owned metadata can be retained
indefinitely just because the app is still currently loaded.

---

## 10. Built-in apps are not exempt

Prominent built-in apps such as `Files` should still own their own assets.

The shell may give them prominent placement, but should not silently become the
asset owner for:

- package icon
- page item icons
- app-specific generated image blobs

If `Files` needs a folder icon, that should usually be a `Files` package asset,
not a shell-private resource that happens to be used by `Files`.

---

## 11. Fallback policy

Fallbacks should be generic and documented.

Acceptable fallback examples:

- use a generic "missing app icon" symbol when package icon decode fails
- use a generic row icon when an item-local icon is unavailable
- log missing or invalid asset references

Unacceptable fallback examples:

- special hard-coded alternate path only for one app id
- shell-private substitution that hides a broken package contract
- board-specific asset replacement for one app's normal runtime path

Fallbacks should help debugging, not hide ownership mistakes.

---

## 12. Generated assets

Generated assets are allowed, but their ownership must still be obvious.

Rules:

- generated outputs should live under the app package
- build steps should stage them explicitly
- runtime references should resolve through documented package paths
- stale generated outputs should not silently survive across builds

If the staged package can accidentally retain an older generated asset, that is a
packaging hazard and should be documented in the build process.

---

## 13. Validation checklist

For every app asset reference, check:

1. Which package owns it?
2. Is the on-device path derivable from documented package rules?
3. If the shell needs it later, has the reference been copied safely?
4. If decode fails, is there a generic fallback?
5. Would this still work if the app became externally packaged?

If the answer to the last question is "no", the design is probably too
shell-dependent.

---

## 14. Relationship to foreground pages

Page-local asset references are part of the foreground page contract.

That means:

- the app may declare them
- the runtime validates and stores them
- the shell renders them through governed services

It does not mean:

- the shell owns the app's asset catalog
- the renderer should browse app package internals on a whim
- runtime-visible page items may retain raw app-private asset pointers

Asset references should be treated like any other retained page field: explicit,
bounded, and copied when needed.

---

## 15. Relationship to other documents

This document should be read together with:

- [Aegis App Model](./app-model.md)
- [Aegis Foreground Page Contract](./foreground-page-contract.md)
- [Aegis App Runtime Memory Model](./app-runtime-memory-model.md)
- [Aegis Shell](./shell.md)

The app model explains package identity.
This document explains the ownership and reference rules that keep package assets
from turning into shell-private implementation accidents.
