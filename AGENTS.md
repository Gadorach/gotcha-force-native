# Native port repository

- Keep game assets, original executables, translated/reconstructed game source,
  private evidence, credentials and build outputs out of Git.
- Game logic stays portable under `engine/`; platform APIs stay under `platform/`.
- Reuse the shared renderer/model/animation/input/audio systems. Do not create
  parallel menu, Story or battle render engines.
- Preserve unsupported-feature refusals, edited-Borg restrictions and exact
  source ownership/lifetime boundaries. Do not label prepared assets as actors.
- The next visible milestone is a shared renderable battle-scene MVP for normal
  G-Red/Sasuke and Story UI integration; it is not implemented yet.
- Test meaningful changes. Use at most two compiler jobs. State what ran and what
  remains unsupported; do not claim original hardware or whole-game equivalence.
- Extraction must be read-only on inputs, bounded and refuse output overwrites.
- No installers, external dependency changes, commits or pushes beyond the
  user's requested scope.
