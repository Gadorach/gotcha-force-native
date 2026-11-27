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
Both examples fire a pulsing paint orb. Their back-mounted test part
animates during the shot: Flight Stick combines a separate bone clip
with scrolling stripes. Jump Stick moves its back-mounted badge to
its arm during the shot, swaps in a record with an authored colour
animation, and returns the badge to its back. The mount turns toward
its new tilt instead of snapping.
Jump Stick's X move (E on keyboard) also demonstrates committed aim:
it locks the direction before release and doesn't retarget a moving
opponent.
Flight Stick's X fires a sustained paint beam. Use left/right while it
is active to steer slowly; it only damages on eligible overlap pulses.
With `--power-field`, Flight Stick's X instead places a red sphere.
Fire B through it to boost the shot's damage and visible size.
With `--slow-field`, X places a blue sphere that slows the other Borg
and its travelling shots, and can cancel their guidance.
With `--field-demo`, X places one textured sphere with several effects:
the source speeds up inside, enemies slow, and the source's shot is
boosted when it travels through.
The sphere starts at a sampled bone socket. Add
`field-follow=paint-scope|source` to that generated gameplay file to
follow the same bone each update instead of staying put.

To try **wall clipping** without retail files, generate a second copy:

```sh
python3 -B -m tools.extract.demo_borgs build/asset-free-wall --clipped-beam
cd build/asset-free-wall
../direct/gf_native --battle-borg example:flight-stick \
  --against-borg example:jump-stick --battle-mvp-assets arena-wall
```

Run `arena-wall` in the normal output to compare the same beam with
pass-through. The wall demonstrates beam obstruction, not actor blocking
or destructible props.
Use `--swept-beam` instead of `--clipped-beam` in a new output directory
to give Flight Stick a straight, downward sweep that leaves visual
explosions at accepted floor and wall contacts.

The wall also demonstrates [authored viewport fading](stage-viewport-occlusion.md):
when it cuts across the camera's view of the target, it switches to the
black translucent alternate. Fading affects
visibility only; it doesn't change which shots the wall blocks.

Use `--proximity-bomb` when generating a fresh copy to replace Jump
Stick's X beam with an authored one-stock bomb. It lands, arms, and
blasts on proximity or fuse expiry; either exit returns the stock.
Use `--attracting-bomb` instead, and select `--battle-mvp-assets
arena-bomb`, to try a bomb that creeps toward an opponent before it
can trigger. The example uses original stage geometry.
`--fragmenting-bomb` instead gives Jump Stick a stock-returning
charge whose blast releases a ring of paint shards.
`--smoke-bomb` gives its X a non-damaging cloud that blocks new
aim samples without becoming a physical obstacle.
`--ink-bomb` instead splatters the viewport of a living opponent
that takes the blast. It leaves targeting and controls alone.
`--accuracy-ink` adds a separate temporary accuracy penalty:
Flight Stick's opted-in B shot can veer while that status is active.
`--elemental-ball` gives Flight Stick's paint shot a travelling
carrier that emits smaller shots at set intervals while still in flight.
`--impact-ball` instead emits a small volley at a wall contact; select
`--battle-mvp-assets arena-wall` for that example.
`--impact-trail` uses the same wall arena but leaves an expanding,
rotating trail of fading rings instead of firing a damaging volley.
`--impact-hazard` pairs the marks with short-lived collision capsules
that can each hit or be blocked once. The rings themselves stay visual-only.
Use `--package-battle-distance 260` in the scripted wall demo to put
Jump Stick within reach of the capsules.
`--impact-area` gives Flight Stick's B one expanding area instead.
Use `arena-wall` and the same distance. The area can hit only after
it starts growing; the visible rings are separate markers.
`--walk-pulse` adds passive ground shockwaves to Flight Stick. With
`--gravity-field`, Flight Stick's X places a well that pulls enemy
travelling shots and separately deals periodic damage near its core.
`--cruise-boost` instead replaces Flight Stick's normal lift with
continuous flight. Tap the jump control to spend boost, then cruise
while the meter refills without landing.
`--magnet-field` instead pulls grounded Borgs in front of the caster;
try `--package-battle-distance 260` in the scripted flat-arena demo.
`--repel-field` reverses that movement. `--electric-fence` makes
Flight Stick's X place two elevated points with a damaging, recoiling
link. In the scripted flat-arena demo, distance 260 places Jump Stick
in its range. These examples are not retail port admissions.

- **Flight Stick:** hold Shift for a bounded powered rise. It has 32 lift
  updates, replenished on landing. Release to coast; powered lift and the
  ordinary jump are intentionally not the exact G-Red controller.
- **Jump Stick:** each fresh Shift press can spend one of three jump
  points, including takeoff. Ground contact resets them. The jump and
  landing cues use generic puffs. No retail sound is bound in either example.

The packages use the same loader, pose and render path as other supported
native packages. The assets are intentionally simple; changing the text
config lets package authors test an independent shot, origin, or effect.
The editable manifests, catalogues and gameplay files are checked in
under [`examples/borgs/`](../examples/borgs/). The generator copies those
files and writes the original example media, refusing to replace an
existing output directory. Generated files can stay under ignored
`build/` or `local/`.

Focused local check (run from the repo root):

```sh
PYTHONPATH=. python3 -B tests/host/demo_borgs_smoke.py build/direct/gf_native
```

Add `--opengl` to that check to exercise desktop drawing for just the
generated models and stage.
