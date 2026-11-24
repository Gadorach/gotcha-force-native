# Engine and portable content

This is the release architecture, not a claim that every loader already exists.
Keep work incremental: qualify portable, playable slices without making the
retail executable or an evidence harness a runtime dependency.

## Separate objectives

- Compatibility: an original-game ruleset and user-supplied content.
- Modernization: platform support, presentation, controls and tooling.
- Extensibility: future Borgs, abilities, content and multiplayer modes.

Avoid making modern controls or community content depend on original executable
addresses, source callback layouts or a source-exact startup sequence.

## Runtime boundaries

- Simulation owns gameplay entities, movement, combat, AI and time.
- Input produces simulation commands; rendering consumes simulation state.
- The engine owns shared rule implementations, resource lifetimes, validation,
  permissions, and authoritative state. Content declares which rules to use,
  their operands, assets, and how to sequence them.
- Original-game readers, address maps, replays, legacy ID bridges and
  extraction belong to compatibility tooling. Retail and authored content
  execute the same versioned runtime interfaces; they need not share an
  on-disc authoring format or qualify for the same game modes.
- Do not promote a retail-ID branch or a reference-only lookup table into the
  gameplay interface. If verified behavior has no portable expression, add
  a reusable capability and test it with authored content, then encode the
  exact retail package. An audit, an asset bind, or an authored demonstration
  is not retail admission. Existing retail-specific lab paths are migration
  seams, not a precedent for new runtime branches.

The current fixed two-Borg MVP is a prototype, not the final Borg registry.

### First per-Borg movement definition

Normal G-Red's combined movement labs now select their reviewed capabilities
from [`normal_gred_movement.hpp`](../engine/gameplay/borgs/normal_gred_movement.hpp).
The source controller and native verifier consume the same list. Earlier lab
permissions and legacy policy IDs are preserved.

The same definition owns the normal G-Red meter's 10,000 capacity, refill
based on the previously accepted grounded cache, and named lift, handoff,
and active-dash costs. The source binding checks those operands against the
qualified GFMI pack. Spending and recovery arithmetic remain shared.

The definition also names the reviewed idle, walk, jump, dash, relaunch
and landing program IDs, including the two directional tables. These IDs
refer to normal G-Red's qualified movement programs, not to universal clips
or movement selectors. VM dispatch, lane ordering and completion still use
the existing engine services.

The admission table lists the order in which each reviewed G-Red selector
tries its possible movement inputs. It does not authorize a frame, bypass a
mode restriction, or run contact: the source controller and native world
still own those decisions.

This is part of a self-contained definition, not a content format or completed
registry. Stage queries, tangent movement, shared-edge checks, body collision
and frame receipts remain engine services. Action predicate details, transition
timing, and other qualified operands still have G-Red-specific code/data owners. Move
them behind the definition as those boundaries become explicit, without
copying terrain code or applying G-Red's rules to Sasuke.

## Content packaging

See [Portable content v1 contracts](portable-content-v1.md) for the
stage, Challenge, Story, Collection and Force-editor implementation plan.

[Borg package discovery](borg-packages.md) scans `borgs/<package>/` for a
manifest that names assets and rules. The catalogue is metadata; an executable
definition uses the [Borg gameplay API](borg-gameplay-api.md). Every retail
Borg, exact variant and reachable form should eventually use that packaging.
Definitions own equipment, moves, cues, form rules and mechanics parameters.
The engine still accepts or refuses requests and owns actual contacts and
outcomes.

| Content owned by packages | Engine-owned service |
| --- | --- |
| Story map, mission graph, conversations, encounters, unlock rules and rewards | Progression, event dispatch, validated battle requests, save transactions |
| Challenge stages, enemy Forces, AI profiles, win rules and options | Challenge/session flow, Force and actor resolution, AI and battle mechanics |
| Stage geometry, collision inputs, spawn markers, hazards and presentation, including [per-viewport obstacle fading](stage-viewport-occlusion.md) | Scene loading, collision queries, actor placement, rule execution and view-local rendering |
| Main menu and HUD layouts, navigation, transitions, labels, icons and cues | Input/focus, UI rendering, audio, and explicit mode/save commands |
| Opening and other sequences: animation tracks, camera, audio, timing, optional video | A shared timeline/player and presentation services; retail's pre-rendered opening is media, not battle timing evidence |
| Localized strings and fonts for menus, dialogue, HUD, names, counters and other numeric values | String lookup, formatting, glyph layout and rendering |
| Cheat definitions, activation conditions, scopes and requested effects | Explicit opt-in and authorization; edits use the same validation and save boundaries as normal actions |

Script Story, menu flow and cheats through bounded, versioned requests and
events. Prefer declarative rules and timelines where they suffice; scripts
must not directly mutate actors or saves, invoke platform code, or load native
plugins. Give packages stable IDs and explicit dependencies. Validate data
before use and reject unsupported or missing definitions without substituting
other retail content or writing saves.

Text and numbers are actual strings and values rendered through dynamically
loadable font assets, not baked words or numerical sprites. A font can use
a glyph atlas internally; an image containing a whole label is not a font.
Do not couple layout to the original encoding. Art and non-text visual
elements remain ordinary media assets. A redistributable engine build must
be usable without retail files, with independently authored content as a
smoke test; users can supply the compatible game content separately.

## Multiplayer/replay preparation

Keep simulation independent of rendering and local devices. Eventually evaluate
stable entity IDs, command recording, explicit randomness, versioned state
serialization and replay tests before selecting a networking model.

The native MVP currently uses floating-point simulation and has no network,
rollback, cross-platform determinism or stable serialization guarantee.

## Distribution and review

An original compatibility distribution and independently authored community
content could share the engine. Licensing/redistribution and legal separation
need appropriate review; an engine's code license does not license original
game assets or other protected material.

Reusable binary-analysis/decompilation tooling may be maintained separately from
the game runtime. The discovery manifest does not introduce dependencies, a
mod API, or network support.
