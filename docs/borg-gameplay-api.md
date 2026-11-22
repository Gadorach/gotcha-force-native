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

## Profile

In a `GFBORG 2` manifest declare `gameplay=gameplay.txt`. The file is optional:
its absence doesn't change discovery, previews, ownership or legacy practice.
This slice uses HSD body and motion assets through the existing shared
sampler. It requires an exact, compatible rig and clips, with no fallback.

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
- `stock=id|capacity|refill ticks`: fills on the last debit's eligible timer
  expiry. Declare `on-return` instead of a number for a pool without a
  timer. Neither policy implies a refund on failed allocation.
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
- `look=projectile|size|R|G|B` is independent of launch and collision
  radius. In addition to `bullet` and `beam`, `shuriken` renders a
  spinning four-blade energy shape. `pose-start=move|frame` starts the
  action's visual clip at an authored frame without changing its marker
  clock. G-Red and Sasuke use recovered key-7 local offsets; Sasuke
  uses the recovered upper `3:0` throw entry at frame 7. The port's
  launch timing, aiming and rendered stars are not exact retail playback.
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

`movement-sound=cue|path` independently binds a local raw stereo 48 kHz
signed 16-bit little-endian PCM clip (up to 3 seconds). Rise loops while
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

### Blast triggers and recipients

`blast=projectile|trigger|radius|damage|recipients` adds one instantaneous
spherical pulse. `actor` requires accepted actor contact; `floor` requires
a known retiring floor impact; `stage` requires an accepted static-stage
segment hit; `impact` allows actor contact or the contact supplied by the
chosen environment policy. `expiry` uses the carrier's lifetime; `remote` needs a
separate command. Contact or floor retirement, unavailable floor, leaving the
arena, and restart don't count as fuse expiry. Recipients are `opponent` or `all`
(explicitly includes the owner). Radius is tested against current body centres
in the native duel. The ring is presentation only; it cannot cause damage.

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
