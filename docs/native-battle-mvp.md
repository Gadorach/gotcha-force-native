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

For a ledge drop, `LEDGE DEPARTURE` precedes `LEDGE FALL ENTRY` and
`LEDGE FALL`. The fall entry starts the accepted drift-lock countdown;
`DRIFT LOCKED` shows its remaining ticks and `DRIFT UNLOCKED` appears if it
expires while still falling without steering. `DRIFT ACTIVE` reports an
accepted direct drift update, not just an arrow press. After touchdown the
HUD can show the retained ledge timer, but no longer shows a drift cue. These labels report the
accepted controller state; they do not change what inputs or contacts are
allowed.

Pause and restart are sampled each rendered frame, even if no 60 Hz movement
tick is due. Resuming does not catch up elapsed pause time.
While paused, press P to attempt one movement tick with the current controls.
Practice stays paused afterward, and holding P does not repeat. A refused
input does not advance the tick.
Press C to switch between the standard view and a close view of G-Red.
This changes framing only; it does not change the target-relative movement
basis, body heading, or controller permissions.
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

An explicitly selected [qualified normal G-Red package](borg-packages.md#qualified-normal-g-red-practice-package)
can supply the player body, rig and controller for the same practice
session. The same path can read the exact variant of an
[enabled owned copy](borg-packages.md#practice-an-owned-copy) from a
selected save without changing it. Stage and target assets stay in the
existing harness.

The [saved Force slot entry point](borg-packages.md#practice-one-saved-force-slot)
uses the same practice session after checking both records for the exact
requested position. The [saved Force browser](borg-packages.md#browse-a-saved-force)
shows those positions and can opt into one-copy practice without editing
them. Neither launches a Force battle.

### Try a demo route with your own inputs

Add `--practice-start` to an interactive G-Red practice command to use its
demo's grounded placement and arena bounds. The simulation waits for live
input; it does not inject any of the demo's movements. **Q** returns to the
same start with a fresh controller. Omit the flag for the ordinary practice
spawn. Scripted demos and unrelated host modes do not accept the flag.

```sh
gf_native --gred-walk-ledge-expiry-push --practice-start --no-audio
gf_native --gred-inclined-walk-ledge --practice-start --no-audio
gf_native --gred-inclined-dash-ledge --practice-start --no-audio
```

For the expiry route, hold **Left Arrow** until G-Red walks off, then
release. The incline walk uses **Up Arrow**; the incline dash uses a
right flick (**D** on keyboard). These are native practice starts, not
original game spawns or new movement permissions.

`--gred-inclined-dash-ledge-relaunch --practice-start` opens a
paused guide on that same incline. Step with **D** on 6 and a
neutral **Shift** tap on 8. The relaunch follows the accepted
fall, spends 150, and keeps the ledge countdown. Use
`--demo-gred-inclined-dash-ledge-relaunch` for playback. This
does not add dash-origin steep contact.

The [owned collection practice launcher](borg-packages.md#practice-an-owned-copy)
offers the same live footholds through its **Left/Right** route selector.
In the [Force browser](borg-packages.md#browse-a-saved-force), use **L/R**
instead so the directional controls can still browse the saved roster.
From Standard, select the previous route twice for **Expiry Air Dash**;
after the zero-timer push, flick left (**A** on keyboard).

Two live **timed ledge** guides start from the walking ledge-push
foothold. Both use Right Arrow on 6–8, then wait through four ordinary
falling updates after the last push. While the ledge timer is still
positive, hold Shift on 30–34 for relaunch or flick W on 30 for air
dash. Direct action from the push still refuses.

```sh
gf_native --gred-walk-ledge-timed-relaunch --practice-start --no-audio
gf_native --gred-walk-ledge-timed-air-dash --practice-start --no-audio
```

The separate `--gred-walk-ledge-direct-timed-air-dash --practice-start`
guide uses the steeper walking fixture. Step with Right Arrow on 6–8,
then flick **A+W** together on 28 after the fifth push. Only this
opt-in allows plain wind-up directly from the still-positive walking
push. Direct relaunch and entry contact responses still refuse.

`--gred-walk-ledge-timed-handoff-relaunch --practice-start` shows the
two-push route with **W** on 30, then **Shift** on 36 to take a plain
relaunch instead of active dash while the countdown remains positive.
The wind-up charge stays paid; the active-entry charge never occurs.

`--gred-walk-ledge-expiry-handoff-relaunch --practice-start` uses the
expired walking-push fixture instead. Step with **Left Arrow** on
6–25, **A** on 57, and **Shift** on 63. This handoff needs the accepted
expiry-push history all the way through plain wind-up, not just a
zero on the clock.

Both handoff guides are also available in the pickers with
`--practice-from-picker`. From **Standard**, select the previous route
three times for **Expiry Handoff**, or four for **Timed Handoff**
(**Left** in the owned picker, **L** in the Force browser).
When the accepted wind-up opens the handoff, the HUD shows
**SHIFT/B RELAUNCH / NEUTRAL DASH**. On the next step, tap Shift/B to
relaunch or leave the controls neutral to continue the dash. The prompt
doesn't override contact checks. Esc returns to the same copy or slot
and route without editing the save.

`--gred-walk-ledge-timed-relaunch-floor --practice-start` opens the
short floor-snap guide. Step with **Down+Right Arrow** on 6–122,
then **Shift** on 147–157, neutral otherwise. The HUD distinguishes
an airborne snap that damps descent from one that retains vertical
speed. Releasing after liftoff gives the separate touchdown and
landing. `--demo-gred-walk-ledge-timed-relaunch-floor` replays it
under the existing contact rules.

`--gred-walk-ledge-timed-snap-dash --practice-start` follows the timed
relaunch snap into air dash. Step with **Down+Right Arrow** on 6–122,
hold **Shift** on 148–150, and tap **D+S** on 150 while still holding
Shift. The wind-up starts at the same height and meter as the
preceding snap, while the ledge timer is still positive. The two
dash charges and actual landing come later. Use
`--demo-gred-walk-ledge-timed-snap-dash` for the automatic replay.

`--gred-walk-ledge-timed-snap-handoff --practice-start` gives a choice
at the end of that floor wind-up. Follow the same inputs through
150, then wait for **SHIFT/B RELAUNCH / NEUTRAL DASH** at 155.
Tap **Shift** without a direction on 156 to relaunch, or leave the
controls neutral to dash. Both keep the 450 cost already paid.
Use `--demo-gred-walk-ledge-timed-snap-handoff` for the relaunch replay.

`--gred-walk-ledge-positive-snap-handoff --practice-start` is the
shorter route with time still left. Step with **Up Arrow** on 6–8,
**Shift** on 25–26 and **W** on 26. At 31 the paid wind-up has seven
ticks left. Tap neutral-direction **Shift** on 32 to relaunch with
six ticks remaining, or leave the controls neutral for active dash.
`--demo-gred-walk-ledge-positive-snap-handoff` replays the relaunch.

`--gred-walk-ledge-expiring-snap-handoff --practice-start` is the
separate expiry-step guide. Use **Up Arrow** on 6–25, **Shift** on
46–47 and **A** on 47. On the accepted paid wind-up at 54, the
HUD shows one ledge tick left. Tap **Shift** without a direction
on 55 to relaunch as it reaches zero, or stay neutral to dash.
The earlier positive and expired modes keep their old clock gates.

`--gred-walk-ledge-active-snap-relaunch --practice-start` compares
relaunch with the active entry already paid. Use **Up Arrow** on
6–8, **Shift** on 25–26 and **W** on 26. Stay neutral through 32,
then tap **Shift** on 33. The 450 wind-up and 50 active-entry
charges stay spent. Tap **Up+Shift** instead on 33 to compare
the directional speed cap with neutral damping.

With `--practice-from-picker`, both browsers offer the floor-snap and
timed relaunch guides. From **Standard**, press **Right** in the owned
picker or **R** in the Force browser:

| Presses | Guide |
| ---: | --- |
| 6 | **Positive Floor Handoff** — choose with seven ledge ticks left |
| 7 | **Expired Floor Handoff** — choose after the timer has expired |
| 8 | **Timed Snap Dash** — follow the snap into active dash and landing |
| 9 | **Expiry Step Handoff** — choose on the actual 1 → 0 update |
| 10 | **Direct Timed Relaunch** — neutral Shift after the accepted walking push |
| 11 | **Direct Timed Up Relaunch** — soft Up + Shift from the same push |
| 12 | **Active Snap Relaunch** — relaunch after the active entry has already been paid |
| 13 | **Relaunch on Expiry** — plain Up + Shift entry on the timer’s 1 → 0 step |
| 14 | **Entry Push on Expiry** — steep entry push on that same timer step |
| 15 | **Adjacent Entry Push** — the neighboring face 1998 on 1 → 0 |
| 16 | **Air Dash on Expiry** — stationary wind-up as the timer reaches zero |
| 17 | **Expiry Wind-up Relaunch** — choose after the six-tick wind-up pays 450 |
| 18 | **Expiry Wind-up Snap** — same-height snap on the 1 → 0 entry, then dash |
| 19 | **Expiry Snap Relaunch** — choose relaunch after the snap's paid wind-up |
| 20 | **Diagonal Expiry Relaunch** — five-tick wind-up followed by Down drift on release |
| 21 | **Timed Release Push** — one airborne push after a neutral relaunch tap |
| 22 | **Two Timed Release Pushes** — two consecutive airborne pushes on the same face |

There are 32 routes in the loop. In the opposite direction (**Left** or
**L**), **Expiry Wind-up Snap** takes 14 presses and **Expiry Snap Relaunch**
takes 13. **Diagonal Expiry Relaunch**, **Timed Release Push**, and
**Two Timed Release Pushes** still take 12, 11, and 10 respectively.

**Q/Y** opens the paused guide. **P** steps with live input, and
**Esc** returns to the same copy or slot and route. These are read-only
practice routes, not changes to movement or battle admission.

`--gred-inclined-dash-jump --practice-start` opens the flat-to-incline
dash-jump guide. Step with **S** on 6, then **Shift** on 14–37.
**DASH CANCELED** appears on accepted takeoff. The old dash does not
move G-Red again, and the flight meter is first charged on lift.

`--gred-timed-incline-landing-dash --practice-start` starts a separate
short guide: **Left Arrow** on 6–9, then **A** on 34. Touchdown and landing
take separate steps before the dash can cross the incline. **DASH FROM
LANDING** is shown on the accepted entry, with five ledge-timer ticks left.
Use `--demo-gred-timed-incline-landing-dash` for the automatic replay.

Both guides are available with `--practice-from-picker`. From
**Standard**, select the next route twice for **Timed Relaunch Floor**
or three times for **Incline Dash Jump** (**Right** in the owned picker,
**R** in the Force browser). **Q/Y** opens the guide paused, **P** steps
with real input, and **Esc** returns to the same copy or slot and route.

Select the next route once from Standard (**Right** in the owned picker,
**R** in the Force browser) for **Recovery Seam**. It starts paused at
tick 0. Press **P** to step once while holding the indicated movement
key. The HUD shows the last accepted tick; the guide lists the ticks
you're stepping into: **Shift** on 6–39, **W** on 43, and **Right Arrow**
on 105–109. Leave the controls neutral between those ranges. **Enter**
resumes real-time play, **Q** restarts paused, and **Esc** returns to the
same picker selection and route. Input is never injected.

The same guide is available with
`gf_native --gred-recovery-first-contact-seam --practice-start --no-audio`.
Without `--practice-start`, that mode begins unpaused.

To relaunch instead, use **Ledge Timer Expiry**. Wait for **Drift Unlocked**
after the zero-timer push, then press **Shift**. In single-step practice,
tick 57 is the neutral fall; hold Shift while stepping ticks 58–77 to
match `--demo-gred-walk-ledge-expiry-relaunch`. In that route, pressing
Shift directly after the push still refuses.

`--gred-walk-ledge-expiry-direct-relaunch --practice-start` is a separate
opt-in guide for **Left Arrow** on 6–25, then **Up Arrow + Shift** on
57–76. The relaunch starts directly after the accepted zero-timer push,
spends 150 without an air-dash charge, and initially slows descent.
Use `--demo-gred-walk-ledge-expiry-direct-relaunch` for the automatic
replay. A positive timer or any contact response on entry remains
unsupported in this route.

`--gred-walk-ledge-direct-timed-relaunch --practice-start` instead
uses **Up+Left Arrow** on 6–7, then **Shift** on 25–32. The neutral
relaunch directly follows the third accepted walking push and
lifts with 13 ledge ticks left. Use
`--demo-gred-walk-ledge-direct-timed-relaunch` for the replay.
No direction or contact response is admitted on this entry.

`--gred-walk-ledge-direct-timed-up-relaunch --practice-start` uses
the same foothold but holds **Up Arrow + Shift** on 25–32. The turn
comes from that input, with the same 150 cost and 13-tick remainder.
Only soft Up is added on entry; the neutral-only guide and contact
checks are unchanged.

`--gred-walk-ledge-expiring-direct-relaunch --practice-start` starts
on a different foothold. Step with **Left Arrow** on 6–7, tap
**Up Arrow + Shift** on 38, then stay neutral. On 37 an accepted
walking push leaves one ledge tick. On 38 the plain relaunch spends
150 as the clock drops to zero; G-Red is still descending. This
is neither the already-expired push route nor the floor wind-up
handoff. Use `--demo-gred-walk-ledge-expiring-direct-relaunch`
for automatic playback.

`--gred-walk-ledge-expiring-relaunch-push --practice-start` is the
separate steep-contact version. Step with **Left Arrow** on 6–25,
hold **Up + Shift** on 56–59, then stay neutral. The 1 → 0 entry
push costs 150, keeps G-Red airborne, and is not a floor snap.
The three held updates each cost 100. **Up** alone on 57 also
works after a tap on 56; releasing into neutral immediately
does not pass the next travel check. Use
`--demo-gred-walk-ledge-expiring-relaunch-push` for the held playback.

`--gred-walk-ledge-expiring-adjacent-relaunch-push --practice-start`
uses the same inputs from a foothold two units farther along Z to
select adjacent face 1998. The **Adjacent Entry Push** picker guide
uses this opt-in; **Entry Push on Expiry** still uses face 1999 only.

`--gred-walk-ledge-expiring-air-dash --practice-start` opens the
air-dash version of the 1 → 0 timing. Step with **Left Arrow**
on 6–7, tap **A** on 38, then stay neutral. Entry is stationary
and free; the existing two wind-up pushes come afterward. The
450 handoff and 50 active-entry charges are separate. Use
`--demo-gred-walk-ledge-expiring-air-dash` for playback.

`--gred-walk-ledge-expiring-air-dash-snap --practice-start` is
the separate same-height contact version. Step with **Left Arrow**
on 6–9, tap **A** on 40, then stay neutral. The entry snap clears
vertical speed without moving, grounding, or charging G-Red.
The handoff and active entry still cost 450 and 50 on 46–47.
Use `--demo-gred-walk-ledge-expiring-air-dash-snap` for playback.
In either picker, this is **Expiry Wind-up Snap**.

`--gred-walk-ledge-expiring-air-dash-snap-handoff --practice-start`
uses the same start and inputs. After the wind-up pays 450 on
46, tap neutral **Shift** on 47 to relaunch for 150, or stay
neutral to dash for the usual active-entry cost. Use
`--demo-gred-walk-ledge-expiring-air-dash-snap-handoff` for the
relaunch playback. In either picker, choose **Expiry Snap Relaunch**.

`--gred-walk-ledge-expiring-air-dash-handoff --practice-start`
uses that same start and inputs, then shows the choice when the
wind-up pays 450 on 44. Tap **Shift** without a direction on 45
to relaunch for 150 instead of entering active dash, or leave
the controls neutral for the usual dash. Use
`--demo-gred-walk-ledge-expiring-air-dash-handoff` for the relaunch replay.

`--gred-walk-ledge-expiring-diagonal-dash-handoff --practice-start`
uses **A+W** on 38 after the same **Left Arrow** 6–7 start. This
shorter wind-up pays on 43. Tap neutral **Shift** on 44, release
Shift, and hold **Down Arrow** on 45–50, then stay neutral.
Use `--demo-gred-walk-ledge-expiring-diagonal-dash-handoff` for
playback. In either picker, choose **Diagonal Expiry Relaunch** for
this route. The existing six-tick guide is unchanged.

`--gred-walk-ledge-timed-release-push --practice-start` shows the
separate contact after release. Step with **Right Arrow** on 6–8,
tap **Shift** on 23, then leave the controls neutral. Ordinary
falling starts at 24. The one accepted push at 33 keeps six ledge
ticks without grounding or refilling G-Red. Touchdown and landing
come later. Use `--demo-gred-walk-ledge-timed-release-push` for
the automatic replay.

`--gred-walk-ledge-timed-release-two-pushes --practice-start`
uses a separate start. Step with **Down+Right Arrows** on 6–7,
tap **Shift** on 20, then stay neutral. On 33–34, G-Red pushes
twice on the same face, staying airborne with five and then four
ledge ticks. Use `--demo-gred-walk-ledge-timed-release-two-pushes`
for automatic playback.

`--gred-walk-ledge-timed-release-three-pushes --practice-start`
uses **Up+Left Arrows** on 6–7, a **Shift** tap on 20, then neutral.
The three same-face pushes occur on 34–36 with four, three and
two ledge ticks left. This separate guide leaves the two-push
mode unchanged. Use `--demo-gred-walk-ledge-timed-release-three-pushes`
for playback.

From **Standard**, advance the route four times for **Timed Landing
Dash**, or five for **Direct Expiry Relaunch** (**Right** in the owned
picker, **R** in the Force browser). On the accepted zero-timer push,
the direct relaunch guide shows **UP+SHIFT/B TO REQUEST RELAUNCH** for
the next step. It does not skip input or contact checks. After the
next accepted step, the hint clears whether you relaunch or keep falling.

`--gred-recovery-recontact` uses the ordinary practice start and opts into
the verified flat-floor recovery overlap. Try
`--demo-gred-recovery-recontact --headless --frames 200 --no-audio` to see
the difference between **Dash Recovery / Floor Contact**, **Touchdown**,
and **Landing**. Keep controls neutral for the update after floor contact.
The separate `--gred-recovery-recontact-drift` mode lets you hold a single
arrow through both contact updates if G-Red stays on the same flat
triangle. The scripted `--demo-gred-recovery-recontact-drift` holds Up on
updates 108–109 and shows **Drift Active** on those accepted frames only.
`--gred-recovery-recontact-seam` also supports a verified shared flat edge
on the second update only. The `--demo-gred-recovery-recontact-seam`
replay holds Right instead of Up and shows **Flat Seam** at touchdown.
`--gred-recovery-first-contact-seam` also admits a verified flat edge on
the descending first contact. In its demo, Right starts three updates
earlier and **Flat Seam** appears one update before touchdown.

For a steep push on the completion update, use
`gf_native --gred-recovery-completion-push --practice-start --no-audio`.
The guide starts paused. Step with **Shift** on 6–38, **W** on 46–72,
and **Left Arrow** on 111–112, neutral otherwise. The
`--demo-gred-recovery-completion-push` replay shows **Recovery Complete**
on the push, then separate touchdown and landing. It uses the existing
combined recovery-push policy, not a new contact allowance.

`gf_native --gred-recovery-completion-exit --practice-start --no-audio`
opens the nearby diagonal-exit guide. Use the same Shift and W timing,
then **Left** on 111 and **Up+Left** on 112. Left alone on 112 still
refuses the raised-side contact on this start. The scripted
`--demo-gred-recovery-completion-exit` shows the accepted **Exit Drift**
cue at touchdown. Use **Q** to restart a refused route, not to retry
from the stopped contact.

To check the actual practice loop without opening a window or audio device:

```sh
python3 -B tools/build_native.py --practice-replay --jobs 1
```

The replay links the desktop host but uses a fake clock and scripted input. It
checks session controls between movement ticks, short taps, separately sampled
jump and dash, resuming after a refused input, restart after a terrain stop, and
live input and restart at the expiry fixture.
It also tests the paused Recovery Seam guide and the unmodified timing
from live inputs in each picker.
It skips if the qualified local stage or movement pack is absent.

[Contact Sheet 59](../local/recomp-evidence/contact-sheets/gred-movement-practice-v1/contact-sheet-59.png)
shows actual offscreen OpenGL captures. All four rendered bar widths matched
the accepted meter within one pixel. Host tests covered both launch commands;
the owner sanitizer covered pending frames, pause, foreign worlds and retired
receipts. The ordinary duel smoke test was unchanged.

## Ground dash on one slope triangle

`--gred-slope-dash` is an opt-in practice mode with the same controls and HUD.
It allows an ordered ground dash to follow one inclined static triangle,
using the source tangent and selected height. The native step, body, and bounds
checks still apply. Cross-triangle slopes and nonflat ledge departures remain
refused; ordinary `--gred-movement` retains the flat-only dash policy.

`--demo-gred-slope-dash --frames 100` runs a downhill dash on original
triangle 1014 from a native grounded fixture. The owner tests also exercise
an uphill dash, reject off-tangent motion and flat-height substitutes, and
keep the older mode and slope seams closed.

`--gred-slope-seam-dash` additionally admits adjacent triangles only if their
stored plane equations and shared edge match exactly. Creases, gaps, nonflat
ledge departures, and obstructed travel still refuse. The original
`--gred-slope-dash` mode remains single-triangle. Try the verified crossing
from triangle 254 to 255 with `--demo-gred-slope-seam-dash --frames 100`.

`--gred-slope-crease-dash` also allows shallow joins between two inclined
faces, up to a 10° change in normal. Both sides must share an exact edge and
agree on its height within 0.01 native units. The existing step, body, and
bounds limits still apply. The 10° cutoff is a native safeguard, not a recovered
game constant. Try `--demo-gred-slope-crease-dash --frames 100` for the
verified crossing from triangle 295 to 508, a turn in the slope of about 6.2°.

`--gred-slope-transition-dash` also permits an ordered dash between flat
ground and a shallow incline, in either direction. It uses the same 10°
native limit, shared-edge and height checks, and native step/body/bounds
limits. The earlier modes keep their narrower policies. Run
`--demo-gred-slope-transition-dash --headless --frames 100 --no-audio` for
the flat-to-incline route from triangle 249 to 1052. The owner test also
replays the incline-to-flat route from triangle 245 to 890. Both joins are
about 9.5°; neither spends flight meter or leaves the ground.

`--gred-inclined-dash-ledge` additionally enables one narrowly verified
departure topology for normal G-Red. An ordered downhill dash can leave the
incline over a connected steep strip onto a lower flat floor; the departure
keeps its height and the existing fall and landing own subsequent frames.
Older modes do not gain this permission. Try
`--demo-gred-inclined-dash-ledge --headless --frames 100 --no-audio`.

`--demo-gred-inclined-walk-ledge` replays the existing ordinary walk
handler across the same authored cliff band from a separate grounded
placement. It leaves on tick 8 with no ordered-dash root update, touches
down at 23, and returns to idle without spending flight meter.

`--gred-walk-ledge-fall-push` additionally permits a verified continued
push during a descending walk-origin fall while its ledge timer is still
positive. The source plane and native travel before and after projection
must agree. Run `--demo-gred-walk-ledge-fall-push --headless --frames 120
--no-audio` for two pushes, a later separate landing, and unchanged meter.
The older modes do not gain this permission.

`--gred-walk-ledge-high-angle-push` adds the verified steeper face range to
that same walking response, up to the measured native cap of 13653 (about
75°). It does not change either landing gate, accept an expired timer, or
skip either travel check. `--demo-gred-walk-ledge-high-angle-push --frames 120`
shows three inter-gate pushes followed by two steeper pushes and a later
landing. The inter-gate mode keeps its original cap.

`--gred-walk-ledge-expiry-push` adds just the first zero-timer update,
with no steering drift and only after an accepted timer-one walking push.
`--demo-gred-walk-ledge-expiry-push --frames 180` shows the two pushes,
the ordinary fall that follows, and a separate landing. It does not enable
general zero-timer or dash-origin pushes.

`--demo-gred-walk-ledge-expiry-drift --frames 180` uses the same mode and
holds Up for 36 updates after the neutral expiry push. It moves the
landing location without another push, body turn, or meter spend.

`--gred-walk-ledge-expiry-air-dash` adds a plain air-dash wind-up directly
after the accepted neutral expiry push. Try
`--demo-gred-walk-ledge-expiry-air-dash --headless --frames 300 --no-audio`
for the separate wind-up, costs, touchdown and idle. A positive-timer
push still cannot use this handoff.

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
