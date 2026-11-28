# Borg API Battle Test, version 1

This is an executable package API for a bounded two-actor native test, not
retail combat admission. It reuses the native battle world's movement/contact,
projectile pool, damage, model, animation and renderer. It doesn't use saves,
Story rosters or the solo controls demo. Packages declare intent and assets;
they do not supply executable code, slot numbers, engine addresses or world
mutations. Audit and catalogue sidecars are not executable.

## Try it

To test without game files, use the original
[Flight Stick and Jump Stick examples](demo-borgs.md).

```sh
# New output folders only. If G-Red is already installed, retain the old
# package outside borgs/ before installing this build of the same stable ID.
python3 -B -m tools.extract.portable_borg_preview g-red borgs/g-red-api \
    --practice local/content/source-gred-ground-v9 --battle-test
python3 -B -m tools.extract.portable_borg_preview arc-scout borgs/arc-scout
python3 -B -m tools.extract.portable_borg_preview grapple-scout borgs/grapple-scout
python3 -B -m tools.extract.portable_borg_preview charge-scout borgs/charge-scout
python3 -B -m tools.extract.portable_borg_preview blade-scout borgs/blade-scout
python3 -B -m tools.extract.portable_borg_preview rotary-scout borgs/rotary-scout
python3 -B -m tools.extract.portable_borg_preview bomb-scout borgs/bomb-scout
python3 -B -m tools.extract.portable_borg_preview remote-scout borgs/remote-scout
python3 -B -m tools.extract.portable_borg_preview seeker-scout borgs/seeker-scout
python3 -B -m tools.extract.portable_borg_preview guard-scout borgs/guard-scout
python3 -B -m tools.extract.portable_borg_preview impact-scout borgs/impact-scout
python3 -B -m tools.extract.portable_borg_preview wingmate-scout borgs/wingmate-scout
python3 -B -m tools.extract.portable_borg_preview rewind-scout borgs/rewind-scout
python3 -B -m tools.extract.portable_borg_preview wire-trail-scout borgs/wire-trail-scout
python3 -B -m tools.extract.portable_borg_preview anchor-arrow-scout borgs/anchor-arrow-scout
python3 -B -m tools.extract.portable_borg_preview target borgs/training-target
# Optional Sasuke test package (same duplicate-ID rule as G-Red):
python3 -B -m tools.extract.portable_borg_preview sasuke borgs/sasuke-api --battle-test
build/direct/gf_native --battle-borg gg4e08:0615 --against-borg test:target
build/direct/gf_native --battle-borg test:arc-scout --against-borg test:target \
    --demo-package-battle --trace-package-battle --headless
build/direct/gf_native --battle-borg test:grapple-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:blade-scout --against-borg test:target \
    --package-battle-distance 90 --trace-package-battle
build/direct/gf_native --battle-borg test:rotary-scout --against-borg test:target \
    --demo-package-battle --trace-package-battle --headless
build/direct/gf_native --battle-borg test:bomb-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:remote-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:seeker-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:guard-scout --against-borg test:seeker-scout \
    --trace-package-battle
build/direct/gf_native --battle-borg test:impact-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:wingmate-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:rewind-scout --against-borg test:target \
    --trace-package-battle
build/direct/gf_native --battle-borg test:wire-trail-scout --against-borg test:arc-scout \
    --trace-package-battle
build/direct/gf_native --battle-borg test:anchor-arrow-scout --against-borg test:arc-scout \
    --trace-package-battle
```

For a reproducible host check with the prepared local assets:

```sh
PYTHONPATH=. python3 -B tests/host/borg_api_smoke.py build/direct/gf_native
```

Add `--opengl` to use the desktop renderer. The smoke test builds temporary
packages and gives the battle host a stage directory containing no loose Borg
art, so it cannot pass by reading the old duel's model and motion files.
To check only the installed normal G-Red/Sasuke socket and movement changes,
run `python3 -B tests/host/borg_presentation_smoke.py build/direct/gf_native`.

The stage is shared with the existing native duel (`--battle-mvp-assets`).
Normal G-Red also uses that stage's qualified grids (`--source-ground-assets`,
default `local/content/source-gred-ground-v9`). G-Red uses his existing
qualified movement subset and a **test-authored** B attack. Its release
ordering and stock are informed by recovered neutral B. It uses the
recovered key-7 local muzzle offset, but its raw timeline, aiming, flight,
collision and damage are not the verified retail beam transaction.
Other G-Red attacks are not approximated or filled in.
Source G-Red is allowed only as the first actor against a declared stationary
target. This preserves the movement adapter's unsupported hit/other-actor
refusals. Opt-in Sasuke uses the authored grounded component and one paid
energy shuriken per release. Holding B can repeat after a check-only stock gate,
with a bounded five-cycle cap and a separate debit at each release marker.
Close B takes precedence when its range gate passes, including in Sasuke's
test package. Those choices are informed by the recovered B path, but
timing, movement, pose, geometry and contact are authored test rules. No
charged B or X is supplied for Sasuke. The default
Sasuke export remains preview-only. Arc Scout and Training Target use local art as test stand-ins and
have no retail ID or custom C++ callback. Arc Scout's X is a paid pair of
ballistic shots with a recipient modifier; it is not Revolver Gunman X.
Grapple Scout's X queues a caster pull after a real hit. Charge Scout taps
or holds X to select different attacks on release; the quick shot spends
stock, and the charged attack is explicitly stockless. An authored grounded Borg can also
fight another authored grounded Borg. Blade Scout's B chooses a close cut
or a ranged shot. `--package-battle-distance` changes the initial separation
in the native test encounter; it doesn't declare retail spawn rules.

Keyboard: arrows move, Shift jumps, Space is gameplay B, E is gameplay X,
Q restarts and Enter pauses. G-Red uses WASD/stick flicks for dash. Authored
grounded movement uses the MVP controller's L/left trigger dash input.
The scripted test runs at least one hit on either side of a restart and
checks movement and airborne state. `--trace-package-battle` logs the
round, tick, selected move, marker, debit, object result, recipient and
retirement. `--borg-variant` and `--against-variant` can request an exact
variant; this version only accepts the profile's declared base variant.

### Choose a roster

Add `--roster-borg TEAM ID` once per CPU fighter, with team `0` or `1`.
`--battle-borg` and `--against-borg` are the two local players on teams 0
and 1. Each team can have up to two additional fighters. For example,
using locally installed packages:

```sh
gf_native --battle-borg test:arc-scout --against-borg test:blade-scout \
  --roster-borg 0 test:grapple-scout --roster-borg 1 test:seeker-scout \
  --roster-borg 0 test:guard-scout --roster-borg 1 test:wingmate-scout
```

For a reusable roster on an authored stage, use
[`--battle-roster FILE`](portable-content-v1.md#reusable-battle-test-rosters)
instead of listing the fighters and spawn markers on every launch.
Use `--list-battle-rosters DIRECTORY` to discover presets and
`--check-battle-roster FILE` to check startup without playing. Use
`--browse-battle-rosters DIRECTORY` to pick and launch a preset in the desktop window.
Presets also support local co-op: set both local rows to team `0` and
add at least one CPU on team `1`. Set `locals=1` to play without a
second local player; only the first fighter is then under local control.
Set `rounds-to-win=2` for a first-to-two
match instead of playing isolated rounds. Add `round-seconds=45` for a
round timer; expiry compares the teams' remaining HP fractions.

CPU fighters use the package's move range and timing. They close in for
short-range attacks and prefer a ready B attack. They can use X when B
isn't ready, including while B's stock refills. They check entry stock
and the first release cost, and release held fire when that weapon empties.
They can hold for a charge and take a simple jump for air-only attacks.
For direct melee they also check the target's height. They can hop to
meet a reachable high target if the move is allowed in the air, but do
not try to jump to unreachable targets or turn a grounded move into an
aerial one. This is a planning estimate; contact is still resolved at
the move's actual active frames.
The normal scheduler still owns move selection, ammo, release timing,
and contact. In this native test AI, short-range fighters with ordinary movement can go around
authored partitions or turn toward a different enemy with a clear
approach. This changes their aim, not which body an attack actually
hits. They do not plan around fighters or floor hazards.
The planner does not reproduce retail tactics or tap combos.

Both local players move relative to the same resizable camera. It fits
each living fighter's posed body and collision proxy without rotating
when an aim target changes. Aim labels sit above the visible body.
Health and KO status are grouped by team; very small windows show
surviving fighters per team instead.
Authored obstacle fades in this shared view account for both living
local players, not just the player in the first slot.

The roster HUD shows ammo for each living local player. These are live
stock-pool counts, not a count of remaining attacks. B/X means that
both buttons reference the same pool. `REFILL` means its timed clock
is active; `RETURN` marks a depleted return-only pool, without
predicting when or whether it comes back. Attacks that do not reference
a stock pool show no counter. The HUD can omit extra rows in a small
window; `--trace-package-battle` reports local changes as `borg-ammo`.

Configured movement resources appear beside ammo: `LIFT` (remaining
powered-lift ticks), `JUMPS` (including takeoff), `AIR ACTIONS` (a shared
jump-and-dash budget), or `BOOST` (cruise boost units). These readings
come from the live native actor; they do not predict whether the next
input will be accepted. Fighters without one of these authored resources
get no movement counter. The trace reports local changes as `borg-mobility`.

P2 moves with I/J/K/;, uses H to jump, M to dash, comma to strafe, U
for B and O for X. N restarts, and keypad Enter pauses.
`--package-battle-distance` adjusts separation between team columns.

Aim starts on the nearest opponent. Press the controller's **right
shoulder** (P1 keyboard **Z**, P2 **.**) to cycle through living opponents
in slot order. The `P1 AIM` and `P2 AIM` labels show each player's current
selection. When a target is defeated, aim returns to the nearest living
opponent. Restarting clears both selections. Cycling does not turn the
body, retarget a committed launch or an in-flight shot, or override
physical contact. If co-op players aim at the same fighter, both
labels are shown above it.

Explicit rosters require native package movement. Use the standalone arena
(`--battle-mvp-assets`) or an [authored stage](portable-content-v1.md#stages).
With an authored stage, supply `--borg-variant` and `--against-variant`
for the local players and one distinct `--stage-spawns` marker per actor:
P1, P2, then CPU fighters in `--roster-borg` order. Stage markers replace
the default team columns and cannot be combined with
`--package-battle-distance`. Additional packages use their default variants.
If using indexed damage, supply one rank per actor in the same slot order.
An optional third group in `--package-damage-indices` supplies one damage
table bank (`0` or `1`) per actor in that order:
`--package-damage-indices 7,7:0,0,0:0,0,1`. It does not change teams.
In a preset, `damage-indices=7,7:0,0,0:0,0,1` stores the same selection
for both direct and browser launches. Supply one source, not both.
Explicit rosters do not combine with the mirrored `--demo-roster-battle`,
battle picker, or borrowed source-motion actors.

## Profile

In a `GFBORG 2` manifest declare `gameplay=gameplay.txt`. The file is optional:
its absence doesn't change discovery, previews, ownership or legacy practice.
This slice uses HSD body and motion assets through the existing shared
sampler. It requires an exact, compatible rig and clips, with no fallback.
For an authored profile, optional `body-record=0` declares that only archive
record zero is the actor's body. The body root must match the declared rig.
Other records are not evaluated as body skin or drawn; separately declared
parts still have their own validation. The selected body keeps the strict
material and skin rules: this does not enable normal repair or shape
deformation. Omit the field to keep legacy archive-wide validation. The
field cannot override the exact G-Red binder.
Optional `body-material=immutable-highlight` admits exactly the reviewed
immutable highlight stage (`flags=0x00030082`, `src=4`, identity texture
matrix, weight one, no TEV). Those meshes sample the original GX
`STQ=(1,1,0)` path at UV `(0.5,0.5)`; the draw code is shared with the
inspection preview. It does not admit other mode-2 stages, generated
coordinates outside that signature, or the general preview relaxation.
Optional `idle-loop=authored-bit` and `walk-loop=authored-bit` admit
their respective clips when every animated joint carries only the supported
AObj loop bit and shares a single end frame. The native locomotion clock
still samples the bounded clip; it does not execute the source AObj
lifecycle. Unnamed roles and action clips remain strict. The walk option
requires animated grounded locomotion, not a cruise-entry or source adapter.
An animated grounded package can declare `takeoff-clip=group:choice`,
`landing-clip=group:choice`, `dash-back=group:choice`, `dash-left=group:choice`,
`dash-right=group:choice`, and `air-dash=group:choice|start frame`.
These are exact local clips and must pass the strict loader. The air start
is an in-clip presentation offset, not a source-scheduled launch. Declare
`air-dash=none` to refuse airborne bursts while leaving ground dash enabled.
Omitting the new keys preserves older authored profiles.
For `grounded-v1` with `cruise-boost`, `body-motion=static-cruise` explicitly
binds a static rig. Such a package declares no motion bank or locomotion clips,
and uses `none` in each move's pose field. The usual move markers, collision
and sockets still run. Action pose offsets and animated/decorative part
bindings are not supported in this scope. This policy never substitutes a
static pose for a failed animated binding.
Alternatively, `cruise-entry=GROUP:CHOICE` binds one strict whole-body
motion-bank clip with `cruise-boost`. It replaces the four ordinary
locomotion rows and is mutually exclusive with `body-motion=static-cruise`.
The clip plays once per spawn and holds its final pose through cruise,
boost, and attacks. Moves in this scope use `airborne` and `none`, with
the same paid-release restrictions as other locomotion-preserving moves.
Draw and launch sockets sample the same pose; no action or boost clip is
substituted.
`body-motion=static-grounded` binds a motion-bank-free body to
`grounded-v1` without cruise. It takes no locomotion clips and permits
only grounded `none` moves under the paid-projectile constraints below.
The authored controller still handles travel, jump and dash; the body
does not need a clip to change position.
An authored `grounded-v1` move may also use `none` to keep its ordinary
locomotion pose while attempting a paid projectile release. This is
limited to paid releases without melee, held fire, deployment, fields,
guards, anchor markers, detonation, or action/decorative pose changes.
The package must still supply its exact four locomotion clips.

```text
GFBORG-GAMEPLAY 1
variant=normal
health=180
movement=grounded-v1|7|13|14
rig-prefix=visual/body.prefix
pose-profile=visual/body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
stock=arc-stock|2|150
projectile=arc|bullet|2|52|5|0.5|0.06|8|100|12|stop-on-floor
on-hit=arc|walk-speed|60|30
move=arc-twin|x|20|grounded|24|0:2
release=arc-twin|6|arc-stock|1|arc
```

- `grounded-v1|walk speed|jump speed|dash speed` is a configurable native
  movement component, not a recovered retail controller. `stationary-v1`
  uses just `idle` and no moves or stock. `normal-gred-gfmm9` must bind the
  exact qualified normal G-Red practice assets and supplies its own locomotion.
- `dash-policy=disabled` is an explicit `grounded-v1` policy for animated,
  non-cruise bodies. Omit `dash` and bind exact `idle`, `walk` and `air`
  clips. Both ground and airborne dash inputs are refused; no other
  animation is substituted. Walking, jumping and move admission are
  unchanged.
- `cruise-boost=height|capacity|cost per tick|refill per tick|delay|minimum boost ticks`
  opts `grounded-v1` into the authored continuous-flight pilot instead of
  ordinary jumping. Cruise uses walk speed, travelling forward when there
  is no steering input; a fresh jump-button press starts boost at dash
  speed while meter is available. Release or depletion exits after the
  minimum boost visits. At zero meter an unfinished latch stays active
  but grants no boost speed; held input doesn't automatically start a
  new boost. Refill uses the previous boost state, not floor contact:
  delay-to-zero does not also refill. The pilot maintains the declared
  offset from accepted floor support and never publishes a landing.
   Ordinary locomotion uses `air`/`dash` clips for cruise/boost; the entry
   and static policies above use one shared pose. This does not reproduce retail
  vertical controls, landing callbacks, glide, or aircraft trajectories.
- `pose-ground-offset=<units>` (integer, `-1000..1000`) shifts the drawn body
  on Y and nothing else. The shift is applied inside the pose evaluation, so
  bounds, the camera fit and the draw all agree, while the actor's position and
  collision stay where they were. It exists for rigs whose authored bind pose
  floats above their root; the value is an authored presentation correction, not
  a recovered source height. Omit it and the pose is unchanged.
- `stock=id|capacity|refill ticks`: fills on the last debit's eligible timer
  expiry. Declare `on-return` instead of a number for a pool without a
  timer. Capacity is 1–255 units; individual debit fields keep their
  existing limits. Neither policy implies a refund on failed allocation.
- `stock-clock=id|add-on-debit`: opt a timed pool into a clock that
  starts on the first one-unit debit and adds one refill interval per
  subsequent debit. Stock returns as the clock crosses the interval
  boundaries, with full capacity at zero. Check-only gates never
  add time; a failed projectile allocation doesn't undo an accepted
  debit. The clock uses native simulation ticks, not recovered
  per-Borg source update rates. Return-only pools and multi-unit
  debits are not supported by this clock.
- `entry-stock=move|stock|minimum` adds a read-only stock condition
  to move selection (including held repeat re-entry). If the check
  refuses, a lower-priority eligible move can still start. It does
  not reserve units or change the timer. The selected move's later
  release still checks and pays its own cost; it does not select a
  new move when that payment fails.
- `release-aim-gate=move|release frame|half angle radians|timeout ticks`
  holds one release marker until the current visible target is within
  the declared angle of the accepted body heading, or until the wait
  expires. A button release cannot replace the marker. The clock
  starts at the marker; the move holds that pose frame while waiting.
  Damage interruption cancels payment and emission, while movement
  and other actors continue. This pilot is for moves with a single
  release marker and no co-scheduled mechanics, not a retail VM
  instruction or a native frame inferred from a source visit.
- `projectile=id|visual|count|planar speed|initial Y speed|gravity|spread
  radians|radius|lifetime ticks|damage|floor policy`. `visual` (`bullet` or
  `beam`) selects only the shared test rendering. `ignore-floor` skips the
  floor check; `stop-on-floor` retires at contact, and `land-on-floor`
  keeps the supported carrier in place. Both use the native floor proxy,
  not a general sweep against all stage geometry. Missing floor isn't
  replaced with a hit. For static walls as well as floors, use
  `stop-on-stage` with the qualified stage-segment service.
  The bounded duel aims horizontally at the other actor from a native
  root-relative muzzle proxy; initial Y speed comes from the projectile.
  The hit still needs
  actual swept contact. A volley uses simultaneous, all-or-none allocation.
- `origin=projectile|bone|local X|local Y|local Z|fixed` binds a logical
  rig bone and a local emission offset. It is sampled on the accepted
  action frame at the accepted simulation root, not from a previous draw.
  `target-height` instead of `fixed` adds a native initial pitch toward
  the opponent's body centre. Neither setting decides the recipient.
  Invalid bone bindings fail at load; an unavailable pose refuses a
  paid emission without substituting the root. Unbound projectiles retain
  the existing root origin. Emitter volleys still use their own positions.
- `launch-sound=projectile|path` binds a local stereo 48 kHz signed 16-bit
  little-endian PCM sample (up to three seconds). A successful direct
  action release starts one sample per accepted volley, including each
  separately accepted follow-up. Refused shots are silent. This is not
  a marker or debit sound, and doesn't run on detached emitter or child
  updates. The native SFX mixer uses bounded voices and drops excess
  requests without changing the simulation. Restart discards old tails.
- `release-sound=move|frame|path` binds the same PCM format to a specific
  paid release marker. It starts once after a successful debit, before
  attempting allocation. It can therefore play when a paid shot fails
  to spawn; it never plays on a refused debit. One actor cue covers a
  marker with follow-up shots. It neither replaces nor implies any
  `launch-sound` bindings.
- `launch-heading=projectile|body` uses the accepted body heading at
  release instead of the tracked or committed target direction. It
  still samples a declared `fixed` socket for position. Target-height
  sockets, sustained beams, and child or emitter projectiles are
  refused with this setting. It does
  not change presentation aim, steering after launch, or contact.
- `aim-lock=move|frame` freezes direct release aim at an accepted
  action frame, before any release on the same frame. Frame `0` locks
  on entry. Presentation uses the stored heading while the action
  is active; a socket still comes from the actual release pose.
  `target-height` uses a frozen centre-height slope, not a late
  retarget from the socket. A move without this binding keeps its
  existing aim behavior. Completion, interruption and each accepted
  repeat clear the lock; a new cycle can commit again. This does not
  steer the Borg, make a hit inevitable, or change the selected wall
  policy. Direct marker projectiles using guidance, held-fire actions,
  and melee moves cannot use this binding. Following emitters retain
  their own targeting policy.
- `look=projectile|size|R|G|B` is independent of launch and collision
  radius. In addition to `bullet` and `beam`, `shuriken` renders a
  spinning four-blade energy shape. `pose-start=move|frame` starts the
  action's visual clip at an authored frame without changing its marker
  clock. G-Red and Sasuke use recovered key-7 local offsets; Sasuke
  uses the recovered upper `3:0` throw entry at frame 7. The port's
  launch timing, aiming and rendered stars are not exact retail playback.
- `pose-loop-window=move|authored-bit` accepts the uniform authored loop
  flag for one explicitly named action on a grounded record-zero body.
  Its native sample window must fit within the exact clip: it never wraps
  or runs the source AObj lifecycle. On a move with a pose follow-up this
  flag applies only to the initial clip. Mixed flags and other unsupported
  metadata still refuse.
- `pose-followup=move|switch tick|group:choice|start frame` plays one
  second exact local clip on a grounded, animated melee-only move. The
  switch must be after tick 0 and no later than the first melee-active
  tick. Before the switch the lead-in uses the ordinary `pose-start`
  clock; from the switch the second clip starts at `start frame`. Both
  assigned windows must fit, and the second clip uses strict metadata
  validation. No missing-clip substitution, loop, or blend is implied.
  This changes presentation only: the move, hitbox, activation, and
  damage clocks are unchanged.
- `projectile-record=projectile|record|scale percent` replaces an `orb`'s
  procedural appearance with a static drawable record from the owning
  package's model archive. Record indices are 1–31 and scale is 1–400%.
  It is mutually exclusive with `look` and `projectile-animation`.
  The shared renderer positions the record at the accepted projectile
  location and applies the authored scale without changing collision,
  aim or simulation. Missing, empty or unsupported records refuse package
  binding rather than falling back to an orb. The record carries no
  scripted effects, animation clock or Dragon-specific behavior.
- `projectile-record-trail=projectile|record|count|interval|scale percent`
  optionally draws another exact static record behind a `projectile-record`
  shot. It retains 1–5 previous positions on the projectile, sampled
  every 1–30 accepted ticks while it moves, and draws oldest to newest.
  Scale is 1–400%; missing or empty trail art refuses binding. History
  expires with the projectile and is never a child allocation, collision
  object, or independent effect clock.
- `impact-record-bank=path` and
  `impact-record=projectile|actor/floor/stage|record|scale percent|ticks`
  add a visual-only static record from a package-local archive. Record
  indices are 1–255, scale is 1–400%, and lifetime is 1–120 ticks.
  The lane must be reachable by the projectile. Only accepted terminal
  contact admits a draw at the final position; blocks, expiry and
  refused terrain queries do not. The pool holds 16 visuals and drops
  new submissions if full. Restart clears it. The visual adds no sound,
  collider, damage, child projectile or stock effect on its own.
- `impact-record-growth=projectile|end scale percent` optionally
  interpolates the impact record from its declared scale to the end
  scale (1–400%) over its visible ticks, multiplying its material alpha
  by the remaining lifetime fraction. The record must last at least
  two ticks and use source-alpha blending without depth writes. A record
  that cannot use the fade refuses binding; fixed appearances are unchanged.
- `impact-sound=projectile|actor/floor/stage|path` admits one package-local
  stereo 48 kHz signed-16 little-endian cue on the selected accepted
  terminal contact. It does not require an impact visual and never fires
  on blocks, misses, expiry, or refused terrain. Impact clips may last
  up to four seconds; launch, release and movement clips keep their
  three-second bounds. The shared SFX mixer owns playback; mute does not
  change simulation events, and restart clears active voices.
- `projectile-animation=projectile|spin radians per tick|pulse period|pulse percent`
  animates the rendered projectile on accepted simulation ticks since
  allocation, independent of its speed, gravity, and collision radius.
  `shuriken` and `orb` have visible rotation; other styles can pulse
  without spin. A positive pulse period (up to 240 ticks) grows the
  display size to its authored peak and back. Use zero period and
  zero percent for no pulse. Existing shuriken bindings without an
  animation row retain the original test spin.
- `part-spin=move|bone|axis|period|forward` adds a cosmetic local-axis
  spin while the move is active. Use `reverse` for the opposite direction.
  `part-motion=move|bone|group:clip|first frame|length` instead reads a
  loop from an authored clip for a specific decorative bone. It replaces
  that bone's render-local pose, and any declared spin adds on top. Both
  start with the action, not the first drawn frame. They are excluded from
  authoritative socket/FX matrices, body heading, and hit geometry. This
  supports a mounted spinner or independently rigged moving teeth.
- `part-scroll=move|bone|u|period|forward` cycles the U coordinate of
  eligible meshes owned by that bone during the action. Use `v` for
  the other axis and `reverse` for negative motion. The period is 2–240
  simulation ticks per repeat; the mesh must have explicit coordinates,
  an identity texture transform, and a repeating texture on that axis.
  Reflection, clamp/mirror and texture matrices are not scroll targets.
  A move can have up to eight
  decorative bindings total, and a scroll can run alongside bone motion.
  This is an authored presentation layer, not a retail material-script import.
  See the [retail attack-visual audit](borg-attack-visual-recovery.md) for
  the currently unbound exact cases.
- `part-record=slot|bone|default record` binds a named cosmetic slot to
  a logical bone. `part-swap=move|slot|first frame|last frame|record`
  selects another published rigid model record for that slot, in an
  inclusive window of the accepted action's frame clock. Outside the
  window (including on interruption), the slot uses its default again.
  `0` hides the slot; it does not select the body. Records are in the
  package model file and use shared model validation. Up to four slots
  and twelve prepared records are supported (material instances count
  separately per slot); a move can override each slot once. The slot
  follows rendered pose, including decorative motion, without
  publishing a socket, collider or child actor.
- `part-pose=slot|x|y|z|rx|ry|rz` places the record relative to its
  bone. `part-swap-pose=move|slot|x|y|z|rx|ry|rz` overrides that
  placement during the corresponding visible swap; otherwise a swap
  keeps the default placement. Offsets are local units (±1000) and
  rotations are degrees (±180). The renderer keeps the bone basis,
  transforms the offset through it, then concatenates local X, Y and
  Z rotations in that order. It does not modify the parent pose.
- `part-basis=slot|world-zyx` keeps the mount point in the bone's
  local space, but uses world axes and unit scale for the record.
  It applies the configured rotations as `Rz * Ry * Rx`, without
  inheriting bone rotation or scale. The default uses the bone basis.
  `part-basis=slot|bone-zyx` keeps the bone basis but changes the local
  rotation order to `Rz * Ry * Rx`.
- `trailing-chain=bone|bone|bone[...]` gives a grounded record-zero body
  a native secondary-motion chain. It must name a terminal rig branch
  in parent-to-child order (3–8 distinct non-root bones, no side branches).
  The first joint follows the posed anchor; the remaining joints retain
  inertia and test their own static-floor heights while keeping the posed
  link lengths. Only the skinned presentation changes. The chain cannot
  change body height, landed state, hitboxes or launch sockets.
- `animated-translation-percent=25..200` (default 100) retargets only
  keyed local translations on an animated, grounded record-zero body.
  Rotations, unkeyed bind translations, model geometry, the world root,
  ground contact and animation timing are unchanged. Render poses and
  socket queries both use the retargeted animation.
- `body-mount=record|bone|x|y|z` explicitly assembles a record-zero body
  from static same-bank rigid subtrees. At most 16 mounts are allowed;
  each record/bone pair must be unique. The root uses the named logical
  bone and signed local translation (±4096); descendants keep their
  own transforms. This does not enable inspection-only materials,
  independently scheduled children, or additional colliders.
- `zero-reflection=hex-pobj-offset|count` opts a record-zero battle body
  into neutral `(0.5,0.5)` reflection coordinates for exact authored
  zero normals on an already supported mode-1 mesh. Up to four distinct
  meshes may be declared; each archive-relative PObj and exact decoded
  count (1–64) must match at bind time. Nonzero
  normals use the original path; the source normals, mesh data, and
  texture are retained. This is a native rendering policy, not a GX
  zero-normal equivalence claim or a general material fallback.
- `part-hierarchy=record` explicitly uses the shared static hierarchy
  path for a visible default part instead of the single-joint rigid
  binder. The exact record must qualify, and the same record cannot
  also declare a material clip in this mode. This does not instantiate a
  source child or enable its animation, collision or logic.
- `part-mount-spin=slot|x/y/z|period ticks|forward/reverse` rotates a
  visible mount about a local axis on the accepted round clock. The
  period is 2–240 ticks. It changes rendering only; sampled sockets and
  parent pose are unchanged.
- `part-swap-bone=move|slot|bone` changes the mount during the same
  visible swap window. Use it for a back-to-hand weapon transfer or
  to replace a hand-held part with a shoulder-mounted one. Outside
  the window or on interruption the default bone returns. This
  does not retarget a projectile origin or enable an emitter;
  those remain separate move bindings.
- `part-turn=slot|x step|y step|z step` limits each local angle's
  approach toward the current placement, in binary angle units
  (65536 per revolution, maximum step 8192). Zero snaps that axis.
  The angle survives a visible record swap and moves by at most one
  step per new presented simulation tick, not per draw. A hidden slot
  or reset starts turning from neutral next time. Skipped updates
  are not replayed; this is presentation, not a gameplay clock.
- `part-material=record|clip|first frame|last frame|loop` opts a used
  part record into its published material clip. `once` holds the last
  sample instead of looping. The inclusive window has 2–240 samples.
  Use `sample` with equal first and last frames for a fixed material
  selector (it never advances).
  Time starts at the first visible simulation update of that slot and
  record, and restarts if that slot changes away and comes back.
  Redrawing the same update does not advance the clip. Instances on
  different slots have separate clocks even when they use the same
  record. This uses the shared MObj/TObj evaluator, not a source child
  lifecycle or a joint-animation override.
- `on-hit=projectile|walk-speed|percent|recipient updates` is optional.
  This walks through the accepted hit's actual recipient, never through
  aim. A surviving `grounded-v1` recipient takes it; unsupported movement
  components refuse it. It scales ordinary planar movement, not dash,
  jumping, input conditioning or animation clocks. Latest accepted effect
  replaces and refreshes the old one; expiry restores the base speed.
- `on-hit=projectile|pull-self|units` is another optional hit effect for a
  `grounded-v1` caster. The hit captures the actual recipient's X/Z at contact
  and stages at most one pull per caster per update. The next unpaused update
  asks the movement owner for one planar step toward that point (1–40 units).
  It doesn't track the target, move the recipient, turn the caster, or carry
  extra velocity into the next update. It takes the ordinary movement visit
  when it applies; a fresh jump, existing dash, landing, or hit lock refuses it.
  This is a short native flat-floor policy with midpoint/end support samples
  and a swept body check, not general wall or terrain traversal. Unknown
  support, height changes, body contact, or arena bounds refuse the entire pull.
  Older pending work never applies to a replacement round. A miss queues
  nothing. A projectile may declare both hit effects; neither is the other
  effect's precondition.
  `pull-queued` and `pull-applied` share the same round/object ID; refusal
  includes a reason, and a round-ending hit cancels its pending request.
  Outcomes are observable in the trace; branching to an authored follow-up
  animation is not implemented yet. This is not Switching Ninja's staged
  two-actor exchange.
- `move=id|button|priority|context|duration ticks|group:clip`: fresh `b` or
  `x` chooses the highest eligible priority, with `any`, `grounded` or
  `airborne` context. There is one active move per actor. Its authored
  upper-body clip advances one frame per simulation tick, not render tick.
- `range=move|units` adds an optional planar entry-distance gate (1–1200
  units to the remaining live opponent in this duel). Constrain the
  higher-priority melee row while leaving the ordinary shot unconstrained
  for a close/far split. Failing the gate on entry can select another row;
  once a move is selected, whiffing doesn't fall back to another attack.
- `melee=move|first frame|last frame|reach|radius|height|half height|damage`
  arms one actor-bound volume on the first active frame. It stays active
  through the inclusive last frame, then retires. Hit tests use a forward
  capsule in the actor's current body basis and an independent height band,
  not the aim point, the model mesh, or a shot object. A volume can damage
  the opponent only once. Interruption before arming prevents the volume;
  ending the action, ending the round, or restarting clears it. Melee
  windows are stockless; an optional projectile marker is a separate
  operation. This is native body-proxy contact, not recovered retail hit
  geometry, wall occlusion, or a continuous bone sweep.
- `contact-power=move|base` opts one direct melee-only or ordinary
  projectile-only move into a different positive HP base (`1..10000`).
  Projectiles capture the base at emission without altering their shared
  definition or other moves. Melee supplies it at accepted contact. In
  either case, existing power-field, role and hit-chain scaling remains
  in charge of the final amount. Mixed moves, child producers, blasts,
  sustained beams, rewind/anchor actions and fields cannot bind this key.
  It does not change collision, timing, stock or receiver rules.
- `contact-minimum=move|1` optionally keeps a positive, accepted direct
  hit from truncating to zero HP after its final multiplier. It has
  the same direct-move restrictions as `contact-power`; a rejected hit,
  zero base, or a native product explicitly scaled to zero stays zero.
  Without this key, the existing native truncation is unchanged.
- `damage-factors=credit|scalar|receiver-rank|relative` and
  `damage-chain=first|second|third|fourth` scale every package hit for the
  declaring package. The four factors are the measured contact multipliers
  (credit rows, battle scalar, receiver rank, relative); the chain is the
  legacy authored hit-count approximation, which can use the first
  four source tier factors (`1.0|0.8|0.5|0.25`). Both default to neutral,
  so a package that omits them keeps its
  authored damage. The chain resets after a 30-tick gap or when the
  attacker changes. These are scale factors, not recovered hit geometry or
  per-Borg branches.
- `damage-vs=model|credit|scalar|receiver-rank|relative` declares a contact
  factor row for one receiver model. The engine picks the row matching the
  target actor's model, otherwise the package `damage-factors` default,
  otherwise neutral. This mirrors the original's receiver-dependent amount
  without a branch on the Borg ID.
- `damage-model=id` optionally declares the package's damage-row id. `damage-vs`
  matches this id; it does not change the actor's motion model.
- `damage-side=team|credit|scalar|receiver-rank|relative` declares a row per
  attacker side (`0`/`1`). Selection order is: receiver `damage-vs` row, then
  attacker `damage-side` row, then the `damage-factors` default, then neutral.
  The side is the attacker actor's team, not its roster index. A package may
  declare only `damage-vs`/`damage-side` rows; the default factors are optional.
  This mirrors the original's side-paired credit rows. The battle host also
  supplies a damage scalar (the recovered settings-ladder value); it multiplies
  the profile factors before the hit-chain row.
- `damage-credit=team|difficulty|attacker-rank` and
  `damage-receive=team|class|receiver-rank` declare independent roles. Each
  declared role requires exactly one row for team 0 and one for team 1, with
  finite factors in `[0.01, 4]`. For direct melee and ordinary direct
  projectiles, the engine multiplies the attacker's credit row and the
  actual recipient's receive row, chosen by their respective teams.
  Without both roles it does not apply a partial multiplier. Blasts,
  fields, detached children and sustained beams do not use the split
  rows. Credit cannot coexist with legacy `damage-factors`, `damage-side`
  or `damage-vs` in the same attacking profile; receivers may still declare
  their own old attack policy independently. The battle scalar and optional
  hit chain apply as before. These rows contain a caller-chosen snapshot,
  not a mechanism for selecting live battle settings.
- `damage-credit-setting`, `damage-receive-setting`,
  `damage-credit-rank`, and `damage-receive-rank` each have the format
  `bank|cell0|...`. Supply 32 setting cells and 8 rank cells, all finite
  factors in `[0.01, 4]`, for both banks (`0` and `1`). Each indexed role must
  be complete and also declare its corresponding fixed role above.
  The caller may supply two setting indices (`0..31`, by team) and one
  rank index (`0..7`) per active actor at round start. This replaces
  the fixed role components on direct split-role hits; it does not
  stack with them. By default the bank equals the actor's team. An optional
  bank per active actor selects the indexed role and HP/force ladder rows
  independently. Both setting indices and the force pools still follow the
  teams, and rank still follows the actor slot. The caller chooses the bank;
  it is not inferred from the controller or CPU seat. Legacy damage, effects
  outside the direct split roles, and contact eligibility are unchanged.
  A declared fixed role
  without matching indexed rows refuses that round setup, as does a
  round with no indexed roles at all. An actor
  with no receiving role does not acquire one through round settings.
  With no round selection, existing fixed rows are used as before.
- `damage-matchup-group=id` assigns a package to an optional named group
  for the split damage roles. `damage-matchup-from=attacking-group|factor`
  declares an incoming multiplier on a **receiver** with `damage-receive`.
  It is selected by the attacking package's group, not by team, model ID,
  or roster index. Up to 20 distinct incoming rows are allowed, with
  finite factors in `[0.01, 4]`. Missing matches use 1. A matchup applies
  only on a direct hit with both split roles present and both actors
  alive. It is multiplied after credit/rank and before receiver
  class/rank. Legacy attack rows and indirect hits never borrow it.
- `damage-receive-hp=bank|cell0|...|cell31` and
  `damage-receive-force=bank|cell0|...|cell31` optionally refine the split
  receiving role. Each declared ladder requires both banks and exactly
  32 finite factors per row (`[0.01, 4]`). HP selection subtracts the raw
  base, not the scaled or power-field-modified amount, from current HP.
  Probes above 200 are neutral; otherwise it uses
  `min(31, 32 - floor(32 * probe / min(maxHP, 200)))` with probe floored at
  zero. The force ladder uses `min(31, 32 - floor(32 * remaining / cap))`
  from the receiving team's pre-hit ledger; no configured cap is neutral.
  These factors do not run without both split roles or on indirect hits.
- `damage-credit-hp=bank|cell0|...|cell31` and
  `damage-credit-force=bank|cell0|...|cell31` declare the outgoing
  equivalents. The HP probe uses current attacker HP without subtracting
  the hit base; the force probe uses the attacking team's pre-hit ledger.
  A package can bind one set of outgoing curves and must opt individual
  direct moves in with `damage-credit-ladders=move`. Unmarked moves,
  defeated attackers and indirect hits do not borrow the rows. Each
  declared curve needs both banks and a complete `damage-credit` role.
  Outgoing curves are applied after credit difficulty and before
  attacker rank, matchup and receiving factors.
- `damage-tier-receive=idle-ticks|cell0|...|cell63` declares a receiver
  meter with a `1..600` tick idle window and 64 finite factors in
  `[0.01, 4]`. It requires `damage-receive`. A package with `damage-credit`
  can add `contact-tier=move|charge|scale` (or `bypass`) to a direct
  melee-only or ordinary projectile-only move. On an accepted hit with
  both roles and declarations, `scale` uses the current tier after
  receiver rank; `bypass` omits only that multiplier. Both refresh the
  window and add charge (`0..255`) after the hit. At 100 the meter
  discards overflow and advances one tier, capped at 63. The meter
  persists across different attackers but clears on expiry, defeat or
  restart. `contact-tier` cannot coexist with the declaring attacker's
  legacy `damage-chain`; unmarked or indirect hits do not advance it.
- `damage-tier-reset=capacity` (`1..32767`) and
  `contact-tier-reset=move|wear` (`0..255`) optionally add a separate
  reset gauge. The receiver must declare `damage-tier-receive`, and the
  move must declare `contact-tier`. Without both reset declarations,
  the hit does not touch the gauge. Damage still samples the old tier.
  After the hit, wear is subtracted; at zero or below, refill the gauge
  and clear the tier and charge before adding this hit's charge.
  Further hits that tick still charge the tier but cannot wear the
  freshly refilled gauge. The gauge has its own idle countdown with the
  receiver's same `idle-ticks` duration. This does not lock movement,
  change the existing hit interrupt, or select a stagger animation.
- `contact-sound-band=0..3` declares a receiver's contact audio band.
  `contact-sound=move|band|lane|even-path|odd-path` declares a local
  PCM pair for one band on a direct melee-only or ordinary
  projectile-only move. The lane (`0..12`) shares an alternation
  counter across moves striking the same receiver. An accepted cue
  starts a four-tick receiver cooldown; further hits in that window
  do not play or advance the counter. A missing band or pair plays
  nothing. A move may not also declare projectile actor-impact
  retirement audio. Both paths are bounded package-relative stereo
  48 kHz s16le PCM, loaded before battle.
- `contact-impact-receive=normal` opts a receiver into move-bound
  hit visuals. `contact-impact=move|team|record|scale-percent|ticks`
  binds one static layer from the local `impact-record-bank`.
  Both attacking teams need 1–3 distinct record IDs (`1..255`) per
  move, with scale `1..400` percent and lifetime `1..120` ticks. Use
  only on direct melee or ordinary projectile-only moves, not together
  with actor-impact retirement art on a projectile from that move.
  Layers replace the normal hit ring at the existing contact
  cooldown; missing receiver admission or insufficient visual slots
  retains the ring. This does not change hit geometry, damage, audio
  or stock. Records share the existing static impact presenter.
- `contact-pose-hold=move|ticks|receiver` or `...|bodies` holds the
  preceding local skeletal sample for `1..7` native ticks on an
  accepted direct hit. `bodies` also holds the attacking body and is
  melee-only. Each participant must declare
  `contact-pose-hold-receive=normal` and use its own native package pose.
  Overlapping holds take the greater remaining count rather than
  adding. The deliberately presentation-only hold keeps the live root,
  reaction angles, attachment clocks and bone/socket queries; movement,
  collision, attack timing and resources continue normally. Missing
  snapshots, defeated actors and borrowed source poses are not held.
- `damage-attack-key=key`, `damage-mask=bits` and `damage-immunity=key`
  (repeatable, up to 48) declare the recovered `8002FD7C` pair-relation fields
  for the MVP hit boundary. Before a package hit is applied, the engine tests
  distinct actors/owners, the attacker key against the target immunity list,
  and the target mask against bit `2`. Defaults are neutral, so undeclared
  packages keep every hit. `damage-kind=0..255` declares the actor class for
  the class rules; the MVP supplies zero record flags, so only the immunity and
  mask rules change behaviour today.
- `reaction-mode=move|0..4` selects a native mapping of the `800300BC` reaction
  direction for hits from that move. On a direct main-move free-flight
  projectile hit, mode 0 uses the projectile's actual
  displacement up to contact, and mode 2 uses its full tested centre sweep.
  Both use the live projectile velocity only when that displacement is zero.
  Unsampled contact paths keep the planar attacker-velocity approximation.
  Explicit mode 1 subtracts the actual carrier origin from the receiver's
  body origin on that same path. Mode 3 instead subtracts the native proxy
  centres: clipped projectile position or melee capsule midpoint against
  the target cylinder midpoint. Without a qualified sample, modes 1 and 3
  use the two actor origins. Omitting `reaction-mode` keeps the existing
  caster-to-receiver default, including for projectiles.
  The source reads the mode from a signed byte at the attacker's
  move-table entry `+0x0E`. Mode 4 uses the attacking proxy's planar
  world-root forward at contact on a melee-only move. A projectile-only
  move requires the explicit `reaction-root` binding below. Mixed or
  indirect moves cannot declare it. Values outside 0–4 are refused.
  Content can explicitly declare a labelled mode-1 approximation. The default is 1. The
  engine writes the receiver's yaw/pitch relative to its facing (the source
  writes absolute link-object angles) and presents them on the upper-body root
  for an authored 8-tick window. The provisional G-Red exact-clip path keeps
  its existing no-aim rule but adds the reaction angles, so the hit pose shows
   without injecting attack aim. The pitch bone axis is an authored native
   mapping.
- `reaction-offset=move|yaw|pitch` declares the recovered attack-table bytes
  (`+0x14`/`+0x15`) subtracted from the reaction angles. Each value is a signed
  byte in 0x100 16-bit-angle units (one full turn per 256), from -128 to 127.
   The default is 0|0. The roster extractor emits offsets for reviewed
   projectile and direct neutral close-B moves whose steps agree. A hit
   without an explicit move can use the attacker's active package move; it
   does not inherit mode 4 or its offsets without the move's direct basis.
- `reaction-root=move|linked-owner` requires mode 4 and a projectile-only
  move. Every release must be a direct damaging contact, not a blast, field,
  sustained beam, child producer, anchor, or rewind. The shot takes the
  living original owner's planar forward at contact, even if the attack
  has ended. If the owner is defeated, the direct hit still applies damage
  with the unbound default reaction. The root is never reassigned to a
  replacement actor or inferred from flight direction, aim, or target.
- `activation=move|release-window|min ticks|max ticks` opts a move into
  selection on **physical button release**. Hold bounds are inclusive,
  from 0 to 600 qualifying simulation ticks. Two moves can share a
  button and use disjoint windows for a tap and a charged attack; if
  more than one qualifies, normal priority applies. A press and release
  sampled before an update can qualify as a zero-tick tap. Selection
  doesn't debit stock. A later marker may be stockless (`none|0`) or
  may declare its own cost; a charge gesture does not imply either.
  Holds don't survive interruption, pause, or restart. No release is
  queued behind an already active move.
- `activation=move|tap|max hold ticks|gap ticks` and
  `activation=move|double-tap|max hold ticks|gap ticks` use a short
  press/release and an inclusive release-to-release gap, each 1–60
  ticks. A lone tap selects after the gap closes. A second short release
  inside the gap selects only the double-tap move, not both. Rows on
  the same button must agree on both limits and use this gesture family
  exclusively. Interrupt, pause and restart clear the pending tap.
- `repeat=move|max cycles` opts a press-started move with one paid
  animation marker into bounded hold-repeat (2–16 total cycles including
  the first). It rechecks context, held input and stock at the authored
  end of the cycle. Stock is only observed there; another animation marker
  must actually debit and emit. Releasing, running out of stock, reaching
  the cap, or becoming ineligible ends the loop. Both activation policies
  use gameplay B/X independently of the native pad's jump binding.
- `release=move|frame|stock|cost|projectile`: at an increasing frame after
  entry, debit the named channel first and then attempt to emit. Use
  `none|0` for an explicitly stockless release. Multiple ordered releases
  compose a timed pattern. An empty pool does not refund a successful
  debit. Prior accepted damage interrupts a pending move before its next
  release. A launched object already in the world has its own lifetime.
  Restart retires the old round's actions, objects and modifiers together.
  Trace object IDs are local to a round.

### Variable-rate held fire

The user-observed normal Gatling Gunner `GG4E08:0102/0` B increases its
fire rate while held and stops firing when released. This is neither a
charge-release attack nor the fixed-cycle `repeat=` policy. The
[archived profile](https://gotchaforce.fandom.com/wiki/Gatling_Gunner?oldid=6629)
names Special Gatling and lists its B stock, but doesn't establish the
startup delay, cadence curve, or whether a release/re-press can reuse
some spin-up. Neither cadence nor cooldown is asserted for the retail Borg.
The [bounded retail check](borg-controller-audit.md#gatling-gunner-held-b-gate)
does verify a held-B repeat gate with a clock, a check-only stock query,
and a separate paid release.

The **authored** component used by Rotary Scout:

```text
move=rotary-b|b|10|grounded|12|0:1
held-fire=rotary-b|rotary-stock|1|rotary-shot|18|9|3|18
```

`held-fire=move|stock|cost|projectile|spin-up ticks|slow interval|fast interval|cooldown ticks`
requires a fresh press **and** a held button. The move's duration is its
upper-body pose loop, not an emission trigger. Starting doesn't fire or pay.
On subsequent eligible held updates, spin advances toward full and the
interval falls linearly in integer steps from slow to fast. When elapsed
held ticks since the last attempt reach the current interval, the service
attempts at most one emission. The normal debit occurs at that attempt;
`none|0` gives stockless fire. Refusal does not bank extra shots.

Release, interruption or loss of context/range stops firing before a due
debit can run on that update. The shot interval always resets. With cooldown
`0`, spin also resets immediately. A positive cooldown is the number of
nonfiring updates needed to cool from full, so a fresh re-press may reuse
partial spin, but never an unfinished shot interval. While idle, cooling
cannot emit. Spin belongs to a particular move and is discarded on
restart. These values (up to 600 ticks each) describe the test component,
not source timings. A held-fire move cannot also declare release-window
activation, fixed repeat, a projectile marker, or a melee window.

The trace reports remaining spin in thousandths (`held-fire-start` and
`held-fire-stop`) and the interval at each attempt (`held-fire-cadence`).

### Movement effect and sound bindings

The shared native mixer binds samples by actor slot for the entire
round. Roster actors can share the same package assets without sharing
playback cursors. Movement, launch, paid-release, action, impact and
contact cues all use those slots. The mixer reserves space for bound
movement lanes and drops new cues at its limit rather than cutting
existing voices or changing the battle. Starting a new round clears the
old cursors.

```text
movement-effect=left-jet|rise|jet|0|-25|22|-42|54|0
movement-effect=dust|landing|puff|0|0|0|0|72|20
movement-sound=landing|audio/landing.pcm
```

`movement-effect=id|cue|style|bone|X|Y|Z|size|duration` binds `jet` or
`puff` to `takeoff`, `air-jump`, `landing` or `rise`. Rising effects have
duration `0` and follow the accepted pose while vertical speed is positive.
Other cues are single accepted transitions: they capture the effect's
bone position and last 1–60 ticks. Pausing doesn't duplicate a pulse and
restart clears it. `air-jump-points=N` is an opt-in for native grounded
movement; takeoff uses the first point and landing resets the count.
Sasuke's normal test profile uses three points, not his recovered controller.

`action-sound=move|path` attaches one bounded PCM clip to a move; it plays once
when that move's start is accepted, through the same one-shot voice pool as the
launch, release and impact cues. The move must exist and the path must be
package-relative; an unknown move, an absolute path or a duplicate all refuse.

Extraction binds it from the curated action-SFX tables: the eleven recoverable
Borgs whose recovered cue group matches a bound button get
`action-sound=<move>|audio/action-N.pcm`, and each binding records
`retail_trigger_verified: false` because only the sample is recovered. The two
remaining Borgs with cues, `0104` and `0c05`, are refused deliberately: both
carry only a `b` move while their recovered cues belong to `X`, so binding them
would play an X action sound on B.

`movement-sound=cue|path` independently binds a local raw stereo 48 kHz
signed 16-bit little-endian PCM clip (up to 3 seconds). The extractor now emits
`movement-sound=landing|audio/lab-landing.pcm` for the 181 packages whose landing
candidate resolves, so that sample is played instead of sitting unused; the
receipt records `retail_trigger_verified: false`, because the landing trigger is
still authored even though the sample is recovered.

That the clip now loads is directly observable: `0004` had no launch, release,
impact or movement clips before, so an audio-enabled run printed no `borg-audio`
line at all. After the binding, the same run reports `borg-audio: device=open`,
because `has_sounds` is satisfied by the landing clip. It reports
`frames-accepted=0` because the test harness never jumps, so no landing cue
fires — audible playback of a package landing is therefore still unverified, and
`--no-audio` gates cannot cover it. Rise loops while
eligible; jump and landing sounds are one-shots mixed through the shared
SFX bus. Sound playback can be disabled without disabling visual effects.
No original media is bundled in Git. Normal G-Red and Sasuke can opt into
locally extracted original movement PCM:

```sh
g++ -std=c++20 -O1 -I . tools/extract/borg_movement_pcm.cpp -o build/borg_movement_pcm
python3 -B -m tools.extract.borg_movement_sfx /path/to/afs_data.afs \
    build/borg_movement_pcm local/content/source-borg-movement-pcm-v1
python3 -B -m tools.extract.portable_borg_preview sasuke borgs/sasuke-api \
    --battle-test --movement-sfx local/content/source-borg-movement-pcm-v1
```

For G-Red, add the same `--movement-sfx` option to his documented practice
export. The tool refuses to overwrite existing directories; move an existing
copy outside `borgs/` first. These are original samples under native cue
policy, not reconstructed sound timing, pan or envelopes. The
[source recovery notes](borg-movement-effects-recovery.md) explain the
separate intro and battle evidence. G-Red's twin jets and Sasuke's dust and
air-jump puffs remain authored visual effects, not retail emitters.

### Direct contacts in roster battles

In roster battles with more than two actors, a direct shot or pure melee
window tests all eligible enemies with the existing collision proxy, rather
than only the current aim target. It contacts at most one body: smallest
sweep fraction first, later slot on a tie. Only that recipient can block.
For shots, an earlier or tied surface still wins. This does not change aim
or move admission. Duel contacts keep the previous policy. Sustained
beams use the same contact ordering at each eligible pulse; area effects
follow their own recipient rules below.

In the same roster scope, native movement checks every other living
body, including allies and damage-immune fighters. Body checks use the
existing vertical overlap and swept planar radius. Walking and cruise
clip at the first blocker, a clipped dash uses its existing exit rule,
and a blocked pull or rewind refuses without moving partway. Externally
accepted source-motion offers retain their original validation path.

### Directional guards

`guard=move|first frame|last frame|half angle radians|contacts|mask` arms a
guard for an inclusive animation window. It blocks up to 16 eligible
contacts in a cone around the defender's current **body heading**. Mask
values are `shots`, `melee`, `direct` (both), and `all` (direct and blast).
Shot direction comes from the incoming velocity; melee uses the attacker's
position and blast uses its centre. A coincident planar origin has no
direction and isn't blocked. A guard is not a separate wall or hit shape.

The engine checks the guard at an otherwise valid body contact, before
damage and accepted-hit effects. Blocking a shot retires it as `7` in the
trace, without applying direct damage, a hit modifier, a pull, an actor-hit
blast, or an actor-contact stock return. Blocking a cut consumes that
window's one contact with the defender. Blocking a blast protects that
recipient without cancelling the blast or its other recipients.

The last eligible block still protects the defender, then breaks the
guard. Later attacks can hit during the same animation. Interrupting or
leaving the window retires it. `guard-armed`, `guard-blocked`,
`guard-broken`, and `guard-retired` are distinct events. Guard Scout's X
braces for one direct hit. This doesn't implement reflection, armour,
shield projectiles, or the retail
[Shield Slash guard](borg-controller-audit.md#normal-knight-and-normal-samurai-shared-movement-distinct-melee).

### Travelling shots that continue after contact

Ordinary travelling shots retire on actor contact unless their definition
opts in with `actor-contact=projectile|continue-once-per-actor`.

The shot keeps its identity, velocity, remaining lifetime, and original
damage credit. An accepted, unguarded contact ends that update at the
contact point. On the next update the same shot can contact another
eligible enemy. It skips actors it already contacted, using their stable
round slots and the shot's own history. A new projectile starts with an
empty history. The engine still owns the sweep, damage, and HP writes;
this option does not spend additional stock.

The existing first-touch rule applies, including stage or floor impact
on a tie. A guard still retires the shot. A miss, ally, or relation-refused
body does not spend the continuation. The remainder of a contact update
is not replayed to search for simultaneous hits.

Only a positive-power, single-projectile release from an ordinary
direct move may opt in. Guided, landing, sustained, child, and detached
shots, release sequences, stock returns, and other actor-retirement
effects are refused in combination. Move-owned contact presentation
can still run on each accepted hit. This is an authored native rule
informed by the [bounded rifle comparison](retail-code-atlas.md#machine-red-one-carrier-two-receivers).
No default retail package selects it. The
[Machine Red Workshop test](retail-roster-extraction.md#machine-red-projectile-continuation-test)
can opt into this one policy; it still uses the native sweep rather
than the original collision scheduler. The
[authored roster smoke](../tests/host/projectile_continuation_smoke.py)
compares both policies without retail assets.

### Stop on contact, return the slot later

```text
actor-contact=shot|stop-and-hold-slot|3
```

A direct projectile stops contacting and drawing on its accepted actor hit,
but its slot remains occupied for the declared **1–8 subsequent** projectile
updates. It cannot move, hit another body, or be allocated to an intervening
release. The opt-in reports `projectile-contact-closed` at the hit and `retire`
when the slot is free. A round restart discards the old pool. Without the row,
an ordinary projectile still returns its slot on the hit update.

This opts in only a single damaging travelling shot on a direct release. It
cannot be combined with recipient continuation, indirect emission, stock
returns, or actor-retirement effects that would otherwise run on contact.
Guard, terrain and expiry keep their existing exit policies. The rule does not
recreate the original attack-shape list or qualify an original callback visit
as a native tick.

### Travelling projectile speed ramp

`projectile-speed-ramp=projectile|gain|cap` optionally increases
a travelling shot's planar speed after each surviving flight
update. The current speed determines that update's sweep. A shot
that survives keeps the increased speed for its next update,
capped at the declared limit. All velocity components scale
together, preserving the accepted socket aim slope. For example:

```text
projectile=bolt|bullet|1|20|0|0|0|8|24|19|stop-on-stage
projectile-speed-ramp=bolt|7|35
```

It travels at 20, then 27, then 34, then 35 native units per
unobstructed update. A slow field shortens travel without changing
the stored speed; an actual shot-pull impulse cancels the ramp
for that projectile so pull physics owns the resulting velocity.
An opt-in actor-contact continuation may retain the ramp, but
guards, terrain, and expiration still retire as usual.

Gain must be `0.01..20`, and the cap must exceed launch speed
without exceeding 140. Guided, sustained, landing, gravity, and
explicit rise combinations are refused. The existing default is
unchanged. The [original rifle replay](retail-code-atlas.md#type-2-carriers-across-the-roster)
informs the order, not the authored units or clock. The separate
[Machine Red Workshop ramp test](retail-roster-extraction.md#machine-red-projectile-speed-ramp-test)
is the only retail-derived opt-in. It preserves the authored
launch speed and scales the verified proportions rather than
replacing the launch or lifetime.

### Ballistic aim and gravity refit

`ballistic-aim=projectile|upward pitch cap` opts a single move-released
projectile into a lower ballistic arc. It samples a visible
enemy's native body centre when the projectile is allocated, then
rechecks the live aim gate before the first flight sweep. A changed
lock can keep that gate open, but it does not replace the sampled
point. If there was no point, the gate closes, or horizontal
separation is negligible, the shot keeps the caster's release heading.

Speed is the total launch magnitude. The upward cap is `0.01..1.4`
radians. An unreachable height or range gets a capped tangent arc,
not a guaranteed intercept. Opted-in shots use half a tick of gravity
in their vertical displacement before the normal full velocity
decrement. Existing shots keep their original native integrator.

```text
projectile=lob|orb|1|40|0|0.3|0|8|60|12|stop-on-floor
ballistic-aim=lob|0.8
ballistic-gravity-refit=lob|4|0|7.5|0.15
```

`ballistic-gravity-refit=projectile|interval|min|max|max change` is
separate. Every `interval` flight visits it estimates the gravity
needed to reach the original tracked enemy's current height. It
adjusts gravity by at most `max change` before that visit's sweep;
it never turns the shot and skips points behind it. The interval is
`1..60`, not longer than the
projectile's lifetime. The `0..8` bounds must enclose launch gravity,
and `max change` is `0.01..8`. Losing the tracked slot or taking a
shot-pull impulse cancels refits without reacquiring a target.

The rule requires launch gravity `0.01..8` and zero authored rise and
spread. It refuses competing socket height aim, angular guidance,
speed ramps, beams, landing carriers, emitter shots, and spawned
projectile children. It can be paired with an authored impact area,
but none of these rules binds a retail move. The
[bounded Ninja comparison](retail-code-atlas.md#ordinary-x-bombs-copied-aim-live-gate-and-gravity-refit)
motivates the separation; the native body centre, gate, and tick
timing are not claims about the original scheduler or hit geometry.

### Blast triggers and recipients

`blast=projectile|trigger|radius|damage|recipients` adds one instantaneous
spherical pulse. `actor` requires accepted actor contact; `floor` requires
a known retiring floor impact; `stage` requires an accepted static-stage
segment hit; `impact` allows actor contact or the contact supplied by the
chosen environment policy. `expiry` uses the carrier's lifetime; `remote` needs a
separate command. Contact or floor retirement, unavailable floor, leaving the
arena, and restart don't count as fuse expiry. `opponent` affects
living enemies only. `all` also includes allies and the owner if in
range. Radius is tested against current body centres. The ring is
presentation only; it cannot cause damage.

Direct damage is independent. A zero-direct-damage carrier must declare a
blast. Set `actor-contact=projectile|ignore` for a carrier that should pass
actors without contact or direct-hit effects. Bomb Scout uses that policy:
its paid X throws a fuse bomb, and only the expiry pulse checks who is in
range. A pulse can miss everyone. If you author both direct damage and a
blast, direct contact runs first and the blast considers surviving recipients.
Neither form silently queues a caster's defeat, and the scoped G-Red
movement adapter cannot opt into self-hitting blasts.

The trace distinguishes `projectile-contact` for zero direct damage,
`blast-triggered`, each `blast-hit`, and retirement under the same object ID.
This is not wall-occluded damage, a persistent or repeating field,
or Death ICBM's retail area
object. That object's contact gate, area allocation, separate self-defeat,
and scoped damage witnesses are described in the
[Death ICBM audit](borg-controller-audit.md#death-icbm-steerable-flight-and-guarded-game-impact).

### Sustained beams

`sustained-beam=projectile|length|damage interval|turn radians per tick`
uses a resident segment instead of a travelling bolt. A single accepted
release creates it, it starts along launch aim, and later horizontal
input turns it by at most the declared step (zero means no steering).
A sustained beam cannot commit an aim pitch: it starts at the caster's
horizontal launch aim, so a low origin is what lets it cross a shorter
recipient's band.
The muzzle offset is sampled once and follows caster translation;
turning does not re-aim at the opponent or resample the socket. The
beam ends on lifetime expiry, interruption, or round end.

The projectile's `radius` controls contact; `look` controls the drawn
width independently. The projectile's `damage` is applied on age zero
and then at the interval **only** when the current segment overlaps an
eligible recipient. A defender with `beam-recovery=ticks` refuses
sustained-beam damage during the declared period after an accepted hit,
including the hit tick. This opt-in does not change ordinary projectile
or melee recovery. A miss, block, or protected tick isn't accumulated
and does not retire the beam.

In roster battles, a main-move beam pulse tests the clipped segment
against all living enemies that pass the pair-relation gate, without
changing its launch aim or steering. Only the first body along the
segment is considered; an exact tie keeps the later actor slot. A
guarded or recovering body consumes that pulse rather than allowing a
hit farther down the beam. This changes which body can take damage,
not the beam's visible length or eligibility for direct-only damage
rows. Duel and detached-effect behavior is unchanged.

This pilot accepts direct single releases. Use `ignore-floor` to pass
through geometry or `clip-on-stage` to stop the visible and damaging
segment at the first qualified static-stage crossing. Clipping does
not retire the beam: each visit evaluates the current line and an
equal-distance obstruction wins. A failed stage query retires a clipped
beam without applying damage. The query tests the centerline, not the
swept beam radius or destructible props. `stop-on-stage` remains the
separate travelling-projectile retirement policy. Desktop Battle Test
admits the pinned ST42 pack separately from authored arenas. An authored
arena opts in with a local `projectile-stage.gfsr` rectangle and local
`secondary.stih` and `tertiary.stih` layers alongside `floor.stih`.
All three grids must cover that rectangle with 500-unit cells; an
explicit empty layer is valid, but missing or malformed data is not.
This does not qualify an authored arena as a retail stage. See
[the asset-free wall example](demo-borgs.md).

This does not cover prop damage, follow emitters, live animated muzzle
tracking, or exact retail steering and damage cadence.
The base projectile `speed` is the planar component of initial launch
aim, not sweep or travel speed. Flight Stick's X demonstrates it.

### Swept beams

`beam-sweep=projectile|travel distance|vertical drop|impact effect radius`
extends a non-steering `sustained-beam` with `clip-on-stage`. The firing
point advances along a straight world-space path from its committed
muzzle, covering the declared planar distance over the beam's active
samples. The ray keeps a fixed downward pitch; its drop must be less
than the beam length. Caster movement and input don't steer or relocate
the sweep after release.

Each contact pulse can place a **visual-only** explosion at its accepted
static-stage crossing. It doesn't add area damage; direct beam overlap
uses the usual damage and recovery rules, and terrain still clips it.
This is a sampled sweep, not continuous swept-volume collision or a
retail blast policy. Try the `--swept-beam` option in the
[wall arena example](demo-borgs.md).

### Registered battle fields

```text
field=scope|3|sphere|210|210|210|105|120|70|135|220|255|130
field-effect=scope|actor-speed|source|130
field-effect=scope|actor-speed|enemies|60
field-effect=scope|shot-damage|allies|200
field-effect=scope|shot-speed|enemies|60
field-texture=scope|scope.rgba
```

`field` takes the move, action frame, shape, three half-extents,
lifetime, forward and height offsets, then RGBA bytes. Spheres require
equal extents; boxes are world-axis aligned. Placement snapshots the
caster's position and facing. All fields share a bounded eight-slot
pool. Colour-only fields don't need a texture. Optional textures are
local 32×32 RGBA8 files, tinted by the RGBA declaration and drawn as
camera-facing art. Texture pixels never define the collision shape.
Add `field-follow=scope|source` to follow the source's accepted
translation instead of staying fixed. The placement offset is kept
in world axes, so later facing changes don't rotate it. Actor speed
uses the field position at the start of movement; projectile queries
use the refreshed position after movement. A followed field stops
if its source is defeated or its centre leaves the arena.

`field-socket=scope|bone|local X|local Y|local Z` replaces the
body-relative placement offsets (set both to zero in `field`). It
samples the same accepted pose path used by projectile origins; a
render-only part spin is not an input. Without `field-follow` the
sample is taken once. With `field-follow=scope|source`, it is sampled
from the caster's current action or locomotion each tick, even after
the placing move ends. The field's shape remains aligned to world
axes. A missing or nonfinite sample refuses placement or retires an
active following field; it does not fall back or keep a stale point.

A field can register up to eight `effect|scope|value` rows.
Effects are `actor-speed`, `shot-speed`, `shot-damage`, `shot-size`,
and `break-guidance` (value `0`); numeric effects are percentages
of normal. Shot-speed currently accepts only slowdown. Scopes are
`all`, `enemies`, `allies`, and `source`. For ammunition, scope looks
at the credited shooter. Battle Test has two opposing actors, so
`allies` includes the source and currently matches the same actor
as `source`; no same-team multiplayer behavior is claimed. A source
may speed up while enemies slow in the same field. Overlapping actor
speed rows use the strongest slow, or the strongest boost if there
is no slow. `--field-demo` supplies a playable combined example.

`field-shot-pull=move|scope|acceleration` adds a 3D inward velocity
increment (0.125–16 units/tick²) toward the field's current centre.
It applies to travelling projectiles whose starting point is inside
the field after guidance updates, before the shot's stage/contact
query. If several wells apply, the nearest centre wins (oldest ID
breaks ties). This does not capture, consume, re-credit, or disable
guidance on a shot. Resident beams and landed objects aren't pulled.
A fast crossing that never starts a tick inside isn't pulled. The
attraction cannot increase speed above 200 units/tick or above the
incoming speed if it was already higher.

`field-core-damage=move|scope|radius|damage|interval` independently
checks actor centres within a spherical core. It first checks after
one interval and repeats while the field is active, respecting `all`
guards. The core fits inside the smallest field extent; it doesn't
pull actors. Either declaration can be used alone. Try both on Flight
Stick's X with `--gravity-field`.

`field-debit=move|stock|cost` pays one declared stock on the accepted
placement frame. The field is refused without spending if the stock
cannot pay; an accepted payment records `stock-spent` and the field
placement follows. The debit happens before the free-slot check, so an
allocation refusal still spends the paid unit, matching the source
factory order. Use `entry-stock` when the move itself must also be
gated.

`move-charge=move|ticks` sets the native dash state when the move starts:
the profile dash speed is used for `ticks`, the normal collision/landing
exits still end it, and the move may also carry a `field` or `melee`.
This maps an authored charged-movement program; it does not implement the
source's typed contact shape.

The same `ticks` value is published as the actor's charge window
(`package_charge_total`) while the move is active, and the Battle Test HUD
draws it as `CHARGE [####------------] elapsed/total` beside the health
line. `battle_charge_gauge(total, born, tick)` is the clamped progress
helper: it reports zero when no window is open and never exceeds the
total. The gauge is presentation for the authored window, not a recovered
retail charge meter.

`free-release=move` marks a static-pose move whose releases are all
zero-cost; it is the explicit no-stock projectile form. It requires one
or more releases, no `entry-stock`, and no paid release in the same move.
Use it only where the source has no debit; it is not a discount on a
stocked move.

`field-drift=move|initial speed|deceleration per tick|active-or-after-stop`
captures the placing actor's heading and moves the field horizontally
from its accepted start. The first step happens on the next tick;
speed then drops to zero without reversing. `active` allows field
effects throughout; `after-stop` draws the travelling field but
enables its effects only once it settles. Lifetime and core cadence
remain measured from placement. Drift and `field-follow` are
exclusive. Drift declines out-of-arena travel but does not query stage
walls. This is not the recovered Power Scope flight or overlap clock.

`field-slot=move|slot ID|on-place` allows one published field
per owner and named slot. Successful placement replaces the prior
field; a failed position or socket sample leaves it alone.
`on-ready` instead requires `field-drift=…|after-stop` and a free
field record. One replacement can drift without effects while the
incumbent remains active. Reaching a valid settled point replaces
the incumbent; failing travel doesn't. Slots are local to the
owner, not to a team or global package ID. This is an authored
publication rule, not an exact retail Black Hole lifetime.

`field-actor-pull=move|scope|step|half angle radians` moves eligible
grounded Borgs toward the field centre without affecting shots. A
nonzero half angle (up to π/2) restricts the field to a forward cone
in the heading captured at placement; `0` allows any direction
inside the base field shape. The nearest eligible field wins, with
ties resolved by field ID. It starts on the visit after placement;
a following field uses the previously accepted position for this
movement step.

`field-actor-pull-point=move|forward offset|height offset|minimum distance`
separates the effect origin from the detection volume. Offsets are
relative to the field centre and its captured heading. The minimum
distance is a 3D eligibility check before a step, not a barrier.
`field-actor-polarity=move|away` reverses the actor step; omitted,
the field attracts. Both modes use the same volume, cone, scope
and collision checks. Neither changes projectile attraction.

The step is capped at 16 units and replaces walking for that visit
without turning the Borg or adding momentum. Jump/dash input and
accepted hit-driven pulls or rewinds take priority. Airborne,
stationary, landing, and interrupted actors aren't pulled in this
pilot. The shared grounded contact check refuses body collisions,
ledge or height changes, and unknown support. If the stage publishes
solid segment queries, the pull also refuses obstructed sweeps.

### Projectile power fields

`power-field=move|frame|radius|ticks|forward|height|damage percent|visual percent`
places a fixed sphere at an accepted action frame. Placement uses the
caster's body facing and authored offset; subsequent actor movement
doesn't move the sphere. Placement spends no stock by itself. There
are eight live field slots; a full pool refuses another placement.

An ordinary damaging package projectile from either actor can take
one boost. The first field reached by its actual travel segment wins;
an actor contact or terrain stop at or before that crossing takes
priority. A shot can start inside a field. The boost scales direct
damage and drawn size independently, without changing collision radius
or stock. Leaving or re-entering doesn't reapply it. The boost lasts
with the shot; restart discards both fields and shots.

Resident beams re-evaluate their current line on every sample. If
it reaches an eligible field, only the segment beyond entry is
visually widened or deals boosted pulse damage. A wall or shortened
reach can prevent entry, and expiry restores the base beam on the
next sample. Explosives with blast payloads and rewind carriers
remain outside the boost. This does not claim retail Power Scope parity.
Use `--power-field` to try Flight Stick's authored X placement, then
fire B through the sphere.

### Slow fields

`slow-field=move|frame|radius|ticks|forward|height|actor speed percent|shot speed percent|break-guidance`
places a fixed, owner-tagged sphere using the shared field pool.
This shortcut affects the opponent, not the owner or the owner's shots.
Use `keep-guidance` if it should only slow travel.

- Opponent walk and dash caps are sampled from the body centre at
  the start of normal native movement. Overlap chooses the lowest
  cap; exiting or expiry leaves no lingering status. Jump, fall,
  accepted pull/rewind and source-owned movement are not modified.
- A travelling projectile spends its update across all of the
  crossed slow intervals. Overlap uses the lowest speed cap rather
  than multiplying rates; outside all fields it uses normal speed.
  Its nominal velocity, fuse, gravity and
  damage are not rewritten. Contacts and boosts use the resulting
  shorter segment.
- `break-guidance` clears an existing shot guidance binding when the
  accepted segment reaches the field, or when the shot starts
  inside. The field does not hide targets, cancel actor aim, or
  destroy projectiles. Resident beams use the same geometric slow
  fraction as shortened reach: neither their pulse clock nor their
  manual steering changes, and there is no live guidance to cancel.

`power-field` and `slow-field` are compatibility shortcuts for a
single sphere plus effect rows. A move cannot mix a shortcut with
the explicit `field` declaration.

Use `--slow-field` to try Flight Stick's authored X sphere. This is
independent of smoke, ink and the power field, and isn't a retail
Jelly Scope binding.

### Static stage impacts

`stop-on-stage` queries the shared three-grid STIH service along the
projectile centre's complete travel segment. The engine compares the
nearest static contact and actor sweep by their distance along that
same segment. Earlier contact wins; a tie goes to the stage. A blocked
target-only blast retires at the stage with no blast. An `impact` blast can
trigger there, using the accepted contact position as its centre.

Today this capability requires the matching qualified **st42** body of
collision data: the stage and floor from `--battle-mvp-assets`, plus the
secondary and tertiary grids from `--source-ground-assets`. Missing or
mismatched grids refuse admission. Missing evidence during a query is not
a clear segment and cannot trigger a blast. The parked face 1130 remains
outside this acceptance seam. `stage-impact` reports the accepted grid
layer; retirement value `8` is static-stage contact and `9` is unavailable.

Impact Scout's B is a fast moving target-only explosive shot; X is an
arcing bomb with actor-or-stage impact. These are authored behavior tests,
not claims about Command Girl, Sasuke, or Normal Ninja's exact weapons.
The query covers static grid faces only, not moving or destructible props,
projectile thickness against terrain, or blast occlusion.

### Projectile guidance

`guidance=projectile|snapshot|turn radians per tick|ticks` steers toward the
opponent's X/Z captured when the shot was allocated. `tracking` instead
reads the same live opponent's X/Z each update and stops if that actor is
no longer eligible. It never searches for a replacement. The turn limit
is 0.001–1 radian per tick, and the guidance window cannot exceed the
projectile's lifetime. After the window the projectile coasts on its last
heading. Landing also stops steering.

Guidance only turns the planar velocity; it does not alter planar speed,
initial Y velocity, gravity, lifetime, or the contact policy. The aimed-at
Borg still has to intersect the projectile. `guidance-bound` reports the
aim recipient and mode (`0` snapshot, `1` tracking); `guidance-ended`
distinguishes window end (`1`) and loss of target (`2`). Retirement or
restart clears the projectile and its binding. Seeker Scout's X uses
tracking; its B stays straight. This is authored planar guidance, not an
inferred retail homing algorithm or vertical target steering.

### Landed charges and remote commands

`bomb-proximity=projectile|radius|arming updates|blast` adds one optional
proximity exit to a single `land-on-floor` carrier with ignored direct
actor contact and an `expiry` blast. The arming clock counts from
allocation; sensing requires an already-landed bomb with current floor
support. The engine selects the living enemy whose body centre is
nearest to the bomb in 3D; exact ties keep the later actor slot. It
does not use the caster's aim target or last contact. The sensor
does not apply damage filters.

`blast` uses the same area spec as expiry. Use `retire` if proximity
should end the carrier quietly while expiry still blasts. Under this
authored policy a valid proximity visit takes priority over same-visit
expiry. `return-stock=projectile|expiry-or-proximity` optionally returns
one paid unit after either exit; support refusal and restart do not pay
it back. A returned unit is not a timer refill. The `bomb-proximity`
trace is a sensor result, not proof of damage.

`bomb-attraction=projectile|outer radius|acceleration|speed cap` is an
optional, separate motion rule. An armed, supported bomb in the
outer radius but outside the trigger radius can move toward the
selected enemy in X/Z. Both ranges are measured to the same current
body-centre proxy. Each accepted step increases speed by the authored
amount up to the cap. Lost attraction eligibility or a refused step
zeros speed. The stage must supply current centre-segment queries;
walls, query refusal, or a change in midpoint or endpoint floor
height prevent the step. This is a flat-support pilot, not a swept
bomb-radius or solid traversal implementation. Attraction neither
damages nor returns stock; the next accepted sensor or expiry visit
still decides retirement.

`bomb-fuse-tail=projectile|ticks` adds a 1–120-update warning window
after the normal lifetime ends on supported ground. The bomb retains
its slot and spent stock, but stops attracting and sensing. On
completion it executes the expiry blast; proximity before the tail
still follows its own policy. Loss of support during the tail, or
expiry before landing, does not blast or return stock. This delay is
authored and does not emulate Roach's full area-object handoff.

The Roach pilot (`0f05`) uses this bomb shape with the source type-5
area row's radius-310 group-3 sphere, a 150-unit proximity radius and an
authored 30/70 arm/lifetime, cap three and return on expiry or
proximity. The source's 300-tick timer, scene receiver and ownership
return are recorded, not claimed.

`blast-children=carrier|child projectile` requests a radial volley
only when the carrier's actual blast runs. The child definition
supplies count, rise, speed, gravity, contact and appearance, but must
have zero spread and no further blast/child request, socket, guidance,
or stock return. The existing pool accepts the entire bounded volley
or refuses it. Actor credit survives the carrier's retirement; the
children cost no second stock unit and start updating on the following
tick. Failure to allocate them does not undo blast damage or stock
return. Use `--fragmenting-bomb` to try this with Jump Stick.

`flight-children=carrier|child projectile|first age|interval|volleys`
is a separate travelling-carrier rule. It makes up to eight
scheduled requests while the carrier is still airborne after its
movement/contact update. A landing, terminal contact, or retirement
on that update suppresses the request. The child volley starts at
the accepted carrier position, uses its current planar direction
and the child's own spread, and first moves on the next tick.
This uses the same bounded deferred pool as blast children: no
recursive child chains, extra stock debit, or retry of a refused
volley. A carrier may use either or both rules and need not deal
direct damage. Try `--elemental-ball` with Flight Stick's B.

`impact-children=carrier|child projectile|stage|forward` requests a
volley from a **terminal stage crossing**. Use `actor` for accepted
actor contact or `floor` for a retiring floor hit. Each must match
the carrier's contact policy. A landing that leaves the carrier live
isn't a terminal floor hit. Use `radial` for even directions instead
of the carrier's planar heading; that mode requires at least two
children and zero authored spread.

The children start at the accepted contact point after the object
pass. Pure impact carriers don't need a blast or direct damage. If
both a blast and an impact request are declared for the same exit,
they are separate requests, in that order. Guard rejection, expiry,
invalid terrain and restart do not qualify as impact triggers.
Try `--impact-ball` with Flight Stick in `arena-wall`.

`impact-trail=carrier|stage|count|interval|radius|advance|degrees|arms|mark size|mark ticks`
is a separate **visual-only** sequence. `actor` and `floor` use the
same accepted terminal contacts as `impact-children`. The sequence
captures the impact point and incoming planar heading. Starting one
interval later, it places one ring per interval. For ring `n` (starting
at 1), the radius is `radius + n × advance`, and its heading is rotated
by `n × degrees + ((n - 1) mod arms) × 360° / arms`. This is a horizontal
pattern at the contact height, not a decal projected onto terrain.
`impact-pattern-space=carrier|world` instead starts at world +Z and
rotates toward world +X for positive angles. It applies to both the
trail and the hazard pattern if the carrier declares both, without
changing the captured position or the travelling shot's heading.
`impact-pattern-timing=carrier|trail first age|hazard first age|hazard first query age`
overrides the three startup clocks independently. Zero preserves the
default (each pattern's interval, or one tick after hazard placement
for the first query). First placement ages are limited to 120 ticks
from impact and first query age to 60 ticks from placement. Later
placements advance by the pattern interval. The hazard's declared
query lifetime starts once warmup ends, not when it is placed.

Each sequence allows up to 32 marks and eight arms; at most eight
sequences can run at once. Rings fade over `mark ticks` using the shared
16-mark impact pool, where new marks replace the oldest slots. A full
sequence pool refuses only the visual effect,
not the hit or stock return. Marks have no collision, damage, or stock
behavior. Restart clears active sequences and marks. Try
`--impact-trail` in `arena-wall`.

`impact-hazard=carrier|stage|count|interval|radius|advance|degrees|arms|hit radius|height|ticks|damage`
uses the same placement pattern to create stationary collision objects.
`height` is the shaft length above the placement point, not the full
shape height; zero makes a sphere. In Battle Test, the receiver is
approximated by a vertical capsule. Sphere hazards include tangency;
nonzero-height hazards require penetration. These are authored
collision rules, not a retail receiver admission.

By default each hazard first queries on the next tick and only checks
the opposing actor. A hit or a block by an `all` guard spends its one contact
while leaving other hazards active. A miss remains live for the declared
number of queries. There are eight active pattern slots and 32 active
hazards; a full pool refuses the request without retry or changes to
the carrier's stock return. Hazards do not use projectile physics,
perform additional terrain queries, or stop at walls. A round restart
clears them.

Gameplay and presentation remain separate: use `impact-trail` with
matching placement parameters to mark the area. The marks can fade
independently of a spent hazard and do not represent its vertical
extent. The `--impact-hazard` demo binds both declarations on Flight
Stick's B. It does not use model 83 or the retail Dragon damage rules.

`impact-area=carrier|stage|initial radius|radius gain|radius cap|queries|damage`
opts into a single expanding sphere at the accepted impact point. Use
`actor`, `floor`, or `stage` to select one reachable retirement reason.
Use `impact` to accept actor contact or the carrier's known retiring
terrain contact, using the one reason that actually retired the shot.
Unknown geometry, guard-only retirement, and expiry do not trigger it.
For example, `impact-area=ball|stage|3.1|31|65.1|4|17` starts at radius
3.1, does not query on the impact tick, and queries at radii 34.1, 65.1,
65.1, and 65.1 on the next four ticks. Initial radius is nonnegative,
gain is 0.01–100 per query, cap is greater than initial radius and at
most 1200, and query count is 1–60.

The area receives a separate object ID and uses the existing
sphere-versus-capsule query. Each living opponent can be hit or block
once per area, in actor-slot order; the area retains its credit and its
remaining lifetime for other opponents. Damage uses the shared damage
path with the authored value. The area consumes no stock and shares
the existing 32-object hazard pool and `impact-hazard-placed` /
`impact-hazard-refused` events; a full pool refuses at impact without
retry. The same carrier cannot also declare `blast` or `impact-hazard`,
and spawned projectile children and emitter shots cannot declare an area.
Existing fixed-radius hazards retain their placement and contact clocks.

`impact-area-shapes=carrier|shared` opts the carrier and its area into
one Battle Test shape budget. The host can set `impact_shape_slots`
to 1–32 (default 32). The carrier reserves a slot before its first
collision visit. If no slot is free, `projectile-shape-refused` retires
it without retry or an ammo refund. An area then needs a **different**
slot at impact, in addition to its normal hazard-pool entry. If none
is free, the area refuses and never retries after cleanup.

On actor contact the parent slot returns at the end of that tick.
An accepted `floor` or `stage` impact holds the slot through the
next projectile pass. This is occupancy only: the retired carrier
does not remain damaging. The area holds its own slot until its
queries end. The policy never spends stock or touches body shapes,
legacy hazards, or projectiles that did not opt in.

These are authored Battle Test capabilities, not retail move
bindings. The optional budget models the order observed in the
[bounded Ninja replay](retail-code-atlas.md#ordinary-ninja-actor-hits-unlink-request-versus-slot-return);
its capacity, geometry, and scheduling do not model the full
retail collision pool. Area visuals remain separate.
Try `--impact-area` in the Flight Stick `arena-wall` demo.

`blast-smoke=carrier|radius|ticks|visual` requests a bounded world
cloud from an accepted blast. A blast with smoke can have zero
damage. The cloud uses generic translucent puffs; its sphere is
centred half a radius above the blast. Use `block-aim` instead of
`visual` to deny ordinary target aim samples and new direct-shot
guidance acquisition when the actor-centre sight segment intersects
the cloud. Denied shots fall back to body facing, with no target
height correction. The check affects both actors; it does not
change target range, contact, beam clipping, following emitters or
already committed aim and guidance. The cloud starts affecting aim
on the visit after creation, lasts the declared number of visits,
and does not survive restart. The pool has eight slots and refuses
new clouds when full, without replacing an existing one.
Use `--smoke-bomb` for the authored demo.

`blast-ink=carrier|ticks|coverage percent|opacity byte` puts a fading
splatter on **each actual eligible blast recipient's viewport**, not
the caster or aim target by implication. It requires a damaging blast
and is applied only after a living recipient takes an accepted hit;
block, miss and defeat do not apply it. Coverage sets the percentage
of viewport width and height occupied by the splatter quad (10–80);
the texture has transparent holes. A new accepted application replaces
the previous one instead of stacking, and opacity falls with simulation
age. Restart clears it. The current authored example uses an original,
procedural texture, drawn behind the HUD. It has no lock-on, accuracy
or movement penalty. Use `--ink-bomb` to try it.

`blast-accuracy=carrier|ticks|max yaw radians` is a separate optional
effect from a damaging blast. It applies after an accepted hit to a
living recipient, whether or not the blast also paints an overlay.
It replaces an existing accuracy penalty and affects subsequent
simulation visits. Add `accuracy-sensitive=projectile` to an ordinary
damaging direct-shot definition to let the status deflect its release.
The volley gets one deterministic, bounded horizontal error; normal
spread is still applied independently. Aim state, release socket,
speed, damage, and receiver collision aren't changed. Guided shots,
resident beams, attached emitters and blast children are not deflected
by this pilot. The release test doesn't use display opacity as input.
Try combined ink and accuracy with `--accuracy-ink`.

This is the authored two-actor slice demonstrated by generating
Jump Stick with `--proximity-bomb`, or `--attracting-bomb` for
`arena-bomb`. It does not reproduce the retail Roach's multi-slot
proximity filters, 3D displacement, inner-phase delay, or area object.

`land-on-floor` stops the carrier on a verified support sample. It does
not trigger a floor blast. The same lifetime continues ticking on the
floor: an `expiry` blast can still run, but a `remote` charge that expires
retires silently. A carrier that loses its qualified support also retires
without a blast. There is no bounce, moving platform, or object targetability
in this slice.

`detonate=move|frame|projectile|landed` asks the engine at an action marker
to trigger all matching **live charges owned by the same actor**. Use `any`
instead of `landed` to include airborne charges. The projectile must declare
`blast=…|remote|…`; the marker itself spends no stock. Both actors may use
the same package/projectile ID without sharing ownership. A command
observes the current set before ordinary projectile travel and lifetime steps
of that update. No matching object means no blast and no queued retry.

`detonation-policy=move|first-eligible` lets only the first matching
charge visited in the engine's projectile pass consume the marker.
It clears the command before retiring that charge, so even a blocked
blast or failed child allocation cannot pass the command to another
charge. An unused command is discarded at the end of that tick.
Selection uses authored pool visit order, not a claim about the
retail scheduler or age priority.

Remote Scout uses paid B to throw, then X to detonate its landed charges.
`projectile-landed` identifies the carrier; `detonation` logs the number
selected by the command; each `blast-hit` is separately resolved at the
charge's location. Restart clears the charges. The
[archived Remote Bomber profile](https://gotchaforce.fandom.com/wiki/Remote_Bomber?oldid=6401)
names B **Set Bomb** and X **Detonation**. Remote Scout does not establish
that Borg's stock, timing, cancellation, or object behavior.

### Stock returned by an object

```text
stock=charge-stock|3|on-return
return-stock=charge|remote
```

`return-stock=projectile|actor`, `floor`, `stage`, `expiry` or `remote` specifies
one qualifying retirement reason, independently of the blast trigger.
The engine binds each successfully allocated carrier to its payer and
exact stock channel. An eligible retirement returns one unit, capped at
the pool size, **after** resolving that carrier's blast. A volley must
pay one unit per carrier from return-only stock. The validator rejects
pairing a return with timed stock or with a stockless release.

No carrier means no return. Failed allocation, unknown support, leaving
the arena, restart, or a different retirement reason doesn't restore a
spent unit. A new round initializes a new pool; it does not return units
from the old round. A return also refuses if the owner is already
defeated. `stock-returned` refers to the exact retired object, not a
generic refill. Remote Scout uses this rule to regain one deploy charge
on accepted detonation, not on natural expiry. The
[Roach audit](borg-controller-audit.md#roach-one-use-jump-control-only-programs-and-bomb-stock)
motivates separating these mechanisms; this test is not a retail Roach
or Remote Bomber stock qualification.

### Independent extra releases

`release-extra=move|frame|projectile` adds one independent projectile
request to a paid `release` marker. The engine spends once, attempts
the primary volley and then the extra volley, and does not try either
one if the debit is refused. Failure of either allocation does not
cancel or refund the other. The extra has its own projectile
definition and usual launch rules, but no stock-return token; neither
branch in this pair may use rewind or a sustained beam. Only a
successful primary emission triggers owner-following emitters below.
This is an emission rule, not a duplicate pose or actor capability.

For up to six total requests, use the paid `release` and up to five
`release-followup=move|frame|projectile` rows. Document order is
execution order after the primary. Each attempts allocation regardless
of earlier success or failure; each individual volley is still
all-or-none. `release-extra` remains the legacy single-followup
spelling; don't mix the two forms on one marker.

For a two-branch release, `release-return=move|frame|all-retired`
uses one unit from a capacity-one `on-return` pool. Both projectile
definitions must emit one ordinary carrier and must not declare
their own `return-stock`. The engine waits until neither accepted
carrier is active. A refused emission resolves its branch immediately;
if both are refused, the return first becomes eligible on the next
owner visit, not during the release. A hit is not required. Defeat
and round reset cannot publish a delayed return into another actor.
This does not implement orbiting or physical return paths.

### Passive walking pulses

`walk-pulse=distance|start radius|end radius|height tolerance|ticks|damage`
adds a passive shockwave to `grounded-v1`. The engine counts accepted
grounded walking distance while there is movement input. Idle, air, dash,
landing, forced movement and interruption do not advance that cadence.
A completed stride snapshots the accepted root at floor height, then
the pulse expands there for the declared ticks. Each update tests the
newly swept annulus against current planar body radii and root heights.
In roster battles it considers all eligible enemies, not the current
aim target. The first radial entry wins; ties keep the later actor slot.
An accepted hit or an `all` guard resolves that wave's only contact.
Allies, the caster, and relation-refused bodies cannot intercept it in
that roster path. Duels retain the previous contact policy. The
visual ring does not drive collision; this pilot does not sweep the
opponent's movement, clip the wave at walls, or conform it to slopes.
There are 16 wave slots; refusal spends the cadence and is not retried.
Try Flight Stick with `--walk-pulse`.

### Owner-following emitters

```text
emitter=left|primary|wing-beam|-35|82|28|35
deploy-emitter=deploy|2|left
```

`emitter=id|watched move|projectile|side|height|behind|lifetime ticks`
defines up to four emitters per Borg. `deploy-emitter=move|frame|emitter`
creates one at an accepted action marker, from the owner's current root
and body heading. A live emitter follows the owner, but it has its own
position, identity, and lifetime. A second deploy won't replace it or
refresh its clock. Deployed emitters retire at expiry or round end;
restart discards all old emitters.
Ordinary emitter expiry doesn't retire shots it already produced.

An emitter fires from its own position when the owner **successfully
allocates** a volley for its watched move. The main release is paid once;
emitter shots add no debit, and cannot claim a paid stock return.
An empty stock pool or failed main allocation signals nothing. If the
main volley fills the pool, followers can refuse independently with no
extra stock changes. Followers do not signal one another. A deploy
marker is processed after that update's owner releases, so it cannot
retroactively join a shot. Projectile collision and damage credit still
belong to the existing battle services.

Wingmate Scout's X deploys two guns above and behind it; B fires the main
shot and both guns while they are live. The test draws a marker at each
emitter, not a separate actor or a recovered weapon model. This covers
the owner-relative coordination described for Cyber Girl Super and
Hyper, not every retail gun phase, target-seeking child, or Double Ninja's
actor and action mirroring.

For Cyber Girl Super and Hyper, the player-observed sequence is
**mounted/inactive → deploying → active**. The pilot has an activation
marker, but neither an inactive back-mounted model nor a gun deployment
animation. Do not use the deploy button press to authorize shooting.

### Ordered anchors and self-rewind

```text
anchor-set=wire|4
place-anchor=place|2|wire|anchor-stock|1
rewind=rewind|2|wire|16
```

`anchor-set=id|capacity` defines at most two sets with 1–8 points each.
`place-anchor=move|frame|set|stock|cost` stores the actor's accepted world
root at a marker, including a midair root. A full set or busy traversal
refuses before debit. Use `none|0` for free placement, or a normal timed
stock pool; there is no automatic refund or return on rewind.

`anchor-return=set|on-reach` opts a self-rewind set into a different
stock rule. Each paid placement takes exactly one unit from an
`on-return` pool and tags that accepted point. The engine returns
one unit only when the owner reaches the point during self-rewind.
Starting the rewind, refusing placement, or stopping before a point
does not return it. A reached point remains credited if a later step
is blocked. Tagged points cannot become a recipient-rewind route
or an electric fence. This does not implement retail midair traversal.

`rewind=move|frame|set|speed` transfers that exact set to a pending
self-traversal, at 1–16 units per accepted tick. It starts on the next
simulation update and visits newest to oldest. The first policy accepts
only same-height, supported planar steps through the existing floor
samples and swept body proxy, not general wall or prop traversal. A
blocked step, midair destination, hit lock, or fresh jump
stops at the last accepted position. It does not skip points, teleport,
restore the consumed list, or continue against a new actor after restart.

Rewind Scout taps X to place and **double-taps X** to rewind. A first
tap waits for the configured double-tap window, so a rewind does not
also record an extra anchor. While stored,
the points display small world markers. The trace identifies each placed
and reached point and reports completion or refusal. This is not exact
retail Wire Girl.

### Electric fences

`place-anchor-offset=move|frame|forward|height` optionally offsets a
placement in the current body heading and world vertical axis. That
accepted point stays in space. This option works independently of fences.

`anchor-fence=set|radius|damage|anchor ticks` connects consecutive
surviving points in placement order. A single point is inactive. The
engine tests each eligible enemy's swept centre sphere against the
finite 3D links. Each enemy can take damage once when entering the
union of a set's links. Staying inside or straddling adjacent links
does not repeat the hit, and a guarded entry also counts. Immune
fighters cannot spend guard or receive recoil. The same link is drawn
only once, regardless of how many fighters cross it. Owners and allies
are excluded. Points expire individually in every actor slot and reset
with the round; defeated owners cannot keep a fence active. There is
no stage clipping or cross-set link, and a fence set cannot also be
consumed as a rewind route.

`fence-recoil=set|units` optionally requests one sideways/away step
on the next visit after an accepted hit. The shared forced-movement
checks can refuse that step without undoing damage. No recoil is
requested for a block. Airborne recoil and exact retail Lightning Robot
placement and connectivity are not yet qualified.

### Hit-triggered recipient rewind

`on-hit=projectile|rewind-recipient|set|speed` gives one contacting carrier
the owner-scoped anchor set at **successful allocation**, then clears that
set from the caster's storage. By default empty sets refuse before debit.
`empty-anchors=carrier|fallback-projectile` can instead choose a separate
projectile at the same release marker. The fallback must not plant or
carry a path, claim an object stock return, or fall back again. It uses the
original release's stock policy. Failed
allocation preserves the set. New placements can start a new set while
the carrier is live, but can never change the sealed one. A miss, guard
block or retirement doesn't queue movement or restore the sealed set.

An accepted hit can move only the actual struck actor; a stationary or
otherwise unsupported actor refuses. The current travel policy is the
same bounded, same-height ground route as self-rewind. It doesn't use
the caster's aim or raw button state. The hit that attaches the route
doesn't automatically cancel its own traversal, but later damage can
interrupt it. The trace includes separate caster and recipient slots
for queued, refused, reached and terminal results.

Wire Trail Scout pays X to place points. Tap B for a stock shot; hold and
release B for a stockless carrier that seals and delivers the path, or
an ordinary damage-only shot when the set is empty. The Move List
reports this distinction for Wire Gunner, and describes double-tap
rewind for Wire Girl. These tests establish API support, not exact
retail timing, control, stock or off-floor travel.

### Stage-contact anchors

`plant-anchor=projectile|set` gives a single `stop-on-stage` carrier
permission to store the point of its **accepted static-stage contact**.
This marker does not use aim, spawn position, actor contact, a computed
miss, or a lookup after the carrier expires. The normal stage-versus-actor
ordering still applies. No point is added if the set is full, its owner
is defeated, or the stage contact lies outside the test arena. The
projectile's stock cost is independent of whether a point is added.

`stage-impact` and `anchor-planted` identify the carrier; `anchor-placed`
identifies the stored point. Anchor Arrow Scout's paid X launches a
non-damaging plant arrow; its charged B seals and delivers the resulting
point to a struck Borg. Actor placement and travel still use the shared
anchor services. A wall contact may be recorded but does not authorize
ground travel to an unsupported height. This is not retail Arrow Ninja's
exact anchoring, ally targeting, stock, or movement.

## Coverage boundary

This slice covers package actor loading, configurable movement,
input priority and context, timed release, held repeat and variable-rate
held fire, phase markers, stock, straight/ballistic/guided
emission, bounded hit resolution, directional guards, a recipient movement modifier, a scoped
deferred self-pull, timed or impact-triggered blast pulses, owned remote
charges, synchronized emitters, stage-contact anchors, ordered self/recipient rewind, and
restart. It doesn't implement a general action graph, retail charge/aim
controllers, melee
combos and cancel trees, guard reflection, vertical guidance, moving links, persistent fields, general relocation, reloads,
form/fusion, custom scripts, or the rest of retail G-Red. Unsupported
fields, assets and capabilities refuse rather than silently choosing a
fallback. Continue using the recovered-behavior audit and wiki capability
matrix to select the next service; the package needn't gain a new operation
for each Borg or cosmetic difference.
