# What makes a self-contained Borg?

This is a requirements map based on the current engine and recovered data, **not
a new manifest schema or an integration plan already in effect**. The existing
[`borg.manifest`](borg-packages.md) discovers files; it does not interpret their
contents or make a Borg selectable. “Self-contained” means one directory owns
the Borg-specific data and declares any dependencies on shared content. It does
not ship a second renderer, physics engine, controller VM, or copy of the menus.

## Identity and classification

Keep these identities distinct. The same Borg definition can have several
variants and many owned instances. A single owned instance can appear in
several Forces.

| Key | Owner and use |
| --- | --- |
| Stable Borg ID | The package's namespaced ID, independent of folder, display name, archive order or current scan. Used for joins between systems. A retail adapter may supply a separate legacy model ID; do not assign custom Borgs spare retail IDs. |
| Name and catalogue metadata | Default readable name, optional localized names, description, and sorting/filtering data. A collection number or display order is not the ID. Tribe, movement family, and mode eligibility are separate classifications. |
| Variant ID | A stable key local to the Borg, with a label and an explicit default. A variant can override presentation and/or gameplay; it is not automatically just a recolor. A gameplay transformation is a state change, not a cosmetic selection. |
| Owned instance ID | Save-owned identity for a particular copy. The save owns its selected variant, progress, edits, and disable state, and caches a readable name for missing packages. Force slots refer to instances, not names or models. |
| Runtime actor and resource identities | Assigned by the world and resource owners when preparing/spawning a valid instance. Live animation cursors, timers, HP, ammo, target, velocity, and receipts are state, not manifest fields or saved type IDs. |

Use separate fields for **tribe/collection filters**, **displayed jump type and
count**, **actual movement and resource policy**, **combat capabilities**, and
**mode availability**. A similar pose, identical displayed jump count, or
shared model cannot select a controller. The original compatibility catalog
already separates model, collection number, tribe, source name, cost, rarity
and XP thresholds ([`ForceCatalog`](../engine/assets/force_catalog.hpp)).
Preserve source sort order and encoded names in that adapter without requiring
native packages to use the original font encoding or numbering.

### Identifiers from the Borg reference

The July 2022 Gotcha Force Wiki snapshot was audited as the source for this
catalogue field set. All 206 numbered entries have name, image reference,
collection number, Force cost, crystal requirement, tribe, combat role, rarity
grade, alternate availability, level-1/level-10 HP, four displayed ratings,
displayed jump type and a level-up schedule. A few additionally report a
source Borg code or observed speed. [`GFBORG-CATALOG 2`](borg-packages.md)
represents these distinctions without making the reference code a model ID or
the displayed jump type a gameplay rule.

The move entries identify B shot, B melee, charged B, X and charged X, as
well as separate form movesets. Reserve stable move IDs linked by input,
context and form; a button does not name a single move. For projectile and
attack definitions, keep these identifiers independent:

- **Solidity/interaction**, **penetrated target classes** (such as Borgs or
  objects), and **explosion** with its trigger and phase. Some moves list
  multiple values, so allow phase-specific rules rather than one flag per Borg.
- **Ammunition applicability**, capacity at levels 1 and 10, and **refill
  pattern** (e.g. all at once or gradually). An unknown endpoint, no ammunition
  for a move, and unlimited stock are different. Do not derive missing values.
- **Context, fallback and passive traits**: airborne/grounded/dash/contact
  checks, chained/charged phases, and special actions such as copying or
  passive damage. These attach to authored action or trait IDs, not to rarity
  or model graphics.

The action record separates **button**, **activation**, **guard/priority**
and **action kind** (melee, projectile, special, no action). Ammo stock,
refill pattern and replenishment timer are separate values. This allows
range-dependent melee versus shot, charge and release branches, and an
explicit no-op when no action is eligible. It does not infer an input priority.

Reserve **mechanical outcomes** as their own event bindings, not VFX/SFX.
User examples guide the slots: Witch charge shots may buff allies, debuff
enemies, invert controls or temporarily replace a target's form and moves;
Hero transformations may have an expiry; Teleportation Ninja needs an
explicit target-position exchange; Acceleration Ninja may combine a form
change with an ongoing contact hit. The Death ICBM example needs impact-area
and actor-interruption conditions: its landing outcome is distinct from being
destroyed before contact. None of these examples supplies validated durations,
radius, damage, priority or carry-over values. A guard, outcome payload,
duration and interruption policy each needs its own owner and tests.

The catalogue reserves sidecars for moves, traits, forms, fusions, acquisition,
effects and variant overrides. The [standalone moves check](borg-packages.md#standalone-moves-check)
validates move/phase data and local fallbacks. The [acquisition check](borg-packages.md#standalone-acquisition-check)
keeps starting grants, progression, crystal assembly and battle-only results
apart. Prose-only requirements such as encounter/unlock conditions are not
discarded just because an infobox lacks a field; neither check executes them.

### Variants and progression

- A complete declaration resolves, per variant, its model/rig compatibility,
  materials and textures, icon or preview overrides, its stats or inheritance,
  and any action/map overrides. Inheritance is explicit; a missing override is
  not permission to silently substitute another variant or animation.
- Entry prices, rarity, level progression/XP thresholds, health and other
  derived stats belong to the definition or a named shared progression rule.
  Current level, XP, acquisition serial, and any user-entered overrides belong
  to the instance. Keep **display ratings** (DEF/SHT/ATK/SPD) separate from
  **simulation values** and ammo; one cannot generally be inferred from the
  other ([spec data](../engine/assets/force_specification.hpp),
  [edit provenance](../engine/gameplay/borg_edits.hpp)).
- The original inventory handles six variants (`0..5`) and ten level rows; its
  bundle has a model wrapper per variant, with separate actor/battle data and a
  motion bank. That proves how originals are keyed and selected, **not** that
  all six variants can spawn. Current checked class, combat level and preview
  paths admit a much narrower normal-variant subset. Do not copy the retail
  slot counts or assume its variant labels/appearance rules for authored
  packages ([bundles](../engine/assets/battle_character_resources.hpp),
  [class](../engine/assets/battle_normal_class.hpp),
  [level rows](../engine/assets/battle_actor_levels.hpp)).
  The [variant checker](borg-packages.md#variant-overrides) keeps asset
  inheritance and HP/ammo-level overrides separate; the standalone preview
  requires a matching profile for a selected nondefault variant.

## Package contents and present evidence

“Needed for” below is a capability gate. A catalogue-only or preview-only Borg
may intentionally stop earlier; a complete playable Borg needs all referenced
data for its declared features. **Observed** means a current reader or owner
uses that data, not that every Borg or variant is supported.

| Area | Self-contained data or explicit shared dependency | Needed for / current evidence |
| --- | --- | --- |
| Identity and description | Stable ID, name, localized labels or localization references, description, tribe/tags, optional display number/order, declared variants and supported features. | Collection, picker, editor and save restoration. Discovery currently recognizes ID, name and file references only. |
| Inventory and progression | Per-variant cost/rarity, stats, XP curve, starting level/resource/stock defaults as applicable. Optional part identities, same-variant recipes or collection relationships by stable ID. | Inventory, stats, Force budgets and rewards. [`ForceSupplement`](../engine/assets/force_supplement.hpp) and [`ForceFusion`](../engine/gameplay/force_fusion.hpp) already separate parts/recipes from ownership; encounter drop odds and unlock policies belong to the mode. Native package saves store identity, an explicitly selected variant and cached label, disable state and reserved slots; XP/level progression is not stored yet. |
| Force editor art | Borg tile/icon, optionally variant badges or icon overrides, framing and transparency sufficient for a small tile. Optional portrait, nameplate, descriptions, collection art and a preview camera/scale/idle selection. | Editor grid, held tile, Force cards, picker and specification. Original icons are **separate static presentation records** selected by model through [`ForceEditorArt::icon`](../engine/assets/force_editor_art.hpp), reused by [Challenge cards](../engine/gameplay/challenge_force_scene.hpp) and [Story cards](../engine/gameplay/story_force_card_scene.hpp). A native image could feed the same role; the HSD record is not a mandatory authoring format. [Nameplate/stats UI](../engine/gameplay/force_stats_ui_presentation.hpp) can run with 3D preview unavailable. |
| Model, rig and material assets | Battle/preview mesh, hierarchy, skin weights, bind/default pose, units/origin/facing, material slots, textures and palettes, UV usage, visibility groups, and optional alternate parts. Name bones or binding points for animation, attachments and collisions. | 3D preview/render. The shared [`CharacterModelScene`](../engine/gameplay/character_model_scene.hpp) currently reads HSD model, bone prefix and pose profile together and refuses unsupported material/attachment paths. It is not a generic glTF importer. The original model/pose binding includes logical bone order, mirror pairing and upper/lower lane membership; [part selection](../engine/assets/character_model_parts.hpp) can select extra records by bone and visibility. |
| Animation assets | Compatible clip bank with stable clip references, skeleton binding, duration/timebase, and channel/track data. Declare shared banks explicitly; no need to copy identical clips. | Clip inspection, preview and action playback. [`CharacterMotionBank`](../engine/assets/character_motion.hpp) binds source group/entry to archives, including aliases, and the bone sampler validates logical bone binding. Unsupported tracks or references are not a new package's idle fallback. |
| Animation bindings and programs | Map each declared action/phase to clip(s) or a supported program: idle, travel/directions, dash phases, takeoff/ascent/apex/fall/landing as applicable; attacks, hit reactions, defeat, spawn, transitions, transforms or fusion as declared. Specify lanes, mirroring, blend/hold/loop/completion, root-motion ownership, and event timing where required. | A clip label is for inspection; a binding drives playback; a controller chooses when to play it. Names in a compatible bank may supply explicit unambiguous mappings; otherwise include an animation map. The [MVP clip map](../engine/gameplay/native_borg_motion_map.hpp) is distinct from recovered [GFMP programs](../engine/assets/character_program.hpp) and the [G-Red program-ID table](../engine/gameplay/borgs/normal_gred_movement_programs.hpp). Do not treat program IDs as clip IDs or give a package authority to run arbitrary code. |
| Movement and contact rules | Name supported behavior primitives and provide selected walk/turn/stop/dash/jump/flight constants, directional/input interpretation, state priorities and legal cancels. Declare resource type/capacity/charges/refill, contact semantics (standing, hover floor, free flight), dimensions/feet and movement/landing limits. Defaults and no-action states must be explicit. | Movement lab and battle. [G-Red's definition](../engine/gameplay/borgs/normal_gred_movement.hpp), [admission table](../engine/gameplay/borgs/normal_gred_movement_admission.hpp) and [GFMM operands](../engine/assets/character_movement.hpp) are the reviewed example, not a generic rules loader. Terrain queries, sweeps, tangent math and accepted-frame sequencing remain engine services. See [observed movement families](borg-movement-families.md) for hypotheses that still need separate proof. |
| Battle fusion and transformations | Explicit relationship IDs, compatible partner IDs and form/variant restrictions for fusion, and source/target forms, triggers, timing and return rules for transformations. Own the transition maps and reference the result's model, rig, moves, effects and behavior. | Conditional battle capabilities, not inventory recipes or cosmetic variants. Reserve the groups below; these runtime paths are not established by the recovered normal G-Red controller. |
| Executable moveset and collision | For each move: semantic input and availability (ground/air, direction, held/charged, combo context), priority/cancel gates, timings and resource/ammo costs, animation phases/events, damage and reactions, and the hit/shot/attachment producer. Declare hurt boxes, active hit shapes/windows, hit rules, and muzzle/weapon offsets, projectile or melee parameters and effects where used. | Battle readiness. The [native duel](native-battle-mvp.md) has actor-slot weapon choices, provisional damage, cooldowns and body proxies. Source [body/attack collision data](../engine/assets/character_collision_bank.hpp) exists in a bounded format, but a parsed shape, animation, or sound cue does not imply a working actor-side callback or combat move. |
| Displayed moveset | Localized move names/descriptions, input hints, activation presentation, icons and optional demo binding. Link to executable move IDs where supported and visibly separate entries that are only documented. | Specification/unit info. The current [display catalog](../engine/assets/force_specification.hpp) reads up to five source move rows with icon, button, activation and text index. Its [page access gate](../engine/gameplay/force_specification.hpp) reads collection state; text/readout does not authorize the move in battle. |
| Actor lifecycle | Spawn/init defaults, semantic hooks for spawn/land/hit/defeat, optional alternate part/attachment dependencies and the resource owners each hook needs. | Required where invoked by a declared behavior; missing mandatory nodes/resources must block that capability. The [legacy binding](../engine/gameplay/battle_character_binding.hpp) ties class data, rig, model, materials and motion hashes before publishing an actor pose. |
| Spawn placement | Independent horizontal and vertical anchors, initial contact/movement state, units, invalid-placement policy and applicable forms/modes. Reserve mode-selected X/Z, stage center, and absolute X/Z separately from floor-relative, arena-top-relative or absolute Y. | Actor admission. The stage and encounter resolve legal spawn points; the Borg can request a policy but cannot publish an actor or bypass bounds. Special spawn flags remain source-recovery questions. |
| Effects and sound | Per-Borg effect and sound banks, with stable IDs and event bindings. Include or explicitly reference bullet/beam/missile graphics and materials, charge/muzzle/trail/impact effects, and firing, impact or other action sounds. See the reserved groups below. | Any declared visual/audio behavior. [Motion sound events](../engine/gameplay/character_program_sound_events.hpp) provide some source inputs, not full allocation/playback receipts. [The native duel](native-battle-mvp.md) has provisional shot/impact presentation, not a generic Borg effects loader. |
| AI and mode participation | Optional compatible AI behavior choice/hints tied to real moves (range, mobility, resource use) and authored capability metadata. Mode admission references the validated Borg/variant rather than a self-granted “Story allowed” flag. | Challenge and Story owners decide roster/encounter eligibility, budget, rewards and scripted availability. [Challenge party assembly](../engine/gameplay/challenge_initial_party.hpp) consumes model/variant/level and instance provenance, and [AI pool selection](../engine/gameplay/challenge_ai_force.hpp) is difficulty/wave/cost data. A different model or rewritten record cannot borrow those source receipts. |

The Force editor's **layout, cards and background panels**, shared glyphs,
button/feature/rarity art, and the battle HUD are engine or mode assets.
Package-owned portraits/icons/nameplates are their inputs, not a duplicate UI.
Keep cockpit layout and other display-mode resources shared. Likewise stage collision,
cameras, targeting/damage resolution, input-device bindings, RNG, account/save
storage, encounter scripts and inventory ownership stay outside the Borg folder.

### Reserve a spawn policy independent of flight

Reserve a `spawn` definition group. Its data needs a placement anchor
(mode-selected, selected floor, verified arena top, absolute height, or
`unresolved`), an offset/clearance in documented units where applicable,
whether the placement is required or preferred, the initial vertical speed
and contact/movement role, and explicit behavior when a stage cannot satisfy
the request. Placement alone must not set a flight meter, treat hover as
ordinary grounding, or grant an impact action. Different forms and modes may
need distinct spawn bindings.

Horizontal placement is an independent request: mode-selected, the stage's
declared center, explicit world X/Z, or `unresolved`. An arena center is
stage-owned data, not necessarily world `(0, 0)` or the midpoint of arbitrary
bounds. The reported Galactic Emperor case motivates a center anchor plus a
separate absolute Y; neither the exact height nor the all-stage behavior is
established here. A large Fortress or boss also needs proof the whole body
fits and the position is clear. Do not substitute a smaller collision proxy,
move a required spawn to another point, or publish a partial actor.

The native simulation's vertical axis is **Y**; X/Z form the ground plane.
Keep source-axis conversion in its adapter. The current
[duel's `spawn_query_y`](../engine/gameplay/native_battle_mvp.hpp) selects a
floor layer for a grounded spawn; it is **not** a forced altitude. A future
admission path needs an actual stage upper bound and clearance/body checks.
Do not interpret a generic ceiling query reference as a playable top.

The reported candidates are Death ICBM's airborne entry, the Fortress Borgs'
fixed aerial stance, and airplane Borgs' flight entry. These motivate
separate initial roles such as descending/impact, fixed airborne and flight.
The archived Death ICBM entry supplies an aircraft/contact-explosion lead;
it does not establish this spawn flag, a height formula, or which Borgs share
it. Leave the exact flag, per-Borg mapping, and heights `unresolved` until
verified independently. The [standalone spawn-policy check](borg-packages.md#standalone-spawn-policy-check)
can validate the declared shape without selecting a stage position or admitting an actor.

### Reserve battle fusion and transformation relationships

**Battle fusion** is different from collecting parts and combining them in the
inventory. Reserve a stable fusion relationship ID with both input Borg IDs and
their permitted forms/variants, each participant's role, and the resulting
definition ID. The result can be its own package with its own model, textures,
rig, animations, moves, effects, and movement policy. A *fusion-result-only*
definition would resolve as a dependency without becoming an independently
selectable or randomly granted unit. An unrelated installed Borg with the same
name is not a partner.

Reserve eligibility and synchronization rules: how the players request fusion,
team/range/state requirements, per-participant approach/combine/fail/separate
animation roles and anchors, when the result takes control, and how both
participants return or retire. Track ownership, health, ammo, resources, timers
and position across both joins. Missing partners, result packages, event
bindings or unresolved transfer rules must not consume either actor or produce
a partially initialized result.
The [standalone fusion check](borg-packages.md#standalone-battle-fusion-check)
validates the declared roles and dependencies without fusing or selecting units.

**Transformations** also need explicit form IDs, including a default form and
the model/rig/animation map, moveset, collision, movement policy and effects each
form inherits or replaces. Reserve both kinds:

| Kind | Transition data to reserve |
| --- | --- |
| Selected or cyclic form (e.g. Victory King) | Permitted source and target forms, selection or cycle order, semantic input, admission/cancel gates, enter and exit animations/events, and what keeps or changes the selected form. |
| Temporary form (e.g. Metal Hero's X-charge transformation) | Start event (including hold/charge/release semantics), eligibility and resource cost, owner and timebase for duration/expiry, warning and forced reversion events, return form, and what interruption, repeated activation or defeat does. |

These are user-identified examples, not verified mode order, charge thresholds,
timers or state-transfer values. For either kind, reserve rules for HP/resource
and ammo carryover, velocity and ground/air contact, active effects and
attachments, facing, and actor identity. In particular, changing form may
change movement family; it must not silently retain or refill the wrong meter.
Bind form changes through the world/controller owner and publish them only
after matching model/rig and gameplay resources are ready. Unknown transitions
remain explicit `unresolved` placeholders.
The [standalone forms check](borg-packages.md#standalone-gameplay-form-check)
validates these declarations, including a temporary form's independent return
paths, without interpreting the transitions.

### Reserve effect and sound slots now

Give these groups explicit names even when the engine cannot run them yet.
Every reference has a type and stable ID. Its declaration may be a local asset,
an explicit shared-bank reference, an explicit `none` for an intentionally
absent optional cue, or `unresolved` for a known feature whose data or runtime
binding is not ready. Do not use one status to mean the other. Unsupported
critical events block the capability that needs them. An unresolved cosmetic
cue can be reported as missing presentation, but cannot be used as evidence
that an original move is complete.

| Reserved group | What the definition owns |
| --- | --- |
| Projectile appearance | The visual for an in-flight projectile, beam, missile or bomb: sprite/mesh/segment or other declared asset kind, its textures/material, scale/pivot and animation/loop if needed. Optional engine-supported trail, tracer, glow or flight sound travels with the **live shot**, not the owner's animation forever. |
| Move/weapon effects | Charge flash or loop, muzzle effect and firing cue, melee slash/arc, attach/detach, throw or deploy effects, and their bone/socket, shot, or world anchors. Bind them to named phases/events, not to guessed frame numbers. |
| Contact and outcome effects | Hit/block/terrain-impact effects and SFX, optional explosion or lingering graphic, and expiry without impact. Event setup states whose position/normal/team context is consumed and which owner retires the effect. Contact detection and damage remain simulation-owned. |
| Actor and ambient effects | Spawn/defeat, landing dust, hover/boost loops, form-change/fusion, hit reactions, voice cues and any conditional material or attached model. Provide the start/stop hooks and fallbacks for absent optional cues. |
| Audio definitions | Stable cue ID, local clip or named shared bank/entry, one-shot/loop and owner-stop behavior, spatial anchor, and bounded mix hints where supported. Sample decoding, queuing, and mixer routing remain shared services. |
| Event bindings | Semantic source (`accepted move`, `shot spawn`, `shot contact`, `shot expiry`, `accepted animation event`), phase/time reference where applicable, target effect/SFX ID, an anchor, and lifetime/stop policy. Preview-only events must not create damaging shots or change live combat state. |

For example, reserve this graph even if the impact visuals are still unknown:

```text
move:primary -> projectile:primary-shot              [gameplay definition]
projectile:primary-shot -> visual:primary-bullet     [appearance]
shot spawn  -> effect:primary-muzzle, sound:primary-fire
shot flight -> effect:primary-trail                  [may be none]
shot hit    -> effect:primary-impact, sound:primary-hit
shot expiry -> stop flight effects and loops
```

The projectile's speed, spread, collision shape, damage and lifetime are
separate gameplay data. The bullet graphic and audio never grant a hit or count
as evidence of a recovered weapons callback. An author may use shared effect
assets without copying them into every package, but must name the dependency.
The [standalone effects check](borg-packages.md#standalone-effects-and-audio-check)
validates these typed cues and named move references without playback.

## Shape of one folder

The following is a **logical grouping**, not new supported filenames, syntax or
encodings. A small Borg can use one file per group or keep validated data
embedded with its model. Required references should be explicit and bounded.

```text
borgs/<package-folder>/
  borg.manifest           discovery identity and file references
  catalog/               names, description, stats, variants, progression
  ui/                    editor icon, optional portraits / variant art / nameplate
  visual/                meshes, rig/bind pose, parts, materials, textures
  animation/             clip bank, optional role map, transitions/events
  gameplay/              movement and resource rules, moveset, body/contact shapes
  relationships/         fusion partners/results, cyclic and timed form transitions
  effects/               per-Borg projectile/weapon/impact/actor visuals
  audio/                 SFX/voice cues, local files or shared references
  events/                effect/SFX bindings and declared unresolved slots
```

Use a versioned format with stable keys and bounded validation. Resolve
variant, action, clip, event, mesh/bone/attachment and dependency references
before publishing a capability. Reject missing required roles, bad formats,
unsupported behaviors or conflicting IDs with a reason. Never derive readiness
from filenames or tint, default to a different Borg, or treat the discovered
files as trusted executable rules. Retail provenance (source IDs, hashes and
callback/layout keys) stays in the original adapter; no source addresses are
callable package fields. Original assets and private extraction outputs stay
out of Git.

Apply the same placeholder convention beyond effects. For an observed but
unrecovered feature, record its stable key, area (e.g. variant override,
transformation, attachment event, hit reaction, AI profile), expected
dependencies, and `unresolved` status. Record whether the missing piece is
asset data, a mapping, or engine behavior. This reserves the place without
inventing numeric defaults, silently omitting the feature, or making the
package battle-ready. It is distinct from a Borg that intentionally has no
such feature.

## What is ready today?

| Slice | Established | Not established |
| --- | --- | --- |
| Native package plumbing | `borgs/` discovery, the [standalone installed picker and body preview](borg-packages.md#read-only-installed-borg-picker), a [read-only owned package collection](borg-packages.md#read-only-owned-collection), scoped recovery, a [read-only retail/package Force browser](borg-packages.md#browse-a-saved-force) with explicit overlaps, and [qualified normal G-Red practice](borg-packages.md#qualified-normal-g-red-practice-package) from an installed package, exact saved copy, or [one unambiguous saved Force slot](borg-packages.md#practice-one-saved-force-slot). | General package actors, level/XP storage for packaged instances, mixed inventory editing, retail recovery UI integration and battle-mode admission. |
| Original normal G-Red | Model/rig and motion data, classified pose references, recovered ordered walk/dash/jump/relaunch/fall/landing subset with measured meter and selected contacts. Opt-in inclined dash departure and scoped continued walking ledge-fall pushes. Existing normal-pair render, spec and native duel paths. | Full animation-controller map, original combat/weapon/auxiliary/effect dispatch, other variants, and broader ledge and steep-contact routes. |
| Original normal Sasuke | Renderable in the duel with the existing fallback prototype selection, and separate per-Borg raw previews. | Verified Sasuke behavior/program mapping from the displayed jump gauge or G-Red's labels. Three air-jump points and dash-height behavior remain observations to verify. |
| Other Borgs and variants | Original catalogue/icon/spec and model-bundle readers cover data beyond the playable normal pair. | Parsing/cataloguing a row does not prove its animation lanes, combat, transformations, variant effects, and modes are ready. Unreviewed and edited-Borg refusals stay intact. |

## Gates before integration

1. **Discovered → catalogue-ready.** Resolve one stable identity and explicit
   variant metadata to a readable row with an icon or a deliberately labelled
   placeholder. Discovery grants neither ownership nor battle permission.
2. **Catalogue-ready → preview-ready.** Validate one model, rig, its textures
   and at least a bound idle/preview role in the shared renderer. A stats-only
   view stays usable if 3D preview fails. This should be the first vertical slice
   before wiring every menu.
3. **Movement-ready → combat-ready.** Only claim explicit roles and supported
   action graphs that pass per-Borg controller tests. Require a complete
   validated move and the right actor/collision/event owners before a package
   can fight. Test an original adapter and a separately authored test package;
   one does not validate the other.
4. **Per-mode admission and save round-trip.** Join stable IDs against owned
   instances and variants once through a shared roster view; let each mode
   apply its own budgets, availability and restrictions. A missing package
   should show its saved name and affected Forces, preserve slots on disable,
   and delete only confirmed instances. Separately test missing variant data,
   corrupt assets, scan failure, duplicates and restoration. Integrate collection
   and picker before enrolling content automatically in Challenge or Story.

Do not expand the discovery manifest or wire every mode from this map alone.
First choose the minimum catalogue/preview contract and validate it with real
data; record what remains unknown rather than filling gaps by analogy.
