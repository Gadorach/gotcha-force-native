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
| 65–66 | Two more wind-up snaps; last motion/contact precedes450 spend,9100 |
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
