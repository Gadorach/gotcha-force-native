# Borg packages

See the [self-contained Borg requirements map](borg-package-requirements.md)
for the current engine consumers, package/engine ownership split, and gates
before integration. The discovery manifest below is only the first layer.

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

## Catalogue and editor-icon check

Add `catalog=catalog.txt` to the manifest for this optional first slice. The
profile supplies a readable description, a namespaced tribe key (or `none`),
a default variant ID, and one or more named variant rows:

```text
GFBORG-CATALOG 1
description=A small clockwork scout.
tribe=example:machine
default-variant=normal
variant=normal|Standard|240|ui/crane.tpl
variant=bronze|Bronze|250|placeholder
```

Variant fields are `id|label|Force cost|editor icon`, with stable local IDs,
at most 16 rows, and integer costs from 0 through 32767. The editor icon
is a package-relative single-image `.tpl` (up to 128×128), decoded by the
existing texture system. `placeholder` intentionally allows a labelled generic
tile; `unresolved` records missing work and blocks the catalogue-ready view
instead of substituting art. No variant inherits another variant's icon unless
it explicitly names the same asset.

Run `gf_native --check-borg-catalog example:clockwork-crane` to validate and
list one discovered package's variant rows without opening a window. A missing
or invalid catalogue or icon does not remove its discovery entry or count as a
missing Borg in a save. This check validates neither 3D assets nor move logic,
effects, battle fusion or transformations, and does not change the menus.

## Standalone body preview

For one default variant with compatible HSD assets, add `preview=preview.txt`
to the manifest. `model` and `animations` now name the HSD model and motion
bank for that variant. The extra sidecar gives the existing pose system the
two rig inputs and an explicit raw `group:entry` binding for idle:

```text
GFBORG-PREVIEW 1
model-format=hsd-body-v1
variant=normal
rig-prefix=visual/crane.prefix
pose-profile=visual/crane.pose
idle=0:0
```

Run `gf_native --preview-borg example:clockwork-crane` to open the body
preview, or add `--headless --frames 3` for validation without a window. The
package must pass the catalogue check. The same model/skin, bone sampler and
renderer used by the animation mosaic handle the in-place clip. An absent or
incompatible clip or rig fails, including a motion-bank request that would
otherwise fall back to a default. Other variants, materials or formats are
not inferred. This does not import glTF, load rules or effects, alter the
classifier, or grant battle capability.

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
