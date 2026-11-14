# Initial native-only consolidation

This repository starts with a new native-only history. The earlier research
repository, its staged work and each paused worker's files were preserved.

## Included work

- Native menu/editor, mouse/input, inventory and account-save systems.
- Challenge setup, resource ownership, partial actor construction and scene
  bootstrap through the initial view records.
- Story setup, progression, force selection, map/guide UI and voice/audio work.
- Shared graphics, model, material, animation, audio and platform backends.
- Reusable bounded disc/archive/texture/HSD readers and relevant hierarchy,
  lane-descriptor and region-analysis tools.
- Native tests and owned synthetic extraction tests.

## Shared-file resolutions

- Kept the primary editor's newer pointer/mouse, modal and source-text changes.
- Combined Challenge's shared static-icon helper with those editor changes.
- Combined Challenge archive identity/instance bindings and translucent-alpha
  selectors with Story's retained-TEV1 texture path.
- Kept independent material/joint selectors shared by Challenge and Story.
- Kept separate `preserve_alpha` and `retain_tev1` selection parameters in the
  model owner, plus submitted pointer-triangle reporting from the editor.
- Kept Story's voice/PCM queue improvements alongside the shared audio engine.
- Kept Challenge's shared cost kernel, decoded bank/skin reuse, scene-pool
  ownership and projection-independent camera orientation.
- Older graphics snapshots were not used to overwrite later integrated code.

No game executable, assets, generated translations, reconstructed decompilation,
emulator/runtime tools, matching controller, provider connections, remote compiler,
private diagnostics/evidence, save files or credentials are included. Two retired
pre-port interface/guest-memory scaffolds and their obsolete memory test are also
excluded; the native backend and typed byte-order layers are the active APIs.

The asset-export frontends use the existing bounded readers. The new disc frontend
extracts to a new directory and hashes outputs; the PZZ frontend preserves explicit
block identities/padding. Texture output now refuses overwriting an existing file.
These operations produce local user-owned artifacts, never repository content.

## Remaining delivery gaps

This is a consolidation of current native source, not a completed game release.
The published host does not yet wire every module, the native battle-scene MVP is
unfinished, and not all private metadata-profile export recipes have a public
end-to-end installer. See the README and asset-extraction documentation.

No new license has been chosen during this consolidation. Public visibility alone
does not grant a license to original game assets or third-party code.
