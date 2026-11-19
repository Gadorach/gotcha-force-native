# Borg packages

The desktop host scans `borgs/` at startup, relative to the launch directory.
Each Borg lives in one subfolder. The subfolder is just a location, not an ID:

```text
borgs/
  clockwork-crane/
    borg.manifest
    visual/
      model.glb
      animations.glb
    rules/
      borg.rules
      movement.map
```

`borg.manifest` is UTF-8, starts with `GFBORG 1`, and lists package-relative
files. `animation-map` is optional: it supplies a mapping when a compatible
animation bank does not already identify its clips by their engine roles.

```text
GFBORG 1
id=example:clockwork-crane
name=Clockwork Crane
model=visual/model.glb
animations=visual/animations.glb
rules=rules/borg.rules
animation-map=rules/movement.map
```

The lowercase `namespace:name` ID is stable across renames or moves. The
`retail:` namespace is reserved for a future original-content adapter. Two
manifests claiming the same ID are both rejected. Relative asset paths cannot
escape the package, and symlink directories or assets are not followed.

`gf_native --list-borgs` lists discovered IDs and names without starting the
renderer. Discovery currently validates the manifest and the presence of its
files, **not their contents**. The listed extensions are examples, not a claim
that the host can import those formats or use those rules today. The packages
do not grant ownership, enter any picker, or create battle actors yet.

## Connecting discovery to gameplay and saves

- Use one catalog in the collection, Borg picker, Challenge and Story. Keep
  discovery, asset/behavior validation, save ownership, and availability in a
  specific mode separate. Reuse the existing model, animation, and gameplay
  services rather than letting a manifest select a separate engine.
- Version the native save extension separately from the existing retail-shaped
  inventory records. Save the stable Borg ID and last known display name with
  owned entries; Force slots continue to refer to owned instances. Do not use
  scan order, package folders, or live catalog fingerprints as native identity.
- After reload, a missing package is not save corruption. Report it by name,
  e.g. **Clockwork Crane is missing from borgs/**, with the affected Forces.
  A failed or incomplete scan is not evidence that the package was removed.
- Offer **Disable in all affected Forces** (keep ownership and slot references,
  exclude the unit from selection and battle until available and re-enabled)
  or **Delete from this save** (require confirmation and remove the owned
  entry and all its references, not files on disk). Closing the prompt must not
  rewrite the save. Another Borg with the same display name cannot substitute
  for the missing stable ID.

The native save extension now has a tested store for package-owned instances
and their Force positions. It records the stable package ID, saved name, and
disable flag independently of installed files. It can produce a missing-Borg
notice and stage a disable, delete, or re-enable candidate. A duplicate-ID
conflict is reported separately and cannot authorize a missing-package action.
Deletion affects only the selected owned instance and its Force references.

The extension uses a new optional per-save record; unchanged accounts and saves
without it still use the existing format. The current game modes do not write
this record yet. Joining package and retail Force slots, loading package assets,
and connecting the missing-Borg prompt to the menus remain separate work.
