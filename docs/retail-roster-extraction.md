# Deterministic GG4E08 roster extraction

The roster-wide extractor creates a **separate, nonplayable** asset package for
each of the 206 normal collection definitions. It does not import original
engine code, use a per-Borg whitelist, overwrite an existing install, or mint
actors. The catalogue identity and the retail battle-member table must agree
with the named AFS member. Output depends only on the selected local inputs.

From the repo root, with a fresh destination under ignored local storage:

```sh
mkdir -p local/my-retail-roster
python3 -B -m tools.extract.retail_borg_roster \
  /path/to/catalog.gfbc /path/to/main.dol /path/to/afs_data.afs \
  local/my-retail-roster/borgs
```

`catalog.gfbc` is the exact local GG4E08 Force Editor export. `main.dol` is
read only as data; `afs_data.afs` is read in bounded ranges. The package
manifest is written last in each folder. `roster-extraction.json` is written
only after all entries finish. If extraction stops, preserve the partial
directory for diagnosis and use a different fresh destination. Don't merge
this baseline over existing curated packages; the two roots use the same
stable IDs.

| Facet | Extracted now | Not assumed |
| --- | --- | --- |
| Model and rig | 206 exact normal HSD model payloads and wrapper prefixes, including source materials, textures and UV data; 4,663 logical joints indexed across all 206 | Missing renderer support is not substituted with a nearby model; a logical joint is not evidence of a projectile muzzle |
| Appearance candidates | All 1,030 non-normal wrappers preserved by exact slot; optional package declarations expose each exact model and rig | No color name, collectible eligibility, in-battle form, or variant-specific movement inferred from the slot |
| Animation | 183 local banks, 23 actual empty banks, 8,191 indexed local slots and 55 common slots; local AObj end frames and loop flags; aliases preserved and null slots do not resolve to 0:0 | An authored loop flag does not name a movement role. The raw `0:0` preview start is not a gameplay idle classification |
| Model parts | 862 authored part rows across 138 Borgs, with 235 default record-to-joint candidates for the supported selector groups; two exact-bank neutral assemblies from a separate child route | No live selector, detached-part state, or battle style qualification |
| Sound | Four shared TSB/CHD/DPK banks, guarded landing tone selection, and static projectile sound operands | A table entry does not prove the actor or child takes that branch; other cues remain unresolved |
| Stats | Stable ID, collection order, tribe, normal Force cost, HP at levels 1–10, four display ratings; raw numeric level and display rows | Runtime meanings are not guessed for the other raw level columns |
| Movement cohorts | 298 literal movement-configuration rows across all 206 Borgs, with source type, flags and raw policy | A config store is not a controller trace, a form selection, or a fly/hover/ground-contact label |
| Movement clip candidates | 19 named shared handler roles across 205 Borgs; 4,793 distinct Borg/role/call-site links to indexed clips across 183 | A present handler or clip is not proof of input admission, reachability, pose lane ownership, or live timing |
| Move display | 556 authored B/X rows with slot, icon and display indicator; 551 named with the optional English text export | No combat kind, activation, weapon socket, ammo link or timing inferred from the display |
| Action tables | 319 static installer table combinations across all 206, including program roots; grouped action rows retain contexts, handler pairs and explicit refusals | Conditional paths are not evidence of the user action, selected form, invoked callback, or release outcome |
| Handler-to-asset links | Class handler candidates for 203 Borgs; 367 distinct Borg/call-site program links and 98 projectile-origin links | Runtime-dependent arguments, other indirect dispatches and observed execution remain unresolved |
| Projectile origins | 273 static local points in five shared factory tables; model-scoped release links are indexed separately | A launch point is not aim, proof of a live pose, or a move-to-projectile mapping for the rest of the roster |

Each `source-index.json` is an offline inventory, not a gameplay declaration.
The optional `--preview-extras` exposes up to seven extra raw slots in the
preview. By default only the raw starting clip is declared, so an unsupported
extra clip does not block inspecting an otherwise supported body. Every raw
slot stays in the index either way.

### Preview the alternate source looks

Add `--source-variants` to a fresh extraction. Each successful wrapper split
uses the existing `GFBORG-VARIANTS 1` overrides, with the exact model and rig
from source slot `4+N` as `source-N` (`N=1..5`). The same PZZ member's
motion bank is inherited only where present; no model can inherit normal as
a fallback. The original slot's force cost is retained. The IDs mean
**numeric wrapper inspection**, not a final Color Collection naming scheme.
Acquisition, gameplay, level offsets and other behavior are unresolved.
This opt-in extraction is separate from the normal-only baseline.

In the extracted parent directory, for example:

```sh
/path/to/gf_native --preview-borg gg4e08:0615 --borg-variant source-3
```

Two alternate-enabled extractions compared byte-for-byte. All 206 override
profiles passed native validation. The three-frame sweep now requires body
geometry submissions, not just successful loading and UI draws. It passed
**992 of 1,030 alternate looks** and **198 of 206 normal looks**. All six
exact looks each for `0202` and `020b` pass. `0107` is now a normal-only
pilot: its five exact wrappers remain as `source-wrapper-*.bin`, but the
playable catalogue declares only `normal`, so those five leave the
declared alternate set. The four Zeta models are now normal-only pilots
too, so their twenty exact wrappers likewise remain as
`source-wrapper-*.bin` without catalogue variant declarations. The three
X-charge models follow the same policy, removing another fifteen, and the
later `010a`, `0f04`, `0004`, `0e00`-`0e05` and `0f03` pilots remove
another fifty-five. The current preview run is one hundred and twenty
requests lower for that reason: 2,578 requested and 2,451 passed. The
remaining looks refuse strict reflection generation. An
undeclared variant request refused.

### Use the coarse movement and move map

The movement scan follows the normal family/member installer index, then
collects explicit stores of literal config pointers to the actor's movement
configuration field. It checks the local straight-line definition and rejects
interior branch entries. It does not execute original code, resolve the
store's branch conditions, or match animation slots to a controller.
Of the 206, 204 have a single candidate type (not necessarily a single
configuration); `0d06` and `0d07` both retain types 3 and 10. The scope is
static source cohorts, not final movement-family labels.

`motion_role_candidates` joins the first 32 selector pairs to the shared
lane tables. A role name requires an exact known **lower handler address**,
not an assumed selector number or a movement-type bucket. For instance,
the ordinary jump ascent and powered ascent have distinct handlers, and
cruise/boost are not grouped as ordinary grounded dash. Other present
lower selectors remain unclassified. Links from those handlers keep the
exact upper/lower request site, program key, local/common clip and missing
slot state. The 4,793 link count deduplicates by Borg, role, and call site
across candidate profiles. Some gliding Borgs also contain ordinary dash
handlers; the mapping does not infer that their input gates admit both.
There are **no** generated movement rules or air-resource mechanics here.

Ground travel has a separate carried-program link: the exact paired turn
entry handlers request program `0:1`, and the lower turn completion may
hand off to selector 2. The exact travel handlers advance the already
bound program rather than request a new clip. Both selector pairs and
all four handler ranges are checked before a carried link is emitted.
That adds `ground-travel` previews for 181 Borgs without inferring that
all Borgs walk or that all bank slot `0:1` clips are locomotion.

#### Inspect authored movement operands

Add `--movement-operands` to a fresh extraction. It indexes **48 fields
in all 230 actor-data records** across the **206** normal packages. Each
record preserves its ordinal, SHA-256, exact field bits and decoded value.
Record ordinals do not imply a variant, form or runtime record selection.

`shared-movement-operand-schema.json` gives the encodings and **36 pinned
read sites** for shared turn, travel, ascent, fall, dash, cruise/boost
and meter handling. `movement_operands.profile_candidates` joins those
sites by exact lower handler and selector, not by a model name or a
movement-type resemblance. The joins cover **311 static profiles across
205 Borgs**. The remaining profile still has its raw records.

For example, the first records for Sasuke and G-Red both have `12` at
`02C`, the ordinary travel-entry read. They differ at `0F0`, the
forward dash entry operand (`30` versus `25`). Blue Striker's first
record has `7` at `02C`, read as a cruise planar target on its shared
handler, plus `16` at `006` for the refill delay and `50` at `008`
for the refill increment. These are source operands, not native
units, playable time windows or proof of which record the original
actor installs. **The labs do not read this index.** The optional
source-relative pace export below uses it at extraction time.

The same option now adds `movement_operands.class_installer_binding`.
It follows the class installer from its validated table-copy prefix
to each static return. Only pointers derived from the published package
block-0 slot table (`803C4D94`) and whole 432-byte offsets are resolved
to record ordinals. The result stores the installer, final store site
and selected record digest. **202** Borgs have a single static record:
196 select record 0, five select record 1, and one (`0620`) selects
record 2. In `0619`, for example, record 0 has `18` at `02C` but the
statically selected record 1 has `12`.

Four Borgs first retained both a block-0 record candidate and an
unqualified pointer alternative (`0802`, `080a`, `0f05`, `0f06`). The
installer now resolves a single model-ID comparison for `0802` and `080a`:
each compares actor halfword `3E8` with its temporary form's model number,
so the ordinary actor keeps the slot-0 record and only the form takes the
shifted pointer. `0f05`/`0f06` compare actor byte `48A` instead. That byte
is the fusion slot: the fusion routines write `1` or `2` while joining two
actors, also copying the partner model to `49E` and testing `4A0`/`4A1`,
and the unfuse routine clears it. A standalone actor therefore keeps `0`,
so both installers bind the slot-0 record with that guard recorded in the
ledger. Fusion itself is not implemented. This covers
class-installer return only, assuming the ordinary slot publication. It does not resolve resource ownership
later in a live battle, forms, alternate producers, or the source
simulation's units.

Add `--role-clips` to a fresh extraction to expose the unambiguous
**normal-variant** candidates in the native body preview. Every extracted
profile must yield the same exact local, pre-wait, lower-lane clip for the
role. Later requests, common-bank matches and gaps are not used as fallbacks.
The preview plays the raw clip in place; it does not apply source phase
timing, lane composition, root motion or input.

```sh
/path/to/gf_native --preview-borg gg4e08:0615 --borg-variant normal --preview-clip landing
```

The generated profile uses `GFBORG-PREVIEW 2` `candidate=role|group:choice`
rows (or versions 3–5 for fixed mounts, lab audio, or origin markers). A candidate
is decoded when selected, unlike ordinary `clip=` rows,
which retain whole-profile validation. An unsupported candidate does not
invalidate the current preview. The default and candidate selection still
require an exact clip; neither uses the game's null-slot fallback. Two
role-enabled extractions were byte-identical. The draw-checked three-frame
sweep passed **1,792 of 1,880** named movement candidates and **198 of 206** normal
defaults. In that sweep, the type-0 directional dash roles account for **508 of 544** passing
previews. The remaining 36 dash requests are 32 strict reflection refusals
and four clips with unported AnimJoint constraints on `0104`.
Air dash wind-up and active requests add **236** distinct previews; **232**
draw. The four remaining requests hit existing strict reflection refusals.
All **19** additional `glide-held` previews draw.

Ordinary ground dash links require the exact paired entry, active, and
recovery handlers on type-0, type-1, or type-2 profiles with both lanes
configured. Pinned
entry code at `80060C28` and `80060D80` requests group 0, program `27+d`,
where the source direction-byte domain is forward, left, backward, right.
Each direction must resolve to one local, frame-zero, pre-wait lower-lane
clip, agreed across all extracted profiles. There are **166** complete
maps: 136 ordinary type-0, two alternate type-0, nine type-1, and 19 type-2.
Authored aliases are kept; missing or ambiguous
directions do not borrow clips. This links the entry pose only, not original
admission, holds, speed, or recovery scheduling.

The alternate type-0 maps are Teleport Ninja (`0005`) and Switching Ninja
(`0009`). Their exact handler entries use a different upper request and
relay through the shared lower helper before the custom scene-child path.
The solo lab uses only their resolved clip sets. It does not teleport,
switch actors, allocate a child, or apply the original dash rules.

### Try one mapped Borg in the provisional walk lab

From the parent of an extracted `borgs/` directory made with `--role-clips`:

```sh
/path/to/gf_native --borg-controls --walk-borg gg4e08:000a --borg-variant normal \
  --battle-mvp-assets /path/to/local/native-battle-mvp-v1
```

The stage directory supplies `stage.hsd` and `floor.stih`. This branch of
the solo controls test requires an exact animated whole-body preview with
an explicit `ground-travel` clip. It uses the shared camera-relative native
movement and stage queries; the walk speed and distance-to-clip cadence
are provisional. **Arrows** walk, **Q** restarts, and **Enter** pauses.
Edges without ground support stop travel. There is one actor and no
jump, dash, opponent, combat, save access or gameplay admission. The
existing G-Red/Sasuke controls demo remains distinct.

In a desktop package lab, click the Borg label to open the installed-look
picker. It lists exact IDs and variants whose declarations match the
requested lab flags. Use the wheel or **Prev/Next** to page through the
list. The lab pauses while the picker is open. Selecting a look validates
its assets and initial pose before switching to a fresh one-actor run on
the same stage. If the assets refuse, the current body and run stay in
place. The picker reads no saves and never substitutes a normal look for
an unavailable alternate.

The 24-frame native smoke passed **all 181** mapped normal packages
with actual position changes, walk poses and zero shots/hits while pressing
the disabled controls. The lab fits its camera to the evaluated body without
changing the camera-relative input basis. For reflected triangles with an
authored zero normal, it approximates a direction from the evaluated faces
on the draw copy only. This is not original material reconstruction;
strict previews and battle rendering do not use the repair. Unmapped movement
and undeclared variants also refuse. This is a native movement prototype,
not retail speed, timing, collision or appearance equivalence.

#### Jump accounting is not the ascent handler's threshold

The shared ascent handler `8005FAF0` dispatches on `+0x544` and, on entry,
increments the air-action counter at `actor+0x747` (`8005FB54`). That counter
is cleared on the ground/refill path (`80055AA0`, next to the `+0x736` flight
meter clamp) and during state initialisation (`800554F0`), so it counts air
actions taken since leaving the ground.

One gate compares it against `config+0x9F` (`80069268`), which is already in the
operand schema as `09F` (u8). That field is **not** a jump count: it clusters by
family rather than by jump ability — the Wing family `0a00`-`0a09` carries `6`,
the Ninja family `00xx` carries `3`, the two Heroes `0802`/`080a` carry `2`, and
Roach `0f05` carries `1`, with `5` on most of the rest. Treating it as an
air-jump cap would put six jumps on every Wing and one on Roach.

`0x02C`/`0x068` were also checked and are not jump operands: `068` is `0.0` for
107 Borgs and `-0.07` to `-0.70` for the rest, uncorrelated with the catalogue
jump display.

The grounded comparison is the same class gate: `800605E8` requires the counter
to reach `config+0x9F` exactly before clearing the ascent flag, so both known
gates use that one field. The remaining candidate, `actor+0x5AA` passed to
`80046510`, is not a count either — `80046510` is a Q13/Q14 fixed-point scale
(`>>13` or `>>14`), and `+0x5AA` has 59 writers and 71 readers.

The evidence therefore says the game budgets **air actions per class** through
`config+0x9F` rather than exposing a per-Borg jump count in the movement
record. The catalogue jump display is a separate UI figure.

The native API now has a matching capability: `air-action-budget=<1..8>` is a
combined budget consumed by the ground jump (which starts it at one), by each
air jump, and by each airborne dash start, cleared with the landing reset. It is
mutually exclusive with `air-jump-points`, `flight`, `cruise` and non-grounded
movement, so the two counting models cannot double. Grounded walkers emit it
from operand `09F`; cruise, flight, ICBM and already-declared jump pilots keep
their existing rules. `air-jump-points` itself stays unbound, because the source
still exposes no jump-only count. This is a relative reconstruction of the
shared counter, not a verified retail jump table.

#### Compare source-relative cruise pace

The cruise families had two hardcoded `movement=grounded-v1|6|13|12` lines —
one in the pose-free cruise branch and one in the charge-shot cruise branch —
so every flying Borg moved at the same authored `6.00`. Their bound actor
records carry the real cruise target at operand `02C`, read by the shared
cruise handler `800633C8`, so `cruise_pace_candidate` now derives the walk with
the same relative normalisation as the ground pace (`12` → `5` native units per
tick) and the same refusals: an unbound record, a missing operand, a value
outside `1..25`, or a target outside `0.40..10.42` yields no declaration
instead of a default.

Measured effects: `0505` `15.0` → `6.25`, `0511` `7.5` → `3.13`, `0d00` `7.0` →
`2.92`, reproducing a recovered 2:1 ratio that the single authored constant had
erased. The declaration is `native_policy = source-relative-cruise-v1` with
`retail_speed_verified: false`; it preserves relative pace, not retail units or
timing. Static Nu bodies (`0f01`–`0f06`) and the ICBM pair (`0d06`/`0d07`) keep
their authored paces, because they are not cruise pilots.

#### Compare source-relative ground pace

Add **`--source-pace`** to a fresh extraction with `--role-clips`. This
includes the movement-operand index and declares `lab-ground-pace` on the
**179** normal looks with an exact ground-travel mapping and a single
statically selected actor record. The travel operand (`02C`) is scaled
so that `12` gives the existing lab target of `5` native units per tick,
then rounded to hundredths. This preserves relative pace, not retail
units or timing. For example, `0619` uses record 1 (`12` → `5`), not
record 0 (`18`).

```sh
/path/to/gf_native --borg-controls --walk-borg gg4e08:0619 --borg-variant normal \
  --lab-source-pace --battle-mvp-assets /path/to/local/native-battle-mvp-v1
```

The flag changes grounded travel only; default pace, air steering,
jump and dash settings are unchanged. It cannot be combined with
`--lab-cruise`. The picker filters to exact looks that declare the pace.
Two of the 181 mapped normal walkers have conditional record sources
and refuse pace mode; alternate looks do not inherit a normal declaration.
The headless sweep measured all **179** declared targets, with **33**
distinct paces. This is still a no-combat, no-save solo lab.

Add **`--lab-jump`** to the same command to enable **Shift** for a single
grounded jump. The exact look must also declare `ground-takeoff`,
`air-continuation`, and `landing`; incomplete bindings refuse instead of
borrowing clips or silently falling back to walking. Gravity, air steering,
and phase timing are native test settings, not recovered retail behavior.
With jump alone, the lab has no dash. Air jump, boost, combat, and save
access are always disabled.

The twelve type-4 Dragons use the `8006523C` / `80065394` takeoff handlers.
Their later phase advances request `0:2` on both lanes and resolve to each
Dragon's exact local body clip `0:2`. The solo lab uses that transition as
its takeoff clip; it does not execute the original earlier conditional
requests, auxiliary lane, or phase timing. Internal Machine Dragon forms
do not inherit the catalogue model's declaration.

The 90-frame jump sweep covers **181** mapped normal packages.
It checks that the one Borg changes position, draws all three jump phases,
lands once despite a second jump press in midair, and produces zero shots or
hits. It also checks that pause freezes the tick and restart restores the
spawn.

```sh
python3 -B tests/host/retail_walk_lab.py /path/to/gf_native /path/to/roster-root \
  /path/to/local/native-battle-mvp-v1 --jump
```

Add **`--lab-dash`** to enable **L** for a ground-only burst. It can be
combined with `--lab-jump`. The exact look must declare all four directional
dash clips. The lab locks the camera-relative travel direction at entry,
selects the pose relative to the Borg's heading, then returns to the shared
ground controller. Speed, duration and cooldown are native test settings,
not retail constants.

The earlier type-0 combined sweep passed **135 of 136** mapped normal packages.
It checks all four displayed dash directions, an ignored airborne dash
press, one jump and landing, and zero combat activity. `0104` refuses dash
without substituting an unconstrained clip; its walk and jump still pass.
Run the same smoke command with `--jump --dash` against a fresh role-enabled
extraction, such as `local/retail-roster-dash-v1`.
The newer type-1 and type-2 bindings use the same provisional lab
controller. The latest dash-only sweep passed **165 of 166** maps, with only
the existing `0104` refusal. `0628` and `0a00` also passed the combined jump/dash smoke in
headless and offscreen OpenGL respectively. This does not recover how the
original sets the direction byte or changes movement during the burst.

Add **`--lab-air-dash`** alongside `--lab-jump` to use **L** in midair.
It requires all three exact `air-dash-wind-up`, `air-dash-active` and
`air-dash-recovery` bindings. The original frame-zero bindings remain
unchanged. Version 9 adds **63** jump-ready profiles whose exact local
clip and nonzero start agree across all installer profiles. The start
must fit the indexed clip, and the native lab checks it again against
the decoded clip. A reused clip can start at a different frame in each
phase. Wire Girl's uniform authored loop bit is allowed for in-range
sampling only. The lab does not run source waits, loops, or repeat branches.
All **181** jump-ready normals now have an air-dash declaration.
All 181 passed the combined headless jump, air-dash and landing-PCM sweep.

The lab uses a 5-tick wind-up, 8-tick directional burst, and 6-tick
recovery. Gravity continues through all three. The press commits the
burst's camera-relative direction and spends the one-burst allowance;
another press in the same airtime refuses, even after recovery.
Landing clears the phases and restores the allowance. Ground dash
remains a separate opt-in. These are native test rules, not recovered
retail gauges, steering, timing or input gates.

```sh
python3 -B tests/host/retail_walk_lab.py /path/to/gf_native /path/to/roster-root \
  /path/to/local/native-battle-mvp-v1 --jump --air-dash --landing-audio
```

Add **`--lab-glide`** alongside `--lab-jump` to hold **Shift** while falling.
This is a separate lab policy from `--lab-air-dash`. It requires
`glide-entry` and `glide-held` in the exact look. The type-2 glide
entry at `800626D8` requests lower-lane program `0:34`; the held clip
comes from its descriptor select after the first wait, not from the
initial `0:0` pose. The 19 accepted normal maps resolve to two exact
local slots: `5:2` for 11 and `4:4` for eight.

The prototype limits descent to 1.5 native units per tick while held and
resumes ordinary fall on release. It cannot climb or hover and only lands
on actual native floor contact. All **19** mapped Borgs passed the headless
entry, held pose, release, landing and landing-audio checks. The six-tick
entry, descent limit and air steering are native settings, not retail
glide timing, braking, resource use or hover-contact behavior.

### Try a flyer in the provisional cruise lab

Add `--cruise-lab` to a fresh extraction. It requires one type-3 movement
candidate and selectors 27 and 28 resolving on every installer path to
the shared cruise/boost handlers. An exact normal look with no motion bank
gets a version-6 profile. A look with a motion bank needs a single exact
frame-zero local entry clip shared by body lanes 0–2 across every profile;
it gets version 8. Other movement types and unbound or conflicting body
poses refuse.

```sh
/path/to/gf_native --borg-controls --walk-borg gg4e08:0d00 \
  --borg-variant normal --lab-cruise --battle-mvp-assets /path/to/stage-assets
```

**Arrows** steer, **Shift** boosts, **Enter** pauses and **Q** restarts.
The shared authored cruise controller keeps a fixed native height above
stage support, not a recovered retail altitude or hover-contact rule.
Boost spends the meter; after boost stops and the delay expires, it
refills **while the Borg remains airborne**. A held button cannot
restart boost after depletion without a fresh press. The static body
follows the native position and heading without invented animation.
Eagle Jet (`061b`, `2:0`) and Victory Jet (`0620`, `4:6`) use the exact
first clip from their selector-0 entry program as the lab entry. The lab
plays it once, then holds the last pose for cruise and boost. It does not
execute the auxiliary common lane, Eagle Jet's later program command, or
a retail flight transition scheduler.
There are no jumps, landings, attacks, or save access in this mode.

All **nine** admitted normal packages (seven static, two animated) passed
the headless flight, depletion, and airborne-refill test. An exact
alternate without a cruise declaration refused for each body type.
Both jets passed offscreen OpenGL, and the picker kept the previous
static flyer when an animated entry clip could not bind.
Use `tests/host/retail_walk_lab.py ... --cruise` with the same three
paths as the movement smoke commands above.

Pass `--spec-text /path/to/spec-text.gftx` to add exact authored display names
from the qualified local Force Editor export, by collection ordinal and
original slot. Each wholly named, nonempty move list is exposed as
`GFBORG-MOVES 1` review metadata. Five displayed slots have blank text:
they stay in the source index without an invented label, and their five
packages do not publish a partial move profile. Eight more have no authored
displayed slots. All 193 emitted move profiles passed native `--check-borg-moves`.
The move labels and the displayed B/X buttons are not executable rules.

The neighbouring `action_candidates` index reads the destination tables of
the installer's shared copy prefix. The static walker keeps the configuration,
context-table and handler-pair pointers on the same candidate path instead of
combining each set independently. It follows bounded local transfers and the
one pinned model-indexed switch; it does not evaluate phase/input conditions
or run an original installer. Groups include B-shot, B-close, X, B-charge
and X-charge; the unresolved fourth alternate group stays numeric. A disabled
group, signed refusal row and all-zero row are kept distinct. The exact normal
tuple from every one of the 206 existing local checkpoints was present, and
all **9,960** normal context rows compared equal. Other tuples remain static
candidates, not qualified normal overrides or in-battle forms.

When the same paths also retain the main handler and both program/descriptor
namespaces, there are 319 distinct tuples. All 206 normal tuples matched the
expanded fields of the prior local checkpoints. The linker recognizes standard
indexed state/substate dispatches, including a bounded conditional prologue.
It follows phases 0–2 for four pinned tables. For other recognized
phase tables it follows phase zero, and adds phase one only after detecting
a straight entry-prefix increment of the same actor's phase byte before
any call. It labels the phase on each candidate. Within each local handler it follows bounded direct edges
and propagates constants through integer arithmetic and initialized source
lookups. It retains the narrower literal-block evidence when available.
Ambiguous values, call clobbers and unsupported operations cannot supply an
argument. It does not evaluate input, pose cues, ammo, or phase conditions.
A zero/no-op action row does not grant a handler link.

Program requests keep the lane mask and program key. Descriptor choices link to
the exact indexed local or common motion slot; a missing slot stays null rather
than using the default. The initial pass found program calls on 108 Borgs
and 98 projectile-origin links on 46. Twelve pinned same-actor
release helpers can contribute one hop from a reviewed B-shot call site.
Forty-four origin links across 25 Borgs use these helpers. The scan keeps
each model gate: for example, Machine Red (`0600`) selects type-2/key 21,
while `0608` selects keys 51 and 52 through a sibling branch. Neither takes
the other's rows. Runtime forms, stock gates, and pose cues remain unresolved.
These are review candidates, not move declarations or movement roles.

Seventeen additional phase windows pin the exact dispatch table prefix,
the handler bytes, and same-actor phase advances to a specific release leaf.
The linker does not use neighboring table words as extra phases. These add
**31** release links, without
changing any prior link or program/clip binding. For example, a shared
B-charge leaf requests type-2/key `20` for `0d00` and key `56` for
`0d04`; the comparison excludes `0d00` from the second branch. The index
records the predecessor store sites, not cue timing or proof that live
input reaches the leaf.

Three further helpers add six links across `0628` and `080c`.
The `0628` X/B-charge route is
qualified separately from its B-shot route. Its helper has additional
`0614` branches, but those are not assigned to `0614` without an exact
caller in that Borg's action contexts.

Nine reviewed two-entry small-data phase tables add ten more links. Eight
of these tables reach phase one through
a shared helper that advances actor byte `+540` and clears `+541`.
The release index retains both the call into that helper and its actual
store site. `0800` and `0801` keep distinct type-0 keys `12` and `125`;
their reviewed release branches check bytes `+541`, `+1CEF` and `+1CF0`
before testing the debit helper's return. The separate `0610`/`061e`/
`0621`/`0623` table has a direct phase advance and model-gated type-2
keys `53`, `85`, `86` and `87`. These are source read/write and call
links, not verified cue producers or live timings.

The neutral nested dispatches for `0100` and `0105` add four links.
Their factories read the side byte `+1B04`: zero
and nonzero select keys `0`/`1` and `56`/`57` respectively. Each pair
shares a call site but has different joint bindings. The installer
comparison at `8019858C` confines config `8036FD68` to model `010a`.
Model `0105` returns with `8036FC50`; the action scan no longer
treats the `010a` write as a possible `0105` path. The movement scan
retains both stores for `010a`, but not the unreachable override for `0105`.
Neither scan infers a live form change.

The pinned model branches in the `0505`/`0511` helper keep keys `103`/`104`
on Death Head and `121`/`122` on Machine Head. The Titan and Panther robot
handlers likewise keep only the two calls on each model's branch.
Eagle Jet (`061b`) uses keys `83`/`84` and Victory Jet (`0620`) uses keys
`93`/`94`; the other keys in those handlers belong to different model
IDs. Violet Attacker adds two selector alternatives at one paid call site.
The index now has **137 release links across 72 Borgs**.

Six elemental Dragons (`0500`, `0509`, `050a`, `050c`, `0515`, `0516`)
share a paid type-47 carrier call. The extractor pins the phase dispatch,
the complete 23-entry model branch table and the carrier constructor. Each
normal model gets its own key `0` through `5` and exact parent-joint-12
local origin `(0,0,0)`. The bounded branch proof preserves the positive
actor cue and one same-actor channel-0 debit. This is separate from their
type-46 breath and type-30 descendants. The child cue is indexed
separately from the carrier rules; only the opt-in authored test
below binds a projectile or launch sound.

`shared-dragon-carriers.json` keeps the six exact 48-byte rows and the
active callback's read sites. The carrier keeps five 20-byte trail
records on itself and samples a four-row curve; those records are not
periodic projectile allocations. The retained-contact and terrain-hit
branches separately read the row's area key and secondary selector:

| Dragon | Carrier key | Area key | Type-30 mode |
|---|---:|---:|---:|
| Flame (`0500`) | 0 | 3 | — |
| Plasma (`0509`) | 1 | 24 | 2 |
| Blizzard (`050a`) | 2 | 21 | — |
| Fire (`050c`) | 3 | 22 | — |
| Thunder (`0515`) | 4 | 25 | 3 |
| Ice (`0516`) | 5 | 23 | — |

The table preserves the collision group, raw motion operands and
lifetime seed without converting them to native speed, hit damage or
wall-clock time. Each group's exact normal slot-2 bank yields one
carrier sphere centered at its local origin: radius 180 for Flame, Plasma and Blizzard, and 90 for
Fire, Thunder and Ice. Plasma and Thunder additionally bind an emitter
sphere (group 7, radius 10) and a descendant capsule (group 8, radius
110, local Y endpoints 0 and 400). These are source shape operands,
not proof of registration or hits. The scoped placement and collision evidence is
documented in the [Dragon carrier audit](borg-attack-visual-recovery.md#type-30-collider-follow-up).

The carrier's visual requests and the type-30 descendant use different
archives. The carrier requests record **9** five times and record **8**
once (`800BED7C` / `800BED9C`) through the caster-slot archive selector.
The receipt checks both record roots on each Dragon's exact normal
`visual/body.hsd`; it does not substitute another look's archive.
Plasma and Thunder's descendant requests record **83** from the shared
effect archive (`80183DE0`). The exporter preserves `efct.pzz` member
697's atlas and distinct single/split model blocks under `effects/`.
Both model blocks contain record 83 and its own material entry. The
native inspection loaders decoded the 256×64 atlas, all six 14-record
caster banks and both 157-record common banks. The source draw callback
uses the onboard trail count and cursor for its record-9 submissions,
then submits the separately owned record 8 at the carrier matrix.
An isolated native geometry smoke accepted record 8 on all six Borgs,
record 9 on Flame and Fire, and record 83 in both modes. On Plasma,
Blizzard, Thunder and Ice, normal record 9 is an authored empty static
record; a drawable request refuses without substitution. None of these
checks executes the original pass, material timing or projectile logic.

`shared-dragon-impact-areas.json` joins the six terminal area keys
to exact type-5 rows. All six bind **group 60** from the scene common
collision bank, select **record 22** from the shared effect
archive, and request **`snd_com01:12`** as an area-child cue. The raw
scale and material operands remain per row. Record 22 drew in both
shared archive variants in the isolated native geometry smoke, and
tone 12 decoded to 150,528 mono frames at 44.1 kHz.

The source loader assigns **`cmn_data.pzz` member 696, slot 0** to the
common collision context. The exporter preserves those exact bytes as
`shared-common-collision.bin`. Group 60 contains only row **94**, a
local-origin sphere with raw radius **310** and parameter **4**. The
area initializer multiplies row `+08` by the size copied from the
carrier; the active scale callback writes the evolving `+148` value
back to the axes and uniform scale. The index keeps those operands
separate from the raw radius. Live radius, registration, contact,
damage, and timing remain unverified and unbound in the Dragon pilot.

```sh
c++ -std=c++20 -O0 -I. tests/host/retail_dragon_art.cpp -o build/retail-dragon-art
build/retail-dragon-art /path/to/roster/borgs
```

The portable `projectile-record` binding can now draw a package-owned
static record at an accepted shot's position. The scratch-only authored
test in `tests/host/retail_projectile_record.py` uses a Dragon record
but no retail moveset. It compares battle events with and without the
visual, and checks that missing and empty records refuse instead of
falling back to a procedural orb.

### Inspect action-program control writes

`shared-action-program-controls.json` keeps **245 program request bindings
across 85 Borgs** that contain a nonzero write to actor bytes `+1CEF`
or `+1CF0`. It checks the opcode-2 dispatch entry and the shared
upper-lane writer. Each binding preserves its exact program namespace,
raw command bytes, both control values, and the authored wait flags and
thresholds. Zero/reset writes and signed `-1` writes are kept in the
same stream. A source wait threshold is not elapsed native ticks.

For the eight reviewed `0800`/`0801` phase tables, the same action
context links the exact phase-zero program request to the phase-one
release branch. In their group-2 choices `0`, `1`, and `2`, the program
writes `(3,1)` after one flag-2 wait with thresholds `9`, `9`, and `6`
respectively. It later writes `(-1,0)`, which is distinct from the
release tuple. The join keeps the exact program address, control command,
phase table and factory call. Other program writes are indexed without
being classified as releases. Nothing in this file executes the
original program or sets runtime cue timing.

For `0610`, `061e`, `0621`, and `0623`, the pinned entry at `8015A8F8`
initializes actor byte `+6EA` to `0` or `1` according to the `0x40` bit
of actor word `+5E0`. The request at `8015AA0C` uses the byte **before**
incrementing its stored value. Both group-2 choices write `+1CEF = 1`
after a flag-2 wait with threshold `14` and join the same positive-cue
test at `8015AA24`. The factory key remains model-specific. This does not
qualify later repeat choices or cue timing.

### Inspect level stock operands

`shared-level-stock-operands.json` separates the two four-halfword stock
records in each normal level row: initial count, limit, mode and preset.
It covers **4,120 channel-level records** across **206 Borgs**, preserving
the original values even when initial stock is zero or exceeds the limit.
For example, `0c06` level 10 channel 0 is `23/22`, not `22/22`.

The schema records the original initializer and shared debit/refill
helpers. For the reviewed helpers, mode 0 arms a preset on empty and mode
1 adds the preset to the running clock. Modes 2 and 3 are retained as raw
values. The preset is not a measured reload duration or a native tick
count, and a stock channel is not a B/X button assignment.

The release index retains the original **14** hand-reviewed `stock_candidate`
links, and adds `local_stock_guards` to the exact handler routes of
**130 release links across 67 Borgs**. This scan accepts only bounded
graphs with no unresolved edges, literal committed debit operands, and
the same input actor at both calls. It rejects a link if the factory is
reachable from the failure edge or can bypass the success edge. This
includes all 14 earlier witnesses. A guard alone does not establish model
selection in a shared handler, and a missing guard is not a stockless
classification.
The scan preserves the saved actor pointer across a subtract that writes
another register. This also exposes guards for `061b` and `0620`.

`080c` has two linked children behind one channel-0 debit site, and
Machine Red's ordinary rifle route links key `21` to a separate
channel-0 debit. `0628`'s X/B-charge pair instead uses channel 2; the
normal level initializer only supplies channels 0 and 1, so no level-row
stock is assigned to channel 2. A local proof does not establish live
release, successful allocation, refill scheduling, or one debit per
spawned child. It does not change executable moves.

### Inspect a mapped action-entry clip

Add `--action-clips` to a fresh extraction. It can be combined with
`--role-clips` and `--origin-markers`. For each of B-shot, B-close, X,
B-charge and X-charge, the exporter keeps the first raw context cell
whose installer profiles agree on one exact local clip. An action-entry
path must have one resolved upper-lane program request that all bounded
local paths reach, with no competing unresolved request. Its program
must select a single nondirectional descriptor before the first wait,
starting at frame zero. Common-bank slots and missing clips do not fall
back to idle.

IDs such as `b-shot-c0-s0` identify the raw context and sector, **not** a
verified way to trigger the clip. `action_preview_clips` in `source-index.json`
retains the selector, state, substate, program and descriptor for each
contributing installer profile. The preview samples the raw clip through
the shared body renderer; it does not compose original lanes or execute the
program. A separately selected origin marker is not tied to a firing frame.

```sh
/path/to/gf_native --preview-borg gg4e08:0007 --borg-variant normal \
  --preview-clip b-shot-c0-s0 --preview-origin type-2-1
```

The four Borgs above expose both agreeing conditional choices as
`b-shot-c0-s0-p0` and `b-shot-c0-s0-p1` (`1:9` and `1:10`).
The extractor does not choose one as a default: both must agree across
installer profiles for the same raw context, or neither is published.

Three reviewed phase-one handlers also request an exact shot clip
after the phase-zero lead-in. Those links retain the phase table and
request site; they are not descriptions of the earlier pose.

Seven rifle Borgs have a separate seed-qualified candidate: the checked
class hook sets `+6EE` to zero when actor word `+5E0` bit 0 is clear.
The group-2 choice-0 program has three local upper-lane release poses
`3:12`, `3:11`, and `3:13`; the index preserves all three and the following
cue command. The preview and authored test use the middle listed choice
only. This does not infer which direction or seed is live, reconstruct
the lead-in, or admit a repeat.

Six throwers use another zero-seed path with a verified control cue.
Arrow Ninja and Shijima preserve all three directional pose choices
(`3:2`, `3:0`, `3:1`) and the original start-frame operand `7`. The
preview uses `3:0` as a raw clip; the authored combat profile also uses
it with its own marker clock and does not reproduce the source start
frame or direction choice.

Death Borg Theta (`0906`) has a separate zero-seed path in context `4:2`.
It requests group-2 choice `6` (`+6EE + 6`); the later directional select
preserves `3:17`, `3:16`, and `3:18`, source start-frame operand `3`,
and the subsequent `(1,1)` control write. The checked consumer requires
both bytes before entering the paid key-106 branch. The native test uses
only clip `3:16`, not a recovered direction, aim gate, or timing rule.

The extraction declares **234 action previews across 149 Borgs**.
The six elemental Dragons keep their original action-entry preview and
add `b-shot-release-pose` for the exact later `3:0` clip; the four Worms
likewise retain their entry clips and declare their own later `3:0`.
The reviewed Girl-family, Knight, Samurai, Ninja, Robot, and Diver close-B routes also
declare their `1:0`, `1:1` or `1:4` clips for inspection. These are raw pose candidates, not recovered hit or
fire frames; the melee pilot applies the recorded starting frame.
In the three-frame headless checks, **215 of 234** submitted body
geometry, including all eight conditional previews and Alien Worm's
release pose. Eighteen hit strict reflection-normal refusals, including
the other three Worm release previews. Beam Gunner's exact B-shot
candidate refused an AnimJoint constraint. No source variant inherits
a normal action clip declaration.

### Check B-shot admission blockers

With `--source-pace --role-clips --action-clips`, the extractor writes
`shared-basic-shot-readiness.json`. It separates missing locomotion and
walk operands, missing clip-to-release evidence, and known strict clip
metadata refusals. It checks exact action contexts across installer
profiles, not just a shared move label. A static match still needs a
strict native trial; it does not create a gameplay profile.

Before the four Tank pilots, with 68 single-shot profiles and Gatling
Gunner's separate held-fire profile included, the other 137 packages had these first
blockers: 23 lack the required grounded locomotion roles, two lack a
single bound walk operand, 110 lack a correlated shot pose, one has
candidates but no qualified release context, and one has a strict clip
refusal. Beam Gunner (`0104`) now has the exact ordinary B-shot pose
candidate `3:11` (alternatives `3:12` and `3:13`) and the existing key-11
release link. Its selected shot clip and forward dash clip both contain
unsupported AnimJoint metadata. The profile described below keeps the
bound locomotion poses, does not use the shot clip, and declines dash
rather than substituting a clip. This does not implement its X beam.

### Try the opt-in authored B-shot Battle Test

Add `--basic-shot-test --source-pace --role-clips --action-clips` to a **fresh,
separate** roster extraction. **60 grounded Borgs** receive a normal-only
`GFBORG-GAMEPLAY 1` profile: `0000`, `0002`, `0003`, `0007`, `0008`, `000a`, `000b`,
`000c`, `0100`, `0103`, `0105`, `0500`, `0501`, `0509`, `050a`, `050b`, `050c`, `050d`, `0515`, `0516`, `0517`,
`0600`, `0602`, `0604`, `0606`, `0608`, `060a`, `060c`, `060e`,
`0610`, `0613`, `0615`, `0616`, `0618`,
`061a`, `061c`, `061e`, `0621`, `0623`, `0627`, `0628`, `0629`, `062a`,
`0800`, `0801`, `080c`, `0900`, `0901`, `0902`, `0903`, `0906`, `0907`, `0908`, `0909`,
`090a`, `090c`, `090d`, `0d03`, `0f01`, and `0f02`. They use their own exact models
and any required clips, level-1 HP and stock limit, and reviewed joint-local sockets.
Every contributing installer must agree on the paid release in the same raw
action context, and on the attack clip where one is used.
Later reviewed additions to this cohort are the four Tank singles
(`0c00`, `0c01`, `0c05`, `0c06`), the Titan/Victory pairs (`0619`,
`061f`, `0625`), Beam Gunner (`0104`) and Gold Hero (`0802`). Cyber Ninja (`0007`) uses `b-shot-c4-s2`
(`3:19`) in the test install because its default `b-shot-c0-s0` does not
join the reviewed debit and factory. Its short clip uses a shorter
authored timeline. Neither choice claims a recovered input rule.

Also passing `--cruise-lab` opts in six static and two animated flyers,
for **68 Borgs** total. The eight flyers use the authored cruise controller. The static
packages use `body-motion=static-cruise`. Death Head (`0505`) and Machine Head
(`0511`) each attempt two model-specific launches behind one debit.
Blue Striker (`0d00`) and Orange Fighter (`0d04`) instead use a bounded
hold-and-release B window to launch their own charge projectile, key `20`
or `56`. These two move profiles are stockless; the reviewed release
leaf has no stock debit, which does not establish the complete retail
charge policy. None borrows a clip or claims original vertical flight
or attack timing.

Death Borg Mu (`0f00`) uses its exact static rig and the reviewed key-60
socket, with one paid B release and authored add-on-debit refill timing.
This does not reproduce the source phase schedule or close B behavior.

Violet Attacker (`0d05`) uses its static body and the first barrel
(type-0 key `16`). The source phase zeroes selector byte `149`, then a
successful debit enters the shot branch and advances the one-bit barrel
selector. Both key `16` and key `17` stay indexed as **alternatives**, not a
two-shot volley. The authored pilot uses key `16` on every accepted press;
it does not reproduce barrel alternation, repeat fire, or Red Attacker's
different factory.

Eagle Jet (`061b`) and Victory Jet (`0620`) bind the same exact entry clip
as the solo lab using `cruise-entry`. One authored airborne B marker pays
for two ordered launches per model. Boost and B leave the entry pose
clock running; no attack clip is invented. Neither retail repeat timing
nor Victory Jet's B-charge association is implemented.

The six elemental Dragons use exact `idle`, `walk`, `air`, and later
B-shot `3:0` bindings. The authored pace is twice the source-relative
lab target so both cohorts stay above the gameplay API minimum.
`dash-policy=disabled` prevents substitution for a missing dash
binding. One paid B marker launches an authored orb from joint 12
using exact carrier art (record 8) at an authored scale. A downward
launch bias for each muzzle-height cohort is native tuning, not a
recovered direction or ballistics rule. The optional
sound is the indexed tone 71 on accepted native launch, not the source
child-phase timing. Flame and Fire also draw their exact record 9
using five samples of accepted projectile positions. The other four
retain the authored empty record without substitution. The trail is
an authored visual only. All six can display the exact single-player
shared record 22 on accepted actor impact for 18 authored ticks. It
expands from 10% to 25% scale and fades over those ticks, without using
the retail area scale curve. Each package includes the exact local
archive; the pilot does not select a
split-screen variant or require a separate content root. The visual
does not bind an area collider, extra damage or a type-30 child.
With `--dragon-impact-audio` and `--basic-shot-audio`, the six Dragon
profiles also play the exact dry `snd_com01:12` sample on accepted actor
contact. It uses the authored contact event, not the retail area-child
collision bind. The impact sample is not shortened to the launch limit.
Retail scaling, trail curves and breath moves remain unimplemented.
For a roster extracted with that cue opt-in, add `--impact-audio`
to the Battle Test smoke alongside `--launch-audio`.

Alien, Venom, Claw and Poison Worm (`0501`, `050b`, `050d`, `0517`)
use their own exact locomotion and stationary B release pose `3:0`.
`shared-worm-shot-routes.json` separately preserves the four model-gated
calls, the paid channel-0 request, joint-12 local offsets, and the
type-49/type-94 collision-bind cue routes. The pilot uses one authored
orb and a 180-tick refill, even for the two mode-2 stock rows. Optional
launch PCM uses `snd_com02:82` for the type-49 models and `snd_com03:4`
for type 94 on accepted native allocation, not the retail bind.
String behavior, poison, traps, descendants, refunds, live timing and
the moving variant of the B pose are not implemented. Venom, Claw and
Poison each have seven authored zero normals in one reflection mesh.
Their normal Battle Test recipes declare that exact mesh and count,
using a neutral texture coordinate only for those vertices. Alien Worm
needs no declaration. Strict whole-body previews still refuse this
condition; normal vectors, source textures, and combat rules are not
changed.

Panther Vehicle (`0628`) keeps its exact locomotion during the authored
B move, with no separate attack clip. The reviewed source helper reads
its timer and remaining count, checks stock without spending, then
attempts one debit before the ordered type-0 keys `79` and `80`. The
native test uses one paid marker and two exact sockets; it does not
implement the original repeat clock, X, or charge paths.

Titan Robot (`0604`), Proto Titan (`0618`), Panther Robot (`0613`), and
Proto Panther (`0627`) use the same authored locomotion-preserving move
policy. Their reviewed direct handlers use a timer and count before one
same-actor debit, then select the pair by model: `43`/`44`, `75`/`76`,
`50`/`51`, or `77`/`78`, respectively. The native marker attempts exactly
one pair, not the original timed sequence.

Death Borg Nu (`0f01`) and Nu II (`0f02`) have empty motion banks. Their
type-1 ground-travel paths carry the reviewed turn requests without a
clip. The authored `static-grounded` profiles use each exact body, a
source-relative pace from its bound actor record, and one paid grounded
B shot: type-0 key `74` or type-2 key `61`. Nu II uses the add-on-debit
stock policy. The test does not claim retail jumping, landing, or charge.

Flying Saucer (`0d03`) uses the same `static-grounded` body policy, but
its pace comes from the type-6 alternate travel handler rather than
the ordinary turn-to-travel path. The reviewed phase pays once before
ordered keys `54` and `55` at distinct sockets. The authored B marker
attempts one pair; it does not reproduce the retail repeat clock, close B,
or the alternate takeoff and landing scheduler. Carrier Helicopter
(`0d02`) shares the indexed movement route but gets no battle profile.

Revolver Gunman (`0100`) uses the reviewed zero-side key `0` and
clip `3:0` from the scoped zero-seed `2:0` path, not the separately
reviewed `2:4` request. Key `1` stays in the evidence index, not as a
simultaneous shot or native side switch. Death Borg Beta II (`0105`)
now uses its own zero-side key `56` and exact clip `3:0`; key `57`
stays indexed separately. The `010a` override is not admitted.

`0608`, `061c`, and `080c` each issue two ordered requests after that one debit,
with separate exact offsets. Their checked source branches call the same
factory twice without inspecting the first result before the second call.
The native profile expresses this using a paid `release` followed by
`release-followup`, not a two-count volley or two debits.
Cyber Machine Seiryu (`0602`) uses two follow-up rows for its three
reviewed keys `22`, `23`, and `24`, each with a distinct socket offset.
The source calls have a fixed order, but native hits can arrive in a
different order. This profile uses only the zero-addend candidate of the
source program choice, not its pitch-dependent alternatives. Wire
Gunner (`0103`) uses its seeded key-38 shot and a short authored marker
window. These additions do not implement their other attacks.

The related Cyber Machines `060a`, `060c`, and `060e` use the same
reviewed release-pose path but keep their own six, five, and four
factory calls and socket offsets, respectively. The native marker
supports up to six requests, each with an independent allocation, behind
one debit. The extractor pins every factory call in order; it does not
assume that one model's branch or geometry transfers to the others.

Eagle Robot (`0606`) and Proto Eagle (`061a`) use the explicit
`idle-loop=authored-bit` binding for their exact looping idle tracks.
Their paid B-shot paths use different factories (type-2 keys `30`/`31`
versus type-0 keys `110`/`111`) and distinct sockets. The test uses one
debit with one follow-up. Removing the idle declaration keeps the strict
AObj-bit refusal; it does not pick a different idle or relax action clips.
For `080c`, the test uses context `4:0` (`3:33`), where a later positive
animation cue invokes the paired release helper. The helper's actor-owned
sound remains separate from the child sounds; its two child operands are
signed `-1`, not valid common-bank tones.

`0800`, `0801`, `0f00`, and `0f02` use the authored add-on-debit clock for their mode-1
stock; the other paid tests use an empty-then-refill clock. Where a B-shot
clip is bound the test picks one reviewed clip. Aim, physics, damage,
release and refill timing are authored. It does not implement alternate action branches,
close B, healing, guard moves or any other X action, and does not claim
that the exported move matches retail combat.
The `0615` test is an authored grounded profile, not the separate exact
G-Red practice movement profile; do not install both versions of that
stable ID in the same root. The same separation applies to the new `000a`
profile and the earlier Sasuke test package. The seven type-14 tests use the
exact indexed joint points and the existing native shuriken visual;
retained-actor branching and source projectile behavior are not reproduced.
Double Ninja (`0003`) uses its scoped zero-seed `3:0` throw and the
primary key-0 release. The separate key-2 call remains in the evidence
index: it requires a nonzero clone state and adds an owner-tracked world
offset. The authored test includes neither that follow-up nor charged X.

The pilot copies these Borgs' alternate wrappers as offline assets without
admitting the variants or forms. The default extractor output is unchanged.
All pilot Borgs except `061e` and `0623` explicitly bind record zero as their body.
For `0610` and `0621`, archive-wide
binding still refuses an unsupported polygon in another record. The selected
body remains under strict material, skin, rig and reflection validation;
unselected records are not drawn or granted gameplay meaning. No material or
model fallback is used.

From the new install root, supply a Battle Test opponent (for example,
`portable_borg_preview target`) and launch the usual native host:

```sh
PYTHONPATH=/path/to/repo python3 -B -m tools.extract.portable_borg_preview \
  target borgs/training-target --art /path/to/native-battle-mvp-v1
/path/to/gf_native --battle-borg gg4e08:061e --against-borg test:target \
  --battle-mvp-assets /path/to/native-battle-mvp-v1
```

Space is B; arrows move and Shift jumps. The headless and offscreen
OpenGL smokes check movement, joint-local releases and target hits before
and after restart. The eight flyers cruise rather than jump. The two
charge tests reject a short tap and launch without spending stock.
The fifteen paired profiles each debit once and hit twice per round;
the Cyber Machine profiles debit once and hit three to six times per round:

```sh
PYTHONPATH=. python3 -B tests/host/retail_basic_shot_smoke.py \
  build/portable-content-v1/gf_native /path/to/roster-install \
  local/content/native-battle-mvp-v1
```

For the recovered reaction rows, use the direct output directory of
`retail_borg_roster` from a fresh conversion with the flags above (including
`--cruise-lab`). This smoke checks modes, offsets, the explicit linked-owner
root on seven mode-4 shots, and the authored training-target damage. It
verifies contact in the host before and after restart:

```sh
PYTHONPATH=. python3 -B tests/host/retail_projectile_reaction_smoke.py \
  /path/to/gf_native /path/to/direct-roster-output \
  local/content/native-battle-mvp-v1
```

A fresh conversion with `--basic-melee-test --source-pace --role-clips
--action-clips` emits neutral close-B reaction mode 4 and offsets wherever
the audited direct contact steps agree. It uses the native planar body root
and leaves the authored melee windows and damage unchanged. For the 14
pose-only openings with a verified first continuation, it also appends
`pose-followup=opening-slash|6|1:1|0`. This shows the exact local swing from
frame 0 before the existing hit window, using an authored switch tick
rather than the source phase timer. To check all 14 continuation packages
and compare the sampled entries with the public map before and after restart:

```sh
PYTHONPATH=. python3 -B tests/host/retail_melee_reaction_smoke.py \
  /path/to/gf_native /path/to/direct-roster-output \
  local/content/native-battle-mvp-v1
```

Add `--contact-scale-test` to a separate, fresh shot or melee conversion for
an authored settings-7/rank-0 damage comparison. Direct hits combine the
attacking team's credit row, a qualified directional matchup, and the
actual receiving Borg's class row.
Unqualified receivers, including combined forms without a proved active
component, receive no class row and keep the authored base. Without this
flag the existing damage rows stay unchanged. The source snapshot and scope
are documented in [the contact roles map](api-map/damage-contact-roles.json).

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_scale_smoke.py \
  /path/to/gf_native /path/to/contact-scale-roster-output \
  local/content/native-battle-mvp-v1
```

`--contact-index-test` requires `--contact-scale-test` and adds the full
setting and rank tables. Use `gf_native` with
`--package-damage-indices TEAM0,TEAM1:R0,R1` (four ranks for
`--demo-roster-battle`) to select them for an explicit package Battle
Test. For example `--package-damage-indices 0,31:3,0` gives the first
actor rank 3 and the second rank 0. Omit the selector to keep the
fixed snapshot. This is not a retail difficulty or Story scheduler.

For the indexed smoke, include `--contact-power-test` but leave the
HP/force ladder option off:

```sh
PYTHONPATH=. python3 -B tests/host/retail_damage_indices_smoke.py \
  /path/to/gf_native /path/to/contact-index-roster-output \
  local/content/native-battle-mvp-v1
```

`--contact-power-test` optionally binds the recovered raw HP base of each
reviewed direct move. Zero-power or disagreeing source steps leave the
authored base intact. To check both raw bases and their composition with
receiver scaling, generate a fresh roster with **both** damage-test flags:

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_power_smoke.py \
  /path/to/gf_native /path/to/contact-power-roster-output \
  local/content/native-battle-mvp-v1
```

`--contact-floor-test` optionally keeps positive direct hits from
truncating below one HP. It is independent of `--contact-power-test`.
For the sub-one damage comparison, generate with scale, index,
power, and floor enabled and leave HP/force ladders off:

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_floor_smoke.py \
  /path/to/gf_native /path/to/contact-floor-roster-output \
  local/content/native-battle-mvp-v1
```

Add `--contact-ladder-test` alongside `--contact-scale-test` to bind the
verified normal receivers' and proved direct attacks' HP/force curves.
An untracked force pool stays neutral. Unqualified receiver records get
no receiving curve, and the outgoing curves apply only to marked moves.
To check the generated HP response against a controlled
low-health recipient, generate with all three contact-test flags:

```sh
PYTHONPATH=. python3 -B tests/host/retail_receiver_ladder_smoke.py \
  /path/to/gf_native /path/to/contact-ladder-roster-output \
  local/content/native-battle-mvp-v1
```

`--contact-tier-test` requires `--contact-scale-test` and is independent
of the HP/force ladders. It binds the verified direct moves' charge and
the qualified receivers' tier table. The native idle window is 60
battle ticks, not the source actor clock. For the following comparison,
generate with scale, power, and tier enabled but leave the ladders off:

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_tier_smoke.py \
  /path/to/gf_native /path/to/contact-tier-roster-output \
  local/content/native-battle-mvp-v1
```

Add `--contact-tier-reset-test` to additionally bind the receiver's
independent gauge capacity and the proved moves' wear. For the reset
comparison pass `--reset-gauge` to the same smoke: it verifies Sasuke's
exported capacity of 500, then lowers it to 50 in a temporary fixture so
that two 25-wear hits exhaust it in one burst.

Add `--contact-audio /path/to/retail_borg_sound` to a shot or melee
Battle Test extraction to bind the reviewed normal hit cues. It is
independent of the damage-test flags and `--basic-shot-audio`.
The decoder uses your local retail banks; do not publish the PCM.

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_sound_smoke.py \
  /path/to/gf_native /path/to/contact-audio-roster-output \
  local/content/native-battle-mvp-v1
```

`--contact-art-test` is a separate opt-in for exact common hit records
at reviewed ordinary projectile and close-B contacts. It uses the
local common effect bank, keeps existing authored actor-retirement
art where present, and does not require the audio decoder.

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_impact_smoke.py \
  /path/to/gf_native /path/to/contact-art-roster-output \
  local/content/native-battle-mvp-v1
```

Add `--contact-pause-test` to hold the native local skeletal sample
on verified direct contacts. This needs neither the audio decoder
nor any damage-test flags:

```sh
PYTHONPATH=. python3 -B tests/host/retail_contact_pause_smoke.py \
  /path/to/gf_native /path/to/contact-pause-roster-output \
  local/content/native-battle-mvp-v1
```

Add `--basic-shot-audio /path/to/retail_borg_sound` to a fresh pilot
extraction to bind the indexed dry launch tones (`snd_com01:6`,
`snd_com01:8`, `snd_com02:57`, `snd_com02:71`) and the separate `080c` actor cue
(`snd_com02:34`).
The extractor resamples a bounded mono WAV to the same stereo 48 kHz PCM
format used by package movement audio. Sixty-three Borgs get launch
samples. `080c` instead plays one actor cue after an accepted debit and
before either launch attempt; neither child gets a substitute. Its
actor cue is joined to the exact B-shot context and checked source
call order, not inferred from the projectile.

The authored trigger for child sound is successful native launch, not the
original child's later sound request. Native gain, spatialization and playback
timing are not retail-equivalent. Add `--launch-audio` to the smoke command
above to verify mixed samples, muted playback and malformed-sample refusal.
Offscreen OpenGL with an SDL dummy audio device verified submission,
not audible playback.

### Try the opt-in close-B melee Battle Test

Add `--basic-melee-test --source-pace --role-clips --action-clips` to the
extraction (the ranged pilot can be included independently). Sword
Knight `0200` and Normal Knight `020a` use their exact normal body,
hand records 2 and 5, and close-B clip `1:1` from source frame 8.
The native test is a single stockless melee window, with authored
reach, damage, and timing. Use `--package-battle-distance 90` for the
scripted hit test, or walk close in interactive Battle Test.

Death Borg Gamma II `020f` shares the opening pose and draws its own
record 1 at joint 10 with the source-local offset `(45.18, 0, 0)`.
The source child uses the bone for position, not orientation, and
subtracts one quarter turn from the actor's roll. The pilot uses world
axes and a fixed −90° roll; it does not run the source child or claim
retail roll, visibility gates, collision, or timing.
`shared-knight-close-b.json` keeps the source program, child and
cosmetic mount evidence separate.

Axe and Hatchet Knight (`0204`, `020d`) reach that same helper through
their own installer and select `1:1` from frame 0. The native test
explicitly accepts the one uniform authored loop bit on their idle and
walk clips. It draws exact records 8 and 9 at the two indexed mounts
at rest and during attacks. No retail child growth, spin, or
collision runs. Sapphire and Ruby Knight (`0208`, `020e`) use a
different neutral-entry descriptor for their own `1:1` clip, also from
frame 0; their earlier moving close-B preview remains available. This
test does not run their ranged B, charge, or follow-up logic.
Their strict whole-body previews still refuse reflection normals.
The normal Battle Tests bind the same neutral-UV policy scoped to
Sapphire's one and Ruby's two exact reflection meshes. Tao Master
(`0801`) uses it on one mesh. This keeps their own bodies visible without
changing contact or adding child visuals. Sapphire and Ruby's record 10
and record 13 child visuals remain indexed, not mounted in Battle Test;
their original live weapon state is still unqualified.

Cyber Ninja's normal Battle Test also draws its two record-8 arm
effects at the indexed offsets on joints 10 and 6. Their continuous
counter-rotation is authored. The source child has additional
gates, record choices, and stock interactions that are not reproduced.

Elemental Knight (`0202`) and Ghost Knight (`020b`) use their neutral
`1:4` opening at frame 0. Their empty record zero supplies the rig; the
battle body explicitly assembles 15 exact static mounts, including the
hands. These are fixed attached transforms, not the retail detached-part
system. Spike Knight (`0203`) and Hammer Knight (`020c`) use their own
`1:1` opening at frame 8 and default hands. All four use the authored
close-B contact window only; their other moves remain unimplemented.

Eight Girl-family pilots share the stockless contact window: Battle Girl,
Killer Girl, Cyber Girl Super, Spinner Girl, Cyber Girl, Cyber Girl Hyper,
Kei, and Death Borg Delta III. Each uses its own exact neutral `1:0`
clip and only its own indexed hand parts. `shared-girl-close-b.json`
keeps the per-installer route, zero opening seed, and local lane joins.
Shadow Girl's exact `1:0` is absent; the extractor records the refusal
without borrowing a clip or issuing a battle profile.

The next close-B batch adds Chainsaw Knight (`0201`, `1:4` from frame 0),
Death Borg Gamma (`0206`) and Alien Insect (`0207`, their own `1:1` from
frame 8), and Death Borg Delta (`030c`, `1:0` from frame 0).
Chainsaw Knight keeps its exact record-2 hand assembly as static art;
the native contact window does not run the source saw motion or hit logic.
Wire Girl (`0301`) uses her exact `1:0` opening through the explicit
`pose-loop-window` policy. All 20 animated joints have the same loop
bit and a frame-38 end; the native 24-frame action samples without
wrapping. Removing the declaration or exceeding the exact end refuses.
Wire and other special moves remain unimplemented.

Shuriken Ninja (`0001`) uses its own seed-zero neutral `1:1` and
its exact hand records. Its Battle Test declares a neutral reflection
coordinate for 28 authored zero normals in one exact mesh, without
changing the normals or the default strict preview path. This adds
close-B only, not the ranged shuriken.

Machine Blue (`0601`), Cyber Atlas (`0609`), Proto Blue (`0617`), and
Proto Atlas (`061d`) share the reviewed neutral entry in
`shared-blue-close-b.json`: the seed at `8030CE94` is zero, selecting
program `3:0` and each package's own `1:1` from frame 0. The authored
test binds their own two hands and no fusion or ranged moves. Flame Ninja
(`0006`) uses the separate seed 5 and local `1:1` from frame 0;
its default hand records 1 and 4 are authored empty, so the test draws
the exact body without replacing either hand record.

Thirteen more normals use the authored stockless close-B window:
Claw Robot, Build Robot, Magnet Robot (+), Megaton Robot, Army Robot,
Magnet Robot (−), Isaac, Death Borg Omega II–IV, Jelly Diver, Tar Diver,
and Jack. `shared-robot-diver-close-b.json` retains their own `1:1`
bindings at frame 0: Robot paths seed program `3:0`, while Divers seed
`3:1`. Only the Robots' exact uniformly flagged walk clips use
`walk-loop=authored-bit`; their attack clips remain strict. Build and
Megaton use their own static left-hand record 4. The Magnet packages
stay separate; neither attraction nor repulsion is bound by this test.

The eleven Wing/Sigma packages (`0a00`–`0a0a`) and eight
Valkyrie/Lambda packages (`0b00`–`0b07`) now have normal-only grounded
close-B pilots. Each uses its own `1:1` pose. Beam Wing Blue and Red use
selector 45 and start at frame 10; the other neutral rows use selector 44
and start at frame 0. Demon Wing and Death Wing read
different source dispatch tables, but their neutral leaves both pass
seed 3 and select the same local descriptor; Death Wing's seed-zero
`1:2` is not the checked opening. Demon Wing and Wind Valkyrie mount
their own static hand hierarchies. These tests do not enable hovering,
aerial attacks, special weapons, or full retail movesets.

Drill Robot (`0401`), Hammer Robot (`0403`), Thunder Robot (`0404`),
and Death Borg Omega (`0406`) add their own neutral `1:1` openings to the
Robot index. All four need the exact flagged walk loop; Drill alone also
needs its flagged idle loop. The same standalone contact window admits
Time Bomber (`0106`) with `1:0`, and Death Borg Beta (`0109`) and Tau
(`0807`) with their own `1:1`. The Bomber/Hero index preserves the exact
neutral seeds 0, 2, and 0. Remote Bomber (`0107`) also ships its `1:0`
opening after its body was admitted with `body-material=immutable-highlight`:
its only refused mesh carries the reviewed `0x00030082` stage, and the
shared draw path already applies the original `STQ=(1,1,0)` UV. The
screenshot check shows the body and both stockless contacts. No bomb
detonation, Thunder Wire, or other special attacks are bound by these
tests.

`shared-grounded-close-b.json` adds six exact normal openings:
Imperial Knight (`1:1`, frame 8), Barrier Girl and Command Girl (`1:0`),
Cyber Hero (`1:30`), and Pop Honey and Death Borg Iota (`1:6`).
Imperial's two neutral seed branches select the same clip and start;
the Girl profiles do not equate the alternate ranged context with the
close row. Cyber Hero's later `3:1` request is separate from its one-frame
hold; both Honey-family tests use the exact later `3:2` request.
No native guard, barrier, projectile, or original combo is implied.

Powered Gunman (`0101`) and Billy (`0108`) use their own matching-lane
`1:1` attack selection after the split-lane lead-in in program `3:0`.
Star Hero (`0804`) uses the later `3:1` request and its local `1:1`,
not the one-frame `1:0` hold. The authored tests start at that later pose;
they do not simulate the original transition or hit clock. Powered
Gunman's exact reflection mesh uses the neutral-UV policy on its 18
authored zero normals. Tank B routes are firing paths, not inferred
melee.

Normal Tank (`0c00`), Gatling Tank (`0c01`), ICBM Tank (`0c05`), and
Leopard (`0c06`) have normal-only single-shot tests. Near and far B
both lead to a verified paid type-0 firing phase. Each recipe keeps
the exact `0:25` idle and `0:26` walk and samples its own muzzle without
an attack animation override. The test uses one indexed barrel per
press and the exact `snd_com01:6` launch cue with the audio opt-in.
`shared-tank-b-shot.json` keeps the other qualified factory call sites
and keys separate. Native timing, aim, damage and refill are authored;
retail bursts, barrel selection, turret tracking, and the ICBM special
are not implemented by this test.

Titan Tank (`0619`), Victory Tank (`061f`), and Victory Machine (`0625`)
add three normal-only paired B-shot tests, bringing Battle Test to 166
of 206 retail Borgs before the Beam Gunner result below. One channel-0 debit gates two ordered muzzle calls.
Titan uses type-2 keys `79/80`, joints `25/23`, and `snd_com02:57`;
Victory Tank uses type-0 keys `87/88`, Victory Machine uses `91/92`, and
both use joints `25/27` and `snd_com01:6`. The distinct routes are recorded
in `shared-paired-ground-b-shot.json`. The tests retain the indexed local
locomotion and use authored timing, projectile behavior, and refill.
Close B, source phase timing, transformation, and fusion are out of scope.

Beam Gunner (`0104`) adds a normal-only single shot from type-2 key `11`
on joint `7` with an empty local point, the exact channel-0 debit at
`80103554`, and `snd_com02:57`. Its indexed idle, walk and air poses bind,
but the source ground-dash pose refuses strict binding on
`AnimJoint/constraints/flags`. The test therefore declares
`dash-policy=disabled`: both ground and airborne dash inputs refuse rather
than borrowing another clip or fabricating a pose. The recipe keeps the
mode-1 `add-on-debit` stock clock and uses authored timing, projectile
behavior and refill. `shared-single-ground-b-shot.json` records the release
entry, muzzle and declined inputs. X, charge shots, forms and source
phase timing are not implemented.

Gold Hero (`0802`) adds a second reviewed single shot: type-0 key `45` on
joint `12` at `(0,-10,50)`, the exact channel-0 debit at `8013D74C`, and
`snd_com01:6`. Its class installer also serves the temporary form, so the
record choice was previously left unresolved. The installer compares
actor halfword `3E8` — the same model field the class handlers test — with
`0803`; only that form takes the `+1B0` record and alternate movement
table. For an ordinary `0802` actor the branch is not taken, which selects
record zero and its `12.0` walk operand. `080a` uses the same field with a
different shape and returns before its form pointer. A symbolic
`bclr`/branch resolution of that single model-ID comparison now records
these two Borgs as one static record. `0f05`/`0f06` are resolved the same
way through the reviewed fusion-slot constant described above. Gold Hero's
muzzle dispatch is likewise model-gated: form `0803` requests type-2 key
`34`, and other models fire nothing. The pilot keeps its bound idle, walk,
air and dash poses and a mode-0 stock. Source phase timing, X, the form
change and its expiry are not implemented. With Carrier Helicopter below,
Battle Test covers **169 of 206** retail Borgs.

Carrier Helicopter (`0d02`) has no motion bank, so it uses the existing
static-body policy: `body-motion=static-grounded` with no borrowed
locomotion. Its ordinary B is one paid channel-0 debit at `8013642C`
followed by a one-bit barrel byte at `14C`. The class initializer clears
that byte at `80135FF8`, so the first barrel is type-0 key `18` on joint
`8` at `(34.43,0,57.15)` with `snd_com01:6`; key `19` is the mirrored
alternate and stays indexed, never fired by this test. The phase chain
`80338C44` and the alternating toggle are pinned, but repeat alternation
is not implemented. Its source-relative pace comes from the alternate
static-body policy (operand `02C`, `15.0`). Native timing, aim, damage
and refill are authored.

Death Eye (`0f06`) shares its release leaf with another model: the
equality branch requests type-0 key `122` on joint `1` at `(0,0,30)` with
`snd_com01:6`, and the fall-through key `123` belongs to that other model.
The verifier pins both calls and the model compare, so only its own key is
fired. It is static, pays one channel-0 debit at `801E4734`, and declares
an authored `6.00` because its movement type has no source-relative pace
policy. Its actor record is now resolved through the fusion-slot guard
above, so record `0` is used for the standalone actor. Retail timing, aim,
damage, refill, X and fusion are not implemented.

Red Attacker (`0d01`) is also static. Its ordinary B pays one channel-0
debit at `8018940C`, then releases two ordered type-2 calls: key `54` on
joint `3` and key `55` on joint `4`, both at `(0,0,100)` and
`snd_com02:57`. The phase chain `80362680` and both call sites are pinned
through the ordered-pair rule, so one press emits two projectiles behind
a single debit. Its movement type (`3`) has no source-relative pace
policy; because the body never walks, the profile declares an authored
`6.00` and records that explicitly. Retail timing, aim, damage, refill and
X are not implemented. Battle Test now covers **171 of 206** retail
Borgs.

Ultimate Cannon (`0c04`) adds a four-shot B volley. One debit on the
source's alternate ammo channel (`r4=2` at `800B194C`) gates four ordered
type-2 calls: keys `4` and `5` from joint `5` at `x=-84.3` and keys `6`
and `7` from joint `6` at `x=84.3`, separated by `y=+-97.5` with
`z=230.9`, all `snd_com02:57`. The engine's ordered-release rule carries
all four behind the single debit. No dash role binds, so both dash inputs
are declined; idle `0:25`, walk `0:1` and air `0:24` are exact, and the
source-relative pace is `1.67`. Retail timing, aim, damage and the channel
distinction are not reproduced: the test spends one authored stock unit.

Death Borg Delta II (`0303`) is the first X-only pilot. Its source B has no
release, but its X group pays one channel-2 debit at `8010D098` and calls
type-0 key `59` on joint `7` at `(-50,0,0)` with `snd_com02:58`. That leaf
is shared with `0301`, `0302` and `0304`; the model compare chain is
pinned so only `0303` reaches this call, and the `+1CEF` sign guard is
recorded but not reproduced. The profile declares `move=x-shot|x|...`, so
the scripted showcase's existing frame-41 and frame-126 X presses fire it
while B does nothing. The HUD now reports `B NOT IMPLEMENTED` for such a
profile instead of always advertising B. Both dash inputs are declined
because the forward dash pose keeps its `aobj-flags` refusal.

Beam Tank (`0c02`) is the second X-only pilot and the first with a bound
dash. Its source B again has no release; the X group pays one channel-2
debit at `80106050` and then fires type-2 keys `8` and `9` from turret
joints `9` and `10` at `(0,0,100)` with `snd_com02:57`, both behind that
single debit. Idle `0:25`, walk `0:26`, air `0:24` and dash `0:2` all bind
exactly, and the source-relative pace is `3.75`. The parallel airborne
release leaf is indexed but not bound. Battle Test now covers **174 of
206** retail Borgs.

Cyber Death Dragon (`0503`), Machine Dragon (`050f`), Cosmic Dragon
(`0504`), Space Dragon (`0510`), Cyber Dragon (`0506`) and Mechanical
Dragon (`0512`) use the engine's **sustained beam** for an authored
breath (drawn with `look=breath|160|255|150|60`). Their shared neutral B callbacks pay one channel-0 debit
(`800B93AC` for the `0503`/`050f` pair, `801091D0` for `0504`/`0510`,
`800B8CDC` for `0506`/`0512`) and then call the program driver
`8006D144`; no origin factory, socket or area row is reached anywhere in
those leaves. The pilot binds that exact debit, the neutral pose where the
source requests one (`0:1` for `0503`/`050f`, none for the other four),
idle `0:0`, walk `0:1`, air `0:7`, the reviewed `27-31` tail, and its
source-relative pace doubled like the elemental cohort. The authored beam
is `length 1500`, contact radius `60` (the engine's cap; `look=160` carries
the drawn width), damage `6` every `8` ticks, lifetime `30` with
`ignore-floor`, anchored at author joint `0` with local `(0,40,60)`. The
beam is horizontal, matching the retail ground form, so no pitch or
`aim-lock` is declared; the low anchor is what lets a level ray cross a
shorter recipient's band. `activation=…|release-window|24|90`
makes B a charged release: the scripted showcase now holds one tick past
the minimum. The retail shape is broader than this proxy: on the ground
these Borgs fire a horizontal beam whose contact volume is larger than the
caster; in flight the Dragons switch to a sweeping beam that leaves the
origin at a fixed angle, and the two mechanical Dragons instead fire
straight down from the chest. The engine's `beam-sweep` expresses both:
the demo passes the qualified source grids through
`--source-ground-assets`, so `clip-on-stage` binds, and the six non-mechanical
Dragons now also carry the airborne sweep (below). The source charge gauge fills toward the stock
requirement while B is held and the skill fires on release; the profile
uses the release window plus the stock minimum and spends one unit on
release, and does not model losing part of the gauge on an early release.
Length, cadence, damage, radius and caster joint are native tuning, not
retail breath values, and the caster joint is authored rather than an
indexed socket. `shared-dragon-breath.json` records the window and the
authored beam.

Ultimate Cannon (`0c04`) keeps its four-missile B volley and adds an
authored charged **X** beam, matching the user-observed retail shape of a
beam larger than the caster, fired horizontally on the ground. The source
X-charge context is exact: state `3`, selector `44`, class callback
`800B0F18`, pose `4:0`, and program requests `800B1024` / `800B10B8`.
That leaf reaches no origin factory or paid release, so the beam — length
`1800`, contact radius `60`, `look=180`, damage `10` every `6` ticks,
lifetime `40`, stock cost `2`, hold window `20-90` — is authored. It is
horizontal with no pitch. In the scripted showcase the first-round X hold
reaches the charge and fires; the second round's hold overlaps the volley,
so that charge is dropped.

The dragon-family X group is settled: callback `800B9D2C` dispatches on the
actor model through table `802FF3FC` and only sets a per-model mode
(`0506` = 4, `050f` = 8, `0512` = 12). It is a stance switch, not a shot, so
the authored beam stays on B where the paid program window lives.

Cyber Dragon (`0506`) and Mechanical Dragon (`0512`) therefore also carry
an authored **airborne** B action: a downward chest beam
(`projectile=dive|beam|1|30|-100|0|0|60|25|40|stop-on-floor`,
`origin=dive|12|0|0|0|fixed`, `move=breath-dive|b|11|airborne|40|none`,
`release=breath-dive|8|breath-stock|1|dive`). It starts at the chest joint
and retires on the floor, matching the user-observed "flying beam straight
down to damage everything below". Damage, speed and lifetime are authored.
The scripted mobility pass jumps and presses B in the air, and the check
asserts the launch, the debit, the chest socket and the floor retirement —
not a contact, because nothing stands directly below the scripted jump.

The other six Dragons' airborne form is a fixed-angle sweep and now ships:
`projectile=sweep|beam|1|1|0|0|0|60|40|8|clip-on-stage`,
`sustained-beam=sweep|2400|6|0`, `beam-sweep=sweep|1200|300|180`,
`origin=sweep|0|0|40|60|fixed`, `move=breath-sweep|b|11|airborne|60|none`,
`release=breath-sweep|8|breath-stock|1|sweep`. The demo already passes the
qualified source grids through `--source-ground-assets`, so `clip-on-stage`
binds on the retail-floor stage.

The earlier "no contact" result was a geometry bug in the experiment, not an
engine or content problem. The sweep velocity includes a vertical component
from `drop`; with `drop=1` the beam sat at `y=193` while the target's
contact span tops out at `actor.position[1] + actor_height + radius = 160`,
so `package_segment_contact` correctly rejected it (`t=32.9 > leave=1`).
With `drop` from `200` to `600` the descending segment crosses that span and
contacts: the trace records `stage-impact`, `hit recipient=1 value=8`, and
the same on the authored open arena and the demo stage. Since the airborne
pose leaves the chest socket unresolved (a joint-12 launch landed at
`y=1359`), the sweep is anchored at the root joint with local `(0,40,60)`.
Sweep angle, length, travel, radius and damage are authored tuning, not
retail values.

The same opt-in adds Normal Samurai, Demon Samurai, Vampire Knight,
Sonic Samurai, Samurai Shogun, Beam Samurai, Chrono Samurai, Akuma
Samurai, Musashi, and Vlad.
`shared-samurai-close-b.json` joins the separate installer routes to the
shared neutral request: each has a zero source seed selecting local
`1:1` from frame 0. The existing `1:18` moving candidates stay in the
preview. All ten use their own exact hand records at logical bones 7
and 11, and the explicit supported idle/walk loop bit. Beam Samurai's
zero seed is limited to the uncharged `+6F6 <= 0` branch; its positive
branch, charge and blade effects are not run. Sonic Samurai's ranged
and X actions also remain unbound. Close-B contact, timing and damage
remain authored.

The four Zeta packages (`0705`, `070b`, `070c`, `070d`) get no executable
profile, and material is not their only blocker. They also have an empty
`model_parts.default_candidates` table, so the shared close-B route has no
hand records at logical bones 7/11 to bind; `0705`/`070b` additionally use
the unmapped installer tuple `80373920` with callback `8019CA64`, while
`070c`/`070d` share the Demon tuple but still lack the parts. A probe that
added the validated diffuse-overwrite-pair material policy and mapped
`070c`/`070d` to the Demon tuple stopped at "Samurai exact hand records
unavailable", so the policy change was reverted rather than shipped
unused.

Teleport Ninja `0005` and Switching Ninja `0009` also have an opt-in
neutral close-B test. Their shared source route seeds program choice
`1`, which selects each Borg's exact `1:1` from frame 0. Hand records
1 and 4 remain attached to bones 7 and 11. The previous moving
`b-close-c2-s0` preview stays separate. The test is stockless melee
only: neither ranged B nor either X relocation mechanic is bound.

### Try Gatling Gunner's authored held B

With `--basic-shot-test`, Gatling Gunner `0102` now has a separate held-B
profile. It uses exact local shot clip `3:11` (the middle of the three
indexed directional candidates), the joint-7 type-0/key-29 muzzle,
and level-1 stock capacity 100. The source check-only repeat gate is
kept separate from the paid release. The native cadence and refill are
authored, and each accepted allocation can play exact `snd_com01:6`.
Releasing B stops the native cadence. X grenade and retail aim or
timing remain unimplemented.

### Use the projectile-origin index

`shared-projectile-origins.json` indexes the bounded type-0, type-2, type-14,
and type-15 data tables. It preserves the exact local point and signed joint
selector, and distinguishes the credited actor, parent, retained actor and
root-matrix sources. A negative selector is not silently replaced by joint
zero. Known extra local/world-offset branches are tracked separately.

`shared-projectile-release-links.json` groups the discovered references
by exact normal Borg, factory site, family and key. Each entry retains
its installer context, action group, phase scope and optional relay path.
Several rows in a release path stay separate; they do not establish
simultaneous emission, launch bearing or accepted fire timing. This file
is an offline review index, not a runtime package rule.

Add `--origin-markers` to a fresh extraction to expose joint-local points
for the **exact normal** looks. Then select a factory/key ID listed in
that package's preview profile, for example:

```sh
/path/to/gf_native --preview-borg gg4e08:0600 --borg-variant normal \
  --preview-origin type-2-21 --preview-clip ground-travel
```

`--preview-clip` is optional and requires that clip to be declared (the
example uses `--role-clips`). The marker follows the displayed pose, not a
reconstructed firing pose or aim vector. A root-matrix case, missing joint,
mismatched parent, or runtime offset is left in the review index and not
shown as a guessed point. Alternate looks do not inherit markers.
The three-frame headless sweep submitted **109 of 110** distinct markers
across **72 Borgs**. `0801` type-0/key `125` hit the existing strict
reflection-normal refusal. Two complete marker-enabled extractions were
byte-identical.

The per-Borg index links the already reviewed ordinary B rows for Sasuke,
Machine Red, and G-Red and cross-checks each logical joint against the
exact normal rig. It also records the ten type-15 rows with explicit
parent-model restrictions, without interpreting that check as an input
or setting a release policy. No row is assigned by similar names or models.
The offline origin extractor does not run original code and emits no
executable weapon rules.

### Export a dry sound sample

`shared-projectile-sfx.json` preserves **263** sound operands for the
reviewed type-0, type-2, type-14 and type-47 factories. Type-0 reads its table
halfword at `+34` hex during the child's first phase, using either the
credited actor or the parent as the sound owner. Type-2 reads `+40` hex
from its table during the child's first phase after its resource check,
using the projectile child as the owner. Type-14 requests literal `8`
with the factory caller after successful allocation. The six type-47
carriers share a literal `199` request (`snd_com02`, tone `71`) through
`800F061C`, at `800BEDFC` in the first child phase after the attack
collision-group bind succeeds. It's a child cue, not an actor B-press sound. These are separate
from the attack selector, aim, origin, hit and damage rules.

The index checks the original factory and consumer code, and resolves IDs
against the **three banks installed by the normal loader** using the
archive member triples. `261` of the operands resolve to exact tone
records; two signed `-1` values are retained without a tone. The fourth
exported bank is not a normal-loader fallback. The release index adds
`sfx_candidate` by exact factory family and key to **136 of 137** release
links; it retains the action contexts. Only the `0102` X type-15 link has no
qualified sound route. For example, the reviewed Machine Red type-2
key `21` points to sound ID `185` (`snd_com02`, tone `57`), not tone 185
in `snd_com01`. No button-press timing, audible playback or other
projectile families' sound routes are established by this join.

`shared-action-sfx.json` separately records **15 actor-owned sound links
across 13 Borgs**, with exact action group, installer context, handler/phase
and call site. For example, the paired `0804` and `080c` B-shot helper
branches both request sound ID `162` (`snd_com02`, tone `34`). Only `080c`
requests the indexed type-2 keys `99` and `100`; their child tone is a
separate request. All **nine** distinct actor tones decoded to identical
44.1 kHz WAVs on a repeat export. The index is not a cue scheduler and
does not imply that an actor and its child play sounds at the same time.

`retail_borg_sound.cpp` uses the engine's shared bank and DSP decoder. The
offline helper outputs one mono 16-bit WAV at the authored sample rate. It
refuses stop/randomized and multilayer tones rather than inventing a single
sample. A WAV is one dry pass, not the original gain, voice, loop policy or
event timing. The type-47 tone decoded to **57,655 mono frames at 44.1 kHz**
and matched byte-for-byte on repeat export. The opt-in Dragon pilot can
bind this dry sample after accepted native allocation.

```sh
c++ -std=c++20 -O2 -I. tools/extract/retail_borg_sound.cpp -o build/retail_borg_sound
(
  set -o noclobber
  build/retail_borg_sound local/my-retail-roster/borgs/shared-snd_com01.tsb \
    local/my-retail-roster/borgs/shared-snd_com01.chd \
    local/my-retail-roster/borgs/shared-snd_com01.dpk 15 \
    > local/my-retail-roster/landing-tone-15.wav
)
```

The shared landing lookup at `800F07E8` joins family/member to the selector
table, then uses the low seven bits to resolve the common tone. Across the
206 normal collection IDs, the static selections are **15** (166 Borgs),
**18** (31), and **22** (9). The selector's high bit marks the alternate
gain dispatch and is retained separately. Both dispatch paths are conditional
on reaching the original shared landing arm; the extractor does not bind
them as live cues. All three dry samples decoded at 44.1 kHz and matched
byte-for-byte on a repeat export. Stop tone 41 refused.

For **provisional solo landing audio**, build the existing PCM exporter and
pass it to a fresh `--role-clips` roster extraction:

```sh
c++ -std=c++20 -O2 -I. tools/extract/borg_movement_pcm.cpp -o build/borg_movement_pcm-roster
mkdir -p local/my-audio-roster
python3 -B -m tools.extract.retail_borg_roster --role-clips \
  --landing-pcm build/borg_movement_pcm-roster \
  /path/to/catalog.gfbc /path/to/main.dol /path/to/afs_data.afs local/my-audio-roster/borgs
```

Launch the walk lab with `--lab-jump --lab-landing-audio`. The extractor
pins the common bank, converts each of tones 15, 18 and 22 once, then
publishes only the selected sample inside each jump-mapped normal package.
Version 4 names the local PCM; silent previews and other game modes do not
load it. A missing exact declaration refuses instead of using another
look or a generic sound. `--no-audio` leaves the device closed.

The native mixer starts the one-shot on the accepted grounded transition.
The native volume, stereo duplication and resampling are presentation
choices, not original gain, trigger guards, pitch or spatialization.
The **181** jump-mapped normal packages use the same three source tones.
The headless smoke checks one cue each: **143** using tone 15, **30**
using tone 18, and **8** using tone 22. Two audio-enabled extractions
were byte-identical. Offscreen SDL
with a dummy audio device accepted PCM on all 12 Dragon jump tests;
audible playback was not verified.

## Mixed Workshop roster check

With the converted normal packages, authored Flight Stick, and
[Workshop stage](portable-content-v1.md#reusable-battle-test-rosters)
installed in one local content root:

```sh
python3 -B -m tools.analyze.native_roster_matrix \
  /path/to/gf_native /path/to/content-root
```

The audit sorts the 206 stable IDs and puts up to five at a time in CPU
slots with an idle Flight Stick in P1. It requires each fighter's body
to submit geometry at tick zero, then runs the same preset for 120
headless frames. It writes temporary presets only and prints a JSON
report with the exact groups, the IDs that started a move, and the IDs
with a positive `hit` or `blast-hit` event on an opposing slot.

To compare two cutoffs without replaying the opening, add
`--frames 240 --checkpoint 120`. The report records first move, first
damaging enemy hit, and KO ticks, as well as the accepted simulation
ticks and outcome for each group. A round can stop on a KO before its
render frame budget is exhausted. A quiet subject is counted separately
if it was defeated, survived an ended round, or was still fighting at
the cutoff. For a targeted run, `--trace-package-battle` also prints the
final HP, position, planar travel, nearest rival and aimed rival for each
explicit roster slot.

For subjects that start a move without damaging an enemy, the audit
also separates those with a recorded payload from those without one.
Payload events are accepted projectile emissions, armed melee
windows, placed fields, triggered blasts, emitter shots, or walking
pulses. First stage-impact and interrupt ticks are separate observations;
they do not prove why a particular move missed.

All **206 of 206** passed preflight and ran the 120-frame budget across
**42** presets. Allowing a normal dash on sufficiently long,
clear detour legs raised the number of CPUs starting a move from
**110 to 150** in the same fixtures; none of the original 110 stopped
starting. The other 56 are not admission failures. This counts starts
in these particular matchups, not hits, every B/X route, later combat,
or original game behavior.

After adding cruise routing and boost, all 206 still passed. The same
120-frame sweep saw **149** move starts: `0e01` added one, while `0e02`
and `0e04` started after the cutoff (at ticks 132 and 135). In a
240-frame check of the two affected groups, `0e01` and `0e05` both
placed and landed their fields, where neither started before the
change. This favors reaching opponents around the wall over maximizing
opening starts.

Height-aware rival choice kept the 120-frame totals at **206 admitted,
149 move starts**. In a separate mixed-height Workshop fixture, the
converted Normal Samurai (`0700`) hit the lower rival at tick 55 after
switching away from a closer cruiser at height 500. The previous
planner landed no hit in 100 frames in the same fixture. This is a
native CPU decision, not a recovered retail targeting rule.

Using the declared dash speed and native dash duration for direct
approaches raised the 120-frame totals from **149 to 153 move starts**
and **135 to 138 subjects with a damaging enemy hit**, with no previous
starts or damaging hitters lost in those fixtures. All 206 still
passed. In a separate 330-unit approach, converted Army Robot
(`0408`) hit at tick 35; the previous planner did not hit within 78
frames. These are native AI results, not retail timing measurements.

Local ally avoidance kept the same 206 admissions, 153 starts and 138
damaging hitters. In a dedicated same-side fixture, converted Claw Robot
(`0400`) stepped around a stationary ally and hit at tick 155. The
previous planner got stuck behind the ally and did not hit within 175
frames. A separate authored fixture verifies both side routes and the
case where neither flank is passable.

Read-only stock checks for paid field-only attacks kept the same
206 admissions, 153 starts and 138 damaging hitters in the 120-frame
sweep. In a separate 260-frame close-range fixture, `0e00`, `0e05`
and `0f03` each placed two fields before and after the change. Each
previously made six empty-stock attempts; the updated CPU made none
and waited for the actual stock refill before its second start. The
native debit marker and refill timing were not changed.

Allowing a grounded owner-following field charge to start on a clear
approach kept the same 206 admissions, 153 starts and 138 damaging
hitters in the sweep. In a separate 350-unit fixture, Acceleration
Ninja (`0004`) started its charge at tick 1 and hit at tick 10, down
from ticks 25 and 34 with the old planner. This uses the native charge
and field timings; it is not a retail range or timing claim.

The field height check also kept the sweep at **206 admissions, 153
starts and 138 damaging hitters**. Against a closer cruiser at height
500, converted Nu III (`0f03`) kept its field stock when no enemy was
in reach. When a lower rival stood inside the field, it paid and hit
that rival. The previous planner paid in both cases. This did not
change either aim selection or the live area damage rule.

### 240-frame follow-up

All 206 passed preflight and each preset rendered its complete frame
budget. In one match, team 0 knocked out the last opponent at tick
131, so its simulation stopped before frame 240. Using the same runs
for both cutoffs:

| Accepted-tick cutoff | Started a move | Damaged an enemy | No start, defeated | No start, survived an ended round | No start, still fighting |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 120 | 153 | 138 | 0 | 0 | 53 |
| 240 | 178 | 172 | 5 | 0 | 23 |

For example, Cosmic Dragon (`0504`) and Space Dragon (`0510`) both
first started at tick 121 and first damaged an enemy at tick 133.
The shorter run missed those starts, not their admission or their
move support. The 23 still-fighting subjects need more specific
matchups before their inactivity can be attributed to a cause.

The baseline's 23 quiet survivors had 20 melee-only profiles, two
cruise/field profiles, and one grounded field profile. That split
describes their authored moves, not the reason they stayed quiet.

Letting simple field CPUs pursue a lower rival kept both audit cutoffs
unchanged. In a separate test, converted Nu III (`0f03`) left a
closer cruiser at height 500 and closed on a lower rival 240 units
away. It started at tick 45 and hit at tick 54. The previous planner
did not start or land a move in 100 frames in that fixture.

Applying ally avoidance to routed legs as well as direct approaches
raised the 240-tick totals to **179 starts and 173 damaging hitters**,
with 22 still-fighting quiet subjects. Sonic Samurai (`0703`) and Ring
Valkyrie (`0b04`) started by that cutoff after neither had started
before. In a separate test with a parked ally directly on the first
wall-route leg, Proto Blue (`0617`) hit at tick 113 and Army Robot
(`0408`) at tick 309. The previous planner hit with the leg clear but
did not attack within 320 frames with that ally in place. The ally and
wall remained solid throughout.

Separating movement clearance from melee visibility raised the
240-tick totals to **183 starts and 175 damaging hitters**, with 19
still-fighting quiet subjects. A legal corner strike no longer forces a
detour just because the Borg's body would not fit. In a corner fixture,
Magnet Robot (−), `0409`, hit at tick 10 instead of tick 23. Both `0407`
and `0409` also landed hits in the full sweep after remaining quiet
through the previous cutoff. A strike whose line crosses a `|melee`
partition is still blocked.

Comparing the lengths of blocked approaches raised the 120-tick
totals from **153 to 161 starts** and **138 to 144 damaging hitters**.
By 240 ticks, the totals were unchanged at 183 and 175; all 206 still
passed. In a two-route fixture, Proto Blue (`0617`) took the shorter
way around the wall and hit at tick 104 instead of tick 149. The
nearer body remained available to the ordinary contact system.

The clear-route preference now has one exception: a valid detour to
the nearer rival can win if it is less than half the clear walking
distance. In a separate fixture, Proto Blue hit the nearer rival at
tick 50 instead of taking the long straight chase and first hitting
at tick 116. The full sweep held at **161 starts and 144 damaging
hitters** by tick 120 and **183 starts and 175 damaging hitters** by
tick 240, with all 206 admitted. A broader comparison that reduced
activity in the full sweep was not kept.

Allowing a planted roster CPU to turn toward its movement input when
a solid peer blocks translation kept the 120-tick results unchanged.
At 240 ticks, starts rose from **183 to 188** and damaging hitters
from **175 to 180**, with 14 still-fighting quiet subjects instead
of 19. All 206 still passed. A controlled contact test stalled with
the old mover, then landed a strike with the new rule without moving
through the rival. The same fixture kept local facing unchanged.

Allowing a checked boost to continue past a short corner leg raised
the 120-tick totals to **164 starts and 147 damaging hitters** and
the 240-tick totals to **191 starts and 182 damaging hitters**. All
206 still passed, with 11 quiet subjects still fighting at tick 240.
Sirius (`0e00`) first hit at tick 108; `0e03` first hit at tick 71.
Neither had started by tick 240 in the previous sweep. In isolation,
Sirius also hit at tick 108, where the previous planner covered only
96 units in 240 ticks without attacking.

The same corner extension is available for a slow CPU's forward
ground dash when it replaces a long walk. In the targeted fixture,
Army Robot (`0408`) hit at tick 51; the previous planner started at
tick 138 and never hit in 240 frames. The narrower rule kept the
240-tick totals at **191 starts and 182 damaging hitters**, with all
206 admitted. At tick 120 it had **163 starts and 147 damaging
hitters**, one fewer start and the same hit count. The broader
ground-dash rule was not kept because it reduced early and late
activity.

As a separate 480-frame spot check of three quiet subjects in their
ordinary audit groups, Army Robot (`0408`) first started at tick 280
and hit at tick 289. Normal Samurai (`0700`) was still fighting
without a start, and Nu III (`0f03`) was defeated at tick 420 without
a start. Extending the window separates some late attacks from the
cases that need a closer look; it does not explain every quiet slot.

In a narrow corridor with an off-center ally, the shorter sidestep
probe let Proto Blue (`0617`) reach the enemy and hit at tick 45.
With only the full-width probes, it stood still for the entire
120-frame fixture. The full Workshop sweep was unchanged at both
cutoffs, with all 206 admitted and 191 starts and 182 damaging
hitters by tick 240.

In a separate three-fighter pursuit fixture, Normal Samurai (`0700`)
switched from the faster Proto Panther to a reachable parked enemy,
started at tick 31 and hit at tick 40. The previous planner kept
following the runner and never attacked in 160 frames. This fallback
did not change either Workshop sweep cutoff or resolve the original
six-fighter Normal Samurai case, which was still quiet at tick 480.

With the early-dash check for a clear runner, the original six-fighter
Normal Samurai fixture started at tick 150 and hit at tick 159. In a
separate fixture with Proto Panther as its only enemy, Normal Samurai
first hit at tick 112; the previous planner never attacked in that
240-frame test. These are native pursuit results, not changes to the
move's entry range or contact rule. All 206 profiles still passed.
The 120-tick totals stayed at 163 starts and 147 damaging hitters;
at 240 ticks the totals rose to **192 starts and 183 damaging
hitters**, with 11 quiet subjects still fighting.

In longer spot checks of the projectile users without an enemy hit
at tick 240, Death ICBM (`0d06`) first hit at tick 313. `0602`,
`0d07`, and Roach (`0f05`) still had no hit at tick 480. All four
emitted projectiles and none logged a stage impact in those runs.
In a separate fixture beginning 550 units from a parked foe, Roach
triggered six blasts without hitting either enemy. That gives a
concrete drop-range case to investigate rather than blaming the
Workshop wall for these misses.

The enhanced audit kept the same 206 admissions, 192 starts and 183
damaging hitters by tick 240. Of the nine that started without
damaging an enemy, four emitted projectiles. The other five recorded
an interruption and no payload. None of the nine logged a stage
impact. This is a breakdown of the trace, not a gameplay change.

Using a finite CPU approach range for slow land-and-expire drops let
Roach (`0f05`) hit at tick 114 in the same 550-unit fixture where its
six earlier mines missed. In the mixed sweep Roach did not start by
tick 240, rather than deploying a mine without a hit. All 206 still
passed: 162 starts and 148 damaging hitters by tick 120, and 191
starts and 183 damaging hitters by tick 240. Both ICBM variants kept
their previous openings. The change is to CPU approach and timing,
not to landing, blast reach, or stock rules.

In a controlled resident-field fixture, Star Hero (`0804`) took five
field hits instead of nine after its melee windup was interrupted.
It resumed melee after the field expired and hit at tick 117, versus
tick 99 without the retreat. The Workshop sweep kept its totals at
both cutoffs, with all 206 admitted. The five interrupted melee
subjects still deployed no payload by tick 240 in that mixed sweep.

With a clear enemy on the safe side of the same field, Star Hero can
now leave the interrupted fight and take that approach. It hit the
alternate rival at tick 111 while the field was still active; the
previous planner waited outside and did not hit that rival in the
136-frame fixture. The hint does not change field occlusion, actual
recipients, or damage. All 206 still passed and the Workshop sweep was
unchanged at both cutoffs: 162 starts and 148 damaging hitters at
tick 120, and 191 starts and 183 damaging hitters at tick 240.

## Baseline check

Two independent full extractions from the local inputs were byte-identical.
The original baseline passed 169 headless previews. The shared package
preview now also accepts AObj descriptors carrying **only** authored loop
bit `0x20000000`, while preserving the bit on each bone. Static inspection
of `80243C34` confirms the flag selects the original end-time wrap path.
This change enables in-range track sampling only. Preview playback keeps
its own clock; the strict gameplay/actor clip loader still refuses the flag.

```sh
python3 -B tests/host/retail_roster_preview.py \
  build/portable-content-v1/gf_native local/my-retail-roster --frames 3 \
  --report local/my-retail-roster/native-preview-results.json
```

The native build discovers **206** with **zero** rejected packages.
The earlier load-and-UI check counted **203** normal previews before the
mode-2 material change, but did not require a body draw. The loop-flag change
recovered 16 load paths; record-scoped skin validation recovered 14 more. Body preview
validates the decoded geometry, material and influences of the selected body
record. An unsupported shape or skin link on an unused secondary record no
longer vetoes that preview. The strict whole-bank skin loader and selected
secondary shape still refuse. Four more previews use the existing
validated immutable diffuse-overwrite rule: the second texture fully
replaces the first texture's RGB at weight one, uses its authored second
UV set and leaves prior alpha alone. Other texture chains are not dropped
or approximated. This is body inspection, not assembly of attachments,
visual comparison, or gameplay.

The archive, rig and extractor now distinguish a required published root
relocated to data-block offset zero from a missing joint. That recovers exact
rigs for Elemental Knight (`0202`) and Ghost Knight (`020b`), whose record
zero contains no drawable body. The generic model-part rows only expose
the two hand candidates. Static inspection of their shared initializer
(`80123438`, setup at `80123534`) found the separate eight-child creation
loop at `801235A4`. Child setup reads the model-record pairs at `803327C8`,
and the neutral follow path uses logical joints and local translations at
`803327E8` and `803327F8`. The first child has no second record. Three
paired children reuse the same record at distinct left/right mounts.

The extractor emits the **15 fixed mounts** as `GFBORG-PREVIEW 3` for each
look, after checking the records and rig in that exact wrapper. The shared
body evaluator mounts each subtree at the current logical joint matrix,
keeps authored child transforms, and draws the selected skin and textures
without modifying the source assets. This is the attached target pose,
not the original matrix blend, detached-part behavior, style callbacks,
or a claim about live visibility. Both complete extractions were
byte-identical. All 12 looks and their 12 normal movement-role previews
submitted body geometry.

`0107` normal and `source-1` retain an immutable, clamped mode-2
highlight stage. Inspection uses the original branch without a bound HSD
directional light: `8023EE44` loads the matrix at `803ABD24`, producing
`STQ=(1,1,0)` and thus `(0.5,0.5)` under GX's zero-Q rule. The shared
body evaluator applies this on the draw copy only. Native fill lights do
not bind original HSD texgen state. The light-driven mode, other stage
shapes, and strict Battle Test material admission are unchanged; the retail
appearance in a lit battle is not verified.

Package preview now fits the current posed bounds to the viewport using
the same camera fit as the solo lab. It reports a refusal when no model
geometry submits, even if the label or panel draws. That revealed eight
normal looks with authored zero normals on reflected geometry; strict
inspection still refuses those rather than applying the lab-only repair.

The six elemental Dragon Battle Tests use the same native trailing-chain
solver on their own exact joints `27–31`, anchored to the posed lower
body (joint 20 is their parent branch). The tail follows the body with
inertia and bends against the stage floor. This is an authored
presentation model, not recovered retail tail-force constants or an
additional collision object. Dragon touchdown still uses the existing
body floor query; the visual tail does not prop up the actor.
Flame, Plasma and Blizzard use the shared motion bank at its authored
translation size. Fire, Thunder and Ice have the same bank byte-for-byte,
but half-size bind translations on the torso, legs and tail. Applying
full-size keyed translations put their soles about 281 units above the
grounded actor. Their package now applies 50% only to keyed translations;
the same sampled pose drives the visible body and the launch socket.
Body floor contact is still independent of the tail.

Battle Test now covers **all 206** normal Borgs, and a roster-wide sweep at
240 frames confirms the stronger claim: every packaged Borg starts its bound
input and reaches at least one payload event (emit, extra-emit, field-placed,
hit, melee-armed, blast-triggered, detonation, landed, guidance, emitter,
anchor, modifier or guard). The sweep uses 240 frames because the Death ICBM
pair's bomb lands after the default 150-frame window; at 150 frames those two
complete the battle but fail the binary's smoke gate, which wants a hit in both
rounds. Each family below records how it was admitted and what stays authored:

- **Breath Dragons**: all eight now use the sustained-beam section above.
  Both airborne forms ship: the mechanical Dragons' downward chest beam
  (`stop-on-floor`) and the other six Dragons' fixed-angle sweep
  (`clip-on-stage`, bound through the qualified source grids).
- **Nu III pilot** (`0f03`, admitted; module `tools/analyze/retail_nu_iii.py`):
  the whole `0f01`-`0f03` family shares
  the state-2 handler `801B3824`. It pays `8006DBE0(actor, 0, 1, 1)` and
  then branches on the model id at `+0x3E8`: `0F01` spawns table child
  **74** through `80082824` (index into 56-byte rows at `802D7B10`);
  `0F02` calls `800C3BE0(actor, 61)`; every other model, `0F03` included,
  calls `8016CC24(actor, 11)`. That last call allocates through
  `80088AA0`, copies the parent transform and velocity, stores the variant
  byte `11` at child `+0x11`, and installs the shared effect-command
  interpreter `8017CD1C` at child `+0x0C` (plus `8017D4AC` at `+0x10C`).

  That difference is why the same family is bound two ways: the exact row
  (key 74, speed 32, lifetime 70) exists only for `0F01`'s branch, so that
  sibling is a shot, while `0F03`'s payload is a bytecode-driven effect
  child with no origin row, speed or lifetime to bind. `8017CD1C` reads a
  command stream into a shared context (`-21068(r13)`), so it is the
  effect-VM runtime.

  `0f03` is admitted through that exact debit and child call: a
  capacity-one `f03-stock` pool plus `field-debit` pays channel-0 cost 1,
  and the owner-following field carries the payload because the child
  inherits the parent's transform and velocity rather than launching. The
  field radius 120, damage 8, interval 6, height band 40 and the family
  pace 3.33 are authored; the command program is unclaimed. Its siblings
  stay as they were: `0F01`'s table child has an explicit speed/lifetime
  row, which is why that one is a shot and this one is a follow.

  The `0f05` child is a third shape again (allocator
  `8008893C`, callbacks `801F8108`/`801F840C`).
- **Zeta body material (admitted)**: the four Zeta bodies each contain one
  two-TObj diffuse pair (`0x00030010` head `src=4`, tail `src=5`) plus
  single/reflection stages. The scoped
  `body-material=diffuse-overwrite-pair` policy admits exactly that pair,
  and the authored idle/walk loop bits bind their Samurai-family clips.
  All four have empty `model_parts.default_candidates`, so the Samurai
  close-B route has no hand records; the admitted route is the shared
  B-shot child below.

  The four also share a neutral **B-shot** row (`state 0`, `substate 0`,
  selector 44, handler `[61,0]`, callback `801487C4`), so hands are not the
  only route. That callback dispatches through `0x8033ECF8` (modes 0-4 →
  `80148838`, 5 → `80148874`, …) and state table `0x8033ED0C`. The
  terminal `800B2190` allocates a child (callbacks `800B26CC`/`800B2CE4`,
  phase table `802FC7AC`, parent `+0x8C`); its handlers use `80089100`
  list registration and `8003D6E4` distance/matrix helpers. It is a
  following child effect, not an origin factory.

  The four are now admitted through that B-shot route. The validated
  `body-material=diffuse-overwrite-pair` policy binds their two-stage
  body, the authored idle/walk loop bits bind the Samurai-family clips,
  and the following-field API expresses the child: `field-follow=source`,
  `field-core-damage` radius 90 / damage 10 / interval 6, lifetime 30,
  and the new `field-debit` paying the exact channel-2 cost 1 through the
  move's stock. The source child asset and scene damage receiver remain
  unclaimed; the field geometry, damage and timing are authored.

  `0107` was the one body whose only refusal was a material stage, and it is
  now admitted. Its single `0x00030082` mesh is the reviewed immutable
  highlight; `body-material=immutable-highlight` admits exactly that
  signature, and the shared draw path applies the original `STQ=(1,1,0)`
  UV. It is not the general mode-2 relaxation.
- **X-only pilots are now supported**: `0303` Delta II and `0c02` Beam
  Tank declare an `x` move and rely on the showcase's existing frame-41 and
  frame-126 presses; the HUD reports a missing B. Remaining X-side routes
  (`080a`) end in effect or child calls rather than an origin factory.
- **Charge-field pilots** (`0806`, `0809`, `080f`, `080a`, `0308`,
  `0004`, admitted): the X group's callback `80156428` dispatches through
  `804346D8` (SDA base `8043B5A0`) to mode 0 → `80156460` → state table
  `8034267C` → `801564B0`/`801565CC`/`801566F0`, driving program `0xC1`
  through `8006D144`/`8006E1AC` and the `800B22F4` velocity helper.
  `080a` (`801C04E8`) and `0308` (`8010A508`) are deeper neutral-B state
  machines that end in the same velocity helper and matrix/state calls,
  not an origin factory.

  `0004` Acceleration Ninja is the **steerable stance**, bound on X.
  Its neutral X row (`state 2`, `substate 0`, selector 44, callback
  `80162780`) dispatches `80162418` → `8034C72C` (byte `+0x582`) →
  `80162780` → `8034C75C` (byte `+0x581`) → `801627C8` → `8034C770`
  (byte `+0x540`). The entry leaf `80162804` sets the forward speed from
  the model movement operand at config `+0x2C` and starts the steering
  clock, which matches the 146-case controller audit's
  `borg-controller-audit.md#acceleration-ninja-steerable-stance-with-actor-bound-contact`
  recovery: a stockless stance with two actor-bound contact shapes
  activated by animation progress. Those shapes are model-local; the
  shared collision bank's group 10 is stage geometry, so the native field
  radius stays authored. The `b-shot-c0-s0` clip requests no projectile,
  which is why the earlier review left this model unsettled; its B chain
  is a separate program-`0xC1` state and is not bound.

  The new `move-charge` API reuses the native dash for that movement, and
  the following field carries authored radius 240 / damage 10 / interval
  6 / lifetime 30. The source contact shape and damage receiver remain
  unclaimed. All six keep their source health, pace and locomotion roles.
- **ICBM pilots** (`0d06`, `0d07`, admitted): the private
  `deep-0d06-death-icbm.json` receipt records a no-stock X terminal, the
  exact eligibility/life/hold phases, a supplied floor/terrain impact to a
  type-73 area request, and the terminal geometry/damage/service as
  unresolved. Both bodies have no motion bank. The new `free-release`
  policy binds a zero-cost static projectile; the native bomb launches,
  lands and blasts with an authored 300 radius, 20 damage and 30-tick
  lifetime. The source terminal geometry and type-73 service remain
  unclaimed.
- **Staged-child pilot** (`0205`, admitted; module
  `tools/analyze/retail_staged_field.py`): its `(0,0)` context maps into
  a multi-stage chain rather than one class callback. `80156FFC` selects `803429C4`/`803429D4` on byte `+0x582`,
  the entry dispatches again on `+0x581` through `803429E4`/`803429F8`,
  and the leaves drive program, timer and effect helpers
  (`8006D144`/`8006E1AC`/`8006A474`/`8006A5A4`/`800679D0`) instead of an
  origin factory. The extractor finds no class callback for the model.

  The neutral `(0,0)` path, with the fresh-move dispatch bytes at zero,
  is also only a setup step: `80156FFC` →
  `80157068` → `80157540` → `80157584`, which clears the carried velocity
  and halfwords, runs the `8020BA78`/`8020BA9C`/`8020BA54` vector calls
  and emits, then advances the sub-counter. The child allocation
  (`800B2190`, four sites) and the `800B22F4` velocity helper appear in
  later substates of the same chain, so the attack is a staged combo
  rather than a single entry. The chain is free (no `8006DBE0` site) and
  has no origin row, so the admitted pilot binds the exact neutral input
  and carries the payload as an owner-following field, with radius 120,
  damage 10, interval 6 and the 30-tick window authored. Only the neutral
  entry is run: the substates, their `800B2190` children and the staged
  timing remain unbound, and a move chain is still excluded as a
  combo/cancel tree.
- **Spawner-shot pilot** (`010a`, admitted): its B state machine
  `80199490` → `8036FB48` spawns rows **11** and **12** through
  `800853EC` and pays channel-2 cost 1 through `8006DBE0`. Rows 11/12
  give speed 25.0 and timer 60.0. The pilot binds the exact row/debit and
  locomotion, and authors a two-shot ordered projectile with chest joint
  12 plus `(0,0,60)`, look and damage; the source child asset and
  animation-sampled placement remain unclaimed.
- **Static spawner pilot** (`0f04`, admitted): its B state machine
  (callback `801F0EC4`, state table `803A15B8`) spawns row **23**
  through `800853EC`; row 23 carries speed 25.0 and timer 60.0, and the
  scoped state has no visible channel debit. The body has no motion bank.
  `free-release` binds a static projectile with an authored joint-2 chest
  origin, look and 8 damage; the shared effect child asset and
  animation-sampled placement remain unclaimed. The same child update
  (`80084740` → `802DA018`: moving hitbox with shape handoff `8008AFF0`
  and release `80088E50`) is documented with the `010a` pilot above.
- **Flying-boss pilot** (`0e00`-`0e05`, admitted; module
  `tools/analyze/retail_flying_boss.py`): the neutral
  B-shot row (`state 0`, `substate 0`, selector 44, handler `[61,0]`,
  callbacks `801D5964`/`801D75A0`/`801D5ECC`/`801D66CC`) scans four bits
  and
  activates type-36 carried children through `8013212C`. The child update
  `801322DC` hands its shape to `8008AFF0`, the same collision handoff the
  Roach and ICBM pilots use, so the contact exists; only the damage amount
  is unrecovered because the HP writer `8003D358` is not statically
  reachable.

  The native `cruise-boost` pilot carries the exact meter and cruise
  operands (cap 10000, refill 2000, delay 0, boost clock 30), and the walk
  target comes from the shared pace normalisation (`0e01`/`0e02` cruise
  planar 10.0 → 4.17). `0e03`'s recovered target is 0.0, below the native
  `grounded-v1` floor, so its walk is the documented 0.40 minimum and is
  labelled authored. `body-motion=static-cruise` covers the missing motion
  bank. An owner-following field carries the recovered corner contact;
  radius 240, damage 10, interval 6, height band 40, cruise height 50 and
  boost cost per tick 1 are authored, and the `(0,±132,±232)` corner
  layout is recorded in the receipt. `0e04`'s posed body spans about
  12,360 x 6,337 x 12,360 units, which exceeded the battle camera's
  25,000-unit far plane, so `camera_dolly_to_fit_bounds` now takes an
  opt-in `grow_far` flag: the packaged-battle call site grows the far
  plane to the measured depth instead of refusing to draw, while every
  other caller keeps its clip planes.

  `0e00`/`0e05` bind **X** instead (`state 2`, selector 45, callback
  `80189FA8`). Their X leaf pays the exact channel-2 cost 1 through
  `8006DBE0(actor,2,1,1)` and then spawns a mirrored part pair through
  `800C74EC(actor,65)`, which indexes 8-byte rows at `803072C8 + id*8`
  and a 24-byte descriptor `{0x33010000, 2.0, ±191, 532, −1192, 4.0}`;
  the child update `800C7664` registers lists with `80089100` and
  releases with `80088E50`. The pilot binds that exact debit with a
  capacity-one `wing-stock` pool plus `field-debit`, and carries the same
  authored field. Their B route is not bound: `801FC1B0` spawns a
  pose-only sequence child (`8020C2A8`/`8020CBB8`, byte `+0x83` = 16)
  that never calls the collision API.
- **Roach bomb pilot** (`0f05`, admitted): its B-shot row (`state 0`,
  `substate 0`, selector 45, callback `801E7F28`) pays `8006DBE0` and
  creates the type-5 area child at `80085E00` (phase table `802DA544`,
  six-slot proximity `8008AE60`, shape handoff `8008AFF0`, release
  `80088E50`). The area row is `record=22, group=3, parameter=4,
  material=0, cue=12`; the common-collision group 3 is a radius-310
  sphere. The native bomb controller already owns the same exits, so the
  pilot binds the exact row/debit/pose/stock and authors a 30-tick arm,
  70-tick lifetime, 310-unit blast, 12 damage and 150-unit proximity
  radius. The source 300-tick timer, scene receiver and ownership return
  are recorded, not claimed. Stock is capped at three and returned on
  expiry or proximity.

  The source damage is still unclaimed. The private
  `bulk-scene-receiver.py` already executes the shared receiver spans
  `8008A28C`-`8008A5D0`, `80089AEC`-`80089C00`, `8008A65C`-`8008AB30`
  and `8003A4B8`-`8003A5E4` with supplied shape/body registration, but its
  own scope excludes the damage/HP step: it observes the published
  receiver shape lists, not the amount. A Roach adaptation would bind the
  group-3 radius-310 sphere and the area child's shape handoff and show
  which lists receive it.

  `8003D358` is the HP writer: it checks the scene flag at `scene+0x1F`
  and the actor byte at `actor+0x83`, writes a type to `actor+0x1C8`,
  stores `HP - r4` at `actor+0x1C6`, clamps to `[0, actor+0x1C4]`,
  mirrors the value through `actor+0x4A0`/`actor+0x4A4`, and returns
  whether HP is now zero. It is **not statically reachable** from
  `main.dol`: a scan of both text sections finds no `bl`/`b` to it, no
  `0x8003D358` word in any data section, and no `lis`/`addi` address
  materialisation. The amount join therefore goes through a
  runtime-registered callback (or a region outside this DOL image), so
  `r4` cannot be recovered by static reading alone. The authored 12
  damage remains the playable value.

Next passes should prefer a shared capability (X moves in the scripted
showcase, breath/area, child ownership) over one-off per-Borg exceptions,
and keep recording whether an origin came from explicit weapon/socket data
or is a clearly labeled approximation. Reserve original execution for
unresolved questions where a result changes the portable mapping.
