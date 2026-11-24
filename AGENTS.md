# Native port repository

- Keep game assets, original executables, translated/reconstructed game source,
  private evidence, credentials and build outputs out of Git.
- Game logic stays portable under `engine/`; platform APIs stay under `platform/`.
- Reuse the shared renderer/model/animation/input/audio systems. Do not create
  parallel menu, Story or battle render engines.
- The release target is an engine executable plus separately supplied content.
  Put retail-specific choices and recovered operands into the same portable
  Borg API format as authored Borgs, not runtime branches by retail ID. Recovery
  notes, replays and legacy test paths are reference, not shipped gameplay.
- Keep Story, Challenge, stages and the Story map, menu/HUD and opening
  presentation, fonts/strings, and cheats pluggable. Use validated content and
  bounded scripts that request engine capabilities; the engine owns mechanics,
  permissions and save writes. See `docs/architecture-directions.md`.
- Preserve unsupported-feature refusals, edited-Borg restrictions and exact
  source ownership/lifetime boundaries. Do not label prepared assets as actors.
- A standalone native G-Red/Sasuke duel MVP now runs. Prioritize playable
  essentials and visible fixes over source-perfect intro/UI reconstruction.
  Current user priority is roster-wide conversion before deep per-Borg move
  recovery. Extract the common assets and stats deterministically; map basic
  movement and moves by shared evidence rather than reproducing SDK internals
  for one Borg. Keep MVP assumptions explicitly labelled.
- Controller input is camera-relative, but camera rotation must not force Borg
  body heading. Attacks temporarily aim the upper body. Use user-assisted,
  per-Borg animation classification before inferring shared locomotion maps.
- Index animation cycles automatically. Preserve saved labels as cross-checks;
  do not require bulk manual classification or generalize G-Red's recovered
  mappings to other Borgs. Use original execution only to settle specific
  unknowns that matter to the shared conversion.
- User-observed movement families are recorded in `docs/borg-movement-families.md`.
  Hover-floor contact and nonboosting perpetual-flight recovery are distinct
  semantics to verify per Borg, not new universal rules to implement by analogy.
- Test meaningful changes. Use at most two compiler jobs. State what ran and what
  remains unsupported; do not claim original hardware or whole-game equivalence.
- Extraction must be read-only on inputs, bounded and refuse output overwrites.
- No installers, external dependency changes, commits or pushes beyond the
  user's requested scope.
