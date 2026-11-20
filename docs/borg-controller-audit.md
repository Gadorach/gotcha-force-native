# Normal Borg controller audit: one per retail tribe

The first pass found **eight movement-selector tables and 20 distinct
class-main callbacks** in a sample of one normal Borg from each retail tribe.
Eleven representatives share Sasuke's selector table byte-for-byte. Several
representatives that share a table differ in movement type, resource policy,
pose data or class actions. **Tribe is not a controller key.** These are
static source comparisons, not 20 recovered playable controllers.

## Scope and representatives

Checked the qualified initialized `GG4E08` DOL revision used by the existing
[G-Red recovery](g-red-animation-controller.md). The source tribe table
is read by `80066298`, separately from collection order (`800661D8`) and
model-specific installer dispatch (`80056CE0`). The 206 retail collection
entries cross-check against per-Borg infoboxes in the
[July 2022 wiki archive](https://archive.org/details/wiki-gotchaforcefandomcom).
The wiki supplies English tribe labels, not executable rules or proof of a
move's source dispatch. Each row cites the AFS battle member used to check
normal wrapper and bank structure. The archive as a whole was not
hash-qualified. `/0` means normal retail variant, not a package ID or form.

| Tribe | Normal representative | Collection / source model `/0` / AFS member | Selector cohort | Source type / installed resource policy |
| --- | --- | --- | --- | --- |
| Ninja | Sasuke | 2 / `000A` / 495 | A | 0 / 0 |
| Gun | Revolver Gunman | 21 / `0100` / 498 | A | 0 / 0 |
| Knight | Normal Knight | 26 / `020A` / 519 | A | 0 / 0 |
| Girl | Battle Girl | 34 / `0300` / 525 | B | 0 / 1 |
| Machine | G-Red | 1 / `0615` / 587 | C | 0 / 1 |
| Dragon | Flame Dragon | 149 / `0500` / 553 | D | 4 / 1 |
| Musha | Normal Samurai | 39 / `0700` / 606 | A | 0 / 0 |
| Knuckle | Kung-Fu Master | 41 / `0800` / 620 | A | 0 / 0 |
| Wizard | Patra Witch | 48 / `0902` / 636 | A | 0 / 0 |
| Nurse | Angel Nurse | 47 / `0900` / 634 | A | 0 / 0 |
| Idol | Pop Honey | 50 / `0904` / 638 | A | 0 / 0 |
| Bug | Roach | 189 / `0F05` / 691 | E | 1 / 0 |
| Demon | Demon Samurai | 186 / `0701` / 607 | A | 0 / 0 |
| Hero | Gold Hero (base) | 122 / `0802` / 622 | A | 0 / 0 |
| Wing | Wing Soldier | 52 / `0A00` / 647 | F | 2 / 2 |
| Angel | Quick Valkrie | 53 / `0B00` / 658 | F | 2 / 2 |
| Tank | Normal Tank | 64 / `0C00` / 666 | B | 1 / 1 |
| Air | Blue Striker | 70 / `0D00` / 672 | G | 3 / 1 |
| Fortress | Beam Satellite | 148 / `0E01` / 681 | H | 9 / 1 |
| Death | Death Borg Beta II | 79 / `0105` / 503 | A | 0 / 0 |

Type comes from the installed configuration (`800562B8`); resource
policy is selected separately (`80056598`). Neither number names a complete
movement family. G-Red has bounded source-controller practice. Sasuke's
native duel behavior is provisional; Beta II's prior qualification is for
initial AI construction only. No additional native asset or actor qualification
comes from this table.

## What is shared, and what is not

Shared input/selection supplies a 32-pair selector table (`4F0`), with
independent upper and lower lanes. Source state `0` is a handler that does
nothing, not the `-1` sentinel for an unavailable lane. Tables compare
**representable states**, not reachable inputs or animation clips.

| Cohort | Observed distinction |
| --- | --- |
| A | Eleven byte-identical tables; selectors 0–17. |
| B | Battle Girl and Normal Tank share A's coverage but replace two lower states. Their movement types and class hooks differ. |
| C | G-Red's table adds selectors 25–26 to B. |
| D | Flame Dragon has a different state at selector 4 and omits 12–14. This is not proof of no dash. |
| E | Roach shares A through selector 11 and disables 12–17. Same movement type as Normal Tank, different table and resource policy. |
| F | Wing Soldier and Quick Valkrie add 18–24 to A, including glide branches that call a floor probe. Their class hooks and pose data differ. |
| G | Blue Striker has 0, 9–11 and 27–28. The latter pair has flight handlers, not an ordinary ground jump/dash map. |
| H | Beam Satellite shares G's 9–11 and 27–28 rows but uses return-only idle and different 1–3 handlers. Shared rows do not prove that a player can reach them. |

- **Resources and contact:** Sasuke uses a three-point count, separate from
  G-Red's meter. His [exact normal follow-up](#normal-sasuke-air-points-and-hud)
  distinguishes request, spend, ground reset and HUD update.
- **Flight:** [Blue Striker's exact normal follow-up](#normal-blue-striker-cruise-boost-and-hud)
  separates boost inactivity from ground recovery. The
  [Wing Soldier and Quick Valkrie follow-up](#normal-wing-soldier-and-quick-valkrie-glide-and-contact)
  separates a glide snap, logical landing and meter refill. The
  [Beam Satellite follow-up](#normal-beam-satellite-placement-stance-and-support)
  found a stationary idle stance but an ordinary steering path out of it.
- **Pose and actions:** Roach has an empty local bank and sampled lane-control
  programs; Blue Striker and Beam Satellite have empty banks and null local
  program/descriptor roots. All three still dispatch class actions. Roach and
  Blue Striker call shared stock admission for a sampled action, then different
  creation paths; Beam Satellite has a sampled class-local flag/timer path.
  The evidence does not name the full B/X branches or authorize assuming a
  missing animation bank means no action.
- **Ordering:** The shared frame driver uses a pre-class hook, lower dispatch,
  previously captured upper dispatch, and a post-class hook (`80055568`).
  A class action runs through upper-lane ownership (`80065DFC`). Stock/refill
  storage is separate from air resources, but action and movement cannot
  simply tick in any order. Similar pose requests do not establish common
  rigs, native maps or transition timing.

## Normal Sasuke: air points and HUD

This follow-up is scoped to `GG4E08:000A/0`, AFS member 495. The exact normal
wrapper and local motion bank match the retained native duel art fingerprints;
that match is not controller admission. [Wiki lead: Sasuke](https://gotchaforce.fandom.com/wiki/Sasuke?oldid=6766).

| Event | Recovered ownership and order |
| --- | --- |
| Ground takeoff | The conditioned input requests selector 4. The takeoff pose begins without spending a point. Its lower-lane handoff selects 5; ascent entry (`8005FB54`) increments **points used**. |
| Air relaunch | The air-jump pulse is fresh, not a continued hold. Requests from falling/recovery (`80069210`) also require the admission latch; active dash (`800692BC`) uses a separate leaf without that latch. Both refuse a used count of 3. The admitted entry (`80060504`), not the request, increments the count. |
| Air dash | Wind-up (`80061370`) writes `0 → 1` and preserves 1, 2 or 3. The normal wind-up handoff and active dash have zero meter debit. Sasuke's own recovery program 12 starts clip `0:25` at frame 0, not G-Red's clip. Dash phases have distinct vertical producers; net height loss over a route is unverified. |
| Landing and reset | Ordinary floor contact selects landing; landing entry clears the airborne flag. The next ordinary grounded resource update resets the count (`80055AA0`). A floor hit alone neither resets it nor proves landing; dash wind-up can probe or snap with no landing continuation. |
| HUD | The count gauge (`80013A28`) leaves draw-ready clear at zero used. At used 1, 2 and 3 it reports 2, 1 and 0 remaining, subject to actor suppression and view gating. Its display transition is separate from the counter write; no rendered pip output was verified. |

The bounded offline comparison against qualified DOL instruction fragments
matched **162 synthetic input cases**. It checked selected input
pulses, requests, counter stores and display bookkeeping, not an original
world frame, physics path or HUD draw. **Walk off a ledge → fall without
jumping → directional air dash** is a source-supported candidate for the
zero-to-one case, not an accepted route. Verify it independently from an
ordinary ground launch → air relaunch → dash → landing witness. Keep exact
stage contact, accepted input, both pose clocks, the used count and display
state in the same trace before enabling Sasuke practice.

The detached `normal_sasuke_air_relaunch_request` checks only the reviewed
count-based leaves for falling, dash recovery and active dash. It emits a
candidate selector without touching the count. The shared air-point entry
component proposes changes only for an explicitly entered ascent, relaunch
or dash wind-up. It models the zero seed as a policy, not a rule for every
Borg. It uses the existing shared grounded recovery for the reset, and the
gauge readout exposes draw-ready/remaining only, not fade, binding or draw
state. No gameplay or picker code calls these new components.

Native results matched the qualified source fragments on **258** cases:
144 relaunch requests, ten entry writes, 72 ordinary recovery updates
and 32 gauge observations. Inputs were synthetic, including cached-airborne
samples, full or partial counts, and an actor-suppression bit. The comparison
does not establish who owns the actual current actor, the input/pose/contact
handoff, or how a gauge is drawn.

## Normal Blue Striker: cruise, boost and HUD

This follow-up is scoped to `GG4E08:0D00/0`, AFS member 672. Source movement
type 3 and policy 1 are distinct observations: sharing policy 1 with G-Red
does not imply the same refill gate. [Wiki lead: Blue Striker](https://gotchaforce.fandom.com/wiki/Blue_Striker?oldid=6731).

| Event | Ordinary source branch |
| --- | --- |
| Enter boost | Cruise selects boost (28) from a fresh conditioned airborne press, clear input lock and positive meter (`80069860`). A held button without a new pulse is not a request. The upper state for these movement selectors is a no-op, not an unavailable lane. |
| Stay in boost | Each lower boost-handler invocation calls the debit helper and sets the boost flag (`80063644`). Positive meter subtracts `50 × 5F4` and sets refill delay 16; the helper leaves already-empty meter and its delay alone. The phase clock starts at 30 source units and opens the exit latch, not an automatic timeout into cruise. |
| Exit to cruise | The exit leaf requires the latch and then accepts release or a nonpositive meter (`800698C0`). Entering cruise (27) clears the boost flag. The earlier refill pass can still see the old boost flag on that update. |
| Refill | Before selection, type 3 checks the boost flag, not ground or airborne status (`80055AA8`). Inactive passes decrement a nonzero delay without adding fuel, including the `1 → 0` pass. From delay 16, the first 16 eligible visits only count it down; the next can add 50 up to 10000. These are branch visits, not elapsed seconds or a guaranteed frame count. |
| HUD | The meter gauge reads live meter and bound actor capacity (`80013CEC`). It submits `1 − f32(meter / capacity)` and is not draw-ready at full. Empty yields sample 1, subject to actor suppression and view gates. No pixel output was verified. |

Cruise and boost both run vertical motion and can call the floor probe. A
checked null-continuation snap adjusted Y without selecting landing or
resetting the meter. That isolated response does not prove when a live
contact occurs, fixed altitude, ordinary landing, or ally boarding.

The qualified DOL audit matched **294 synthetic branch cases and one staged
scalar sequence**. The staged sequence checked refill before entry, held
depletion, exit after a supplied latch clock, 16 delay-only visits and the next
increment. It did not run an actor frame, integrate a route or draw the HUD.
To qualify the ordinary cycle, witness entry and release on both sides of the
latch plus held depletion with exact source input, clock, meter and flag
observations. Qualify floor and boarding behavior separately.

The detached native boost component reuses shared meter debit/refill, but
keeps the pre-selection refill, cruise/boost request, accepted-handler debit,
cruise entry and phase-one latch clock separate. The meter HUD component
returns only draw-ready and the source sample; it does not look up actors
or draw. The native comparison matched **301** qualified source-fragment
cases: 66 selector requests, 96 refill visits, 105 debits, six cruise
entries, 16 clock steps and 12 HUD samples. A separate synthetic unit
sequence checks the ordering through depletion and delayed refill. These
components do not handle earlier action arbitration, live input, pose,
contact, flight trajectory or actor admission.

## Normal Wing Soldier and Quick Valkrie: glide and contact

Exact normal `GG4E08:0A00/0` and `GG4E08:0B00/0`, AFS members 647 and 658,
install movement type 2 and resource policy 2. Their selector tables match
byte-for-byte; their motion banks, sampled pose continuations and class
actions do not. Neither report supplies an exact X Fly input-to-action path.
[Wiki leads: Wing Soldier](https://gotchaforce.fandom.com/wiki/Wing_Soldier?oldid=6424)
and [Quick Valkrie](https://gotchaforce.fandom.com/wiki/Quick_Valkrie?oldid=6425).

| Mechanism | Ordinary source finding |
| --- | --- |
| Glide request | From fall, held input and a clear lock can request 18 (`8006952C`) after earlier choices. Release can return certain glide selectors to fall, not every selector in the family. The ordinary generic air-dash request refuses type 2; this does not rule out class-owned transfers. |
| Floor offset | The ordinary pose-offset producer writes `6D0 = 0` for both (`8004D1E8`). The shared floor snap still uses `floorY − 6D0`. This is a logical root operand, not a measurement of the rendered body above the floor. |
| Snap and landing | Glide entry 18 installs a continuation that can select landing 7 after a valid snap. A transfer to steering 19 clears it; a snap without the continuation does not select landing. The landing entry (`80060228`), not the snap, clears airborne status and leaves `0x80` intact. |
| Refill | Before selection, the shared type-2 branch holds both meter and delay while `0x80` is set. When clear, it decrements a nonzero delay or adds 2000 up to 10000 if delay was already zero. It does not test floor contact, landing, the logical airborne flag or the cached ground field. Neither the ordinary snap nor landing fills the meter. |

The offline audit matched **282 source-fragment cases**, 141 per model.
Stage planes and completion fields were supplied to the tested tails; no live
surface query, animation clock, actor update or HUD draw was compared.
Next, witness exact fall-to-glide contacts with and without a landing
continuation, then trace each Borg's X Fly and any class-owned refill-inhibit
writers separately. There is no general hover-height rule or new gameplay
admission.

The detached `normal_type2_glide` component shares only the reviewed
normal Wing Soldier/Quick Valkrie scalar paths. It proposes the fall/glide
selector suffix and keeps the frame scalars, pre-selection meter refill,
floor callback, and phase-zero landing flags separate. Floor input must
declare a qualified static snap or an ordinary no-response return; it
does not generate a floor or accept a side approach. The floor tail
consumes the supplied continuation, not a landing guessed from height.
The native comparison matched **420 bounded source-fragment cases**:
128 selector requests, 24 frame-scalar steps, 192 refill visits, 64 floor
tails and 12 landing prefixes across the two exact models. Neither
character gains admission, shared clips or a shared X Fly action.

## Normal Beam Satellite: placement, stance and support

The `GG4E08:0E01/0` follow-up uses AFS member 681. It observes movement
type 9 and resource policy 1, with no local motion-bank data. This does
not remove class or model processing. [Wiki lead: Beam Satellite](https://gotchaforce.fandom.com/wiki/Beam_Satellite?oldid=6725).

| Mechanism | Ordinary source finding |
| --- | --- |
| Placement | The type-9 table path (`800579DC`) selects a complete XYZ row using stage code, team and a retained side index. A fresh placement updates the team's cursor; a repeat uses the retained side without advancing it. At raw stage code 0, team 0 has two pre-projection Y values, 1243 and 399. The common static and dynamic projection queries can change Y afterward. A separate per-slot fallback can add 2000 to a queried floor; that is not the height rule for the table path. |
| Idle and steering | Idle's two return-only movement states do not lock XYZ. With earlier checks clear, held steering requests selector 1, and the turn-completion path can select 2, whose lower handler integrates movement. Neutral steering can request 3 before returning to idle. This does not establish live movement timing or a reachable lower 27/28 boost path. |
| Air and meter | Class hooks reassert `0x40`; the resource pass uses cached logical airborne status, not height or boost inactivity. Ordinary landing selector 7 is disabled. Floor correction is not landing or refill, and no ordinary 0–3 movement debit was found. |
| Support and response | Three normal body descriptors carry the `0x20` bit used by a family-`0E` support path. That is eligibility, not registration or a live rider/parent relationship. Nominal weight 50000 enters the shared displacement rules; it does not confer displacement immunity. |

All **314 bounded offline cases** matched, including 256 placement-table
selections. These used supplied stage, cursor, steering and support fields;
they did not execute stage queries or an original actor frame. Next,
witness placement with actual projection returns, then held/released steering
with root and flag observations. Any live attachment or solid response
needs its own owner and contact evidence. Galactic Emperor `0E04` and other
Fortresses remain independent.

The detached Beam Satellite planner takes explicit XYZ rows for the current
stage and returns a **pre-projection** candidate. Fresh placement proposes
actor fields and one team cursor update; repeat placement proposes neither.
No source table is bundled. The steering suffix, turn-completion and braking
helpers return scalar proposals without integrating a trajectory. Airborne
flag writes and cached-ground refill remain separate; the per-slot fallback,
live projections and support links are not wired. The native comparison
matched **790 bounded cases**, including 640 table placements and 150
steering, handoff, flag and resource cases.

## Normal Galactic Emperor: class pin and placement

Exact normal `GG4E08:0E04/0`, AFS member 684, installs movement type **8**,
not Beam Satellite's type 9. Both use resource policy 1 and lack local
motion-bank data. This does not remove their distinct class actions.
[Wiki lead: Galactic Emperor](https://gotchaforce.fandom.com/wiki/Galactic_Emperor?oldid=6772).

| Mechanism | Exact normal source finding |
| --- | --- |
| Class position and heading | With constructor role byte `493 = 0`, the class initializer and both frame hook prefixes write **world `(0, 2000, 0)`** and heading `8192`. All reassert logical airborne status. Nonzero role bytes skip those coordinate writes in the inspected prefixes; the distinct preview route is not a battle start or a verified alternate form. |
| Shared placement | Ordinary construction invokes the class initializer **before** shared placement. Type 8 can then use the stage/team/side XYZ table and the subsequent static/dynamic projections. Its separate zero-authored-Y per-slot fallback adds **1000 to a queried floor**, compared with Beam's **2000**. None of these paths makes the class pin an arena ceiling, the final initial root, or a clearance guarantee. |
| Steering and actions | Only idle 0 and shared recovery selectors 9–11 are installed. The actual lane setters refuse Beam's steering selectors 1–3, ordinary jump and landing, and plane selectors 27/28. The class action exit's upper-only request for 27 also refuses, unlike Beam's. Neither this refusal nor the idle no-op proves that class actions or recovery are absent. |
| Body and refill | Five of 25 ordinary body descriptors carry the family support-eligibility bit, compared with three of 11 for Beam. This and nominal weight 50000 prove neither live attachment nor immunity to displacement. Meter recovery requires the logical grounded cache, not height or boost inactivity; the class pin itself is not a refill. |

All **116 bounded offline cases** matched, including both lane setters,
guarded class pose writes, the separate placement paths and refill.
They do not establish final live spawn, collision ordering relative to the
hooks, body clearance, special starts, child ownership, action outcomes or
mode admission. A live witness needs the constructor role, stage/team rows
and projection returns, then the root and heading around both class hooks
and the physical response. Do not borrow Beam's steering or placement
candidate as an Emperor actor receipt.

## Package implications and next checks

The [controller sidecar](borg-packages.md#standalone-controller-check) records
these independent observations per exact variant and form; it does not enable
gameplay. Reuse input conditioning, lane scheduling, stock and pose mechanisms
only where qualified, keeping resource gates, logical contact, pose mapping and
class actions separately owned. Camera rotation still must not force body
heading. No missing row inherits another Borg's rules.

Priorities are a normal Sasuke input/launch/dash/landing witness, an exact
Blue Striker boost cycle, and independent Wing/Angel floor-height and
Satellite placement traces. Then compare more same-tribe members and sample
individual class action guards, stock and effects. Verify Gold Hero's
transformed state, vehicle/robot toggles, temporary forms, copy effects, and
Galactic Emperor's special starts independently. [Wiki leads: Gold Hero](https://gotchaforce.fandom.com/wiki/Gold_Hero?oldid=6576)
and [battle fusions](https://gotchaforce.fandom.com/wiki/Category:Fusion_Borgs?oldid=6272).
Battle fusion is a separate two-owner transition, not collection assembly.
No assumption is made about form timers, state transfer, transformed or
alternate animation maps, or Challenge/Story admission.
