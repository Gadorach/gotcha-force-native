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
- `roles` — the locomotion clip roles bound for the package.
- `actions` — per group, the profile identity and the neutral `(0,0)` rows
  only, with state, selector, handler pair, class callback and how many
  contexts the row covers.
- `capability` — the pilot that binds the Borg, or `null` when the package
  carries no executable gameplay.
- `audio` — action cue keys, the lab landing PCM and whether a launch sound
  was authored.
- `preview_declaration` — whether a preview is declared.

## Reading it honestly

The map records bindings, not equivalence. An operand value is a recovered
number, while a `capability` label such as `flying-field` names the native
mechanism that carries it. Whether that mechanism reproduces the retail
behaviour is stated per family in `retail-roster-extraction.md`, and every
authored geometry, damage or timing value is listed there as authored.
