# G-Red's original animation-controller layout

Manual classification is paused. Existing labels are useful visual references,
not a prerequisite for recovering the controller. This investigation is scoped
to **normal G-Red (`0615`) and the shared routines it actually uses**.

User-observed cross-Borg differences to preserve: Sasuke's animations appear
similar in intent, but his three-point air-jump gauge resets on landing and is
not spent by air dashes; successive midair dashes gradually lose height. G-Red
instead has a grounded-refilling flight meter that is also consumed by midair
dashes. These are per-Borg observations to verify against each controller, not
permission to copy G-Red's timings, resource policy or motion map to Sasuke.
Additional user observations about hover contact and perpetual-flight boost
recovery are recorded in [candidate movement families](borg-movement-families.md).

## Air-dash recovery completion on a flat floor (GFMM9)

`--gred-recovery-recontact` admits one narrow overlap between recovery
completion and floor contact. Other modes still refuse it. In the initialized
DOL, `617E4` calls `677B0` before reading the lower-lane completion byte.
The `65E64` callback zeros vertical speed and acceleration and selects 7.
`6A750` does not clear the lower completion byte, so the recovery tail can
still overwrite selector 7 with selector 6 on the same update. The snapped
height and zero vertical speed are retained, along with planar momentum.
Landing program 8 hasn't run and there is no meter refill on that frame.

```sh
gf_native --demo-gred-recovery-recontact --headless --frames 200 --no-audio
```

The replay uses the ordinary native spawn on the unchanged stage: hold
Shift for updates 6–39, release, flick backward at 43, then leave the
controls neutral. Six candidates reached the old refusal in a bounded
200-route scan. The selected route completes:

| Accepted tick | Result |
| --- | --- |
| 80 | Program 12 starts; 5750 meter |
| 108 | Floor contact on triangle 1631, lower recovery completes; both lanes still use program 12 and selector becomes 6 |
| 109 | Neutral program 13 update confirms touchdown on the same triangle, without another descent |
| 110 | Separate program 8 landing entry; meter still 5750 |
| 111 | Refill starts; idle eventually reaches 10000 |

The source owner seals the actual recovery completion before native acceptance.
The native check independently requires continued recovery with the previous
travel heading, zero timer698, the same selected flat triangle for approach,
contact and recontact, and the existing travel/body checks. The next update
must be neutral ordinary falling with the same meter, not a dash, lift, drift
or forced landing animation. The
first frame shows **DASH RECOVERY / FLOOR CONTACT**, not **TOUCHDOWN**.
Slopes, changed planes, positive timers and other overlapping transitions
are not covered.

### Soft drift through both contact updates

`--gred-recovery-recontact-drift` additionally accepts soft cardinal
input on either of the two contact updates, only if each path stays on
the same verified flat triangle. The source planner uses the existing
four-unit camera-relative drift before retained planar momentum; the
native check pairs the sealed input with its own camera observation. It
does not change body heading, timer, meter, or the program 12 → 13 → 8 order.
The earlier neutral mode still refuses drift at these contacts.

```sh
gf_native --demo-gred-recovery-recontact-drift --headless --frames 200 --no-audio
```

The replay holds **Up** on ticks 108–109 of the same grounded setup. It
lands and refills at the same times, about eight units from the neutral
landing. The HUD shows **DRIFT ACTIVE** only on the two accepted updates.
Fifteen of twenty bounded timing/direction combinations completed. In this
mode, holding **Right** at the replay's timing accepts the first contact but
refuses recontact when the selected triangle changes. Diagonal input, a
full flick, and relaunch input do not qualify for the recontact update.

### One verified shared flat edge on the second contact

`--gred-recovery-recontact-seam` adds one exact shared edge to the steered
second contact only. The first contact still requires one triangle. Both
source and native sample the previously accepted start and the proposed
endpoint, require equal heights and flat normals, and use the existing
coplanar crossing verifier to prove exact shared vertices, identical
planes, and a path through the edge interior. The native travel checks run
before and after contact. Equal height on its own, a corner, a gap, a step,
or a crossing without soft cardinal drift do not qualify.

```sh
gf_native --demo-gred-recovery-recontact-seam --headless --frames 200 --no-audio
```

On the same native setup, hold **Right** on ticks 108–109. Tick 108 stays
on triangle 1631, and tick 109 touches down on 1624, both at Y=10.
**FLAT SEAM** appears only on the accepted crossing. Landing is separate
at 110; the meter stays at 5750 through landing entry and refills
normally. Two other probe timings also completed exact second-contact
crossings. Neither earlier mode got new permissions.

### A shared flat edge on the descending contact

`--gred-recovery-first-contact-seam` additionally permits the first
recovery contact to cross one exact shared flat edge. It requires soft
cardinal drift, zero timer698, and a descent from strictly above the
starting plane. The source includes that starting triangle in the offer;
native acceptance samples it independently before verifying the edge
and both travel checks. The first contact still finishes program 12,
not touchdown or landing. Earlier modes keep their first-contact refusal.

```sh
gf_native --demo-gred-recovery-first-contact-seam --headless --frames 200 --no-audio
```

Begin **Right** three updates earlier than in the second-contact demo
(hold on 105–109). The crossing 1631 → 1624 is accepted on tick 108
while descending; program 13 confirms touchdown on 1624 at tick 109.
**FLAT SEAM** appears on 108 but not 109. Landing at 110 and the meter
refill are unchanged. A separate **Up** route held on 81–109 crosses
1630 → 1631 on 108 and then 1631 → 1624 on 109. Each frame proves its
own edge; the first proof cannot authorize the second.

## Continued walking ledge-fall push (GFMM9)

`--gred-walk-ledge-fall-push` opts into a descending, continued program 13
push after an accepted **walking** departure. It requires positive owned
timer698 and a selected angle greater than 6144 and at most 10240. Fall
entry, dash-origin falls, held jump, expired timers, steeper angles, and
air-dash or relaunch handoffs from the push are not enabled by this change.
Earlier modes keep their refusal.

The native receipt carries walking provenance from departure. It checks
the usual 0.99 retained momentum on approach, consumes the integrated
descending velocity and selected source plane, then applies the existing
0.9 scalar/velocity damping. The exact projected endpoint and both native
travel checks must pass. There is no landing callback, timer reset, meter
spend, or refill on a push. A solver candidate on a raised plane still
refuses and cannot publish a pose, timer, or root update.

A bounded original-stage scan of walks and dashes from inclined starts
checked 1,253 routes before finding four source/native travel witnesses.
Nearby grounded variations found three complete push-and-landing routes.
The demo starts at native X/Z `1000.65021/-3363.34546`, floor query
`1549.25293`, and uses **D-pad Right** after five idle ticks, then neutral:

| Accepted tick | Result |
| --- | --- |
| 8 / 9 | Walking departure / separate fall entry |
| 24 / 25 | Face 1953 pushes, angle 8191, timer 15 / 14 |
| 77 / 78 | Flat touchdown / separate landing program 8 |
| 120 | Grounded idle, meter still 10000 |

```sh
gf_native --demo-gred-walk-ledge-fall-push --headless --frames 120 --no-audio
```

The nearby refusal fixtures include an out-of-range face after three
accepted pushes, a steep candidate after timer expiry, and the earlier
raised-side approach. Native clocks, terrain queries, body response,
and the demo spawn are not original hardware or whole-actor equivalence.

### Relaunch and air dash after four ordinary falling updates

The same grounded start can use either action while timer698 is still
positive. Both replays accept the same pushes at 24–25, then four
ordinary falling updates at 26–29. The input on tick 30 uses the
ordinary fall as its predecessor and decrements the timer from 10 to 9.
Neither action can start directly from the push. No contact permission
or asset changed.

```sh
gf_native --demo-gred-walk-ledge-timed-relaunch --headless --frames 180 --no-audio
gf_native --demo-gred-walk-ledge-timed-air-dash --headless --frames 180 --no-audio
```

Hold **Right Arrow** on 6–8, then neutral. For the relaunch, hold
**Shift** on 30–34: the 150 entry charge is followed by four 100-unit
charges. Falling resumes at 35 with 9450 meter and timer 4, touchdown
at 96, and separate landing at 97. For the air dash, flick **W** on
30: wind-up entry is stationary and free, the 450 handoff charge
lands on 35 with timer 4, and the separate 50 active-entry charge
on 36 with timer 3. Falling resumes at 96 with 8000 meter, touchdown
at 122, and landing at 123. Both reach full-meter idle. Timer698
expires at 39 in both, without interrupting the running dash or fall.

Add `--practice-start` to the corresponding live command (without
`--demo-`) for a paused guide on the same native start. Inputs stay live.

### Steeper continued walking contact

`--gred-walk-ledge-high-angle-push` preserves the same walking provenance,
positive timer, descending velocity, and separate native checks before and
after projection. It raises only the opt-in native ceiling from 10240 to
13653 (the steepest witnessed face, about 75°). The original landing gates
remain 6144/10240. The earlier walking-push mode still stops at the stronger
face; the new mode verifies the qualified source angle and displacement
epsilon operands when binding.

```sh
gf_native --demo-gred-walk-ledge-high-angle-push --headless --frames 120 --no-audio
```

This is the earlier refusal setup, one native unit away in Z from the
inter-gate demo: X/Z `1000.65021/-3362.34546`, query `1549.25293`, and
the same D-pad Right then neutral. After three pushes on face 1953 at
ticks 23–25, it accepts two more on face 1949 at ticks 26–27 (angle
13653, timer 13/12). Touchdown at 80 and landing at 81 remain independent
of those pushes. The meter stays at 10000 throughout. An expired timer,
blocked approach or projection, and angles above the verified ceiling
still refuse.

### One continued push on the walking timer's expiry tick

`--gred-walk-ledge-expiry-push` admits one neutral push at the natural
counter transition **1 → 0**, immediately after an accepted timer-one
walking push. It does not authorize ordinary zero-timer pushes, steering
drift on the expiry-push update, or dash-origin pushes. The response uses
the expired landing gate of 10240 instead of 6144, retains the native
ceiling of 13653, and requires the same selected plane and both native
travel checks. No timer reset, ground callback, meter spend or refill is
part of the push.

```sh
gf_native --demo-gred-walk-ledge-expiry-push --headless --frames 180 --no-audio
```

The demo uses a native grounded placement at X/Z
`-1250.96863/-3313.80225`, query `1846.01367`, and D-pad Left after
five idle ticks, then neutral after departure. It departs at 25, enters
the fall at 26, pushes on face 2009 at tick 55 (angle 8192, timer 1),
then pushes on face 1999 at 56 (angle 13653, timer 0). Tick 57 is an
ordinary zero-timer fall; touchdown at 103 and landing at 104 are
separate. It returns to idle at full meter. The high-angle mode without
expiry permission still refuses before tick 56.

Dash-origin pushes remain unsupported. A bounded scan of 284 ordered-dash
routes found eight matching source/native first-push candidates. A
separate isolated replay of 648 nearby variations accepted at least one
push in 274 routes, but none completed a clean fall and landing. These are
partial witnesses, not a reason to grant dash-origin permission or relax
collision checks.

A wider follow-up kept the same positive-timer projection and travel
checks. It found first pushes in 37 of 384 approaches with alternate
grounded target placements, and four of 2,720 stage-wide approaches with
the usual target placement. Neither scan reached a later touchdown after
a push. There is still no complete dash-origin push route for admission.

A further scan tested 2,952 approaches from grounded ledge starts,
also varying the walking lead-in to the dash. This produced 33 ordered-dash
departures and eight accepted first inter-gate pushes in isolation.
All eight refused the following contact before an ordinary fall could
be accepted, so there was no valid predecessor for a relaunch recovery.
Dash-origin push permissions remain unchanged.

### Direct drift after the expiry push

The existing zero-timer fall can steer **after**, not during, the neutral
expiry push. On the same grounded expiry fixture, D-pad Up adds the
four-unit camera-relative drift before retained momentum on accepted ticks
57–92, without turning the body. The final position is about 144 units
farther in Z than the neutral replay, with the same touchdown at 103,
landing at 104, and full meter. The HUD shows `DRIFT ACTIVE` only on
accepted falling updates that include that drift. The opposite input on
this fixture reaches an unsupported new steep contact and still refuses.

```sh
gf_native --demo-gred-walk-ledge-expiry-drift --headless --frames 180 --no-audio
```

This replay adds no movement permission. Immediate relaunch after a
walking push remains refused: an isolated 72-case replay yielded no
complete, settled route after that handoff, and 372 offered positive-timer relaunches
across 1,176 nearby trials yielded no accepted entry. The owner regression
retains a real staged relaunch candidate and checks that it publishes
neither a motion frame nor a meter spend.

### Air-dash wind-up after the expiry push

`--gred-walk-ledge-expiry-air-dash` opts into a **plain** wind-up directly
after an accepted walking expiry push. The preceding push and the new
entry must both carry zero timer698. A timer-one push followed by a
prospective decrement does not qualify. Neither a floor snap, steep
response, nor relaunch can borrow this entry permission. Earlier modes
still refuse this immediate handoff, and all existing contact checks run.

```sh
gf_native --demo-gred-walk-ledge-expiry-air-dash --headless --frames 300 --no-audio
```

On the same expiry fixture, D-pad Left starts the walk; neutral input
keeps the two pushes. A single left stick flick after accepted tick 56
produces wind-up at 57. It halves the incoming vertical speed without
moving or spending meter. The handoff spends 450 at 62; the separate
active entry spends 50 at 63. Ordinary falling resumes at 123 with meter
8000, then touchdown at 150 and landing at 151 are separate. Idle reaches
full meter. Six complete probe routes supported the handoff; the demo
uses the one-sample left flick.

The expiry mode still refuses a positive-timer push. An earlier isolated
scan offered 720 immediate cardinal flicks across 1,800 positive-timer
trials. It reached 712 wind-ups and 176 active entries without relaxing
travel checks, but no route completed. That partial evidence did not
grant permission.

### Direct air-dash wind-up with time remaining

`--gred-walk-ledge-direct-timed-air-dash` opts into a **plain** wind-up
directly after an accepted positive-timer walking push. The countdown
must still be positive after its ordinary decrement. An accepted
timer-one push cannot borrow the pending expiry; this does not admit
direct relaunch, a floor response or a steep push on the entry, or a
dash-origin ledge push. The older timed guides still require an
accepted ordinary fall. Existing native motion and contact checks are
unchanged.

```sh
gf_native --demo-gred-walk-ledge-direct-timed-air-dash --headless --frames 200 --no-audio
```

The diagonal-flick probe found five complete routes in 265 attempts.
An independent steer-after-entry probe found five in 177 attempts,
including a two-push inter-gate route. The demo uses the steeper walking
fixture at X/Z `1000.65021/-3362.34546`. **Right Arrow** on 6–8
leads to five pushes ending at tick 27 with timer 12. A single **A+W**
flick on 28 starts stationary wind-up with timer 11 and full meter.
The 450 charge comes on 34, the separate 50 on active entry at 35.
Timer expiry at 39 does not stop the dash. Falling resumes at 95,
then touchdown at 123 and landing at 124; idle refills normally.
**W** alone on this same start does enter the dash, but the next travel
after active entry still refuses a raised-side contact at 35.

### Air-dash wind-up on the 1 → 0 walking-push update

`--gred-walk-ledge-expiring-air-dash` adds one stationary
wind-up on the exact expiry step after the accepted timer-one
walking push on face 2001 (angle 8192). Entry halves the incoming
speed, moves no distance, spends nothing, and neither snaps nor
pushes. It does not create the already-zero entry's wind-up-to-
relaunch permission. The positive and already-expired modes
still refuse this pending decrement.

```sh
gf_native --gred-walk-ledge-expiring-air-dash --practice-start --no-audio
gf_native --demo-gred-walk-ledge-expiring-air-dash --headless --frames 175 --no-audio
```

From X/Z `-1067.30347/-3415.41772` (query `1846.01367`, arena
half-width `4500`), hold **Left Arrow** on 6–7, tap **A** on 38,
then stay neutral. The push on 37 leaves one tick. Wind-up
on 38 halves −27 to −13.5 as the timer expires. The two pushes
on face 2007 at 39–40 are existing continued wind-up contacts,
not part of the new entry response. Tick 44 spends 450 for the
handoff, 45 spends the separate 50 for active entry, and falling
resumes at 105 with 8000. Touchdown and landing at 132–133 keep
that meter until normal recovery.

The 3,888-route local probe accepted 2,136 stationary entries but
none reached the paid handoff. On the older face-2009 start,
the next wind-up push's pre-projection floor is above the actor,
even when the projected endpoint clears the floor. That refusal
is not relaxed. The stage sweep found 20 complete routes in 1,002
attempts, all from face 2001.

#### Steep contact on expiry-step wind-up entry remains closed

The stationary guide does not grant a steep push or snap on entry.
In the steep-entry probe, 40 offers were staged in 3,888 local
trials and 16 entered. All 16 refused a later wind-up push.
A tighter 1,782-trial sweep around the two accepted starts
staged 189 and accepted 106; none reached the paid handoff.
Another 2,756 stage-sweep attempts staged no projected entries.

One partial route starts at X/Z `-1242.96863/-3307.80225`
(query `1846.01367`, arena half-width `4500`). The scratch
gate accepts a face-1999 entry on 55. On 56, the pre-projection
endpoint still selects face 1999 at Y `1408.39746`, above the
actor's starting Y `1406.63159`. The post-projection endpoint
selects face 2185 at Y `10` and clears its floor, but that does
not make the approach valid.

#### Same-height snap on expiry-step wind-up entry

`--gred-walk-ledge-expiring-air-dash-snap` is a separate opt-in.
It adds the six-tick snap after the accepted 1-tick walking push
on face 2001. Both faces use angle 8192, but the snap must select
face 2000 with the exact same height and no displacement. Native
reselects the plane and checks the staged half-speed, zero-speed
tail, body and travel independently. Other faces, height changes,
steep pushes and relaunch handoffs are not added.

```sh
gf_native --gred-walk-ledge-expiring-air-dash-snap --practice-start --no-audio
gf_native --demo-gred-walk-ledge-expiring-air-dash-snap --headless --frames 175 --no-audio
```

The snap-only probe found two complete routes in 2,756
stage-sweep attempts. This guide starts at X/Z
`-1078.91504/-3407.48291` (query `1846.01367`, arena half-width
`4500`). Use **Left Arrow** on 6–9 and **A** on 40, then neutral.
The wind-up entry halves −27 to −13.5 before the contact tail
clears it to zero, at unchanged Y `1416.95068`. G-Red stays
airborne with full meter. The route pays for handoff at 46 and
active entry at 47. Falling resumes at 107, touchdown and
landing remain separate at 137–138, and idle refills normally.

#### Relaunch after the paid same-height snap wind-up

`--gred-walk-ledge-expiring-air-dash-snap-handoff` verifies one
different predecessor for plain rising relaunch: the accepted
1 → 0 snap above, followed by six ordinary wind-up updates
through the 450 charge. Source and native track that sequence
separately from the two-push stationary-start sequence and the
flat-floor wind-up. Another push, snap, clock or early request
cannot borrow it. The snap-only mode still refuses the handoff.

Use the same start and **Left Arrow** on 6–9, then **A** on 40.
After the accepted paid handoff at 46, tap **Shift** without a
direction on 47 for relaunch, or stay neutral for active dash.
Relaunch keeps the 450 spent, costs 150 and skips the 50 active
entry. The incoming vertical speed of about −0.42 becomes
14.58 before displacement and 15.38 after acceleration. It
neither changes facing nor makes another contact. Release at 48;
touchdown at 131 and landing at 132 keep 9400 until idle.

The scratch probe staged 16 entries in 24 attempts; eight were
accepted and reached idle, across both verified snap starts.
The checked-in permission uses the proven sequence, not a generic
zero timer. Steep wind-up entry remains closed.

### Relaunch instead of active dash after that expiry-step wind-up

`--gred-walk-ledge-expiring-air-dash-handoff` adds a separate
choice on the paid handoff. Source and native each require the
stationary 1 → 0 entry from face 2001 with a six-tick wind-up,
two consecutive pushes on face 2007 at angle 13653, then four
ordinary wind-up updates through the 450 charge. A different wind-up
length, a missing push, another face, a floor snap or a generic zero
timer cannot earn this history. The standalone air-dash mode
still refuses this relaunch.

```sh
gf_native --gred-walk-ledge-expiring-air-dash-handoff --practice-start --no-audio
gf_native --demo-gred-walk-ledge-expiring-air-dash-handoff --headless --frames 145 --no-audio
```

Use the same grounded start, **Left Arrow** on 6–7 and **A** on
38. The paid wind-up at 44 shows **SHIFT/B RELAUNCH / NEUTRAL
DASH**. Tap **Shift** without a direction on 45 for a plain rising
relaunch, or leave the controls neutral for the unchanged active
dash. Relaunch keeps the 450 spent, charges 150 instead of the
50 active-entry charge, and leaves 9400. The one-tap release at
46 begins ordinary falling; touchdown at 111 and landing at 112
keep the spent meter until idle. A directional relaunch or entry
contact cannot borrow that choice.

The isolated probe staged and accepted 48 neutral entries; 20
reached landing and idle. The checked-in permission uses only
the complete six-tick sequence; accepting entry alone was not
treated as proof of a complete route.

### Five-tick diagonal wind-up with release drift

`--gred-walk-ledge-expiring-diagonal-dash-handoff` is a separate
opt-in. It verifies the same expiry entry and two face-2007 pushes,
but requires an actual five-tick wind-up and three ordinary updates
after the pushes. Both source and native distinguish the two
wind-up lengths; the six-tick mode still refuses this handoff.

From the same grounded start, use **Left Arrow** on 6–7, **A+W**
on 38, and a neutral **Shift** tap on 44. The wind-up pays 450
at 43; relaunch pays 150 instead of active dash's 50, leaving
9400. Release Shift and hold **Down Arrow** on 45–50, then
stay neutral. The six ordinary falling updates drift without
turning the body or spending meter. Touchdown at 110 and landing
at 111 keep 9400; normal idle restores full meter. Immediate
neutral release refuses at 45. One tick of Down is not enough
on this start; it refuses at 48.

The four-start probe found 12 complete routes in 272 trials, all
with a tap and sustained drift. The nearby-position sweep found
16 more with only one drift update, but from different starts.
No terrain, bounds or body check was relaxed for either sweep.

### Relaunch instead of active dash after a timed plain wind-up

`--gred-walk-ledge-timed-handoff-relaunch` accepts a plain relaunch
after a completed plain wind-up while the ledge countdown remains
positive. The preceding receipt must already have paid the wind-up
charge and selected active entry without running it. A floor snap,
steep response on relaunch entry, pending expiry, or already-expired
countdown cannot borrow this handoff. Direct relaunch from the
walking push is still closed: four entries in the 4,500 directional
trials never produced a complete route.

```sh
gf_native --demo-gred-walk-ledge-timed-handoff-relaunch --headless --frames 180 --no-audio
```

Use the two-push fixture: Right Arrow on 6–8, neutral, then **W** on 30.
The wind-up pays 450 at 35, with 4 ticks left. Tap **Shift** on 36
instead of entering active dash. That plain relaunch adds 15 to
vertical speed, spends 150, and leaves 9400 meter and timer 3.
It never charges the active-entry 50. Falling starts at 37, touchdown
at 103, and landing at 104; idle refills normally. A separate route with
direct timed dash and a longer Shift hold also completes this handoff.
The neutral-keyboard recipe is available as a live, paused
`--practice-start` guide.

### Relaunch after plain wind-up from the accepted expiry push

`--gred-walk-ledge-expiry-handoff-relaunch` adds one zero-timer
handoff. It needs an uninterrupted **plain wind-up** that starts
directly after the accepted walking expiry push and finishes before
active entry. Source and native carry that history frame by frame.
A zero timer by itself, a pending 1 → 0 timer, or an intervening
wind-up contact response cannot supply that history.

```sh
gf_native --demo-gred-walk-ledge-expiry-handoff-relaunch --headless --frames 200 --no-audio
```

On the expiry fixture, hold **Left Arrow** on 6–25, then neutral.
The expiry push is accepted at 56. Flick **A** on 57, wait through
the 450 wind-up charge at 62, then tap **Shift** on 63. Plain relaunch
spends 150, leaving 9400 with no active-entry charge. Release on 64;
touchdown is at 126 and landing at 127. A separately verified Up+Shift
hold also completes. Neither route admits relaunch directly from the
push. Use the live command with `--practice-start` to step either
input yourself.

### Relaunch after the first ordinary fall

In the earlier expiry and air-dash modes, relaunch cannot follow the
push directly. On this fixture, one neutral update after the zero-timer push is
enough: tick 57 accepts ordinary falling, and Shift on tick 58 enters
relaunch under the existing rules. No movement permissions changed.

```sh
gf_native --demo-gred-walk-ledge-expiry-relaunch --headless --frames 200 --no-audio
```

Entry adds 15 to vertical speed before displacement, slowing the fall
without immediately reversing it. It spends 150 once. The replay holds
Shift on ticks 58–77, spends 100 per continued update, and first runs
active program 15 at tick 63. Release at 78 returns to ordinary falling
with 7950 meter. Touchdown at 141 and landing at 142 don't refill it; idle
eventually does. A one-tick tap also works: it skips active relaunch,
returns to falling at 59 with 9850, and touches down at 110 before
landing at 111.

### Direct soft-Up relaunch after the accepted expiry push

`--gred-walk-ledge-expiry-direct-relaunch` opts into one new predecessor
for a plain descending relaunch: the immediately preceding accepted
**zero-timer walking push**. It does not borrow a completed wind-up or an
ordinary falling update. The source entry requires soft Up; native
acceptance independently checks the camera-relative turn from the old
body heading, then applies the existing body, bounds, and travel checks.
A still-positive timer, a pending 1 → 0 update, neutral input, floor snaps,
and steep responses at entry cannot use this permission.

```sh
gf_native --demo-gred-walk-ledge-expiry-direct-relaunch --headless --frames 170 --no-audio
gf_native --gred-walk-ledge-expiry-direct-relaunch --practice-start --no-audio
```

**Left Arrow** on 6–25 reaches the accepted expiry push at 56. Hold
**Up Arrow + Shift** on 57–76. The accepted expiry push shows
**UP+SHIFT/B TO REQUEST RELAUNCH** only while that input can be asked for;
the cue does not promise the next contact check will pass. Entry adds
15 to the projected downward speed, so G-Red still descends. It spends
150 and shows **DIRECT EXPIRY RELAUNCH** on that accepted frame.
Continued relaunch first moves up on 76. Release at 77 with 7950 meter;
touchdown at 136 and separate
landing at 137 preserve it until normal recovery. A single Up+Shift tap
on 57 also finishes: falling resumes at 58 with 9850, touchdown at 110,
and landing at 111. Neither route pays an air-dash charge.

The isolated scan completed 17 of 63 expired-push attempts. The new mode
qualifies just the soft-Up entry. None of the candidates from 4,800
positive-timer trials on the earlier footholds passed native entry.
The expiry-only mode keeps that boundary.

### Direct rising relaunch from a still-positive walking push

`--gred-walk-ledge-direct-timed-relaunch` opts into a different
predecessor and velocity range: an immediately preceding accepted
walking push with time still positive after the next decrement, whose
projected descent is slow enough for the plain additive relaunch to
lift. Entry takes **neutral direction**, preserves body heading, and
must pass the unchanged travel checks without a floor snap or push.
The mode does not admit a pending 1 → 0, directional entry, or a
descending direct entry on the positive timer.

```sh
gf_native --demo-gred-walk-ledge-direct-timed-relaunch --headless --frames 150 --no-audio
gf_native --gred-walk-ledge-direct-timed-relaunch --practice-start --no-audio
```

The declared grounded native start is X/Z `-4066.18164/-3400.83032`
(floor query `1846.01367`, arena half-width `4500`). **Up+Left Arrow**
on 6–7 walks off the rim. No input is needed through the three
descending pushes on 22–24, first on face 2021, then on face 2011.
Hold **Shift** on 25–32. Entry adds 15 to the projected speed,
lifts immediately, and spends 150, with timer 14 → 13. Active
relaunch starts at 30. Release at 33 with 9150 meter and five ticks
left. The timer reaches zero during the fall at 38; touchdown and
landing are separate at 117–118 before full-meter idle. A one-tick
tap instead lands at 101–102 with 9850 meter.

A wider original-stage scan found 61 accepted walking-push witnesses
in 912 approaches and completed ten direct relaunch routes in
172 follow-up attempts. The new permission uses only the neutral
rising branch. Older policies, other Borgs, and contact rules are unchanged.

### Soft Up on the same positive-timer entry

`--gred-walk-ledge-direct-timed-up-relaunch` is a separate opt-in for
soft camera-relative **Up**. It inherits neutral entry, but does not
allow diagonals or a full-stick direction on the entry step. The source
requires the mapped soft-Up input, and native verifies the exact turn
from the accepted body heading with the current movement camera.
Changing the presentation camera alone does not turn the Borg.

```sh
gf_native --demo-gred-walk-ledge-direct-timed-up-relaunch --headless --frames 150 --no-audio
gf_native --gred-walk-ledge-direct-timed-up-relaunch --practice-start --no-audio
```

Use the same grounded start and **Up+Left Arrow** on 6–7. Hold
**Up Arrow + Shift** on 25–32, then release both. On 25 G-Red gets
the same 150 charge and timer 14 → 13, but turns and accelerates
horizontally instead of applying neutral damping. The existing
directional cap is reached on 29. Release at 33 still leaves 9150
meter and five ticks, but this trajectory touches down at 114 and
lands at 115. A one-tick Up+Shift tap also completes. The neutral-only
mode still refuses this same Up entry; a pending 1 → 0 and entry
contact responses remain outside this permission.

### Plain soft-Up relaunch on the 1 → 0 walking-push update

`--gred-walk-ledge-expiring-direct-relaunch` allows a plain
descending entry on the exact decrement from one to zero, directly
after an accepted timer-one walking push. Unlike
`--gred-walk-ledge-expiry-direct-relaunch`, it does not need a
second, already-expired push. Unlike the floor-wind-up handoff,
it has no prior dash charge. Both source and native still require
soft Up, the additive lift, and a plain descending frame without
a snap or steep response. Native rechecks the actual clock, Up
turn, integration and terrain/body/bounds travel independently.

```sh
gf_native --gred-walk-ledge-expiring-direct-relaunch --practice-start --no-audio
gf_native --demo-gred-walk-ledge-expiring-direct-relaunch --headless --frames 125 --no-audio
```

From grounded X/Z `-1080.47473/-3357.9165` (query `1846.01367`,
arena half-width `4500`), hold **Left Arrow** on 6–7 and tap
**Up Arrow + Shift** on 38. At 37 the push on face 2009 (angle
8192) leaves one tick and can offer the next input without
guaranteeing acceptance. On 38, the timer expires, entry adds
15 to −27 before displacement, and meter drops from 10000 to
9850. G-Red stays airborne and descending. Release at 39,
touchdown at 91, and landing at 92 keep 9850 until idle recovery.

The bounded local scan found no plain entries in 1,215 trials.
The wider 848-route sweep staged four plain entries; three passed
native travel and completed. A separate scratch test on the older
expiry foothold exercised a projected relaunch entry rather than
a plain one. That response is not part of this mode; on that
foothold an immediate neutral release also fails the next native
travel check. The older positive and already-expired modes still
refuse the pending 1 → 0 request.

### Projected soft-Up relaunch on the same timer transition

`--gred-walk-ledge-expiring-relaunch-push` adds one separately
qualified projected entry. The immediately preceding frame must be
an accepted walking push on face 2009 (angle 8192) with one tick
left. On 1 → 0, soft **Up + Shift** can enter descending relaunch
and project against face 1999 (angle 13653). Other face pairs, floor
snaps, raised-side entry and a merely already-zero clock do not
gain this permission. The plain expiry-step mode still refuses
this projection. A nearby probe at Z `-3315.80225` stages against
face 1998 at the same angle; the opt-in still refuses that pair.

```sh
gf_native --gred-walk-ledge-expiring-relaunch-push --practice-start --no-audio
gf_native --demo-gred-walk-ledge-expiring-relaunch-push --headless --frames 145 --no-audio
```

Use the earlier expiry foothold, X/Z `-1250.96863/-3313.80225`
(query `1846.01367`, arena half-width `4500`). Hold **Left Arrow**
on 6–25, then **Up + Shift** on 56–59; stay neutral afterward.
Entry on 56 adds 15 to −27 before displacement, applies relaunch
acceleration then projects and damps by `.9`. It costs 150, stays
airborne, and shows **RELAUNCH ENTRY PUSH ON EXPIRY** only on
that accepted frame. Native reselects the contact plane and
checks the input turn, projected endpoint, and both travel checks
independently. No dash or landing callback is involved.

The three held wind-up updates each spend 100. Release at 60,
touchdown at 114 and landing at 115 leave 9550 meter; idle then
recovers normally. Alternatively, tap **Up + Shift** on 56 and keep
**Up** alone on 57. That ordinary falling drift reaches touchdown
and landing at 110–111 with 9850. An immediate neutral release on
57 still fails native travel; the accepted entry does not authorize
that next push.

### Adjacent expiry-entry face 1998

`--gred-walk-ledge-expiring-adjacent-relaunch-push` is a separate
opt-in for the **2009 → 1998** pair. Face 1998 has the same 13653
angle as 1999, but native must select the actual triangle and check
its projection and both travel paths again. The face-1999 mode
still refuses this approach. An unrelated angle, face, floor snap
or timer does not gain permission.

The guide starts two units farther along Z, at
`-1250.96863/-3315.80225`, with the same query and bounds. Use
**Left Arrow** on 6–25 and **Up + Shift** on 56–59, then neutral.
The timer-one walking push at 55 and projected relaunch at 56
spend the same amounts as before. The held route releases at 60,
touches down at 114, and lands at 115 with 9550 meter. A tap on
56 with **Up** alone on 57 also completes, with 9850 at
touchdown/landing on 110–111. Immediate neutral release still
fails native travel on 57. The exact-start probe completed 16
of 20 input variants; four failed after the accepted entry.
Waiting instead of pressing on 56 does not reach an already-zero
push on this start: the neutral walking fall hits the existing
raised-side refusal and the last accepted timer remains one.

## Ordered dash off an inclined ledge (GFMM9)

`--gred-inclined-dash-ledge` admits a scoped normal G-Red departure using the
existing `670DC` ground tangent and step result, ordered dash root, and
ground-loss/landing handlers. It is not a new slope-snap or steep-push
response. Source planning and native acceptance each check their own selected
faces and the authored strip. The segment must leave the incline through
an edge interior, cross descending steep faces joined by exact 3D edges,
then end above a single flat face that underlies the full segment. Gaps,
overlapping elevated faces, corner exits, and drops no larger than the step
limit refuse. These geometry limits are native safeguards, not original
game constants. Native body, bounds and terrain checks still run.

The demo uses a native grounded fixture on original triangle 827, crosses
the narrow cliff band (826, 825), and falls toward flat floor 2060. A single
right flick after five idle ticks departs at accepted tick 6, enters falling
at 7 (timer698 = 30), touches down at 21, then enters landing at 22. The
departure retains its Y; the fall does not spend meter. The previous
flat/incline transition mode still refuses this trajectory, and removing the
native edge proof or substituting an instant floor snap retains the prior
accepted motion, timer, meter, root, and pose. No source-frame equivalence
or steep ledge-fall pushes are claimed.

```sh
gf_native --demo-gred-inclined-dash-ledge --headless --frames 100 --no-audio
```

### Ordinary walk off the same inclined ledge

The ordinary walk handler already produced a separate ground-loss offer on
this geometry. This demo verifies it without changing runtime admission or
using the ordered-dash proof as a substitute for the walk path. A bounded
original-stage scan found eight full walking replays; the regression
independently checks the authored cliff strip for the replay below.

```sh
gf_native --demo-gred-inclined-walk-ledge --headless --frames 100 --no-audio
```

From the demo's grounded native placement at X/Z
`2599.00806/-2758.12109` (initial floor query `125.757187`), **D-pad Up**
starts after five idle ticks. Ordinary walk program 1 advances on the
incline before the departure at tick 8. Neutral input then replays the
existing fall entry at 9 (program 13, timer698 = 30), flat touchdown
at 23, and separate landing at 24 (program 8). The previous slope-transition
policy and the newer inclined-dash policy both pass this walk replay.
There is no dash root update, flight-meter spend, or steep push. This is one
verified route, not a claim about arbitrary slopes or original spawns.

## Dash interruption on an incline (GFMM9)

`--demo-gred-inclined-dash-jump` uses the existing flat-to-incline
fixture and dash-jump rules together. On the original stage, **S** on
6 starts the ordered dash, which crosses from flat triangle 249 to
incline 1052 at 11. Hold **Shift** on 14–37. Takeoff at 14 interrupts
dash recovery at the already accepted height, without another root
update or a meter charge. The old root lease retires only after that
takeoff is accepted, when **DASH CANCELED** appears.

```sh
gf_native --demo-gred-inclined-dash-jump --headless --frames 120 --no-audio
gf_native --gred-inclined-dash-jump --practice-start --no-audio
```

The first lift spends 150 at 26. Release into falling at 38 with
8750 meter, touch down on the incline at 84, and enter the separate
landing at 85. The alternate uphill fixture also completes an
interruption during active dash. The bounded scan completed 153 of
252 routes; incomplete trajectories did not grant any new movement
or contact permissions.

### Timed landing dash across a shallow incline

The slope-transition controller already combines the retained ledge timer
with landing interruption and independently checked shallow joins. From
the declared grounded native start at X/Z `-1719.38672/-508.356781`
(floor query `343.13092`, arena half-width `4500`), step with **Left Arrow**
on 6–9 and neutral until **A** on 34.

```sh
gf_native --demo-gred-timed-incline-landing-dash --headless --frames 70 --no-audio
gf_native --gred-timed-incline-landing-dash --practice-start --no-audio
```

| Accepted tick | Role | Ledge timer |
| --- | --- | ---: |
| 8–9 | Walking departure, then fall entry | — / 30 |
| 32 | Touchdown on original incline 1226 | 7 |
| 33 | Separate landing entry | 6 |
| 34 | Dash from landing crosses onto incline 1014 | 5 |
| 38–39 | Countdown expires during the ordered dash | 1 / 0 |
| 59–60 | Recovery resets, then idle | 0 |

**DASH FROM LANDING** appears only on the accepted dash entry. Its
25-unit ground step uses the actual selected heights and fresh root;
remaining speed is 24.5 and the flight meter stays full. A lone A flick a
tick earlier doesn't start or queue a dash: touchdown hasn't entered the
landing callback yet.
The older flat-only retained-timer mode refuses the same incline dash.

A bounded 3,030-route original-stage scan found 586 completed nonflat
routes with positive time at dash entry. This guide publishes one; it
doesn't relax terrain or add air or steep-push permissions. In particular,
the old flat landing fixture's forward dash still refuses, even with the
shallow-join mode.

## Descending air-dash wind-up entry push in the combined controller (GFMM9)

`--source-ground-windup-entry-push` adds steep projection during wind-up entry
at zero698 for a nonpositive incoming vertical speed. Entry halves scalar and
vertical speed, checks contact at the unintegrated position, and applies .9
steep damping. It does not move before contact, apply gravity, spend meter, or
decrement the newly set timer. A gentle floor snap remains a separate response.

The demo uses the existing native placement and floor reference on the original
stage, with a continuous jump hold. After the first falling push, a forward
flick produces the wind-up entry push at tick81 with 8450 meter. Continued
wind-up applies gravity on the following updates and charges the usual 450
handoff cost at86. Active entry at87 and touchdown at140 remain separate.
Active-entry, rising wind-up entry, ground-loss falling, and positive698
pushes remain unsupported.

```sh
build/direct/gf_native --demo-source-ground-windup-entry-push --headless --frames 600 --no-audio
```

[Contact Sheet 58](../local/recomp-evidence/contact-sheets/source-ground-windup-entry-push-v1/contact-sheet-58.png)
shows the entry push, handoff, and later touchdown in offscreen OpenGL.
The isolated host and all-header builds, three targeted host tests, and the
unchanged 64-tick duel smoke passed. The wind-up entry sanitizer test passed
2,695 checks, including rejection of early cost, timer decrement, pre-contact
displacement, and applying the future vertical acceleration.

### Why active-entry steep pushes remain disabled

The recovered active handler resets vertical speed to zero, spends the active
cost, then moves and checks contact. On entry the pre-contact displacement is
horizontal. With the same selected native plane, steep projection requires the
endpoint to be at or below the plane, but native airborne travel refuses a
plane higher than the start. Thus a projected endpoint above the final floor is
not enough: the path to the pre-projection endpoint must also pass.

A bounded original-stage probe found 39 steep candidates in 720 routes but no
candidate satisfying both checks on the same plane. The owner regression follows
one genuine wind-up to tick84 near the mountain. The would-be active step enters
triangle2536 about 22.8 units below the selected surface, even though its
projected endpoint is above the final floor. Tick85 remains refused without
publishing entry motion, cost, or animation. Neither the native terrain check nor
the active-entry push policy was widened. Existing gentle-floor active entry and
continued active steep pushes remain distinct.

### Ledge-fall steep contact still requires a verified travel path

Ledge-fall contact uses the accepted countdown: a positive timer selects the
6144 angle gate rather than the expired-timer gate of 10240. That can classify
a face as steep before the falling actor reaches the floor, but it does not
authorize the projection. In two bounded original-stage scans (2,000 routes
and 1,764 nearby variations), no steep proposal passed both native plane
matching and pre/post-contact travel checks.

The ground-loss owner regression follows a genuine ledge departure at tick138
and program13 falling. At the proposed tick141, the countdown would be 28
and the selected face has angle 9172. The proposed projection clears its final
floor, but native travel to the pre-contact point is blocked by the raised face.
The earlier policy refuses the frame and leaves the last accepted countdown,
pose state, position and meter unchanged; the source lease retires without a
fallback. The later continued walking-fall mode still rejects this raised
approach at native acceptance; no collision rules changed.

## Descending relaunch-entry push in the combined controller (GFMM9)

`--source-ground-relaunch-entry-push` adds a steep push on the relaunch entry
update at zero698, but only when the vertical speed is still nonpositive
after the +15 lift. The entry cost of 150 and separate program14 requests
occur once. The lift changes the displacement before acceleration and .9
steep damping; it does not grant landing, refill, or a second lift.

The original-stage demo pushes on entry at tick79, leaving 8400 meter.
The following update releases into falling21 with soft left drift and accepts
touchdown at80. Landing8 at81 clears the airborne cache separately. The
previous continued-only mode refuses this entry push. Rising entry,
dash entries, ground-loss falling and positive698 pushes remain unsupported.

```sh
build/direct/gf_native --demo-source-ground-relaunch-entry-push --headless --frames 240 --no-audio
```

[Contact Sheet 57](../local/recomp-evidence/contact-sheets/source-ground-relaunch-entry-push-v1/contact-sheet-57.png)
shows the entry push and separate touchdown in offscreen OpenGL. The isolated
host and all-header builds, three targeted host tests, and unchanged 64-tick
duel smoke passed. The entry sanitizer test passed 1,273 checks, including
rejection of incorrect lift, cost, lane, timer, and damping records.

## Continued relaunch push in the combined controller (GFMM9)

`--source-ground-relaunch-push` adds descending steep projection at zero698
during ongoing relaunch8, with independent program14–15 lane checks. It applies
.9 to the integrated scalar and vertical speed, retaining the relaunch
acceleration and normal continued cost. This is distinct from the .8 null-floor
snap and cannot replay the additive entry lift. Rising approaches, relaunch
entry, dash entries, ground-loss falling and positive698 pushes remain refused.

On the original-stage demo, relaunch enters without a push at tick82. The
continued program14 projection at83 costs the normal 100 meter, leaving8100.
Release on the following update takes the damped motion into falling21 and
touches down at84. Landing8 and the refill remain separate. Program15 is checked
by the numeric fixtures; the rendered route witnesses program14.

```sh
build/direct/gf_native --demo-source-ground-relaunch-push --headless --frames 240 --no-audio
```

[Contact Sheet 56](../local/recomp-evidence/contact-sheets/source-ground-relaunch-push-v1/contact-sheet-56.png)
shows entry, the continued push, and the separate touchdown in offscreen
OpenGL. The capture stays local. The isolated host and all-header builds,
three targeted host tests, and unchanged 64-tick duel smoke passed. The
relaunch sanitizer test passed 1,482 checks, including checks that reject
incorrect lane, resource, lift, and contact records.

## Continued active air-dash push in the combined controller (GFMM9)

`--source-ground-active-push` adds descending steep contact during continued
active16/program11 at zero698. The combined path checks the 50 meter spend,
timer, readiness and depletion state before validating the selected-plane
response. The push applies .9 to the integrated scalar and vertical speed and
clears planar acceleration for the following update. It does not replay active
entry, refill the meter or land the actor. Older policies remain unchanged.

The demo uses grounded native placements for both actors and wider bounds on the
unchanged stage. The ready active dash pushes at tick157 with meter1900→1850.
Neutral input takes the existing recovery branch at158, which pushes again with
its own gravity and no cost. Touchdown follows at159, landing8 at160, and idle
refills normally. The owner also verifies a separate relaunch input after the
accepted active push. Pre-readiness and depletion during contact are covered by
numeric fixtures, not by this rendered route.

Continued wind-up, recovery and falling pushes are retained. Active entry,
wind-up entry, active relaunch, ground-loss and positive698 pushes remain
unsupported.

```sh
build/direct/gf_native --demo-source-ground-active-push --headless --frames 360 --no-audio
```

[Contact Sheet 55](../local/recomp-evidence/contact-sheets/source-ground-active-push-v1/contact-sheet-55.png)
captures the active push, separate recovery push and landing in offscreen
OpenGL. The capture stays local and did not take desktop or classifier focus.

The isolated host and all-header builds, three targeted host tests, and the
unchanged 64-tick duel smoke passed. The active-contact sanitizer test passed
2,741 checks, including refusal of incorrect timer, cost, readiness and
contact records without advancing the world.

## Continued air-dash wind-up push in the combined controller (GFMM9)

`--source-ground-windup-push` adds descending steep contact during continued
wind-up15/program10 at zero698. Wind-up entry remains separate and cannot push.
The shared selected-plane response retains the damped vertical speed and −0.07
acceleration, unlike the null-callback floor snap that zeroes vertical speed.
The push itself does not start landing, stall the wind-up timer, or refill the
meter. Older combined and standalone policies are unchanged.

The original mountain fixture uses a grounded native placement and wider demo
bounds. Pushes at ticks86 and87 retain both program10 lanes. The second push
coincides with timer expiry and the existing once-only 450 meter charge. Tick88
normally enters active11 on a separate update. The controller also accepts the
verified relaunch input at that handoff only after the accepted push record
proves selector16, stage0, and timer zero.

The mode retains recovery and falling pushes. Wind-up entry, active-dash,
active-relaunch, ground-loss and positive698 pushes remain unsupported.

```sh
build/direct/gf_native --demo-source-ground-windup-push --headless --frames 360 --no-audio
```

[Contact Sheet 54](../local/recomp-evidence/contact-sheets/source-ground-windup-push-v1/contact-sheet-54.png)
shows both pushes and the separate active entry in offscreen OpenGL. Host and
all-header builds, three targeted host tests, and the unchanged 64-tick duel
smoke passed. The wind-up sanitizer test passed 2,147 checks across the legacy
and combined routes, including invalid timer, cost, and contact records.

## Air-dash recovery push in the combined controller (GFMM9)

`--source-ground-recovery-push` adds descending, airborne selector17 steep
contact at zero698. It inherits the existing falling push modes without
granting push authority to wind-up, active dash, active relaunch, ground-loss
falling, or positive698. The preceding combined mode still refuses contact
inside recovery. The older `--source-recovery-surface` mode remains unchanged.

Both paths share the program12 recovery, motion, and selected-plane response.
The combined path verifies the .99 approach, pre-contact motion, projected
position, and .9 damping against the accepted prior motion and contact records.
It preserves gravity, clears planar acceleration for the following update, and
does not spend or refill the meter on contact. Lower-lane completion is tested
after the push, while the upper update still uses program12. A real landing
requires a separate contact. This earlier mode still refuses simultaneous
recovery completion and touchdown.

The demo uses the older recovery fixture's grounded native placements and
unchanged stage. At accepted tick73 the push keeps program12, selector17,
cached87, and meter7650. Tick74 accepts a separate touchdown, tick75 switches
to landing8, and the 240-tick run ends in idle with a full meter. This fixture
does not demonstrate a push on the completion tick.

```sh
build/direct/gf_native --demo-source-ground-recovery-push --headless --frames 240 --no-audio
```

See the offscreen GL
[contact sheet](../local/recomp-evidence/contact-sheets/source-ground-recovery-push-v1/contact-sheet-53.png)
for the approach, recovery push, and separate touchdown. The capture stays
local and did not take desktop or classifier focus.

The isolated host and all-header builds, three targeted host tests, and the
64-tick duel smoke test passed. The sanitizer recovery test passed 1,240 checks
across the legacy and combined routes, including refusal of mismatched motion,
contact, completion and meter records without advancing the world.

### Steep push on the recovery completion update

`--demo-gred-recovery-completion-push` verifies the existing combined
recovery-push policy when lower program 12 finishes on the same update.
The first 2,520 nearby routes did not offer that coincidence. Releasing
jump before the dash and approaching the toe from a different grounded
start produced 21 accepted completion pushes in 2,240 variants, of
which two completed and settled. The other 19 refused a raised-side
contact on the next update. The regression keeps one such route
(`-585/-2990`) closed at tick 112 despite its accepted push at 111.

```sh
gf_native --demo-gred-recovery-completion-push --headless --frames 180 --no-audio
```

On the original stage, the native start at X/Z `-575/-2990` reaches
triangle 1321 at tick 111. The steep push applies the existing `.9` tail
(`-30 → -27` vertical speed) and keeps both program 12 lanes for that
frame. The lower completion selects falling for the next update.
`RECOVERY COMPLETE` appears only on that accepted push, not on an
unfinished push or on touchdown. Falling program 13 inherits the damped
motion and touches down at 112. Landing program 8 starts at 113 with
5850 meter; refill starts at 114. The second full route uses triangle
1320 and the same program timing.

Combined recovery push offers now carry the producer's actual lower-lane
completion proof. Native still checks the motion, selected plane, and
pre/post travel separately. Neither a forged completion flag nor a
plausible projection can advance the world without its matching proof.
Older contact policies still refuse recovery pushes where they did before.

### Diagonal exit on the following update

At X/Z `-580/-2990`, Left on tick 111 produces a valid completion push
on triangle 1320. Keeping Left on 112 hits the unsupported raised-side
contact. Holding **Up+Left** instead sends the existing four-unit drift
along a different camera-relative heading before retained momentum.
That route touches down on 112 and starts landing at 113, still at
5850 meter. The push itself is identical in both cases. The body does
not turn, and **EXIT DRIFT** is shown only on the accepted follow-up.

```sh
gf_native --demo-gred-recovery-completion-exit --headless --frames 180 --no-audio
```

The nearby `-585/-2990` route remains closed for the tested inputs. In
294 follow-up trials, only one previously stalled approach completed;
33 first action updates were accepted, but none led to a complete
action route. In particular, a plain wind-up can be accepted at 112
and still hit the raised-side refusal at 113. Neither the diagonal
route nor the partial wind-up adds a contact allowance.

## Released-relaunch zero698 falling push in the combined controller (GFMM9)

`--source-ground-relaunch-fall-push` adds downward selector6 steep contact after
relaunch release (or depletion), at zero698. It inherits ordinary and post-air
fall pushes. Active relaunch, air-dash callbacks, ground-loss falling and
positive698 still cannot push. Earlier combined policies are unchanged.

The same falling planner selects program21 plus the recorded coarse direction
for relaunch history. Native acceptance binds that lineage to the preceding
relaunch or falling receipt, checks the accepted meter (including for the
release/depletion predicate), and rejects an additive relaunch lift on this
path. It checks the pre-contact fall motion and selected-plane projection before
applying the .9 scalar and vertical damping. Only a later callback lands the
actor. The native pre- and post-contact checks are unchanged.

The demo uses a grounded original-stage placement at x/z−606.4512/−1373.5873,
with unchanged target and bounds. Jump is held on input ticks5–32 and relaunch
on45–59. Released falling starts at accepted tick61. The angle11582 push at123
keeps program21 and meter6800; vertical velocity changes from −30 to −27.
Touchdown follows at124, landing8 at125, and idle with a full meter at147.

The owner also checks a shorter route that releases on input tick82 and pushes
on the first falling update at83. Both routes run600 updates in the same world
and producer without teleporting or injecting motion. The rendered routes
exercise program21, not every direction, and do not demonstrate depletion into
a push. Holding relaunch through the immediate contact refuses, as does using
the preceding post-air-only policy on the continued route.

```sh
python3 -B tools/build_native.py --host --headers --jobs 2
build/direct/gf_native --demo-source-ground-relaunch-fall-push --headless --frames 600 --no-audio
```

The offscreen GL capture
`local/recomp-evidence/contact-sheets/source-ground-relaunch-fall-push-v1/contact-sheet-52.png`
shows relaunch, release, steep projection, callback, landing8 and idle at
accepted ticks46/51/61/122/123/124/125/147. It remains local; the capture did
not take desktop or classifier focus. All eight frames matched the final build
byte-for-byte.

The isolated host and all-header builds passed. The sanitizer owner passed
6,241 checks, including rejection of a fabricated depletion on a release frame
that has not yet touched the slope. Both new CLI tests and the preceding
post-air push test passed. The ordinary 64-tick duel still produced 11 shots,
4 hits, and health92/76.

### One inter-gate push while the released relaunch still has time left

`--gred-walk-ledge-timed-release-push` opts into one positive-timer
response. It needs a plain neutral relaunch tap from an accepted
ordinary **walking** ledge fall, then an accepted ordinary falling
update after release. Source and native independently carry that
history through neutral falling until the first contact. The push
uses the actual timer and the inter-gate angle range (above 6144
through 10240). It adds no active relaunch, repeated positive push,
entry-contact or dash-origin permission.

```sh
gf_native --demo-gred-walk-ledge-timed-release-push --headless --frames 125 --no-audio
gf_native --gred-walk-ledge-timed-release-push --practice-start --no-audio
```

From the grounded native start X/Z `1000.65021/-3365.34546`
(floor query `1549.25293`, arena half-width `4500`), hold
**Right Arrow** on 6–8 and tap **Shift** on 23. The plain entry
costs 150 with 16 ticks left. Release into ordinary falling at 24.
On 33, neutral falling pushes on triangle 1961, angle 8191, with
six ticks left. It keeps the existing `.9` damping and costs
nothing. **AIRBORNE / TIMED RELAUNCH-FALL PUSH** appears only on
that accepted push. Touchdown and landing are separate at 90–91
with 9850 meter; normal recovery takes over afterward.

Both pre- and post-response travel checks are unchanged. The
older zero-timer policy still refuses this same contact. In the
bounded 4,872-route scan, 22 timed pushes were accepted and
three routes completed. A nearby route at Z `-3371.34546`
accepts face 2008 at 35, then still refuses native travel on 36.
The accepted push does not grant that following step.

### Two consecutive positive-timer pushes on one face

`--gred-walk-ledge-timed-release-two-pushes` adds one successor to
the contact above without changing the single-push policy. The very
next update must still be a descending, neutral selector-6 push
with a positive clock. It must select the same triangle and normal
and independently pass the same angle, projection, pre-response and
post-response travel, body and bounds checks. A different face, a
nonconsecutive contact or a third positive-timer push does not qualify.
This does not add an expiry-step or active-relaunch permission.

From grounded X/Z `-1298.54333/-3395.96802` (query `1846.01367`,
arena half-width `4500`), hold **Down+Right Arrows** on 6–7, tap
**Shift** on 20, and stay neutral. The plain entry costs 150;
ordinary falling starts at 21. Pushes on triangle 1993 (angle
8192) occur at 33 and 34, with five and four ticks left. The
second cue appears only at 34. Neither push grounds G-Red or
changes meter. Touchdown at 101 and the separate landing at 102
keep 9850 meter; normal idle returns to 10000. The old one-push
policy accepts the prefix through 33 but refuses 34.
The scoped 274-route replay found 22 accepted pairs and eight
completed routes. Nearby replays also verified that a real
different-face successor and a third positive contact still refuse.

## Post-air zero698 falling push in the combined controller (GFMM9)

`--source-ground-air-fall-push` / `--demo-source-ground-air-fall-push` separately
admits downward selector6/program13 steep response after completed air-dash
recovery, at zero698. Earlier ordinary-only mode retains its post-air refusal.
This reuses the shared controller, source VM, resource/motion/contact planners
and renderer; it does not enter the broader legacy push capability hierarchy.
Existing enum ordinals are preserved with the new mode/policy appended.

Read-only initialized-DOL disassembly of617E4 confirms motion/contact precede
the lower-done test, which then sets history590=1 and selects6. In5FECC, that
history selects program13; entry resets4C/50, retains cached87 and installs
the ordinary landing callback. The common .99 scalar/direct-drift motion and
677B0 contact tail follows. Upper5FE04 independently requests program13.
The animation differs from ordinary falling6, not its shared contact arithmetic.

Native admission now permits an immediate falling push only after the actual
accepted recovery17 lower-done/next6 proof. The existing jump-motion validation
independently binds air origin, entry, old scalar/vertical velocity, drift,
heading, gravity/clamp and projected endpoint. Post-air scalar is not clamped
to ordinary ascent8. Exact selected-plane/triangle evidence, .9 retained
scalar/vertical tail, all-role698 chain and existing native pre/post contact
checks remain required. VM, cache, resource, counters and pose publish only on
the matching receipt. Neither recovery12 nor active11 may borrow this authority.

The demo starts at a declared native grounded original-stage placement
x/z−585/−3010 with4500 bounds and unchanged target/stage. It holds jump5–54
and forward flick45–71, then adds low left only after completed recovery selects
falling6. There is no midair teleport/rebind, velocity/resource injection or
edited terrain.

| Accepted tick | Actual role / resource |
| --- | --- |
| 46 | Air-dash wind-up entry;7150 |
| 52 | Active entry after deferred450 handoff;6650 after active50 |
| 83 | Recovery12 entry after minimum active timer;5150 |
| 111 | Lower recovery completion selects6; captured12/cache1 |
| 112 | Immediate falling13 push on angle11582; scalar≈21.081482→18.973333, vertical−30→−27, y≈21.060921;5150/cache1 |
| 113 | Separate callback at y≈20.665068; captured13/cache1/5150 |
| 114 | Separate landing8 clears cache |
| 136 | Idle0,10000 |

The owner also runs a second grounded placement−580/−3010 through the same
genuine air/recovery/entry-push/landing sequence, plus the inherited ordinary
falling6 route. Each runs600 accepted updates on one producer/VM. It checks
missing/swapped numeric and plane proofs, caller/entry/timer tampering, exact
receipts and explicit retirement. A continued post-air fall from−570/−3030
(flick46–72, jump5–55, low left after recovery) stages a push114 whose
projection is below an adjacent native floor. Native rejection preserves tick113,
the previous live pose/receipt and uncommitted counters; no clipping or UP-floor
substitute is introduced.

A separate grounded5/10 fixture accepts a first post-air push113 and rejects a
penetrating second push114. The second candidate uses the accepted .9 retained
scalar followed by .99 falling damping, and accepted−27 vertical velocity before
gravity−28 and the next .9 tail. Its rejection keeps exactly one accepted push,
the prior meter and the previous pose/receipt live until explicit retirement.

Ground-loss, released-relaunch, wind-up/active/recovery callback and positive698
pushes, nonflat ordered dash, unknown/raised-side contacts and combat remain
refused. Host trigonometry, clocks, camera/target-center and body/terrain queries
remain labelled native proxies, not original-hardware or whole-game equivalence.
This does not generalize G-Red's recovered mappings to other Borgs.

```sh
python3 -B tools/build_native.py --host --headers --jobs 2 \
  --output build/source-ground-air-fall-push-host
build/source-ground-air-fall-push-host/gf_native \
  --demo-source-ground-air-fall-push --headless --frames 600 --no-audio
```

Contact Sheet51 contains actual isolated offscreen GL frames46/52/83/111/112/
113/114/136. The inspected sheet shows wind-up/active/recovery, completed
recovery, airborne falling13 push, separate callback/landing8 and idle without
taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-air-fall-push-v1/contact-sheet-51.png`;
captures and original assets are excluded from publication.

The isolated host/all-header builds and six sanitizer binaries passed. The new
owner passed11419 checks across two post-air and one inherited ordinary600-update
routes, including old-policy/caller/entry/plane/timer and penetration refusals.
Earlier ordinary-push, combined-air, legacy falling-surface, selected-plane push
and native acceptance regressions also passed. The unchanged64-tick ordinary
duel smoke still reports11 shots,4 hits and health92/76. The qualified G-Red
motion bank and immutable GFMM1–9 bytes are unchanged.
The full isolated Python run completed167 tests:160 passed, including all49
world-host integrations; seven separate source-replay tests were skipped because
their opt-in host was not requested. Assets, captures, private evidence, builds
and unrelated Story/preparation work are excluded from the checkpoint.

## Ordinary zero698 falling push in the combined controller (GFMM9)

`--source-ground-fall-push` / `--demo-source-ground-fall-push` adds one opt-in
permission to the existing combined controller: ordinary downward falling6 at
zero698, without ground-loss, post-air or released-relaunch lineage. Existing
enum ordinals and earlier policies are unchanged. This does not enter the legacy
push capability hierarchy or grant its broader callers.

The same `gred_surface_contact` planner selects the pre-response plane and
projects the integrated position. Native acceptance independently verifies the
ordinary fall predecessor, .99 scalar tail, displacement before gravity, −1
acceleration/−30 clamp, exact plane/triangle/angle and projection, then .9 damping
of retained scalar and vertical speed. Gravity and cached87 persist. No landing,
meter refill, VM reset or root handoff is inferred from the push. The existing
native pre/post body, bounds and terrain/penetration checks still apply.

The demo uses a declared native grounded original-stage placement
x/z−606.6735/−1373.0029 with ordinary arena bounds/target. Jump is held at5–32,
then input is neutral. There is no midair position/velocity/resource injection,
teleport/rebind, edited terrain or substitute renderer.

| Accepted tick | Actual role |
| --- | --- |
| 82 | Selected steep angle11582 projects y≈10.846179; post-gravity−30→−27; still falling6/program6/cache1, meter8350 |
| 83 | Separate actual callback lands at native y10; program6/cache1/meter8350 retained |
| 84 | Separate landing8 clears airborne cache |
| 106 | Idle0, meter10000 |

The owner also exercises a genuine longer-rise release38 route (push90,
callback91, landing8 at92, idle114; pre-landing meter7850) and a nonzero-scalar
route from grounded x/z−626.6735/−1393.0029 with one low D-pad-up update32:
push82 retains scalar≈0.3457004→0.3111304 and vertical−30→−27, then lands
separately. Each route runs600 accepted updates through one producer/VM.

Release31 produces a genuine push candidate79 that projects below a different
adjacent native floor. It remains rejected at world tick78 with the previous
pose/receipt/resource/counters live until explicit retirement. Replacing the
floor with UP, clipping, or forcing acceptance would violate the native boundary.
Ground-loss/post-air/relaunch/dash/positive698 pushes, nonflat ordered dash,
unsupported contacts and combat remain refused. Earlier combined relaunch mode
still refuses the new steep response before publication. Nothing here grants
original-hardware/whole-game equivalence or applies to other Borgs.

```sh
python3 -B tools/build_native.py --host --headers --jobs 2 \
  --output build/source-ground-fall-push-host
build/source-ground-fall-push-host/gf_native \
  --demo-source-ground-fall-push --headless --frames 600 --no-audio
```

Contact Sheet50 contains actual isolated offscreen GL frames18/33/60/81/82/
83/84/106. It shows ascent, falling, the airborne push pose, separate callback,
landing8 and idle, without taking desktop/classifier focus. The inspected sheet
remains local at
`local/recomp-evidence/contact-sheets/source-ground-fall-push-v1/contact-sheet-50.png`;
captures and game assets are not published.

The isolated host/all-header builds and six sanitizer binaries passed: the new
owner (10918 checks), prior ground-loss relaunch and paired-surface owners,
legacy selected-plane push, shared surface contact and native motion acceptance.
The full isolated Python run completed164 tests:157 passed, including all46
world-host integrations; seven separate source-replay tests were skipped because
their opt-in host was not requested.
The unchanged64-tick ordinary duel smoke still reports11 shots,4 hits and
health92/76. Original G-Red motion-bank and immutable GFMM1–9 bytes are unchanged;
assets, private evidence, builds and unrelated Story/preparation work are excluded.

## Owned-timer ground-loss relaunch (GFMM9)

`--source-ground-loss-relaunch` / `--demo-source-ground-loss-relaunch`
separately admits verified relaunch after accepted ground-loss history,
including positive698. Earlier modes retain their ground-loss/positive-timer
relaunch refusal. It reuses the same VM, heading, motion/resource/contact/
renderer systems and complete per-role timer witness.

Read-only6235C disassembly confirms entry changes marker590 to the distinct
relaunch origin, adds data4C15 to old48 before displacement and keeps698.
Native acceptance now recognizes only the separately enabled accepted ground-
loss entry/air predecessors; it still requires exact additive motion, resource,
independent lower/upper14/15 latches, current timer and endpoint evidence.
Released relaunch uses21–24 provenance, not ground-loss13 or ordinary jump6.
Discarding ground-loss origin does not discard its timer.

This also qualifies a previously refused numeric variant: the shared source
air wrapper clamps planar speed to8 **only when4C is positive**. Neutral
relaunch sets4C0 and applies0.98 scalar damping but does not blanket-clamp
inherited speed8. The new policy checks that exact conditional behavior,
including a genuine air-to-neutral-relaunch/released-fall scalar above25,
without relaxing ordinary jump limits or widening older policy refusals.

`gred_relaunch_static_contact` gains an explicit698 argument; default0 keeps
legacy callers unchanged. The new owner supplies actual timer, and relaunch
floor evidence binds it to the same accepted countdown and6144/10240 gate.
Null754 snaps retain50, scalar, latches and cached87; only negative
post-integration48 receives0.8. Positive crossing gets factor1, then ordinary
active motion may lift off. Steep-push candidates still refuse in the owner.
Resource, VM, timer, both latches and pose publish only after exact receipt.

The default fixture uses declared grounded original-stage placement
x/z−1368.47/−466.667, query reference94.339, native bounds4500 and unchanged
target/stage. Down-right is held from5 until accepted falling entry; a genuine
relaunch press146–595 stays held through actual depletion. No resource or
airborne state is injected, and no midair teleport/rebind is used.

| Accepted tick | Role / actual698 / resource |
| --- | --- |
| 147 | Entry14 adds15 to old−25, moves−10 before gravity;698=5,9850 |
| 151 | First null-floor snap, post48−6→−4.8;698=1,9450; latches2 but captured14 |
| 152 | Timer naturally0; separate15 requests/latches3; second snap,9350 |
| 153–156 | Four more snaps, last newly positive48≈0.7232 uses factor1;8950 |
| 157 | Ordinary active lift above floor,8850 |
| 247 | Previous accepted meter0 selects released21 and clears relaunch latches |
| 333 | Real callback touchdown, captured21/cache1, still meter0 |
| 334 | Separate landing8/cache0, still meter0 |
| 356–600 | Shared idle0 and common grounded recovery10000 |

`native_source_ground_loss_relaunch_owner` checks six600-update genuine routes:
early neutral release/callback199, early held depletion/callback327, late
positive-timer release/callback148, held floor/lift/depletion/callback333,
air-to-neutral high-scalar release/callback187 and directional clamp/callback180.
It verifies conditional clamp/lineage, exact shared150/100 costs, actual timer
and paired plane gates, negative-only tail, independent latch receipt data,
both callback/landing boundaries, stable idle and old-policy refusal.
Numeric inter-gate tests distinguish positive698 from declared-zero contact.

### Short relaunch floor-snap guide

A narrower input script on the same grounded `-1368.47/-466.667`
fixture shows the floor response without waiting for depletion.
The continued positive-timer **push** stays closed: two bounded
1,800-route scans of the walking-push and floor-snap approaches
offered no qualifying continued projection.

```sh
gf_native --demo-gred-walk-ledge-timed-relaunch-floor --headless --frames 200 --no-audio
gf_native --gred-walk-ledge-timed-relaunch-floor --practice-start --no-audio
```

Use **Down+Right Arrow** on ticks 6–122, then neutral, then hold
**Shift** on 147–157. Tick 151 snaps while timer698 is 1 and damps
descending speed. At 152 the timer expires and the snap is still
airborne. The last snap at 156 retains newly positive vertical
speed; tick 157 lifts above the floor. The cue distinguishes the
two accepted snap responses. Release on 158, touch down at 162,
and start landing at 163 with 8850 meter; idle refills normally.
Releasing immediately after the first snap also works: 152 is
then real touchdown, with landing at 153. Neither snap itself
counts as landing.

### Timed relaunch snap to air dash

On the same grounded start, use **Down+Right Arrow** on 6–122.
Hold **Shift** on 148–150 and add a one-tick **D+S** flick on 150,
**without releasing Shift** on that step. The relaunch floor snap at
149 is still airborne. The wind-up entry at 150 uses its velocity and
meter, halves the pre-response velocities, and zeros vertical speed
at the selected floor without translating or spending meter.

```sh
gf_native --demo-gred-walk-ledge-timed-snap-dash --headless --frames 220 --no-audio
gf_native --gred-walk-ledge-timed-snap-dash --practice-start --no-audio
```

| Accepted tick | Role | Ledge timer | Meter |
| --- | --- | ---: | ---: |
| 149 | Descending relaunch snap on face 1116 | 3 | 9750 |
| 150 | Stationary wind-up entry floor snap | 2 | 9750 |
| 151–152 | Wind-up continues through timer expiry | 1 → 0 | 9750 |
| 155 | Wind-up handoff charge | 0 | 9300 |
| 156 | Separate active entry charge and floor snap | 0 | 9250 |
| 186–187 | Callback touchdown, then landing entry | 0 | 7750 |

The **AIRBORNE / RELAUNCH TO WIND-UP** cue marks only the accepted
handoff. Releasing Shift on the flick step instead gives touchdown,
and the buffered landing dash still refuses on the following step.
Release **without** the flick to finish a normal landing. Owner
regressions also cover a wind-up on the 1 → 0 boundary and a
positive-speed entry after expiry, with the same separate landing.
No contact permissions changed. Of 2,480 scanned routes, 48
completed and 19 still had a positive timer at wind-up entry.
Incomplete routes do not authorize raised-side travel or steep contact.

### Neutral relaunch from the completed timed floor wind-up

`--gred-walk-ledge-timed-snap-handoff` opts into one later choice:
relaunch instead of active entry after the verified floor wind-up.
It must start from an accepted descending relaunch snap with enough
ledge time to enter wind-up **still positive**. Every wind-up update
must remain an accepted snap on the same flat level; a triangle
change also needs its own shared-edge proof. That sequence cannot be
inferred from a zero timer or from the last floor height alone.

```sh
gf_native --demo-gred-walk-ledge-timed-snap-handoff --headless --frames 220 --no-audio
gf_native --gred-walk-ledge-timed-snap-handoff --practice-start --no-audio
```

Follow the timed snap-to-dash inputs above through 150, then neutral
through 155. Wind-up crosses the verified coplanar edge 1116 → 900 at
151 and the ledge timer expires at 152. Tick 155 pays 450, leaving
9300 meter, and shows **SHIFT/B RELAUNCH / NEUTRAL DASH**. On 156:

- Tap **Shift** with no direction held to rise 15 units above the
  same triangle. This costs 150, leaving 9150, with no active-dash
  charge or entry snap. Release on 157; touchdown and landing are
  separate at 190 and 191.
- Leave the controls neutral to start active dash at 9250, with its
  unchanged touchdown and landing at 186 and 187.

The source and native owner each verify the accepted sequence and
same-triangle entry. Older policies still refuse the floor handoff.
A wind-up that enters at zero, a relaunch request before expiry,
directional entry, and entry contact responses remain unsupported
under this new permission. This does not change landing or refill.

### The same floor handoff while the countdown is still positive

`--gred-walk-ledge-positive-snap-handoff` is a separate opt-in.
The same certified wind-up history and neutral, same-triangle
relaunch checks still apply. After the paid wind-up, the previous
timer must exceed 1 and the next accepted value must be exactly
one lower and still positive. A pending 1 → 0 is neither this route
nor the previously accepted expiry case.

```sh
gf_native --demo-gred-walk-ledge-positive-snap-handoff --headless --frames 110 --no-audio
gf_native --gred-walk-ledge-positive-snap-handoff --practice-start --no-audio
```

The new grounded native start is X/Z `2892.57739/-2198.41479`
(floor query `151.148178`, arena half-width `4500`). Step with
**Up Arrow** on 6–8, **Shift** on 25–26 with **W** on 26, then
release. The first relaunch snaps at 25 (timer 13), then the
wind-up starts at 26 with timer 12 and 9850 meter. All six
wind-up snaps stay airborne on original flat triangle 1543.

Tick 31 pays 450 (9400 meter, timer 7) and offers the choice.
Tap **Shift** without direction on 32 to rise (9250 meter, timer 6).
The countdown continues through release and expires at 38.
Touchdown and landing occur separately at 66–67 before normal
recovery. Neutral on 32 instead pays 50 for active entry
(9350 meter, timer 6) and lands separately at 62–63.

A bounded original-stage scan found 140 early flat snap candidates
in 6066 approaches, then verified 16 full positive handoffs.
The expiry-only mode still refuses this same relaunch. No floor,
steep-contact, body or landing checks were relaxed.

### Completing the floor handoff on the expiry step

`--gred-walk-ledge-expiring-snap-handoff` opts into the actual
**1 → 0** update, separate from the still-positive and already
expired cases. The wind-up must already have paid its charge
on a certified uninterrupted flat-floor sequence. Older modes
still refuse this exact request.

```sh
gf_native --demo-gred-walk-ledge-expiring-snap-handoff --headless --frames 135 --no-audio
gf_native --gred-walk-ledge-expiring-snap-handoff --practice-start --no-audio
```

The grounded native start is X/Z `3895.56787/-1816.47241`
(floor query `262.01004`, arena half-width `4500`). Step with
**Up Arrow** on 6–25 and **Shift** on 46–47; add **A** on 47.
The first relaunch snap has nine ticks left. The wind-up enters
free with eight, stays on flat triangle 1542, then spends 450
at 54 with one tick left and 9400 meter.

On 55, tap **Shift** without a direction to rise 15 units while
the timer reaches zero. Meter becomes 9250; release on 56 and
touch down at 89, then land at 90. Neutral instead starts the
ordinary active dash at 9350 and touches down/lands at 85–86.
The timer changes on that accepted action; it does not retroactively
rewrite the preceding snap or count as grounded contact.

Eight complete 1 → 0 relaunch routes were found in the bounded scan.
No entry snap, steep contact, directional relaunch or earlier-policy
permission was added by this change.

### Relaunch after a paid active floor snap

`--gred-walk-ledge-active-snap-relaunch` uses the same grounded
start as the short positive floor handoff, but the existing
ground-loss relaunch mode. In this mode, pressing Shift before
active entry still refuses. Step with **Up Arrow** on 6–8 and
**Shift** on 25–26, adding **W** on 26. Stay neutral through 32.

```sh
gf_native --demo-gred-walk-ledge-active-snap-relaunch --headless --frames 110 --no-audio
gf_native --gred-walk-ledge-active-snap-relaunch --practice-start --no-audio
```

Tick 31 pays 450 for wind-up (9400 meter). Tick 32 pays an additional
50 for the active floor snap (9350 meter and six ledge ticks left).
The active dash's **Release in 30 ticks** cue does not delay relaunch.
Tap **Shift** with no direction on 33 to relaunch at 9200 with five
ticks left. This differs from relaunching before active entry: the
50 is already spent. **AIRBORNE / ACTIVE DASH TO RELAUNCH** marks
only the accepted entry. Release at 34; touchdown and landing remain
separate at 67–68.

Neutral input damps the inherited 29.95 planar speed to about 29.35
without clamping it to 8. Adding **Up Arrow** to Shift on 33
instead caps the retained speed at 8 **after** movement and
acceleration. It keeps the same charges and touches down/lands
at 66–67. Both trajectories finish normally. The earlier mode
completed all 24 bounded post-active trials; no motion or contact
permission changed.

Native placement/input/clocks/libm/camera/target/body/terrain approximations
remain explicit, not original-hardware or whole-game equivalence. Push,
nonflat ordered dash, raised-side/clipped/unknown contacts and combat remain
refused. Other Borgs do not inherit these recovered timer/flight semantics.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-loss-relaunch-host
build/source-ground-loss-relaunch-host/gf_native \
  --demo-source-ground-loss-relaunch --headless --frames 600 --no-audio
```

The isolated host/all-header builds, targeted new host integration,600-tick held
floor/depletion demo and unchanged64-tick ordinary duel smoke passed.
Contact Sheet49 contains actual isolated offscreen GL frames147/151/152/156/
157/247/333/334 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-loss-relaunch-v1/contact-sheet-49.png`;
captures, game assets and private evidence are not published.
All six isolated sanitizer binaries passed, including prior air/relaunch/floor
owners and shared relaunch/world regressions. The new owner passed24675 checks
across six600-update genuine routes plus old-policy/gate/lineage refusals.
All161 isolated Python tests passed, including50 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged; assets, captures, private evidence,
builds and unrelated Story/preparation work are excluded.

## Owned-timer ground-loss air dash (GFMM9)

`--source-ground-loss-air` / `--demo-source-ground-loss-air` separately admits
air dash after accepted ground-loss history while698 remains positive. It
retains the shared controller's established ground/jump/landing/dash/floor
permissions. Previous modes still refuse this positive-timer handoff.

61338 does not clear698. The same all-role native-accepted countdown therefore
continues through wind-up/active/recovery instead of disappearing with the
ground-loss origin bit. Wind-up entry halves44/48 without moving or spending;
later handoff450 still follows movement and precedes contact, and active50
precedes motion. VM/resource/timer/contact/pose publish only after exact receipt.

The shared `gred_air_dash_static_contact` now has an explicit timer argument.
Its default0 preserves all legacy callers; the new live owner supplies actual
698. Wind-up/active floor evidence carries that same value and native
acceptance checks6144 while positive or10240 after expiry, with the existing
pre/final body/terrain checks and caller-specific zero48/no-tail responses.
Callback landing also uses actual timer/plane evidence. Tests explicitly
distinguish a45-degree numeric plane accepted at0 from refusal at positive698;
no default-zero substitution, guessed UP floor or steep fallback is allowed.

Read-only617E4 disassembly at`80061890–800618C8` also suppresses direct676D8
drift while698 is positive, without skipping0.99 scalar damping or contact.
The new source/native guards preserve that rule. The complete live routes below
reach recovery after expiry; no positive-timer low-meter recovery fixture is
claimed from an injected airborne/resource state.

The default demo retains the previous declared grounded original-stage
placement x/z−1368.47/−628.674, query reference94.339 and native arena-half4500,
with unchanged target/stage. It walks down-right from5 until accepted ground-
loss entry and supplies a genuine right flick122–171. There is no midair
teleport/rebind or original actor-spawn equivalence claim.

| Accepted tick | Role / actual698 / resource |
| --- | --- |
| 120–121 | Ground departure then accepted falling13/698=30/cache1 |
| 122 | Still ground-loss13,698=29,10000 |
| 123 | Wind-up entry10: halve old−2 to−1, no translation/cost;698=28,10000 |
| 129 | Final wind-up movement then450 spend before contact;698=22,9550 |
| 130 | Active entry11, source30 motion, no callback yet;698=21,9500 |
| 150–151 | Natural timer decay1→0 during active, not on selector change;8500→8450 |
| 173 | Released recovery12, actual timer0;7400 |
| 185 | Actual paired-surface callback touchdown, captured12/cache1;7400 |
| 186 | Separate accepted landing8/cache0, still7400 |
| 208–600 | Shared idle0 and common grounded recovery10000 |

`native_source_ground_loss_air_owner` tests five600-update genuine routes:
right/left handoffs with callback185/183; shorter release with callback176;
a later handoff with callback186; and a roof-origin exact-expiry handoff37,
normal wind-up/active snaps and callback73. The earlier mode's prior-frame
expiry refusal remains unchanged. These are bounded native placement/input/
clock fixtures, not whole-frame or original-hardware equivalence.

A later ground-loss flick also produces genuine wind-up snaps on ticks150/151
with timers2/1. The native gate rejects forged zero timers or coherent45-degree
metadata. That route later reaches an unsupported raised-side response on158
and still refuses and retires authority; geometry is not edited or flattened to
make it pass. Steep push, raised-side/clipped/unknown contacts, positive698
relaunch, ground-loss relaunch, nonflat ordered dash and combat remain refused.
No other Borg inherits G-Red's movement-family or timer/contact semantics.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-loss-air-host
build/source-ground-loss-air-host/gf_native \
  --demo-source-ground-loss-air --headless --frames 600 --no-audio
```

The isolated host/all-header builds, corrected targeted host integration,
600-tick demo and unchanged64-tick ordinary duel smoke passed. The targeted
test explicitly checks actual expiry on151 (timer1 on150), rather than an
off-by-one expectation.
Contact Sheet48 contains actual isolated offscreen GL frames123/130/150/151/
173/185/186/208 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-loss-air-v1/contact-sheet-48.png`;
captures, game assets and private evidence are not published.
All six isolated sanitizer binaries passed, including prior landing-input
ownership and air-floor/air-dash/surface/world regressions. The new owner passed
21371 checks across five600-update genuine routes plus old-policy, expiry,
positive-floor gate and raised-side refusal boundaries.
All160 isolated Python tests passed, including49 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged; assets, captures, private evidence,
builds and unrelated Story/preparation work are excluded.

## Retained-timer ground-loss landing jump/dash (GFMM9)

`--source-ground-loss-landing` / `--demo-source-ground-loss-landing` separately
adds target12 owned-root dash cancellation after an accepted ground-loss
landing8 entry while698 remains positive. It retains target4 jump support and
the complete per-role accepted timer chain. The previous jump-only mode still
refuses the same genuine positive698 dash input.

This is admission through existing shared systems, not a new dash engine.
The original selector/heading helper and60C94 path do not clear698. Native
acceptance requires accepted landing provenance, exact target12/phase12
matching, current timer evidence, matching frame-start/selected surface and a
privately constructed current-world/current-producer post-VM root scope.
Landing permission or a copied previous root cannot mint that authority.
Entry moves25 then retains24.5; active, held/recovery reads, zero-control1D9B
and motion-before-done/reset remain with the unchanged ordered controller.
VM/root/timer/counters/pose publish only after the complete matching receipt.

The default native fixture is declared grounded original-stage placement
x/z−1368.47/−466.667, query reference94.339 and arena-half4500; target/stage data
are unchanged. It holds down-right from5 until accepted falling entry, then
supplies one genuine backward flick149 after accepted landing8. This is not an
original actor spawn or a midair teleport/rebind.

| Accepted tick | Role / timer / root |
| --- | --- |
| 121–122 | Walking departure then following-frame ground-loss13/698=30 |
| 148 | Actual flat callback13/cache1,698=4 |
| 149 | Separate accepted landing8/cache0,698=3 |
| 150 | Target12 entry27: move25/retain24.5,698=2, first actual root observation |
| 151–152 | Active13:698 naturally decays1→0; source VM/root ordering unchanged |
| 157 | Recovery14 on captured27; actual post-VM move19.35/retain18.85, root updates8 |
| 176–600 | Shared idle0 after done/reset;25 actual root evaluations retained, meter10000 |

`native_source_ground_loss_landing_owner` tests three600-update genuine routes:
single-flick dash recovery157/idle176, a held variant recovery159/idle178,
and inherited landing jump150 with a separate second callback203.
It covers landing provenance, missing/stale root scopes, fabricated entry
scalar/timer, root/VM holds, zero-control retained-root reads, receipts and pose
retirement. The earlier jump-only mode refuses before root publication.

Geometry is deliberately unchanged. The same landing with a forward flick
reaches a nonflat ordered-dash destination and still refuses; longer/other
directions can leave the qualified flat subset as well. No plane is flattened
or collision guessed to make a route pass. Positive698 air dash/relaunch,
ground-loss relaunch, steep push, nonflat ordered-dash terrain, clipping, combat
and unsupported variants remain refused. Native setup/input/clocks/libm/camera/
target/contact approximations remain labelled, with no original-hardware,
whole-game or other-Borg movement-family equivalence claim.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-loss-landing-host
build/source-ground-loss-landing-host/gf_native \
  --demo-source-ground-loss-landing --headless --frames 600 --no-audio
```

The isolated host/all-header builds, targeted new host integration,600-tick demo
and unchanged64-tick ordinary duel smoke passed. The demo reports one ordered
entry,6 active/19 recovery updates, one reset and25 actual root evaluations.
All six isolated sanitizer binaries passed, including prior landing-jump/
landing owners and ordered-dash/root/world regressions. The new owner passed
12257 checks across its three600-update genuine routes and refusal boundaries.
Contact Sheet47 contains actual isolated offscreen GL frames148/149/150/151/
157/175/176/600 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-loss-landing-v1/contact-sheet-47.png`;
captures, game assets and private evidence are not published.
All159 isolated Python tests passed, including48 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged; assets, captures, private evidence,
builds and unrelated Story/preparation work are excluded.

## Retained-timer ground-loss landing jump (GFMM9)

`--source-ground-loss-jump` / `--demo-source-ground-loss-jump` separately permits
the reviewed target4 jump after an accepted ground-loss landing8 entry while
698 is still positive. Earlier modes retain their refusal. This is jump-only:
positive698 target12 dash, air dash and relaunch remain refused, as do steep
push, ground-loss relaunch, clipping, combat and unsupported variants.

Read-only initialized-DOL disassembly of65E64,60228,6A750 and5F978 shows no698
reset in touchdown, landing entry, selector change or normal takeoff. Entry
clears the appropriate old motion/cache/selector fields but retains this
separate countdown. New takeoff2 does not integrate air motion or spend jump
resource; actual rising4 entry later applies launch15 and costs150.

The new policy requires `NativeBorgMotionOffer.controller_timer698` on **every
role**, not just the ground-loss ledger. Native acceptance derives the expected
value from the previous accepted same-world/same-producer offer and fixed native
tick decay; only a qualified ground-loss entry resets it30. Ledger and callback
timer evidence must agree with that carrier, and receipts compare it exactly.
Thus a fresh takeoff cannot discard698 simply by clearing ground-loss origin or
omitting walk metadata. Ordinary callback contacts still use the actual carried
gate. Source VM/resource/timer/pose changes remain staged until matching receipt.

This uses the same source admission, accepted-landing check, VM, movement,
resource and renderer. No touchdown skip, direct air launch, forced698 reset,
early150 spend or extra animation classification is introduced. Positive698
flight stays refused even if a new jump has cleared the ground-loss origin bit.

The scripted fixture retains the previous declared grounded original-stage
placement/query reference and4500 native bounds; target/stage data are unchanged.
It walks down-right from5 until accepted ground-loss entry, then genuinely holds
jump140–154 after accepted landing8. There is no midair teleport/rebind and no
original actor spawn or whole-frame equivalence claim.

| Accepted tick | Role / timer / resource |
| --- | --- |
| 139 | Ground-loss callback13/cache1 on original incline;698=12,10000 |
| 140 | Separate accepted landing8/cache0;698=11 |
| 141 | Jump target4 selects shared takeoff2, no movement/cost;698=10 |
| 150–151 | Takeoff continues while698 naturally decays1→0 |
| 152 | Lower takeoff done selects5, captured2/cache0, still10000 |
| 153 | Actual rising4 launch, cached87=1,150 spend;9850 |
| 154–155 | Continued rising spends100 per update;9650 |
| 156 | Released ordinary falling6, not ground-loss13, still9650 |
| 194 | Second real surface callback, captured6/cache1;9650 |
| 195 | Separate second landing8/cache0, still9650 |
| 220–600 | Shared idle0, common grounded recovery10000 |

`native_source_ground_loss_jump_owner` tests four600-update genuine routes:
jump entry timers10,5,2 and0 with second touchdowns194,169,203 and148. The
last route exercises natural expiry on the accepted jump boundary rather than
a positive carry. It checks exact shared resource timing, complete per-role
timer witnesses, stale/missing/reset metadata and receipt refusals, both
landings/pose lifetimes, older-policy rejection, touchdown-skip refusal and
continued positive698 dash/air-overlap refusal. Fresh placements and arena
references remain explicit native assumptions on unchanged original terrain.

Native setup/input/clocks/libm/camera/target/contact approximations remain
labelled, not original-hardware or whole-game equivalence. Other Borgs do not
inherit these timer, landing, hover or flight semantics.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-loss-jump-host
build/source-ground-loss-jump-host/gf_native \
  --demo-source-ground-loss-jump --headless --frames 600 --no-audio
```

The isolated host/all-header builds, targeted new host integration,600-tick demo
and unchanged64-tick ordinary duel smoke passed.
All six isolated sanitizer binaries passed, including prior ground-loss surface
and accepted-landing owners plus landing/jump/world regressions. The new owner
passed19048 checks across its four600-update routes and refusal boundaries.
Contact Sheet46 contains actual isolated offscreen GL frames139/140/141/150/
153/194/195/220 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-loss-jump-v1/contact-sheet-46.png`;
captures, game assets and private evidence are not published.
All158 isolated Python tests passed, including47 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged; assets, captures, private evidence,
builds and unrelated Story/preparation work are excluded.

## Timer-bound ground-loss surface landing (GFMM9)

`--source-ground-loss-surface` / `--demo-source-ground-loss-surface` separately
adds eligible selected-plane landing for the combined controller's ground-loss
path. The earlier ordinary-surface mode retains its flat-only ground-loss
restriction. Original stage/model/profile bytes and all other Borg mappings
remain unchanged.

Read-only initialized-DOL disassembly at`80067888–800678AC` selects classAE when
698 is positive and classAC otherwise. Qualified normal-G-Red operands are6144
and10240 respectively. Lower falling uses the existing program13/history
handoff,0.99 retained scalar and positive-timer direct-drift suppression.
This is not ordinary jump6 or proof that original actor constructors executed.

The shared source contact now receives the actual owned ground-loss countdown
only in the new mode. `NativeBorgSurfaceLanding.timer698` must equal the same
current-frame `NativeBorgWalkLedge.timer698` that independently matches prior
native acceptance and30→0 decay. Native validation checks the paired selected
plane, pre-response motion, signed16 angle, approach test and pre/final native
body/terrain budget against that gate. Matching forged zero timers cannot evade
the history chain; coherent45-degree metadata cannot borrow AC while positive
698 requires AE. No default-UP floor or steep-response fallback is invented.

65E64 clears69C/48/50, not698. Touchdown therefore keeps the remaining timer,
captured13 and cached87; accepted landing8 clears airborne history separately
while timer decay continues. Jump/dash landing cancellation during positive698
is deliberately refused in this mode until its follow-up lifetime is reviewed.
Natural expiry restores the earlier accepted-landing input permission. Positive
698 air-dash overlap and ground-loss relaunch also remain refused.

The default scripted fixture uses a declared **grounded** native placement at
x/z−1368.47/−628.674 with query reference94.339 and arena-half4500. These are
native demo assumptions on unchanged original terrain, not an original spawn
or forced actor height. Target/stage bytes are unchanged; no midair teleport or
rebind is used. D-pad down-right is held from5 until accepted falling entry,
then released. Interactive mode retains ordinary native spawn/bounds.

| Accepted tick | Role / timer / contact |
| --- | --- |
| 120 | Last walking motion leaves the ledge; retained Y, no immediate air integration |
| 121 | Priority correction/falling13 entry; native-owned frame start,698=30/cache1 |
| 138 | Still falling13,698=13, no fabricated landing or refill |
| 139 | Actual callback on inclined original plane at y≈16.71319, angle3911;698=12/cache1 |
| 140 | Separate accepted landing8/cache0;698=11, not reset |
| 150–151 | Landing continues while698 decays1→0 |
| 220–600 | Shared idle0, timer0, no flight/combat/resource spending |

`native_source_ground_loss_surface_owner` tests six600-update genuine original-
stage routes. Callback timer/angle pairs are12/3911,7/3676,2/2038,0/354,
4/0 and0/0. Placements/query references and4500 bounds are explicit native
fixtures, never midair state injection. One positive-timer route retains held
direction to verify direct-drift suppression/body-heading independence.
Tests cover timer/history/plane/scalar/receipt tampering, the stricter inter-gate
refusal, both prior-mode refusals, continuing landing/idle, positive landing-
input refusal, natural expiry and the existing positive698 air-overlap refusal.
The declared source resource remains on the shared kernel; a full meter here
does not certify an original actor/resource lifecycle.

Steep push, ground-loss relaunch, positive698 air/landing input, raised-surface/
side-entry, clipping, combat and unknown variants remain refused. Native
setup/input/clocks/libm/camera/target/contact approximations remain labelled,
not original-hardware or whole-game equivalence. This does not establish hover
or perpetual-flight contact rules for any Borg.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-loss-surface-host
build/source-ground-loss-surface-host/gf_native \
  --demo-source-ground-loss-surface --headless --frames 600 --no-audio
```

The isolated host/all-header builds, targeted new host integration,600-tick demo
and unchanged64-tick ordinary duel smoke passed. All six isolated sanitizer
binaries passed, including prior surface/walk-ledge owners and shared surface/
ground-correction/world regressions. The new owner passed22012 checks across
the six600-update routes plus timer/gate/history and input-refusal boundaries.
Contact Sheet45 contains actual isolated offscreen GL frames120/121/138/139/
140/150/151/220 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-loss-surface-v1/contact-sheet-45.png`;
captures, game assets and private evidence are not published.
All157 isolated Python tests passed, including46 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged; assets, captures, private evidence,
builds and unrelated Story/preparation work are excluded.

## Combined paired static-surface landing (GFMM9)

`--source-ground-surface` / `--demo-source-ground-surface` separately adds
eligible static-surface callback landing to the combined normal-G-Red controller.
Ordinary falling, released relaunch, active air dash and dash recovery use the
shared `gred_surface_contact`/`gred_air_dash_static_contact` callers. No parallel
physics, input, animation, resource or renderer engine is introduced.

Every new-mode `JumpTouchdown`, even on a flat plane, requires a distinct
`NativeBorgSurfaceLanding` payload in addition to its exact jump/air numeric
proof. It binds the pre-callback endpoint, post-integration velocity and
acceleration to the same selected height/normal/triangle and quantized angle.
Native acceptance re-queries that result and checks the qualified normal-G-Red
zero698 gate10240, motion epsilon`0x2edbe6fe` and bounded host displacement/
normal approach calculation. It does not fabricate an UP floor, accept height-
only metadata, or require the previous and selected plane heights to be equal.
The prior combined policy retains those narrower contact refusals.

Downward velocity, callback readiness, source pre-height/side-entry guards,
native pre-snap and final endpoint/body/bounds checks and the real callback
transition are still required. Snap/push metadata cannot substitute for landing.
Touchdown retains its captured air/fall program and cached87; accepted landing8
then clears that cache before ordinary resource recovery. All state and pose publication still
wait for the exact complete native receipt.

The default demo retains ordinary native spawn/arena/stage/target and one
producer. It holds jump5–20 and supplies a genuine backward air flick48–107,
releasing on actual touchdown before landing-entry input:

| Accepted tick | Phase / plane / resource |
| --- | --- |
| 49 | Wind-up entry10 at y209.8, no translation/cost;9550 |
| 58 | Last wind-up movement then450 spend before contact;9100 |
| 59 | Active entry11, source30 move/29.95 retained;9050 |
| 89 | Minimum active timer becomes callback-ready while still above ground;7550 |
| 108 | Actual active callback on original inclined plane y82.003227, angle7187;6600/cache1 |
| 109 | Separate accepted landing8 at the same selected height;6600/cache0 |
| 131–600 | Shared idle0; common recovery to10000 |

The prior mode stages that active callback candidate but native flat-only
acceptance rejects it without retiring the prior live frame. A shorter genuine
flick/release reaches recovery landing105 on another plane at y−5.238786,
angle562; the prior recovery source path refuses that nonflat contact and
retires authority. The new mode accepts both with their distinct contact
payloads, not a silent fallback.

`native_source_ground_surface_owner` tests five600-update routes: those active/
recovery cases, ordinary jump/fall and released relaunch on a declared original
shallow triangle, and an unchanged default-spawn flat landing. The shallow
fixtures start with a fresh **grounded** native seed at x/z−619.2617/169.4453,
height32.432983 and angle4448, not a midair teleport. Jump landing79 retains
ordinary6; released-relaunch landing171 retains21. Tests cover exact shared
resource accounting, selected-plane/integration/normal/angle/kind tampering,
height-only refusal, receipt/copy/pose lifetime and stable separate landing/idle.

Ground-loss landing deliberately keeps its existing flat/equal-height subset;
the new ordinary callback contract does not infer alternate positive698 gates.
A declared original steep-triangle jump still refuses on update82 before
landing, push, resource publication or fallback. Steep push, ground-loss relaunch,
positive698 air overlap, raised-surface/side-entry, clipping, combat and unknown
variants remain refused. Ordered dash geometry remains its earlier subset.
Native setup/input/clocks/libm/camera/target/contact approximations remain
labelled; this is not original-hardware or whole-game equivalence and establishes
no other Borg's movement-family semantics.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-surface-host
build/source-ground-surface-host/gf_native \
  --demo-source-ground-surface --headless --frames 600 --no-audio
```

The final isolated host/all-header builds,600-tick surface demo and unchanged
64-tick ordinary duel smoke passed. All six final isolated sanitizer binaries
passed; the new owner passed21061 checks across its five600-update routes plus
old-policy/steep/timer refusals and the separate pre-snap body-sweep regression.
Contact Sheet44 contains actual isolated offscreen GL frames49/59/89/107/108/
109/131/600 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-surface-v1/contact-sheet-44.png`;
captures, game assets and private evidence are not published.
All156 final isolated Python tests passed, including45 host integrations.
Original motion-bank/GFMM1–9 bytes are unchanged; assets, captures, private
evidence, builds and unrelated Story/preparation work are excluded.

## Combined wind-up entry-floor response (GFMM9)

`--source-ground-windup-entry-floor` /
`--demo-source-ground-windup-entry-floor` adds only the verified wind-up
entry-floor contract to the combined normal-G-Red controller. Both genuinely
reachable nonascending and positive-velocity entries are covered, with distinct
native kinds and sign domains. The older combined relaunch-floor policy still
refuses both, even with identical qualified profiles.

61338's entry halves44/48, clears4C and retains the data70 gravity operand in50,
but does not move, integrate/clamp velocity, or decrement the newly chosen turn
timer. It then queries677B0; null754 ret1 clears48 separately. Positive half-
speed at unchanged floor height is therefore not rising displacement. The
native air payload and paired selected-floor proof independently require the
actual pre-response half-speed, unchanged endpoint, timer/stage and exact
resource. No old jump/root, ordinary update, negative-entry or steep-push kind
can substitute for the positive-entry contract.

Entry spends no resource and keeps airborne cached87. On later wind-up updates,
timer expiry spends450 **after movement but before contact**, then selects16.
Active entry remains separate, costs50 before movement, and early-active snaps
retain accumulated negative48. Only the later ready-stage landing callback
grounds the actor; separate accepted landing8 then clears the airborne cache.
VM/timer/resource/counters and pose leases commit only after exact native receipt.

The default demo retains ordinary native spawn/arena/stage/target and one
producer. It holds jump5–30, genuinely re-presses78–86, then supplies right
flick86–124, releasing on touchdown before landing-entry input. The eighth
relaunch snap supplies positive48 at floor height; no actor injection,
teleport/rebind, stage edit or forced landing/refill is used.

| Accepted tick | Phase / contact / resource |
| --- | --- |
| 86 | Real relaunch snap crosses48≈0.3929, factor1;7700/cache1 |
| 87 | Wind-up entry10 halves pre48≈0.19645, unchanged position; contact zero48,7700 |
| 88–94 | Seven continued wind-up snaps; final movement then450 spend before contact,7250 |
| 95 | Separate active entry11/floor snap; retain48−0.07,7200 |
| 96–124 |29 continued active snaps, accumulating48≈−2.1;5750 |
| 125 | Ready-stage callback touchdown, captured11/cache1;5700 |
| 126 | Separate accepted landing8/cache0, still5700 |
| 148–600 | Shared idle0; common grounded recovery returns10000 |

The negative fixture instead flicks80 after two relaunch snaps. Entry81
halves old−7.808 to−3.904 without moving; contact then zeros48 at meter8300.
Handoff88 leaves7850, active entry89 leaves7800, touchdown119 leaves6300,
and separate landing120 clears the airborne cache.
`native_source_ground_windup_entry_floor_owner` checks both600-update routes,
sign-kind substitution, fabricated movement/gravity/cost/timer/callback,
receipt/pose lifetime, old-policy refusals and positive ground-loss698 refusal.
Saved legacy entry labels/fixtures are cross-checks, not a requirement for more
manual classification.

The capability is deliberately independent of the old push hierarchy.
Steep pushes, ground-loss relaunch, unresolved positive698 air overlap,
nonflat landing, raised-surface/side-entry, clipping, combat and unknown variants
remain refused. Native initialization/input/clocks/libm/camera/target/contact
assumptions remain labelled; this is not original hardware or whole-game
equivalence and establishes no other Borg's hover/flight semantics.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-windup-entry-floor-host
build/source-ground-windup-entry-floor-host/gf_native \
  --demo-source-ground-windup-entry-floor --headless --frames 600 --no-audio
```

The isolated host/all-header builds,600-tick positive-entry demo and unchanged
64-tick ordinary duel smoke passed. The source-order note above corrects an
earlier floor-demo description: the450 handoff cost follows movement but
precedes contact, not the other way around.
Contact Sheet43 contains actual isolated offscreen GL frames86/87/94/95/124/
125/126/148 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-windup-entry-floor-v1/contact-sheet-43.png`;
captures, game assets and private evidence are not published.
All six isolated sanitizer binaries passed, including the prior combined
relaunch-floor and legacy entry-floor owners plus air-dash/world regressions.
The new owner passed7445 checks across both600-update genuine routes, both
older-policy refusals and the positive698 refusal.
All155 isolated Python tests passed, including44 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged. Assets, captures, private evidence,
build outputs and unrelated Story/preparation work are excluded.

## Combined relaunch-floor response (GFMM9)

`--source-ground-relaunch-floor` / `--demo-source-ground-relaunch-floor` adds
only the verified null-callback relaunch static-floor contract to the combined
normal-G-Red controller. The previous airborne-relaunch mode retains its floor
refusal. Legacy push helpers are not used as a capability hierarchy: the source
owner reuses `gred_relaunch_static_contact`, explicitly refuses a steep-push
result in this mode, and publishes the shared VM/motion/pose only after matching
native acceptance.

Native acceptance cross-checks the complete relaunch scalar/velocity/endpoint/
resource/latch payload against the pre-response selected-floor payload. 6235C's
ret1 caller multiplies **negative** post-integration48 by0.8, retaining50,
scalar, independent lower/upper latches and airborne history. If positive
acceleration crosses48 above zero during a snap, its factor is1, not0.8.
Null754 does not invoke landing7, clear cached87 or refill resource. The next
positive-velocity update may lift off through the ordinary active path.
Depletion/release, falling21–24, actual callback touchdown and accepted landing8
remain separate transitions.

The default demo retains ordinary native spawn/arena/stage/target and one
producer: jump5–30, then genuine relaunch press78–242 held through depletion.
There is no airborne state injection, teleport/rebind, stage edit or forced
landing/refill.

| Accepted tick | Phase / contact / resource |
| --- | --- |
| 79 | Entry14 adds15 to old−29; move−14, integrate48−13.2 then snap/.8→−10.56;8400 |
| 80 | Continued14 null snap,48−9.76→−7.808;8300 |
| 83 | Fifth snap; lower/upper latches2 while captured14/14 still runs;8000 |
| 84 | Sixth snap; independent15/15 requests and latches3;7900 |
| 85 | Seventh negative48 snap;7800 |
| 86 | Eighth snap crosses48 positive≈0.3929, factor1;7700 |
| 87 | Ordinary active relaunch lifts above floor without another snap;7600 |
| 164 | Previous accepted meter0 selects separate falling21 and clears relaunch latches |
| 242 | Actual callback touchdown, program21/cache1, still meter0 |
| 243 | Separate landing8/cache0, still meter0 |
| 265–600 | Shared idle0; common grounded recovery returns10000 |

`native_source_ground_relaunch_floor_owner` tests three600-update genuine
routes: two snaps then release/land81; eight held snaps, positive lift87,
depletion/fall164 and land242; and four directional snaps then release/land83.
It checks exact shared150/100 costs, negative-only tail, retained acceleration/
latches/history, native endpoint/velocity/surface/callback tampering, receipt/
pose lifetime, stable idle and unchanged older-mode refusal.

The combined controller now genuinely reaches both negative and positive
wind-up **entry** floor candidates after relaunch snaps (flick80 or86). They
still refuse before air-dash spending/publication: relaunch-floor permission is
not wind-up-entry permission. These fixtures no longer need a legacy-only
controller, but accepting their distinct contract remains a later scoped step.

Steep push, wind-up entry snaps, ground-loss relaunch, positive698 air overlap,
nonflat landing, raised-surface/side-entry, clipping, combat and unknown variants
remain refused. All initialization/input/clocks/libm/camera/target/contact
assumptions remain native proxies, not original hardware or whole-game
equivalence. No other Borg inherits this mapping or a hover/flight rule.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-relaunch-floor-host
build/source-ground-relaunch-floor-host/gf_native \
  --demo-source-ground-relaunch-floor --headless --frames 600 --no-audio
```

The isolated host/all-header builds,600-tick demo and unchanged64-tick ordinary
duel smoke passed. Contact Sheet42 contains actual isolated offscreen GL
frames79/84/86/87/164/242/243/265 without taking desktop/classifier focus.
It remains local at
`local/recomp-evidence/contact-sheets/source-ground-relaunch-floor-v1/contact-sheet-42.png`;
captures, assets and private evidence are not published.
All six isolated sanitizer binaries passed. The new owner passed11024 checks
across its three600-update routes, old-policy refusal and both genuine
wind-up-entry refusals, alongside prior combined and legacy relaunch-floor
owners plus relaunch/world regressions.
All154 isolated Python tests passed, including43 host integrations. Original
motion-bank/GFMM1–9 bytes are unchanged; the published checkpoint excludes
assets, captures, private evidence, builds and unrelated Story/preparation work.

## Combined airborne relaunch (GFMM9)

`--source-ground-relaunch` / `--demo-source-ground-relaunch` separately adds
reviewed normal-G-Red airborne relaunch to the shared ground/ledge/jump/accepted-
landing/air-dash/floor controller. Older modes retain their narrower permissions.
It does not inherit the legacy relaunch-floor/push lab hierarchy.

The source owner uses the existing relaunch planner, heading/acceleration,
motion driver, VM, resource, stage query and renderer. Its distinct relaunch
payload independently checks additive15 entry before displacement, previous
accepted retained scalar/resource, neutral0.98 damping versus directional
45-degree acceleration, source8/18/−30 clamps, exact endpoints,150/100 costs
before motion and lower/upper stage latches. It does not replace old velocity
by15 or infer scalar from rendered displacement. Receipts compare the complete
relaunch payload; rejected native candidates preserve the prior accepted frame.

Lower completion changes stage1→2 while still updating14; the following lower
update requests15/stage3. Upper stage1 observes that lower change but still
updates14 before requesting15 on its own following stage2 update. No generic
“mirror lower” shortcut is used. Resource, VM, both latches, counters and pose
leases publish only after exact native acceptance.

Release or depletion selects the distinct verified21–24 falling family through
explicit relaunch origin/coarse57D metadata, not ordinary jump6, post-air13 or
ground-loss history. The selector8→6 admission clears relaunch latches, retaining
the separate falling-origin/coarse metadata instead. Relaunch may interrupt
active air dash/recovery; a genuine
air flick may interrupt relaunch and restore air-dash lineage. Ordinary ground
input, owned dash/jump, landing-entry cancellation and scoped air-floor behavior
remain in the same controller/producer.

The default demo keeps ordinary native spawn/arena/stage/target: hold jump5–30,
release, re-press60–99, release, fall and land. No airborne state injection,
teleport/rebind, stage edits or further manual animation classification is used.

| Accepted tick | Role / latches / programs / resource |
| --- | --- |
| 61 | Relaunch entry adds15 to old−11, moves4;14/14, stages1/1,8400 |
| 65 | Lower completes; both latches2 but captured14/14 still used,8000 |
| 66 | Each lane separately requests15; stages3/3,7900 |
| 100 | Held relaunch, source upper18 clamp;4500 |
| 101 | Release selects falling21 with−1 gravity, retains airborne history |
| 171 | Real callback touchdown, captured21/cache1,4500 |
| 172 | Separate landing8/cache0, still4500 |
| 194–600 | Shared idle0 and common grounded recovery to10000 |

`native_source_ground_relaunch_owner` tests five600-update genuine routes:
neutral release, directional lift, held-resource depletion (fall146/land243),
active-air-dash interruption (relaunch81/fall121/land200), and air flick out of
relaunch (wind-up81/recovery118/post-air fall147/land151). It checks exact shared
resource accounting, latch/scalar/velocity/proof tampering, receipt/pose lifetime,
older-mode input refusal and a genuine relaunch-floor refusal before spending.

Relaunch-floor contact and wind-up entry-floor snaps still refuse, as do steep
push, relaunch from ground-loss history, positive ground-loss698 air overlap,
nonflat landing, raised-surface/side-entry, clipping, combat and unknown variants.
An input sweep of96 bounded original-stage jump/flick routes found no wind-up
entry-floor fixture without relaunch; that is not proof of universal
unreachability and does not widen its policy. The sweep logs remain private
under `/tmp/opencode/ground-air-entry-sweep-v1/`.

All initialization, input mapping, clocks, libm/camera/target/contact assumptions
remain explicitly native proxies, not original hardware or whole-game
equivalence. No other Borg inherits G-Red's mappings or hover/flight rules.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-relaunch-host
build/source-ground-relaunch-host/gf_native \
  --demo-source-ground-relaunch --headless --frames 600 --no-audio
```

Contact Sheet41 contains actual isolated offscreen GL frames61/65/66/100/101/
171/172/194 without taking desktop/classifier focus. It remains local at
`local/recomp-evidence/contact-sheets/source-ground-relaunch-v1/contact-sheet-41.png`.
The final isolated owner sanitizer passed18546 checks across its five600-
update routes plus older-mode/floor refusals. The final isolated host/all-header
builds,600-tick demo and unchanged64-tick ordinary duel smoke passed.
All six final isolated sanitizer binaries passed, including the previous
ground-air/floor owners and relaunch/jump/world regressions.
All153 final isolated Python tests passed, including42 host integrations.
Original motion-bank/GFMM1–9 bytes are unchanged. Assets, private evidence,
captures, builds and unrelated Story/preparation changes are excluded.

## Combined ground/air floor response (GFMM9)

`--source-ground-air-floor` / `--demo-source-ground-air-floor` is a separate
permission over the shared combined controller. It joins verified continued
wind-up and early-active static floor responses without inheriting the legacy
relaunch/push capability hierarchy. `--source-ground-air` and all older modes
remain unchanged.

The source owner reuses `gred_air_dash_static_contact`, the shared motion/VM/
resource driver and the existing stage query/renderer. Native acceptance checks
both the selector15/16 phase/timer/resource/integration payload and the paired
selected-floor payload, including the exact pre-response endpoint and velocity.
61338's continued wind-up ret1 tail clears48, while61560's early-active caller
has no such tail: its accumulated negative48 and −0.07 acceleration50 survive
the height snap. Neither null754 path grounds the actor, clears cached87,
increments landing, or refills resource. Only the later ready-stage callback
can select landing7; separate accepted landing8 then clears the airborne cache.
All effects remain staged until the matching complete native receipt.

The default demo uses ordinary native spawn/arena/stage/target with jump held
5–20 and a genuine forward air flick60–96, releasing on touchdown before
landing-entry input. No airborne state injection,
teleport/rebind, stage edits or additional animation classification is used.

| Accepted tick | Phase / contact / resource |
| --- | --- |
| 61 | Wind-up entry10 at y35.8, old−21→−10.5 velocity; no movement/cost,9550 |
| 64 | First continued wind-up floor snap at y10, pre48−10.71→0;9550 |
| 65–66 | Two more wind-up snaps; last motion precedes450 spend, then contact,9100 |
| 67 | Active entry11 moves30 and snaps height; retains48−0.07,9050 |
| 68–96 |29 more early-active snaps;48 accumulates to approximately−2.1,7600 |
| 97 | Active timer reaches ready stage; real callback touchdown,7550/cache1 |
| 98 | Separate landing8 entry,7550/cache0 |
| 120–600 | Shared idle0 with common grounded recovery to10000 |

Two nearby genuine flick routes cross-check five wind-up/thirty active snaps
(flick61, touchdown98) and zero wind-up/eight late early-active snaps (flick58,
touchdown95). `native_source_ground_air_floor_owner` covers all three600-update
routes, exact shared resource accounting, phase/timer/contact/endpoint tampering,
receipt/pose retirement, unchanged older-policy refusal and positive698 refusal.

Wind-up **entry** floor snaps, including positive half-speed at unchanged floor
height, remain refused in this combined policy: the older qualified entry labs
reach them through relaunch, which this policy does not authorize. No fixture
has established their genuine combined-controller admission yet. Relaunch,
steep push, unresolved positive ground-loss698, nonflat landing, raised-surface/
side-entry, clipping, combat and unknown variants also remain refused.
This is a normal-G-Red native proxy subset, not original hardware or whole-game
equivalence, and does not establish hover/flight rules for other Borgs.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-air-floor-host
build/source-ground-air-floor-host/gf_native \
  --demo-source-ground-air-floor --headless --frames 600 --no-audio
```

Contact Sheet40 contains actual offscreen GL frames61/64/66/67/96/97/98/120
from the isolated native-only tree, without taking desktop/classifier focus.
It remains local at
`local/recomp-evidence/contact-sheets/source-ground-air-floor-v1/contact-sheet-40.png`;
no captures, game assets or private evidence are published.

The final isolated floor-owner sanitizer rerun passed12124 checks. Five prior
air/entry-floor/kernel/world sanitizer binaries also passed against byte-identical
test sources and engine headers. The final isolated host/all-header builds,
600-tick floor demo and unchanged64-tick ordinary duel smoke passed.
All152 final isolated Python tests passed, including41 host integrations. The
owner regression explicitly requires program0/0 idle after releasing on
touchdown, rather than accepting a held-through-landing stop program as idle.
Original motion-bank/GFMM1–9 bytes are unchanged; the checkpoint excludes assets,
captures, private evidence, builds and unrelated Story/preparation work.

## Combined ground/air-dash controller (GFMM9)

`--source-ground-air` / `--demo-source-ground-air` joins the recovered normal
G-Red air-dash wind-up/active/recovery subset to the existing ground/ledge/jump/
accepted-landing-input controller. It reuses the same VM, motion driver, resource,
contact/receipt and renderer systems. Older modes retain their narrower policies.

This capability is deliberately independent of relaunch and the later floor/
push labs. Wind-up/early-active null-callback floor response, steep push, relaunch,
nonflat landing, native clipping and unknown root/geometry variants refuse.
61338 does not clear698: a new air dash during positive ground-loss timer698 also
refuses until that overlap has its own qualified contact contract. This is not a
new universal movement-family rule.

The native air payload distinguishes source selectors15/16/17 from jump and
ground-loss evidence. It independently checks actual entry/continuation order,
stage/timer/callback readiness, pre-integration scalar/velocity, −0.05 active
deceleration, −0.07 wind-up/active gravity,0.99 recovery damping/4-unit drift/−1
gravity, exact endpoints and selected flat contact. Wind-up entry halves44/48
without moving or applying gravity. The450 handoff cost occurs after the final
wind-up motion; active50 cost precedes motion. No old jump/root payload can mint
that authority, and receipts compare the complete air evidence.

Recovery tests lower completion after motion/contact. A real callback touchdown
remains separate from “recovery completed while still airborne”; the latter
selects the distinct post-air program13 falling path. Its retained scalar can
exceed ordinary jump's8-unit bound, so the explicit air-origin lineage—not a
blanket relaxed jump limit—carries that value through falling/landing. Copied
ground-loss origin/timer or ordinary-jump metadata cannot substitute for it.
Resource recovery/costs and all VM/phase/counter/pose changes remain staged until
matching native acceptance.

The unchanged-spawn/arena/stage/target demo holds the source jump/rise input5–349
and supplies two genuine air flicks45–86 and90–131, then recovery and landing:

| Accepted tick | Role / programs / resource |
| --- | --- |
| 46 | Wind-up entry10: old18→9 velocity, no translation/cost; meter7150 |
| 51 | Last wind-up motion10, then450 spend; next selector16, meter6700 |
| 52 | Active entry11 moves30, retains29.95; zero48 then−0.07, meter6650 |
| 88 | Recovery12 after callback-ready active release; meter4900 |
| 91 | Fresh flick interrupts recovery with another wind-up10 |
| 100 | Second active entry11, meter4400 |
| 133 | Second recovery12, meter2800 |
| 159 | Actual flat callback touchdown, captured12/cache1 |
| 160 | Separate landing8/cache0, still2800 |
| 182–600 | Shared idle0, one landing; common recovery10000 |

The same world/producer/binding is retained, with no airborne teleport or edited
stage/target. A high-jump route reaches recovery completion first and enters
post-air falling at150 before touchdown168. A held-active resource-depletion
route enters recovery156 with meter0, falls after completion185 and lands196.
That long held-active/depletion fixture declares native arena-half4500 to avoid
the ordinary endpoint bound; it does not move/teleport the actor or edit stage
data. The default two-dash demo retains ordinary arena/spawn settings.
These are declared native input/clock/libm/camera/target/contact fixtures, not
original hardware, full actor construction or whole-frame equivalence.

`native_source_ground_air_owner` covers those three600-update original-stage
routes plus positive698 refusal, phase/root/floor/push separation, early-spend/
movement/callback tampering, exact shared costs/recovery, airborne completion
lineage, receipt/pose retirement and stable idle. No extra animation
classification or other-Borg flight/hover inference is introduced.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-air-host-v1
build/source-ground-air-host-v1/gf_native \
  --demo-source-ground-air --headless --frames 600 --no-audio
```

Contact Sheet39 shows actual offscreen GL frames46/51/52/88/91/133/159/182
without taking desktop/classifier focus. Isolated native-only wind-up46 and
touchdown159 match the full working tree pixel-for-pixel. Captures remain local:
`local/recomp-evidence/contact-sheets/source-ground-air-v1/contact-sheet-39.png`.
The isolated new-owner test passed11768 checks across its three600-update
original-stage routes plus positive698 refusal. The isolated host suite passed
all151 Python tests, including40 opt-in host integrations. Host/all-header builds
and the unchanged64-tick ordinary duel smoke also passed.
All20 isolated sanitizer binaries passed, including prior controller owners and
legacy air-dash/jump/landing kernels. Original motion-bank/GFMM1–9 bytes remain
unchanged; assets, private evidence, captures, builds and unrelated work are
excluded from the checkpoint.

## Accepted-entry landing jump/dash responsiveness (GFMM9)

`--source-ground-landing` / `--demo-source-ground-landing` adds the already
recovered landing jump/ground-dash predicates to the combined ground/jump
controller. They run before the old landing dispatch, but **only after a
previously accepted landing-entry update**. Physical touchdown alone is not that
permission. The shared VM, driver, root sampler, motion/contact, producer,
resource and renderer systems are reused.

The owner requires selector7 with an entered landing callback, grounded native
contact and cached87=0, plus the reviewed unblocked upper/lower fields. Source
buffered/held jump selects4 and resets normal-class takeoff motion; a full fresh
flick selects12 through the owned-root ordered-dash path. Neither branch advances
the old landing program first. A jump still takes off at zero displacement;
an ordered landing dash immediately moves25 and retains24.5 after−0.5.

A separate native policy and exact target4/12 evidence bind each interruption
to the preceding accepted landing-entry role. Missing, unknown, swapped or
premature targets refuse. The target is also part of exact receipt comparison.
The new permission cannot mint root/VM provenance for a dash, skip a touchdown
callback, turn arbitrary ground motion into a landing cancel or grant air dash,
relaunch, combat, clipping or nonflat landing. Existing modes—including
`--source-ground-jump`—retain their cancellation refusals. Helper capability
groups explicitly avoid inheriting the legacy air-dash/relaunch hierarchy.

Resource recovery is not special-cased on interruption. The common5568C branch
uses the **previous accepted** cached87 and recovery operands before admission;
664A0 still handles actual jump costs. Thus touchdown and the first landing-entry
update retain the depleted meter, while the following grounded update can
legitimately recover it—even when a jump/dash is admitted on that update. Tests
model the exact shared recovery/cost kernels rather than assuming “no refill”
means “the value can never increase during an interruption.” All resource,
VM/root, counter and pose state remains staged until matching native acceptance.

The default demo uses unchanged native spawn/arena/stage/target. It supplies one
neutral held jump at5–34, then requests the next jump and later backward dash
from **accepted owner counters/landing state**, not guessed clip timers:

| Accepted tick | Role / programs / resource |
| --- | --- |
| 85 | Actual first touchdown, falling6/cache1, meter8150 |
| 86 | Accepted landing entry8/cache0, still8150 |
| 87 | Target4 interruption: takeoff2, no old landing/root/motion; common recovery10000 |
| 99 | Second ascent4, launch cost leaves9850 |
| 150 | Second actual touchdown6/cache1, meter9150 |
| 151 | Accepted landing entry8/cache0, still9150 |
| 152 | Target12: fresh ordered backward dash29, move25/retain24.5; common recovery10000 |
| 600 | Stable idle0 after dash recovery; two landings, one landing jump/dash |

The second jump is held20 updates from its accepted-entry trigger; the landing
dash12 updates. No actor is teleported and the world, producer, model/driver/VM
and render binding are not replaced.

`native_source_ground_landing_owner` covers three600-update original-stage
routes: ordinary jump landing → jump → dash; ground-loss landing → jump → dash;
and ordinary landing → dash. It also checks raw touchdown/old-mode refusals,
counter/resource staging, common cached-history recovery, missing/swapped/stale
native targets, root authority and exact receipts/retirement. Native clock,
input mapping, libm, camera, target-center and collision remain explicit proxies,
not original actor construction or hardware/full-frame equivalence.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-landing-host-v1
build/source-ground-landing-host-v1/gf_native \
  --demo-source-ground-landing --headless --frames 600 --no-audio
```

Contact Sheet38 shows actual offscreen GL frames85/86/87/99/150/151/152/184
without taking desktop/classifier focus. Isolated native-only landing-jump87 and
landing-dash152 match the full working tree pixel-for-pixel. Captures stay local:
`local/recomp-evidence/contact-sheets/source-ground-landing-v1/contact-sheet-38.png`.
The isolated new-owner test passed9299 checks across three600-update original-
stage routes plus raw touchdown/old-mode refusal fixtures. Host/all-header builds
and the unchanged64-tick ordinary duel smoke also passed.
The isolated host suite passed all150 Python tests, including39 opt-in host
integrations. Original motion-bank bytes and all immutable GFMM1–9 packs are
unchanged.
All18 isolated sanitizer binaries passed, including prior controller owners,
legacy landing-input/dash-jump kernels and native battle regressions. Game assets,
private evidence, captures, build outputs and unrelated work are excluded.

## Ground/ledge/jump controller with ordered dash interruption (GFMM9)

`--source-ground-jump` / `--demo-source-ground-jump` extends the combined
normal-G-Red ground controller with the already recovered takeoff, rising,
release/depletion falling and flat landing subset. Walking, stopping, owned-root
dash, both ground-loss origins and jumping use one VM, driver, world, producer
and render binding. Older controller modes retain their narrower permissions.

A buffered/held jump can interrupt active or recovery ground dash **before**
the old dash callback, root evaluation or motion. Normal class12 resets44/4C
and48/50 during takeoff and requests program2 without moving or spending launch
resource. The old root lease is only retired when that takeoff is accepted.
Ascent waits for actual lower-program completion, requests4, integrates the
initial15 velocity, then applies the source directional acceleration/clamps.
Launch costs150 and continued rise costs100 per accepted update. Release or
depletion enters normal falling program6, distinct from ground-loss program13.

Native jump evidence independently checks source phase/entry/completion order,
pre-integration speed, planar/vertical acceleration, the8/18/−30 bounds, retained
0.99 falling momentum, optional4-unit drift, exact endpoints and actual selected
flat contact. It cannot skip takeoff/landing completion, carry dash momentum into
takeoff, manufacture ascent acceleration, borrow ground-loss lineage or replace
a required ground-loss payload. Receipts compare every evidence field. VM,
resource, phase, root/counters and pose remain staged until that exact receipt.

The selector helpers explicitly separate dash-jump permission from relaunch and
air-dash permission; enabling this mode does not inherit either. Fresh takeoff,
airborne or landing cancellation presses, airborne flick/dash, relaunch, combat,
nonflat landing, clipping and unknown root/geometry variants refuse. Held rise
is the reviewed normal-G-Red jump/resource path, not support for another Borg's
perpetual-flight/hover semantics. Native jump/air input is limited to32 units;
this is still a convenience mapping, not original device calibration.

The unchanged-spawn demo supplies walking5–24, neutral25–26, a backward
flick27–34, held jump/direction35–70, then release. It never teleports or rebinds:

| Accepted tick | Role / captured programs |
| --- | --- |
| 28 | Ordered dash entry27 moves immediately |
| 35 | Active dash,7 actual ordered-root evaluations |
| 36 | Jump wins before old dash motion; takeoff2, zero movement, meter10000 |
| 48 | Actual ascent entry4: old15 velocity then acceleration; meter9850 |
| 72 | Release selects normal falling6; meter7550, no new ordered-root evaluation |
| 124 | Actual flat touchdown at native selected Y≈35.04099, still6/cache1 |
| 125 | Separate landing8/cache0, no premature resource recovery |
| 147–600 | Completed landing returns shared idle0; meter10000, stable position |

The selected floor is queried, not forced to10. A completed ordinary jump landing
may return to ground control without falsely requiring a previous ledge payload.
Incomplete landing still cannot do so. A pending ground-loss correction retains
priority over late jump input; it is not converted into an airborne takeoff.

`native_source_ground_jump_owner` exercises eight600-update original-stage
routes: neutral, walking, active dash, recovery dash, mixed walk/stop/dash/jump,
held-resource depletion, walking ledge and dash ledge. It checks interruption
order/root retirement, kinematic and lineage tampering, resource staging,
camera-independent heading, selected-floor landing, completed-ground return,
unsupported cancellation and stable idle. No new animation classification or
non-G-Red mapping is inferred. Native initialization, clocks, libm, camera,
target-center and contact remain labelled proxies, not hardware/full-frame
equivalence.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-jump-host-v1
build/source-ground-jump-host-v1/gf_native \
  --demo-source-ground-jump --headless --frames 600 --no-audio
```

Contact Sheet37 shows actual offscreen GL frames25/28/35/36/48/72/124/150 without
taking desktop/classifier focus. Isolated native-only takeoff36 and touchdown124
match the full working tree pixel-for-pixel. Captures remain local and outside Git:
`local/recomp-evidence/contact-sheets/source-ground-jump-v1/contact-sheet-37.png`.
The isolated new-owner test passed23478 checks across eight600-update original-
stage routes plus late-jump priority/refusal and old-mode capability checks.
Host/all-header builds and the unchanged64-tick ordinary duel smoke also passed.
The isolated host suite passed all149 Python tests, including38 opt-in host
integrations. Original motion-bank bytes and all immutable GFMM1–9 packs remain
unchanged.
All16 isolated sanitizer binaries passed, including the older combined-ground,
dash-only, walk-only, ordered-ground and legacy jump/ground owner regressions.
Game assets, private evidence, captures, build outputs and unrelated work are
excluded from the checkpoint.

## One walking/ordered-dash ground-ledge controller (GFMM9)

`--source-ground-ledge` / `--demo-source-ground-ledge` combines the verified
walking, stop and owned-root ordered-dash paths in **one opt-in controller**.
Both walking and dash departures use the existing next-frame priority
correction, program13, timer698/direct drift and flat landing. The VM, driver,
producer, world and render binding stay the same through walking, stopping,
flicking, dashing, falling, landing and idle. No parallel controller or animation
mapping is introduced.

The combined native policy keeps the two departure contracts distinct. A
walking candidate must use a qualified normal-G-Red walking scalar and pass the
walking tangent check; its larger dash/fall budget cannot authorize speed25.
An ordered dash still needs its own phase, VM/root provenance and post-motion
scalar. Removing a dash payload cannot turn an in-progress dash into walking.
Airborne damping continues from the accepted origin's scalar, not an inferred
animation label, rendered displacement or root value. Walking never mints an
ordered-root lease; historical ordered-root diagnostics are not airborne
authority.

Older policies are unchanged: walk-ledge remains walk-only, dash-ledge refuses
walking ledges and ordered-ground refuses all ledges. The combined mode still
refuses jump/air-dash/relaunch/flight/combat, nonflat dash surfaces/landings,
clipping and unowned input/root/history/geometry variants. It is normal-G-Red
only, with declared native clock, camera, target-center and contact proxies—not
original actor construction or hardware/whole-frame equivalence.

The recovered input window matters: pushing a long-held low-deflection walk to
full deflection does **not** fabricate a fresh flick. Release and flick again.
The native demo supplies that actual input, rather than forcing a selector:

| Accepted tick | Same-controller role |
| --- | --- |
| 6–70 | Walking program1 on original roof249 |
| 71–72 | Real neutral release/stop, program31 |
| 73 | New flick requests27 and moves25; no extra walking motion |
| 80 | Held-active dash departure; retained22, flags20/cache0,7 root evaluations |
| 81 | Following priority entry13; scalar21.78, timer30, flags40/cache1 |
| 111 | Timer0 permits one direct camera-relative drift update |
| 112 | Actual flat touchdown on captured13/cache1 |
| 113 | Separate landing8/flags200/cache0 |
| 600 | Stable idle0; one dash-origin departure/landing, meter10000 |

The demo starts once on real ground at X/Z`(1010.166015625,2315.240722656)`,
floor reference482.747558594 and arena-half4500. Target and stage bytes are
unchanged. Input is walk5–69, neutral70–71, flick72 onward through one post-delay
drift, then release. There is no world restart, actor teleport, controller/driver
replacement or rebind inside the sequence. Interactive spawn/bounds are unchanged.

`native_source_ground_ledge_owner` runs four600-update original-stage routes:
walking alone; the mixed walk/stop/flick/dash route; recovery dash departure; and
a sustained-walk/full-push route that correctly stays walking without a new
flick. It checks walking versus dash provenance, no double entry motion, one
producer/world/render binding, staged rejection, history, camera-independent
body heading, resource/pose lifetimes, older-pack refusal and stable idle.
The existing narrower-policy owner tests remain regressions.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-ground-ledge-host-v1
build/source-ground-ledge-host-v1/gf_native \
  --demo-source-ground-ledge --headless --frames 600 --no-audio
```

Contact Sheet36 shows actual offscreen GL frames69/71/73/80/81/111/112/150
without taking desktop/classifier focus. Isolated native-only entry73 and
touchdown112 match the full working tree pixel-for-pixel. Captures remain local:
`local/recomp-evidence/contact-sheets/source-ground-ledge-v1/contact-sheet-36.png`.
The isolated host suite passed all148 Python tests, including37 opt-in host
integrations. Host/all-header builds and the unchanged64-tick ordinary duel smoke
also passed. Original motion-bank bytes and immutable GFMM1–9 packs are unchanged.
The isolated combined-owner test passed9910 checks across its four600-update
original-stage routes, including sustained-walk input and the genuine mixed route.
All15 isolated sanitizer binaries passed, including the existing walk-only,
dash-only and ordered-ground owner regressions. Assets, private evidence, build
outputs and unrelated Story/preparation changes remain outside this checkpoint.

## Live ordered dash-to-ledge handoff (GFMM9)

`--source-dash-ledge` / `--demo-source-dash-ledge` adds a **separate** native
permission for normal G-Red's ordered ground dash to leave a verified static
ledge. It reuses the owned-root dash, native-owned frame-start history, shared
priority correction, VM, falling/contact and renderer systems. No new movement
engine or per-Borg animation mapping is introduced.

The departure frame is still a ground-dash controller update. It moves using
the ordered entry/active/recovery speed and then applies the ground scalar
change, including the recovery done/reset tail if present. A strict downward gap
beyond60 retains incoming Y/zero48. Physical native air does **not** mean a
takeoff callback, immediate falling program or resource spend occurred.

Only the following accepted frame's higher-priority correction selects6/marker1,
then program13 sets airborne flags/cache and begins falling. That correction
uses the exact same-world/tick frame-start snapshot; it cannot reconstruct
momentum from the last ground displacement, an earlier render pose or1DAC.
Falling retains the **post-ground-update** scalar, multiplies it by0.99 and
integrates old48 before applying gravity. Timer698 starts30 and suppresses only
direct4-unit camera-relative drift, not retained scalar momentum.

Native acceptance cross-checks the existing sealed dash/VM/root evidence against
the separate ground-loss payload on departure. Walking departures cannot borrow
this permission. Airborne frames cannot carry a copied ordered-root offer.
The accepted handoff retires the old root lease; the numeric last-ordered-root
diagnostic may remain, but is not fresh airborne root authority. All selector,
flags, history, root/counter and pose changes remain staged until exact matching
native acceptance. Native rejection preserves the prior frame; source refusal
retires it without implicit MVP fallback.

All earlier policies remain unchanged: ordered-ground still refuses every
ledge; walk-ledge still refuses flick/dash input. The new mode refuses walking
ledges, jump/air-dash/relaunch/flight/combat, nonflat dash surfaces/landings,
unresolved root/multiplier/history/geometry variants and native clipping. The
native input mapper limits fall/landing deflection to32 source units so held
stick input does not manufacture a fresh airborne flick. This is a declared
native convenience, not recovered device calibration. A newly buffered flick
cannot outrank pending ground loss; its later unsupported airborne admission
refuses rather than being silently consumed.

The demo declares a **fresh native ground spawn** on the original roof at X/Z
`(1012.360657,1403.599731)`, floor reference482.747589 and arena-half4500.
Target and all stage bytes are unchanged. It holds one forward flick from5
through the first post-delay drift, then releases. Interactive spawn/arena are
unchanged; no airborne position is teleported and no original spawn script is
claimed.

| Accepted tick | Role / captured programs | State |
| --- | --- | --- |
| 6 | Dash entry/departure /27 | Moves25, retains24.5; Y held, flags20/cache0 |
| 7 | Priority falling entry /13 | Scalar24.255001, old zero48 then−1;698=30, flags40/cache1 |
| 8–36 | Falling /13 | Retained scalar×0.99; direct input drift inhibited |
| 37 | Falling /13 | 698=0; one direct drift update, body heading unchanged |
| 38 | Actual flat touchdown /13 | Y=10; flags40/cache1 still captured |
| 39 | Landing /8 | Flags200/cache0, separate from touchdown |
| 600 | Stable idle /0 | One departure/entry/landing, meter10000, no combat |

Additional read-only original-stage fixtures cover:

- Active departure from native Z1520: tick11 uses22.5, retains22; falling starts12
  and touchdown43 follows.
- Held-active departure from Z1600: tick14 uses/retains22 without inventing a
  new AObj/root evaluation; falling starts15 and touchdown46 follows.
- Recovery departure from Z1600 with input released at6: tick14 uses actual root
  speed≈18.750244, retains≈18.250244 after−0.5; falling starts15 and touchdown46
  follows. It does not inherit root speed, movement distance or legacy speed22.

These are labelled native spawn/input/clock/libm/target/contact fixtures, not
original hardware or whole actor-frame equivalence. All four start on real
ground and run600 updates. `native_source_dash_ledge_owner` tests their phase,
root/history/camera/receipt lifetimes, priority over a fresh flick, edited or
missing evidence, held-root semantics, stable landing and older-mode refusals.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-dash-ledge-host-v1
build/source-dash-ledge-host-v1/gf_native \
  --demo-source-dash-ledge --headless --frames 600 --no-audio
```

Contact Sheet35 contains actual offscreen GL frames5/6/7/36/37/38/39/75 without
taking desktop/classifier focus. Isolated native-only departure6 and touchdown38
match the full working tree pixel-for-pixel. Captures stay local and outside Git:
`local/recomp-evidence/contact-sheets/source-dash-ledge-v1/contact-sheet-35.png`.
Isolated validation passed14 sanitizer binaries, all147 Python tests (including
36 opt-in host integrations), host/all-header builds and the unchanged64-tick
ordinary duel smoke. The new owner passed10313 checks across four600-update
original-stage routes. All GFMM1–9 packs and original motion-bank bytes remain
unchanged; game assets, private evidence and unrelated work are excluded.

## Live owned-root ordered ground dash (GFMM9)

`--source-ordered-ground` / `--demo-source-ordered-ground` connects the ordered
dash candidates to the existing movement owner, VM, motion driver, numeric root
evaluator, native contact/receipt and shared renderer. Other modes retain their
earlier dash prefix. Walk-only ledge mode still cannot acquire dash permission.

This is a deliberately **ground-only** checkpoint: walking uses the existing
shared controller; dash motion requires a flat, unchanged-height floor. Ledges,
airborne motion, jumping/flight/combat, nonflat dash surfaces and any native
body/bounds/terrain clipping refuse. The ordinary MVP is unchanged.

The owner requires the original355264-byte motion bank (FNV`276b4e9e`),
in addition to the existing model/program/profile fingerprints; edited motion
bytes cannot mint original root-speed authority. It finds bone0's actual lane
in the qualified pose rig. Around each
indicated dash VM request/advance it captures that lane's exact immutable driver
snapshots, checks library/successor identity and evaluates the shared root kernel
from the raw AObj observations. This is **not** root speed inferred from model
rendering, world displacement, a previous offer or an animation label.

The supported detached path declares native delta5F4/1DC8=1, bias1D9C=0 and
suppress-selector1DB2=0. It reads actual lane1 metadata and requires1D9B=0: root
speed is computed, but actor root position is never applied a second time.
Public-null roots, alias/mirror-root substitutions and selected multipliers
without an owned table remain refusals. No new manual classification is needed.

Each post-VM root read receives a sealed, weakly published lease carrying the
world, producer, exact observed tick, root speed/evaluation count and actual VM
phase/control/completion result. The lease cannot be publicly constructed or
copied; a float payload cannot mint that authority. Native acceptance checks it and
independently verifies phase order, active minimum/timer rules, recovery damping/
root latch, scalar update, exact flat-ground displacement and post-motion reset.
The native per-step scalar/root budget is25. This is a scoped native contract,
not original actor construction, input calibration or whole-frame equivalence.

The distinction between **reading** a root state and **evaluating** a new root
delta matters. A control-bit1 hold performs neither VM advance nor root sample.
The actual final done-only VM operation can leave the root snapshot unchanged.
That tail seals a fresh current-frame read of the retained root value without
inventing an AObj/root update. Non-successors outside that exact done case refuse.
Control3 is acknowledged through a narrow class-scoped VM write; no motion,
clock, root, secondary control or callback authority comes with that write.

All root/VM/phase/counter state remains pending until the exact matching native
receipt. A rejected native candidate leaves the old accepted frame live; fatal
source refusal or retirement revokes pending/root/pose leases. Receipts compare
the complete ordered evidence, including lease identity, not just position.

The native demo uses the **unchanged** ordinary spawn, target, stage and arena.
It supplies one backward flick at ticks5–16, then releases. Accepted tick roles:

| Tick | Ordered role |
| --- | --- |
| 6 | Entry immediately moves25; retained speed24.5, root request speed0 |
| 12 | Incoming22 still decelerates to21.5; seventh actual root evaluation |
| 13–18 | Minimum22/zero-change; VM hold retains root12.5; release selects14 |
| 19 | First recovery:22×0.9=19.8, observed root12.5 |
| 24 | Recovery phase546 latches to actual root12.5 |
| 29 | Root becomes0; recovery visibly stops at X/Z≈−568.50/592.83 |
| 32 | Post-VM control3 is acknowledged once |
| 37 | Done-only retained-root read, then idle selection; still25 evaluations |
| 38–600 | Shared idle program0, stable position/meter/health |

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-owned-ordered-host-v1
build/source-owned-ordered-host-v1/gf_native \
  --demo-source-ordered-ground --headless --frames 600 --no-audio
```

`native_source_ordered_ground_owner` covers real-stage operation ownership,
neighbor/stale/edited/missing leases, staged rejection, phase/geometry tampering,
VM holds and no-motion completion, root-derived stopping, camera-independent idle
heading, exact receipts, producer retirement and unchanged ledge refusal from a
fresh ground spawn at the verified roof edge. The synthetic
`character_program_dash_recovery_control` checks the scoped acknowledgement.
Host clocks, libm, target-center and collision remain labelled native proxies.
Dash-to-ledge is not included in this mode; the separate permission above is
required.

Contact Sheet34 contains actual offscreen GL frames5/6/19/24/28/29/37/38, without
taking desktop/classifier focus. Final isolated native-only frames24/29 match
the full working-tree captures pixel-for-pixel. Captures remain outside Git:
`local/recomp-evidence/contact-sheets/source-owned-ordered-ground-v1/contact-sheet-34.png`.
The real-stage owner passed2481 checks through600 updates. Isolated validation
passed13 sanitizer binaries, followed by final owner/VM/native rechecks after
sealing VM outcomes and qualifying the original motion bank. Host/all-header
builds and the unchanged64-tick ordinary duel smoke also passed.
The final isolated host rerun passed all146 Python tests, including35 opt-in host
integrations, with the hardened root/VM leases and original-bank guard enabled.

## Earlier ordered ground-dash recovery (GFMM9 numeric candidates only)

Before admitting dash-to-ledge motion, the next recovery pass checked
`80060C48/60C94`, `80060EC4/60F78` and `80061084/61160` against bounded
initialized-DOL disassembly. Two important corrections to the earlier native
prefix are now explicit:

- **Entry moves immediately.** After requesting program27+direction, the helper
  zeros48, copies data6C to50, loads25/−0.5/min22/timer30, initializes travel and
  steers, then calls670DC. Selector13 follows that motion, not an empty entry
  frame. At rank1/rate1 on flat ground this means distance25 but retained44=24.5.
- **Recovery consumes animation-derived speed even with1D9B=0.** That metadata
  disables root-position application, not the shared root evaluator's1DAC.
  For normal G-Red (movement type0/class flags12), phase546=0 damps44 using
  `80437570` (0.9), then clamps and latches546=1 only when strictly below the
  post-VM1DAC. Phase1 copies the new1DAC directly. It must not be inferred from
  rendered displacement, a previous native offer or a missing root sample.

The existing `gred_ground_dash.hpp` now exposes ordered numeric plan/update/
finish candidates, reusing the shared motion and root systems:

| Phase | Order |
| --- | --- |
| Entry12 | Request VM → initialize motion → steer → ground motion → select13 |
| Active13 | Steer using incoming44 → strict minimum clamp → conditional VM advance → control/timer/exit test → ground motion → optional select14 |
| Recovery14 | Advance VM → control3 latch/clear → root-speed update → steer only if44>0 → ground motion → done test/common idle reset |

The active exit frame still moves. At exactly22 the −0.5 change is retained;
only the following value below22 triggers the minimum clamp/zero-change branch.
Recovery's first control-bit2 path still refuses the unqualified4CFF4 forced
continuation. Continued recovery advances normally. The source's done test is
**after** motion; zeroing speed before that final move is incorrect.

GFMM9 is376 bytes, FNV`409f4acc`, SHA-256
`a3b383e875881749f1702ee65f424d10d830a83dbc2e586c3bdd567565cc78fc`.
Its new offset372 separately owns recovery's`80437570` damping. The numerically
equal steep-contact value at`804375D4` grants no recovery authority. GFMM1–8
remain byte-for-byte unchanged and accepted with their previous capabilities.
The read-only, bounded extractor now emits9 and retains overwrite/symlink
refusals. The six other local pack components are unchanged.

**No live controller changes in this milestone.** File bounds in the existing
owner and inspection replay now admit376-byte GFMM9 data; tests compare their
legacy output against older packs. Earlier modes retain their labelled
request-only dash entry and legacy recovery prefix; GFMM9 alone does not opt them
into the ordered controller. Walk-only ledge mode still refuses dash/flick/jump/
flight/combat. The new candidates require an explicit post-VM root state for
recovery, but their numeric `updates` check is not freshness or source ownership.
They neither execute a VM nor issue actor/contact/frame receipts. A live dash
owner must bind the shared root evaluator to the actual operation, rig and frame,
then add distinct native phase/kinematic checks before dash-to-ledge admission.
Unknown movement types, airborne/blocked motion and recovery phases remain
outside this normal-G-Red subset.

`gred_ground_dash_ordered` tests immediate entry motion, post-deceleration
ground-loss momentum, steering/clamp/VM order, control/timer boundaries,
root-speed derivation without root-position application, recovery latch equality,
missing/invalid samples, motion-before-idle, old-entry preservation and strict
GFMM9 truncation/edit refusals. These are numeric/native tests, not original
hardware or full actor-frame equivalence. Bounded source evidence stays local:
`local/recomp-evidence/gred-dash-order-v1/`.
The seven-component GFMM9 pack re-exports byte-for-byte from qualified inputs;
an overwrite attempt refuses, and DOL/setup/stage hashes remain unchanged.
Actual offscreen GL legacy dash frame60 is pixel-identical with the default and
GFMM9 packs. This is an unchanged-controller regression, not a new live dash:
`local/recomp-evidence/contact-sheets/source-ordered-dash-compat-v1/`.
Isolated validation passed11 sanitizer binaries and all145 Python tests
(including34 opt-in host integrations), host/all-header builds and the unchanged
64-tick ordinary duel smoke. The ordered numeric test passed815 checks; the
existing live walk-ledge owner still passed1963. Final owner tests were rebuilt
after updating the bounded GFMM9 file readers.

## Live walk-only ledge handoff (GFMM8)

`--source-walk-ledge` / `--demo-source-walk-ledge` connects the recovered
ground-loss subset through the existing owner, VM, motion, contact and renderer.
Older ground and movement policies retain their ledge refusal. This mode accepts
low-deflection walking only: jump, flick/dash, flight, attacks and landing
cancellation refuse rather than skipping unresolved input/controller work.

The owner creates a **native-owned frame-start snapshot** from the exact
same-world/tick/binding observation. It does not relabel a previous render pose
or the last movement's starting point as source2C/30/34. The unblocked detached
subset declares4A0/732 zero, reads the VM's actual1D9B metadata, and refuses
unowned overrides/attachments. No original actor constructor or whole callback
frame is acknowledged.

Distinct native departure, priority-entry, falling and touchdown kinds carry
exact frame-start, timer698, scalar44, travel5AE, optional drift heading and
selected-surface evidence. Native acceptance independently verifies source
normal-G-Red/rank1 walking tangents, strict downward gap beyond60, retained
departure Y/zero48,0.99 scalar damping, old-velocity air integration, timer order
and delayed4-unit drift. All body/bounds/terrain clipping and endpoint/vertical
budgets remain refusals. Actual landing is limited to a flat selected surface;
overflight does not authorize a nonflat contact or borrow old push policies.

The last ground step still publishes walking program1 and source flags10/cache0.
The **following** priority correction selects6/marker1; scoped numeric VM flag
portions run before program13 entry. That entry integrates old zero48, then
applies50=-1, sets flags40/cache1 and starts698=30. Only after the timer reaches
zero can direct camera-relative drift run. Retained travel and body heading are
separate; camera motion does not force the Borg to turn.

Flag writes reuse the shared VM with exact class-field/scope checks for the
numeric portions of6A5A4,5FECC,60228 and6A474. They grant no actor/contact receipt
on their own. All history, flags, counters, resource and pose-frame leases remain
staged until matching native acceptance; source retirement revokes them.

The native demo starts on authored roof triangle249, X/Z
`(1010.166015625,2315.240722656)`, initial floor reference482.747558594 and
arena-half4500. Target placement is unchanged. Interactive mode retains ordinary
MVP spawn/arena/references. These are native demo assumptions, not original spawn
data, and no airborne position, input calibration or stage asset is fabricated.
It holds low walking input from5 through one accepted post-delay drift, then
releases:

| Tick | Role / program | Native/source state |
| --- | --- | --- |
| 85 | Ground departure /1 | Y held482.747589,48=0; flags10/cache0, no takeoff |
| 86 | Priority falling entry /13 | Same Y then48=-1;698=30, flags40/cache1 |
| 87–115 | Falling /13 | Retained momentum×0.99, direct drift inhibited |
| 116 | Falling /13 | 698=0, one direct drift update while input remains held |
| 117 | Actual flat touchdown /13 | Y=10, flags40/cache1 still captured |
| 118 | Landing entry /8 | Scoped landing flags200/cache0; not early refill |
| 240/600 | Stable idle /0 | Flags0/cache0, meter10000, one departure/entry/landing |

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-walk-ledge-host-v1
build/source-walk-ledge-host-v1/gf_native \
  --demo-source-walk-ledge --headless --frames 600 --no-audio
```

The mode retains explicit native clock, initialization, camera, collision and
libm approximations, not original-hardware/full-game equivalence. Nonflat
landings, small-step/native-proxy disagreements, clipping, unknown/layered
geometry, external momentum/history, forced732, attachments, offsets and
platforms remain refused. Tests: `character_program_ground_loss_flags`,
`native_source_walk_ledge` and `native_source_walk_ledge_owner`.

Contact Sheet33 shows actual offscreen GL frames84/85/86/115/116/117/118/180
without taking desktop/classifier focus. It distinguishes the walking departure,
program13 entry, delayed drift, captured touchdown and later landing/idle flags.
Fall-entry frame86 is pixel-identical between the isolated native-only checkpoint
and the full working tree. Captures remain local and outside Git:
`local/recomp-evidence/contact-sheets/source-walk-ledge-v1/contact-sheet-33.png`.

Isolated validation passed14 sanitizer binaries, all143 Python tests (including
32 opt-in host integrations), host/all-header builds and the ordinary64-tick duel
smoke. The real-stage owner test passed1963 checks through600 updates, including
old-mode refusal, staged history/flags/timers, one post-delay drift, pose/receipt
retirement and stable idle. The original seven-component GFMM8 pack is unchanged.

## Earlier ground/ledge correction recovery (numeric candidate only)

The ground-to-air investigation now has a bounded native `68DF8/6A5A4` numeric
planner, `gred_ground_correction_plan()`, and separately qualified **GFMM8**
operands. That milestone did **not connect the live movement owner**. Existing
source modes and the ordinary MVP retain their policies; the later explicit
walk-only bridge above is the sole new actor permission.

Initialized-DOL disassembly resolves the paired-single decompiler truncations:

- `670DC` performs ground movement and applies its strict step limits. A large
  downward step retains incoming Y while keeping X/Z motion and updated44.
  It does not select falling or run air integration there.
- On the next applicable selection pass, ground handlers call `68DF8` before
  jump/dash/walk admission. It queries `3D6E4` and compares the new height with
  **previous30**, not incoming24. A strict drop below `-674` restores incoming24
  and selects the falling handoff. A strict upward excess rewinds all coordinates
  only if X/Z differ from previous2C/34; otherwise the queried height remains.
- The drop path can increase44 to horizontal displacement length divided by5F8
  and change travel5AE, but only when that value is **strictly greater** than
  current44. It does not force the Borg's body heading.
- `6A5A4` sets590=1,690=0, clears71B, masks unblocked flags and selects6.
  Crucially, `5FECC` therefore requests **program13**, not ordinary program6.
  Source marker590=1 is not uniquely an “after air dash” origin.
- `68DF8` sets698=30. While positive, `5FECC` suppresses direct676D8 drift;
  `677B0` uses the stricter AE angle gate instead of AC. Lower falling entry
  separately sets airborne flags/history, installs the landing callback and
  sets50. The correction planner does not pretend those callbacks executed.

The planner consumes explicit, matching previous2C/30/34 fields and returns an
optional transition. It preserves incoming48/50, and makes no VM/resource,
collision, actor or lifetime acknowledgement. Forced732, stored1D9B momentum,
offset/attached/platform and blocked/effect branches remain refused.

History ownership is a prerequisite for integration. The common5685C writer
copies20/24/28 into2C/30/34 before its caller's dispatch;55DCC/55EE4 snapshot
airborne flags around update. The live lab's declared native previous-Y field
must not simply be relabelled as this source-owned snapshot. Connecting that
boundary, priority over buffered input, program13, scoped VM flag writes,
timer698 decay, drift suppression and AE contact receipts is still pending.

GFMM8 is372 bytes. Offset364 owns `8043762C`'s zero;368 owns `80437648`'s30 for
this ground-loss path. They are not aliases of numerically equal operands owned
by other callbacks. Motion SHA-256:
`81f07734d657e3f1b85e3252e121c32610c9de020cf9edab1ab187e738368fe7`,
FNV `9525c46e`. Other six pack components are unchanged; GFMM1–7 remain readable
with their narrower capabilities. Both shared consumers accept the372-byte data
extent without enabling new actor/controller authority.

This milestone emitted GFMM8; the current exporter emits GFMM9 as described
above. Versioned packs in older milestone sections are
saved immutable inputs, not paths to overwrite. Existing labs can read a new
pack through `--source-ground-assets local/content/source-gred-ground-v9`;
loading it does not enable live ground-loss dispatch.

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v9
python3 -B tools/build_native.py --jobs 1 --sanitize --opt 0 \
  --tests gred_ground_correction,gred_movement_motion
```

Extraction stays read-only, bounds DOL input to8 MiB and refuses existing outputs
and symlinks. Saved original inputs and immutable packs are not overwritten.
Private bounded disassembly is under `local/recomp-evidence/gred-ground-ledge-v1/`;
no original executable, decompilation or game data is distributed with this code.
Host libm/square-root approximations are not original MSL/FPSCR equivalence.

Isolated native-only validation passed14 sanitizer binaries, all142 Python tests
(including31 host integrations), host/all-header builds and the64-tick ordinary
duel smoke. The new correction/history/refusal test passed52 checks, including
native ground-only ledge rejection without tick mutation or legacy fallback.
All seven GFMM8 components re-export byte-for-byte; the other six components
match GFMM7 and all qualified original inputs retain their hashes. GFMM8 supplied
to the existing entry-push actor mode produces the same GL frame81 as the saved
GFMM7 milestone. This is compatibility evidence, not a live ledge demonstration.

## Wind-up entry on a steep selected surface

`--source-windup-entry-push` / `--demo-source-windup-entry-push` adds a separate
entry-push contract to the existing GFMM7 source owner. Older wind-up/floor
policies retain their refusal. It reuses the same planner, VM, selected-surface
math, native collision checks and renderer; no new assets, operands, animation
labels or cross-Borg rules are introduced.

`61338` entry halves44/48, clears4C and declares50=data70 **without** displacement
or acceleration. If `677B0` takes its steep ret0 branch, it projects from that
unchanged point and damps44/48 by0.9. Wind-up's ret1-only zero48 tail does not run.
Thus the entry response is half-speed then0.9, not a repeated integration,
continued-frame acceleration, floor snap, landing or refill.

The distinct entry-push kind verifies same-position pre-contact evidence,
half of previous accepted48, selected height/normal/triangle/angle, exact
quantized projection and0.9 response. Native phase order, movement budgets,
body/bounds/terrain clipping and post-projection floor checks remain unchanged.
Resource, timer, VM and pose leases publish only after the exact receipt.

The original-stage fixture uses authored triangle145. The **native demo only**
declares arena-half4500, player X/Z `(-2760.075195312,-3528.048583984)` and initial
floor query reference385.716827393. Target placement is unchanged; interactive
mode retains ordinary arena/spawn/references. These are not recovered original
spawn or input data, and no airborne position or stage asset is edited.

Its bounded input holds jump5–7, re-presses9–31 without cancelling takeoff,
then flicks toward the target for27 updates after the first accepted falling push:

| Tick | Response / dispatch | Program | Meter |
| --- | --- | --- | --- |
| 80 | Ordinary falling push reaches triangle145 | 6 | 8450 |
| 81 | Entry half48=-13.5, then ret0 response48=-12.15 | 10 | 8450 |
| 82–85 | Continued wind-up pushes integrate/apply50, no entry replay | 10 | 8450 |
| 86 | Fifth continued push; deferred450 cost/select16 before contact | 10 | 8000 |
| 87 | Separate active-entry reset | 11 | 7950 |
| 140 | Recovery's real landing callback | 12 | 6450 |
| 240/600 | Stable idle after ordinary grounded recovery | 0 | 10000 |

The entry in this verified route is exactly on the selected steep plane:
projection distance is zero, but the source ret0 path still damps speed and stays
airborne. It is not relabelled as a floor snap. A bounded read-only geometry
witness search supplied the native placement, then ordinary input and accepted
world/contact steps reproduced the route. The previous mode refuses after
accepted tick80 and revokes its retired leases without staged cost/pose or MVP
fallback.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-windup-entry-push-host-v1
build/source-windup-entry-push-host-v1/gf_native \
  --demo-source-windup-entry-push --headless --frames 600 --no-audio
```

Positive-speed entry pushes, active-dash entry pushes, raised-side/unknown
geometry, clipping, attachments, offsets, platforms, combat/effects and whole
original actor frames remain refused. The mode inherits earlier floor contracts
separately; it does not turn them into pushes. Native initialization, clock,
camera, collision and libm approximations remain explicitly non-equivalent to
original hardware. Tests: `native_source_windup_entry_push` and
`native_source_windup_entry_push_owner`, plus earlier controller suites.

The isolated native-only tree passed14 sanitizer binaries, all138 Python tests
(including29 host integrations), host/all-header builds and the ordinary64-tick
duel smoke. The real-stage owner test passed1309 checks. Its continued pushes
independently reselect pre-contact metadata across authored faces (including83),
not an invented requirement to keep the entry triangle after projection.

Contact Sheet32 shows actual offscreen GL frames79/80/81/82/86/87/140/180,
without taking desktop/classifier focus. Frame81 was pixel-identical between
the isolated checkpoint and full working tree. Captures remain local:
`local/recomp-evidence/contact-sheets/source-windup-entry-push-v1/contact-sheet-32.png`.
Bounded wind-up/contact handler bytes, independent0.5/0.9/zero operands and all
seven GFMM7 pack components still match the qualified read-only inputs.

## Earlier positive-speed wind-up entry at floor height

`--source-windup-positive-floor` / `--demo-source-windup-positive-floor`
separately admits positive48 during the **no-motion** wind-up entry. Older
floor policies retain their refusal, including `WindupEntryFloor`'s
nonascending-only kind. The new mode inherits that older kind separately for
nonpositive entries; it does not relabel them as positive.

The initialized-DOL `61338` entry branches around `67310`: it halves44/48,
sets50, requests10 and reaches `677B0` without displacement or acceleration.
Return1 unconditionally zeros48, including a positive half-speed. This does not
authorize upward integration, rising relaunch contact, raised-side entry or a
general hover/perpetual-flight rule for another Borg.

The new positive entry receipt requires previous accepted48>0, exact half-speed
evidence, retained data70 and zero post-contact48. Both pre-contact and final
positions must equal the accepted starting position, with independently matched
floor metadata. Even a tiny upward height adjustment refuses. Sign, kind,
producer, phase, clipping and receipt checks remain separate from the older
nonascending entry.

The native demo keeps ordinary spawn/arena/target/query references, holds
jump5–30, re-presses at78 until accepted dash entry and flicks right86–112:

| Tick | Result | Program | Meter |
| --- | --- | --- | --- |
| 79–85 | Seven negative-speed relaunch snaps | 14→15 | 8400→7800 |
| 86 | Eighth snap leaves48≈+0.392899, still airborne | 15 | 7700 |
| 87 | Positive entry: half48≈+0.196449, then zero; no motion | 10 | 7700 |
| 88–93 | Continued wind-up, no entry replay/refill | 10 | 7700 |
| 94 | Deferred450 cost/select16 before contact | 10 | 7250 |
| 95 | Separate active entry | 11 | 7200 |
| 125 | Later minimum timer installs landing callback | 11 | 5700 |
| 240/600 | Stable idle after ordinary grounded recovery | 0 | 10000 |

The previous mode can prepare the numeric candidate after tick86, but its
native policy refuses the positive speed. That rejection leaves private staged
state unpublished and the current frame live until explicit producer retirement
revokes it. Neither mode silently falls back to MVP movement.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-windup-positive-floor-host-v1
build/source-windup-positive-floor-host-v1/gf_native \
  --demo-source-windup-positive-floor --headless --frames 600 --no-audio
```

No new operands, asset pack, animation classification or per-Borg mapping is
introduced. This still uses GFMM7, shared VM/model/render/contact systems and
explicit native clock, initialization, collision, camera and libm assumptions.
Entry pushes, actual rising motion/contact, raised/unknown geometry, offsets,
attachments, moving platforms, combat/effects and whole original actor frames
remain unsupported. Regression targets: `native_source_windup_positive_floor`
and `native_source_windup_positive_floor_owner`, plus earlier floor suites.

Contact Sheet31 contains actual offscreen GL frames85/86/87/88/94/95/125/150,
without taking desktop/classifier focus. Positive entry frame87 was
pixel-identical between the isolated native-only checkpoint and the full working
tree. Captures remain local and excluded from Git:
`local/recomp-evidence/contact-sheets/source-windup-positive-floor-v1/contact-sheet-31.png`.

The isolated native-only tree passed14 sanitizer binaries, all137 Python tests
(including28 host integrations), host/all-header builds and the ordinary64-tick
duel smoke. The positive-entry owner regression passed1881 checks, including the
older policy's rejection and inherited nonascending route. The bounded440-byte
wind-up handler still matches the qualified DOL; all seven GFMM7 components and
the half-speed/zero operands remain unchanged.

## Earlier nonascending wind-up entry at floor height

`--source-windup-entry-floor` / `--demo-source-windup-entry-floor` separately
extends the GFMM7 air-dash-floor owner. Earlier floor modes retain their entry
refusal. The same original programs, source planner, shared VM/renderer and native
contact systems are reused; no new operands, asset pack or Borg mappings are
introduced.

Qualified `61338` entry clears754, halves44/48, clears4C, sets50 to data70 and
requests10 **without** translating or applying acceleration. Its `677B0` return1
tail then zeros48 only. A directional dash during an airborne relaunch floor
snap can therefore enter wind-up at floor height without landing or refill.
This is distinct from a continued wind-up update, which does integrate/apply50.

The new entry-floor kind validates unchanged pre-contact position, half of the
previous accepted vertical speed, retained data70, zero post-response48 and an
independently selected eligible surface. Native phase/order/clipping and exact
receipt checks remain in force. Private resource/timer/VM state and pose leases
publish after acceptance; fatal source refusals revoke their old leases.

The bounded native demo keeps ordinary spawn, target, arena and ground-query
references. It holds jump5–30, re-presses at78 until accepted wind-up entry, and
supplies a right flick80–106:

| Native tick | Result | Program | Meter |
| --- | --- | --- | --- |
| 79/80 | Two airborne relaunch snaps | 14 | 8400/8300 |
| 81 | Wind-up entry: half48=-3.904 before contact; zero48 after | 10 | 8300 |
| 82–87 | Continued wind-up snaps; no entry replay or refill | 10 | 8300 |
| 88 | Timer spends450/selects16 before contact | 10 | 7850 |
| 89 | Separate active entry and null-callback snap | 11 | 7800 |
| 119 | Minimum active timer installs landing callback | 11 | 6300 |
| 240/600 | Stable idle after ordinary grounded recovery | 0 | 10000 |

The entry snapshot does not add -.07 or translate by the half-speed. The cost at
timer completion is not charged again on entry. The real-stage owner regression
also verifies that `AirDashFloor` refuses the same input after accepted tick80
with no staged cost/pose publication or legacy fallback.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-windup-entry-floor-host-v1
build/source-windup-entry-floor-host-v1/gf_native \
  --demo-source-windup-entry-floor --headless --frames 600 --no-audio
```

In that narrower mode, positive-speed entry, entry pushes, raised-side/unknown
geometry, attachments,
nonzero offsets, moving platforms, combat/effects and original whole actor
frames remain unsupported. Native clock, initialization, collision, camera and
libm approximations are still explicitly not original-hardware equivalence.
Regression targets: `native_source_windup_entry_floor` and
`native_source_windup_entry_floor_owner`, plus the earlier floor suites.

The isolated native-only checkpoint passed14 sanitizer binaries, all136 Python
tests (including27 host integrations), host/all-header builds and the ordinary
64-tick duel smoke. The real-stage owner test passed1304 checks. All seven GFMM7
components and the three qualified original inputs retained their hashes.

Contact Sheet30 contains actual offscreen GL captures79/80/81/82/88/89/119/150,
without taking desktop/classifier focus. Entry frame81 was pixel-identical
between the isolated checkpoint and the full working tree. Captures remain local:
`local/recomp-evidence/contact-sheets/source-windup-entry-floor-v1/contact-sheet-30.png`.

## Null-callback relaunch floor response (GFMM7)

The opt-in `--source-relaunch-floor` / `--demo-source-relaunch-floor` route
extends the existing source movement owner, shared animation VM, native contact
acceptance and renderer. Ordinary MVP gameplay and older source policies remain
separate. Earlier sections below describe milestones, not broader permissions.

Selecting8 through `6A750` clears callback754 and lower544–547/588, but does not
reset upper540. Neither `6235C` nor G-Red's pre-dispatch callbacks reinstall
`65E64`. Therefore eligible `677B0` floor contact snaps height **without**
zeroing48/50 or selecting landing7. `6235C` then multiplies negative48 by
`8043751C` (0.8) at the declared delta1DC8=1. The additional `80437584` term
vanishes at that delta. Acceleration50, selector8, cached airborne history and
independent lower/upper latches survive. No grounded refill occurs.

This response has distinct entry/wind-up/active motion kinds and exact receipt
evidence: integrated position, pre-response vertical speed, acceleration,
response factor and selected height/normal/triangle/angle. Native acceptance
independently verifies geometry and integration, refuses clipping and height-only
providers, and remains ungrounded at exact floor height. Resource/VM state and
revocable pose-frame leases publish only after the matching native receipt.
Normal `65E64` touchdown and steep 0.9 pushes keep their separate contracts.

GFMM7 is364 bytes; offset360 stores the separately owned `8043751C` operand,
not an alias of numerically equal jump acceleration. Qualified motion SHA-256:
`ecef86de989a21835973e5eaad22a1920e1fec68264c6cfe6990d3ca3c7d9320`,
FNV `77c3d18e`. Other six pack components are unchanged. GFMM1–6 remain readable,
but GFMM6 cannot admit the new floor mode.

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v7
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-relaunch-floor-host-v1
build/source-relaunch-floor-host-v1/gf_native --demo-source-relaunch-floor \
  --headless --frames 600 --no-audio
```

Extraction reads qualified inputs only and refuses an existing output directory.
The native demo uses ordinary MVP spawn/bounds/target/references, holds B at5–30,
represses at78 until two committed floor contacts, then releases. Observed route:

| Native tick | Result | Selector / program | Meter |
| --- | --- | --- | --- |
| 79 | Entry snap;48=-10.56, airborne | 8 /14 | 8400 |
| 80 | Held snap;48=-7.808, airborne | 8 /14 | 8300 |
| 81 | Release → falling21 → actual touchdown | 7 /21 | 8300 |
| 240/600 | Completed landing recovery, idle | 0 /0 | 10000 |

Validation includes sanitizer tests `native_source_relaunch_floor`,
`native_source_relaunch_floor_owner`, `gred_movement_motion` and26 prior
movement/contact/native regressions, plus host/all-header builds. The owner test
checks real-stage snaps79/80, separate touchdown81, staged costs/latches, older
mode refusal and pose/receipt retirement. Source-world integration checks those
ticks and the600-frame one-shot route.

This is still a bounded native integration: ascending/raised-side entry, nonzero
offsets, attachments, moving platforms, unresolved geometry, weapons/effects and
whole original actor frames remain unsupported. Native clock, initialization,
camera, collision and libm approximations are not original-hardware equivalence.

Contact Sheet28 contains actual offscreen GL frames78/79/80/81/90/140, without
taking desktop/classifier focus. It distinguishes both airborne snaps from
touchdown and the later grounded refill. Captures and qualified evidence remain
local, outside Git:
`local/recomp-evidence/contact-sheets/source-relaunch-floor-v1/contact-sheet-28.png`.

The publication checkpoint was also built from an isolated native-only tree,
without the dirty Story/preparation backlog. Its real-stage floor-owner test
passed638 checks and its offscreen GL tick79 was pixel-identical to the full
working tree. This checks the selected dependency boundary, not original-game
equivalence.

### Held relaunch after floor contact

A separate real-stage owner regression keeps the re-pressed jump button held
through600 native ticks instead of releasing after two snaps. It observes eight
snaps at79–86. The normal independent animation lanes hand off14→15 at84 while
the Borg remains airborne at floor height and continues spending resource.

The seventh negative-speed response leaves48≈-0.407101. At86, acceleration makes
48≈+0.392899: that eighth snap uses factor1, not0.8. Ordinary active relaunch
motion lifts off at87 with meter7600; no new entry or resource refill is invented.
Exhaustion later enters falling21, followed by actual touchdown at242 with
meter0. Holding the old button does not fabricate another jump edge after
grounded recovery; by600 the owner is idle with one relaunch, eight snaps, one
landing and meter10000.

This extends the regression of the same bounded source/native integration, not
its supported contact geometry or a movement rule for other Borgs. Run
`native_source_relaunch_floor_owner` to exercise both the two-snap release route
and this held/depletion route.

### Air-dash null-callback floor integration

The opt-in `--source-air-dash-floor` / `--demo-source-air-dash-floor` mode now
connects the recovered caller tails below through the existing source owner,
shared VM, native receipt validation and renderer. It inherits the GFMM7
relaunch-floor mode without widening any older policy. No new asset pack or
animation mapping is required.

Distinct continued-wind-up, active-entry and continued-active floor kinds carry
their own exact pre-contact position/speed/acceleration and selected-surface
evidence. Native acceptance independently checks geometry, caller response,
phase ordering, motion bounds and clipping. All remain airborne at floor height.
Resource, timer, VM and pose-frame leases publish only after the matching receipt.
Fatal source refusals retire the producer and revoke its prior pose/receipt
leases; a rejected native candidate alone leaves the staged producer live.

The native demo uses ordinary MVP spawn/bounds/target/references, holds jump5–30,
and supplies a right flick78–104:

| Tick | Contact/dispatch | Program | Meter |
| --- | --- | --- | --- |
| 80–85 | Wind-up snaps;48=0,50=-0.07, airborne | 10 | 8550 |
| 86 | Seventh wind-up snap, cost/select16 before contact | 10 | 8100 |
| 87 | Separate active entry, snap retains48=-0.07 | 11 | 8050 |
| 88–116 | Early active snaps retain accumulating negative48 | 11 | 8000→6600 |
| 117 | Minimum timer installs65E64; actual touchdown | 11 | 6550 |
| 240/600 | Ordinary landing recovery then stable idle | 0 | 10000 |

Wind-up entry floor snaps and entry pushes, ascending/raised-side contacts,
attachments, offsets, moving platforms and unresolved geometry still refuse.
A leftward version hits an unresolved raised-surface seam after accepted tick104
and refuses atomically; the rightward demo does not bypass that refusal or alter
spawn/arena/query references. This is bounded normal-G-Red native integration,
not original actor-frame/hardware equivalence or a rule for other Borgs.

```sh
python3 -B tools/build_native.py --host --headers --jobs 1 \
  --output build/source-air-dash-floor-host-v1
build/source-air-dash-floor-host-v1/gf_native --demo-source-air-dash-floor \
  --headless --frames 600 --no-audio
```

Regression targets: `gred_air_dash_floor`, `native_source_air_dash_floor` and
`native_source_air_dash_floor_owner`, plus existing relaunch/wind-up/active
acceptance and source-world replay tests.

Contact Sheet29 shows actual offscreen GL entry, both caller responses,
timer/callback handoffs, landing and recovery without taking desktop/classifier
focus:
`local/recomp-evidence/contact-sheets/source-air-dash-floor-v1/contact-sheet-29.png`.
The captures and qualified source evidence remain local and excluded from Git.

Validation from the isolated native-only staged tree passed14 sanitizer binaries,
all135 Python tests (including26 opt-in host integrations), host/all-header
builds, and the ordinary64-tick duel smoke. The real-stage air-dash-floor owner
test passed1579 checks, including fatal-refusal lease revocation. Its GL tick87
was pixel-identical to the full working-tree capture.

### Earlier numeric air-dash floor caller recovery

Air-dash contact must not inherit relaunch's0.8 floor response. Qualified
initialized-DOL disassembly distinguishes:

- Wind-up `61338` clears callback754 at entry, halves44/48 without translating,
  and sets50 to data70. Continued frames integrate with `67310`, decrement568,
  and may spend/select16 **before** contact. After `677B0` returns1, the caller
  unconditionally zeros48, but does not reset50 or select landing7.
- Active `61560` initially clears754 and later installs `65E64` when its minimum
  active timer expires. Its `67310`/`677B0` tail has **no** additional response:
  null-callback contact preserves48/50. A real installed callback remains normal
  landing; depletion's selection17 can clear754 before the current contact.
- Relaunch `6235C` instead multiplies only negative48 by0.8 after return1.
  These three caller responses are not a shared Borg floor rule.

`gred_air_dash_static_contact()` reuses the paired static query and explicit
`Snapped` result to model the first two caller tails. Above/away and steep push
returns do not trigger wind-up's vertical zero. It requires the reviewed GFMM7
null-callback candidate domain but adds no new pack operands. Attachments,
offsets, recovery-specific callers and invalid wind-up callback claims refuse.

That earlier checkpoint was **numeric candidate recovery only**: older native
owner modes and producer policies still refuse early air-dash floor contact.
A bounded original-stage probe in `RelaunchFloor` mode with ordinary spawn and a
left flick at78 refuses after accepted tick79, while preparing its first continued
wind-up contact. No receipt, cost, pose or
legacy-MVP fallback is committed for that refused tick.

The bounded handlers were cross-checked byte-for-byte against the qualified
DOL; private evidence is under
`local/recomp-evidence/gred-air-dash-floor-v1/`. Numeric regression:
`gred_air_dash_floor`. No new manual animation classification is required.

## Native movement-input/resource subset

The next native pieces are now implemented independently of the playable MVP:

- `engine/gameplay/character_movement_input.hpp` conditions source-domain stick
  and jump inputs into the recovered directional and button buffers.
- `engine/gameplay/gred_movement_admission.hpp` evaluates the reviewed normal
  G-Red movement predicates, preserving jump-versus-dash and airborne-relaunch
  priority. It does not infer animation completion or spend resource by itself.
- `engine/gameplay/character_air_resource.hpp` contains the shared recovery
  scalar kernel and the recovered consumption operation. Existing startup
  bookkeeping now reuses that same recovery kernel rather than maintaining a
  parallel refill implementation.

The input profile is a strict, model/revision-bound GFMI resource. Its octant
lookup was traced through the original small-data base: the direction helper
loads **Y into the first heading argument and X into the second**. The native
implementation reuses the shared bounded heading helper. This is not a claim of
original hardware/libm rounding equivalence.

Verified input behavior includes:

- Rapid deflection through source radial thresholds20/52 produces a dash nibble;
  a slow ramp can instead admit walking.
- Accepted dash admission consumes that nibble; holding the stick does not
  repeatedly request the burst. Neutral release rearms the directional buffer.
- Jump edges have the authored bounded pulse; simply becoming airborne while
  holding the button does not invent another airborne jump edge.
- Jump wins over a simultaneous ground dash. An admitted airborne relaunch wins
  over a simultaneous air dash. Source lock bits gate those actions separately.
- Flight release/resource exhaustion is considered before another air dash.
- Active-dash relaunch uses a different readiness predicate from falling-state
  relaunch; the source's distinctions are not collapsed into one jump flag.

For G-Red, recovery consumes the **previous accepted post-update's cached
airborne value**, not a newly sampled floor test. Consumption preserves the
source fused subtraction, integer conversion and signed16-store/clamp ordering.
This kernel is not Sasuke's count-based jump-gauge admission policy.

The real-model regression now connects a source forward-flick admission to an
original ground-dash program, verifies held input does not re-request it, and
then interrupts it with an admitted jump into the original takeoff program.
The body/camera forward classification is explicitly declared in that fixture;
general heading/contact/physics production is not claimed.

### Ownership and refusal boundaries

This is a **movement-only prefix and predicate subset**, not a whole actor frame:

- Nonmovement/weapon inputs refuse instead of bypassing weapon/charge producers.
  The `686F0` effects and `6DE44`/field749 producer remain outside this subset.
- Movement admission requires an explicit declaration that prior source
  terrain/weapon/auxiliary handlers declined. That declaration is not a lifetime
  or actor-admission token; a live owner must actually produce the result.
- Airborne paths can return `contact_tail_required`. Their missing contact work
  is not silently acknowledged as completed.
- The profile/admission entry is G-Red-specific. Sasuke and other candidate
  movement families do not inherit its flight meter, refill gate or admission
  rules.

The playable duel and source-program renderer replay are unchanged. Production
integration still needs accepted world/contact/root observations, native-device
mapping and the movement-state/physics owner.

The new profile is prepared read-only from the qualified DOL and the existing
qualified normal-G-Red setup profile, with a **new** output path:

```sh
python3 -B -m tools.extract.gred_movement_input MAIN.dol GRED.gfcs \
  local/recomp-evidence/new-source-inputs/movement.gfmi
```

Current local profile:
`local/recomp-evidence/gred-input-resource-v1/movement.gfmi`.

Validation: four sanitizer suites passed (`gred_movement_input`,
`gred_source_locomotion`, `startup_actor_bookkeeping`, `native_gred_locomotion`).
The input/resource suite passed66619 checks, including exhaustive signed16
recovery and edited/truncated-profile refusals. The real-model source-VM suite
passed2696 checks with19 explicit requests,916 VM updates and the input-to-program
link enabled. Existing bookkeeping regression passed65766 owned-fixture checks.
Sixteen Python recovery tests passed; source DOL/setup hashes and output-overwrite
refusal were checked. These are bounded native tests, not original hardware or
whole-actor equivalence.

## Source heading and motion proposals

`character_movement_heading.hpp` now evaluates the original held/cardinal/octant
direction domains, signed16 sector boundaries and turn progression. The native
camera adapter observes camera-forward and body-facing separately; it never
assigns body heading from camera orientation. The input-to-program regression
can now derive the ground-dash program from recovered tables instead of supplying
a hardcoded forward-direction class.

The source motion operands are packaged into a separate strict **GFMM** profile.
It retains constructor rank16, walk/turn values, air/dash values, direction
tables, steering operands and held-direction masks. No values have been copied
into the approximate MVP controller.

`character_movement_motion.hpp` produces source-backed numeric candidates:

- Ground motion uses the existing shared static-ground normal and height queries,
  tangent projection, old-speed position integration, speed update and step
  restoration.
- Air motion preserves position-before-acceleration ordering, normal/special
  velocity clamps and the explicit static-ceiling-policy boundary. Missing
  upward ceiling observations and attached/moving-platform contexts refuse.
- The original floor, secondary and tertiary STIH grids are prepared separately.
  They are not replaced by copies of the floor grid or an invented flat normal.

The tests include a60-update source-motion proposal sequence over all three
original stage473 query grids. Query parameters and history are explicit
declared inputs, not proof of an accepted original actor frame. Final body/prop
collision, contact acceptance and the live state owner still need to join these
candidates before they can drive the playable actor.

### Ground-dash handler planning versus raw bytecode replay

The source active-dash handler can **hold** VM playback after the initial control
boundary while its movement timer continues. Recovery resumes playback. Advancing
the bytecode unconditionally, as the earlier timeline-only replay did, is not a
complete ground-dash controller.

`gred_ground_dash.hpp` now exposes bounded handler planning around the VM callback:
entry requests the original program, active control can suppress VM advancement,
the post-callback control byte governs timer/held-direction exit tests, and
recovery can resume the VM. Lower and upper callbacks are tested in source order.
The separate steering kernel requires its target-distance observation when used.
Unreviewed forced-continuation recovery entries refuse; callback-owned control
clears, common idle reset and full actor lifecycle are not acknowledged by this
numeric planner.

The existing replay view can opt into **held-input dash handler planning**:

```sh
build/gred-motion-handler-host-v1/gf_native --animation-mosaic \
  --animation-programs local/recomp-evidence/gred-movement-admission-v1/locomotion.gfmp \
  --animation-height-profile local/recomp-evidence/gred-movement-admission-v1/pose-height.gfhc \
  --animation-motion-profile local/recomp-evidence/gred-motion-integration-v1/motion-v2.gfmm \
  --animation-grid 2 2 --size 1152 800 --no-audio
```

Only the four ground-dash cells use handler planning; the remaining selected
programs are still timeline inspections. This is a fixed held-input fixture,
not device input or live world simulation. Completed poses are held until the
inspection window restarts, not presented as original idle-state execution.

Read-only preparation, always into **new** output paths:

```sh
python3 -B -m tools.extract.gred_motion profile MAIN.dol GRED.gfcs \
  local/recomp-evidence/new-motion-inputs/motion.gfmm
python3 -B -m tools.extract.gred_motion stage-grids STAGE473.gfnb \
  local/recomp-evidence/new-motion-inputs/stage
```

The stage export is restricted to the qualified existing stage473 bundle. These
tools do not modify annotations, original inputs or the playable duel.

Validation for this pass:

- Source heading/turn/motion/planning sanitizer checks passed66112 checks,
  including signed16 angle domains and all three original static grids.
- The source-VM regression passed2866 checks,21 explicit requests and966 VM
  updates, including source-order lower/upper callbacks and active animation
  holds followed by recovery.
- Input, native movement, native terrain and shared stage-ground sanitizer
  regressions passed; host/header/catalog builds passed.
- Twenty Python recovery tests and five opt-in replay-host integration tests
  passed, including edited-motion-profile refusal.
- Original input hashes and output-overwrite refusals were checked.

Contact Sheet10 compares unconditional bytecode playback and handler-qualified
playback at the **same** inspection tick22, then shows handler recovery at tick50:

```
local/recomp-evidence/contact-sheets/source-ground-handler-v1/contact-sheet-10.png
```

The pictured view is in place. The real-grid motion tests produce numeric
candidates separately; neither result acknowledges final body/prop contacts,
target-distance production, whole actor callbacks or live gameplay integration.

## Native-world ground controller lab

The opt-in ground lab now supports normal G-Red source **walk entry, turning,
steady steering, stopping and ground dash**, with native input:

```sh
build/source-ground-walk-host-v2/gf_native --source-ground --no-audio
```

- Arrows/D-pad:32-unit deflection for walking, below the source flick threshold.
- WASD/stick: full/analogue deflection, interpreted by the recovered source
  input kernel; an initial strong flick requests dash, not an invented dash key.
- Enter/Start pauses; Q/Y restarts; Escape closes.
- Jump, flight, combat, shoulder and trigger inputs refuse **before** preparing
  a candidate, without advancing simulation or retiring the accepted pose.

This is a separate movement lab, **not** a replacement for the ordinary duel.
The controller-to-source adapter is native convenience policy, not recovered
GameCube calibration. Source rate664 and delta1DC8 are explicitly fixed to1.
The original native collider, static-query, spawn and target-center
approximations below still apply. Ledges, floor disagreements and unreviewed
dash-recovery continuations still refuse; this is not a full actor controller.

`--demo-source-ground` runs a bounded script covering walking, steering, stops,
re-entry and dash interruption, using the same adapter/owner/world/renderer.
`--demo-source-ground-dash` retains the old dash-only mode and version1 pack.
General ground mode requires a newly prepared `source-gred-ground-v2` pack:
GFMM2 keeps the320-byte extent and adds the original walk-angle divisor32768
and stop damping operands at308/312/316. The other six components are unchanged.
The reader still accepts the qualified GFMM1 profile for dash-only consumers.

Walking requests original program1, with turn duration interpolated from the
source11/19 counts. The lower lane is processed before the upper; the upper
entry duration reads the lower **blend duration**, not its current clip frame.
Turn completion selects steady walk2, which recomputes held direction, updates
body heading through the recovered steering helper and travels along that
heading. Changing the camera without movement does not rotate the body.

Ordinary stop3 requests program31 (the idle alias), zeroes speed and waits for
the lower driver's `blending()` flag to clear, corresponding to motion-buffer
`+2F`; it does not wait for the looping idle clip to finish. This is scoped to
normal G-Red's reviewed ordinary descriptors: both force unmirrored playback.
The source's stop pre-flip is metadata-only before that replacement; the shared
driver already freezes the previously converted quaternion/translation pose,
matching the reviewed B94C-before-D244 ordering. Directional/shared-buffer
variants and their separate completion consumers remain unsupported.

Validation for this extension: the general-ground script completed720 accepted
updates with health100/100 and no hits/projectiles. The real-stage/model
transaction suite passed1699 checks, including slow walk entry, steady steering,
blend-based stop, short interruption/re-entry, dash interruption, idle camera/
body independence and non-destructive native-button refusal. Seven sanitizer
regressions, host/header builds,23 Python recovery tests and10 opt-in world/replay
tests passed. The ordinary MVP smoke still ends at64/0. Original input and both
pack hashes remain intact; existing output directories still refuse.

Contact Sheet12 shows six actual offscreen GL captures, without taking desktop
focus or touching the paused classifier:

```
local/recomp-evidence/contact-sheets/source-ground-walk-v1/contact-sheet-12.png
```

### Descending relaunch-entry surface contact

The newest mode admits reviewed **entry** projection only when normal G-Red
remains descending after adding data4C:

```sh
build/source-relaunch-entry-surface-host-v2/gf_native --source-relaunch-entry-surface --no-audio
build/source-relaunch-entry-surface-host-v2/gf_native --demo-source-relaunch-entry-surface --headless --frames 240 --no-audio
```

It inherits the earlier continued relaunch/dash/falling features and reuses
unchanged GFMM6/GFMI/GFMP operands, the source owner/VM, shared motion/projection,
native world receipts and renderer. Older modes still refuse entry projection.

`6235C` entry adds15 to old vertical velocity **before displacement**, then
applies the neutral/directional jump acceleration and source clamp, followed
by `677B0` projection/damping. It requests14 and spends entry cost150 once.
The pushed return0 does not become landing, repeat the additive step, skip the
separate upper request14 or grant grounded refill.

Native `RelaunchEntryPush` has a separate capability/kind. It requires legal
pre-relaunch phase history, exactly the qualified additive15, exact integrated
pre-push position using that additive speed, reviewed positive acceleration/
clamp and0.9 damping, and the same fresh selected-plane, geometry/budget/
body-clipping and exact-receipt checks. The post-additive displacement must
remain nonascending. The old continued kind cannot borrow the delta; an entry
receipt cannot be relabelled as continuation or replayed as another additive
entry. Following motion must use the appropriate continued or falling contract.

The bounded original-stage demo retains ordinary arena1800, target/floor
references and only changes native G-Red placement to the known triangle1320
interior. Jump is released at31 and repressed at78. After the first accepted
entry push, release plus one soft left input uses the existing camera-relative
falling drift to reach the lower floor. This is explicit native demo input,
not a motion/position override or recovered original spawn/AI script.

Rising entry pushes, wind-up/active-dash entry pushes, landing during relaunch,
offset/attached/platform contact, grounded recovery, unresolved seams, weapons
and combat still refuse. These are normal-G-Red source subsets, not rules
generalized to other Borgs or claims of original actor/frame/hardware equivalence.

The original-stage route accepts one entry push at79, requesting14 on both
lanes with lower/upper latches1/1 and charging150 once (meter8550→8400).
The additive15 is included in displacement before acceleration/clamp and0.9
damping. The next accepted soft drift/falling21 update gives actual lower-floor
touchdown at80; that is not landing inside relaunch. Meter8400 remains unchanged
through touchdown and landing entry, then normal recovery returns10000 and idle.
The rendered route is neutral; directional acceleration and clamp boundaries
also have explicitly numeric fixtures, not additional rendered-route claims.

Validation:26 sanitizer test binaries pass, including615 source-owner and130
native additive-entry/projection/clamp/receipt checks. Host/header builds,
23 Python recovery tests and24 source world/replay integration tests pass.
The one-shot route is stable through600 updates, and ordinary MVP smoke
remains64/0. Original DOL/setup/stage and all six immutable packs retain their
recorded hashes; no extraction, asset edits or dependency changes were needed.

Contact Sheet27 shows actual offscreen GL falling approach, additive entry push,
soft drift/falling21 touchdown, separate landing entry, normal recovery and idle.
No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-relaunch-entry-surface-v1/contact-sheet-27.png
```

### Earlier continued relaunch surface contact milestone

The newest mode adds detached, descending **continued selector8** steep
projection during programs14/15. The additive entry remains separate:

```sh
build/source-relaunch-surface-host-v1/gf_native --source-relaunch-surface --no-audio
build/source-relaunch-surface-host-v1/gf_native --demo-source-relaunch-surface --headless --frames 240 --no-audio
```

The existing GFMM6/GFMI/GFMP packs and shared source owner, VM, motion/projection,
native world/receipt and renderer systems are reused. Older active-surface and
narrower modes retain their relaunch-contact refusals.

Qualified `6235C` disassembly distinguishes entry's additive data4C step and
entry spending from continuation spending and its lower14→15 latch. The common
tail steers with the existing jump gain, derives neutral or directional jump
acceleration, applies neutral scalar damping when appropriate, calls clamped
`67458`, and then `677B0`. It is **not** active dash's-0.07 or recovery's-1.
The separate `62284` upper latch still advances14 on the completion-observing
update, requesting15 later; contact cannot synchronize those latches early.

New `RelaunchPush` capability and distinct wind-up/active relaunch push kinds
preserve those motion-phase boundaries. Their evidence requires a relaunch-only
acceleration operand: exactly the qualified neutral0.8 or its shared signed16
pitch/cosine directional value. Native acceptance recomputes the clamped
pre-push vertical speed, exact accepted old-velocity displacement and0.9-damped
speed, independently queries the selected static plane and retains all earlier
geometry/budget/clipping checks. No additive entry delta is permitted on these
continued kinds. Every other push kind refuses the relaunch-only operand, and
exact receipt matching includes it.

A pushed return0 keeps positive vertical acceleration50, clears4C, damps44/48
and remains airborne. Continuation cost100 is spent once before motion/contact;
entry cost150 and additive15 are not replayed. If spending empties the meter,
that captured relaunch update still finishes; the next admitted input pass can
select falling6 with its own acceleration reset and history-specific program.
There is no grounded refill, recovery17 substitution or landing callback in
the push tail.

Only the reviewed normal-G-Red descending/static/zero-offset path is enabled.
Entry-frame projection, rising/uphill push entries, landing during relaunch,
offset/attached/platform contact, grounded recovery, unresolved seams, weapons
and combat still refuse. This does not establish an original actor constructor,
whole-frame/game or hardware equivalent.

The one-shot demo keeps ordinary native arena-half1800, target `(240,-300)`
and floor-query references0. Its only placement change is a declared native
G-Red XZ `(-606.4511719,-1373.5872803)` on unchanged original triangle1320.
It jumps at5, releases at33, represses at81 and releases again after the first
accepted continued relaunch push. That script is a native demonstration, not
recovered original input/spawn data.

The rendered route covers neutral program14 contact. Directional acceleration,
program15 projection and lower/upper handoff also have source/numeric/native
phase fixtures; they are not claimed as additional original-stage rendered
routes. Private evidence remains:

```
local/recomp-evidence/gred-air-dash-v1/fn_8006235C.asm
local/recomp-evidence/gred-air-dash-v1/fn_80062284.asm
```

Validation: the240-update route accepts66 airborne updates, one additive entry
at82, one continued14 push at83, and separate falling21/touchdown at84. Entry
pays150, leaving8200; continuation pays100, leaving8100. The push keeps both
lanes on14 and lower/upper latches1/1, with positive0.8 acceleration retained.
Touchdown and the next landing entry do not instantly refill; normal recovery
then restores10000 and idle, stable through600 one-shot updates. The older
active-surface mode refuses that same contact before tick83 commits.

Twenty-four sanitizer test binaries pass, including814 source-owner and145
native relaunch-phase/acceleration/receipt checks. Host/header builds,23 Python
recovery tests and23 source world/replay integration tests pass. Ordinary MVP
smoke remains64/0. Original DOL/setup/stage and all six immutable packs retain
recorded hashes; no extraction or asset edits were needed.

Contact Sheet26 contains actual offscreen GL falling approach, separate additive
entry, continued program14 push, falling21/touchdown, landing entry and idle.
No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-relaunch-surface-v1/contact-sheet-26.png
```

### Earlier continued active air-dash surface contact milestone

The newest mode extends **continued selector16/program11** static steep contact,
not the active-entry launch reset:

```sh
build/source-active-surface-host-v2/gf_native --source-active-surface --no-audio
build/source-active-surface-host-v2/gf_native --demo-source-active-surface --headless --frames 360 --no-audio
```

It inherits wind-up/recovery/falling contact features and reuses unchanged
GFMM6/GFMI/GFMP operands, the same private source owner/VM, shared motion and
projection kernels, native receipts and renderer. Older modes keep their
continued-active push refusal.

Qualified `61560` disassembly shows the ordering: an already-entered active
callback decrements its readiness timer; expiration enables588 and the landing
callback and clears4C, but policy1 does **not** force recovery. It advances11,
spends cost3, then (if depleted) selects future17 while preserving588. Steering,
unwrapped `67310` motion and unconditional `677B0` contact still execute as the
captured active callback on that update. The upper lane also consumes11.

A steep return0 can therefore occur before the landing callback is enabled,
after readiness, or on the depletion update without becoming landing. It
clears4C, damps44/48 by0.9 and retains data70's-0.07 acceleration. The next
continued active callback keeps that damped speed and cleared acceleration:
it does not replay entry's speed30/vertical0/negative-drag reset. At depletion,
the later recovery entry separately requests12 and resets vertical
acceleration to-1. Clip completion does not end active flight, and projection
does not refill the meter or create a per-flight dash count.

Native `ActivePush` capability and `AirDashActivePush` kind require accepted
active-entry/active history, exact accepted old-velocity integration,
active-acceleration/clamp and0.9 damping, plus every existing selected-plane,
projection, budget, body/bounds/clipping and exact-receipt check. Wind-up and
active share an acceleration operand but **not** phase authority or receipts.
The new kind can continue active motion or use the separately supported
recovery/relaunch entry contracts; it cannot bypass wind-up to replay active
entry.

Only descending, detached, zero-offset normal G-Red static pushes are enabled.
Active-entry pushes, early/null-callback or unreviewed depletion/touchdown
combinations, active-relaunch pushes, grounded recovery, attachments, moving
platforms, unresolved seams and combat remain refused. Native initialization,
camera/collider/ceiling/clock and host-math approximations are not original
hardware or whole-frame equivalence. Source evidence remains private:

```
local/recomp-evidence/gred-air-dash-v1/fn_80061560.asm
```

The bounded demo uses explicit **native-only** placements: G-Red at
`(295.518066406, 1631.279907227)` XZ and Sasuke at `(-2850,3000)`, with
arena-half4500 to include the passive target. Both floor query references
remain0 and both actors stand on real selected original floors. Interactive
mode retains ordinary MVP bounds/spawn. The one-shot script jumps at5, admits
a camera-relative right dash at38, and releases direction only after the first
accepted active push. That counter-driven demonstration is not original AI or
an original input/spawn script.

Its full native route accepts an active push during program11, then a separate
program12 recovery push on the adjacent slope, and finally ordinary lower-floor
touchdown/refill. Input-planning estimates were never offered as motion or
published poses: only the normal source owner/native geometry receipts qualify
the route. Near misses that took the source's away-displacement early return
remain ordinary air packets and correctly refuse below-floor acceptance.

Validation: the360-update route accepts141 airborne updates and111 active
callbacks. The program11 push at157 pays its one50-unit active cost, leaving
meter1850, while both lanes retain11 and readiness remains enabled. The separate
program12 recovery push follows at158 with meter1850 unchanged; actual
touchdown follows at159 without instant refill. Normal landing recovery restores
10000 and idle, stable through600 one-shot updates. The source also admits the
already-supported separate request14/additive relaunch after the active push.

Readiness-before-contact, timer-expiry and zero/depleted-meter pushes are
numerically covered; the rendered route is the after-readiness, nondepleted
case. A push clears4C even while the readiness timer remains positive, and the
following active update does not reapply entry's negative drag or reset speed.
Native typed phase/evidence cases include repeated pushes and separate
recovery/relaunch handoffs. These are not claims of additional rendered routes
or original actor/frame authority.

Twenty-two sanitizer test binaries pass, including1364 source-owner checks and
164 native active-capability/phase/evidence checks. Host/header builds,23 Python
recovery tests and22 source world/replay integration tests pass. Ordinary MVP
smoke remains64/0. Original DOL/setup/stage and all six immutable packs retain
recorded hashes; no extraction, asset edits or dependency changes were needed.

Contact Sheet25 shows actual offscreen GL separate entry, active approach,
program11 push, separate program12 recovery push, touchdown and post-refill idle.
No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-active-surface-v1/contact-sheet-25.png
```

### Earlier continued air-dash wind-up surface contact milestone

The newest mode adds **continued selector15/program10** steep pushes, not
entry-frame pushes or a general wind-up landing response:

```sh
build/source-windup-surface-host-v1/gf_native --source-windup-surface --no-audio
build/source-windup-surface-host-v1/gf_native --demo-source-windup-surface --headless --frames 360 --no-audio
```

It inherits the earlier recovery/falling contact features and uses the same
unchanged GFMM6 profiles, source VM/owner, motion and projection kernels, native
world receipts and renderer. Loading that pack in an older mode does not grant
continued wind-up push authority.

Qualified `61338` disassembly separates entry from continuation. Entry halves
old scalar/vertical speed and requests10 without translating. A later callback
advances10, updates its existing body-turn clock, calls unwrapped `67310`,
then decrements timer568. Timer completion charges cost2 and selects16 **before**
the `677B0` contact tail. Clip completion is not the wind-up timer. A steep
push returns0, so the caller's landed-only vertical-speed zero is not applied.
Vertical acceleration50 remains original data70 (`bd8f5c29`, approximately
**-0.07**), not falling/recovery's-1. Projection still clears4C and damps44/48
by0.9; it does not repeat the half-speed entry or refill grounded resource.

The distinct `WindupPush` capability and `AirDashWindupPush` kind require an
accepted wind-up entry/update, exact accepted old-velocity integration and the
wind-up acceleration/clamp before0.9 damping. All existing selected-plane,
projection, budget, clipping and exact-receipt checks remain. Ordinary wind-up,
recovery and falling packets cannot borrow this evidence or receipt. A continued
push may be followed by another wind-up update; when the source timer has
selected16, active entry remains a **separate** accepted reset/request of11.
Projection does not supply a body heading; the existing wind-up turn continues.

Entry-frame pushes, rising/uphill push entries, early/null-callback wind-up
landing, active-dash/relaunch pushes, grounded recovery, offsets/attachments,
moving platforms, unresolved seams and combat remain refused. These are normal
G-Red mappings, not inferred movement rules for other Borg families.
Source evidence remains private:

```
local/recomp-evidence/gred-air-dash-v1/fn_80061338.asm
```

**Demo-only placement/bounds:** the tiny steep faces inside the1800-half-width
MVP arena feed into still-unsupported early landings. The bounded wind-up demo
instead uses unchanged original mountain triangle2536, with native G-Red XZ
`(-2302.447265625, 528.269836426)`, Sasuke `(240,-300)` and native arena-half4500.
A declared static-floor query reference591.912719727 selects the real original
floor near that height; it is **not** an airborne position override or recovered
actor constructor. The reference is finite/bounded and defaults to0, preserving
ordinary MVP spawn selection. Interactive mode retains the old arena and spawn.
No original stage/model/program or immutable pack is edited.

The one-shot demo jumps at5, releases at33 and holds a camera-relative left
flick from79 through128. Its qualified native route accepts two continued
wind-up pushes while both lanes retain10, then progresses through active dash,
recovery and normal landing/refill. Native collider/camera/ceiling/clock and
host-libm assumptions remain labelled; no original hardware or whole-frame
equivalence is claimed.

Validation: the360-update route accepts141 airborne updates, two wind-up
pushes at86/87 (signed16 angle10382), one distinct active entry at88 and actual
touchdown at159. Both push frames retain program10 on both lanes. Meter8350
stays unchanged at86; only timer completion at87 spends450, leaving7900.
The separate active entry spends50 and performs its own velocity reset. Normal
landing/refill eventually restores10000 and idle; the one-shot route remains
idle through600 updates without replaying its input sequence. The older
recovery mode refuses the same steep contact before tick86 commits.

Twenty sanitizer test binaries pass, including1040 source-owner and140 native
wind-up/reference/receipt checks. Source input also successfully admits the
already-supported relaunch after the timer-completing push, not while15 still
owns continuation. Host/header builds,23 Python recovery tests and21 source
world/replay integration tests pass. Ordinary MVP smoke remains64/0. Original
DOL/setup/stage and all six immutable packs retain their recorded hashes.

Contact Sheet24 shows actual offscreen GL entry, both pushes, separate active
entry, later touchdown and refill. The detail sheet enlarges crops of the same
pixels with nearest-neighbour sampling, without another camera/render path.
No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-windup-surface-v1/contact-sheet-24.png
local/recomp-evidence/contact-sheets/source-windup-surface-v1/contact-sheet-24-detail.png
```

### Earlier airborne air-dash recovery surface contact milestone

The newest opt-in mode adds steep projection during **selector17 recovery**,
not wind-up/active dash or active relaunch:

```sh
build/source-recovery-surface-host-v1/gf_native --source-recovery-surface --no-audio
build/source-recovery-surface-host-v1/gf_native --demo-source-recovery-surface --headless --frames 240 --no-audio
```

It inherits the earlier falling-surface features and reuses the unchanged GFMM6
pack, source owner, VM, shared motion/contact, native receipt and renderer paths.
Older modes retain their selector6-only push authority.

Qualified `617E4` disassembly confirms the airborne recovery ordering: request/
advance lower program12; reset vertical acceleration from data6C on entry;
apply normal falling drift/scalar damping; refuse the unreviewed grounded
control1D0F branch; install the ordinary landing callback and perform `67458`
air motion; call `677B0`; only **then** test lower completion1D0E and, if done,
set history590=1 and select6. The upper callback still consumes its captured
program12 update. This is separate from post-recovery program13.

A steep push returns “not landed,” retains vertical acceleration50, clears4C
and applies the existing0.9 speed damping. If lower program12 is unfinished,
recovery continues. If it completed on that update, the same post-contact tail
selects falling without an invented extra recovery tick. Neither path grants
grounded refill or substitutes falling/landing animation for program12 early.
Actual touchdown remains separate; the unreviewed simultaneous touchdown/
completion case still refuses.

Native acceptance adds an explicit `RecoveryPush` capability and
`AirDashRecoveryPush` kind, retaining the same selected-plane evidence. It
requires accepted active/recovery history, exact old-velocity pre-integration,
reset falling acceleration/clamp, damped vertical speed and all existing
geometry, budget, clipping and exact-receipt checks. A narrower producer cannot
borrow this kind, and its receipt cannot be relabelled as selector6 falling.
Recovery may continue, finish into falling, or admit the already-supported
wind-up/relaunch inputs afterward; these do not bypass their entry contracts.

Only the declared airborne control1D0F=0, zero-offset, zero698/7C9, normal-G-Red,
descending static path is enabled. Grounded recovery, rising/uphill push
entries, wind-up/active-dash/active-relaunch pushes, attachments, moving
platforms, unresolved seams and combat still refuse. No new general movement
family rule or original actor-constructor/hardware/whole-frame equivalence is
claimed. The source evidence remains private:

```
local/recomp-evidence/gred-falling-contact-v1/fn_800617E4.asm
```

The bounded demo uses two explicitly **native** placements: G-Red at
`(44.928558350, -332.878112793)` XZ and Sasuke at `(1200,1200)`. The target
placement establishes the existing native camera-relative backward-dash route;
it is not a recovered spawn or an edited stage. Jump starts at5, the backward
flick is held from20 through60, and input releases into normal recovery. The
accepted route reaches unchanged original triangle1320 during program12.

The source-owner regression accepts one recovery push at tick73 and a separate
lower-floor touchdown at74. At the push, both lanes retain program12,
selector17 stays airborne, body heading is unchanged and meter7650 does not
refill. Pausing there advances no motion/VM/resource state. Subsequent landing
entry recovers normally to10000 and idle. The older falling-surface mode refuses
the same recovery contact at tick72 without committing or falling back.
Native phases, repeated projection, completion handoff and receipt refusals
also have numeric fixtures; these are not additional original-stage routes.

Validation:18 sanitizer test binaries pass, including592 source-owner checks
and146 native recovery-capability/phase/evidence checks. Host/header builds,
23 Python recovery tests and20 source world/replay integration tests pass.
Ordinary MVP smoke remains64/0. Qualified original DOL/setup/stage and all six
immutable packs retain recorded hashes; no extraction or stage edits were
needed.

Contact Sheet23 contains actual offscreen GL active-dash, recovery entry,
approach, still-airborne program12 push, separate touchdown and post-refill
idle captures. No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-recovery-surface-v1/contact-sheet-23.png
```

### Earlier selector6 contact after dash/relaunch history milestone

The newest mode extends **falling** surface contact after G-Red has left the
special air-dash/relaunch callback, not contact within those active callbacks:

```sh
build/source-fall-surface-host-v1/gf_native --source-fall-surface --no-audio
build/source-fall-surface-host-v1/gf_native --demo-source-fall-surface --headless --frames 240 --no-audio
```

It reuses the existing immutable GFMM6 pack, shared source owner/VM, motion
kernels, native selected-plane acceptance, receipts and renderer. No new pack,
actor constructor, source pointer ownership or parallel render engine is added.
Older surface/steep modes retain their narrower contact refusals.

Qualified initialized-DOL disassembly of `5FECC` shows that selector6's
**same lower callback** chooses program13 for history590=1 and program21+coarse57D
for history590=2, otherwise the ordinary falling program6. Its entry sets
cached87=1, resets planar acceleration4C and vertical acceleration50 from
data6C, installs the ordinary landing callback and sets the airborne behavior.
All three history paths then perform the same falling drift/damping, motion and
`677B0` contact tail. The upper callback `5FE04` uses the corresponding history
program. The different animations do **not** imply different falling-contact
physics or permission to replay a special launch/reset.

The reviewed producers remain distinct: recovery `617E4` sets history590=1 and
selects6 only after its lower completion; normal relaunch `6235C` declares
history590=2, with its existing input release admitting6. The mode preserves
program13 and programs21–24 instead of replacing them with ordinary program6.
It does not generalize these normal-G-Red mappings to other Borgs.

Native `FallHistoryPush` capability adds only the immediate falling-entry
contract from accepted air-dash recovery or relaunch history. The next
selector6 update must integrate the accepted **old** vertical velocity exactly,
apply reset falling acceleration/clamp, and pass every existing selected-plane,
projection, damping, budget, native collision and exact-receipt check. The older
`StaticPush` capability still refuses those immediate special-history entries.
Wind-up/active-dash kinds cannot borrow this permission, nor can a packet borrow
a relaunch's additive delta while declaring a falling push.
Push admission remains descending-only; rising/uphill entry responses still
refuse rather than inheriting authority from the selector number.

The source owner generates these offers only from selector6. Steep contact
inside selectors8/15/16/17 still refuses, including active relaunch and recovery
before the callback has actually handed off to falling. Pushed results retain
cached airborne history and resource value; a separate later touchdown and
entered-landing update must perform landing/refill.

The bounded original-stage demo uses a declared **native-only spawn** at
`(-606.4511719, -1373.5872803)` XZ inside unchanged steep triangle1320. It jumps,
relaunches at tick45, releases at60, and reaches the selected steep face in
post-relaunch falling program21. Native collider/camera/ceiling bounds and host
math remain approximations, not original actor-constructor, hardware or
whole-frame equivalence. Qualified static evidence is private:

```
local/recomp-evidence/gred-falling-contact-v1/
```

The240-update demo completes106 airborne updates, one post-relaunch falling
push at tick123 and a separate lower-floor touchdown at124. Both lanes retain
program21 at the push, selector6 stays airborne and meter6800 is unchanged.
The later landing entry refills normally to10000 and returns to idle. The old
steep mode still refuses this same post-relaunch contact; the new mode also
refuses a tested late relaunch that hits the steep face while still selector8.

Numeric tests cover program13 and every21–24 variant with the reset falling
acceleration, plus native immediate-entry capability and exact-receipt checks.
The original-stage air-dash route still completes recovery/program13 and normal
landing. A positive post-dash steep projection is numerically covered, not
claimed as an original-stage rendered route.

Contact Sheet22 contains actual offscreen GL captures of relaunch, falling,
program21 push, separate touchdown and refill. No desktop/classifier focus
was taken; no original asset or immutable pack was changed:

```
local/recomp-evidence/contact-sheets/source-fall-surface-v1/contact-sheet-22.png
```

Validation: the15 existing sanitizer regressions pass, and the new falling-
history source test passes1689 checks after selecting a high-enough dash route
to execute program13 before touchdown. Native immediate-entry/receipt tests
pass118 checks. The normal host/header build,23 Python recovery tests and19
opt-in source world/replay tests pass. Ordinary MVP smoke remains64/0. All six
immutable packs retain their recorded hashes and unchanged shared components.

### Earlier ordinary-falling steep projection milestone

The newest movement-only mode extends the shared owner/native receipt path,
not the ordinary MVP or older modes:

```sh
build/source-steep-host-v4/gf_native --source-steep --no-audio
build/source-steep-host-v4/gf_native --demo-source-steep --headless --frames 240 --no-audio
```

It requires the immutable `local/content/source-gred-ground-v6` pack. GFMM6
extends GFMM5 to360 bytes with the original `804375D4` float at356:
`3f666666`, approximately0.9, **not** air-dash windup's0.5. All six other pack
components are unchanged. GFMM1–5 retain their qualified identities and
capabilities; merely loading GFMM6 does not grant push authority to older modes.
Extraction remains qualified, bounded, read-only on inputs and refuses output
overwrites:

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v6
```

The reviewed677B0 steep response uses the paired selected height/normal, after
the existing above/away early returns and classAC/AE angle gate. It computes
`(selectedHeight - proposedY) * sourceCos(signed16Angle)`, scales the selected
normal without normalizing it, and adds that vector to the proposed position.
The separate float scale/add and shared quantized host cosine model are
preserved. It then clears planar acceleration4C and multiplies scalar planar
speed44 and vertical speed48 by0.9. Vertical acceleration50 is **not** cleared.
There is no floor snap, callback754, selector7, cached87 reset or grounded refill:
the response means **pushed, not landed**.

Native acceptance has a distinct `StaticPush` policy/kind and optional evidence
containing the integrated pre-push position/speed and selected static plane,
triangle, angle and damping. Other motion kinds refuse that evidence. The
world checks exact accepted old-vertical-velocity integration, normal G-Red falling
acceleration/clamp, the native default42-unit planar and50-unit vertical budgets, an independently
queried native selected surface, the shared exact projection, and damped
vertical speed. Height-only terrain providers cannot certify a selected plane.
The existing bounds/body/terrain response must not clip either checked motion;
the corrected result must remain at/above the native final floor within its
existing0.01-unit contact tolerance. Every evidence
field participates in receipt matching before source state or pose publication.

The source does not re-query after its push. A corrected position may therefore
end above a **different lower adjacent floor** without becoming grounded. Native
acceptance does not demand the post-push triangle/normal equal the selected
pre-push plane; it still refuses penetration or an unresolved seam. This does
not authorize new solid/capsule/platform behavior.

Live push generation is restricted to normal selector6 falling, not ascent,
air-dash/relaunch callbacks or their special falling programs. Existing source
jump/dash/relaunch/entered-landing controls are inherited, but unreviewed steep
contacts in those special paths remain refused. The native owner still declares
timer698=0,7C9=0, zero pose offset, rank1 and fixed clocks. G-Red's gates and
damping are not generalized to Sasuke or other Borg movement families.

The bounded demo uses an explicitly **native spawn** at
`(-606.6735, -1373.0029)` XZ inside original stage473 floor triangle1320,
approximately63.6degrees. It releases jump at tick33, projects across the edge
while remaining airborne, then lands on the lower original floor through the
ordinary source landing callback. The stage is not edited; actor constructor/
spawn, collider, camera, ceiling and host-libm assumptions remain labelled.
Another tested input duration at the same spawn produces an unresolved second
projection and must refuse, rather than receiving a guessed floor snap.

The240-update demo accepts65 airborne frames, exactly one push at tick82
(signed16 angle11582) and one actual lower-floor touchdown at tick83. At the
push it remains selector6/ungrounded with meter8350 unchanged; touchdown
selects7 without instant refill, followed by ordinary entered-landing recovery
to10000 and idle. The accepted push finishes at approximately
`(-602.995, 10.853, -1376.754)`, above the adjacent original floor at10.
Original inputs and all six immutable packs retain recorded hashes, including
the six unchanged shared components; existing outputs refuse overwrites.

Contact Sheet21 shows actual offscreen GL spawn, ascent, approach, still-airborne
push, separate touchdown and post-refill idle. The final host's push/touchdown
captures are byte-identical to the sheet's frames. No desktop/classifier focus
was taken:

```
local/recomp-evidence/contact-sheets/source-steep-v1/contact-sheet-21.png
```

Fifteen sanitizer regressions and the sanitizer host/header build pass, including
the exact pre-push integration check, selected-plane spoof/receipt refusals,
height-only terrain, invalid body clipping, adjacent-floor penetration and
original-stage source/pose/resource lifecycle cases. The sanitizer host completes
the240-frame steep demo and ordinary MVP smoke (still64/0). All23 Python
G-Red recovery/extraction tests pass.
The final normal host/header build and18 opt-in source world/replay tests also
pass. Its strict push/touchdown GL captures remain byte-identical to Sheet21.

### Earlier paired static-surface contact and takeoff input milestone

The current opt-in mode enables the reviewed normal G-Red static-surface
landing subset, using the same source movement owner, native acceptance and
shared renderer:

```sh
build/source-surface-host-v3/gf_native --source-surface --no-audio
```

Jump, air dash, relaunch, dash-jump and entered-landing controls are inherited.
This mode requires a new `local/content/source-gred-ground-v5` pack. GFMM5 adds
original signed16 classAC/AE landing-angle gates at348/350 and the displacement-
squared threshold at352, for a356-byte extent. The other six pack components
are unchanged; old GFMM1–4 identities and mode-specific refusals are retained.

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v5
```

Always use a new output directory. Existing packs refuse overwrites.

Contact now reuses the existing shared `stage_ground_surface` query corresponding
to3E0A8: **height and normal come from the same selected original triangle**,
including strict gates and lowest-floor fallback. It does not combine3D6E4's
height with3DE80's separately selected/default-UP normal. Missing/outside
selected surfaces refuse; defaultUP is not fabricated collision evidence.

The reviewed677B0 subset retains its above-height early return and normalized-
displacement dot early return for movement away from the surface. A no-contact
numeric result still requires native world acceptance; it is not permission to
move below the native floor. Eligible incoming surfaces use the source signed16
angle domain and classAC gate (`10240`,56.25degrees). Positive timer698 selects
the classAE gate (`6144`,33.75degrees). The movement-only owner explicitly
declares timer698=0 and7C9=0; no original hit/size-animation producer is claimed.

Only the source landing/snap branch is enabled: detached actor, zero pose
offset, selected static surface within the angle limit, and no unresolved
raised-surface/side entry. The steeper push/velocity-damping branch, attachments,
moving platforms,7C9, props/solid equivalence and early relaunch/dash contact
variants still refuse. Native body/bounds response and source/native height
agreement still gate commit. Horizontal body clipping on a slope may therefore
refuse if it invalidates the proposed surface; no response is guessed.

**Takeoff is not cancellable locomotion.** Source selector4/593AC samples input
but admits no normal jump/dash transition under the reviewed movement-only
conditions. In this mode, a release/re-press during grounded authored takeoff
is allowed through the input kernel without restarting the VM, launching early
or inventing a double jump. Older modes retain their narrower re-press refusal.
Weapon/auxiliary/blocked-lane branches are not acknowledged as executed.

`--demo-source-surface` places the initialized native G-Red actor at an interior
point of original stage473 floor triangle1297, about24.4degrees. This placement
is explicitly a **native demo spawn**, not a recovered actor constructor/spawn
or an edited stage. The bounded jump includes a takeoff re-press, then lands and
refills at the same point using the original selected height and normal.
Interactive mode retains the normal MVP spawn.

Profile/source instructions remain read-only and qualified; focused static
evidence is private:

```
local/recomp-evidence/gred-surface-contact-v1/
```

The numeric host atan/normalization model, fixed source clocks, native camera/
collider/ceiling bounds and omitted actor/cosmetic callbacks remain labelled.
This is not original hardware or whole-game/frame equivalence, and G-Red's
angle gates are not generalized to other Borgs.

Validation: the original-stage demo completed240 accepted updates and59 airborne
updates, landed once on the selected incline at signed16 angle4448, refilled
to10000 and returned to idle with health100/100 and no hits/projectiles.
Baseline and takeoff re-press runs reach the first airborne update on the same
tick. An older flat-only mode still refuses this landing without falling back
to native gravity. Numeric tests cover paired lowest-floor normals, both angle
gates, away/above early returns, missing surfaces and unsupported steep/offset/
attached/7C9 cases.

Fourteen sanitizer regressions, host/header builds,23 Python recovery tests and
17 opt-in source world/replay tests passed. The ordinary MVP smoke still
reaches64/0. Original inputs and all old packs retain their hashes; the six
shared pack components remain unchanged.

Contact Sheet20 contains actual offscreen GL captures of the original incline,
takeoff re-press, ascent, paired-surface touchdown and refill. No desktop or
classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-surface-v1/contact-sheet-20.png
```

### Earlier entered-landing jump and ground-dash interruption milestone

The current opt-in mode adds normal G-Red's landing input predicates to the
existing movement owner, VM, resource kernel and native world/renderer:

```sh
build/source-landing-host-v2/gf_native --source-landing --no-audio
```

After an accepted landing-entry update, Shift/B may select another ground
takeoff and a WASD/stick flick may select ground dash. Jump has priority over
dash. Arrows/D-pad remain sub-flick steering; **soft walking does not cancel
landing**. Previously supported dash-jump, airborne steering, air dash and
relaunch remain available. Older modes retain their landing-interruption
refusals. The same qualified `source-gred-ground-v4` pack is reused unchanged.

The source selector7 handler59574 tries69038's buffered-and-held jump predicate
before69354's buffered direction predicate. Ground jump does not require a
positive air meter; ground dash consumes the original direction nibble, not an
air-dash charge. A held jump with an expired source command does not bounce
automatically. Unblocked upper/lower synchronization still requires59F5C's
`flags5E0 & 81000003 == 0` guard; attacking/blocked variants are not authorized.

**The first landing entry is a native admission boundary, not a recovered
extra input lock.** This lab does not yet permit a touchdown packet to skip the
landing-entry callback. A pure input preview refuses a fresh or still-buffered
held jump that would cancel that entry, without moving the world or retiring
the displayed pose. Release it to let entry run, then press again. Raw API
attempts also refuse before native simulation. Directional input retains the
native sub-flick cap during touchdown; full landing flicks are enabled after
entry. These convenience mappings are not original GameCube calibration.

Landing entry uses the existing original lower prefix and program8/clip9, clears
cached87 and contact timer69C, and publishes only after its matching native
receipt. Bookkeeping on that first update still uses the **previous airborne**
cache, so touching ground does not instantly refill. Starting on the following
update, the shared recovery kernel adds2000 per source tick (up to10000), even
while landing is unfinished or being interrupted. Cancellation does not force
refill or wait for looping/clip completion as a refill trigger.

An admitted jump enters the same original program2/clip6 takeoff path, without
advancing old landing first or applying a guessed immediate launch. An admitted
dash uses the same cardinal heading, buffer consumption and ground-dash entry
as the ordinary ground controller. No new collision engine or world-acceptance
exception was added: existing landing-to-ground/takeoff receipts apply only
after accepted landing entry. Input/VM/refill/interruption counts remain staged
until the exact current-world/model/producer frame is accepted.

`--demo-source-landing` runs a bounded jump-cancel then dash-cancel sequence and
returns to idle. Contact still has the previously documented native body/
terrain/ceiling/camera assumptions. Instant touchdown cancellation, unreviewed
takeoff/blocked-lane input, combat, props/platforms and non-flat/unknown responses
remain refused. Cosmetic/original actor callbacks are not claimed executed;
this is not original actor-constructor, hardware or whole-frame equivalence.

Focused source/decompiler/initialized-instruction evidence:

```
local/recomp-evidence/gred-landing-interrupt-v1/
```

It covers59574,69038,69354,60228/601D0,59F5C and5568C. These are navigation
addresses, never native call targets or original actor-execution receipts.

Validation: the adaptive demo completed480 accepted updates,118 airborne updates,
two landings, one landing jump and one landing dash, then returned to idle with
resource10000, health100/100 and no hits/projectiles. The depleted-meter test
observed0 on landing entry, then2000/4000/6000 recovery while landing remained
unfinished and was dash-cancelled. Fresh and held-buffered touchdown cancellation
both refuse without simulation/pose retirement. Old-mode refusals, staged
counts/spending and exact receipt labels remain covered.

Twelve sanitizer regressions, host/header builds,23 Python recovery tests and16
opt-in world/replay tests passed. The ordinary MVP smoke still reaches64/0.
Contact Sheet19 contains actual offscreen GL captures; no desktop/classifier
focus was taken:

```
local/recomp-evidence/contact-sheets/source-landing-v1/contact-sheet-19-verified.png
```

### Earlier ground dash-to-jump interruption milestone

The current opt-in mode enables normal G-Red's ground-dash jump predicate:

```sh
build/source-dash-jump-host-v2/gf_native --source-dash-jump --no-audio
```

Flick WASD/stick to ground dash, then press Shift/B during the dash. Arrows/D-pad
provide steering. The existing source jump, air-dash and relaunch paths remain
available in this mode; older modes retain their explicit dash-to-jump refusal.
The same `local/content/source-gred-ground-v4` pack is reused unchanged.

Selectors12/13/14 use the recovered ground jump command: buffered source bit1
**and held** jump bit100, unless the source jump lock is set. Admission happens
before lower/upper dispatch. An admitted jump selects4 and resets direction57C,
while retaining coarse57D and body heading. The old dash VM/steering/movement
handler is not advanced on that update.

For the reviewed unblocked two-lane path,59F5C synchronizes upper selection
through6A6FC before55568 captures dispatch state. The owner requires the original
`flags5E0 & 81000003 == 0` guard rather than assuming that a lower jump predicate
authorizes replacement of an attacking/blocked upper state.

Takeoff uses the already-integrated original program2/clip6. **Normal G-Red's
class12 clears dash speed and planar acceleration**; it does not inherit the
MVP's dash-momentum carry, and class-flag1 preservation is not generalized to
this Borg. The actor stays grounded during the authored takeoff program, then
enters the existing source ascent on lower VM completion. No native takeoff
duration or early launch is substituted.

This is a newly enabled source input transition, not a new motion/render engine
or a relaxation of world contact checks. The existing grounded moving-takeoff
offer/receipt path accepts it. Jump count, VM/input and the interruption selector
remain private until exact native world acceptance and pose publication. An
admitted recovery jump does not need CFF4's old recovery continuation because
that callback does not execute; its unsupported-feature refusal is unchanged
when recovery actually continues.

The native convenience adapter caps directional deflection below the air-flick
threshold while requesting/taking off, avoiding an unreviewed queued air-dash
entry before accepted airborne state. Once airborne, the previous steering,
flick-dash and readiness/resource-based relaunch semantics apply. This is not
original GameCube device calibration. Landing/takeoff interruption, combat,
props/moving platforms and unreviewed contacts still refuse. The native body/
terrain/ceiling/camera assumptions and non-equivalence labels remain in force.

`--demo-source-dash-jump` scripts two dash interruptions, authored takeoffs,
flights and landings. Bounded source/decompiler/disassembly evidence is private:

```
local/recomp-evidence/gred-dash-jump-v1/
```

Key evidence points: shared selection59068, dash selector handler59690,
jump predicate69038, upper synchronization59F5C/6A6FC, dispatch55568 and
takeoff5F978. They are navigation addresses, never native call targets or
original actor/frame execution receipts.

Validation:480 accepted updates, two dash jumps,134 airborne updates, two
landings and full refill, with health100/100 and no hits/projectiles. The
real-stage suite covers all four dash directions, immediate/active/recovery
interruption, staged counts, no queued phantom air dash, unchanged neutral
heading, old-mode refusals and pose retirement. Eleven sanitizer regressions,
host/header builds,23 Python recovery tests and15 opt-in source world/replay
tests passed. The ordinary MVP smoke still reaches64/0.

Contact Sheet18 shows actual offscreen GL captures. The accepted root position
is identical at tick15 (dash) and tick16 (takeoff entry), and remains grounded
during the authored takeoff wait. No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-dash-jump-v1/contact-sheet-18.png
```

### Earlier airborne relaunch milestone

The new opt-in extension enables normal G-Red's source selector8 using the same
movement owner, VM, resource kernel, native contact gates and shared renderer:

```sh
build/source-relaunch-host-v3/gf_native --source-relaunch --no-audio
```

Release and freshly press Shift/B while falling, actively dashing or in dash
recovery. The source predicate must admit the request and resource must remain.
Falling/recovery uses the source readiness latch; active-dash relaunch does not
wait for the minimum dash timer. A too-early falling press is a normal gated
input: it is neither charged nor queued until readiness. Release/repress again
when ready. Old modes still refuse relaunch, and landing/takeoff interruptions,
dash-to-jump, combat and unreviewed contacts remain outside this subset.

Ground movement, source jumping and air dashing remain available in this mode.
Arrows/D-pad provide sub-flick steering; airborne WASD/stick flicks can request
air dash. The extension reuses the existing qualified
`local/content/source-gred-ground-v4` pack. No new assets, input calibration,
animation labels or operand overrides are required.

Relaunch entry pays source cost0 (`150`), adds `data4C` (`15`) to the **current**
vertical velocity, resets timer690/readiness, and requests program14/clip14.
It does not overwrite vertical speed with15: a descending actor can initially
keep descending, and a rising actor's first displacement can exceed the later
upward-speed clamp. The shared air wrapper applies its clamp **after** that
displacement. Every continuation pays cost1 (`100`) once on the lower callback.
Source rate664/timer768/5F4/1DC8 remain explicitly fixed to1.

The lower wind-up updates program14 until its VM completion sets lower stage2.
The upper lane still updates14 on that callback while observing the lower
completion and setting its own stage2. On the following callback each requests
program15/clip7 and enters stage3. This preserves `8006235C/80062284`'s separate
stage latches; no guessed clip timer or immediate upper-lane switch is used.
Held-direction steering uses shared turn gain1, with the original pitch
acceleration and neutral damping. Camera rotation alone cannot turn the body.

Release/depletion selects falling. The relaunch-origin branch uses program
`21 + retained coarse57D`, not the ordinary fixed-threshold apex or the
air-dash-origin program13. In the qualified normal G-Red pack,21–24 are
byte-identical; the harness retains the reviewed ground-dash coarse field or its
declared native spawn zero. These aliases are not generalized to other Borgs.
Air-dash recovery completion restores its separate program13 fall origin.

Native relaunch requires a separate producer capability and entry/wind-up/
active offer kinds. Only relaunch entry may carry a nonzero declared additive
vertical step. Its exact value joins receipt matching; ordinary jump/dash
packets still require zero and retain their existing old-velocity checks.
Skipped phases, edited additive steps, old capabilities and foreign/restarted
world or renderer leases cannot authorize a relaunch pose. Input/VM/meter/count
changes remain staged until native world acceptance.

**Contact boundaries:** selector8's floor-contact/damping branch is not yet
integrated; a relaunch that reaches a surface refuses rather than fabricating a
landing or bounce. Surface clearance, later falling/landing, the native body
proxy and declared ceiling bound remain the previously documented subset.
Cosmetic/sound/actor side-effect callbacks are omitted, not acknowledged as
executed. This remains a native movement lab, not original actor-constructor,
solid-response, hardware or whole-frame equivalence.

`--demo-source-relaunch` scripts two fresh-press relaunches in one flight,
including both wind-up/ascent handoffs, release, landing and refill. Separate
real-stage tests cover active-dash interruption before its minimum timer,
readiness-gated input, deferred spending, pause, neutral camera/body
independence and mid-flight restart. No fixed jump-count limit is substituted
for G-Red's installed resource policy1.

Validation:480 accepted updates,238 airborne updates and two relaunches in one
flight, followed by one landing/refill to10000, with health100/100 and no hits/
projectiles. The real-stage tests also interrupted an active dash before30
active updates and verified mid-flight restart revocation. Ten sanitizer
regressions, host/header builds,23 Python recovery tests and14 opt-in world/
replay tests passed. The normal MVP smoke still reaches64/0.

Contact Sheet17 contains actual offscreen GL captures of entry, lower completion
at stages2:2, the following callback's ascent request at3:3, a second relaunch
and landing/refill. No desktop/classifier focus was taken:

```
local/recomp-evidence/contact-sheets/source-relaunch-v1/contact-sheet-17-verified.png
```

### Earlier air-dash wind-up, active phase and recovery milestone

The opt-in air-dash extension reuses the same source movement owner and native
world/renderer:

```sh
build/source-air-dash-host-v4/gf_native --source-air-dash --no-audio
```

Jump with Shift/B. Once airborne, center/release directional input and then
flick WASD/stick to dash. Arrows/D-pad remain32-unit steering input below the
source flick threshold52. Unlike `--source-jump`, this mode does not cap airborne
stick/WASD input and then skip a dash predicate: the recovered buffered-input
kernel and movement admission consume the real flick nibble. Takeoff/landing
input is still capped as a native convenience to avoid unreviewed ground
interruptions; this is not original GameCube calibration.

Keep a direction held to continue dashing. After the source minimum active timer
has elapsed, centering direction selects recovery. Releasing Shift by itself is
not the active-dash stop command. Fresh airborne jump/relaunch, dash-to-jump and
combat remain refused before simulation advances. This is normal G-Red only,
not a flight-family rule or a mapping for Sasuke.

The extension requires a new `local/content/source-gred-ground-v4` pack.
GFMM4 adds the original `.5` wind-up speed factor and30-unit contact timer at
offsets340/344, for a348-byte extent. The other six components are unchanged.
GFMM1/2/3 retain their qualified identities and older modes; they cannot
authorize the new dash mode.

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v4
```

Existing outputs still refuse overwrites.

The source-backed sequence is selector15/program10/clip14 wind-up,
selector16/program11/clip23 active dash, then selector17/program12/clip23 from
frame10 recovery. Wind-up uses the buffered octant heading and half the
absolute-angle sector's turn count. Its entry halves old planar/vertical speeds
and pauses translation for that update; subsequent wind-up updates turn and
move along the retained travel heading. Completion pays cost2 (`450`) **after**
movement and selects the active phase.

Active entry resets planar speed to30 and vertical speed to0, with the original
planar/vertical changes (`-.05` / `-.07`). It spends cost3 (`50`) on every active
update, including entry, and steers using shared66E40 gain2. For G-Red's installed
resource policy1, the30-update timer enables direction-release recovery and the
landing callback; it does **not** force a stop while direction remains held.
Neither program11's earlier VM completion nor a guessed per-flight dash count
ends the active handler. Depletion selects recovery with the source readiness
preservation and callback reset.

Recovery requests original program12, applies the reviewed ordinary air-motion
branch, and restores the landing callback. Its completion returns to selector6
using program13's falling request, not another invented apex animation. A new
admitted flick may interrupt recovery while resource remains. Input/VM/resource
changes stay private until the corresponding native motion receipt is accepted.

The native world has a separate air-dash producer capability and explicit
wind-up-entry, wind-up-movement, active-entry, active and recovery offer kinds.
Thus the source entry pauses/velocity resets do not weaken the exact
old-velocity check on ordinary jumping. Skipped phases, old jump producers,
edited phase/velocity packets and stale renderer/producer leases still refuse.
The same bounded native body/terrain response remains authoritative for native
position acceptance; no parallel collision or rendering engine was added.

The reviewed677B0 **above-surface early return** now permits overflight without
requiring flat equal-height floor samples below an airborne actor. Actual
landing remains the detached, zero-offset flat/equal-height subset. Early
wind-up/active floor contact without a declared landing callback, simultaneous
recovery completion plus landing, slopes, moving platforms and other unreviewed
contact responses refuse rather than inventing a fallback. Visual/afterimage/
sound callbacks are omitted, not claimed executed. Source clocks/rates are
still fixed to1, and native camera/solid/ceiling approximations remain labelled.

Private decompiler hints and initialized-DOL disassembly:

```
local/recomp-evidence/gred-air-dash-v1/
```

Key navigation points: `800612E0/80061338` wind-up,
`800614F0/80061560` active, `8006178C/800617E4` recovery. Relaunch's
`80062284/8006235C` was evidence-only in this narrower mode; the new route above
models the reviewed numeric/VM subset without calling original callbacks.
These addresses are never native call targets or actor-execution receipts.

Validation:480 accepted native updates,141 airborne updates, two wind-ups and69
active-dash updates in **one flight**, followed by one landing and full refill.
No hits/projectiles were produced and health stayed100/100. The real-stage/
pose/transaction test passed10676 checks. Nine sanitizer regressions, host/header
builds,23 Python recovery tests and13 opt-in world/replay tests passed. Coverage
includes policy1's held-after-timer behavior, early depletion planning, skipped/
edited reset phases, prior-cache recovery, refused relaunch input and producer/
renderer leases. The ordinary MVP smoke still reaches64/0.

Contact Sheet16 shows actual offscreen GL wind-up, active-after-minimum,
recovery, second-dash and landing/refill captures. No desktop/classifier focus
was taken:

```
local/recomp-evidence/contact-sheets/source-air-dash-v1/contact-sheet-16.png
```

### Earlier moving takeoff and airborne steering milestone

The current opt-in extension admits walk/idle takeoff, source ascent turning and
ordinary G-Red falling drift:

```sh
build/source-moving-jump-host-v3/gf_native --source-jump --no-audio
```

Use arrows/D-pad to walk, then Shift/B to take off. While jumping, WASD/stick/
arrows steer; release Shift/B to fall. A held jump still spends the source meter
and falls on depletion. The native jump-direction adapter caps each axis to32
units while jumping or requesting jump. Thus directional steering stays below
the source radial flick threshold52, rather than detecting an air-dash request
and silently ignoring its missing consumer. Ground stick/WASD flick dashes are
unchanged. **Dash-to-jump, fresh airborne relaunch, landing interruption, air
dash and combat remain refused**, before simulation advances.

`--demo-source-jump` runs a480-update script with two walk-interrupted jumps,
direction changes, falling drift and landings. This uses the same controller
owner, input kernel, VM, motion/pose systems, native world and renderer as the
earlier modes, not a parallel air controller or render engine.

Moving mode requires a newly prepared `local/content/source-gred-ground-v3`
pack. GFMM3 extends the numeric motion profile from320 to340 bytes:
ascent/falling damping base/gain at320/324/328/332, signed16 ascent pitch at336,
initial timer690 byte at338 and reserved zero at339. The other six pack
components remain byte-identical. Qualified GFMM1 and GFMM2 readers/modes remain
supported; old packs explicitly refuse moving-jump binding.

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v3
```

Always choose a new output directory; existing packs refuse overwrites.

The recovered ordinary class12 path has several important distinctions:

- Takeoff clears planar speed/change. Do not copy class-flag1 momentum retention
  into normal G-Red or preserve walking speed by analogy with the native MVP.
- Ascent entry uses the held camera-relative heading and a source minimum planar
  speed6 for directed input. Its initial signed16 turn uses the absolute-angle
  sector's original turn count. During that turn it retains the entry direction.
  Steady ascent then recomputes heading and uses the shared66E40 operation with
  turn gain1 (`.05`), separate from ground gain0 (`.04`).
- Directed ascent splits acceleration `.8` using source pitch8192; scalar planar
  speed is clamped to8 and upward speed to18 by the existing air wrapper.
  Releasing direction applies the original `.98` planar damping. Camera movement
  without a held direction does not force body heading.
- **Correction to the earlier neutral prototype:** neutral ascent evaluates
  `.8 * cos(0) + data68`, not `data68` alone. With these inputs this is positive
  vertical acceleration `.8`, capped at18. The correction also applies to the
  old vertical-only mode using its existing GFMM2 operands. Earlier captures and
  the237-air-update milestone predate this correction; the current600-update
  neutral script has257 airborne updates and still two landings.
- Ordinary falling applies676D8's direct4-unit camera-relative displacement
  when a direction is held, then `.99` damping and retained scalar momentum/
  travel integration. For G-Red's type0 it **does not** turn the body toward that
  input. Lower VM, heading/motion/contact and the previously captured upper
  callback retain their reviewed ordering.

The moving producer capability and offer family are distinct from both ground
and vertical-only capabilities. Native acceptance retains exact phase/velocity/
producer receipts and now sends declared vertical segments through the shared
opponent sweep. A descending segment cannot skip the body slab merely because
its old endpoint was above it. Unsupported descending/standing-on-Borg overlap
refuses rather than inventing a vertical solid response. Accepted horizontal
clipping is committed back to the source owner's position.

Contact remains the reviewed detached, zero-offset **equal-height flat static
endpoint** subset. Slopes, platforms, props, ceiling/solid equivalence and moving
floor changes remain unsupported. Timer698 is explicitly declared zero in this
movement-only lab; no attack/hit producer is claimed. Source clocks/rate664/
timer768 remain fixed to1. Cosmetic/sound callbacks are omitted. These are native
contact/device/camera approximations, not original whole-frame equivalence.

Additional private source/disassembly evidence:

```
local/recomp-evidence/gred-air-steering-v1/
```

Validation: the scripted moving route completed480 accepted updates,134 airborne
updates and two landings with no hits/projectiles and health100/100. The corrected
neutral script completed600 updates and257 airborne updates, including held
depletion and complete refill. Eight sanitizer regressions, host/header builds,
23 Python recovery tests and12 opt-in world/replay tests passed. Tests include
walk-interrupted takeoff, held versus neutral ascent heading, camera/body
independence, body-slab sweep and unsupported overhead-contact refusal, pose/
receipt leases, old-pack refusal and edited GFMM1/2/3 operands. The normal MVP
smoke still reaches64/0. Original input and old-pack hashes are retained.

Contact Sheet15 contains six actual offscreen GL captures, without taking
classifier/desktop focus:

```
local/recomp-evidence/contact-sheets/source-moving-jump-v1/contact-sheet-15.png
```

### Earlier neutral vertical-jump milestone

The separate opt-in mode below adds a **stationary, neutral-stick** source
takeoff/ascent/falling/landing subset. The ground-only modes keep their jump
refusal; no default controller has silently gained an airborne branch.

```sh
build/source-vertical-jump-host-v3/gf_native --source-vertical-jump --no-audio
```

Stop moving first, then hold Shift/B to jump vertically. Release to select the
source falling sequence. Continuing to hold consumes the source resource until
depletion selects falling; the existing press may remain held through landing
without inventing another jump. Enter pauses and Q restarts. Ground walking/
dashing are still available when not in the jump subset. Moving takeoff, airborne
directional input, fresh airborne relaunch, air dash and combat refuse before
simulation advances. Fresh jump interruption during the landing animation is
also outside this subset. Release an unsupported input to resume the accepted frame.

`--demo-source-vertical-jump` runs a600-update script with a short release-driven
jump and a held-through-depletion/landing jump. It reuses the existing GFMM2/
GFMI/GFMP/GFHC pack at `local/content/source-gred-ground-v2`; no new assets or
operand overrides are needed.

The closed subset uses selectors4/5/6/7 and programs2/4/6/8 respectively:

- Takeoff requests the original lower/upper program, staying grounded until
  lower VM completion selects5. No guessed native takeoff timer is substituted.
- Ascent entry spends cost0; later ascent updates spend cost1 through the shared
  resource kernel. For these qualified inputs the limit is10000, costs150/100,
  delay0 and grounded recovery increment2000. Source clocks are explicitly1.
- Neutral ascent launches with source vertical speed15. The corrected current
  path applies `.8` vertical acceleration up to the source clamp18 (see above).
  Release/depletion selects program6's authored apex-to-falling
  sequence while preserving upward velocity; source falling acceleration is-1.
  This is not the MVP ballistic jump.
- Only the reviewed detached, flat, zero-pose-offset contact branch is admitted.
  A descending floor crossing selects7, then the original landing program8 and
  its VM completion return to idle. The landing lower prefix also retains the
  original takeoff-program request before the landing request.
- Recovery consumes prior accepted cached87 history, not a newly observed floor
  or pose classification. Touchdown does not instantly refill the meter; the
  first landing callback clears cached87 and subsequent bookkeeping refills it.

The native world requires a distinct vertical-capable producer lease plus
takeoff/air/touchdown/landing offer kinds. It rejects skipped phases, lateral
motion, rising/early touchdown and edited velocity/phase receipts. Position,
grounded state and vertical velocity join the exact accepted receipt. Once
source motion owns an actor, there is still no fallback to native MVP gravity
or curated animation. The source owner commits VM/input/resource state only
after the matching world acceptance; renderer/model/frame leases are unchanged.

**Native assumptions remain explicit:** the existing native body proxy,
three-grid static queries, flat-floor contact and declared20000 ceiling bound
are not original solid/prop/ceiling collision equivalence. Source effect/boost/
sound callbacks are omitted, not acknowledged as executed. Auxiliary actor
timers whose consumers belong to disabled steering/relaunch branches are not
claimed as integrated. Sloped/attached/offset contact and unreviewed animation
variants refuse. A source-only presentation adjustment fits both actors in the
existing camera at large vertical separations; it changes neither actor heading
nor the normal MVP's grounded camera, and is not recovered camera behavior.

Focused recovery evidence, including initialized-DOL disassembly of677B0:

```
local/recomp-evidence/gred-jump-controller-v1/
```

Relevant source navigation points: upper/lower takeoff `8005F920/8005F978`,
ascent `80061EF0/80061F50`, falling `8005FE04/8005FECC`, landing
`800601D0/80060228`, resource use `800664A0`, contact `800677B0` and the
reviewed landing continuation `80065E64`. These are evidence addresses, never
native call targets or whole-frame execution receipts.

Earlier validation (before the neutral-acceleration correction):600 accepted
updates,237 airborne updates and two landings, including
release, depletion, held input through landing and complete grounded refill.
Eight sanitizer regressions, the host/header build,23 Python recovery tests and
11 opt-in source world/replay tests passed. Targeted tests cover skipped/edited
vertical phases and velocities, no gravity fallback, neutral input refusals,
prior-cache recovery, stale poses, deferred commit and high-ascent camera bounds
in landscape/portrait. The ordinary MVP smoke still reaches64/0; original input
and both existing pack hashes are unchanged.

Contact Sheet14 contains six actual GL captures after the high-ascent framing
fix, generated offscreen without taking classifier/desktop focus:

```
local/recomp-evidence/contact-sheets/source-vertical-jump-v2/contact-sheet-14.png
```

### Earlier dash-only milestone

A separate opt-in lab now connects the recovered ground-dash components to
initialized **native MVP actors** on the real stage. It is not a replacement for
the playable controller or an original actor-constructor/whole-frame receipt.

```sh
build/source-ground-world-host-v3/gf_native --demo-source-ground-dash --no-audio
```

The lab scripts short forward/left/back/right stick deflections, source VM
holds/recovery and source motion proposals. Escape closes it, Enter pauses and Q
restarts. It does **not** implement interactive source walking, jumping, flight
or combat; those requests refuse through the lab API rather than falling back
to the MVP controller.

### Accepted motion and publication

- Both the existing MVP and the source lab now use
  `native_borg_contact.hpp` for the same bounded native terrain/body response:
  arena clipping, swept opponent contact and axis-aligned wall-slide fallback.
  This is explicitly a **native contact proxy**, not recovered original solid/
  body collision.
- The world accepts a source ground offer only for its own current actor/tick/
  producer lease. It preflights contact before mutating the tick, rejects
  airborne exits and rejects source/native floor-height disagreement.
- The accepted receipt records the clipped native position, not the proposed
  endpoint. Source input, VM and handler state are privately staged and committed
  only after the exact offer receives that receipt.
- Once source movement owns the actor, omitting an offer cannot silently resume
  the legacy controller. Explicit world restart is required to return to normal
  MVP mode.
- Source-pose publication requires the matching native receipt, exact renderer
  model-instance binding and a live producer-frame lease. Stale ticks, neighboring
  worlds, model reloads and retired producers cannot replay an old source pose.
  The normal curated-pose path refuses source-owned movement.

The source rig/library is seeded from the same already-evaluated native actor
pose; prepared files alone cannot provide that seed or authorize motion.

### Explicit native assumptions

This lab uses the existing native collider center as the target-hitpoint proxy
for the recovered planar distance/steering calculation. Static query bounds,
the source-free spawn/idle entry and common idle scalar reset are native harness
policy. Original weapon/charge/auxiliary and contact-tail producers are not
acknowledged as executed. Ledges, unsupported callbacks and floor-query
disagreements refuse instead of acquiring a fabricated original frame.

The original three static query grids and all four animation/input/motion
profiles must match the qualified normal G-Red/stage473 inputs. Prepare a
**new** local pack (the shared MVP model/stage art is reused):

```sh
python3 -B -m tools.extract.gred_ground_demo MAIN.dol GRED.gfcs STAGE473.gfnb \
  local/content/source-gred-ground-v1
```

For the current general-ground extension, use a new
`local/content/source-gred-ground-v2` output instead; never overwrite the old
pack. New extraction emits GFMM2; the dash-only mode accepts either revision.

Use `--source-ground-assets DIR` to select a different newly prepared lab pack.
Inputs are read-only and existing output directories refuse overwrites.

Validation: the scripted lab completed720 accepted native updates with56
body-proxy contacts, no projectiles/hits and health100/100. Ownership/contact
refusal tests passed118 checks; the real-stage/model transaction/pose suite
passed1102 checks, including retained-old-model reload rejection and producer
retirement. The native duel, Borg-pose, terrain and classified-locomotion
sanitizer regressions passed. Host/header builds,23 Python recovery tests and
eight opt-in world/replay host tests passed. The normal combat smoke run still
reached a G-Red win at64/0. Prepared-pack and original input hashes were checked,
and pack overwrite refusal was verified.

Contact Sheet11 contains actual GL captures with accepted world movement:

```
local/recomp-evidence/contact-sheets/source-ground-world-v1/contact-sheet-11.png
```

## What has been recovered

The qualified original DOL contains:

- Six local program-group slots, with groups0/1 aliasing the same table.
- 156 group/choice references to **65 distinct animation-program entry points**.
- 149 descriptor references to **71 distinct two-lane descriptors**.
- A separate common program/descriptor namespace, installed on G-Red too.
- G-Red's class callbacks, five-way main override dispatch and normal-model
  substate table.
- A32-entry movement-selector table connecting G-Red to shared upper/lower
  state handlers. Disabled selectors remain explicit.

All65 local programs can now be structurally followed through their terminal
waits. This is **not** a claim that all paths execute, that waits complete, or
that the original controller runs natively. Side effects and external state
changes are not executed by the recovery tool.

## It is not one clip-per-action table

```text
input / target / collision / actor state
                 |
       shared state selection
          /              \
upper/lower movement    G-Red-specific overrides
state handlers          and attack substates
          \              /
          animation-program request
          (group, choice, lane mask,
           start and transition overrides)
                 |
       bytecode interpreter + waits
          /              \
 ordinary descriptor    directional descriptor selection
          \              /
      per-lane clip / mirror / loop /
      model / event / root-control metadata
                 |
   shared motion sampling, blending,
   pose evaluation and root handling
```

**State selector, program choice, descriptor choice and raw clip ID are different
number spaces.** For example, base program8 requests descriptor5, which selects
raw clip9 on both lanes. Treating all of these as “animation8” would be wrong.

Programs also contain controls and timed events, not just clip selection.
Bytecode17 dispatches through G-Red's class-event callback, so an attack's
effects are not recoverable from its body pose alone.

## Concrete base-program links

These are links from original program bytes and descriptor tables, cross-checked
against the saved visual labels. They do not import annotations into runtime.
Groups0/1 alias for these base entries; runtime default-group substitution still
belongs to the original request/selection routines.

| Program choice | Descriptor choice | Raw Borg clip | Observed/recovered role |
|---|---|---|---|
| 0 | 0 | 0 | Idle; authored mode2/duration10 selection |
| 1 | 1 | 1 | Walk; authored mode2/duration10 selection |
| 2/3 | 2 | 6 | Takeoff |
| 4/5 | 3 | 7 | Ascending/air pose |
| 6/7 | 4, then14 | 8, then24 | Apex-to-falling sequence, separated by real waits |
| 8/9 | 5 | 9 | Landing |
| 10 | 10 | 14 | Air-dash wind-up, selector15 |
| 11 | 11 | 23 | Air-dash pose, starting at frame0 |
| 12 | 11 | 23 | Same air-dash pose, starting at frame10, authored mode2/duration4 |
| 13 | 14 | 24 | Falling-pose request |
| 14 | 15 | 14 | Alternate airborne-launch entry, selector8 |
| 27/28/29/30 | 28/29/30/31 | 2/4/3/5 | Forward/left/back/right ground-dash programs |

The apex sequence includes a wait with flags2/threshold9, model metadata work,
another wait, and then the falling-pose selection. The dash programs contain
control changes at authored wait thresholds including5 and19. These are
**animation-program clock operands**, not permission to substitute fixed
physics durations.

Idle/walk scripts have nonterminal waits and physical fallthrough into later
bytes. That is a conditional continuation: a looping motion may never satisfy
the wait. Static traversal must not misreport all later selections as an action's
unconditional sequence.

## Original movement-state organization

The normal G-Red table maps the following selector values to shared state IDs:

| Selector | Shared upper/lower states | Key lower handler / evidence |
|---|---|---|
| 0 | 1/1 | Idle program maintenance, `8005F2A4` |
| 1 | 2/2 | Walk entry/turn updates, `8005F3A0` |
| 4 | 5/5 | Requests takeoff; completion selects5, `8005F978` |
| 5 | 22/22 | G-Red-installed ascent/flight path, `80061F50` |
| 6 | 7/7 | Air continuation selected using actor fields, `8005FECC` |
| 7 | 8/8 | Ground/landing entry and completion, `80060228` |
| 8 | 23/23 | Alternate airborne launch, `8006235C` |
| 12 | 13/13 | Directional ground-dash entry, `80060C48` → `80060C94` |

The dash-entry helper selects program`27 + direction`, reads movement parameters
from G-Red's actor-data resource, and sets a travel heading separately. Subsequent
handlers consume program control bytes and clocks. This is richer than the
MVP's single speed/duration/cooldown burst.

The shared frame routine `80055568` dispatches lower movement first, then the
previously captured upper state, with class hooks around them. That ordering and
the independent lane masks matter when an attack interrupts movement.

## Input admission and resource recovery

The next pass recovered15 focused movement-selection handlers, their direct
call order, ten buffered-input command records and42 typed numeric operands.
Call order is not itself a control-flow graph: the guards/early returns were
inspected separately, including initialized-DOL instructions where decompilation
truncated.

For normal G-Red, class movement type is0 and the installed resource policy is1.
In particular:

- Ground jump requires buffered source input bit1 **and** the held jump/flight
  bit`100`, subject to the actor lock flags. Walking is lower priority than
  ground jumping and directional-dash admission.
- Directional dash admission consumes a buffered direction nibble. The source
  conditioner tests radial stick thresholds20 and52, with a short2/3-update
  initial window and subsequent bounded buffers. It is not the MVP's L button;
  do not describe it as a simple double-tap without the complete input trace.
- Ascent can end when the held jump/flight input is released or the resource is
  exhausted, once its admission field allows the transition.
- Airborne relaunch is admitted through a separate buffered command and
  resource check. The resource policy changes which count-limit checks apply;
  a raw repeat-limit operand does not mean “G-Red gets exactly five jumps.”
- Ground/ledge correction is a higher-priority admission path, not just a
  post-animation visual adjustment.

The ambiguous clip14 is now explained by the original air-dash sequence:

```text
directional air-dash admission
    → selector15: wind-up, program10 / clip14
    → selector16: active dash, program11 / clip23 from frame0
    → selector17: recovery, program12 / clip23 from frame10
    → selector6: airborne/falling continuation
```

The active phase has resource- and input-dependent exits. Its timer is not
universally a forced transition for G-Red's resource policy. It can also be
interrupted by an admitted airborne relaunch. None of this is equivalent to the
MVP's one-dash-per-flight rule.

The original resource contains walk/turn speed slots, turn operands, air
steering/launch values, air-dash values and directional ground-dash blocks.
These values remain in the private typed report, with their offsets and
consumers. They have **not** been copied into MVP configuration: source clocks,
scale/rank, resource consumption, integration and heading conventions must join
correctly first.

## First native animation-VM integration check

`tools/extract/gred_locomotion.py` packages unaltered base-program choices0–31
and their required descriptors into the existing **GFMP3** format. Source
group0/default-group descriptor aliases are retained; no waits, authored
transitions or instructions are rewritten. Attack/event programs are excluded.

The existing shared motion driver and VM accepted that profile with the real
normal G-Red model, rig and13 source clips. A sanitizer test evaluated11 explicit
animation requests over660 VM updates, including the original apex-to-clip24
sequence and the clip14 wind-up/alternate-launch poses. It passed2131 checks.
These are **animation timeline tests**, not input/physics/gameplay actor execution.

Ground-dash requests still refuse under the basic control-flow entry because
their pose-height opcode needs a separately scoped consumer. That entry and its
refusal are unchanged.

The explicit height-control entry now evaluates all four original ground-dash
programs with the shared scope-bound consumer and source-extracted GFHC operands.
For the declared grounded G-Red inspection state, the source branch returns
zero height offset without world-bone observations. Opcode8 and its operand1
remain intact. The tests also verify the source frame5/frame19 control changes,
directional clip selection, separate upper/lower requests and foreign/reloaded
scope refusal. The expanded real-model sanitizer test passed2678 checks across
17 explicit requests and910 VM updates. These remain animation/declared-field
tests, not live gameplay state production.

### Renderer replay

An opt-in **source program replay** is now part of the existing animation mosaic.
It uses the same model scene, shared VM/motion driver, pose/skin and GL renderer,
not a parallel render engine. It shows14 selected movement programs, with the
four ground dashes first, and labels program identity, inspection tick and lower
control phase. It repeats a96-tick inspection window; that loop is not an
original game-state transition.

```sh
build/gred-source-replay-host-v2/gf_native --animation-mosaic \
  --animation-programs local/recomp-evidence/gred-movement-admission-v1/locomotion.gfmp \
  --animation-height-profile local/recomp-evidence/gred-movement-admission-v1/pose-height.gfhc \
  --animation-grid 2 2 --size 1152 800 --no-audio
```

Left/right changes page, Space pauses, Q/E changes speed and Z rotates the
inspection view. This mode refuses Sasuke, edited source-profile payloads and
raw-classifier export. The original clip classifier and its annotations are
separate and unchanged. No additional manual classification is requested.

Prepare the additional source operand profile using a **new** output path:

```sh
python3 -B -m tools.extract.gred_locomotion MAIN.dol \
  local/recomp-evidence/new-source-inputs/pose-height.gfhc --kind pose-height
```

This28-byte GFHC file preserves the source zero/half/negative-factor/gain/step
operands and is consumed by the existing strict reader.

Validation for this pass: the host/header build and source-VM, height-control,
height-refresh, control-flow, animation-catalog and existing MVP-pose sanitizer
checks passed. Four opt-in host tests cover every source-replay page and wrap,
the unchanged raw classifier route, incompatible options and edited-profile
refusals. The recovery tools passed14 Python tests. Ground-dash poses were also
captured through actual GL at inspection ticks4/12/22; the replay camera has
extra display margin for limbs extending beyond the initial stance.

```
local/recomp-evidence/contact-sheets/source-ground-dash-v2/contact-sheet-09.png
```

Additional private recovery outputs:

```
local/recomp-evidence/gred-movement-admission-v1/movement-v2.json
local/recomp-evidence/gred-movement-admission-v1/movement-layout-v2.md
local/recomp-evidence/gred-movement-admission-v1/locomotion.gfmp
local/recomp-evidence/gred-movement-admission-v1/pose-height.gfhc
local/recomp-evidence/gred-movement-admission-v1/*.asm
```

The movement report requires the qualified original DOL and the existing
normal-G-Red GFCS setup profile; both are opened read-only and checked against
their identities. Outputs refuse overwrites. Thirteen Python tests cover the
controller/movement recovery tools, including synthetic operand decoding,
relative/absolute PPC calls, unaltered GFMP packaging and refusal boundaries.

## Provenance and local inspection

Source revision SHA-256:
`21454a8880c6254616662710d6ec626e43d853999da2cdfad700030960a94836`.

Key provenance addresses (never native call targets):

- Class-field installer: `8018CCFC`; normal config: `80365AC8`.
- Local program/descriptor roots: `80366220` / `80367460`.
- Common program/descriptor roots: `802BCC64` / `802BB698`.
- Shared request/interpreter/update: `8004BEB8` / `8004C050` / `8004CD24`.
- G-Red main dispatch: `8018D24C`, table`80365588`.
- Class bytecode-event dispatch: `8018CFF0`, table`80365578`.

Full reports, decompiler hints and bounded original-instruction views are private:

```
local/recomp-evidence/gred-animation-controller-v1/controller-v3.json
local/recomp-evidence/gred-animation-controller-v1/controller-layout-v3.md
local/recomp-evidence/gred-animation-controller-v1/shared/
local/recomp-evidence/gred-animation-controller-v1/*.asm
```

The DOL hash and initialized-section boundaries qualify data extraction. Decompiled
text is only a navigation hint: some functions truncate at paired-single
instructions, and its stored baseline identity hash is not a raw DOL-span digest.
Class installation and shared-frame ordering were checked against initialized
DOL instructions as well as those hints.

Recreate a report with a **new** output path:

```sh
python3 -B -m tools.analyze.gred_animation_controller MAIN.dol \
  local/recomp-evidence/gred-controller-new.json
python3 -B -m tools.analyze.gred_animation_controller MAIN.dol \
  local/recomp-evidence/gred-controller-new.md --format markdown
```

The tool is bounded, read-only on its input, revision-specific and refuses output
overwrites. It does not read or modify `annotations.json`.

## Remaining boundaries

- Structural bytecode decoding is not actor-state-machine execution. Event
  callbacks, input admission, target selection, collision responses and resource
  lifetimes still need explicit consumers.
- The common inventory has19 unsupported bytecode-entry views beginning with
  opcode255; they are retained explicitly, not fabricated as idle. This is
  separate from the classifier's14 unsupported skeletal previews.
- Native compatibility VM execution refusals are unchanged. The analyzer knowing
  an event's size does not authorize the runtime to execute it.
- The playable MVP still uses its labelled approximate controller. No original
  program or recovered parameter has silently replaced its movement policy.

Next: recover the remaining special dash/relaunch steep, offset/attached/ledge/solid contact responses
and blocked-lane/weapon input rules with accepted contact/resource history.
The surface lab now qualifies static angle-gated landing and verifies normal
takeoff's non-cancellation rule; it does not authorize those remaining branches.
The entered-landing path joins landing cancellation
and reviewed ground-dash interruption with
the normal policy1 relaunch and air-dash
subsets; this does not authorize the remaining
branches. Source walk/turn/stop and native-device mapping remain in the
separate ground lab; ordinary stop completion uses the existing driver's blend
state rather than looping-idle completion.
Keep weapon/event and
original whole-frame branches explicit. Do not expand this into whole-game
recovery or require further bulk manual classification. Saved visual labels are
cross-checks; further manual work should answer specific unresolved questions.
