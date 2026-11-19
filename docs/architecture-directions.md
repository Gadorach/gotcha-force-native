# Long-term architecture considerations

User-provided planning notes; **not an immediate implementation checklist**.
The current priority remains the playable MVP and animation classification.

## Separate objectives

- Compatibility: an original-game ruleset and user-supplied content.
- Modernization: platform support, presentation, controls and tooling.
- Extensibility: future Borgs, abilities, content and multiplayer modes.

Avoid making modern controls or community content depend on original executable
addresses, source callback layouts or a source-exact startup sequence.

## Candidate boundaries

- Simulation owns gameplay entities, movement, combat, AI and time.
- Input produces simulation commands; rendering consumes simulation state.
- Original-game readers and behaviour adapters belong to a compatibility layer.
- Community content should use the same gameplay interfaces without pretending
  to be original assets or weakening original-content validation.

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

This is part of a self-contained definition, not a content format or completed
registry. Stage queries, tangent movement, shared-edge checks, body collision
and frame receipts remain engine services. Action admission, program selection,
and other qualified operands still have G-Red-specific code/data owners. Move
them behind the definition as those boundaries become explicit, without
copying terrain code or applying G-Red's rules to Sasuke.

## Future content packaging

A self-contained Borg package could contain a manifest, models, animations,
textures, audio, abilities and AI definitions. glTF2/Blender authoring is a
candidate direction. Prefer validated behaviour primitives; sandboxed scripting
is a possibility to evaluate later, not a dependency to introduce now.

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
the game runtime. None of these notes authorizes dependency changes, a mod API,
new content formats, networking work or publication during the current MVP task.
