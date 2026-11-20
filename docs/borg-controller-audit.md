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
  separates boost inactivity from ground recovery. Wing Soldier and Quick
  Valkrie have offset floor probes, but minimum-hover-height contact and
  reset behavior still need witnesses. Beam Satellite's ordinary idle and
  disabled jump selectors do not prove fixed height or immunity to
  displacement. [Wiki leads: Quick Valkrie](https://gotchaforce.fandom.com/wiki/Quick_Valkrie?oldid=6425)
  and [Beam Satellite](https://gotchaforce.fandom.com/wiki/Beam_Satellite?oldid=6725).
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
Galactic Emperor `0E04` independently. [Wiki leads: Gold Hero](https://gotchaforce.fandom.com/wiki/Gold_Hero?oldid=6576)
and [battle fusions](https://gotchaforce.fandom.com/wiki/Category:Fusion_Borgs?oldid=6272).
Battle fusion is a separate two-owner transition, not collection assembly.
No assumption is made about form timers, state transfer, transformed or
alternate animation maps, or Challenge/Story admission.
