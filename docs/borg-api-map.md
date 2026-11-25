# Borg API map

`docs/api-map/gg4e08-borg-api-map.json` is a generated, metadata-only view of
what the engine was given for each retail Borg. It carries no model, texture
or audio bytes, so it can be published with the portable API documentation.

Generate it from an exported roster:

```sh
python3 -B -m tools.extract.borg_api_map <roster>/borgs docs/api-map/gg4e08-borg-api-map.json
```

The tool is read-only on the roster and refuses to overwrite an existing file.

## Fields

- `id`, `name`, `collection_number`, `tribe`, `hp`,
  `ratings_def_sht_atk_spd` — identity and stats from the manifest and
  catalogue.
- `logical_joints`, `wrapper_slots` — rig extent and how many exact wrappers
  the package carries.
- `movement.candidates` — every movement configuration the extractor found,
  with its source type and flags.
- `movement.pace` — the opt-in source-relative pace, its operand and the
  native target, when the model qualified.
- `movement.operands` — the bound slot-0 record, its SHA-256 and every
  labelled operand value. `movement.operand_uses` is the shared schema in
  `<roster>/shared-movement-operand-schema.json`, which names the offsets
  (for example `02C` travel-entry planar, `050` ascent planar, `058` boost
  planar, `004`/`008`/`006` flight meter).
- `movement.jump_display`, `movement.move_display_slots` — the catalogue
  display slots.
- `air_action_budget` — the combined jump and air-dash budget bound from operand
  `09F` on 181 grounded walkers, or `null` where the profile keeps the older
  jump-only rule. It records the operand, the value and
  `retail_behaviour_verified: false`.
- `roles` — the locomotion clip roles bound for the package.
- `actions` — per group, the profile identity and the neutral `(0,0)` rows
  only, with state, selector, handler pair, class callback and how many
  contexts the row covers.
- `capability` — the pilot that binds the Borg, or `null` when the package
  carries no executable gameplay.
- `binding` — the fidelity record: the same capability, the fields the receipt
  marks as authored (with its note), the source fields it did recover, and
  `retail_behavior_verified`. On this roster that flag is `false` for all 206
  Borgs: 23 carry an explicit authored `damage` in the receipt and the rest
  keep damage in the generated rules. Nothing here claims retail equivalence.

### Where damage is not

Damage was searched for in the two projectile row tables and is in neither. The
56-byte child rows at `802D7B10` for `0f01` key 74 carry `51.8, 40.0, 100.0,
0.75, 15.0`, and the 80-byte rows at `802D9758` for `0f04` key 23 carry `-35,
100, 60, 0.3, 7.5, 25`: speed, lifetime, scale and timing, with no `8` among
them. It cannot be derived from the attack rating either — `0f01`'s ratings are
`[1,2,0,2]`, so its attack stat is zero while its authored damage is 8.

So damage lives in the program/cue path that the projectiles do not describe,
and recovering it means decoding that path rather than reading the row. Until
then every pilot's damage stays explicitly authored.

The static route to that path is now exhausted. The HP writer at `8003D358`
(the `HP - r4` site found during the Roach work) has no `bl`, no `b`, no 32-bit
pointer word in any data section, and no `lis`/`addi`/`ori` materialisation of
its address anywhere in either text section. The direct `+0x1C6` HP writes that
do exist are initialisation sites (`HP = +0x1C4`), such as `8004E318` and
`80081CFC`, not damage application.

Settling the amount therefore needs an observation the binary cannot give
statically: a bounded original execution under the project's strict replay gate
(fresh exclusive slot, one isolated process, no concurrent compiles or replays),
recording `r4` at the damage site for a known attacker and defender. Until that
runs, authored damage stays labelled rather than guessed.
- `audio` — the landing candidate (bank, tone, whether the trigger is
  qualified, and whether the PCM file is present), the `other_sfx` state,
  whether a launch sound was authored, and how many action cues and routes
  the shared `shared-action-sfx.json` recovers for that Borg.
- `preview_declaration` — whether a preview is declared.

## Reading it honestly

The map records bindings, not equivalence. An operand value is a recovered
number, while a `capability` label such as `flying-field` names the native
mechanism that carries it. Whether that mechanism reproduces the retail
behaviour is stated per family in `retail-roster-extraction.md`, and every
authored geometry, damage or timing value is listed there as authored.

## Model render inventory

`docs/api-map/model-render-inventory.json` is the companion view of what the
renderer is asked to draw:

```sh
python3 -B -m tools.analyze.material_stage_inventory <roster>/borgs docs/api-map/model-render-inventory.json
```

It aggregates each packaged `body.hsd` fingerprint into `unknown_render_bits`,
`texture_formats`, `tlut_formats`, `vertex_attributes` and `pobj_types`, and
lists the models that carry unknown render bits. It records counts and format
names only, never pixels.

The first run is mostly a negative result worth keeping: every texture is a
standard format (`128x128/CMPR` 4,951 MObjs, `256x256/CMPR` 1,572,
`64x64/CMPR` 540, `128x128/RGBA8` 6), so an untextured look is not a decode
gap. The single dominant "unknown" bit is `0x00000010`, present on 7,061 MObjs
across 204 of 206 models, which makes it a universal flag the analyser does not
name rather than a per-model difference. `0x00000030` appears on four models
only. The remaining work is therefore texture binding, UV/matrix stages and
material-stage support, not extraction.

## Texture binding tally

The packaged battle now prints a per-slot mesh tally on its admission line:

```text
slot=0 id=gg4e08:0004 variant=normal moves=1 textured=28 untextured=0 unresolved=0
```

`textured` counts skinned meshes whose texture index resolves in the decoded
scene, `untextured` counts meshes authored with no texture stage, and
`unresolved` counts meshes naming an index the scene does not carry. It is a
read-only accessor (`CharacterModelScene::texture_tally`), so it never changes
what is drawn.

Running one frame of every packaged Borg gives:

| Measure | Value |
| --- | --- |
| Textured meshes | 5,924 |
| Authored-untextured meshes | 641 |
| Unresolved texture indices | 0 |
| Models with no textured mesh | 2 (`0f05` 3/3, `0f06` 5/5) |

So an untextured or flat look is **not** a missing-index or missing-asset bug:
every referenced texture resolves. The highest untextured shares are ordinary
stages rather than failures — `0806` 7/18, `0d05` 7/18, `0d01` 6/16, `0c02`
10/31, `0d00` 8/26, `0d04` 8/26. The remaining visible causes to investigate
are the CMPR decode path, UV and texture-matrix stages, and the strict
reflection refusals already documented in `retail-roster-extraction.md`.

## Audio state

The map records what is actually bound rather than what is hoped for:

| Measure | Value |
| --- | --- |
| Landing PCM present | 181 of 206 |
| Landing trigger qualified | 0 of 206 |
| Action cues recovered in `shared-action-sfx.json` | 13 of 206 |
| `other_sfx` still `unresolved` | 206 of 206 |

A generic scan was tried and rejected. Walking each action entry's bounded
local CFG for `SOUND_REQUEST` (`800F036C`) calls and accepting only a
same-block literal `r4` recovered nothing: at the sites that exist the id is a
caller argument, not an immediate. For example the type-36 child update calls
`800F036C` at `8013229C` with whatever `r4` the caller supplied. Emitting those
would mean inventing the id, so the scan was removed rather than shipped.
Coverage has to come from reviewing individual sites, which is what the curated
`DIRECT`/`RELAY` tables do.

So the "missing sfx" complaint is accurate and locatable. Most Borgs carry a
landing candidate with no qualified trigger, and only thirteen have recovered
action-SFX routes at all (`0007`, `0104`, `0203`, `020c`, `0619`, `061b`,
`061f`, `0620`, `0625`, `0804`, `080c`, `0c05`, `0f05`). That is the next audio
unit: recover the cue tables per action from the source banks for the other 193,
then bind them per move through the existing `movement-cue`, `launch-sound` and
`impact-sound` API instead of adding per-Borg exceptions.

## Posed floor

The package battle reports the posed body's vertical extent on its per-tick body
line, for actor 0:

```text
borg-body: round=0 tick=1 actor=0 draws=29 floor=9.35..292.29
```

`floor` comes from `NativeBattleMvpView::package_body_bounds`, which reads the
same posed scene the renderer uses (`model_slot` indirection, so it is the right
model). It is read-only.

Two things it separates immediately:

- **Airborne models read their spawn height.** `0d00` BLUE STRIKER reads
  `250.0`, `0620` VICTORY JET `250.6`, `061b` EAGLE JET `432.0` — those are the
  cruise heights from their own `cruise-boost` rows, not errors.
- **The verified dragon scale works.** `050c` FIRE DRAGON, `0515` THUNDER
  DRAGON and `0516` read `-8.04`, which is what the reviewed
  `animated-translation-percent=50` should produce. Full-size keyed
  translations would have put them about 281 units high.

Among grounded packages most read within ±10 units. The stable outliers, all
present at ticks 1, 10 and 30, are:

| Model | Name | floor low |
| --- | --- | --- |
| `0109` | DEATH BORG BETA | 215.72 |
| `0a07` | DEATH WING | 83.93 |
| `050d` | CLAW WORM | 83.33 |
| `0517` | POISON WORM | 83.33 |
| `0d01` | RED ATTACKER | 74.30 |
| `0d02` | CARRIER HELICOPTER | 71.30 |
| `0401` | DRILL ROBOT | 67.99 |
| `0b00`/`0b03`/`0b06` | VALKRIE family | 49.52 |
| `0d07` | DEATH BOMB | -50.00 |
| `0d06` | DEATH ICBM | -134.54 |

No level-record field correlates with these values (`050b` sits at 9.35 with
`180/180` while `050d` sits at 83.33 with `0/0`), and `translation_scale` is a
verified per-cohort fact for the six dragons rather than a general rule. So the
remaining offsets cannot be recovered from the data read so far; forcing them
to zero would be an authored presentation offset, not a recovered behaviour.
They are therefore left as-is and recorded here instead of being quietly
"fixed".

### Corrected offsets

Those measured values are now bound through `pose-ground-offset`, with each
package recording `retail_behaviour_verified: false` and its provenance
(measured posed floor, stable at ticks 1, 10 and 30):

| Model | Name | before | after |
| --- | --- | --- | --- |
| `0109` | DEATH BORG BETA | 215.72 | −0.28 |
| `0a07` | DEATH WING | 83.93 | −0.07 |
| `0517` | POISON WORM | 83.33 | 0.33 |
| `0b00` | QUICK VALKRIE | 49.52 | −0.48 |
| `0619` | TITAN TANK | 21.81 | −0.19 |
| `0f06` | DEATH EYE | 26.40 | 0.40 |
| `0004` | (not in the table) | 9.92 | 9.92 |

Twenty-six rigs are bound in total. The shift happens inside the pose
evaluation, so `bounds()`, the camera fit and the draw use the same corrected
geometry while the actor itself never moves.
