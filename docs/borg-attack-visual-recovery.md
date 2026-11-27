# Attack visual recovery: Shuriken Ninja, Chainsaw Knight, Demon Samurai

Read-only checks of the normal `GG4E08` members in `afs_data.afs`. The
initial scan is asset readback and offline track sampling. The Cutter
follow-up includes bounded original code execution, not a whole actor
frame. Authored Borg API examples are not evidence for retail timings
or bindings.

The scan used PZZ member 4 (the model wrapper), followed the
`scene_data` record table, and inspected the static `JObj → DObj →
MObj → TObj` chains of record 0. The repeat column counts distinct
TObjs with explicit coordinate mode and GX repeat on either axis,
before checking vertex formats or other material state.

| Exact normal | AFS member | Record 0 joints / DObjs | Static repeat candidates | Published joint/material tables |
| --- | ---: | ---: | ---: | --- |
| Shuriken Ninja `0001/0` | 486 | 20 / 33 | 0 | Material only on records 9 and 10 |
| Chainsaw Knight `0201/0` | 510 | 20 / 16 | 0 | None |
| Demon Samurai `0701/0` | 607 | 22 / 18 | 0 | None |

- **Shuriken Ninja:** Record 9 (`0x20f28`) has three material AObjs and
  record 10 (`0x218a8`) has two. Their published material clips contain
  material channels 1–10 (RGB terms and alpha), not TObj animation tracks.
  All five AObjs are nonlooping and end at **1.0**. Detached evaluation
  finds different diffuse values at requested samples 0 and 1 on three
  meshes in record 9 and two in record 10; advancing past 1 does not
  change those values.
  The [Cutter follow-up](#shuriken-ninja-cutter-hand-parts-and-mounted-child)
  identifies record 9 as a selectable mounted-child visual, not a body
  bone animation. Record 10 has not been joined to a live effect.
- **Chainsaw Knight:** The base body does not qualify for the API's
  direct explicit-UV repeat binding. This does not rule out runtime
  material changes, animated geometry, or another record. The recovered
  grounded B prefix selects body clip `1:4` and actor-owned hit group 1;
  the paid X path can request a separate type-25/key-0 child. Neither
  trace identifies the teeth animation. Trace the active body joints and
  render-object state across B before selecting a mechanism.
- **Demon Samurai:** The [owner/child return
  findings](borg-controller-audit.md#follow-up-soul-ball-return-and-blade-growth)
  separate the orbiters, outbound children and owner stock gate. The
  body record scan does not identify either child's visual. Qualify the
  child model/texture binding and its update clock before assigning a
  retail orb appearance.

No retail package, action timing, hit shape, or actor admission was changed
on the basis of this scan.

## Shuriken Ninja: Cutter hand parts and mounted child

The exact `0001/0` model wrapper and motion bank separate three routes:

1. **Body pose.** The engine's requested-time HSD sampler sees Y rotation
   spanning about **6.28 radians** over requested frames 0–16 of Cutter
   clip `4:2` in slots 2 and 13 (the earlier raw-track readback omitted
   the last interpret step). The wrapper maps these to the upper-body and
   leg branch roots, not a standalone spinner joint.
2. **Hand records.** From a supplied `P4:0` start on the loaded normal rig,
   the original VM runs opcode 7 at `8031AC7C` after the authored wait at
   **pose cursor 9**. In this fixture the effective pair changes from
   `0/0` to `2/2` on VM visit **18**; the sampled cursor after that visit
   is 10 on `4:1`. In both tested render styles the subsequent original
   registration changes from records `[4, 1, 0]` to `[6, 3, 0]` (queue
   order). The exact part table maps the left/right record groups to keys
   **7/11**. Registration leaves the logical world matrices unchanged.
   Visit 18 is measured from the supplied request, not from a controller
   press or an original scheduler frame.
3. **Mounted visual.** The exact class initializer (`800EA5FC →
   80098220`) requests a separate dependent for model `0001` before a
   Cutter request. On the supplied allocation-success path, its startup
   requests record **7** twice for the regular/alternate render copies
   and record **9** twice via the animated-record helper. Record 7 is
   flat rigid geometry (raw X/Z bounds about ±130, Y about ±9); record 9
   contains broader geometry and the material clip above. In the
   bounded display callback, child phase 0 uses the record-7 copies
   (registration bins 1 and 68 for the two styles). Supplied phases 4
   and 6 select record 9 for normal style (bin 68); phase 5 selects
   record 7 again. The normal/alternate copies are independent. The
   earlier source action replay observed the parent writing `6` at
   `+0x6E9` on Cutter advance (`800EBE98`); the child update reads the low
   seven bits of that field. These are separate witnesses, not a measured
   same-frame handoff. Static readback of `80007338 → 800086B8` shows the
   animated record path requests material clip 0 at a supplied selector
   time, rather than declaring a loop. The portable `sample` option
   expresses a fixed material selector without assuming that all styles
   or re-entry paths use the same value.

The mounted child update (`80098500`) consumes the **separate**
opcode-17 parent byte: cue `2` clears the byte and requests auxiliary keys
**121–124**, cue `1` clears it and requests keys **53–56**, and `−1`
sets the child latch to `−1` without requesting those auxiliaries. A
changed parent sequence also sets the latch to `−1` in the tested path.
With the checked parent flag clear and child phase 0, a signed angle field
approaches its authored target by a clamped step of **546 angle units per
callback** in either direction (roughly 3°); setting the flag leaves the
field unchanged. The transform follow-up below identifies its axis.

### Mounted transform handoff

The child constructor installs `800480F4`. A six-case original fragment
ran the child update and then that matrix callback with a supplied,
nonidentity logical key-3 pose. The output takes the key's basis,
transforms a **local `(0, 3, −31.5)` offset** in phase 0 (local
**`(0, 3, −33)`** in the tested nonzero phases), and applies local **X
then Y** rotation. Translation is published before the rotations;
neither callback changed the supplied parent matrix.

With cleared child angles and the tested parent flag clear, all five
sampled phases advanced X by `−546` binary angle units on the first
callback. Phases 1 and 3 also advanced the Y angle by `1040` with the
supplied rate of 1. Phases 0, 5 and 6 produced no first-step Y rotation.
The phase-6 flag-set check left the angle fields unchanged. These are
callback results from specific inputs, not established wall-clock
rates. Rotation construction used a supplied trigonometry leaf; scene
services and scheduler ownership were not executed in this check.

A consecutive-update check kept the same child across phase changes.
In phase 0 X approaches `−16384` by `546` per visit, clamps the last
increment from `−16380` to `−16384`, and stays there. In phase 1, the
second Y field approaches `2048` (first step `1040`, then clamped);
the displayed Y angle **accumulates** that field at the supplied rate.
At rate 1 the first four displayed Y values were `1040, 3088, 5136,
7184`; the next visit at rate 0.5 added `1024`. The checked flag holds
both, and returning to phase 0 zeroes the displayed Y field without
zeroing the accumulator input. The portable `part-turn` consumes the
shared clamped-approach rule used by X, not this separate Y integration,
phase reset or scheduler gate.

The selector replay executes the exact VM and record registration on the
loaded rig with declared scene and collision boundaries. A separate
bounded child replay checks the initializer, seven update cases and six
display request cases with supplied allocation and render service leaves.
Detached native asset checks admitted records 3, 6 and 7 to the shared
rigid attachment loader, and admitted record 9's clip 0 to the shared
material-animation loader (three AObjs). The rigid loader correctly
refuses record 9. None of the checks draws the retail Cutter as a live
actor. The next missing join is the dependent's scheduling, matrix
ordering and material selector changes across entry, interruption and return.
Keep its appearance separate from the Cutter's actor-owned hit
shapes and optional airborne effects.

The Borg API's `part-record`, `part-swap`, `part-material`, and
`part-pose`/`part-swap-pose` cover record selection, material playback,
and local mount placement. Jump Stick exercises these in a playable
package. They do not guess a native action window from VM visit 18,
stand in for the child's phase-specific angular integrator, or admit
the full retail actor.

## Deployment leads: Beam Gunner and Neo G-Red

These extend the [package requirements](borg-package-requirements.md#observed-attacks-with-secondary-emitters-and-anchor-paths);
they are not new retail bindings.

- **Beam Gunner `0104/0`, Giga Beam Cannon:** The existing bounded
  original replay separates `P4:0` setup and a check-only channel-2
  gate from a later paid request for type **32**, key **1**. A failed
  late debit takes the fall route without the beam. A later expired
  sustain clock requests `P4:2` without paying again. This supports a
  shared deploy/active/recover sequence but does not identify the cannon
  records, when they swap, how the child traces the world, or which
  prop contacts do damage. The supplied Move List also describes the
  muzzle dead zone; obstacle penetration must not be implemented as an
  unconditional hit. Its damage figure is not a recovered constant.
  Static readback of the three X programs found no opcode-7 hand-record
  swap. The model swap instead belongs to a separate weapon child,
  described below. This is not the child that publishes the beam.
- **Neo G-Red `0629/0`, Neo G Crash:** The existing bounded replay
  observes stockless `P4:1`, advancement at authored pose cursor **40**,
  and a recovery exit on supplied contact or clock expiry. The Move
  List describes the sword, an aimed lunge, charged-B cancellation, and
  post-move fall, while the player's report adds the mounted-to-hand
  transfer, a conditional melee-range homing gate, and the launch-time
  facing fallback. The exact transfer, steering, and shape/length
  differences with G-Red's fist charge still need source verification.
  Both normal builds install the same class main and program/descriptor
  roots, but the `P4:0` and `P4:1` authored openings are different:
  requested actor-bound groups **15** and **29**, and clip descriptors
  `4:0` and `4:3`, respectively. Shared class code supports reusing
  mechanics, not treating the two actions as identical other than art.

### Beam Gunner weapon child

The exact class initializer requests a separate weapon child. In the
bounded original replay, its startup requests **model records 9 and 10**
from Beam Gunner's own bank twice, once per render style. Default normal
display selects **record 9 on logical key 7**. The upper-lane opcode 2 at
`8032109C` in setup publishes parent presentation phase **2**; the
lower lane does not publish it. Running the actual child update and
display-request callbacks after that write gives:

- The normal gun shrinks first. With the fixture's starting scale
  (`~1.2495`) it reaches zero on visit 7 and requests effect **192**.
- On visit 8, the child selects **record 10 on logical key 1** and grows
  it. The checked sequence later requests effect **195** on visit 34.
- With the parent return to phase 0 supplied, the cannon shrinks before
  the child restores **record 9 on key 7**. That fixture restores the
  record on return visit 7. These are callback counts from supplied
  boundaries, not battle frames or release timings.

The portable `part-swap-bone` represents the confirmed record/key
difference and also gives Neo G-Red's proposed back-to-hand transfer a
generic binding. Jump Stick exercises it without introducing a retail-ID
branch. Phase-driven scale, actual weapon/beam schedule, and
retail-package admission remain separate.

The **beam** is a different type-32/key-1 object. Static readback of its
row and active callback shows an origin on **logical key 1** with local
offset **`(0, 25, 350)`**, optional direction correction when there is
a target pointer and the direction passes a dot-product gate, and a
periodic segment handoff (`80139A6C`) to a type-11/key-10 child. The
positive active path did not use a terrain raycast to trim the two
transformed segment endpoints. That does **not** prove the segment's
downstream prop or target collision, damage, or guaranteed contact.
The player's clarified claim is **non-occluding but dodgeable**: aim
stops adjusting at launch commitment, and the target can move off the
line before it becomes damaging. Static readback's optional target
correction does not tell us how long its copied target pointer stays
eligible or whether the actor stops adjusting the key-1 pose first.
The exact cutoff and shorter-window comparison remain open.
The portable `aim-lock` marker provides an authored version of the
distinction: commitment snapshots direction, terrain policy controls
obstruction, and a later move off the line still avoids contact.
Jump Stick demonstrates the marker; it does not use a retail cutoff.

The player reports comparable charged beams for G-Red, Neo G-Red and
G-Black. Keep the recovery distinction: the checked charged-B path
requests type **32**, key **15**, for G-Red and Neo G-Red. G-Black's
checked charged-B entry requests type **89**, key **16**; its separate
charged-X path at cursor 12 requests type **32**, key **17**. This is
a route distinction, not evidence that the type-89 beam has different
occlusion semantics. Preserve the reported close movement similarity
between Neo G-Red and G-Black without treating their exact controllers,
variants or attack tables as interchangeable.

### Steerable sustained beams: Dragon Borgs and Ultimate Cannon

The player distinguishes these from obstacle-pass-through committed-aim
beams: initial aim determines launch direction, but the active beam can
be steered slowly while firing. Apparent diameter and overlap time can
affect how much damage is received; recovery invulnerability can prevent
damage during overlap. These are recovery leads, not shared retail
constants. Steering need not imply obstruction or target tracking;
collision diameter, visible diameter, and damage cadence should be
declared separately.

- The supplied Move List distinguishes Ultimate Cannon's ordinary **X**
  from its **charged X**, describing the latter as a massive laser and
  reporting that some large staggered targets escape most of the damage.
  That supports testing exposure duration; it does not establish steering
  input, the damage clock, or whether the escape is motion, invulnerability,
  or both. The bounded original replay distinguishes the charged-X context
  from an ordinary X stock/launch fragment; the full charged beam path is
  not yet joined.
- The bounded Flame Dragon `0500/0` and Ice Dragon `0516/0` X replays
  distinguish a stockless type-46/key-4 cue from paid key-0/key-3 child
  requests and a separate stockless hold-budget transfer. Those checks
  identify where to trace the active breath, not how it steers or deals
  repeated damage.

#### Type-46 contact child follow-up

Six bounded source cases covered Flame Dragon (`variant row 0`) and
Ice Dragon (`variant row 5`) for a supplied active key-0 breath
substate. The visual update requests a separate **class-3 type-46**
contact child before its own key-0 Y-angle addition. In the checked
visit the child copied the previous angle (0), while the visual
advanced by the supplied `6553`-unit angle operand. This is not a
stick-controlled aim rate. The extra active timers can also request
additional children; no complete frame cadence is inferred.

The contact child uses **logical key 12** and a table-selected local
origin: `(0,150,30)` for Flame and `(0,75,15)` for Ice. The source
computed position and requested a group-0 shape from that position
using a supplied parent matrix and geometry leaf. Allocation refusal
made no child, shape refusal requested child teardown, and successful
binding still requested teardown when the visual parent entered phase
2. No target overlap, accepted damage, recovery immunity, or physical
removal was executed. Neither origin nor render scale establishes the
collision diameter.

The portable Battle Test now also has an authored [sustained beam
pilot](borg-gameplay-api.md#sustained-beams). That tests bounded steering,
static-stage clipping and periodic overlap/recovery; it does not
qualify the retail clocks, input paths, or destructible prop behavior.

The player's follow-up assigns sustained fire to the **charged Dragon
attack** and distinguishes the regular elemental ball that spawns
bullets. The supplied Move List lists Flame and Ice Dragon's elemental
projectiles under B and their continuous, slightly steerable lasers
under X; it also describes a downward air-dash context. Keep that
input-label difference open until the exact start/charge routes are
joined. The type-46 cases above are X-child checks; they are not
evidence that the type-47 B carrier uses the same contact children.

Eight bounded **type-47 B-carrier terrain-tail** cases now distinguish
the selected variant rows. With a supplied accepted terrain crossing,
key 0 (Flame Dragon) requested area **3** only, and key 5 (Ice Dragon)
requested area **23** only. The checked key-1 and key-4 rows requested
areas **24/25** plus separate **type-30** child requests with mode
operands **2/3**, respectively. With a supplied clear query those same
visits requested neither. The table and dispatch do not establish
live damage, projectile graphics, or periodic in-flight child bullets.
The portable `flight-children` example is an authored pattern, not
an exact type-47 mapping.

The roster index now joins the six ordinary B model branches to their
type-47 constructor keys and the parent-joint-12 launch point. It
preserves the paid cue path and later pose separately. All six child
variants request common sound ID `199` after attack collision-group bind, separate from
the paid actor path. The dry sample is not an executable audio binding.
The pinned active trail loop updates five onboard records using a
four-row curve, not projectile allocations. The area and type-30
requests are separate impact branches. The roster records exact keys
for all six rows; this does not settle final damage, an independently
scheduled child route, or the playable X breath path.

The carrier's own slot-2 descriptor is a sphere of source radius
180 or 90, depending on the normal Borg. The collision bind is not a
model load. The five visual requests that follow it select caster
record 9; the sixth selects caster record 8. The roster keeps their
exact normal record offsets and the shared single/split record 83
archives separately. The isolated native draw test finds a renderable
record 9 only on Flame and Fire; the other four use an authored empty
record 9, not another mesh. This does not publish a live draw.

The opt-in authored Dragon orb uses exact record 8, and Flame and Fire
may append an authored record-9 history trail. It may display shared
record 22 at an accepted actor impact with authored growth and fade.
Neither visual runs the retail trail curve, terminal collider or
secondary requests. The optional
authored impact cue uses the accepted native contact instead of the
retail area-child bind.
Treat its timing, collision, aim, contact and damage as native test settings.

### Worms: ordinary B, stationary-entry branch

The four normal Worm packages use the same source controller but have
distinct model-gated calls. After the lead-in, program `2:0` selects
local clip `3:0`; the source program's wait operand `4` precedes the
positive `+1CEF` cue. Phase 2 clears that byte and calls the channel-0
debit before choosing a factory. The moving branch has a different pose
and is outside this test.

| Borg | Factory | Joint 12 local offset | Cue request |
| --- | --- | --- | --- |
| Alien Worm | type 49, key 0 | `(0, 68.5, 11.5)` | `snd_com02:82`, credited actor |
| Claw Worm | type 49, key 1 | `(0, 32.25, 5.525)` | `snd_com02:82`, credited actor |
| Venom Worm | type 94, key 0 | `(0, 68.5, 11.5)` | `snd_com03:4`, allocated child |
| Poison Worm | type 94, key 2 | `(0, 32.25, 5.525)` | `snd_com03:4`, allocated child |

Both cue routes follow a successful collision bind in the static path.
Type-94 key 1 has a separate channel-2 caller, not another ordinary-B launch.
The native test uses the static pose and socket evidence but only fires
one authored orb; the launch sample starts with native allocation.
Neither that test nor the index establishes live string or poison
behavior, contact damage, the retail stock/refund lifecycle, or timing.

### Knight close-B openings

Sword Knight, Normal Knight, Gamma II, Axe Knight, Hatchet Knight,
Sapphire Knight, and Ruby Knight reach the shared program request through
different class wrappers. All seven select local `1:1` for the indexed
neutral entry. Sword, Normal, and Gamma II enter at frame 8; the other
four enter at frame 0. The two lance models keep their separate moving
entry `1:13` outside the authored neutral test.

Axe and Hatchet allocate two independent source presentation objects.
Their model-local rows request record 8 on joint 7 at `(-10,-5,0)` and
record 9 on joint 13 at `(0,0,-22)`. The second row supplies quarter
turns about X and Z; the source mount concatenates Z before X. The
native test draws both as cosmetic parts, including at rest, using
fixed mount transforms. That does not establish source timing, growth, spin, collider ownership,
or HP changes, and the lance models do not borrow those records.

The separate type-5 impact-area initializer uses shared record 22 and
common collision group 60 for all six Dragon rows. This group is taken
from the scene's common collision resource, not any Dragon's slot-2
carrier, emitter, or descendant shape. An accepted bind can reach a
separate child cue (sound ID 12). The offline row and isolated draw
checks do not establish the collider's live size, damage or lifetime.
The common loader joins member 696 block 0 to group 60, which selects
row 94: a sphere at local zero with raw radius 310. The area uses its
own scale fields, so that raw descriptor is not a measured impact radius.

Four further scoped cases with a supplied original contact latch
produced the same per-row area/secondary requests, then an unlink
request. A follow-through now joins those terrain-tail requests to
the actual type-30 constructor and update dispatch for key 1
(Plasma Dragon `0509/0`) and key 4 (Thunder Dragon `0515/0`):

- Across **10 bounded branches**, the separate area request still ran
  when type-30 allocation or its group-7 bind was refused.
- With group 7 accepted, the type-30 object sampled the parent matrix
  at zero local offset and seeded a 30-count lifetime. In the
  supplied valid-position update path it attempted one further
  type-30 allocation per visit for 30 visits, then unlinked. The
  offset starts at 200 and increases by 15 before each request at the
  supplied unit scale. The angle advances by 8° per visit, with a
  0°/120°/240° phase cycling across requests. Both counters advance
  even when the descendant allocator refuses the request.
- The further child uses submode 1 and requests **group 8**. On
  acceptance it requests model key **83** and seeds a 15-count
  lifetime. In a follow-up through the original material setter with a
  supplied loaded model hierarchy, the last six updates wrote
  decreasing material alpha, reaching zero before unlink. Group-8
  refusal skipped the model request. This doesn't identify the
  rendered model or its draw state.

This establishes a timed emitter *after* carrier impact for the two
checked rows, not periodic bullets emitted by the original ball in
flight. The portable `impact-children` binding handles a terminal
volley, not this two-generation sequence. The authored `impact-trail`
demonstrates delayed expanding and rotating marks on the shared
renderer; it has no collision or connection to model 83.

#### Type-30 collider follow-up

**16 bounded allocation/publication cases** used the exact normal
Plasma and Thunder Dragon collision banks, a supplied effect matrix,
and the original bind, shape, and registration callbacks. At unit
scale, group 7 initializes a **radius-10 sphere** at the emitter
origin. Group 8 initializes a **radius-110 vertical capsule**, with
endpoints at its object origin and 400 units above it. Both publish
as attack shapes when eligible. An empty shape pool refused the
bind; suppression or a degenerate matrix prevented publication
after allocation. Neither shape is inferred from the visible art.

In **16 narrow-phase comparisons** with a supplied radius-22 Sasuke
body sphere, the group-7 side tangent counted as an overlap while
the group-8 side tangent did not. One unit inward hit and one unit
outward missed for both at the tested height. **12 pair scans** using
a supplied receiver entry then reached the original reaction entry
only for eligible overlaps: a clear separation and a repeated token
both skipped reaction. The accepted contact credited the Dragon and
latched the receiver without changing HP in that prefix.

**12 amount/HP checks** continued at the shared post-reaction call
site with supplied normal level-zero match state. With either group
and the tested Sasuke receiver, Plasma's raw **10** deducted **9 HP**
(180 → 171), and Thunder's raw **5** deducted **4 HP** (180 → 176).
The checked reaction mask yielded a computed amount of zero; the
scene hold preserved HP and damage counters despite a nonzero
amount. These are scoped results, not package damage constants.
The intervening reaction setup, model asset, and full scheduler-owned
lifetime remain open.
The portable `impact-hazard` demonstration can place timed spheres or
capsules with one contact per hazard in the two-actor test. It does not
assign a collider to the emitter itself, load model 83, or admit either
retail controller.

#### Placement and retirement gates

**14 bounded placement cases** followed both Dragon rows through the
original terrain wrapper and type-30 updates. The terrain wrapper
wrote the supplied crossing into the carrier's root, so the area
request used that corrected point. The actor-contact latch requested
the area at the carrier root without that terrain lookup. The emitter
instead read the carrier's current matrix translation on its first
update. Changing the matrix after the seed did not move the emitter.
The seeded carrier basis also did not rotate the pattern: the checked
SDK rotation and point transform placed offsets from world +Z toward
+X. The first group-8 object's initializer then perturbed the copied
request point by `(-2.5, +2.5, +7.5)` for the supplied RNG draws.

The checked emitter retired without another allocation when its
origin exceeded the source bounds, or when the credited owner's
phase or non-exempt identity no longer matched. The tested stage
upper X bound plus its 750-unit margin was inclusive. These cases used
supplied terrain leaves, class-3 pool and model callbacks and RNG
returns, not the complete live scene. `impact-pattern-space=…|world`
now covers the world-axis placement pattern for authored content, but
still captures the accepted impact point: it does not reproduce
the retail matrix read, RNG, or retirement gates.

**Four class-3 pass cases** then kept the carrier, emitter, and first
descendant in the same original owner list. With a supplied terminal
contact on pass 1, the allocator prepended the emitter to the list.
The pass did not revisit the head: the emitter first bound on pass 2
and requested its first descendant on pass 3. That descendant first
bound on pass 4 and ran its first active update on pass 5. Slots 0
and 5 both followed that order, and the class-3 owner mask suppressed
the initial visit. This checks insertion and callback order, not the
complete stage, collision, or drawing schedule.
`impact-pattern-timing` can express separate startup delays for
authored placement, marks and hazard queries without changing their
existing defaults. It does not infer when a retail collider joined
the scene's separate list pass.

### Normal Ninja bomb launch and area ownership

Fourteen bounded normal `0000/0` cases joined the type-15/key-0
constructor to its group-1 bind and its no-target launch initialization.
The constructor sampled logical key 11 at `(55,0,0)`. The initializer
sampled the same point again after acceptance; refusal skipped that
second sample. With the supplied no-target state, launch speed was
40 and its direction followed the owner's heading operand (tested at
zero and a quarter turn), not the orientation of the supplied key-11
matrix. This used a supplied pose and math leaves, not a fresh retail
action or its target-dependent launch solve. The portable
`launch-heading=…|body` makes origin and heading independent for
authored shots but doesn't qualify the retail route.

The supplied actor-contact latch requested a type-5/key-0 area from the
carrier root. A supplied terrain crossing instead requested that object
from the corrected crossing point. In both cases the area recorded
the bomb as its parent and retained the original caster's credit.
Refused area allocations left no area object. A clear terrain query
did not request one. Area overlap, accepted damage, target-dependent
aim, and the complete scheduling and teardown remain open.

The later [paid-release floor replay](retail-code-atlas.md#ordinary-ninja-bombs-floor-response-and-same-pass-area-bind)
uses real triangle geometry for Normal Ninja and Sasuke. The floor
route keeps the parent shape active through the same-pass area
bind, so the child needs a spare shape slot on that visit.

### Movement contact and attraction leads

The player recalls short spreading shockwaves around some Dragon feet
while walking. The supplied Move List describes damaging foot contact
during walking **or in the air** for several Dragons; it does not
establish an independent expanding damage ring. Retain both accounts
until the exact per-Borg pose cues, ground/air gates, visual children
and damage shapes are joined. The authored `walk-pulse` uses accepted
ground distance, not recovered foot-bone contact or retail timing.

The Move List attributes an enemy-projectile-pulling, enemy-contact-
damaging black hole to charged B on **Space Dragon and Cosmic Dragon**.
The current source audit also finds a separate charged **X** type-7
request (key 1 / key 0, respectively). Its effect is not yet joined to
the projectile and actor consumers; do not settle the input label
or identify that request as the well on this evidence alone.
The guide's descriptions for **Magnet Robot (+) and (−)** instead name a
forward effect pulling **Borgs**, not projectiles. A bounded follow-up
on normal `0405/0` and `0409/0` traced **36 type-45 actor-roster cases**.
Both keys made the same inward movement request in the tested identity
pose and unit-scale path:

- The checked scan visits six actor slots, excludes its owner and
  skips suppressed or ineligible targets. It does not enumerate
  projectiles. A projectile effect on another path is not ruled out.
- Its range predicate compares to a sphere centered 2,000 units
  forward from the supplied emitter socket, with radius 2,000 at
  the tested scale. A second minimum-distance check is measured
  from the socket itself: at 150 and 4,000 units forward the candidate
  passed; 149, 4,001 and the checked rear point did not.
- The accepted on-axis target received an inward vector of magnitude
  25. A second 36-case pass followed it through the original
  accumulator and isolated receiver tail. The receiver applied that
  vector to its root, then cleared the request; a floor return and
  subsequent services were supplied. This is not a 25-unit displacement
  guarantee across live collision. Sign-specific behavior outside
  this tested branch, the scheduler, and actual motion still need
  separate evidence. The player explicitly distinguishes **Magnet
  Robot (+)** attraction from **Magnet Robot (−)** repulsion. The
  guide and the checked inward operand do not settle the final
  minus response. Keep that conflict open instead of giving both
  retail packages the same polarity.

The authored `field-actor-pull` is a separate grounded forced-movement
test with an optional planar cone, offset origin and polarity. It does not qualify either Magnet
Robot or establish airborne magnet response. Do not substitute it for
the checked source predicates, or apply the black-hole shot rule to
the magnets. Dragon core collision, projectile capture/retirement and
exact timing still need original execution.

The player also describes **Lightning Robot** connecting points in
space to form damaging electric lines that force recoil. The authored
anchor, fence, and recoil capabilities are separate. Their ordered
links and entry latch are test policies, not recovered retail point
connections, damage or knockback.

The source catalogue names the candidate **Thunder Robot `0404/0`**,
whose X move is Thunder Wire. **Ten bounded type-43 checks** joined
the supplied contact flag and reference to the first endpoint refresh.
An actor contact retained the exact reference and saved its local
attachment point. The next update sampled the caster's logical key 13
with local offset `(0, 20, 0)` and the recipient's supplied logical
key separately. Both endpoints moved when their supplied matrices
moved. This contact branch seeded 1,200 for matching team fields and
2 for differing fields; these counts are not live durations.

The next registration used group **9**. A recipient phase at least
2, a non-actor/other contact, or ten refused registration attempts
requested channel-2 stock return and retirement in the checked
paths. The attachment conversion and registration/return services
were supplied. This does not establish contact acquisition, damage,
recoil, or a free-standing point-to-point route. Do not relabel the
authored fence as an exact Thunder Wire port.

### Swept beams: Flying Saucer and Death Head

The player reports a fast straight sweep aligned with the target and
explosions at terrain contact along that path. This is a third lead,
separate from committed travelling shots and player-steered sustained
beams. The supplied Move List describes Flying Saucer's X as a
ground-crossing laser with a shockwave where it hits and Death Head's X
as a diagonally downward beam. It does not establish sweep tracking,
explosion spacing, or damage eligibility for either.

The bounded original X checks identify different keys on the shared
type-32 route: **Flying Saucer `0D03/0` key 8**, **Death Head `0505/0`
key 16**. The first has distinct borrowed actor state and an observed
scratch-refusal path. The second gates the paid request on a numeric
accumulator exceeding **8192** after adding **682** in each tested
callback. Static readback now also distinguishes the update routes:
Flying Saucer's factory installs `801DEC90`, whereas Death Head uses
the shared `800E1AB4` dispatch. In their checked active code paths both
can request `80139A6C`, but Flying Saucer copies endpoints from its
scratch path and Death Head's shared path transforms model-relative
endpoints. That service request does not by itself identify terrain
hits or explosions.

**Eight source callback cases** verified what the shared request
does with supplied endpoints: the allocator creates type **11**,
retains both endpoint addresses and credits the parent owner. With
registration accepted, initialization samples the first endpoint
and the active update samples the then-current second endpoint
before retirement. Refusal takes the teardown branch. These cases
verify endpoint reads, not overlap or HP change.

**Sixteen key-8 Flying Saucer cases** checked its separate active
callback. Given a supplied terrain crossing, it requested a
type-9/key-4 object at the corrected terrain point. Independently,
a supplied actor-contact latch requested an area with selector 0
at the recorded actor-contact point. Either path could happen
without the other, and refusing the terrain allocation didn't
cancel the area request. Neither request establishes damage or
draw output. This check does not assign the Saucer callback to
Death Head.

The package API's [`beam-sweep`](borg-gameplay-api.md#swept-beams) is
an authored pilot for a translating downward beam with visual contact
explosions. It does not infer exact retail sweep geometry, terrain-blast
damage, or child lifetime from those requests.
