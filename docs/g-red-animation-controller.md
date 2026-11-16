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
