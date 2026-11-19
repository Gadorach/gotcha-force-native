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
do not grant ownership, enter the retail or battle pickers, or create actors
at discovery time.

## Qualified normal G-Red practice package

The first playable package path reuses the existing movement-only G-Red
practice session. It requires an explicit stable package ID and the exact
`normal` variant; it does not add an owned copy or admit a package to the
duel, Challenge, or Story.

```sh
gf_native --gred-movement --practice-borg example:normal-gred --borg-variant normal --no-audio
```

Declare `model`, `animations`, `catalog`, and `rules` in `borg.manifest`.
The catalogue must declare `normal` as its base variant. No variant
overrides or separate `animation-map` are supported for this path. The
manifest's `rules` file must declare:

```text
GFBORG-PRACTICE 1
controller=normal-gred-gfmm9
variant=normal
rig-prefix=visual/g-red.prefix
pose-profile=visual/g-red.pose
locomotion=movement/locomotion.gfmp
pose-height=movement/pose-height.gfhc
movement-input=movement/movement.gfmi
movement-motion=movement/motion.gfmm
```

The manifest's body and motion bank and these six files must match the
qualified normal G-Red inputs already used by the native harness. This is
not a generic import format, a rules interpreter, or a claim of package
signature verification. File paths and reads are bounded. Missing, linked,
modified, or unsupported files refuse instead of falling back to the
loose G-Red files. Once loaded, the selected package bytes are held for the
session and reused on **Q** restart.

The qualified stage, its three static collision grids, and the stationary
Sasuke target remain separate harness inputs; the package does not supply
them. Preview art and editor icons are not required. Existing practice
options such as `--practice-start` also work with an eligible package.

### Practice an owned copy

To use the identity and variant from a particular saved copy instead of
supplying an installed package ID:

```sh
gf_native --gred-movement --practice-owned-borg ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID INSTANCE_ID --no-audio
```

The account, save, and instance IDs are explicit positive decimal values.
The selected account store must already exist. An absent or disabled
copy, an unspecified variant, or an exact variant whose package cannot
run this practice path refuses. `--practice-borg` and `--borg-variant`
cannot override the saved identity. The live package may be renamed or
moved to a different folder: its stable ID must still match.

The selected save is checked after preparing the package assets, before
starting the session, and again on **Q** restart. A changed save cannot
silently redirect the launch or restart; close and reopen to use it. No
practice progress, Force placement, or ownership changes are written.
This is not a Force or battle-mode admission path.

To launch and return without leaving the owned collection, open it with
practice enabled:

```sh
gf_native --browse-owned-borgs ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID --practice-from-picker --no-audio
```

**Q/Y** checks the highlighted copy and opens ordinary live practice if
it qualifies. The check can still refuse a disabled copy, unsupported
variant, or unavailable assets. **Esc** in practice returns to the same
copy; release and press it again to close the collection. Each view owns
its renderer resources only while active. If the selected save has changed,
reopen the collection rather than continuing with a stale selection.

## Read-only installed Borg picker

`gf_native --browse-borgs` opens the standalone installed-package picker.
Use **Up/Down** to browse packages, **Left/Right** to switch the selected
variant, and **Space/A** to inspect its exact IDs. The picker reads the
selected catalogue's variant labels and Force costs, and decodes only the
selected editor icon through the shared texture system. Placeholder,
unresolved and unavailable icons are labelled separately. A package with
no available catalogue stays visible without inventing a variant.

**E/X** opens the selected package and exact variant in the shared body
preview. **Shift/B** returns to the same selection. Missing or unsupported
preview assets leave you in the picker with an unavailable notice rather
than changing the variant. The preview releases its own device textures
between visits. The [standalone body preview](#standalone-body-preview)
also remains available by explicit package and variant IDs.

This is a read-only picker for installed definitions, not the retail box,
an owned collection, or mode admission. It does not open a save or a
battle. `--demo-borg-browser --frames 26` drives a repeatable navigation and
preview sequence for checks.

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

### Standalone gameplay-form check

Set `forms=gameplay/forms.txt` in a version 2 catalogue. A
`GFBORG-FORMS 1` file starts with `default-form=ID` and declares up to 16
gameplay forms and 64 transitions in at most 64 KiB. The default form is
separate from the catalogue's default costume variant.

Each `form=ID|label` starts a block with explicit `model`, `rig`,
`animations`, `animation-map`, `movement`, `moves`, `collision` and `effects`
fields. Each field is a package-relative file, `inherit` (package default
for that role, with no inferred form-to-form inheritance), `none` or
`unresolved`.

For example, after declaring `base` and `powered`, a temporary entry may be:

```text
transition=power-on|base|powered|temporary-entry
trigger=move-event
event=charge-complete
guard=can-power-up
interrupt=charge-cancel
timer=power-life
animation=power-on
effect=power-flash
transfer=unresolved
priority=10
```

- The transition header is `ID|from|to|kind`, where `kind` is `selected`,
  `temporary-entry`, `return` or `unresolved`. The source form must be
  declared; the destination may explicitly be `unresolved`. A transition
  cannot target its own source form.
- Triggers are `selection`, `move-event`, `expiry`, `stagger`, `damage`,
  `contact` and `unresolved`. Event, guard, interrupt, timer, animation,
  effect and transfer are local IDs, `none` or `unresolved`. Priority
  is a nonnegative ordinal or `unresolved`; no selection is performed.
- A selected transition cannot declare a named expiry timer. A known
  temporary destination requires a separate `return` path; if the entry
  names a duration timer it also requires an expiry return referencing that
  timer or explicitly leaving the timer unresolved. A stagger or other
  interrupt return is distinct. A return can reserve its destination as
  unresolved without inventing a model swap.

Run `gf_native --check-borg-forms example:clockwork-crane` to check the
declarations, nonempty local override files and form IDs in any declared
move/spawn profiles. It does not parse inherited or overridden assets,
resolve transition payloads, start timers, replace a model or transfer HP,
movement, ammo or other state. Battle fusion remains a separate facet.

### Standalone battle-fusion check

Set `fusions=gameplay/fusions.txt` in a version 2 catalogue. This is battle
combination, **not** collecting or assembling crystal parts.

```text
GFBORG-FUSIONS 1
fusion=example:twin-join
first=example:left|base|*
second=example:right|base|normal
result=example:joined|combined|normal
result-use=fusion-only
trigger=hyper-request
guard=both-ready-same-team
synchronize=join-window
commit=combine-complete
interrupt=unresolved
separation=unresolved
duration=unresolved
entry=combine-left|combine-right|join-idle
exit=left-return|right-return|unresolved
transfer=unresolved|unresolved
```

- `fusion` starts a record with a namespaced relationship ID. Declare
  `first`, `second` and `result` as `package ID|form ID|variant ID`.
  Participant forms/variants can be `*` or `unresolved`; result selectors
  can be concrete or `unresolved`, not `*`. A wholly unresolved package
  needs unresolved form and variant as well. This version requires a
  separate package for a known result. Two roles can reference the same
  Borg type; a future runtime must still require distinct actor instances.
- `result-use` records `fusion-only`, `independent` or `unresolved`. It
  describes intent and does not unlock the result or change any picker.
- `trigger`, `guard`, `synchronize`, `commit`, `interrupt`, `separation`
  and `duration` are local reference IDs, `none` or `unresolved`.
  `entry` and `exit` give three cue references in first/second/result order.
  `transfer` gives the combine and separate policy references. Each slot
  can remain unresolved without inferring a runtime rule.

`gf_native --check-borg-fusions example:left` accepts up to 32 records
within 64 KiB. It checks that the declaring package is a participant or
result, that named packages resolve uniquely, and that referenced forms
and catalogue variants exist. It reports unresolved parties separately.
It does not reconcile duplicate relationship declarations across folders,
bind cues, synchronize actors, transfer state, or admit the result to
any mode.

### Standalone acquisition check

Set `acquisition=gameplay/acquisition.txt` in a version 2 catalogue. This
declares how a mode may grant or lend a Borg or part; the mode still owns
its missions, drop odds, progression checks and inventory changes.

```text
GFBORG-ACQUISITION 1
route=founder|story|normal|start-grant
source=story:new-run
condition=campaign-start
grant=owned
component=none
repeat=once-per-save
trade=blocked
discard=blocked
retirement=unresolved
```

- The `route` header is `ID|mode|variant|method`. Mode and variant can
  be local IDs, `*`, or `unresolved`. Methods are `start-grant`,
  `encounter-drop`, `progress-reward`, `crystal-assembly`, `battle-fusion`,
  `scripted-loan` and `unresolved`. All eight route fields are required.
- `source`, `condition` and `retirement` are local or namespaced reference
  IDs, `none` or `unresolved`. `repeat` is `once-per-save`,
  `once-per-play`, `repeatable` or `unresolved`.
- A `grant` is `owned`, `crystal-part`, `battle-only`, `loan` or
  `unresolved`. An explicit part grant names a local `component` or
  leaves it unresolved; it is not an owned Borg grant. A battle fusion
  cannot itself grant ownership, and a scripted loan cannot silently
  become permanent. `trade` and `discard` are `allowed`, `blocked` or
  `unresolved`; nonowned battle results and loans cannot opt into either.
- A `recipe=ID|variant|part-a|part-b` row names at least two distinct
  local parts for one variant. More parts use additional `|` fields, up
  to 16. A `crystal-assembly` route may leave its `source` unresolved
  or link a local recipe of its own variant; it cannot use `*` for that
  variant. This is separate from combat fusion.

`gf_native --check-borg-acquisition example:clockwork-crane` checks up to
64 routes and 32 recipes in 64 KiB, including known catalogue variant IDs.
It does not resolve mission/rule references, award a Borg or crystals,
retire ownership, or change any save.

### Variant overrides

Set `variant-overrides=gameplay/variants.txt` in a version 2 catalogue. Each
override starts with `variant=ID`, matching a declared catalogue variant:

```text
GFBORG-VARIANTS 1
variant=bronze
model=inherit
animations=inherit
preview=visual/bronze.preview
textures=none
materials=none
stats=unresolved
movement=inherit
moves=inherit
collision=inherit
effects=inherit
acquisition=unresolved
hp-level-offset=unresolved
ammo-level-offset=unresolved
```

Each asset field is a package-relative file, `inherit` (package base for
that role, not another variant), `none` (no separate override), or
`unresolved`. The two displayed-level offsets are independent signed values
from −255 to 255, `inherit` or `unresolved`. A literal `0` is explicit;
these bounds are for storage, not a retail rule. The format does not
infer colour palettes, stat effects, or a level curve from a variant's name.

Run `gf_native --check-borg-variants example:clockwork-crane` to check up to
16 override rows in 16 KiB, including catalogue IDs and nonempty local
files. That check does not decode the referenced assets or apply the stat
declarations.

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
package must pass the catalogue check. Select a declared nondefault variant
with `--borg-variant ID`; it must name its own preview profile in the
overrides file. The profile's `variant` must match the request. Model and
motions can explicitly inherit the base body, but an incompatible profile,
missing clip, or unsupported separate/unresolved texture or material override
fails instead of displaying the default look. This lets a stat-only variant
explicitly share visuals without claiming a recolour.

The same model/skin, bone sampler and
renderer used by the animation mosaic handle the in-place clip. An absent or
incompatible clip or rig fails, including a motion-bank request that would
otherwise fall back to a default. Materials and formats are not inferred.
This does not import glTF, load rules or effects, alter the
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
disable flag independently of installed files. If explicitly selected, it
also saves the variant ID and last known variant label. Older native roster
records leave that choice unspecified; loading them never guesses the current
default. It can produce a missing-Borg
notice and stage a disable, delete, or re-enable candidate. A duplicate-ID
conflict is reported separately and cannot authorize a missing-package action.
Deletion affects only the selected owned instance and its Force references.

An installed Borg with a missing saved variant is reported separately. A
metadata-only catalogue check can recognize the exact variant even when
its editor icon is unresolved. An unavailable or stale catalogue cannot
prove removal. Disabling preserves that variant and the Force positions,
and re-enabling a named variant requires the same ID to be present again.
Neither changing a label nor changing the default substitutes another
variant. This is still save and recovery data, not mode or actor admission.

The extension uses a new optional per-save record; unchanged accounts and saves
without it still use the existing format. The current game modes do not write
this record yet. Joining package and retail Force slots, loading package assets,
and connecting the missing-Borg prompt to the menus remain separate work.

The native recovery session now owns the confirmation flow for an
explicitly selected save. Opening and cancelling only inspect the roster;
disable and delete require their own confirmation state. On confirmation it
rechecks the notice against current catalogue data and the save against the
loaded record. It publishes a new view only after the storage write is
acknowledged. An unavailable scan, stale notice or unconfirmed write cannot
be replayed as a destructive action.

### Standalone save recovery

On the Linux desktop host, open an existing explicit account-store root
and decimal account and save IDs:

```sh
gf_native --recover-borgs ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID
```

The recovery menu displays the saved Borg and variant names and affected
Force numbers. **Up/Down** moves the cursor, **Space/A** accepts, and
**Shift/B** cancels or closes. The default is **Cancel** on both the action
page and the confirmation page. Disable preserves ownership and slots;
delete removes only the selected owned instance and its slots from that
save. Neither operation touches package files.

To inspect a particular notice directly, add `--recovery-notice INSTANCE`.
`--headless` is inspection-only and cannot write, even if confirmation input
is supplied. A failed scan never authorizes a write. The menu does not create
accounts or saves, migrate legacy files, or join the retail and package
inventories. Retail menu integration remains separate.

### Read-only owned collection

```sh
gf_native --browse-owned-borgs ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID
```

The installed picker's shared display and body preview can also browse the
packaged copies in one explicitly selected save. **Up/Down** selects a copy;
`--owned-instance INSTANCE` opens directly on that saved instance. The
collection shows the cached names, the saved variant ID and Force placement
count even for disabled or missing copies. It never changes a variant or
silently falls back to the installed default. **E/X** requests a body
preview only for an enabled copy whose exact variant resolves; the preview
can still refuse missing or unsupported assets. **Shift/B** returns to the
same copy. This is a snapshot of packaged ownership only, not a unified
retail collection, editor, or battle picker.

### Read-only Force preflight

```sh
gf_native --inspect-borg-force ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID FORCE_NUMBER
```

The Force number is one-based. This prints only packaged positions from the
explicitly selected save, retaining each owned instance ID, saved name and
saved variant. It distinguishes missing/incomplete/conflicting packages,
unavailable catalogue metadata, unspecified legacy variants, missing
variants and saved disable state. It never replaces a missing or unspecified
variant with the default.

`selectable-metadata` means the exact saved ID and variant resolved and the
instance is not disabled. It does not imply art, physics, or battle readiness.
The declared package cost is available only if every packaged slot is
metadata-ready and enabled; it is not a total for retail slots or a mode
budget. The command doesn't open devices or modify the save.

To include an existing retail roster, add `--retail-catalog FILE` with
the exact GFBC catalogue used by that save:

```sh
gf_native --inspect-borg-force ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID FORCE_NUMBER \
  --retail-catalog FILE
```

This read-only join shows retail and packaged entries in the same slot
order. An overlap prints both claims and withholds the combined cost; it
does not move or replace either entry. It retains the retail model,
variant index, original box slot and edit provenance. It does not decode
retail names that are not printable UTF-8. A mismatched or unreadable
retail record fails rather than appearing as an empty Force. A known
absent retail record is reported separately. Even a complete declared
total is not a mode budget or admission receipt.

### Place an owned Borg in an empty Force slot

```sh
gf_native --place-owned-borg ACCOUNT_STORE_ROOT ACCOUNT_ID SAVE_ID FORCE_NUMBER SLOT_NUMBER \
  [--owned-instance INSTANCE] [--retail-catalog FILE]
```

Force and slot numbers are one-based. **Up/Down** selects an owned copy and
**Space/A** stages it. Confirmation defaults to **Cancel**: press **Down**
to highlight **Confirm Placement**, then press **Space/A** separately.
**Shift/B** or closing the window cancels without writing. Supply the exact
GFBC catalogue with `--retail-catalog` if a retail inventory record exists.
`--headless` cannot commit a placement.

This operation places one already-owned, enabled copy with its exact saved
variant. It refuses an unverified retail record, either kind of occupancy,
and repeated membership in the same Force. Confirmation rescans the
package and rechecks both records on the selected account/save. A saved
result changes only the package position, not ownership or the retail
record. After a saved, stale, or unconfirmed result, reopen to inspect the
save; the UI does not retry a write. This is not a full mixed Force editor
and does not check a mode budget or admit the Force to battle.
