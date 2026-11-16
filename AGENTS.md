# Native port repository

- Keep game assets, original executables, translated/reconstructed game source,
  private evidence, credentials and build outputs out of Git.
- Game logic stays portable under `engine/`; platform APIs stay under `platform/`.
- Reuse the shared renderer/model/animation/input/audio systems. Do not create
  parallel menu, Story or battle render engines.
- Preserve unsupported-feature refusals, edited-Borg restrictions and exact
  source ownership/lifetime boundaries. Do not label prepared assets as actors.
- A standalone native G-Red/Sasuke duel MVP now runs. Prioritize playable
  essentials and visible fixes over source-perfect intro/UI reconstruction.
  Current user priority is the Borg movement controller. Story integration and
  accuracy refinement come later; keep MVP assumptions explicitly labelled.
- Controller input is camera-relative, but camera rotation must not force Borg
  body heading. Attacks temporarily aim the upper body. Use user-assisted,
  per-Borg animation classification before inferring shared locomotion maps.
- Bulk animation classification is paused. Current focus is recovering normal
  G-Red's original animation controller and its required shared movement subset.
  Preserve saved labels as cross-checks; do not require more manual classification
  or generalize G-Red's recovered mappings to other Borgs.
- User-observed movement families are recorded in `docs/borg-movement-families.md`.
  Hover-floor contact and nonboosting perpetual-flight recovery are distinct
  semantics to verify per Borg, not new universal rules to implement by analogy.
- Test meaningful changes. Use at most two compiler jobs. State what ran and what
  remains unsupported; do not claim original hardware or whole-game equivalence.
- Extraction must be read-only on inputs, bounded and refuse output overwrites.
- No installers, external dependency changes, commits or pushes beyond the
  user's requested scope.
