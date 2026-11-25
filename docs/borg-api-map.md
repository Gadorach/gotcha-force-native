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
