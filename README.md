# Gotcha Force — native port

Work-in-progress, independently implemented C++20 native game systems with
platform backends and offline asset readers. Linux is the current reference host.
This repository contains engine source, tools and original examples. For retail
content, supply your own original game image. No retail assets, original
executable, emulator, generated translation, reconstructed C decompilation,
private evidence or credentials are included.

The [release target](docs/architecture-directions.md) is a standalone shared
engine with separately supplied game content: portable Borgs, stages, modes,
scripts, presentation, and fonts.

## Current state

- **All 206 normal retail Borgs are playable** through authored Battle Tests
  (install v62, 222 combat selections). Every package records what stays
  authored, and a roster-wide sweep confirms each Borg starts its bound input
  and reaches a payload. See the [Borg API map](docs/borg-api-map.md) for the
  per-Borg binding and fidelity, and
  [attack-pipeline recovery](docs/borg-attack-pipeline-recovery.md) for what
  the native contact path does and does not reconstruct.
- Shared GL renderer, model/material/animation machinery, input and audio.
- Native menu/editor systems, inventory and save formats.
- Challenge setup and ordered battle initialization in progress.
- Story setup/progression/UI/audio components.
- Standalone native G-Red/Sasuke duel MVP with provisional gameplay rules.
- Recovered `800300BC` reaction direction and the `+0x2AE` 4-tick impact
  cooldown, with content keys for per-move mode/offset and rendered torso
  yaw/pitch. Local 2v2 accepts a second keyboard player (I/J/K/semicolon,
  U/H/O) or attached controllers on the same player ports.
- Opt-in G-Red source walk/turn/stop/dash lab with native input, using the shared
  world, contact proxies and renderer; source jump/flight/combat remain unsupported.
- Opt-in source moving-jump extension with takeoff, ascent turning, falling drift,
  landing and resource spending; air dash/relaunch/combat remain unsupported.
- Separate source air-dash extension with original wind-up/active/recovery
  programs and meter policy; relaunch/combat and unreviewed contacts still refuse.
- Opt-in source relaunch mode with additive airborne launch, original animation
  handoff and meter/readiness predicates; combat and unreviewed contacts still refuse.
- Source dash-jump mode adds buffered ground-dash interruption through the
  existing takeoff path, preserving normal G-Red's source speed reset.
- Source landing mode permits reviewed jump/dash cancellation after accepted
  landing entry, with cached-history refill; instant touchdown skips still refuse.
- Source surface mode adds paired static-surface landing gates and verified
  non-cancelling takeoff input; steep/attached/unknown responses still refuse.
- Source steep mode separately admits validated ordinary-falling static pushes,
  keeping them airborne; unresolved seams and dash/relaunch push variants refuse.
- Source fall-surface mode adds reviewed post-dash/relaunch selector6 pushes
  while preserving their falling animations; active-callback pushes still refuse.
- Source recovery-surface mode separately admits airborne dash-recovery pushes
  during program12, preserving contact-before-completion and landing/refill order.
- Source windup-surface mode adds continued program10 pushes with distinct
  acceleration/timer handling; entry pushes and early wind-up landings still refuse.
- Source active-surface mode adds continued program11 pushes without replaying
  entry or skipping readiness, spending, depletion/recovery and landing boundaries.
- Source relaunch-surface mode adds continued program14/15 projection with
  positive jump acceleration and separate lane latches; entry/landing still refuse.
- Source relaunch-entry-surface mode separately validates descending additive
  entry projection; rising entries and relaunch landings remain refused.
- **No original-battle or full-game compatibility claim.**

The immediate priority is a [playable native G-Red/Sasuke MVP](docs/native-battle-mvp.md),
using provisional gameplay rules and the shared model/animation/renderer systems.
The detailed [source-reconstruction progress](docs/battle-scene-mvp.md) is retained
for later accuracy work, not as a prerequisite for that prototype.
Current controller recovery and the source-dash lab are documented in
[G-Red's animation-controller layout](docs/g-red-animation-controller.md).

Some development demonstrations use private generated metadata profiles and
integration harnesses. Those artifacts are not distributed here. The public
desktop entry point does not yet connect every newer menu/Story/Challenge
component. A successful build or extracted ISO is not a completed game.

### Published maps

Generated, asset-free views of the roster are checked in. None contains model,
texture or audio bytes; each regenerates from a local roster.

- [Borg API map](docs/borg-api-map.md) — per Borg: identity, movement
  candidates and the bound operand record, locomotion roles, neutral action
  rows, binding capability and its fidelity, air-action budget, pose-ground
  offset, recovered stock operands and which audio keys are actually bound.
  Data: [`docs/api-map/gg4e08-borg-api-map.json`](docs/api-map/gg4e08-borg-api-map.json).
  Tool: `tools/extract/borg_api_map.py`.
- [Model render inventory](docs/api-map/model-render-inventory.json) — render
  bits, texture and TLUT formats, vertex attributes and PObj types, with the
  models that carry unknown bits. Tool:
  `tools/analyze/material_stage_inventory.py`.
- [Descriptor lane events](docs/api-map/lane-events.json) — per Borg, each
  resolved entry clip's lane group, choice, event and flags. Tool:
  `tools/extract/retail_borg_roster.py` with `--action-clips`.
- [G-Red/Sasuke contact damage](docs/api-map/gred-sasuke-contact-damage.json) —
  the first live original damage chain: base 20, credit difficulty 1.02,
  attacker rank 0.85, defender class row 1.13 and receiver rank 1.15, amount 22,
  with the DOL addresses and the observation that produced it. This is a
  measured single-pair contact record, not a general rule. Tool:
  `tools/analyze/gred_sasuke_contact_damage.py`.
- [Contact scalar ladder](docs/api-map/contact-scalar-ladder.json) — the
  32-entry defender class row reached from root `80433600` and indexed by the
  per-team settings byte. Index 16 is 1.13 (Kakeru) and index 30 is 1.4 (Mana),
  both confirmed by captured hits. Tool:
  `tools/analyze/contact_scalar_ladder.py`.
- [Damage amount model](docs/api-map/damage-amount-model.json) — the translated
  19-stage chain of `8003CD5C`, the eight table roots, the model-index mapper
  and five captured amounts (`22, 33, 27, 16, 3`). Tool:
  `tools/analyze/damage_amount_model.py`.
- [Borg damage selectors](docs/api-map/borg-damage-selectors.json) — the three
  defender row selectors at `plXXXXdata.bin +156..158`: 198 single-block models
  (Sasuke `1,1,0`, G-Red `2,2,2`) plus 13 combined forms with one triple per
  component; eight asset-only ids have no block. Tool:
  `tools/analyze/borg_damage_selectors.py`.
- [Reaction scaffold map](docs/api-map/reaction-scaffold-map.json) — the 47
  direct calls of `8002E2A8` in order: 16 pre-amount, the amount and HP write,
  then 29 reaction/presentation calls, with the amount boundary and labelled
  helper roles. Tool: `tools/analyze/reaction_scaffold_map.py`.
- [Collision walk map](docs/api-map/collision-walk-map.json) — the 68 direct
  calls of `8002BB14` in four phases (14 broad-phase, 7 geometry, 24 apply,
  23 post), with the pair-loop call `8002C4CC -> 8002DB58` and its reaction
  link. Tool: `tools/analyze/collision_walk_map.py`.
- [Reaction installer tables](docs/api-map/reaction-installer-tables.json) —
  the 42-model basic-shot cohort's installer and per-Borg 24-byte table
  addresses, decoded from bounded `actor+0x27C` installer paths.
  Tool: `tools/analyze/reaction_installer_tables.py`.
- [Reaction attack table](docs/api-map/reaction-attack-table.json) — the
  per-Borg 24-byte action tables (G-Red `80365EC0`, Sasuke `802D4160`,
  Proto King `80344C88`), with mode `+0x0E`,
  flags `+0x10`, yaw/pitch `+0x14`/`+0x15`, repeat `+0x16` and the
  installer/pair pointer chain. Tool: `tools/analyze/reaction_attack_table.py`.
- [Reaction action map](docs/api-map/reaction-action-map.json) — the
  `pl####hit.bin` queue groups, 0x50-byte steps and their attack-table
  entries for G-Red, Sasuke and Proto King. Tool:
  `tools/analyze/reaction_action_map.py`.
- [Projectile reaction map](docs/api-map/reaction-projectile-map.json) —
  seven factory selector routes and verified ordinary-B examples. Authored
  projectile moves emit `reaction-mode` and `reaction-offset` when their
  resolved steps agree. Seven mode-4 shots also bind the verified fighter link
  to the native projectile owner's body root at contact. Other unsupported
  routes retain an explicit fallback. Input contexts are not hit-queue indices.
- [Close-B reaction map](docs/api-map/reaction-melee-map.json) — the neutral
  program request, lane, opcode-10 queue and reaction rows, including audited
  continuations after pose-only windups. The extractor binds 91 close-B
  test moves to the native planar body-root direction and their recovered
  offsets. Fourteen also switch from the windup to the recovered local
  swing clip without changing hitboxes, timing, or damage.
- Damage row emission — authored `basic-shot` packages append the recovered
  `damage-vs` row for the training target (attacker settings 0, defender 16,
  labelled as authored test state) and both side-paired `damage-side` rows;
  the target fixture declares
  `damage-model=10`. Smokes: `tests/host/damage_row_content_smoke.py` and
  `tests/host/damage_side_content_smoke.py`.
- [Paired contact-scale test](docs/api-map/damage-contact-roles.json) —
  an optional, authored setting/rank snapshot that combines credit and
  defence from the actual two packages, including sparse directional
  matchups. Ordinary Battle Test packages keep their existing damage rules.
- [Direct-contact power](docs/api-map/damage-contact-power.json) — the
  optional HP base for 170 reviewed direct moves. Zero or disagreeing
  source rows keep the authored base; hit geometry and timing stay authored.
- [Receiver response ladders](docs/api-map/damage-receiver-ladders.json) —
  two separate optional multipliers, indexed by projected HP and by the
  pre-hit receiving team's force ledger.
- [Attacker bonus ladders](docs/api-map/damage-attacker-ladders.json) —
  pre-hit HP and team-force curves, opted into by 172 verified direct
  moves without applying them to unrelated or zero-power routes.
- [Contact damage tiers](docs/api-map/damage-contact-tiers.json) —
  the 64-cell receiver table, per-hit charge, expiry, and the separate
  reset gauge. The optional Battle Tests use native idle tick windows.
- [Ordinary contact sound](docs/api-map/contact-sound.json) — source
  tone pairs selected by receiver size, with opt-in native PCM playback.
- [Ordinary contact art](docs/api-map/contact-impact-visuals.json) —
  exact common effect records presented as bounded static hit layers
  in the optional Battle Test.
- [Indexed contact damage](docs/api-map/damage-contact-indices.json) —
  complete setting and rank tables with explicit native round selection.
- [Contact amount finalizer](docs/api-map/damage-amount-model.json) —
  the one-HP floor and the earlier zero returns, with an opt-in native test.
- [Contact pose pause](docs/api-map/contact-pose-pause.json) — local
  slowdown operands and the opt-in native skeletal hold.
- [Battle hit ledger](docs/api-map/battle-hit-ledger.json) — source
  knockout Force cost, observer slots, and ring relation codes.
- [Reaction effect map](docs/api-map/reaction-effect-map.json) — the 29
  reaction-band calls with decoded argument registers (immediates, field
  offsets, paired addresses) and the known impact/color/spawn parameters.
  It also identifies the distinct object and collision-pair inputs behind
  direct-hit motion and origin/centre reactions.
  Tool: `tools/analyze/reaction_effect_map.py`.
- [Collision branch map](docs/api-map/collision-branch-map.json) — the 98
  conditional branches of `8002BB14` with their decoded compare/mask
  conditions (cmpi, cmpl, fcmpo, rlwinm./extsb./extsh. masks), phases, the
  field/immediate provenance that fed each tested register, and named
  broad-phase/early-geometry gates. Tool:
  `tools/analyze/collision_branch_map.py`.
- [Pair-loop branch map](docs/api-map/pair-loop-branch-map.json) — the 32
  conditional branches of `8002DB58` in prepare/inner/select/publish bands,
  including the mode, carrier-flag, validity, distance and CTR-copy gates,
  field/call provenance for each tested register, and the `80030490` category
  dispatch (48 category bytes -> 15 handlers, bit `0x80` swaps the arguments).
  Tool: `tools/analyze/pair_loop_branch_map.py`.
- [G-Red/Sasuke move bases](docs/api-map/gred-sasuke-move-bases.json) — the
  per-move base records: B-close/boost base 20 → 22 and X base 30 → 33 measured,
  B-shot base 30 → 33 inferred from the third 0x198-stride entry, with the
  config offsets and measured factors. Tool:
  `tools/analyze/gred_sasuke_move_bases.py`.
- [Decomp battle inventory](docs/api-map/decomp-battle-inventory.json) — metadata
  for the private matching decompilation in the scene/actor, collision, damage
  and action ranges: matched versus open counts and the largest open functions.
  Tool: `tools/analyze/decomp_inventory.py`.
- [Retail battle call graph](docs/api-map/retail-battle-engine-map.json) — the
  direct-call graph from twelve scene/actor/collision/reaction/damage/action
  roots, plus the dispatch-table entries. Tool:
  `tools/analyze/retail_battle_engine_map.py`.
- [Action class map](docs/api-map/retail-action-class-map.json) — the original
  dispatch hierarchy: 32 family entries, six sub-tables and five phase tables
  selected by `actor+0x580`, `actor+0x581` and `actor+0x540`, with match status.
  Tool: `tools/analyze/action_class_map.py`.
- [Contact side rows](docs/api-map/contact-side-rows.json) — the side-paired
  credit difficulty/rank and receiver-rank rows for team 0 and team 1. Tool:
  `tools/analyze/contact_side_rows.py`.

Two recovery records carry the reasoning rather than the data:

- [Borg API map notes](docs/borg-api-map.md) — what each field means and what is
  authored versus recovered.
- [Attack pipeline recovery](docs/borg-attack-pipeline-recovery.md) — the full
  contact trace from input to data, the three remaining steps, and every
  candidate that was measured and rejected.
- [Battle engine recovery](docs/battle-engine-recovery.md) — the shared action
  class table (`802D3B68`), the scene/actor state dispatch, matched versus open
  decompilation coverage, and the native mapping.

## Build

Requires a C++20 compiler. The desktop executable additionally needs `pkg-config`,
SDL2 and desktop OpenGL development packages.

```sh
cmake -S . -B build/native -DGF_BUILD_DESKTOP=ON
cmake --build build/native -j2
ctest --test-dir build/native --output-on-failure
```

Without CMake, the included direct build runner uses the same native sources:

```sh
python3 tools/build_native.py --host
python3 tools/build_native.py --tests backends,hsd_record_animation,force_inventory
build/direct/gf_native --headless --frames 3
```

Tests using original data skip their optional source-data portion when it is
absent; generated test fixtures are separate from original game assets.
Hardware/display-dependent tests are opt-in. Compilation is limited to two jobs.

## Solo Borg control test

With the local native MVP and G-Red movement assets prepared, run:

```sh
build/direct/gf_native --borg-controls --no-audio
```

Click the top-left dropdown to switch between **G-Red** (recovered movement
subset) and **Sasuke** (provisional native MVP controller). Switching starts the
selected Borg fresh on the stage. The scene has one active actor, no opponent
or combat, and does not read or write a save. Animation previews and other
unqualified Borgs are not selectable. The required local asset directories
default to `local/content/native-battle-mvp-v1` and
`local/content/source-gred-ground-v9`.

Controls are shown in the window. **Q** resets, **Enter** pauses, and **Esc**
closes. G-Red uses the existing bounded movement acceptance checks and a
native solo steering setting, not an original no-target controller claim.
`--demo-borg-controls` runs a short scripted switch-and-movement smoke test.

## Interactive combat demo

With a locally extracted playable roster and prepared Battle Test stage:

```sh
python3 -B -m tools.extract.interactive_battle_demo ROSTER_ROOT \
  build/direct/gf_native local/content/native-battle-mvp-v1 local/interactive-battle \
  --practice local/content/source-gred-ground-v9
local/interactive-battle/play.sh
```

Click the top-left dropdown to switch between installed playable Borgs.
Use **Prev/Next** or the mouse wheel to page. **Flight Stick** and **Jump
Stick** are at the end. Switching starts a fresh fight against the
training target; it does not read or write a save. The battle window is
resizable and the camera backs out to fit oversized Borg bodies. Arrows move,
R holds grounded facing for strafing, **Alt+Arrow** dashes,
Shift jumps, flies or boosts according to the package, Space uses B,
and E uses X where present. The HUD says **X NOT IMPLEMENTED** when a
package has no X move and **B NOT IMPLEMENTED** when it has no B move, so
a package like Beam Tank or Delta II, whose only authored attack is X, says
so instead of advertising a dead key. Jump Stick and Flight Stick are quick ways to test
the binding. Q restarts and Esc exits.
For a standalone battle with up to six fighters,
[choose a package roster](docs/borg-gameplay-api.md#choose-a-roster)
or launch a [competitive, co-op, or one-local roster preset](docs/portable-content-v1.md#reusable-battle-test-rosters).
`--list-battle-rosters DIRECTORY` lists preset metadata, and
`--check-battle-roster FILE` checks headless startup without playing.
`--browse-battle-rosters DIRECTORY` opens the desktop preset picker.

For a scripted attack showcase, run
`local/interactive-battle/show-attacks.sh gg4e08:0500` (Flame Dragon).
Add `--exercise-battle-mobility` to watch an elemental Dragon jump, land,
and trail its articulated tail along the stage floor.
To script Sasuke's movement as well as attacks, run
`local/interactive-battle/show-attacks.sh gg4e08:000a --exercise-battle-mobility`.
`--practice` adds the checked G-Red flight controller, Sasuke's three-jump
test, and authored mechanics examples such as Rotary Scout and Remote Scout.
It also binds qualified package movement clips and source-relative ground
pace where indexed. The examples are not complete retail movesets.
For source G-Red, touchdown must complete its landing-entry update before
a jump or dash can interrupt. A Shift press on that exact update is not
queued: release and press Shift again after contact.
Alt+Arrow is translated to a source-direction flick for G-Red; raw
modifier and trigger buttons are not passed to the source controller.
An unsupported staged input pauses the live round with a diagnostic.
Use Q to restart or pick another Borg; the demo never switches to a
different movement controller after a refusal.

## Extract your own assets

Python 3 is sufficient for the supplied raw-disc/AFS/PZZ readers and PNG export.
Extraction writes only to a new directory or new file and never modifies
the input image.

```sh
python3 -m tools.extract.disc game.iso original/extracted
python3 tools/extract/member.py original/extracted/files/afs_data.afs \
  pl0615.pzz local/focus/pl0615.pzz
python3 -m tools.extract.pzz_files local/focus/pl0615.pzz local/focus/pl0615
python3 tools/convert/texture.py original/extracted/files/opening.bnr \
  local/assets/banner.png
```

See [asset extraction](docs/asset-extraction.md) for supported formats and limits.
Extraction currently exposes the original data and reusable decoding primitives;
it does **not** yet recreate every private native metadata/profile used by all
development harnesses. Do not download or distribute original assets.

## Layout

- `engine/`: portable native systems; no SDL/OpenGL headers in game logic.
- `platform/`: desktop rendering, input, audio and save backends.
- `tests/unit/`: native and extraction tests.
- `tools/extract/`, `tools/convert/`, `tools/analyze/`: bounded offline readers.
- `tools/content_package.py`: deterministic loose-content packaging.

The first publication consolidates the paused native menu/editor, Challenge,
Story and graphics work, plus relevant reusable Borg asset-analysis utilities.
It deliberately starts a new Git history: the older decompilation/controller
research repository and its private data remain separate.

Recovery checks for the contact model:

For optional retail-input checks, set `GF_RETAIL_ROOT` to your local
extracted content directory (default `original/extracted`).

```sh
python3 -B -m unittest discover -s tests/unit -p 'test_*.py'
# native API and battle-damage fixture
g++ -std=c++20 -O1 -I . tests/unit/borg_gameplay.cpp -o build/direct/borg_gameplay
./build/direct/borg_gameplay
# recovered pair-selection seam
g++ -std=c++20 -O1 -I . tests/unit/battle_pair_targeting.cpp -o build/direct/battle_pair_targeting
./build/direct/battle_pair_targeting
# recovered tables -> content row -> real host (needs the original DOL)
PYTHONPATH=. python3 -B tests/host/gred_sasuke_contact_fixture.py
# emitted reaction keys and damage row in the real package battle
PYTHONPATH=. python3 -B tests/host/reaction_content_smoke.py
PYTHONPATH=. python3 -B tests/host/damage_row_content_smoke.py
PYTHONPATH=. python3 -B tests/host/damage_side_content_smoke.py
# host package/roster, presentation, stage, collection, arrange and Story
PYTHONPATH=. python3 -B tests/host/borg_api_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/borg_presentation_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/portable_stage_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_partition_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/coop_roster_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/one_local_roster_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_tools_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_browser_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_series_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_clock_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_stock_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_ammo_hud_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_mobility_hud_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/roster_cpu_height_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/borg_collection_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/borg_arrange_smoke.py build/direct/gf_native
PYTHONPATH=. python3 -B tests/host/portable_story_smoke.py build/direct/gf_native
# rendered variants (no window needed)
SDL_VIDEODRIVER=offscreen PYTHONPATH=. python3 -B tests/host/borg_api_smoke.py build/direct/gf_native --opengl
SDL_VIDEODRIVER=offscreen PYTHONPATH=. python3 -B tests/host/borg_presentation_smoke.py build/direct/gf_native --opengl
```
