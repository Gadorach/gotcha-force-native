# Candidate Borg movement families

These are **user-observed categories to guide source recovery**, not an
implemented universal movement map or a substitute for per-Borg controllers.
The [roster extractor](retail-roster-extraction.md#use-the-coarse-movement-and-move-map)
now records source configuration candidates for all normal Borgs. Those raw
cohorts do not by themselves resolve the families below.

| Family | Resource/contact behavior reported by the user | Examples |
|---|---|---|
| Flight meter | Refills while grounded; airborne flight and mid-flight dashes use the meter | G-Red |
| Air-jump gauge | Three jump points for Sasuke; points return on grounding; ordinary chained air dashes do not spend more points and reportedly lose height | Sasuke |
| Fixed airborne stance | Static aerial position; jumping/flying are unavailable | Fortress Borgs |
| Floating/hovering | Flight-meter movement with glide; minimum hover height counts as **ground contact** and triggers the gauge reset | Mainly fairy/angel Borgs |
| Perpetual flight with boost | Gauge boosts movement speed; recovers gradually while **not boosting**; ground contact is neither required nor expected | Planes, flight transformations and some special flying Borgs |

## Important distinctions

- **Physical floor intersection is not universal ground contact.** Floating
  Borgs can reach their minimum hover height without standing on the floor;
  that state must still deliver the controller's contact/reset semantics.
- **Not boosting is not the same as grounded.** Perpetual-flight regeneration
  must not be forced through G-Red's grounded-only flight-meter recovery.
- **Jump points are not a flight meter.** Shared animation intent does not
  authorize charging Sasuke's air dashes against his three-jump gauge.
- **A similar pose is not a common rule.** Clip layouts, authored timing,
  vertical drift, resource cost and admission conditions still need per-Borg
  evidence.
- Transformations may change movement family. Which resource state survives a
  transformation, and which contact/boost producer owns it, remain unverified.

### Drill Robot: move-specific solid traversal

The player reports that Drill Robot can pass through solids, including fixed
stage geometry, during one move. The leading candidate is **Tunnel Attack
(X)** on `GG4E08:0401/0`: the [archived Drill Robot article][drill-robot-wiki]
reports burrowing into the permanent blue barrel and stumps on Little Hill,
and emerging on top of obstacles. This is a player observation and a recovery
lead, not a verified general terrain bypass. Drill Missile penetration concerns
the projectile, not movement of Drill Robot's body.

In bounded original-source replays, Tunnel Attack took body row 0 out of
the `297C` support-check list. During phase 5, rows 1 and 15 returned to
their collision lists while row 0 stayed excluded. In the unobstructed
stage-42 fixture, the normal emergence transition then folded the vertical
offset into root Y, zeroed the offset, and selected lower state 45.
Row 0 stayed excluded both at emergence and when the original cue later
returned Drill Robot to the ordinary fall state. The replay stopped before
the next fall update. A separate timeout replay restored row 0's eligibility
as it returned to fall. Later restoration after normal emergence, obstacle
crossing, and interruption by a hit remain unverified.

Compare ordinary movement and Tunnel Attack on the same obstacle. Trace
entry, traversal, emergence, and interruption separately, and distinguish
static floor queries from fixed prop collision, other Borgs, hurtboxes, and
arena bounds. The normal movement-child allocator and the X pose bind alone
do not establish traversal. Do not generalize this exception or use it to
admit an otherwise unsupported solid-contact route.

[drill-robot-wiki]: https://gotchaforce.fandom.com/wiki/Drill_Robot?oldid=6716

### Recovered examples and remaining limits

A scheduled original-source replay verified G-Red jumping while walking.
Takeoff cleared planar speed without moving the root or spending meter
on the transition. During ascent, weak perpendicular input changed
the desired heading by 90 degrees. With the original constructor's
turn multiplier initialized, travel and body heading turned together
by about 13 degrees over three updates, while camera and sampled upper
aim stayed unchanged. On release, ordinary fall kept that travel and
body heading instead of turning toward the held stick. The tested weak
input added four units of sideways drift per update, separate from
momentum along the retained heading. Static-floor contact entered
landing; the following grounded update restored full meter. The replay
stopped before the landing animation completed.

In a scheduled original-source replay, normal G-Red accepted a fresh jump
press during ground-dash recovery. The transition stopped dash movement
without spending meter on that update. The subsequent ascent spent 350
meter in the tested input sequence. Releasing jump entered fall, and
landing on static terrain made refill eligible on a later update, not on
the contact update itself. In the matching first-recovery-update test,
normal Sasuke recognized the fresh jump input but stayed grounded in
recovery, then returned to neutral without spending meter or using a jump
point. A separate scheduled G-Red replay accepted a fresh jump during
the active dash, before recovery. Both animation lanes switched to
takeoff before the old dash handlers ran. The transition stopped planar
movement without changing position or meter. The takeoff completed while
G-Red was still grounded; ascent and landing were not tested for this
active-dash route. With the jump-inhibit bit `0x2` set in the same test,
the original gate rejected the jump and both dash lanes continued.
This verifies the bit's effect, not how it is set in play or the
outcome for every dash or recovery update.

Teleport Ninja `0005/0` and Switching Ninja `0009/0` each completed a
scheduled alternate ground dash on static terrain. In both exact replays,
releasing the stick after dash entry led through active dash and recovery
to neutral on visit 40; fresh direction entered walk on visit 42. The
authored animation lanes ran, but the scene-child request was not joined
to an allocation, and no competing action was supplied.

Space Dragon `0510/0` completed an original ground-loss, static landing,
and fresh-walk route from a supplied elevated stage-slot position. The
original pose hook ran on all 163 scheduled visits; its target-dependent
pitch-vector branch did not run. This verifies the pose hook and movement
on that route, not an authored spawn or target tracking.

A separate scheduled G-Red replay exhausted the flight meter while jump
was still held. The original game entered fall, then rejected a fresh
relaunch press at zero meter despite readiness and an expired timer.
After landing cleared the airborne state, five grounded updates restored
2,000 meter each in this fixture.

Another scheduled replay accepted a fresh jump on the first update after
G-Red's landing entry. The floor callback had already selected landing
and cleared its pointer, and the landing-entry update had cleared the
airborne cache. The next update refilled meter before admitting takeoff
on both lanes without running either landing handler. A separate fresh
press on the landing-entry update itself also entered takeoff before the
landing handlers ran. Takeoff cleared the airborne cache, but meter
refilled only on the next update. A press on the update that would
otherwise have made floor contact instead selected air relaunch. The
relaunch handler spent 150 meter and the floor query missed. On the
next update, releasing jump returned G-Red to fall and the floor
callback selected landing, one update later than in the baseline.
That replay stopped before the next landing-entry update.

A fresh directional flick can also start G-Red's ground dash on the
first grounded update after landing entry. In the tested route, meter
refilled before dash admission, neither landing handler ran, and the
first dash update moved the root 25 units on the static floor. When a
fresh jump and flick arrived in the same packet on that update, the
original game selected takeoff without calling the dash gate. On the
following update, both inputs had been released and the strong-direction
buffer was still present while the game continued takeoff. A matched
continuation kept the pad neutral. On the first update after takeoff
completed, the original air-dash gate consumed the buffered flick and
selected wind-up before ordinary ascent. No new press or ascent debit
occurred. A matched continuation kept the pad neutral through the dash.
Wind-up completion and active dash spent 2,000 meter in total. Eight
floor hits did not select landing while the callback pointer was null.
Once the callback ran, the original game entered landing directly from
active dash, bypassing dash recovery. The following grounded update
restored full meter. The replay stopped before the landing animation
completed.

A separate original-source replay landed G-Red on `st42` face 1014,
a 10.73-degree incline. The terrain solver read the authored height
and normal and took its landing branch, not its projection branch.
Landing entry cleared the airborne cache; meter returned to full on
the next grounded update.

A matched continuation used a fresh flick on that first grounded
update. The original game refilled meter before admitting the dash,
consumed the buffered flick, and skipped both landing handlers. The
dash entry moved 25 units uphill, raising root Y from 59.585 to 63.244.
With the stick released on the next update, both active dash handlers
ran and the pose published at Y 66.829, still on face 1014. The replay
stopped after that first active update.

An independent grounded walk on the same face moved the root by
approximately `(-0.125, +1.713, +9.851)` in X/Y/Z on its first travel
update. The original queries read the slope's height and normal; both
history vectors retained the prior root, and the new pose matched the
new root. No landing callback ran.

In a separate descent onto face 1949, the
authored slope was about 75 degrees, above the tested landing threshold
of 56.25 degrees. The solver projected the root along the surface and
damped downward velocity instead of calling the installed landing
callback. G-Red stayed airborne and did not refill on that update.
A matched follow-through projected off face 1949 a second time, then
left the ridge in ordinary fall. The next three updates queried the
flat floor far below him and did not land. A longer replay verified
that the original integrator used G-Red's initialized downward
velocity limit of −30 in this state. He eventually landed on lower-floor
face 758 at Y=10, and the next grounded update restored full meter.
These results verify exact faces and approaches, not every slope
or prop collision.

A separate walking departure set G-Red's fall timer to 30 through
original code. When he contacted the 45-degree face 1953, the timer
was still positive and the solver used the stricter 33.75-degree
landing threshold. It projected him along the surface, damped his
downward velocity, and left the landing callback installed without
running it. Neither the timer nor the contact was supplied. A matched
continuation recorded four more projections while the timer was
positive. G-Red then fell toward the flat floor below, and the timer
expired before he reached it. In a separate neutral-jump replay on
face 1953, the timer stayed at zero and the solver used the
56.25-degree landing threshold. This time the callback ran and
selected landing, and the next grounded update restored full meter.
The two replays verify the threshold difference on the same face,
not a matched approach or velocity comparison.

The [normal Blue Striker follow-up](borg-controller-audit.md#normal-blue-striker-cruise-boost-and-hud)
verified that refill checks boost inactivity before selector changes.
In a later scheduled Mu replay, floor contact snapped the root without
ending cruise or clearing the airborne state. Refill began in cruise only
after the boost bit cleared and the refill delay expired. This does not
make Mu's boost cost or refill timing universal. The
[authored `cruise-boost` option](borg-gameplay-api.md) shares the meter
arithmetic but uses a fixed terrain-relative height and native
simulation ticks; it does not bind a retail aircraft controller. The [normal
Sasuke follow-up](borg-controller-audit.md#normal-sasuke-air-points-and-hud) verified
his three-point limit, the conditional zero-to-one wind-up write, the later
grounded reset, and the HUD's hidden-at-zero-used rule. A scheduled
original-source replay then verified that a fresh airborne jump was refused
at three used points even with readiness set and the timer at zero.
Floor contact left all three points spent; a later grounded update reset
them to zero. A separate directional flick at the same cap admitted an air
dash. Wind-up and the first active dash update preserved the three used
points and spent no meter. That replay did not reach landing.

In the [Wing Soldier and Quick Valkrie follow-up](borg-controller-audit.md#normal-wing-soldier-and-quick-valkrie-glide-and-contact),
the tested ordinary paths use zero floor offset. A snap can lead to grounding
if it runs the landing continuation and landing state, but the meter does
not reset on that event. A later scheduled replay for normal Wing Soldier
verified that releasing glide led to fall, a fresh press used one jump point
without spending meter, and landing left both used points intact. This is
not evidence for Quick Valkyrie's glide exit or a shared hover floor.

In a separate scheduled replay, weak input while jump was held moved
Wing Soldier into steered glide and cleared the installed landing
callback. Both terrain queries found him above the static floor, so
neither snapped nor landed. The lower selector then chose straight
glide while the upper selector remained in steered glide. Meter and
used points were unchanged; the current pose was still airborne.
A matched continuation ran both straight-glide lanes for three updates.
Each moved the root seven units forward at the same height. Downward
velocity was damped but not applied to the root, and the original
height queries found the floor below. There was no snap or landing
callback, and the brake gate did not admit braking on the held input.
On the next update, centering the stick while still holding jump
admitted braking. Forward speed fell from 7.00 to 6.86, height stayed
fixed, and all three animation lanes published the brake-entry pose.
The landing callback remained absent, and meter and used points stayed
unchanged. A matched replay verified that brake exit waited for the
lower lane's blend to clear at age 7, not for the authored clips to
finish. The lower selector returned to stationary glide while the upper
selector was still braking. On the next update, both lanes ran stationary
glide and the original game reinstalled the landing callback. The actor
was still above the floor, so neither a snap nor landing ran.

The [Beam Satellite follow-up](borg-controller-audit.md#normal-beam-satellite-placement-stance-and-support)
found table-based placement and ordinary steering out of its stationary idle.
The [Galactic Emperor follow-up](borg-controller-audit.md#normal-galactic-emperor-class-pin-and-placement)
instead found a guarded class pin at absolute world Y=2000, separate from
constructor placement and projection, and no admitted ordinary steering.
The [loaded Emperor comparison](emperor-pin-checkpoint.md) confirms that
the shared constructor owns the initial placed root and the later class
update restores a different pose. Neither establishes a fixed-XYZ Fortress
family. Other hover-height producers, live Fortress clearance and Sasuke's
dash height over a full route remain unverified. Do not fill animation
annotations or generalize G-Red's recovered profile from these audits.

The [loaded Angel Nurse/Wing Soldier contrast](pose-offset-contact-checkpoint.md)
found a pose-derived logical contact offset on Nurse, but it places the root
**below** the queried floor at snap. A separate landing visit precedes its
point reset. Wing's logical offset is zero despite a positive class pose
height, and its inhibit-bit meter recovery can occur before contact. This
does not establish the reported above-floor grounding rule.

### Ordinary air-dash roster checkpoint

The retained-air-dash audit covers all 206 normal profiles. In that
fixture, all **160** eligible ordinary routes reached neutral; **155**
also accepted fresh directional input. The other five traces ended at
neutral. Outside that set, 27 profiles refused the specific tested
ordinary request and 19 lacked the ordinary selector-15 row. Neither
result rules out an alternate dash. This checkpoint does not establish
natural startup, owned support, or an entire controller.

### Ordinary fall roster checkpoint

Saved replays record a complete scoped fall route for **181 of 188** triaged
normal profiles. That count is not proof of original mode-owned spawn,
unmasked startup, or moving-support behavior.

| Remaining profiles | What the saved replays establish |
|---|---|
| Drill Robot `0401/0` | The class initializer and constructor-owned body binder both returned. Placement, first pose publication, and ordinary fall remain open. The air-dash and Tunnel Attack results are separate. |
| Quick Valkyrie `0B00/0` | Ordinary fall remains open. The verified glide hold does not establish its live exit. |
| Carrier Helicopter `0D02/0`, Flying Saucer `0D03/0` | Their alternate type-6 routes are not ordinary fall passes. |
| Death Borg Nu `0F01/0`, Nu II `0F02/0`, Nu III `0F03/0` | The evaluator stopped at the guarded `0/0` path; no original fall refusal was established. |

Carrier Helicopter and Flying Saucer each have a separate loaded-actor
replay in which a supplied alternate-fall state reached real static
contact, selected neutral rather than ordinary landing, and published
the current pose without animation clips. Their independent ground-dash
replays returned to neutral and accepted a fresh direction into
alternate travel. These results do not verify a continuous route from
authored placement and natural startup.
The roster exporter indexes their separate travel handler and its planar
operand. The opt-in Flying Saucer Battle Test reuses that operand for a
relative ground pace on an authored static body, not the alternate
takeoff, contact, or landing scheduler.

Drill Robot's constructor has two kind-0 requests for four descriptors
each, five kind-0 requests for two each, and a settings-gated key-11
request for two more. This is a static request count, not an observed
allocation count. The key-11 factory uses the same source allocator
callsite verified during Flying Saucer's construction, but it needs
Drill Robot's own resource and Scene owners. For normal slot 0, a
successful key-11 allocation would populate group 25 before movement.
The later movement test must use the actual list at that point rather
than assume the earlier empty-group fixture still applies. In a bounded
replay, the first constructor request linked a class-2 cell into group 1
and consumed four row-0 descriptors. The actor's delivered resource and
revision were current before pose publication. A fresh replay matched
that prefix and reached the original class initializer return. All
eight requests succeeded: seven entered group 1 and key 11 entered
group 25, consuming eight class-2 cells and 20 row-0 descriptors.
The replay verified insertion into an empty list and insertion ahead
of an existing list. It stopped before body registration or first
pose publication.

A subsequent replay joined body registration to the same constructor.
Ordinary delivery published the exact role-2 collision bank, and the
binder read it to register 15 class-4 bodies in physical group 49 for
slot 0. Their links pointed to matrix slots owned by the same actor;
the binder did not update those matrices. This verifies registration,
not a current pose, collider sweep, or contact. The earlier Tunnel
Attack fixture used a supplied Scene bank pointer and remains a
separate test.

The placement preflight stopped before replay. The earlier constructor
fixture skipped the original ancillary setup, which writes a value read
by first placement and consumes a byte-RNG stream not retained by that
fixture. A supplied slot and terrain cannot replace those inputs. The
separate boot-pose replay used a retained root, not a source-placed one.
The follow-up identified the missing handoff: the same Scene must supply
its nonhuman slot-0 setup bytes to the original constructor. Those bytes
select the AI helper rows and the source write used by placement. The
original RNG initializer is known, but another fixture's stream cannot
supply the current state or account for intervening draws. The omitted
stock setup also has two earlier allocation requests that can populate
group 25. A fuller replay must observe those requests and carry forward
the resulting list, not require the scoped fixture's original cell order.

Gatling Gunner `0102/0` matched its earlier ground-loss update: the
original check saw a 140-unit drop against its loaded 40-unit threshold.
The new replay stayed in bounds rather than moving the actor outside the
terrain grid. The original floor callback selected landing on visit 22,
the actor returned to neutral on visit 48, and fresh direction entered
walk on visit 50. A separate refill probe refused while the airborne
cache was set on contact and accepted after landing entry cleared it.

Wire Gunner `0103/0` stayed neutral in the earlier raised-terrain
control. From a different supplied slot, the original check measured a
125-unit drop against his loaded 40-unit threshold and selected fall.
The floor callback selected landing on visit 21, he returned to
neutral on visit 47, and fresh direction entered walk on visit 49.
This verifies the threshold-based route, not an authored spawn.

Machine Red `0600/0` stayed neutral at Y=150, where the drop to the
floor was 140 units. From a supplied Y=235 slot, the original check
measured a 225-unit drop against his loaded 140-unit threshold and
selected fall. The floor callback selected landing on visit 18, he
returned to neutral on visit 44, and fresh direction entered walk on
visit 46. Proto Red `0616/0` independently showed the same threshold and
ordering in the same supplied-height tests. In both replays, a separate
refill probe was refused on contact and admitted after landing entry
cleared the airborne cache.
