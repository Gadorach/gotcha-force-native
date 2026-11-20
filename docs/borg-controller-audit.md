# Normal Borg controller audit: one per retail tribe

The first pass found **eight movement-selector prefixes and 20 distinct
class-main callbacks** in a sample of one normal Borg from each retail tribe.
Eleven representatives share Sasuke's first 32 selector pairs byte-for-byte.
Several representatives that share a prefix differ in movement type, resource policy,
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

The first pass compared the first 32 pairs of the selector table (`4F0`),
with independent upper and lower lanes. This is not the full extent for the
later-audited Ninja and action rows. Source state `0` is a handler that does
nothing, not the `-1` sentinel for an unavailable lane. Tables compare
**representable states**, not reachable inputs or animation clips.

| Cohort | Observed distinction |
| --- | --- |
| A | Eleven byte-identical first-32 prefixes; ordinary selectors 0–17. |
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
install movement type 2 and resource policy 2. Their audited movement
prefixes match byte-for-byte; their motion banks, sampled pose continuations and class
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

The detached Emperor component keeps four independent boundaries: the
three class-pin prefixes, shared table arithmetic, the separate per-slot
floor-lift amount, and the exact upper/lower selector coverage. Beam now
uses the same table arithmetic behind its own identity check and retains
its distinct steering. Neither wrapper embeds original rows or performs
projection. Emperor native proposals matched **422 bounded source-fragment
cases**, including 256 table placements, 48 hook prefixes, six fallback
heights, 64 lane requests and 48 refill passes. These are not actor or
spawn receipts.

## Normal Gold Hero: temporary form and deferred return

`GG4E08:0802/0` is Gold Hero's normal retail selection. `0803` is the
verified destination of its in-battle form change, not a separate collection
entry or retail color. Both model IDs resolve to AFS member 622; the
transformed installer selects a second actor-data block in the same resource
slot. The selected retail variant still resolves the normal wrapper.
That does not establish the visible form or live collision rebinding:
different loose hit banks exist, and their handoff is unresolved.
[Wiki lead: Gold Hero](https://gotchaforce.fandom.com/wiki/Gold_Hero?oldid=6576).

| Stage | Source finding |
| --- | --- |
| Entry request | A qualified fresh press, clear action guard and positive channel-2 stock are required. Successful debit spends one charge and queues class state 4; neither the debit nor the request changes the form. The wiki names this move X / Gold Set Up; raw pad-to-command mapping was not tested. |
| Entered form | After program and motion processing, lower-lane completion gates the commit. The rebind prefix changes the model to `0803` on the same actor, preserves its selected variant, and chooses new class and data inputs. Later cleanup and initialization still belong to the transition before publication. |
| Expiry and return | The transformed initializer seeds 1800. Qualifying class-hook calls count it down. The last tick can queue a return on that invocation if the guard allows; otherwise the negative pending sentinel retries later. An accepted request queues state 5 without another stock debit; that state restores `0802`. This is not a verified wall-clock duration or immediate swap on expiry. |
| Controller and stock | Both forms use type 0, count policy 0 and the same selector table. The limit rises from **2 to 4**; movement operands and the effective pose default differ. Shared stock setup runs again, and sampled weapon dispatch and refill delay differ. A sampled rebind prefix preserving HP, root and used air points does not prove the full transition preserves those fields. |

All **84 bounded offline cases** matched. Completion, guard and actor
fields were supplied explicitly; no live animation, collision, device input,
action outcome or complete actor update ran. A live witness needs both
rebinding directions, accepted lower-lane completion, stock and used-count
observations, actual collider rebinding and a blocked-then-retried expiry.
Whether interruption cancels or refunds entry, stagger can trigger return,
or damage protection holds remains unresolved. This evidence neither
authorizes a separately deployable `0803` nor defines another Hero's form.

The detached native form helper separates the stock-debit request and
observed result, lower-completion-gated entry rebind, guarded expiry and
return rebind. It preserves the selected variant in the proposal and models
the raw revision byte separately from actor identity. The native comparison
matched **187** qualified source-fragment cases: 96 entry checks, 56 expiry
steps, 32 rebind prefixes, two form-data observations and the timer seed.
The helper does not run the stock owner, schedule an action, rebind pose or
collision data, publish a form, or infer interrupted state transfer.

## Normal Titan Robot: cyclic form and the separate Titan Tank

| Source identity | Retail and form boundary |
| --- | --- |
| `GG4E08:0604/0` | Titan Robot, collection #166, AFS member 578. |
| `0605`, carried normal wrapper | Titan Robot's in-battle tank form. It selects the second actor-data block in member 578 and is not a second collection entry or a color variant. |
| `GG4E08:0619/0` | Retail Titan Tank, collection #66, AFS member 591. It has its own normal art and motion bank and a distinct X weapon route. |

[Wiki leads: Titan Robot](https://gotchaforce.fandom.com/wiki/Titan_Robot?oldid=6455)
and [Titan Tank](https://gotchaforce.fandom.com/wiki/Titan_Tank?oldid=6437)
name the Transform and Javelin Shot moves; the identity and dispatcher
distinctions above come from source data and branch checks.

- **Selected cycle:** An admitted fresh conditioned X pulse goes through
  group 2 and chooses class state 2 in both `0604` and `0605`. Neither ordinary
  direction spends a transformation charge. Both checked floor-result
  branches choose program `4:0` and the opposite model; the model ID is
  staged in `548`, not a lifespan clock. Both directions wait for lower
  completion before the in-place rebind. No Gold Hero-style expiry was
  found on this cycle.
- **Return exceptions:** An exact `0605` callback with `5E0 & 0x02000000`
  requests `0604`; `0619` does not take that branch. A separate class
  state-1 leaf can also rebind to `0604`, but its live trigger and
  priority are not joined. Neither is evidence that all stagger or
  defeat conditions revert the form.
- **Controller and stock:** The cycle changes movement type **0 ↔ 1**,
  disables Robot-only selector rows in tank form, and rebinds its own
  pose inputs. The variant and actor storage survive the rebind prefix;
  shared stock setup runs again from the destination data. Retail
  `0619` uses a different X action and requests channel-2 stock debit
  in the checked weapon leaf, not an X transformation.

All **43 bounded offline cases** matched. They supplied contact and
completion results, so they do not verify input priority in a live frame,
full state transfer, actor/collider rebinding, interruption before commit,
or the actual retail Tank projectile. Verify both directions and the
flagged return independently; do not reuse the result for Proto Titan.

The detached Titan planner takes the conditioned X request, observed class
and floor return, and actual staged model/completion as separate inputs.
The flag-return prefix is separate from the X completion gate. Titan and
Gold Hero share only the scalar rebind prefix (target model, retained
variant and wrapping revision byte), not class actions, stock, pose clocks
or transfer rules. The native comparison matched **134 bounded cases**:
48 input leaves, four preparations, 64 cyclic rebind prefixes, 16
flag checks and two form-data observations. It does not enact the
unresolved state-1 return, rebind a live actor or admit retail Titan Tank.

## Cyber Machine Seiryu: two-slot battle fusion

The exact normal `GG4E08` battle matcher pairs **Machine Red `0600/0`**
(#164, AFS member 575) and **Machine Blue `0601/0`** (#165, member 576).
It stages **Seiryu `0602/0`** (#176, member 577) and **internal companion
`0603/0`** (same member, no separate nameplate). These are two reconfigured
owner slots, not a third combatant or a single-actor form alias.

| Boundary | Recovered source behavior |
| --- | --- |
| Request and role | The conditioned fusion command sets a short battle latch. The team scan matches two eligible, requesting slots, then stages reciprocal links, destinations and roles without changing current models. Red ordinarily gets the primary; human Blue takes it when Red is AI. The service's returned anchor and the primary role are distinct. This exact normal pair uses variant 0 for both destinations; non-normal inputs were not audited. |
| Synchronization and commit | The pair advances through linked readiness and approach stages. Both must reach stage 7 before reconfiguration is staged. The slot lifecycle then reloads both bound resources through the primary owner, retaining owner indices. That is not the scalar in-place rebind used by Titan or Gold Hero. |
| Active roles | Primary `0602` uses movement type 0 and its own controller. Companion `0603` uses type 7, admits only selector 0 among the first 32, has distinct class hooks, and follows the primary through the companion copy pass. Shared art does not make it a second freely moving Seiryu or a purely visual attachment. Fused initialization pools saved HP; ongoing damage and other state transfer remain unverified. |
| Expiry and split | The team timer counts **1800 eligible updates** from the last applicable initialization. Zero clears the active team state, but does not split the pair by itself. When the guards allow, each owner queues its own saved original model and variant, independent of which owner piloted Seiryu. Resource retirement and link cleanup are separate, not guaranteed by the request. |
| Pre-commit interruption | A separate branch can clear the two transition stages and team entry state before commit. It is not the active-fusion split or evidence that every interruption or defeat follows that path. |

All **44 bounded offline cases** matched. They covered latch, role assignment,
synchronization, timer, return requests, and selected stat/companion fields
without executing a battle. Live ownership, controller input priority,
full split, cleanup, and damage/stock transfer need separate witnesses.
Catalogue discovery writes are independent of battle ownership; no crystal
acquisition or drop rule was traced.
[Wiki lead: battle fusions](https://gotchaforce.fandom.com/wiki/Category:Fusion_Borgs?oldid=6272).
Do not extend these rules to another fusion pair.

The detached normal Seiryu planner keeps the request latch, two-entry match,
stage-seven reload proposal, eligible team clock and per-owner restore
separate. Match input requires the exact normal pair in two distinct slots
and a filtered team list; it does not implement the scan or Hyper fallback.
At expiry each guarded request refers to the caller's saved model, variant
and ancillary byte. It does not clear links or publish a split. The native
comparison matched **130 source-fragment cases**, including 36 latches,
eight assignments, six reload gates, eight clock cases and 72 restores.
No resource loads, state transfer or actor admission are wired.

## Machine Dragon: battle result versus catalogue entry

The exact normal `GG4E08` battle pair is **Machine Head `0511/0`** (#198,
AFS member 569) and **Mechanical Dragon `0512/0`** (#199, member 570).
It stages **primary `0513/0` and companion `0514/0`** in the same two
owner slots. Both resolve to member 571; catalogue Machine Dragon
**`050F/0`** (#200, member 567) instead has separate source identity and
class initialization. Similar art does not authorize that substitution.

| Mechanism | Source-backed comparison |
| --- | --- |
| Request and role | The battle team latch and matcher are shared with Seiryu. Head normally takes the primary role; human Mechanical takes it when Head is AI. The orchestration anchor can differ from the primary. Original identities are retained per owner, not reconstructed from the assigned role. |
| Entry and movement | Dragon uses lower handlers 57/58 for participant entry, not Seiryu's 31/32. The Dragon primary is type 4 and its fused-entry handler has a **local 120-count** fallback for channel-15 completion. Its companion is also type 4 but has its own config and class initialization, and admits only selector 0 among the first 32; Seiryu's companion is type 7. |
| Commit and timer | Both battle slots must reach stage 7 before the shared paired reload is staged. The active team clock counts 1800 eligible updates, independent of the Dragon-local animation timeout. Neither seed gives a wall-clock duration. |
| Return | A guarded expiry queues each owner's saved model and variant. For the later shared mode-4 cleanup, **nonzero `48A` skips** the block at `8005538C`; with `48A = 0`, it clears that slot's fused role and partner pointer before requesting single-slot reload. This corrects the polarity in the earlier private Seiryu audit. The request itself is not cleanup, and the shared cleanup is not an atomic split. |

All **38 bounded offline cases** matched. A complete reload lifecycle,
contact and weapon ownership, active damage/stock transfer, live split and
final cleanup remain unverified. The separate catalogue discovery path
mentions `050F`, not the battle models. No crystal part, acquisition rule,
inventory grant or new actor is implied.

The detached Seiryu and Machine Dragon helpers now share the verified
handshake and team clock behind exact pair checks. Dragon's channel-15
completion and 120-count fallback stay local to its primary entry. Mode-4
cleanup is a separate per-slot proposal on an observed pending tuple, with
the corrected `48A` gate. The native comparison matched **53** bounded
cases for Dragon assignment, staging, restore and entry, including 24
cleanup checks across both pairs. The **130** prior Seiryu comparisons
still match. No ledger publication, asset reload, split or actor admission
is wired.

## Revolver Gunman and Death Borg Beta II: shared movement, distinct shots

Exact normal **Revolver Gunman `GG4E08:0100/0`** (#21, AFS member 498)
and **Death Borg Beta II `GG4E08:0105/0`** (#79, member 503) install
byte-identical first-32 movement prefixes. Their class dispatch, primary
pose roots and projectile records differ. Beta's existing native initial-AI
construction and first-world evidence does not qualify human control,
later AI decisions, or combat. Gunman has no current native battle admission.

In the reviewed idle/no-target B branch, a fresh conditioned pulse selects
the ordinary class action. It does **not** spend stock at selection. The
later class handler requires a positive signed program event at `1CEF`;
this is an animation release event, **not a physical button release**.
It then attempts one channel-0 debit before invoking the shared projectile
constructor with the firing actor as owner. A repeat input checks
availability without spending another shot at that check.

| Reviewed difference | Gunman: Fast Shooting | Beta II: Shot |
| --- | --- | --- |
| Base-side class program request | Group argument 2, entry 4 | Group argument 2, entry 0 |
| Projectile key | 0 at base side, 1 at the other checked side | 56 at base side, 57 at the other checked side |
| Normal level-index-0 channel-0 row | Initial/max 20, mode 0, refill preset 120 | Initial/max 5, mode 0, refill preset 60 |

The archived [Gunman](https://gotchaforce.fandom.com/wiki/Revolver_Gunman?oldid=6547)
and [Beta II](https://gotchaforce.fandom.com/wiki/Death_Borg_Beta_II?oldid=6600)
pages supply those move names. Stock rows are original data, not a verified
visible reload duration. The bounded constructor records the firing owner
on successful allocation; its tested failure path does not publish a
projectile or refund the earlier debit. Both classes call a shared aim
primitive on other branches, but this B trace does not establish an aim,
body-heading, or target rule for all shots.

All **44 bounded offline cases** matched. Animation timing, live input
priority, later AI, X, charge/melee contexts, trajectory, hit and damage
remain unverified. No move or actor was admitted.

The detached basic-shot helpers keep the conditioned request, signed
program event, stock check/debit and projectile request separate. The
shared mode-0 stock planner copies the previous count on debit and loads
the refill preset only when that debit empties the channel. It does not run
the reload clock. The native comparison matched **114 bounded cases**:
16 action requests, 14 release gates, 16 projectile requests, eight repeat
checks and 60 stock cases. No live stock, actor or projectile is published.

## Patra Witch and Angel Nurse: distinct X contact outcomes

Exact normal **Patra Witch `GG4E08:0902/0`** (#48, AFS 636) and
**Angel Nurse `GG4E08:0900/0`** (#47, AFS 634) share the audited ordinary
movement prefix, but not their class dispatch or pose data. The reviewed
conditioned X path selects group 2 and class state `2/0` for both;
an earlier action can preempt it. Both use a channel-2 stock debit in
the audited branches, at different points.

| Move | Stock and contact | Recovered recipient behavior |
| --- | --- | --- |
| Patra Witch: Big Spell | Group 4 entry 0 emits event pair `(1,1)`. The class consumes the event and attempts one debit before requesting a caster-owned type-60 object. | That object's qualified contact can request a distinct type-33 effect owned by the **contacted actor**, not necessarily the stored target. Scale mutation, duration, refresh and complete team eligibility are unresolved. |
| Angel Nurse: Healing | The class attempts the debit during setup. In the audited fallback pose, a lane-gated program command requests a Nurse-owned contact shape; later visual events are not the contact producer. | The checked same-team ordinary branch adds **200 HP** to the **contacted actor**, capped at its own maximum, and refuses zero current HP. It is not a revival, party-wide heal, or guaranteed hit on the stored target. |

[Wiki leads: Patra Witch](https://gotchaforce.fandom.com/wiki/Patra_Witch?oldid=6420)
and [Angel Nurse](https://gotchaforce.fandom.com/wiki/Angel_Nurse?oldid=6419)
supply the move names. All **39 bounded offline cases** matched. They
supplied animation fields, contact owners and allocation results; they
did not establish full targeting, cast interruption, reload/effect
lifetimes, or live contacts. Neither actor gained admission.

The detached support planners reuse the mode-0 stock proposal and keep
Patra's release event, caster object and recipient effect separate. Nurse's
setup, lane-gated shape and ordinary same-team HP write also remain distinct.
A contact input requires the reviewed row and resolved recipient; a target
hint or cosmetic cue cannot give it one. The native comparison matched **55**
cases covering release, cast and setup results, shape lane, effect ownership
and HP/mode gates. No stock, object, collider, actor HP or effect is applied.

## Teleport Ninja: caster movement, not a swap

Exact normal **Teleport Ninja `GG4E08:0005/0`** is catalogue #20, AFS
member 490. Its audited 48-pair selector table differs from normal Sasuke
only at slots 12–14. Shared entries do not qualify its motion tuning or
admit an actor.

**Extended selector correction:** `0x2C` resolves to upper state 61 and
lower state 0 in the audited normal Ninja, Gunman, Beta II, Patra and
Nurse tables. It hands the upper lane to the actor's class callback;
it is not unavailable. The first-32 locomotion comparison does not
cover that row or prove the full tables match across classes.

For the reviewed ordinary X route, **Ninpo Teleport** uses a fresh
conditioned X pulse and attempts one channel-2 debit during setup. That
setup does not branch on the debit result. Its normal stock mode is 1:
a successful debit adds the refill preset to the existing stock clock.
The distinct charged-X dispatch is not the ordinary teleport.

The caster's goal normally comes from its own position history; a
guarded branch aims using a cached target point. Both move **only the
caster**, through iterative travel and the floor/contact path, not a
position exchange with the target. The action suppresses selected body
registrations and clears that bit on completion. This is not blanket
collision immunity or a guarantee about travel distance or elapsed time.
[Wiki leads: Teleport Ninja](https://gotchaforce.fandom.com/wiki/Teleport_Ninja?oldid=6693)
and [Switching Ninja](https://gotchaforce.fandom.com/wiki/Switching_Ninja?oldid=6659).
Switching Ninja `0009` dispatches separately; see the
[separate exchange audit](#switching-ninja-contact-staged-position-exchange).

All **34 bounded source-fragment checks and four staged vector checks**
passed. No live contact, charged Slash outcome, device input, or
interruption sequence was verified.

The detached Teleport Ninja helpers propose the history-based goal, first
step, iterative travel and completion flags for the caster only. The
directed-target gate refuses instead of borrowing the history path. Motion
outputs still require contact; the helper cannot establish immunity or
publish a root. Mode-1 stock now shares the stock planner but adds the
preset on each successful debit, preserving mode-0 behavior. The comparison
matched **40 source-branch cases and 40 staged vector cases**. No directed
normalization, phase scheduler, live contact, swap or actor is wired.

## Switching Ninja: contact-staged position exchange

Exact normal **Switching Ninja `GG4E08:0009/0`** is catalogue #115,
AFS member 494. It shares Teleport Ninja's installer family but has an
exact action/config override. [Wiki lead: Ninpo Change](https://gotchaforce.fandom.com/wiki/Switching_Ninja?oldid=6659)
names the move; the exchange below is source-backed.

- **Cast:** A conditioned X press selects the reviewed class action. A
  later positive signed program event attempts one channel-2 debit. On
  success the class requests a caster-owned type-20 object; neither the
  cast nor its cached aim point is a hit or exchange.
- **Contact:** The checked hit row 12 and receiver guards select the
  **actual struck actor**, not the lock-on target. The resolver stages a
  destination for each original slot from current positions at contact.
  Incoming X/Z come from the other actor; incoming Y adds the other
  actor's height reference minus one's own. Neither current root changes
  at staging.
- **Independent transfer:** Each actor's later callback can accept its own
  leg. A busy or defeated actor can skip it. Mode 2 clamps the queued
  X/Z to stage bounds, then writes current, previous and older position
  samples before a separate terrain query. There is no paired preflight
  or observed rollback. A successful leg does not prove the other ran.
- **Ownership and health:** Relocation does not exchange slot ownership,
  team, model, variant, HP or stock. The overall move is not HP- or
  stock-neutral: the cast spends stock and the hit can damage the
  recipient. The raw hit-row amount is 30, not verified final damage.

All **43 bounded offline cases** matched, including staged busy legs,
clamps, and copied history. Live contact, final terrain response, hit
damage and complete interruption/rollback behavior remain unverified.
No actor or swap was admitted.

The detached Switching Ninja planner stages both contact destinations and
then treats each actor's arm, countdown, clamp and history copy independently.
The placement proposal still owes a terrain query; no paired rollback,
damage or live actor writes are supplied. The native comparison matched
**80 bounded source cases**: 64 staging snapshots and 16 independently
processed transfer legs, including busy refusals and clamped destinations.

## Copy Man: contact-derived same-slot reload

Exact normal **Copy Man `GG4E08:0806/0`** is catalogue #44, AFS member 625.
The [archived Copy Attack name](https://gotchaforce.fandom.com/wiki/Copy_Man?oldid=6705)
corresponds to the reviewed **charged X release**, not ordinary X.

| Boundary | Recovered source behavior |
| --- | --- |
| Cast and hit | Exact program events `(1,1)` advance the class stage. With no existing head latch, the action requests a caster-owned type-85 head. No ordinary weapon-stock debit was found on this launch path. An eligible hit records the **actual contacted actor**; a stored aim target does not supply the donor. The hit's damage path is separate. |
| Request and reload | The reviewed ordinary contact consumer reads the donor's **current model and variant**. It queues a single-slot reload of Copy Man's own slot, retaining caster level and saved original identity. It does not clone the donor's in-progress pose, pointers, HP, stock, or ownership. The destination installer and caster-level rows regenerate pose, collision and stock bindings. |
| HP | The normal non-fused reload uses the caster's absolute current HP, capped at the installed model's maximum. Returning caps the then-current HP against the original maximum, rather than restoring lost HP or converting a ratio. The exact normal Sasuke donor sample is a bounded data witness, not a general donor whitelist. |
| Expiry | The copied-mode clock starts at **900 qualifying updates**. On a blocked expiry it retries from 1; an ordinary zero does not itself queue return. A permitted return stages the caster's saved original tuple and uses the same single-slot lifecycle, not a two-actor exchange or a new owner. |

All **56 bounded offline cases** matched. Special donor modes, exact
exclusions, live contact, resource retirement, interrupted return and
complete damage behavior remain unverified. This is neither Gold Hero's
fixed in-place form pair nor Switching Ninja's reciprocal movement.
No live actor or copy was admitted.

The detached Copy Man helpers separate head request, contact donor,
ordinary same-slot request, eligible expiry and observed re-entry HP.
The reload planner covers only the exact normal Copy Man/Sasuke witness;
it refuses other donors instead of substituting that witness. The native
comparison matched **95 bounded cases**, including release/contact,
outbound and return requests, guarded clock visits and HP clamping.
Donor live fields, actual resource loading, damage and actor admission
are not implemented here.

## Flame Dragon: type-4 takeoff and airborne dash

Exact normal **Flame Dragon `GG4E08:0500/0`** is catalogue #149, AFS
member 553. It uses movement type **4** and meter policy **1**. Compared
with normal G-Red, the inspected 48 selector pairs differ at **4, 12–14,
and 25–26**. That does not make the shared handler indices a pose map.

| Mechanism | Recovered ordinary path |
| --- | --- |
| Ground takeoff | A conditioned jump can select **4 → state 56** on both lanes. The checked idle-derived lower startup requests local program 2 and hands off to selector 5 only on program completion. Startup itself does not spend ascent meter. |
| Dash | Ground-dash rows 12–14 refuse without consuming the flick. An eligible buffered airborne flick can instead request **15 → 16 → 17**. Wind-up pays at the handoff; active visits debit separately. There is no ordinary Blue Striker-style 27/28 cruise/boost route. |
| Meter and floor | Capacity 10000, increment 2000, delay 0; ascent entry/update and dash handoff/active costs **100/25/200/15**, not G-Red's row. Refill checks the cached ordinary grounded state, not boost inactivity. A snap with no landing continuation does not request landing or refill. |
| Initial placement | The inspected type-4 floor-relative paths add **1000** to a queried height before later scene projection. That does not set a takeoff selector or establish hover height, final placement, or entry velocity. |

All **268 bounded offline cases** matched using supplied input,
program-completion and scene operands. Live motion, pose timing, contact
and weapon controls remain unverified. [Wiki lead: Flame Dragon](https://gotchaforce.fandom.com/wiki/Flame_Dragon?oldid=6670).
No other Dragon or actor inherits this evidence.

The detached Flame Dragon component models only the reviewed selector
leaves, idle-derived takeoff, wind-up debit and active-depletion handoff.
It reuses the shared meter calculation with Flame Dragon's own costs and
grounded refill gate. The native comparison matched **242** cases:
117 requests, eight takeoff/handoff cases, 48 charges, 36 refill visits,
nine wind-up steps and 24 active-debit outcomes. No VM, floor, actor,
other takeoff predecessor, or initial placement is wired here.

## Roach: one-use jump, control-only programs and bomb stock

Exact normal **Roach `GG4E08:0F05/0`** is catalogue #189, AFS member 691.
Movement type **1** does not make it Normal Tank's metered controller:
Roach uses policy 0 with a **one-use** jump count. Its full 48-pair
selector table differs from the checked Sasuke table at 12–17 only.
Unlike Flame Dragon, it refuses both ordinary ground and airborne dash
without consuming the flick at the refused selector.

| Mechanism | Recovered ordinary path |
| --- | --- |
| Jump and landing | Takeoff request and startup spend nothing; ascent entry consumes the single use. A count-zero fall can admit an airborne relaunch, unlike falling after an ordinary jump with count one. A floor snap is not a reset: the later eligible grounded bookkeeping clears the used count. |
| Empty motion bank | Both program roots are present; both pose-descriptor roots and the local bank are absent. Sampled movement programs write lane controls and run wait clocks without binding a local clip. Those controls can change the airborne flag or gate transitions, even without a clip. This does not establish that the rendered body never moves. |
| Normal B / Bomb | An admitted B uses upper class action ownership. A class phase requests one channel-0 debit, then can request a caster-owned type-16 bomb. The exact stock mode zeros the generic refill clock on debit; a checked bomb retirement callback returns one stock to the owner, capped at maximum. This is independent of jump reset and does not prove that every removal or failed allocation refunds stock. |

All **167 bounded offline cases** matched, including control lanes and
clocks as well as movement and bomb branches. Live movement, floor contact,
rendered pose, bomb behavior and the alternate constructor role remain
unverified. [Wiki lead: Roach](https://gotchaforce.fandom.com/wiki/Roach?oldid=6558).
No other Bug or actor inherits the findings.

The detached normal Roach helpers use shared air-point entry and grounded
bookkeeping, with both dash refusals kept local. The control samples are
metadata only: the current program asset loader requires descriptors, so this
does not fabricate one or add a second VM. Mode-2 stock clears the generic
clock on debit; one observed owner-bound bomb retirement proposes a capped
return, not a timed reload. The native comparison matched **89** bounded
cases across requests, entries, bookkeeping, program metadata, and stock.
No live control program, bomb, contact or actor is published.

## Normal Knight and Normal Samurai: shared movement, distinct melee

Exact normal **Normal Knight `GG4E08:020A/0`** (#26, AFS member 519)
and **Normal Samurai `GG4E08:0700/0`** (#39, member 606) share all
48 selector pairs and the three-use jump policy. Their movement operands,
class programs, and hit geometry differ. The shared jump count also does
not make their separate dash meter operands identical.

| Audited action | Recovered source behavior |
| --- | --- |
| Knight neutral X / Shield Slash | A negative first program event can arm the shield without launching. A later positive event can debit one channel-2 stock and request a Knight-owned type-24 hit object. Its moving spheres are distinct from the resident shield plane. The inspected return clears the owner latch; an owner hook can restore depleted stock. No live block, reflection or blanket refund was verified. |
| Samurai ordinary close B / Nitoryu | The close-route classification uses a conditioned input and cached target context, not a hit. The first strike does not debit stock. A lane-gated program command requests group-1 shapes directly on Samurai; a later opcode-17 event can create a separate presentation child. Actual contact supplies the receiver, not the selected target or that child. |

The archived [Knight](https://gotchaforce.fandom.com/wiki/Normal_Knight?oldid=6204)
and [Samurai](https://gotchaforce.fandom.com/wiki/Normal_Samurai?oldid=6412)
pages provide the move names. All **169 bounded offline cases** matched.
Registration and raw hit rows do not establish live overlap, final damage,
or combo hit count. No action, live guard, or actor gained admission.

The detached Knight/Samurai helpers separate the conditioned request, shield
arming and launch, owner-stock hook, direct melee group, and presentation.
The Knight launch carries a separate post-factory latch write; it does not
assume the factory succeeded or refund stock. Samurai's visual request
never replaces its actor-owned shapes. The native comparison matched
**106 bounded cases** across requests, shield events/stock, and melee and
presentation masks. No collider, stock or actor is changed live.

## Normal Tank and Battle Girl: shared metered routes

Exact normal **Normal Tank `GG4E08:0C00/0`** (#64, AFS member 666) and
**Battle Girl `GG4E08:0300/0`** (#34, member 525) share all 48 selector
pairs and meter policy 1, despite movement types **1 and 0**. Their
ordinary ground dash, ascent, relaunch, air dash, and landing use the
same inspected handler paths, not the same pose maps or motion tuning.

| Boundary | Verified difference or common rule |
| --- | --- |
| Ground dash ending | Both gate the release clock on a lower program-control byte. Tank seeds 10 and Girl 20, with different authored waits; a dash request does not start an unconditional wall-clock timeout. |
| Air costs | The ascent entry/update and dash handoff/active costs are **200/90/150/50** for Tank and **150/50/450/50** for Girl. Both refill on eligible cached-grounded visits, not on the floor snap itself. |
| Active air dash | The release latch uses **60 source units for Tank, 30 for Girl**. An open latch permits a direction release to enter recovery; held direction and fuel can keep the dash active. Depletion can enter recovery before the latch opens. |
| Ordinary neutral B | Tank's action takes the upper lane and preserves the existing lower movement selector. Girl's takes both lanes. This is a selector-ownership difference, not a measured movement or shot outcome. |

All **320 bounded offline cases** matched. Actual ground and airborne
routes, contacts, pose timing and live attacks remain unverified.
[Wiki leads: Normal Tank](https://gotchaforce.fandom.com/wiki/Normal_Tank?oldid=6435)
and [Battle Girl](https://gotchaforce.fandom.com/wiki/Battle_Girl?oldid=6750).
No actor gained admission.

The detached Tank/Girl component shares only the verified metered dash
arithmetic with Flame Dragon. It retains distinct cost and clock operands,
the ground program-control gate, the active release/landing latch, and
the ordinary B lane distinction. It matched **170** bounded source cases;
all **242** preceding Flame Dragon cases still match. The helpers do not
run an actor, program VM, terrain query, or weapon outcome.

## Pop Honey: contact-delivered targeting status

Exact normal **Pop Honey `GG4E08:0904/0`** is catalogue #50, AFS member
638. All 48 inspected selector pairs and the three-use jump policy match
normal Sasuke; the movement operands and pose data do not.

The reviewed neutral X spends one channel-2 stock during setup. The paid
marker gates the later voice sequence; the separate startup hit does not
carry its status. Caster cues can make an owned carrier request a
**collidable type-62 note** credited to Pop Honey. Sound and presentation
requests are not the status producer.

On an accepted ordinary row-5 contact, the **actual recipient** gets a
timer of **641 qualifying bookkeeping updates**, not a wall-clock duration.
A new accepted contact refreshes that value; the first application can
also request recipient-owned presentation. Sampled team checks allow
ordinary allies and opponents, but this is not a team-wide, target-lock,
or guaranteed self-hit effect.

While the timer is nonzero, the checked aim helper clears its active target
snapshot and the targeting helper swaps lateral sectors 2 and 8. This is
**not proof of full controller inversion**, allegiance changes, or healing.
Entry can clear the timer; another recipient flag can pause its decrement.
The note lifetime, caster's stock clock and recipient timer are independent.

All **138 bounded offline cases** matched. No live cast, contact, expiry
timeline, audio, or new actor admission was verified.
[Wiki lead: Lovely Voice](https://gotchaforce.fandom.com/wiki/Pop_Honey?oldid=6708).

The detached voice component models paid setup, the post-startup program
request, note group and caster credit, status guards and recipient-only
timer/targeting proposals. It matched **101** bounded cases covering
stock/paid state, note records, application/refusal, timer updates and
sector meaning. No note, damage, audio, target or live status is written.

## Death ICBM: steerable flight and guarded game impact

Exact normal **Death ICBM `GG4E08:0D06/0`** is catalogue #109, AFS
member 678. **Death Bomb `0D07/0`** is a separate model with a different
selector table and class branch, not an alternate name or variant.

| Boundary | Recovered game behavior |
| --- | --- |
| Placement and start | The inspected ordinary type-3 placement adds 2000 game units to a sampled floor height before later scene projection. This does not select fall. The ordinary active route promotes idle to steerable cruise/boost; the requested fall row is unavailable. Separate setup roles suppress the ordinary impact hook. |
| Flight | All 48 inspected pairs match Blue Striker. Boost has the eligible-input and release-latch gates, and eligible inactive-boost visits refill independent of grounding. This does not make their motion tuning, class termination or contact behavior interchangeable. |
| Impact and defeat | The ordinary active post-class hook checks lifecycle, phase, entry hold and qualifying contact from its inspected latches or queries. It is not triggered by a countdown or B/X. It then requests a caster-owned type-73 area object and queues the caster's own defeat. The defeat request is not conditional on successful allocation. |
| Interruption | Defeat already advanced to lifecycle 2 blocks the later impact hook. It does not prove that defeat cancels an area object already created, or that competing hits always resolve in the same order. The area's collider, lifetime and actual recipients belong to separate services. |

All **161 bounded offline cases** matched using supplied scene/contact
operands. Live flight, scene placement, contact priority, final area
coverage and damage remain unverified. [Wiki lead: Death ICBM](https://gotchaforce.fandom.com/wiki/Death_ICBM?oldid=6658).
No game actor was admitted.

The detached Death ICBM helper reuses verified type-3 scalar flight steps
with Blue Striker, keeps contact-query order explicit, and separates the
area-object request from the later pending-defeat consumer. It matched
**142 bounded source cases**, and **301** Blue Striker comparisons still
match after sharing the flight code. Nothing here queries a real scene,
creates an area object, applies damage or admits an actor.

## Death Bomb: steerable fall and stockless Drop

Exact normal **Death Bomb `GG4E08:0D07/0`** is catalogue #108, AFS member
679. Its type-10 selector table differs from Death ICBM's at fall 6 and
cruise/boost 27–28: Death Bomb admits the former, not the latter.

| Boundary | Recovered game behavior |
| --- | --- |
| Placement and entry | The inspected floor-relative placement adds 3000 game units before later projection. Entry code 2 separately selects fall; the height offset does not. Alternate setup roles do not qualify the ordinary impact hook. |
| Fall and ordinary X | The normal fall is steerable. The reviewed fresh X uses the class-owned **Drop** descent with no stock debit; it does not trigger impact. Its motion is not the ordinary lower fall controller. A floor snap or a landing request does not install an unavailable landing row. |
| Contact and defeat | The late lifecycle/contact gate matches the reviewed Death ICBM gate, independently of whether the actor is falling normally or using X. It requests a different caster-owned effect, **type 5 / argument 17**, and queues the actor's own defeat even if allocation fails. Already-committed defeat blocks the gate, but an already-created effect was not shown to cancel. |

All **211 bounded offline cases** matched. Final placement, contact and
hit outcomes, live interruption and effect lifetime remain unverified.
[Wiki lead: Drop](https://gotchaforce.fandom.com/wiki/Death_Bomb?oldid=6657).
No actor was admitted.

The detached Death Bomb planner shares only the verified game-impact
and terminal-consumer gates with Death ICBM. Entry code, ordinary
fall steering, stockless Drop and history restore stay model-specific.
It matched **191 bounded source cases**; all **142** preceding Death ICBM
comparisons still match after sharing the gate. No live motion,
world query, impact object, or damage is applied.

## Acceleration Ninja: steerable stance with actor-bound contact

Exact normal **Acceleration Ninja `GG4E08:0004/0`** is catalogue #19,
AFS member 489. The reviewed fresh X enters a **stockless stance** on
the existing actor. It does not change the model, selected variant, or
owned identity, unlike the audited form and reload paths.

| Boundary | Recovered game behavior |
| --- | --- |
| Start and steer | The class requests startup program `4:0` and separately requests a visual trail. Its active branch uses conditioned steering and can continue through a successful floor probe. The trail is not the carrier of the hit shapes. |
| Passive contact | A lane-gated command in the startup program requests two **actor-bound** shapes in group 10. Their activation follows animation progress, not planar speed. Actual admitted contact supplies the receiver, not the aim target. Ordinary air dash has a separate group-11 path; neither is an always-active body hit. |
| Exit and interruption | After startup lockout, a fresh X edge requests recovery; timeout and insufficient movement can also do so. Changing the program can retire the old shapes through their own updater. Committed defeat replaces the stance, but same-update hit priority and rollback are unverified. |

All **146 bounded offline cases** matched. Live steering, exact
animation/contact windows, final damage and interruption remain
unverified. [Wiki lead: Ninpo Full Throttle](https://gotchaforce.fandom.com/wiki/Acceleration_Ninja?oldid=6692).
No actor was admitted.

The detached Acceleration Ninja helper proposes separate stance clocks,
fresh-edge exits, trail updates, and actor-bound shape requests. Shape
activation uses the reviewed lane's progress; a changed program or
retiring carrier can remove the old shape independently. The native
comparison matched **119** bounded cases. No live motion, VM, collider,
damage, trail, or actor is published.

## Demon Samurai and Normal Samurai: shared slash, different shapes

Exact normal **Demon Samurai `GG4E08:0701/0`** (#186, AFS member 607)
and **Normal Samurai `GG4E08:0700/0`** share all 48 selector pairs and
a three-use jump count. Some movement parameters differ, and the
verified X route is not Normal Samurai's previously audited close B.

Neutral X selects the shared slash driver without spending stock or
changing model or variant. The driver uses each Borg's own record and
programs. In the checked neutral route, program `4:1` requests group 7:
**three actor-bound capsules** for Demon Samurai, **one actor-bound
sphere** for Normal Samurai. Their later group-8 shape sets and visual
cue timing also differ. A visual child or trail does not carry those
hits; the collision resolver supplies the actual receiver, not the
press-time aim target.

The inspected control branches use posture, animation progress and
fresh button edges. [Random Slash](https://gotchaforce.fandom.com/wiki/Demon_Samurai?oldid=6580)
is the archive's move name; no RNG-based choice was verified in this
route. All **249 bounded offline cases** matched. Live pose timing,
contact, damage and interruption remain unverified. No actor gained
admission.

The detached X planner keeps shared button-transfer counters and the
lane-event handoff separate from model-specific program, shape-descriptor
and visual-key requests. It matched **142 bounded source cases**. This
does not run the class VM, generate world geometry, add damage, or
alter the separate Normal Samurai close-B planner.

## Kung-Fu Master: close-B priority and paid Spirit Shot

Exact normal **Kung-Fu Master `GG4E08:0800/0`** is catalogue #41,
AFS member 620. All 48 selector pairs and the three-use jump policy
match Normal and Demon Samurai; movement operands and pose data differ.

- **One B edge, two candidates.** The ordinary conditioner can publish
  a close and a shot pulse. The close route tries first when its guarded
  target context is eligible. That classification is not a hit; the
  close-strike outcome was not audited.
- **Choice, availability, release.** The checked neutral shot stages
  action state `0/0` and takes both lanes. Its entry stock query is
  check-only. The later program event pair `(3,1)` attempts one
  channel-0 debit in the release subphase. Successful debit adds to the
  mode-1 stock clock and requests projectile key 12. A refused debit
  creates no projectile request; the release subphase still advances.
- **Separate owners and clocks.** The projectile is the spatial carrier,
  Kung-Fu Master gets credit, and the receiving collider determines
  the actual recipient. The hand presentation is separate. The
  projectile lifetime and stock refill are independent, even though
  the checked source operands are both 300.

All **185 bounded offline cases** matched. Actual aim, contact, damage,
hit timing, close/air actions and interruption ordering remain unverified.
[Wiki lead: Spirit Shot](https://gotchaforce.fandom.com/wiki/Kung-Fu_Master?oldid=6583).
No actor gained admission.

The detached Kung-Fu planner models the scoped request priority,
check-only stock, exact release cue and separate hand presentation.
The shared mode-1 refill visit preserves a negative overshoot when it
restores the cap, unlike mode 0. It matched **121 bounded source cases**.
No class VM, projectile allocation, hit test or actor update is wired.

## G-Red: original neutral B release

Exact normal **G-Red `GG4E08:0615/0`** (#1, AFS member 587) does not
fire its original beam on the native duel's provisional cooldown rule.

| Boundary | Recovered source behavior |
| --- | --- |
| Request | A fresh B edge can generate close and ordinary shot pulses. An admitted charged-B request or close-B request precedes the neutral shot. The scoped ordinary row selects both lanes as 44, giving the upper class action and lower no-op. |
| Pose and release | Local group 2 slot 0 uses directional pose records and a separate authored release cue. The release requires the cue **and** an aim pass or local timeout. Timeout without the cue does not fire; holding a pending cue pauses the inspected VM call, not the whole actor. |
| Stock and beam | Entry availability is check-only. The release attempts one channel-0 debit, then requests object argument **43** only on success. An allocation refusal follows the debit and is not a verified refund. |
| Contact | The beam carries the geometry, G-Red owns credit, and the receiving collider supplies the actual recipient. The aim snapshot is not a hit. The separate object lifetime does not measure stock refill or live range. |

All **134 bounded offline cases** matched. A live witness still needs
one exact normal slot from fresh input through accepted VM cue, aim or
timeout, stock, beam allocation, and actual contact/HP outcome. Neither
native cooldown output nor the source fragments qualify that route.
[Wiki lead: Beam Gun](https://gotchaforce.fandom.com/wiki/G_Red?oldid=5897).

The detached neutral-B planner models the bounded request order, fresh-idle
program and directional descriptors, VM step hold, post-VM aim/cue gate,
mode-0 stock and argument-43 beam request. It matched **169** source cases.
The new mode-0 refill proposal requires an already armed stock clock and
clears it at expiry. No live pose, contact, allocation or duel shot changed.

### Headless G-Red cue and no-target request

The locomotion profile still lacks the shot program and is unchanged.
`--kind neutral-b` exports a separate 436-byte GFMP3 slice: exact idle
`P0:0`, shot `P2:0`, and nine descriptors (`D0:0/1/2`,
`D2:0/1/2/6/7/8`). The headless check loads all nine clips from the
exact normal bank without fallback. Generated profiles stay local.

With a settled idle seed, zero steering, effective lane mask 3 and step 1,
the shared `CharacterProgramVm` and directional driver reached the first
accepted upper `Control0=(1,0,1,0)` at source `80366664` after **16 VM
updates** following the shot request. The opening `(0,0,1,0)` did not
release. At that cue both lanes had wait/cursor `0x40/0x44`, centre/side
descriptors `D2:6/7`, clips `3:11/3:12` at frames `1/1`, and separate
endpoints `35/36`. Candidate model bytes were `4/0`. Only a local pose
was composed. The step count is for this fixture, not original gameplay
timing. The existing ordinary preview and movement-only pose receipts
were not widened.

The same check refused a foreign VM scope and a missing zero-weight
release-side clip, leaving the previous pose and cue state intact.

The no-target aim path (`6D0DC` with flags `193`, offset `0`, and
active target `CC` already null) clears both aim references, sets
the aim headings to the supplied body heading, and passes with zero
angular error when the signed turn allowance is nonnegative. It
does not turn the body. A selected neutral action row is not evidence
of a null active target; the fixture declares that input explicitly.
The helper matched **160** bounded original-code cases and refused all
**64** tested negative-wrap cases. Targeted aim remains out of scope.

With that path and the actual VM cue, the fixture proceeds to the
detached channel-0 debit. The opening control write requests nothing
even when aim passes. At the authored cue, the check-only count of 5
becomes a proposed count of 4 with a type-2, argument-43 request.
Independent last-round and empty-stock alternatives arm a 180-unit
refill and refuse the request, respectively. No actor or stock is
updated, and no allocator is called.

The same fixture now evaluates the argument-43 muzzle inputs from the
accepted cue pose, at a declared unit-scale placement. The initializer
reads the **parent's key 7** to transform the local point
`(-106.8, -3.6, 30)`. With no copied target it takes the **credited
actor's key-0 +Z column**, normalized, for the bearing instead of
using key 7's direction. The direct fixture uses slot 3 for both roles,
but supplies the keys separately. **128** bounded source-routing and
numeric comparisons matched; normalization used a host reference, not
paired-single hardware parity.

Run from the repository root with `DOL` set to the qualified initialized
executable and the local MVP and practice packs already present:

```sh
work=$(mktemp -d)
python3 -B -m tools.extract.gred_locomotion "$DOL" "$work/neutral-b.gfmp" --kind neutral-b
c++ -std=c++20 -O1 -I . tests/host/gred_shot_cue.cpp -o "$work/gred-shot-cue"
"$work/gred-shot-cue" local/content/native-battle-mvp-v1 "$work/neutral-b.gfmp" \
  local/content/source-gred-ground-v9/pose-height.gfhc local/content/source-gred-ground-v9
```

Omit the final directory to run without the optional stage-grid fixture.

A test cue, request, or fixture muzzle does not authorize a beam.
The original allocates the object first, and its initializer gets
successful shape registration before it reads the matrices. A live
witness still has to join the accepted actor cue, aim or timeout and
stock debit to the allocated object's generation, registration and both
owned transforms at initializer time. The cue's pose alone does not
establish which matrices were live at that time. Contact, actual
recipient and HP changes need separate evidence.

#### Staged ordinary-B animation visit

The portable normal G-Red visit now owns a checked local animation
library: the exact model, rig and motion bank, neutral-B `GFMP3`, and
pose-height profile. It uses the shared VM, directional controls and
pose sampler. The admitted phase-1 visit is neutral steering with no
target and no root-control consumer. It stages the two bound lanes,
local pose, and aim/release gate together; a late aim refusal cannot
publish a newly raised cue. A pending cue visits with VM step zero,
not a second cue. Only a staged due cue can propose the one-round
debit, including last-round and empty-stock alternatives.

The headless fixture still reaches the first cue after 16 step-1
updates. It refuses foreign controls or motion libraries, altered
inputs, a missing side clip and a targeted visit. Neither a staged
pose nor a debit proposal commits native actor state or allocates a
beam; the battle owner must accept the staged visit and perform that
join in its current update.

#### Native stationary neutral-B trial

The opt-in host trial joins that visit to the existing native ground
controller and renderer. It requires a freshly bound stationary
normal G-Red/Sasuke world, the exact local B assets, and explicit
initial trial stock. The native world first accepts an unchanged
ground position and heading. Only the matching offer receipt then
publishes the staged B pose and, on the cue, updates the trial's
stock and exposes the paid type-2/argument-43 request. The first
frame opens the shot; the fixture reaches the cue on native tick 17.

Both a full-stock and last-round trial stop after that single request.
Pause retains the last accepted pose and stock. A mismatched offer
serial cannot publish the pending release, even if the same native
position was accepted.

The paid trial can hand off allocator inputs once, only after the
same accepted pose is published by the same current renderer. That
handoff exposes class 3, group `8` resolved to `9`, kind `16`, and one
descriptor for slot 0. It samples no bones and supplies no allocator
outcome. Republishing the same tick invalidates the old handoff without
making another available. The numeric stock readout is not a handoff.
The host trial passes **327** checks in native and sanitizer builds.

The trial uses the shared pose owner and stock planner, but is not in
the duel, pickers or Story. It does not resolve a live target, continue
recovery/reload, allocate a beam or collider, apply damage, or write
a save.

With the same local inputs and the exported `neutral-b.gfmp` above:

```sh
c++ -std=c++20 -O1 -I . tests/host/gred_neutral_b_native_trial.cpp -o "$work/neutral-b-trial"
"$work/neutral-b-trial" local/content/native-battle-mvp-v1 \
  local/content/source-gred-ground-v9 "$work/neutral-b.gfmp"
```

### Argument 43: allocation and shape ownership

The detached argument-43 checks distinguish two original allocations.
On a paid request, `C3BE0` supplies the credited actor to the class-3
allocator, base group 8, head insertion, and one scene descriptor.
Direct normal caster slot `s` selects group `9+s`, kind 16. A null
return proposes no header and does not undo the debit. The cue fixture
now checks those allocator operands but supplies no allocation result.

On the new beam's first dispatch, `84000` checks the credit's lifecycle
and model before calling the initializer. Credit phase 2 or higher, or
a model mismatch, skips registration. Otherwise it seeds phase 1 and lifetime
300, then calls `8AC80(beam,0)`. That call reads the credited actor's
bank and requests a **class-4** allocation in base group 48, resolved to
`49+s`, kind 26, with no scene descriptors. A registration refusal
leaves the 300 seed, stages phase 2, and skips the muzzle branch.

For the exact normal G-Red bank, attack group 0 references descriptor
15, hit row 0, and local radius 20. The shape's spatial carrier and
transform (`beam+114`) belong to the **beam**, not the credit actor
or its key 0. The credit still supplies the bank and hit table.
These branches use declared allocator outcomes; they do not create
leases, run removal, insert a collider into a live registry, or
identify a contact receiver. **132** bounded factory, dispatch and
shape-owner comparisons matched, using the exact credited bank for
shape operands. The playable duel still does not use them.

### Argument 43: authored beam art

The argument row has `+42 = 0`: `C3D2C` selects the common effect
bank at `803CB32C + 48`, not a record from G-Red's model bank.
`42878` loads AFS member 697 (`efct.pzz`); ordinary mode `29 = 0`
selects model block 1. The `726C` call selects record 0, and
`86B8` requests its first material clip at time 0 for team 0.
The alternate effect bank and other team selectors remain separate.

The qualified local bank loads through the shared HSD record and
animation systems. With an explicitly supplied beam matrix, the
headless render test submits both authored blended meshes (708
vertices), with no textures or provisional billboard. This verifies
the record's draw path, not a screenshot, live carrier, material
update schedule, or collision.

Given the local `efct.pzz` extracted from the original AFS:

```sh
work=$(mktemp -d)
python3 -B -m tools.extract.gred_beam_art "$EFCT" "$work/beam.hsd"
c++ -std=c++20 -O1 -I . tests/host/gred_beam_art.cpp -o "$work/gred-beam-art"
"$work/gred-beam-art" "$work/beam.hsd"
```

### Argument 43: untargeted flight prefix and lifetime

The detached initializer math adds the unit bearing to the muzzle
point **before** scaling the motion vector to 80. With `C8` null,
the active prefix normalizes the prior motion, moves at the **old**
speed, then adds 0.5 up to a cap of 160 for the next visit. The cue
fixture goes from distance 1 to 81 from the muzzle in that numeric
sample; these are source units, not device-frame speed or live range.

Retirement has a separate order: the `83714` six-slot helper result
precedes latch mask `1D9 & A2`; both precede the stage segment and
bounds checks. A stage stop also requests a distance refresh from the
stage-corrected position. Only an all-clear visit subtracts a lifetime
count. The earlier stops write retirement marker 1; bounds and expiry
write marker 2. An unobserved query cannot be treated as clear.
**285** bounded comparisons matched with host vector math and supplied
query outcomes. The proposals neither run the scene queries nor activate
or remove shapes; targeted steering and modifier-dependent update tails
remain out of scope.

### Argument 43: stopping, hiding, and releasing the carrier

The phase-1 stop writes `1D8`; that marks the class-4 child for its
own `8A65C` retirement visit. It does not free the class-3 carrier.
Phase 2 (`C426C`) dispatches by marker: 0 hides immediately; 1
retracts `184` by the current speed without moving the position;
2 drifts by the **stored** motion vector while raising the scalar
speed and fading `B4`. This last branch updates visual scale but
does not rewrite the physical `C0/C4`.

Hiding writes phase 3 and clears `82`. On that update an existing
late `C44A8` registration can still write the affine matrix, including
the negative axial scale from the final retract. This is not active
collider geometry. The later phase-3 callback is what asks `88E50` to
unlink the carrier, drop its descriptor/registration and return the
slot. No native allocation or release is acknowledged here.

**3,841** bounded dispatcher cases matched, including **1,280**
late-frame and child-stop checks. The comparison used aligned
distance fixtures with host vector math and stopped at pool release.
Live retirement must still join the actual child, callback
registration, descriptor and carrier leases; none can be inferred
from a phase, visibility byte, or matrix.

### Argument 43: static stage segment

`84170` clips the current and previous endpoints separately, in X-then-Z
order, each against the **other original endpoint**. For a long-enough
segment, `3E978` starts at the clipped **previous** endpoint and searches
the tertiary, secondary, then primary grid. It keeps the strictly
nearest hit across all three, so an equal-distance hit in a later grid
does not replace it. A hit proposes the surface position; a miss keeps
the unclipped current position. Missing grids or a degenerate clip are
refusals, not computed misses. The existing secondary-only spawn query
keeps its own entry point and behavior.

The optional headless fixture loads all three exact stage473 grids with
declared X/Z bounds of `[-4500,4500]`. The cue's flight segment tests
seven triangle references and misses. A separate vertical segment
from `(0,100,0)` to `(0,-60,0)` hits primary triangle 2301 at
`(0,10,0)`. **138** bounded original-code comparisons matched for the
clips and query gates; the strict-nearest branch was checked separately.
Neither scene fixture is a live beam trace.

### Argument 43: detached phase-one stop chain

The `83714` helper visits six scene slots in order. It reads position
only for an enabled, non-null slot passing the scene's optional bitmask.
The first far slot increases the nudge weight by 3, capped at 100.
Every far contribution uses the same post-motion beam position and
that same weight. A slot at distance `<=150` stops the scan: it keeps an
earlier weight increase but discards the accumulated positional nudge.
This is not a collider hit or an actor eligibility rule.

The detached untargeted phase-one check connects those ordered slot
samples to the latch, the three-grid segment, and `83244`'s point
bounds check. The latter allows 750 extra units in X/Z and above the
declared ceiling, with a fixed lower Y limit of `-1000`. Exact edges
are inside. A stage stop replaces the proposed position and refreshes
muzzle distance, without running the bounds check or decrementing
lifetime. A nudge alone does not refresh the earlier flight distance.
**720** six-slot and **63** bounds comparisons matched the bounded
original fragments; vector math used a host reference.

With declared empty slots, prior position and ceiling, the stage473
cue fixture reaches the all-clear branch and proposes `300 -> 299`.
The vertical stage hit refreshes muzzle distance to 90 and retains
300. A separate out-of-bounds probe also retains 300. This does not
admit the beam into a scene, run the stage effect or modifier/scale
tail, update a collider, or resolve damage.

### Argument 43: beam matrix and separate shape scale

`C44A8` builds the beam's own `+114` matrix from **current position
minus muzzle origin**, not from its motion vector. It tries mode 5
with +Y as the reference, then mode 4 with +X. If both fail, it keeps
the supplied prior columns. It still applies the explicit scales and
writes the current translation.

The seed scale is `parent B4 × (40 / 36.96)`. The unmodified active
tail keeps physical `C0` separate from `min(distance, 1000) / 22` on
the matrix's Z axis. The fixture's first flight step gives `C0 ≈ 1.082251`
and axial scale `≈ 3.681818`. **212** bounded original-code cases matched
the scale, basis selection and matrix writes using host vector math.
The first sphere initializer reads the carrier's `C0` for its physical
radius, not the beam's axial scale. No collider update is wired.

The exact descriptor's flag 1 also reaches a separate activation write
guarded by `1CA`. In **16** bounded source cases, the null-target
branch used `1CA` to suppress repeats, assigned a nonzero wrapping
token to beam `26C`, and incremented the credited actor's `404`
counter. These are initialization bookkeeping writes, not hits or
HP changes. They remain unwired.

### Argument 43: fresh shape activation and list gate

The detached fresh-shape tail now joins those bookkeeping writes to
the shared `8AA40` matrix filter and `8A2BC` list planner. This entry
requires the exact normal beam shape operands and a declared shape-0
initialization visit. The activation writes (`1CA`, `26C`, `2AD`,
scene token, and credited `404`) precede the column-length filter;
a degenerate matrix can skip list admission without undoing them.
The scene token skips zero on wrap and is not a native allocation
generation.

For descriptor flag `0x0001`, the only candidate list is `477C`,
subject to the beam carrier's suppression and disable fields. The
carrier supplies `+114` and registration flags; the credited actor
supplies the peer fields and credit counter. The declared freeze
byte can select phase 2 without repeating activation. **768** bounded
original-code comparisons matched, using host column-length math. Nothing here
initializes or advances sphere geometry, writes the shared lists or
counters, establishes a live collider lease, or applies damage.

### Argument 43: same-update world scheduling boundary

In the ordinary `29434` update with scene `66` clear, `53A2C` runs
the actors before `89A94(8)` walks the beam groups. `54070` can finish
the actor's `465FC` world update after the actor phase callback, so
the matrices used by the beam initializer are not snapshots taken at
the release cue. A class-3 beam inserted during that actor pass can
be visited later in the **same** update. `C3D2C` can allocate its
class-4 child and build beam matrix `114` before `8A28C` resets the
registry counts, `89AEC` walks the colliders and `2BB14` uses the
resulting lists.

`8997C` visits the slot-biased carrier group `9+s` when the physical
actor is active and its `D1` bit is clear. It copies position `20`
and angles `70` into `2C/76` before the active carrier callback,
then clears `1CA` if the **post-callback** `86` byte is nonzero.
Thus `1CA` is not a once-per-lifetime flag. The class-4 bucket
`49+s` is separately reached by `89AEC` when the physical actor is
active, even if `D1` skipped the carrier. A new registry pass does
not necessarily imply a new beam transform.

**5,832** bounded dispatch comparisons matched, including injected
same-update insertions and retained-shape cases, with actor, beam,
collider and contact callback bodies stubbed at their boundaries.
The planner does not walk, allocate or publish a live object.
The remaining join must bind a paid release to the current actor
world and exact class-3 lease, then bind its class-4 child to that
carrier's current or source-retained `114` in the registry pass.
A slot, beam token, or numeric matrix alone is not that evidence.

#### Native practice matrix ownership

The accepted movement frame now owns its immutable local pose and
original renderer binding. A copied pose packet supplies no editable
bone data, and retagging its public binding cannot carry the old pose
into a reloaded renderer. The shared view can inspect keys 0 and 7
together only after a successful publication for the exact current
native tick, producer frame, and render instance. A newer movement
receipt alone, a same-model neighbor, a legacy pose, or a retired
producer does not publish either key.

These are diagnostics from the real movement-practice renderer, not
the source combat actor's post-release matrices. The ordinary-B VM,
paid release, and class-3/class-4 owners still need a common battle
update; the practice matrices cannot satisfy that join.

### Argument 43: admitted normal Sasuke receiver prefix

In the scoped ordered pair, the normal G-Red beam uses attack descriptor
15/row 0 and normal Sasuke supplies body descriptor 9/row 0. The latter
is a sphere on logical binding 2. `2E2A8` reads the attack carrier,
attack credit, and actual receiving body independently; both the beam
and G-Red record the receiving body at `+290`. No aim pointer supplies
that receiver.

For this pair the two row prefixes skip the special modifier branches.
The ordinary receiving kind adds beam latch `0x10`. G-Red receives
the resulting latch only on the explicit `beam+2AC & 1` branch.
`0x10` alone does not satisfy the beam's earlier-audited `0xA2`
retirement mask. The prefix also copies the declared contact vector,
records the beam's `26C` token in the receiving body's 48-entry history,
and stages a small set of row and binding fields on Sasuke.

All **768** bounded source-prefix comparisons matched. The proposal
starts from a declared already-admitted pair. It does not query
overlaps, determine whether a token is a duplicate, apply the reaction,
write HP, or prove that a beam pierces a body. The current duel does
not use it.

### Argument 43: ordinary amount and HP proposal

For the same declared normal pair, the `3CD5C` amount starts at
attacking row 0's **20**, then applies the beam's relative `C4`, source
table factors, and the receiver's scale. The checked matchup table
entry is 1; the other factors must be supplied explicitly. Binding 2's
mask or the ordinary `1DA & 2` branch picks zero before those reads.
This does not define a universal hit amount for G-Red.

All **384** bounded source amount and writer comparisons matched.
In one declared normal fixture (HP samples 200/180, unit scales,
zero difficulty/rank/tier indices, and force ratios `0/100`), the
resolved amount was **19**, proposing Sasuke HP `180 -> 161`.

`3D344` is a separate writer. In the checked ordinary case it records
the prior HP and clamps subtraction at zero. A held scene or already
zero HP skips the write, even if the amount was computed. This is a
detached numeric proposal: live table selection, collider admission,
credit statistics, reactions and KO dispatch are not joined.

The local `GFDA` profile now supplies the nine coefficient rows for
the declared level-zero, unbuffed 0/1-team scope. The bound entry
selects factors from the declared HP **history**, force numerators
and caps, difficulty, rank, and tier. It applies the source receiver
projection (`max(0, history - 20)`) before choosing its HP row.
History is separate from the HP value the writer would change. Missing
tables or out-of-scope selectors refuse rather than substituting unit
factors. The explicit zero branches still bypass table lookup.

Run the detached amount fixture from the repository root with `DOL`
set to the qualified executable:

```sh
work=$(mktemp -d)
python3 -B -m tools.extract.gred_sasuke_damage "$DOL" "$work/ordinary.gfda"
c++ -std=c++20 -O1 -I . tests/host/gred_sasuke_damage_replay.cpp -o "$work/damage-replay"
"$work/damage-replay" "$work/ordinary.gfda"
```

The 600-byte profile stays local. **1,178** comparisons matched the
exact coefficient reads and bounded original HP fragments. The
current fixture also checks the fresh-sphere geometry described below;
it still does not write live HP.

### Argument 43: fresh sphere overlap on Sasuke body 9

The detached check follows the exact `2DB58` list pairing (`477C`
attacks into `417C` receivers). It checks the ordinary receiver flag,
then `2FD7C`'s repeat-token scan of **all 48** entries before receiver
mode-2 suppression. The token history is not bounded by its write
cursor. This is one declared pair, not a scene list traversal.

For the exact first-phase kind-0 spheres, `39F6C` rejects broad
radii `<= 0.1`, and tangency is inclusive. `30500` reports the point
on the **receiver's** sphere in the beam's direction from its center;
coincident centers keep the receiver center. **442** bounded source
comparisons matched the gates and point with host vector math.

The headless table replay now computes the detached overlap at
`(22,0,0)` and then proposes `180 -> 161`. A one-unit gap and a
repeated token do not reach the receiver proposal. Sphere centers
and list memberships are still declared fixture inputs, not current
scene leases. A moving flag-1 beam can switch to kind 2 after travel
greater than `0.5`; this entry refuses that shape rather than
approximating it as another sphere. No live hit, HP write, or
reaction is connected.

### Argument 43: moving shape-2 overlap candidate

The exact flag-1 moving slice of `3A5E4` keeps the two previous
endpoints and sweeps from the old `+98` endpoint to the new center
from beam matrix `+114`. Travel must be strictly greater than `0.5`.
The broad center is the midpoint, with radius `physical radius +
half the travel`; physical radius still uses the beam's `C0`.

The moving pair reuses the ordinary list/token gate. `39564` first
tests the centerline against **only** Sasuke's sphere radius.
`30A3C` takes the one-root-on-segment result directly. With both
roots on the segment, or without that direct result, the fallback
projects onto the swept endpoints/interior and tests a **strict**
sum of radii. The fallback therefore misses exact tangency, unlike
the scoped fresh sphere case. **210** bounded moving-shape and
candidate-point comparisons matched using host vector services.

The overlap entry itself returns a point only; it does not select a
receiver across the list or move the beam.

### Argument 43: moving selection and contact continuation

The detached `2DB58` selection compares squared distance from the
sweep's **start** to each receiving center, not distance to the
candidate point. The initial limit is `4e8`; a later list entry
replaces an equal-distance earlier candidate. This entry requires a
complete declared list of reviewed body-9 samples, at most one per
slot. An unknown or inconsistent later entry does not publish the
earlier proposed winner.

For the selected normal Sasuke row, the source adds
`point - endpoint98` to the endpoint and beam position and half that
offset to the broad center. A nonzero offset reaches `C44A8`, which
rebuilds the matrix from the moved carrier using stored scales; it
does not refresh `distance180` here. A zero offset still reaches
the contact callback without rebuilding the matrix. **344** bounded
nearest/displacement comparisons and two initial-distance limit
checks matched. Very long synthetic sweeps were used only for the
limit branch, not to qualify their contact-point arithmetic.

The moving headless fixture now selects `(-22,0,0)`, computes the
move and matrix, and threads that specific receiver through the
contact prefix and source-table amount. Its separate proposed HP
result is `180 -> 161`, not a second debit on the fresh-sphere
fixture. Scene list enumeration, live owner handles, reaction and
HP publication remain unwired.

### Argument 43: ordinary post-hit counters and KO branch

In the scoped opposing normal pair, `2F110` adds the positive computed
amount to credited G-Red `+418` and receiving Sasuke `+41C` **before**
the HP clamp. For the `7 -> 0` fixture those totals still get 19, not 7.
The zero crossing selects the KO continuation instead of the surviving
counter tail. A scene HP hold skips HP and stats but does not by itself
skip that surviving tail.

`2F304` uses shooting row `+2` (base 50 with the beam-relative
adjustment), not HP damage, for `+6C6`. The nonzero `7DE` byte inhibits
the decrement. With `5E0 & 0x01000000` set it checks the scaled
threshold; with the bit clear it bypasses that threshold. The surviving
tail also resets three clocks to 60 and applies row `+4/+5` to
`6C2/6C8`. A depleted `6C2` restores `6BE` and resets progress before
the separate progress addition; tier `6CA` caps at 63.

The moving headless replay checks surviving, zero-crossing and held
alternatives independently. **864** scalar-tail and **216** joined
post-hit/KO-route comparisons matched the bounded source. The counters
are numeric proposals, not a hitstun animation or measured reaction.
The KO selector does not run the KO routine, team ledger, effects, or
destruction.

### Argument 43: ordinary KO ledger prefix

For the same new zero crossing and exact direct opposing pair,
the detached mode-0 `2F8DC` prefix reads the receiver's **runtime
cost `4AA`**, not the raw or clamped HP debit. It conditionally
charges the receiving team's `118` budget, clamps a nonpositive
result to zero, and checks the separate signed reserve gate.
A reserve decrement can name the scene's credit/receiver pair
without exhausting the budget. The actor counts, cost totals
and credit-team `+100` halfword retain their original widths.

The headless KO alternative uses the verified ordinary normal
Sasuke full-cost operand of 150 as its declared runtime cost:
after `7 -> 0` HP the fixture budget goes `140 -> 0`, with credit
on slot 0 and receiver on slot 1. **1,440** bounded ledger,
reserve, credit and stat comparisons matched. This stops at
the `2FB14` observer scan boundary. It does not publish a team
budget, remove an actor, notify observers, or choose victory.

### Argument 43: ordinary KO observer queue writes

The `2FB14..2FC14` tail scans six physical records in order. It skips
inactive records and skips the fallen receiver by record identity.
Active opposing records, including the credited attacker, queue code
3. Other active records on the receiver's team queue code 4 and
increment `437` and `430` by one loss and the receiver's runtime cost,
respectively, at the original widths. Both branches write the
receiver's `3E4` field as the event subject, not the physical scan
index, and advance the local eight-entry `464` queue cursor `484`.

**9,216** bounded source comparisons matched across all active masks
and cursor positions, including fixtures that deliberately distinguish
the subject from the physical receiver. The headless KO proposes
`(3,1)` for G-Red at cursor `7 -> 0` and leaves Sasuke's queue untouched.
This stops at queue writes; no live notification, event consumption,
actor lifecycle change, or victory decision is wired.

### Argument 43: ordinary KO caller and Story tally

After the direct KO ledger and observer scan, the opposing kind-zero
pair in context `32 = 0` takes the caller's `2F204..2F304` path.
With the credited actor's selector `3E6 = 0` and no receiver link,
it calls `18214C` with the receiver's current model/variant `3EA/3ED`,
then stores the credited actor reference in the receiver's `5A4`.

The tally updates the first matching complemented model/variant word
in the active Story reward table, or appends a new word. It uses one
draw from the existing `55FC` byte stream and adds `1 + (draw & 15)`
only if the low halfword can hold the result. Overflow leaves the old
word intact but still consumes the source draw. The detached planner
refuses an append at capacity rather than writing into the backup.

**1,764** bounded comparisons matched the original caller, tally and
RNG. The headless fixture proposes a normal Sasuke tally of 9 and
G-Red's slot for `5A4`. It does not advance an owned RNG, publish a
live reference, run the common hit-effects tail, grant a Borg, or
write a save.

### Argument 43: common contact cues

The exact shooting and receiving rows have no special status bits in
their `+10/+12` hex halfwords. The ordinary `2F514..2F8BC` tail gates
visual and sound requests independently, using the accepted contact
point. `2AF = 0` proposes setting it to 4 and calling `19550` with
visual row 1 and the **beam carrier**, not its credited actor. The
source cue scan verified the class-2 allocation request with an
allocator refusal; sound selection still followed.

`2AE = 0` proposes setting it to 4, then derives sound family 8–11
from strict `7FC > 1.5`, `> 2.5` and `> 3.5` checks. `F088C` uses
the receiver's render index `96`, separate from its battle slot.
The old family counter's low bit chooses the variant at base cue
64, 66, 68 or 70; the counter advances as a byte.

**12,636** bounded source comparisons matched the routing, latches,
counter writes and playback arguments. The headless fixture proposes
visual row 1 and cue 67 at the moving contact in surviving, held and
new-zero alternatives. No effect is allocated, no sound is played,
and no live latch, counter, or actor state is published.

## Sasuke: original neutral B and paid repeat

Exact normal **Sasuke `GG4E08:000A/0`** is catalogue #2, AFS member 495.
An eligible close B takes priority over the neutral shot; unlike G-Red,
Sasuke does not admit the shared charged-B request. The checked neutral
shot is the no-target row, not every aiming context.

| Boundary | Recovered source behavior |
| --- | --- |
| Pose | Selector 44 uses the same upper-action/lower-no-op pair as G-Red, but Sasuke requests upper program `2:0` and lower `2:9` separately. An already-pending cue makes Sasuke skip the checked VM call; G-Red calls its VM with a temporarily zero step instead. |
| Release | The upper-pose cue and aim-or-timeout gate must both pass. Each paid release attempts **one** type-14 shuriken, argument **4**, not the native duel's provisional three-projectile fan. The shuriken carries the hit shape; Sasuke gets credit; the receiving collider determines the actual actor hit. |
| Repeat and refill | In the checked grounded phase, held B can request another cycle after a check-only availability test and a positive local attempt count. Later release still needs its own cue and debit. Refill arms for 90 source units only when B stock empties; no projectile return was joined. |

All **134 bounded offline cases** matched. Live pose timing, contact,
damage, repeat length and interruption order remain unverified. The
native duel's provisional shot policy is unchanged.
[Wiki lead: Shuriken](https://gotchaforce.fandom.com/wiki/Sasuke?oldid=6766).

The detached Sasuke planner shares only the post-VM aim/cue calculation
with G-Red. The gate result types remain model-specific, and Sasuke keeps
its own split pose requests, VM skip, attempt count, and grounded repeat
or cleanup request. A refused debit still uses a local attempt, and a
repeat request does not itself spend stock. **114** Sasuke source cases
matched, and all **169** G-Red comparison cases still matched. No live
allocator, VM, or contact handler is connected to this planner.

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
individual class action guards, stock and effects. Verify Gold Hero's and
Titan Robot's live form handoffs, other form transitions, copy effects, and
Galactic Emperor's special starts independently.
[Wiki lead: battle fusions](https://gotchaforce.fandom.com/wiki/Category:Fusion_Borgs?oldid=6272).
Battle fusion is a separate two-owner transition, not collection assembly.
No assumption is made about form timers, state transfer, transformed or
alternate animation maps, or Challenge/Story admission.
