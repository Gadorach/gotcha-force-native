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
