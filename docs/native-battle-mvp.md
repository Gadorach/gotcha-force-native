# Playable native battle MVP

This is now the priority, ahead of source-perfect battle reconstruction.
Long-term, non-blocking design notes are kept in
[architecture directions](architecture-directions.md).

## First playable target

- One real battle stage.
- Normal G-Red versus normal Sasuke, with animated movement.
- Player movement, jumping and aiming; a basic attacking opponent.
- Beam/bullet/shuriken/bomb entities with collision, damage and health.
- A simple camera, health display, victory/defeat and restart.
- An immediately runnable desktop demo and actual renderer contact sheets.

Use the shared renderer, model/animation, input and terrain systems. Do not wait
for original briefing/HUD callbacks, exact intro timing or full battle scripting.
Keep source-accurate entry points and their refusals intact; the MVP is a separate,
explicit native gameplay policy, not a claim of original behaviour.

## Initial assumptions

`NativeBattleMvp` owns two gameplay actors and a bounded projectile pool. Its
60 Hz movement, jump, hit sizes, damage, cooldowns, simple opponent behaviour and
weapon selection are provisional tuning, not recovered constants. Ground can be
supplied through an adapter to the existing stage grid. Weapon origins can be
supplied by the animation system; the fallback muzzle offset is provisional.

## Working desktop prototype

The duel now runs with the original stage's two normal records, original normal
G-Red/Sasuke models, and shared skeletal animation/skin/render systems. Static
terrain uses the existing STIH decoder/ground math. Prepared model resources
alone are not gameplay actors: `NativeBattleMvp` owns their gameplay state.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1
build/direct/gf_native --battle-mvp --no-audio
```

Controls: WASD move, L dash, Shift jump, Space primary, E secondary, Q restart,
Enter pause, Esc close. `--demo-battle-mvp --frames 90` runs scripted input.
`--battle-mvp-assets DIR` selects the loose art directory. Prepare it with
`tools/extract/native_battle_mvp.py --help` from existing normal native bundles
and pose profiles; inputs remain read-only and output overwrites refuse.

The first actual45-tick GL capture exchanged damage (health96/76). A90-second
interactive session reached a G-Red win with health52/0. Contact Sheet05 compares
the first camera with the corrected shoulder view. CPU tests passed588 checks
per native/sanitizer mode; asset preparation tests and overwrite/input-integrity
checks passed. These are prototype checks, not original-game equivalence.

## G-Red movement practice

```sh
build/direct/gf_native --gred-movement --no-audio
```

This opens the existing combined G-Red movement controller on the original
stage using the ordinary practice spawn and arena bounds. Arrows walk or steer,
WASD flicks dash, and Shift jumps or rises. Release and press Shift again to
relaunch when eligible. Enter pauses, Q restarts, and Esc closes. The HUD's
flight bar and movement label read the last accepted source frame, not pending
input or a guessed animation phase. For example, wind-up still displays
`DASH WIND-UP` on the handoff tick, and `TOUCHDOWN` precedes `LANDING`.
During a continuing air dash, the HUD counts down to directional release and
shows `CENTER TO RECOVER` once that release is eligible. Neither a landing nor
a dash already transitioning to recovery shows the cue.
Pause and restart are sampled each rendered frame, even if no 60 Hz movement
tick is due. Resuming does not catch up elapsed pause time.
One sampled Shift tap can carry to the next movement tick if released before
then. It is still subject to the source jump and relaunch rules. Pending taps
do not survive pause, restart, or a stopped practice session.
A sampled high-deflection WASD or stick flick can also carry for one tick.
Only the latest eligible sample is retained. Live jump, stick, or arrow
input takes priority. Separate sampled jump and flick presses are never merged
into a new combination, and the source controller still decides what to admit.
[Contact Sheet 61](../local/recomp-evidence/contact-sheets/gred-practice-clock-v1/contact-sheet-61.png)
shows an offscreen replay of pause and resume at tick 2, then one restart
while Q remains held.

If interactive practice refuses a motion, it stops at the last accepted tick
and shows a restart screen instead of closing. The stopped scene does not draw a
retired pose or meter and does not advance or switch controllers. Press Q to
start a fresh round, or Esc to close. Scripted movement labs still fail on a
refusal so regressions are not hidden.

An input rejected before movement (for example, pressing a combat button)
shows `INPUT REFUSED - RELEASE AND RETRY` instead. The current scene and meter
remain visible, and valid input can continue without a restart.

[Contact Sheet 60](../local/recomp-evidence/contact-sheets/gred-practice-restart-v1/contact-sheet-60.png)
shows an offscreen input replay that stops on a raised face after tick 104.
The simulation remains at that tick until Q. Holding Q restarts once, and the
meter returns only after a fresh source frame is accepted.

`--demo-gred-movement --frames 600` runs the existing scripted wind-up-entry
route with the same HUD. Both commands need the qualified local GFMM9 pack
and native stage/model data. They do not change the duel, enable combat, or
relax unsupported contact refusals.

[Contact Sheet 59](../local/recomp-evidence/contact-sheets/gred-movement-practice-v1/contact-sheet-59.png)
shows actual offscreen OpenGL captures. All four rendered bar widths matched
the accepted meter within one pixel. Host tests covered both launch commands;
the owner sanitizer covered pending frames, pause, foreign worlds and retired
receipts. The ordinary duel smoke test was unchanged.

## Current priority: Borg movement controller

User feedback: movement is rough. Focus on camera-relative control, acceleration/
braking, turning/strafing, jump/landing and animation-to-motion matching. Combat
and effect tuning are deferred during this pass.

User correction: controller vectors are camera-relative, but camera rotation
must not force the Borg's body heading. The camera follows the targeted opponent
from behind the player using the player-to-target vector. Locomotion owns body
facing; attacks temporarily aim the upper body toward the target.

The first clip guess was wrong: bank1 conditional-melee entries are not a
walking map, even when nonnull. That guess is superseded. Source ground
descriptors point to bank0/clip1 for walking legs, but low/high-speed, dash, flight
and attack semantics should now be established with the
[numbered classification tool](animation-classification.md), not more guesses.

The first controller pass adds an actual camera-relative input basis, bounded
ground acceleration/braking, slower air steering, bounded facing turns, and a
short landing transition. Animation playback follows actual distance after
collision/bounds clipping, not input strength or render-frame count. Root-motion
distance is used as a stride hint when available; physics still owns position.
Upper-body attacks and lower-body locomotion use the existing pose lanes, with
fractional sampling and short per-lane transition blends.

154 movement checks passed in native/sanitizer builds, along with the duel and
older diagnostic regressions. A real-model200-tick run passed. Sheet06 contains
actual walking/strafe and jumping/attacking captures at ticks90/200. A90-second
interactive controller run completed with G-Red winning (health68/0).

The above run was the first feel pass and was subsequently corrected by the
user's camera/body and clip feedback. It is not a complete original movement
controller. Multi-jump, exact per-Borg movement parameters and more sophisticated
wall/ground response remain unimplemented. Directional animation mappings still
need visual refinement.

### Classified G-Red locomotion pass

The user's per-Borg labels now select G-Red's walk (1), forward/back/left/right
ground dashes (2/3/4/5), takeoff (6), ascending/held-air pose (7), apex (8),
landing (9), Air Dash (23) and falling (24). Uncertain clip14 and conditional
melee entries are not used. Sasuke retains its provisional map; no shared
locomotion scheme has been inferred.

L/shared left-trigger starts a bounded ground or air dash. Ground dashes retain
body heading and select a directional pose; Air Dash faces travel direction.
Steering remains camera-relative. Speed12, duration12 ticks, cooldown30 ticks,
and one Air Dash replenished on landing are explicit native MVP assumptions,
not recovered original parameters.

`--demo-locomotion --frames 230` runs a repeatable movement-only sequence without
opponent AI or combat. The host/header build and five sanitizer suites
(`native_gred_locomotion`, `native_battle_mvp`, `native_borg_movement`,
`native_battle_mvp_borg`, `native_animation_catalog`) passed. A real-model
230-tick headless run completed with health100/100 and no projectiles.
Contact Sheet07 records actual GL directional-dash, Air Dash and landing
captures at ticks66/96/186/219:

```
local/recomp-evidence/contact-sheets/classified-gred-v1/contact-sheet-07.png
```

The next transition pass makes takeoff sample the complete source clip within
its six-tick gameplay window, and shortens G-Red's takeoff/apex/landing blends to
two ticks so those phases can settle before they end. Sasuke's six-tick blend
policy is unchanged. Ground dashes cancel when leaving a ledge; jumping out of a
burst and other dash exits return to bounded normal movement speed rather than
retaining dash-speed air drift. Interrupted ground bursts preserve body heading
on the interruption tick. These remain native MVP choices, not original tuning.

Validation: the host/header build and the same five sanitizer suites passed,
including747 controller checks and270 shared-pose/lifetime checks with both
original normal models. A720-frame movement-only headless run exercised two
scripted restart cycles. Contact Sheet08 compares actual GL takeoff/landing
captures before and after at identical ticks166/219:

```
local/recomp-evidence/contact-sheets/gred-transitions-v1/contact-sheet-08.png
```

### Bounded wall-slide pass

If terrain blocks a diagonal move, the native controller now probes the X-only
and Z-only alternatives and takes the longest valid displacement. Each probe
reuses the shared swept actor-contact test and the existing terrain adapter.
At most two additional terrain queries are made per actor; unknown terrain
still refuses the tick and retires the world, rather than becoming guessed
floor. Straight-on contact and blocked corners remain stopped, and walkable
steps retain the original diagonal move.

Dash contact interrupts the burst while retaining free-axis displacement and
body heading for that tick. Distance-driven animation reads actual displacement,
not the requested diagonal. This is explicitly an axis-aligned MVP fallback,
not recovered wall physics: arbitrary wall normals, continuous terrain sweeps,
ceiling contacts and moving platforms remain unsupported.

The new `native_borg_terrain` sanitizer suite passed327 checks for free-axis
sliding, corners, walkable steps, dash interruption, actor-contact sweeps,
missing-terrain refusal and repeatable fixed-input sequences. The five existing
controller/duel/pose/catalog sanitizer suites also passed. No new animation
bindings were inferred from the uncertain conditional-air-dash notes.
The host/header build passed, as did a710-frame headless movement run (one
scripted restart, final round tick350) and a300-frame combat run ending64/0.
A90-tick actual GL smoke capture exchanged damage at84/76:

```
local/recomp-evidence/contact-sheets/gred-wall-slide-v1/battle-smoke.png
```

That capture checks renderer/combat regression, not a visual proof of wall
contact; the wall-slide cases above use bounded synthetic terrain fixtures.

Stage props, exact weapon attachments/effects, sophisticated collision response,
full AI, waves and audio remain incomplete.

Accuracy work comes later, guided by visible errors and targeted source checks
(for example muzzle bones, animation choices and attack offsets). Story
integration, full waves/rosters, exact effects and audio are subsequent milestones.
