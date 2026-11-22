# Try Battle Test without retail assets

The generator creates two original stick figures, a flat arena, their
rigs and animation clips, and text gameplay packages. It reads no disc,
game archive, save, texture, sound or prepared content.

```sh
python3 -B tools/build_native.py --host --jobs 2
python3 -B -m tools.extract.demo_borgs build/asset-free
cd build/asset-free
../direct/gf_native --battle-borg example:flight-stick \
  --against-borg example:jump-stick --battle-mvp-assets arena
```

Switch the two IDs to control Jump Stick. Arrow keys move; Shift jumps,
Space fires a generic paint shot, Q restarts, and Esc exits. The opponent
can be controlled on a second port; it is not a Story or AI encounter.

- **Flight Stick:** hold Shift for a bounded powered rise. It has 32 lift
  updates, replenished on landing. Release to coast; powered lift and the
  ordinary jump are intentionally not the exact G-Red controller.
- **Jump Stick:** each fresh Shift press can spend one of three jump
  points, including takeoff. Ground contact resets them. The jump and
  landing cues use generic puffs. No retail sound is bound in either example.

The packages use the same loader, pose and render path as other supported
native packages. The assets are intentionally simple; changing the text
config lets package authors test an independent shot, origin, or effect.
The generator refuses to replace an existing output directory. Generated
assets should stay under ignored `build/` or `local/`; only their original
generator belongs in Git.

Focused local check (run from the repo root):

```sh
PYTHONPATH=. python3 -B tests/host/demo_borgs_smoke.py build/direct/gf_native
```

Add `--opengl` to that check to exercise desktop drawing for just the
generated models and stage.
