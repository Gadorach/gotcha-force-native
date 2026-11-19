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

### Extended catalogue fields

`GFBORG-CATALOG 2` keeps the version 1 description, tribe, default variant and
variant rows, and adds the catalogue identifiers found in the Borg reference.
For example, append these declarations to a version 2 profile:

```text
collection-number=33
combat-role=all-round
rarity-grade=C
crystals=2
alternate=unresolved
level-schedule=normal
experience=unresolved
display-jump=air:3
hp=200|unresolved
ratings=3|0|unresolved|5
moves=unresolved
traits=unresolved
forms=none
fusions=none
acquisition=unresolved
effects=unresolved
variant-overrides=unresolved
```

All fields in the example are required in version 2, but may explicitly record
missing information. `source-reference-code` (four hex digits) and
`observed-speed-note` (display text) are optional; neither feeds simulation.

| Fields | Values and boundaries |
| --- | --- |
| `collection-number` | A positive display ordinal or `unresolved`, not the stable Borg ID or a runtime model key. |
| `combat-role`, `rarity-grade` | `short-range`, `long-range`, `all-round`, `speed`, `support`; and `D`, `C`, `B`, `A`, `S`, respectively. Both accept `unresolved`. Role is not tribe; grade is not a colour variant. |
| `crystals`, `alternate` | A positive crystal count, `fusion-only`, `none` or `unresolved`; and `yes`, `no` or `unresolved` for alternate availability. Availability alone does not declare a loaded variant. |
| `level-schedule`, `experience` | `fast`, `normal`, `slow`, or `unresolved`; `earns-xp`, `no-xp` or `unresolved`. A schedule and permission to gain XP are independent. |
| `display-jump` | `air:N`, `air:unresolved`, `boost`, `none` or `unresolved`. This is the displayed classification, not a controller or a resource implementation. |
| `hp`, `ratings` | HP endpoints at levels 1 and 10; DEF, SHT, ATK, SPD readout ratings. Separate values with `\|`. Each value can be `unresolved`; neither endpoints nor ratings imply live combat stats or a level curve. |
| `moves`, `traits`, `forms`, `fusions`, `acquisition`, `effects`, `variant-overrides` | Each reserves a package-relative sidecar, `none` for intentional absence, or `unresolved`. The catalogue validator checks the path syntax only; it does not load these sidecars or admit a move, form, reward or actor. |
| `spawn` (optional) | A package-relative spawn sidecar, `none` or `unresolved`. If omitted it remains unresolved; existing version 2 profiles do not require this field. |

Version 1 remains readable. Both catalogue versions use explicit per-variant
editor icons and Force costs; version 2 does not assume all the retail colours
are supported by the package. See the [requirements map](borg-package-requirements.md)
for what the reserved move and relationship sidecars need to represent.

### Standalone moves check

Set `moves=gameplay/moves.txt` in a version 2 catalogue. `GFBORG-MOVES 1`
records declared move data without executing it:

```text
GFBORG-MOVES 1
move=bolt|base|b|projectile|Training Bolt
contexts=grounded|airborne
fallback=none
ammo=finite:5|unresolved
refill=all-at-once
activation=press
guard=has-ammo
priority=30
replenishment=magazine-refresh
interrupt=unresolved
phase=flight|shot-spawn|fire|bolt-trail|bolt-sound|non-solid|borgs|none
phase=impact|shot-contact|none|bolt-flash|unresolved|not-applicable|not-applicable|on-contact
outcome=hit-buff|shot-contact|stat-modifier|ally-hit|buff-life|power-bonus
```

A `move` starts a record and supplies its unique local ID, gameplay form ID,
button, action kind and readable name. The same button and form can have
multiple records with different conditions. Declare all nine scalar fields
and at least one phase, except for an explicit `no-action` branch. The first
version caps a file at 64 KiB, 64 moves and 8 phases and 8 outcomes per move.

- Buttons: `b`, `x`, `none` (e.g. passive contact), or `unresolved`. Action
  kind: `melee`, `projectile`, `special`, `no-action` or `unresolved`.
  These are independent. An X charge can be a projectile or a special
  action, and a B press can resolve to melee, a shot, or no action.
- Activation: `press`, `hold`, `release`, `charge`, `automatic`, or
  `unresolved`. `guard` and `interrupt` name condition references;
  `priority` records a nonnegative ordinal or `unresolved`. The validator
  does not evaluate conditions or choose a branch.
- Contexts: `any`, `unresolved`, or a `|`-separated set of `grounded`,
  `airborne`, `ground-dash`, `air-dash`, `flying`, `above-target`, `jumping`,
  `landing` or `contact`. These labels are not a compiled predicate.
- Fallback: `none`, `unresolved` or another move ID in the same form and
  button. Missing IDs and cycles refuse. An unresolved button is not assumed
  to match a known one at runtime.
- Ammo: `none`, `unlimited`, `unresolved` or `finite:level-1|level-10`.
  Each finite endpoint can be `unresolved`. Refill is independently
  `none`, `unresolved`, `all-at-once`, `gradual` or `fragmented`.
  `replenishment` names the refill timer reference, or `none`/`unresolved`.
  A refill pattern does not specify a duration.
- Phase fields: `id|trigger|animation role|effect cue|sound cue|solidity|penetration|explosion`.
  Trigger and cue slots accept a local identifier, `none` or `unresolved`.
  Solidity is `solid`, `non-solid`, `not-applicable` or `unresolved`.
  Penetration is `none`, `borgs`, `objects`, `borgs-and-objects`, `all`,
  `partial`, `not-applicable` or `unresolved`. Explosion is `none`, `present`
  (known effect, trigger unspecified), `on-contact`, `on-expiry`,
  `not-applicable` or `unresolved`.
- Outcome fields: `id|event|kind|target|duration|payload`. Outcome kinds are
  `damage`, `stat-modifier`, `behavior-modifier`, `control-modifier`,
  `transform`, `swap-positions`, `passive-contact`, `area-impact`, and
  `unresolved`. Targets are `self`, `selected-target`, `ally-hit`,
  `enemy-hit`, `contact-target`, `impact-area`, or `unresolved`. Event,
  duration and payload are local reference IDs, `none`, or `unresolved`.
  These are **gameplay outcome slots**, separate from the phase's effect
  and sound cues. A projectile can have several outcomes with different
  targets. No outcome rows means none have been declared yet; it is not proof
  that a move has no effect. Unsupported effects are not approximated.
- A `no-action` branch has no phases or outcomes and has `none` for ammo,
  refill, replenishment and fallback. This is an explicit terminal result,
  not missing work.

Run `gf_native --check-borg-moves example:clockwork-crane` to check the
sidecar without opening a window. It does not require valid editor icons.
The validator does not resolve forms, timers, guards, outcome payloads or
animation/effect/sound assets. It runs no triggers, physics, resource spending
or actor logic. `moves=none` and `moves=unresolved`
remain valid catalogue declarations without a move file.

### Standalone effects and audio check

Set `effects=effects/definitions.txt` in a version 2 catalogue. The sidecar
names visual/SFX assets and how they would be triggered and retired:

```text
GFBORG-EFFECTS 1
visual=bolt-body|mesh|local:effects/bolt.hsd
visual=flash|sprite|shared:engine:muzzle-flash
sound=fire-sound|one-shot|local:audio/fire.dsp
sound=flight-hum|loop|unresolved
cue=fire|move-start|socket:muzzle|event|none|flash|fire-sound
cue=flight|shot-spawn|projectile|projectile|none|bolt-body|flight-hum
cue=impact|shot-contact|contact|timer:impact-age|none|unresolved|none
```

- Visual rows name an ID and `sprite`, `mesh`, `beam`, `trail`, `material`,
  `decal` or `unresolved`. Sound rows name an ID and `one-shot`, `loop` or
  `unresolved`. Asset references are `local:relative/path`,
  `shared:namespace:id`, or `unresolved`.
- Cue fields are `id|event|anchor|lifetime|stop event|visual ID|sound ID`.
  Anchors include `actor`, `projectile`, `contact`, `world`, `socket:ID`
  and `unresolved`. Lifetimes are `event`, `projectile`, `owner`,
  `until-stop`, `timer:ID` or `unresolved`. `until-stop` cannot declare
  `none` for its stop event. A loop cannot be tied to event-only lifetime.
  Visual and sound IDs may explicitly be `none` or `unresolved`.
- In a move phase, `effect cue` names a cue row and `sound cue` names a
  sound row directly. Use the cue path for looping sound so its lifetime
  and stop behavior are explicit. These are presentation bindings, not
  gameplay outcome bindings.

`gf_native --check-borg-effects example:clockwork-crane` checks the
64 KiB/64-visual/64-sound/128-cue limits, local references, and that concrete
local assets exist as nonempty files. If a move file is declared, it also
checks its cue/sound cross-references. It does not decode assets, resolve
shared references, subscribe to events, trigger timers or play anything.
The gameplay projectile, damage and status effects remain separate.

### Standalone spawn-policy check

Set `spawn=gameplay/spawn.txt` in a version 2 catalogue. This example reserves
a near-top entry without inventing an altitude or velocity:

```text
GFBORG-SPAWN 1
spawn=top-entry|base|challenge|normal
anchor=arena-top-clearance
height=unresolved
horizontal-anchor=mode-selected
horizontal=none
initial-state=falling
initial-vy=unresolved
requirement=required
on-obstruction=refuse
```

- `spawn` starts a rule and gives its local ID, form, mode, and variant.
  Use `*` in any of the three selector positions to match all values.
  Overlapping scopes refuse; no precedence or fallback order is guessed.
- Vertical `anchor` is `mode-selected` (`height=none`), `floor-offset`,
  `arena-top-clearance`, `absolute-y`, and `unresolved`. Floor offsets and
  clearance **below** the arena top are nonnegative. Absolute Y is signed.
  Any unresolved custom height can be recorded as `unresolved`.
- `horizontal-anchor` is independently `mode-selected`, `stage-center`,
  `absolute-xz` or `unresolved`. `horizontal=none` is required for
  mode-selected or stage-center placement; the stage supplies the position.
  For `absolute-xz`, use signed `X|Z` coordinates or `unresolved`.
  An unresolved anchor also requires `horizontal=unresolved`. The actual
  stage center is not assumed to be world `(0, 0)`.
- Initial state is independently `grounded`, `falling`, `fixed-airborne`,
  `flying`, `hovering` or `unresolved`. Initial vertical speed (`initial-vy`)
  is signed or `unresolved`. Y is vertical in the native world; none of
  these declarations sets a movement policy or reserves a legal stage position.
- `requirement` is `required`, `preferred` or `unresolved`.
  `on-obstruction` is `refuse`, `mode-selected` or `unresolved`; a required
  placement cannot also opt into mode-selected fallback.

`gf_native --check-borg-spawn example:clockwork-crane` checks the file and
any named catalogue variants, not the form/mode bindings or stage limits.
A file may contain up to 32 rules within 8 KiB. The parser bounds numbers for
storage, not to source-game height limits. The diagnostic does not load a
stage or spawn a Borg. This example does not specify Death ICBM's actual
height. A boss can instead describe `horizontal-anchor=stage-center` with a
separate absolute Y, without assuming that any stage or large body can fit
there. See the [spawn requirements](borg-package-requirements.md#reserve-a-spawn-policy-independent-of-flight).

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
