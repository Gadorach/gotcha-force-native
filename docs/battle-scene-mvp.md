# Shared battle-scene MVP progress

**Priority change:** build the [playable native MVP](native-battle-mvp.md) first.
The source-accurate reconstruction diary below is retained for later refinement.
Exact briefing/HUD/callback reproduction is no longer a blocker for the prototype.

The target is a stage and normal G-Red/Sasuke actors rendered from owned current
world frames, with a battle camera and scene lifetime that Story can reuse beneath
its dialog/progression UI. This is not implemented end to end yet.

## Current integration

The ordered Challenge path now owns the initial descriptor/physical pools,
fixed six-slot actor seeds, singleton stage seed and initial view records.
Prepared fields are still distinct from constructed actors and drawable frames.
Normal G-Red/Sasuke construction and their qualified first world evaluations are
now owned by the scene. Their first appearance update also produces the actual
nonzero render-registration selectors and spawn colour/scale. Actor model
publication, source model queues and shared body/part geometry are now joined.
The two additional AI constructors selected by the real update order now reach
first worlds and shared model publication too. Initial actor-linked camera poses
and their first ordinary spawn-world follow update are also joined. Shared
projection supports the source split-screen operands. The stage's first draw
callback now publishes its two normal roots with shared rigid geometry; active
presentation, object publication and the remaining frame callbacks still prevent
a rendered scene. The initial collider/contact pass and inactive-view visibility
boundary now permit first-camera activation in the same native scene; the later
spawn-frame collider sweep, qualified contact pass and active-view visibility
casts now reach the existing follow-camera kernel too. Object draw callbacks
have a published48-prop prefix with shared prepared geometry, followed by seven
camera-invariant side roots and the final side selected by the owned first
presentation viewer. Child callbacks and
presentation still prevent a rendered scene.

The scene-service continuation has passed owned and actual-resource tests in
normal and ASan/UBSan/float-cast/leak builds:

- A private RNG identity travels with the genuine party handoff. Equal bytes in
  another RNG owner do not authorize use; real intervening draws remain valid.
- Team initialization uses the existing four-draw kernel, including the two empty
  team records. The stream and field candidate commit together in the serialized
  native owner. There is no reset, hidden seed, extra draw or automatic retry.
- The pause-context initialization, source background-color stores and six tail
  clears are represented. Source sound flags are reset in the same native control
  owner. The program-gain equality branch is admitted only for that owner's
  initialized bank0/program1=127 state; arbitrary gain/effect or device execution
  is not acknowledged.
- Source-origin or sound-owner retirement invalidates the continuation. Inventory
  images/revisions and the actor/pool state are preserved.

Three genuine sessions passed 181 native checks per normal/sanitizer mode, with
12 actual team draws. 398 owned checks passed per mode. An independent comparison
matched 15 rows/147 components. Existing party/finalization/sound-control tests
also passed in the consolidated repository.

## Verified common/stage resource join

`BattleCommonResources` owns the four decoded common resource blocks used by
40F74, including the actual common motion-bank bytes. It performs a single checked
native-bundle read and refuses retries, changed content and retirement during
read. It does not instantiate a motion player or an actor frame.

`BattleStageResources` takes the privately retained final stage-data owner,
checks it against the selected stage and seed, loads the shared HSD bank/static
grids, allocates a real same-domain archive descriptor and binds authored lights
and supported fog. It does not instantiate 87D2C model callbacks or grant a draw.
Normal and sanitizer checks passed: 31 checks per mode across three genuine
sessions, including a real stage binding, corrupt-input refusal and safe
retirement during a read. Party/finalization regressions and combined-header
compilation also passed.

The initial real-stage test correctly hit the old static-fog refusal. Source
08CA8 loads only the authored FogDesc, leaving its animation unrequested. The
explicit authored initializer now handles that reached path; the existing static
entry still refuses animation metadata, and unsupported fog types/range
adjustment remain guarded. The failed attempt and source comparison are retained
in ignored local evidence.

## Side-resource continuation

The next join registers the zero-argument battle cleanup callback in a typed
64-word queue, then binds side archives to the same descriptor pool. Its empty
queue originates in the genuine final-setup cleanup callback (190EBC→7FD5C).
Registration stores the callback/count and advances the write cursor; it does
not execute teardown.

Side data contains a sector table, a placement block and up to eight HSD banks.
The retained final payload supplies those bytes without another file read.
Placement remains data; side-object/model callbacks are still separate.
Normal and sanitizer checks passed: 117 checks per mode across two genuine
sessions, including successful same-pool binding and actual pool exhaustion.
Failure retains the earlier callback registration and refuses replay. Independent
source checks cover finalization's reset lineage, ordered calls and 44 retained
side-archive layouts.

## Initial light colours and static-grid bindings

The shared `initialize_scene_light_colours` operation now seeds the eight-slot
baseline and four cleared channel states. Startup lighting reuses it too.
The stage binds the three existing checked STIH grids through typed references,
retaining their source byte extents without another decode or mutable relocation.

The genuine stage join passed 24 checks in normal and sanitizer builds; independent
comparison matched both authored colours and all three grid dimensions/extents/
triangle counts. Inventory images, RNG and descriptor/object/model pools were
unchanged. That stage-binding step reaches 80028640 / 4104C at native revision 12.

Owned light-colour tests, the startup scene-pass regression and combined-header
compilation passed with sanitizers. The scene-pass test was corrected to inspect
backend vertices after `end_frame`, matching deferred upload; the unchanged
baseline reproduced its earlier failure.

The archive metadata's legacy `cameras`/`cameras18` names refer to the
**scene+8 light-root table**, not camera objects. `light_root_count()` exposes the
correct meaning without changing stored values or breaking older callers.

## Briefing resources, widget allocations and initial cue

`BattlePreparationModels` now supports resource-first loading through its shared
bank decoder. The ordered scene reads the briefing bank once and allocates its
archive descriptor after the stage/side archives. Only then does the existing
widget allocator make all 58 attempts in the same physical/model pools. The real
fixture creates 58 widgets and consumes 243 model descriptors. Model instances,
root attachment and widget callback execution remain pending.

The following F03C0(null,372) call now mutates the caller's existing shared sound
staging queue through the existing command kernel. It uses the same sound-control
owner's current OPTION settings, bypasses the disabled flag as EFC04 does, and
preserves queued packets. A full queue still increments the source call counter.
Missing bank2 and unsupported randomized tones refuse before mutation. The
generic null-owner helper is also reused by OPTION sound commands.

The resource-first owner protects reads against retirement or moves, including
the older widget-first component entry. Resource-only readiness cannot grant
widget/model-instance readiness; later component instance preparation must use
the same descriptor domain and bank.

Normal and sanitizer verification passed 90 resource/scene checks across four
genuine sessions and 107 sound checks across three more. Owned resource-order,
legacy model/widget and UI-sound regressions passed; combined-header compilation
also passed. Command admission and shared-ring transport were tested, not device
playback.

That continuation reaches 8002864C / 41288 at native revision 15.

## First-member character loading and deferred publication

The ordered scene now queues the six-slot 41288/410BC work using its actual
fixed-slot seeds and member 0. Inactive slots have cleared publication/cache
state and fourteen skip cells. Each active slot queues a row load followed by a
two-argument publication callback. Enqueueing performs no asset reads or archive
allocation. A private scene claim and archive-domain identity prevent another
queue from borrowing that origin.

The existing `ChallengeCharacterResources` owner also executes these native
operations through a bounded component interface:

- Loading changes the current row's cells. Empty blocks clear cells; unselected
  nonempty blocks retain their skip state. Byte loading alone grants no receipt.
- Publication consumes slot/variant arguments, reads the current row, publishes
  its four resource views and allocates a real blank header in the same archive
  pool. HSD binding and model instances are still separate.
- A successful drain requires every operation and argument consumption. Failed
  allocation or retirement keeps earlier pool occupancy but invalidates the
  batch's resource/roster authority. Failed work cannot replay.

Normal and sanitizer checks passed 202 checks per mode across seven genuine
sessions: ordered enqueue, a component drain, partial allocation failure and four
in-flight lifetime cases. Both human slots select the intended normal Sasuke and
G-Red first members. AI resource inputs are loaded as data; their actor classes
are not newly admitted.

The character-queue step reaches 80028650 / 42520 at native revision 16.
Its character queue is exposed read-only: dispatch remains closed until the
later synchronous consumers and shared deferred-load ordering are joined. The
component drain tests do not advance that scene boundary or claim DVD timing.

## Display selection and cockpit/HUD bytes

The genuine party handoff now retains 1962C4's display-layout stores: one player
selects layout 0; two players select layout 1 and write split field 3D to 1.
The one-player branch leaves 3D unknown rather than inventing a zero.

42520 now reads the matching cockpit archive once into the scene owner, with its
header slot explicitly null. It neither allocates a descriptor nor drains the
queued character loads. This is raw HUD-resource ownership, not a HUD instance,
active camera or rendered screen.

Four genuine sessions passed 28 checks per normal/sanitizer mode, including
two-player scene binding, corrupt data, retirement during read and a genuine
one-player handoff selecting the other archive. Owned tests passed 38 checks;
party/finalization regressions and combined-header compilation passed with
sanitizers. Independent comparisons matched both archive selections and 19 raw
source instruction anchors.

That continuation reaches 80028654 at native revision 17.

## Shared effect queue and texture binding

42878 now appends the common-effect load and texture callback to the same native
item/argument queue as the character loads. Four typed destination references
replace pointer-valued guest words; unwritten argument cells remain unknown.
Single-player selects model block 1 and split-screen selects block 2.

The component executor reads the checked three-block bundle and performs the
texture callback using the existing TPL decoder. It owns the actual 256×64
RGB5A3 sheet as RGBA pixels with the source clamp/linear sampler state. Model
archive binding, effect instances and GPU uploads remain separate.

Normal and sanitizer checks passed 87 native checks per mode across four genuine
sessions, plus 59 owned checks. Corrupt or retired effect loading invalidates
batch authority while retaining prior character descriptor occupancy.
Independent decoding matched the checksum of the 65,536 RGBA output bytes.

## Face-slot clear and common UI archive

Normal Challenge's genuine common+0x32=1 branch clears all six face header/data
pairs and writes their FF selectors, then skips the face-file loading branch.
It does not invent player-role table inputs.

439A0(1) now loads and binds `unitall` through the shared HSD/archive systems.
The real bank contains 202 model records; none are labelled constructed actors.
Its synchronous archive allocation occupies index 4 **before** the queued
character publications, which then receive indices 5–8 in the tested component
drain. Existing widget/model pools retain their occupancy.

Four genuine sessions passed 64 native checks per normal/sanitizer mode, including
corrupt-input and retirement refusals. The owned suite passed 32 checks with
sanitizers, and combined-header compilation passed. Independent source/data
comparison matched the bank, texture pixels, allocation order, 18 instruction
anchors and seven call edges.

That continuation reaches 80028668 at native revision 20.

## Bootstrap tail and actual scene-owned resource drain

The fresh normal entry retains the zero route15DD value from the qualified
context clear. The tail takes that branch, writes dispatch selector 5, and
performs the initial BGM request against the same shared sound-control epoch.

`MusicRequestOwner` owns real player2 scalar controls and a bounded 64-word
request ring. E99C8 uses the current BGM preference and appends load/play
commands without resetting existing requests. This fresh native owner has no
playing stream; importing live CRI playback, applying gain to an existing stream,
music command dispatch and speaker output remain unsupported. Ambiguous
full-ring writes refuse rather than being reported as an empty successful queue.

After the bootstrap returns, the scene can execute its actual ten resource
operations. Successful drain publishes four character headers and the effect
texture binding, preserving the common-UI-before-character allocation order.
In-flight method objects remain alive through filesystem callbacks; retirement
immediately invalidates child scopes. A scene move during a read refuses late
completion, and a failed later load retains earlier descriptor occupancy.

Normal and sanitizer verification passed 125 checks per mode across four genuine
scenes. The music owner passed 399 owned checks. A completed drain reaches native
revision 31 and **800286D8**, the preparation wait/update boundary. Widget
callbacks, constructed actors and current renderable frames are still pending.
The drain receipt stops before the full286D8 transition; later component joins
below do not implicitly acknowledge its remaining calls or the288FC dispatcher.

## Normal construction: actual scene archive and model roots

Normal Sasuke and G-Red construction now consumes the scene-owned member0
publication receipts. At 419EC it binds each actual blank archive header and
allocates all normal model descriptors followed by all alternate descriptors in
the existing model pool. The exact prepared root graphs transfer into those
owners; their aggregate-bank views disappear while scoped identities survive.
The material context is owned by the same scene resource row and weakly referenced
by its model descriptors.

The tested prefixes consume 38 Sasuke and 28 G-Red model descriptors, beginning at
indices 243 and 281 after the existing widgets. That step reaches level0 scalar
defaults before 800562B8. These are construction prefixes, not initialized actors
or a rendered battle.

Normal and sanitizer checks passed 1,468 checks per mode across two genuine
scenes, including a late allocation failure that retains two attached roots
without acknowledging full model setup. Context/root ownership tests, existing
normal-only binding guards, preparation-model regressions and combined-header
compilation passed. Independent comparison matched the 66 allocations against
the original model tables and checked the bootstrap/music source stores.

## Scalar effects, child construction and dimensions

The reached normal 562B8 path now copies the actor/class operands, seeds the
current level's reload rows, classifies the three channels, clears their timers
and resets the same resource-owned material colour context. Startup bookkeeping
reuses the extracted numeric reload-seeding kernel.

Its required child constructors allocate from the existing class2/model pools.
Normal G-Red creates one five-descriptor bar child and two zero-descriptor shape
children; normal Sasuke creates two shape children. Headers, source parent field
copies, slot-biased groups and prepend order are represented. Parent aliases are
typed lifetime references to the construction body. They do not grant initialized
actor, child-update or world-frame authority.

The source's no-object/no-descriptor outcomes are exercised through actual
exhausted pools; the parent continues with the recorded absent child as the
original constructor does. Retiring the native parent closes its aliases without
claiming that source child cleanup ran.

55DCC uses the shared dimension kernel, copies the fifteen current source values,
and reaches 4D08C's fresh-zero branch without reading unbuilt bone matrices.
68004 uses unscaled class dimensions. 27A44 applies the actual human control mask;
its AI branch remains unjoined.

Normal and sanitizer checks passed 1,746 checks per mode across three genuine
scenes for scalar/effect construction, then 27 checks per mode for both human
dimension prefixes. Owned tests and shared bookkeeping/object/model-binding
regressions passed, as did combined-header compilation. Independent comparison
matched 164 scalar trace rows, 62 dimension trace rows and 15 allocation outcomes
using the original data, reload and rank tables.

That continuation reaches 8004BB14 at native revision 8.

## Local initialization, battle-data reset and timer prefix

4BB14 now binds the actor's actual published specific-motion input and the same
scene's common-motion owner. Common-resource references follow moves and expire
on retirement; equal bytes from another owner do not substitute for that binding.

Both local pose buffers receive logical rest translations and identity-case
quaternions. Used bone scales are one, all 32 extra-angle slots are cleared,
eight motion records receive their source indices, and the same four auxiliary
records are cleared in both passes. The source clock scalars and four class
ratios are initialized. The shared local-pose helper is also used by the existing
preview path. No animation request or world matrix is produced by this step.

3CC24 binds the actual battle-data publication, resets its represented fields
and installs mask272=EF. This is not battle-program execution or collider
readiness.

The reached 1C80C prefix now uses the party's actual current RNG owner and commits
six draws plus the source timer/control writes. Foreign equal-byte RNG owners
refuse; intervening draws are preserved. Target selection and the later suffix
draws remain pending.

Normal and sanitizer verification passed 134 checks per mode for two genuine
human prefixes, including 12 timer draws and five intervening draws. Owned local
initialization/common-resource tests and pose, preview and timer regressions
passed; combined-header compilation passed. Independent comparison matched
1,522 initialization trace rows and 88 timer trace rows, using original HSD
translations, class data and timer tables. The Gekko paired-single identity
routine was checked from its raw encodings rather than LLVM's newer-ISA labels.

That continuation reaches 8002532C at native revision 11.

## Initial target selection and constructor RNG suffix

The scene now registers each normal constructor at its fresh revision0 boundary,
checking the same drained resource receipt and physical slot's member0 seed.
Private typed references follow constructor moves, expire on retirement and
retain the old slot's consumed claim when an owner is replaced.

2532C observes all six real physical slots. Registered slots read their current
construction owner; only untouched slots use the factory's cleared fields.
Retired/failed registrations and consumed-but-unregistered slots refuse instead
of falling back to an empty or untouched seed. Factory team88 remains zero until
the constructor writes it; party team metadata is not substituted.

The two qualified normal classes use their six zero policy selectors and the
shared nearest-target query. The fresh null-target branch now commits its actual
writes and three remaining draws against the current party RNG. Both writes to
328 are retained: the20FD4 table result followed by1C80C's final draw masked by7.
Non-null changed-target handling through23BF4 remains unsupported and refuses
before mutation. Observations are explicitly bounded to the represented
constructor revisions; future class writes cannot silently inherit zero flags.

Normal and sanitizer verification passed 79 checks per mode across four genuine
scenes, including moves, stale registrations, unregistered consumed slots,
foreign equal-byte RNG owners, six suffix draws and five intervening draws.
Independent source comparison matched 18 target/suffix write rows and both
actual4B8 callback keys. Owned profile/query/construction/pool checks and combined
headers passed; five extraction-tool tests passed.
The existing child-construction regression also passed 1,746 checks per mode
with normal and sanitizer builds, including actual object/descriptor exhaustion.

That continuation reaches each class4B8 initializer at native revision12:
Sasuke8006F708 and G-Red8018CE1C.

## Class initializers and post-class children

The shared class-initialization kernel now drives actual source-ordered attempts
in the existing class2/model pools. Sasuke attempts attachments6/7; G-Red attempts
common auxiliaries40/41 followed by attachments24/25. Each consumes two model
descriptors on success. Attachments use the slot-biased group0 family and the
explicit kind83=18 store; common auxiliaries use the group24 family. Headers and
parent aliases/scalars are copied without executing update/draw callbacks.

G-Red's4A8/6F6 stores and allocation-dependent parent708 writes are represented.
The existing auxiliary-construction kernel supplies its conditional parent mask.
No-object/no-descriptor failures remain absent children; a successful first
auxiliary followed by exhaustion leaves708=1, rather than claiming both channels.

An explicit V2 fresh-entry recipe owns the enclosing normal dispatcher lineage:
common00=1 and cleared caller4, the801968CC transition into the first battle
dispatch, then80196948's caller increment and common00=3/common01=1 return.
The old V1 recipe retains unknown common mode and still refuses G-Red'sC74EC gate.
There is no public late mode setter or equal-byte replacement of the scene owner.

Class verification passed 2,243 checks per normal/sanitizer mode across five
genuine scenes, including eight class initializers, shared-pool exhaustion,
partial auxiliary success, moved parents and the legacy unknown-mode refusal.
Entry/profile, shared class/auxiliary and object-pool tests passed.

The following807E4/813A8 consumers allocate from unparented group56, requesting
six descriptors then one. They copy only the two parent aliases. The native
That continuation reaches8008AB30, revision14, before class4 collision
allocation. These children remain callback-pending, with no current world frames.

Post-class verification passed 2,161 checks per normal/sanitizer mode across four
genuine scenes. It covers both allocations succeeding, exhausted objects, a
six-descriptor refusal followed by a successful one-descriptor allocation, and
one remaining object. Source comparison matched 32 allocation outcomes and 12
ordered call/write rows across the class and post-class stages.

## Actual class4 collision-body construction

Class4 now uses its actual 384-cell physical bank and the same mutable group
head/tail owner as class2. Links carry physical-arena and index identity rather
than pretending all neighbors belong to class2. Mixed-class prepend, append and
unlink preserve existing occupancy and FIFO free chains. Standalone class2
owners cannot invent a class4 bank.

8AB30 reads the signed body-ID list from the actor's current battle-data owner.
It attempts 16 normal Sasuke records and 11 normal G-Red records, with the actual
slot-biased group48 allocator, class4 header and source field copies. Class4
allocation consumes no model descriptors. The flag200 records update501 and
typed530..53C references only when their allocation succeeds.

Each body retains typed parent, source-descriptor, class-parameter-slot, self and
bone/body-matrix relationships. Descriptor access resolves to the same owned
battle-data bytes and follows constructor moves. Parameter-table bytes, matrix
values and collider updates/registration are not exposed as ready. Constructed
body release refuses until the source cleanup consumer is joined; retirement
invalidates bindings without reclaiming occupied cells.

Normal and sanitizer verification passed 1,085 checks per mode across three
genuine scenes: 81 attempts, 27 normal allocations, eight partial-prefix
allocations, and full exhaustion. Source comparison matched all 81 outcomes,
35 bound-body rows and 35 parent trace rows. Mixed-class pool, constructor,
collision-bank, model-binding, widget/resource-order and collider-lifecycle
regressions passed, as did combined-header compilation.

That continuation reaches80056D28 at native revision15.

## First placement/history and initial control selectors

The normal placement gateway now uses the actor's actual team and the current
mutable team cursor in the same scene. It reuses the shared spawn, ground,
heading and secondary-grid segment kernels. Static ray bounds come from the
owned stage seed, rather than a caller-supplied replacement stage observation.
The constructor's491 write, first43B/43C fields, position/angle histories and
following3F1/741/742 stores are committed in source order.

37B98's two dynamic loops read counts80436244 and8043624C. The fresh physical
scheduler owns the actual zero counts established by8853C; body allocation has
not executed collision update/registration. These counts prove the reached
no-hit return without reading either reference array. This is not an inferred
empty class4 pool, initialized reference-array claim or generic dynamic-query
implementation. Nonempty counts refuse until their shape consumers are joined.

Two genuine human placements passed normal/sanitizer verification, 37 checks per
mode. Both use successive current team cursors, copy all represented histories
and consume no further RNG. Foreign scenes, missing profiles and replay refuse.
Independent raw-STIH/source-table comparison matched both positions/headings and
60 ordered trace rows; source instructions confirmed the exact count reads and
all ten zero halfword stores.

The subsequent6A750/6A6FC prefix binds selector0 of the actual normal class4F0
table. It copies the previous selections, installs secondary/primary control1
and performs the ordered source clears. This is control-state initialization,
not input handling, AI or animation playback. Normal/sanitizer verification
passed 27 checks per mode; source comparison matched 42 trace rows. Profile,
constructor, target, spawn/projection regressions and combined headers passed.

That continuation reaches8004BEB8, revision17.

## Initial animation request and normal constructor return

The source-bound `(group0, choice8)` request now uses the shared program VM,
motion driver, root/height controls and local-pose composer. Clip0/9 is decoded
from the same specific-motion bank already bound by4BB14; no caller-supplied clip,
preview actor or second model/root allocation substitutes for it. Descriptor
fallback model selections bind the actual class configBE/BF bytes.

The shared VM has an explicit GFMP4 request-only entry. It executes through the
first real wait, preserving the program bytes, wait flags and threshold.
Sasuke's nonterminal `−8` wait is not rewritten as a terminal: its later mode3
continuation remains unsupported. Updates and replay refuse before changing
clocks or motion. Existing GFMP1/2/3 entries retain their original admission.

Both lanes select the real source clip and populate the current primary local
buffer. The retained buffer stays unchanged; root position remains the actual
placed position. Public-null-root handling comes from the selected clip's real
public/AObj data. Initial height selection uses its reached no-world-read branch.
Opcode3 installs behavior200, which the physical target observer now reads from
the actual VM owner rather than assuming cleared flags.

The fresh normal541AC tail stores500=0 and returns. Native revision18 is
**ConstructedAwaitWorld**: normal construction is complete, but this grants no
world matrix, model draw, collider update or scene-frame authority. The pending
program continuation remains separate from the completed initial request.

Normal/sanitizer verification passed 86 checks per mode for two genuine normal
constructors. Independent source/HSD/FObj comparison matched all 1,012 sampled
channel components, 322 local-pose components and 14 ordered program operations.
Source-profile boundary checks passed 778 cases with sanitizers. Shared program,
height, root, directional, follow-tail, motion-driver and preview regressions
passed, along with combined-header compilation.

## Source-ordered first actor world

An explicit fresh54070 gateway now runs5685C before dispatching the phase0
constructor. It preserves the actual history-copy order and retains a private
caller relationship through construction and moves. The older constructor-only
entry cannot retroactively acquire first-update authority.

After541AC returns, the same scene gateway joins the caller's6DC store, actual
sceneEB observation, qualified in-bounds5A580 return and465FC world work. It uses
the existing first-pass, ground/contact, two-bone IK, constraint and look kernels
with the current program pose, actor fields and owned stage grids. The source
741=1 gate skips initial aiming. Caller tail clears and06284's actual empty
colour-stream branch finish the first update.

Both actors now own generation1 world matrices at native revision19. Each has
two ground contacts, two IK chains and two constraints. Program time, source
positions/histories, all pool occupancy, registry counts and RNG remain intact.
Repeated first updates, foreign scenes, missing operands, late prelude claims
and the unsupported clamp/effect branch refuse without fabricated worlds.

Normal and sanitizer verification passed 46 checks per mode across two genuine
scenes. Independent raw-class/STIH and matrix/IK replay matched 746 numeric
components (maximum measured error0) and 48 ordered prelude/tail trace rows.
First-update profile, constructor, grounded-world and look regressions passed,
as did combined-header compilation.

This is **FirstWorldReady**, not a draw grant. The actual descriptor roots remain
unmodified and no model publication or rendered frame has been claimed.

## Source visibility producer and spawn colour

The constructor's82=0 cannot be bypassed to publish a model. The reached
5475C→547DC phase1/substate0 update now performs the first appearance entry:
1A=1, curve0 on colour channel1, D4/D8=20, selector82=96+65, and the source scale
calculation followed by D4=19. The first45204 input is exactly zero; this path
does not invent a sine approximation, frame rate or later growth continuation.

Curve0 is supplied through an explicit GFAF2/fresh binding path, retaining the
existing material owner. GFAF1 still lacks that curve and refuses this update.
The shared fade kernel produces its first white sample and updates the same
resource-row colour context. Live58 scale is separate from the initialized rank
operands. The shared grounded-world path then produces generation2, and the old
first-world accessor no longer presents generation1 as current.

The entry does not reach55568 or4CD24: both real motion contexts and program
waits remain unchanged. Source-comparison checks matched102 ordered trace rows.
Native and sanitizer verification passed38 checks per mode for both normal
actors, including staged-failure rollback, foreign-scene and missing legacy
colour refusals. First-world regression passed46 checks per mode; fade,
bookkeeping, actor-frame, model-part and preview regressions passed.

The existing `ModelPassQueue` now has a separate fresh global-operand profile,
without importing startup-only camera/fog policy. Legacy entry checks remain
intact, and the new entry refuses reload/reset of a populated queue. Queue/pass
regressions and combined headers passed. This is shared queue infrastructure,
not actor collection, registration or a native render-pass acknowledgement.

## Actor callback collection and model publication

The bounded first world-group0 entry now performs8C098's actual late-count reset
and8A16C's render-list clear, then53928 observes all six physical slots. Only
untouched factory slots use their cleared82 value. Retired registrations and
consumed-but-unregistered slots refuse. Actor render heads/tails and110 links
use typed physical-arena/index relationships in the existing scheduler.

8C0DC queues the actor callback separately from execution.8A0A8 then walks the
captured actors in source slot order;09790 starts the shared model queues.
Queued or partially executed work grants no completed pass plan. Captured body
and world generations, moves and retirement are checked throughout.

56AA8 now copies all46 logical body matrices into the exact existing normal
roots, applies node visibility/user-matrix/dirty flags, and registers each body.
The normal initial270/59A/720 values prove style0 independently of10514's
read-only view query; this does not select or qualify an active camera.
The two normal4D4 slots are null.4FCFC selects Sasuke records1/7 and G-Red3/8,
publishes their actual rigid-root matrices and performs the remaining097B4 calls.

All six roots retain their original identities and material contexts. Source
queue links are typed references, not guest pointers written into host words.
The resulting18 visits preserve opaque/texture-edge/translucent queue order.
Frame retirement invalidates current-world authority without clearing consumed
root state or pretending source cleanup ran.

The existing body skinning and rigid-part evaluators consume those published
matrices. Part geometry reuses the same decoded source bank; it creates no second
scene descriptor or mutable HSD root. This is evaluated geometry preparation,
not a camera, draw, collider update or whole-scene grant.

Native/sanitizer verification passed293 checks per mode across two genuine
scenes. Source comparison matched six roots,50 joint bindings,16 callback rows
and18 model-pass visits. Queue, pass, instance, descriptor, body/part renderer and
profile regressions passed, as did combined-header compilation. Queue IDs
(descriptor index+1) were244/245/251 and293/296/301, derived from actual occupied
pools.

## Stage allocation prerequisites

Class3 now uses its actual 512-cell physical bank through the same owner as
class2/class4. `ScenePhysicalPools::bank(3)` exposes that mutable bank, rather
than a separate factory snapshot. All three classes share one group head/tail
store and typed cross-class links. The8893C route checks descriptor availability
before object availability, including group−1; group−1 then consumes no model
descriptors. Allocation/release preserves occupied prefixes, FIFO reuse and
generation guards. Attached-root cleanup is checked before unlinking.

Class3 descriptor attachment reuses `SceneModelBindings::attach_normal` and
exclusive HSD root transfers. Foreign archives, stale leases and out-of-range
slots refuse before consuming a pending root. Two descriptors referencing the
same authored record still own distinct mutable roots. Unrequested metadata
cleanup retires both identities; this does not qualify stage callbacks.

Sanitizer checks passed for class3 allocation and model binding, alongside
class2/class4, physical-pool and actor-model-binding regressions. Combined native
headers compiled. The stage2 probe also constructed normal metadata roots for all
51 main-bank records and the two side banks'9/8 records through the existing HSD
instance system. All three main records0/1/2 contain geometry.

An allocation-only diagnostic in a fresh, genuinely drained scene passed160
checks per native/sanitizer mode. Independent source-table/placement comparison
matched93 ordered allocations:56 objects,33 collision slots and four direct stage
descriptors (94 model descriptors total). The resulting free counts were
class2=432, class3=478, class4=351 and model=1152. Existing preparation occupancy
was retained. These slots deliberately remain unconstructed; the diagnostic
does not bind collision parents or claim it executed87D2C.

The actual eight placement rows select four class3 objects (two each of types4/5)
and four class2 objects; type4/5 collision selectors are171/172. The main prop
ranges are3..32 and33..50. The source prop loop exits its current range on object
allocation failure; it does not continue attempting the remaining records.
Instruction80087B08 writes type11 through the retained last ordinary-prop
register31, not the newly allocated static-prop register27. Preserve that
source destination when joining the constructor.

The allocation-only evidence above does not advance the stage phase or provide
a rendered scene. The following continuation now executes its side-object prefix.

## Stage side-object constructors

The scene-owned `BattleStageConstruction` entry joins87D2C's phase/draw stores
and818D0's eight actual placement rows. That prefix ends at `AwaitProps`.
Enclosing-frame tracing establishes that initial actor updates precede this
stage entry (see below). Consumed actor slots must have current registered first
worlds; untouched factory slots remain available to isolated stage-component
diagnostics. Initial actors cannot be attached retroactively after this prefix.

81980 dispatches four81B9C class3 constructors and four81D20 class2 constructors.
819B4 installs their actual headers, type/bank selectors, positions, quantized
angles and13 factor values. The12 source roots occupy the existing descriptors.
The four07030 roots own descriptor-local colour contexts; every repeated record
still has a distinct exclusive mutable root.

The second class3 root receives07C54's shared material updates,07CD0's diffuse
RGB zero and07F38's texture removal. Authored material/diffuse alpha is preserved:
these side constructors do not call07CAC. Type5 additionally clears PObj culling
through07D68(-1); type4 does not call it. Source program/radius/duration, scale,
HP and enabled fields are retained without executing their later consumers.

89100(1,append) writes the existing scheduler's typed heads/tails and actual
class3 callback100/104/108 fields. The four class2 objects evaluate their initial
114 matrices with the shared identity-transform kernel. Class3 constructors do
not evaluate114; their collision bindings carry a matrix-parent reference,
not a fabricated identity or a ready world.

8AE60 now constructs six real class4 bodies from the same common98 owner, using
typed same-pool class3 parents and the source descriptor/parameter keys. Actor
and non-actor collision bindings remain distinct. Parent retirement invalidates
borrowed descriptor access; bound bodies and registered callbacks refuse cleanup
until the corresponding source consumers exist.

Native/sanitizer verification passed2,832 checks per mode across five genuine
scenes: the normal eight objects, an exhausted collision bank, an exhausted
class3 bank, one remaining model descriptor and a reused descriptor-local
material cache.07030 binds its existing cache before819B4 writes18=255; the
following word remains intact. Failure branches preserve
consumed prefixes and continue subsequent placements.818D0's final16 global
stores run after the loop. Owner moves, replay refusals, original pool occupancy,
RNG and untouched registry/render queues are covered. Shared allocator,
material, instance, model-binding and resource-order regressions passed, and
combined headers compiled.
Independent source-data comparison matched eight constructor rows,12 root
bindings,12 styled meshes, four styled joints and six collision bindings. The
actor publication regression also passed293 checks per native/sanitizer mode.

82060 supplies the class3 side world later through47AA4 and the180 vertical
offset; that callback has only been registered here.

## Main props and stage roots

The next owned transition runs87924's30 ordinary and18 static prop constructors,
then87D2C's four actual model-descriptor/root operations. It reaches
`InitializedAwaitFrame`, with94 stage-related roots in total, including the
previous12 side roots. Each allocation uses the existing pools and leases.

8784C binds the main-bank record, copies authored translation, quantizes rotation
and evaluates47AA4's flags0E/direct-position branch through the shared transform
kernel. The existing0F/local-position entry retains its separate input domain.
Ordinary props bind a second root, remove its exclusive default child subtrees,
apply the source shadow style with alpha0.5 and run the optional collision
selectors. Custom, referenced or deforming subtree cleanup refuses. Normal roots
retain their children, and root identity/immutable source geometry are preserved.

Ordinary/static callbacks87B64/87C84 are installed after the common constructor.
The static loop preserves80087B08's actual retained-r31 destination: record32's
type11 becomes1 when a static allocation succeeds; new static objects retain0.
Allocation failure exits the current range. A successful static allocation after
an ordinary allocation failure reaches an invalid null-r31 source store and
refuses, retaining its consumed prefix. Bound collision parents refuse release
before unlinking until source collision cleanup is available.

The four stage roots are normal record0, styled record0, colour-mobj record1 and
normal record2. The stage shadow has alpha0.5, no culling and authored Y minus1.0.
070E4 has an explicit `ColourWrapper` material-context kind; equal cache words
cannot substitute that class into07030's actor entry.09C28/09C30 update the same
descriptor-local words to255/FFFFFFFF in source order.

Native/sanitizer verification passed6,353 checks per mode across seven genuine
scenes, covering normal construction, exhausted collision/object/model pools,
the null-r31 refusal, partial stage-root allocation, source actor-first occupancy,
stale/advanced actor-prefix refusals and owner retirement. Source-data comparison
matched48 prop rows,78 prop roots,35 shadow meshes,27 prop collision bindings,
four stage roots and six preserved actor roots. Shared transform, material,
allocator, model-binding and resource-order tests passed; combined headers and
`git diff --check` passed. The side-constructor regression passed2,832 checks per
native/sanitizer mode.

In the actor-first component fixture, stage roots occupy descriptor indices
430..433. Actor root indices remain243/244/250 and292/295/300. Stage completion
leaves417 class2 cells,478 class3 cells,324 collision cells and1,055 model
descriptors free; actor model publication and geometry preparation allocate no
replacement roots.

### Enclosing-frame ordering correction

29434 calls53B30 (actor ordering),0BEF4 (view aliases), then the group40/32
updates,53A2C→54070 (actor updates), and87CF0 (stage dispatch), followed by the
remaining object groups and later callbacks. This corrects the earlier
stage-before-actor assumption. The combined diagnostic now constructs actor
first worlds, runs stage construction against those occupied pools, and only
then advances the actors' spawn entry and publishes their original roots.
Changed, retired, unregistered or already-advanced actor prefixes refuse.

This is still a component-level composition. The first53B30 producer is now
joined below, but the complete29434 callback sequence, camera activation and
whole shared render frame remain pending. Stage construction alone does not
publish any stage model world or admit a rendered scene. The earlier explicit1/3
slot sequence verifies those components, not the full dispatcher.

The camera path is now located:0BEF4 selects the viewer aliases, while0BDA4
activates the12 actor-linked view records, allocates their8AE10(selector61)
collision bodies and calls0C5F8/0D46C. These parents are view records, so they
require their own typed lifetime relationship rather than a class3 or actor
parent substitute. The first-entry slice is now joined below; later tracking
and projection remain pending.
The reached962C4 display setup already has an owned producer in
`ChallengeInitialPartyCandidate::display_setup()`.0685C(0) copies the selected
activity mask/count into15DB/15DC.1910B4 copies the selected controller order
into C0..C3 and writes FF to the unused tail.

## Actual first actor update order

The new source-owned53B30 entry consumes two draws from the same party's current
RNG stream. It observes all six genuine unclaimed factory cells, resets the
actual24 team references/four counts, performs the factory scalar writes and
builds the physical update list. Unwritten update-array tails remain unknown.
Factory18=0,1DB=0 and4A0=0 prove this first call's empty phase1 team lists and
equal dependency ranks; later-frame dependency/combined branches remain closed.

The first54070 gateway now requires the next selected slot and qualified caller
when this producer is active. It transfers the actual factory664=1 value into the
actor;541AC's540 clear resets it before562B8 writes it again. The cursor advances
only after that actor returns its first world. Generic construction, out-of-order
entry, skipped stage entry, stale claims, expired RNG owners and replay refuse.
Owner moves and real intervening RNG draws preserve the same stream.

This exposed a missing prerequisite that the earlier two-human component tests
did not exercise. The genuine selected Challenge party has active mask0F:

| Slot | Initial model | Current constructor coverage |
| --- | --- | --- |
| 0 | `070B`, variant0 | Joined through first world/model publication, V2 |
| 1 | Sasuke `000A`, variant0 | Joined |
| 2 | `0105`, variant0 | Joined through first world/model publication, V2 |
| 3 | G-Red `0615`, variant0 | Joined |

Slots4/5 are inactive. At the actual initial stream state(low=4,high=6),53B30
draws22/76, starts its circular scan at5, steps+1 and produces0/1/2/3. The full
first dispatch originally stopped at slot0's unsupported constructor. The V1
normal-pair diagnostic retains that refusal (and, with intervening draws, the
slot2 refusal). The explicit V2 AI path below now constructs both actual entries.

Source keys recovered for the next join:

- `070B`: initializer8019C510, config80373920, callback table80373760 and
  class4B8 initializer8019C60C. That initializer callsC627C/FCC50 and has a
  level>8 additional-child branch.
- `0105`: initializer80198490, config8036FC50, callback table8036FA78 and
  class4B8 initializer801985B0. Model010A's alternate-config/child branch is
  distinct from this actual0105 entry.

Both have null4D4 slots. Their later AI behavior, child updates, combat and
program continuations remain separate from the constructor work below.

Native/sanitizer verification passed128 checks per mode across six genuine
scenes. Independent source comparison matched four ordering runs/eight draws,
16 selected-slot rows and18 subsequent constructor RNG draws. It verifies the
actual additional AI entries and the refusal to skip them. Owned order,
constructor, physical-pool and resource-order checks plus combined headers
passed. The existing actor/stage component regression passed6,353 checks per
native/sanitizer mode.

## Initial AI constructors and shared body rendering

Explicit V2 class/setup/level/effect/target/control/program profiles admit the
actual initial070B and0105 members. V1 domains retain their two-model refusals.
Native admission also requires the genuine `InitialAi` roster origin and a
non-human slot; this does not admit edited Borgs or arbitrary new classes.

Both rigs have20 logical bones.070B's complete bank has three records;0105's has
one. Their source part tables begin with a negative sentinel. GFPT2 represents
those empty tables explicitly, leaving per-bone part counts/references empty.
419EC still allocates every normal root followed by every alternate root.
Empty part selection reads no selectors and publishes no fabricated attachments.

The shared scalar/rank/reload/pose/motion systems are reused.55F90 applies the
real AI two-thirds costs(93/66), and the separate27A44 AI gateway reads the
source selector table, yielding334=0 here.263EC takes its actual no-write branch.
Both initial programs retain their original0/8 request, specific0/9 clips and
first unchanged wait. Their descriptor transition/flags differ from the old
pair and are validated separately; time advancement remains closed.

8019C60C now performs070B's C627C/FCC50 child attempts in the shared class2
pool, with the source callbacks, two descriptors each and typed references to
parent168/150/169. The level>8 source branch is represented separately. Parent
flags168..16B and the two(0,10000,0) vectors are written after the calls, including
on allocation failure.0105 takes801985B0's two-byte clear and skips010A's extra
children. Post-class children, body collisions, placement, first/spawn worlds and
model publication then use the same systems as the original normal pair.

Child subfield references follow actor moves and borrow the physical arena's
lifetime. Scene retirement closes those references even if a C++ actor object is
still held. This is native invalidation, not a claim that source cleanup or child
update callbacks executed.

### 070B material prerequisite

One070B body material contains two ordinary diffuse TObjs. Source80242A9C visits
them in order;8023FB48 applies colour mode3 at weight1 with alpha mode0 for both.
The last texture therefore replaces RGB completely while alpha passes through.
An explicit immutable-material decode policy selects that last texture and its
UV1 coordinates through the existing shared renderer. Both texture resources
remain validated. The raw archive and source instance identities are retained.

The legacy decoder still refuses this chain. Contributing alpha, partial blend
weights, different placement/coordinates/transforms, custom expressions/classes,
global texture injection and longer/cyclic chains remain refused. Material
animation is rejected when it could invalidate the reduction proof. This is not
general multi-texture support or a second actor renderer.

Native/sanitizer verification passed2,826 checks per mode across three genuine
scenes, including class2 exhaustion, partial descriptor exhaustion, parent moves
and retirement. The actual0/1/2/3 update order now returns all four first worlds,
constructs the stage and prepares eight actor body/part roots with24 pass visits.
Source comparison matched280 initial-pose components with zero numeric error,
both new child bindings, six AI collision bodies and all eight actor root IDs.
Legacy normal-pair/stage regression passed6,353 checks per mode; owned domain,
material, renderer, constructor/pool and combined-header checks passed.

In this four-actor fixture, actor roots occupy243;260/261/267;309;318/321/326.
After stage construction, free counts are407 class2,478 class3,318 class4 and
1,029 model descriptors. These are component/world receipts, not an active
camera or complete rendered-frame acknowledgement.

## First actor-linked camera entry

The native view owner now retains1910B4/962C4/0685C participant, display and
controller-selection inputs from the same scene. An explicit0BEF4 gateway runs
at the unconsumed factory/order boundary. Factory43D and initial view18 prove
10514's zero result. The actual aliases are3/1/3/1, with the fifth alias still
using the common root. Its487 writes transfer into the subsequently constructed
actors; late alias binding and replay refuse.

After all four first actor worlds and stage construction,0BDA4 initializes
records0..3 and6..9. Each attempts the real class4/group48 selector61 body, using
common98 record96. Typed view references identify parent20/24 and mutable114;
they are not actor or class3 references. Allocation failures leave absent bodies
while the source camera initialization continues. Existing model/class2/class3
occupancy, registry counts, callback queues and RNG are retained.

Normal records run0C660/0C314 against the current shared VM group, source88C
camera operands, rank scalar, placement and heading. The helper uses the source
pitch1536, opposite heading and radius/height arithmetic, then the shared view
matrix kernel. Positive-forward collider114 and negative-Z render394 remain
distinct. C660's actor3F1/5B0 writes have a separate camera generation and do not
pretend an actor world tick occurred.

Alternate records perform the source allocation/HP/self prefix, then0D46C's
43D=0/scene1F=0 early return deactivates them. They keep their initialized
matrices and receive no invented evaluated pose. Camera-body registry updates,
later contacts and cleanup are still unsupported.

First-camera currency follows the exact registered actor/view owners and world
generations. Actor or scene moves retain it; retirement invalidates borrowed
references. The next spawn-world update makes the stored first camera stale
without erasing its source data. Once initial aliases are opted in, actors cannot
skip first camera initialization and jump into their next spawn tick.

Native/sanitizer verification passed682 checks per mode across three genuine
scenes, including actual class4 exhaustion and stale-owner refusals. Independent
comparison matched four normal camera poses, eight collision bindings, five
aliases and188 numeric components with zero numeric error. Camera/physical-pool
owned checks and combined headers passed; the four-actor regression passed2,826
checks per native/sanitizer mode.

Projection remains separate. The located8CBD4→8C440 path selects the actual
05668 projection and058D0/09CBC viewport. For the reached horizontal split,
projection row4 is41°/aspect8:3/near10/far40000; viewports7/8 are640×224 at Y0/224.
The explicit source aspect differs from the viewport pixel ratio. This path also
depends on the real33/31 presentation state and framebuffer composition, so a
default or fitted `Camera` is not substituted.

## First spawn-world camera follow

The native owner now joins0C918→CB8C(0→2)→CDC0 for the first ordinary,
unbound-target/lock-free update after every active actor reaches world generation2.
FC2C retains the source target-height addition order and actor894/898 radius
bounds. CDC0 uses4/5 horizontal and9/10 vertical smoothing.101C8 reads fresh50C
even with null508, quantizes pitch/yaw, applies the width/distance pitch limits
and writes the exact actor's5B0 without advancing its world or program clock.

1063C/10664 preserve the eye before FFFC contracts the orbit. The actual selected
stage bounds gate the shared3DD34 minimum-floor query over STIH grid0. That query
uses strict global bounds, minimum projected height and the source high34-equality
fallback to low68. It is distinct from actor ground selection. A focused test
places a floor between the pre/post-contraction eyes and demonstrates that moving
the query after contraction changes the source2E7 result.

Current view currency now covers camera generations1/2 separately from actor
world generations. Mixed/stale worlds and wrong mutable actor correspondence
refuse before mutation; first-follow replay refuses. Model publication requires
current generation2 camera poses when initial aliases were selected. Moves retain
currency, and scene retirement closes the borrowed owner graph.

Native and ASan/UBSan/float-cast/leak verification passed464 checks per mode over
three genuine scenes. Two successful scenarios followed eight views total and
published sixteen actor roots, including actual first-camera class4 exhaustion
without allocation retry during follow. Pools, registries, queues and RNG were
unchanged by following. Independent DOL/STIH comparison matched four follow poses,
four floor queries and204 numeric components with zero numeric error. All four
floor probes differed from the final contracted eye. Camera-follow/pose, ground,
view/pool unit checks and combined headers passed.

Receipts are retained under ignored `local/recomp-evidence/camera-follow/`:
`verify-v2` and `comparison-v1`. The initial diagnostic compile warning is retained
in `verify-v1`. These are bounded component checks, not a rendered frame or
original-hardware equivalence. Later tracking, targets/locks, contact updates,
program continuation and full frame dispatch remain unsupported.

## Shared horizontal-split projection

The shared `Camera` now accepts an explicit projection aspect and viewport origin.
Point projection, clipped triangles and geometry-cache keys use those same
fields. Existing scenes retain their pixel-derived aspect and zero origin by
default. Both static and character model paths use the shared cache key.

The bounded source projection adapter accepts only row4 and viewports7/8:
41°,8:3,near10,far40000 and640×224 at Y0/224. Both source video tables contain
identical operands for that row. The adapter preserves the supplied orientation;
it cannot initialize a camera, select an active scene or acknowledge framebuffer
history.

Native/sanitizer diagnostics applied those operands to the two independently
verified current follow poses selected by aliases3/1. Independent DOL projection
and view-matrix comparison matched ten points/30 components with maximum error
0.0000458 pixels. Shared projection/cache, pointer-plane round trips, frustum,
scene-pass, character-model and startup-camera sanitizer tests and combined-header
compilation passed, including invalid/overflowing projection-domain refusals.
Receipts are in ignored `local/recomp-evidence/split-projection/{verify-v1,comparison-v1}`.

The remaining presentation dependency is concrete:286D8 calls2AAAC to clear the
retained640×224 texture and reset common31, then stores common33=0.8CBD4 chooses
one current half from common31 parity.2ACD0 restores the retained texture into
the opposite half (Y224 when parity0, Y0 when parity1);2AB70 copies the newly drawn
half back into that same texture, and8CBD4 increments common31. Drawing both
current cameras together would not preserve that history. These source operations
still need the shared renderer's copy/composition owner and complete transition.

Stage continuation tracing also confirms that the post-constructor dispatch
87F20 is empty.87814 instead queues87F38 before the actor callback registration.
That draw callback selects the normal/shadow stage root from the active viewer's
2E7, uses scene45 for descriptor0B, and registers stage roots through097B4.
The initial normal branch is now joined below; the empty stage update does not
authorize bypassing the callback or its gates.

## First stage draw-callback publication

The scene now admits the initial87814→8C0DC→87F38 prefix with its existing
model-publication scope and shared queue. It retains8A16C/53928 actor collection
without executing the later actor callback. The current entry requires scene45=1,
the shared physical-control owner's sceneD9=0, and current generation2 camera poses. Every active
normal view must have2E7=0, proving the same root choice independently of the
not-yet-selected viewport. A shadow or viewer-dependent choice refuses.

The two actual stage descriptors58/64 publish records0/2 in source order, set
descriptor0B=1 and register mask0F with typed same-scope links. Their original
single-root SRTs are evaluated with the shared transform kernel, without actor
user-matrix flags or animation requests. The rigid renderer consumes those same
worlds through `AttachmentModelScene`; its new static-world entry clears actor
fade additions and preserves the existing material, normal and texgen guards.
The shadow root5C and scene45-hidden colour-wrapper60 remain unpublished.

The resulting scope stops at `AwaitObjectCallbacks`. Actor publication cannot
skip that boundary, and the stage queue is not exposed as a completed actor or
whole-scene pass. Invalid recipes and stale camera actors refuse collection
without resetting the scheduler; replay and replacement queues refuse. Scene
moves retain publication; stale frame owners close geometry/queue inspection,
and scene retirement closes the descriptor/object owners.

Native and ASan/UBSan/float-cast/leak diagnostics passed100 checks per mode across
three genuine scenes. Two successful scenes published four stage roots total,
preparing26 meshes/6,988 vertices. Independent DOL/HSD comparison verified the
selected scene's two descriptors,32 matrix components,13 meshes/3,494 vertices/
7,686 indices and six source queue visits. These passes are1/4/2 for queues0/1/2,
with reversed root order inside each queue. No pool allocation, actor-world
advance or RNG draw occurred.

Receipts: ignored `local/recomp-evidence/stage-publication/{verify-v1,comparison-v1}`.
Combined headers and the static/actor-fade rigid-renderer checks passed.
Publication helpers retain private native-owner access restrictions and are
exercised through the genuine scene diagnostic. Existing actor-construction and
actor-model binding sanitizer checks passed. The camera/four-actor publication
regression passed464 checks per native/sanitizer mode, with unchanged output,
under `camera-follow/verify-v3-stage-regression`.

Remaining group0 callbacks before8A0A8 are89EB4 and8A010;89F28/89F9C are registered
in the other two callback queues. Their actual lists, stage props/side updates,
and subsequent collision/child consumers must be joined in source order.
Descriptor0B also controls09838's fog switch, which the eventual shared frame
coordinator must preserve. Active presentation, framebuffer history, full286D8
completion and enclosing frame dispatch remain unsupported. No battle demo has
been launched.

## First stage-object updates and render-list collection

The native stage owner now executes the four actual registered82060→47AA4
side-world callbacks at both initial boundaries: after stage construction before
the first camera, then after actor spawn worlds before camera following. It walks
the typed89554 callback links in their source append order and refuses unknown
occupied callbacks. The existing identity/local-position transform kernel supplies
114, and82060's local(0,180,0) offset is applied after rotation. The admitted
initial progress180/184 is genuinely zero; later side progression stays refused.

The first group40 update runs between the first camera and actor spawn updates.
It traverses the actual shared class2/class3 group, including all56 stage objects,
and preserves source history20→2C and70→76.87B58 sets prop82=1;81DD4 sets side18=1
and82=1; collision-enabled parents receive the source1CA clear. Unknown occupants,
nonempty adjacent per-actor/group47 lists and repeat updates refuse before writes.
Opting into this continuation prevents actors or camera following from skipping
the intervening object-update/world boundaries.

Stage publication now requires those completed object worlds. After8A16C/53928,
89D28 appends all56 selector82=1 objects into the actual group0 render list used
by89EB4, with typed110 links in source traversal order. It no longer leaves that
list empty from constructor-only metadata. This collects objects; it does not
execute their draw callbacks or publish their models.

The sceneD9 gate now reads the existing shared physical-control owner initialized
by8853C, replacing the redundant preparation-entry copy. Source ownership stays
with that control owner.

Native and ASan/UBSan/float-cast/leak integration passed424 checks per mode
over three genuine scenes, including changed-group invalidation after collection.
The retained source group and render110 chains now jointly qualify inspection;
adding an unknown occupant closes the published stage geometry/queue views.
Independent source comparison matched all56 update
links, eight side history positions, four callback links and64 world-matrix
components with zero numeric error. The stage publication comparison still
matches its two roots,13 meshes and six model-pass visits. Shared object/class3
pool, transform and actor-construction sanitizer checks and combined headers
passed. Final integration receipts are in ignored
`stage-publication/verify-v3-currency`; its trace is byte-identical to the
independently compared `verify-v2-objects` trace. Comparisons are in
`stage-publication/comparison-v2-objects` and `stage-object-update/comparison-v1`.
The actor/camera regression passed464 checks per native/sanitizer mode in
`camera-follow/verify-v4-object-regression`, with unchanged output.

Collider registration, shape/contact and visibility consumers remain required
before trusting side/prop270/286 for89EB4's viewer-dependent draw selection.
Object-model publication, the remaining callbacks and frame presentation are
still unjoined; no rendered battle-frame acknowledgement is produced.

Next collision lead: all these bodies use8A65C. Its fresh10=0 branch transforms
the descriptor offset through the typed owner matrix, seeds30/3C history, consumes
start48, dispatches the shape initializer, then applies8AA40's basis-length gate
before8A2BC registration. The reached non-actor records use shape kinds0/1/2/4;
their flags are0x804/0x806,0x6/0x26, and camera record96 uses0x2002. Those non-actor records
do not take the flag1/80 parameter-effect branch; actor banks need their own
observations.2CDA4 clears270/286 before visibility/contact processing, so fresh
constructor zeros cannot stand in for completed frame visibility.

## First complete collider initialization and registration

The native scene now joins8A28C→89AEC→8A65C for all66 first-frame colliders:
33 actor bodies and33 stage bodies. It walks actual group48, active actor groups
49..54, then55, checking the full allocated membership and typed parent/matrix
relationships. Unknown occupants, omitted bound bodies, stale actors and missing
side worlds refuse before registry writes. The admitted boundary is actor world1
and side callback world1, before any camera colliders are allocated.

New sphere0, box1 and capsule2 initialization adapters share the existing source
transforms and registration planner; plane4 reuses the existing plane evaluator.
The box uses the actual eight-corner/six-face source tables. Sphere flag10 changes
the broad radius to50×C0 without replacing its physical radius. Capsule endpoints
use the owner's matrix while radii use C0. Source conditional early returns retain
the cleared fields instead of inventing a fallback shape.8AA40's column-length
gate runs after shape initialization. Shape3/5, delayed clocks, finite end timers,
retirement and flag1/80 parameter effects remain refused.

Initialized fields live in the same class4 arena as construction, while the scene
owns the source-written registry prefixes as typed collision leases. There are74
entries across the ten lists, whose lengths in117C→477C address order are
`0,0,10,4,4,0,0,33,23,0`. Unwritten reference-array tails remain unknown. Registry
inspection distinguishes a current empty list from an unavailable/stale list.
Actual parent activity, C0, visibility/suppression fields and body/bone matrices
are read from their existing owners.

Native and ASan/UBSan/float-cast/leak verification passed514 checks per
mode over three genuine scenes, initializing132 bodies across two successful
scenes. Independent DOL/asset comparison matched66 bodies (25 spheres,16 boxes,
21 capsules and4 planes),2,789 numeric components with zero numeric error, and
all74 registry entries in source order. The shape, plane, lifecycle, registration
and collision-pool sanitizer tests and combined headers passed. Final receipts
are in ignored `collider-initial/verify-v2-currency`; its numeric trace is
byte-identical to the independently compared `verify-v1` trace retained with
`comparison-v1`. The stage/object regression passed424 checks per native/sanitizer
mode in `stage-publication/verify-v4-collider-regression`, with unchanged output.
Changes to the actual class4 topology and retired owners invalidate inspection
without resetting source counters; valid empty registries remain distinguishable
from unavailable ones.

This is initialization and registration, not collision response or visibility.
Once this path is selected, first-camera activation refuses until the intervening
contact/visibility continuation is implemented. Later collider updates and camera
body cleanup remain unsupported. No source tick, model publication, pool
allocation, RNG draw or complete rendered-frame acknowledgement is fabricated.
The numeric evidence is a bounded host model, not original-hardware equivalence.

The next enclosing consumers are2BB14 and36E24.2BB14 resets contact/visibility
fields through2CDA4 for actors, occupied object groups and the twelve view records,
then runs collision/ground/response passes.36E24 clears view26F and casts each
selected active eye→target segment against registry3B7C, writing owner270/286 and
view26F; it also processes active unselected views for descriptor flag800. The
initial views have not been activated yet, but that does not authorize skipping
2BB14's actor/object state updates or its nonempty ground/shape lists.

## First contact-reset and sphere/plane prefix

The next native continuation joins2BB14's initial actor scalar stores,2CDA4
resets and2CF80's first sphere-versus-plane traversal. It takes the exact mutable
registered actor objects; copied identities or a different pointer order cannot
authorize the writes. World/camera generations and actor lifecycle revisions do
not advance merely because contact fields changed.

The shared reset kernel preserves source order for1DB→1DE and274→275 before
clearing contact/visibility fields. Nonzero byte timers decrement with the
source wrap semantics. The scene walks the occupied source groups in
40/32/0/16/8/24 order and resets only collision-enabled objects. Existing
actor-child constructors retain their known no-collision branch; an unknown
occupied object refuses rather than becoming an assumed empty group. All twelve
inactive pre-camera view records receive their unconditional reset; the
thirteenth common record stays untouched. View reset observations have their own
typed owner kind; they are not encoded as cells in an unused physical arena.

The actual reset order comprises four actors,21 collision-enabled stage objects
and12 view records. Source actor scratch prefixes are explicit, while inactive
slots remain unknown.803B0708 is one owner: its initial−10000 sentinel is then
overwritten by the current sphere's centerY−radius, not stored as an unrelated
default or second scratch field. Team118 values remain with the scene-service
owner established by the fresh Challenge context.

2CF80 traverses the genuine297C spheres against237C planes.39F6C uses its strict
radius>0.1 gate and inclusive distance≤radius-sum test. All16 reached pairs are
separated in this scene; no narrow-phase miss is invented. Overlap explicitly
refuses the unjoined30348/303B0 response before committing this bounded prefix.
The source3A05C displacement operation has no effect after the actual1DD resets.

This entry stops at2D35C. It does not claim ground/contact response,36E24
visibility, frame completion or a drawable scene. Camera activation remains
guarded. The next3B7C pass has33 current bodies:27 carry flag0x800, while the other
six are frozen stage bodies.357C/477C/177C are genuinely empty in the initialized
registry prefixes. Those facts are recovery leads for the following source
branches, not permission to omit the rest of2BB14.

An independent comparison verified37 reset owners in source order, all16
broadphase pairs/32 distance and radius-sum components with zero numeric error,
and the four current actor sphere-bottom values. It uses the separately verified
collider trace and source DOL constants. Final native/sanitizer verification
passed249 checks per mode over three scenes, with74 resets and32 broadphase tests
across the two successful scenes. Exact mutable actor correspondence and
reset-group topology changes are covered. Shared reset/broadphase, initial-shape,
actor-construction and object-pool sanitizer tests and combined headers passed.
Receipts are in `contact-prefix/{verify-v4-typed-owners,comparison-v2-typed-owners}`.

Private receipts remain under ignored `local/recomp-evidence/contact-prefix/`.
The source baseline−10000 correction is retained in `inputs-v2`; earlier inputs
and build attempts were not overwritten. `next-pass-audit-v1` records the exact
six-call source order and the following registry gates, explicitly as an audit,
not an executed continuation or original-hardware equivalence.

## Initial contact-tail and pre-camera visibility join

The native initial contact pass now continues from2D35C through2BB14's return
for the qualified first scene. The actual3B7C outer loop examines32 candidates:
27 exit on descriptor flag0x800, and five exit on the current stage owner's281
freeze flag. The final registry body is not an outer candidate, matching the
source loop rather than counting it as another executed test.

The remaining initial passes are admitted only through their reached gates:
357C is empty for2D58C/2E1C8;477C and177C are empty for the attack/effect passes;
the actual reset1DD fields prevent attachment/ground correction;71E/71F do not
request peer propagation; current health remains positive and unchanged.2CE44's
collider-center correction has no displacement or ground-correction scratch to
apply. All four actors take5A580's ordinary in-bounds return using the same stage
bounds and source20-unit margin.

These are explicit non-response branches over the current owners and registries,
not general collision response. A movable solid pair, nonempty attack list,
damage/death, forced663 state, attachment, position correction or bound-clamp
branch refuses. Actor contact generation advances separately from world, program
and camera generations; it does not fabricate another actor tick.

36E24 then performs the actual thirteen view26F clears. Every view is still
inactive before the first0BDA4, so no eye→target shape query is reached. The
following8972C/89658 callback lists must be genuinely empty. Only after this
boundary may the collider-opted-in path activate its first cameras. Their eight
new class4 allocations are not retroactively registered; the earlier pre-camera
collider/contact observations become historical when that actual topology changes.

Native and ASan/UBSan/float-cast/leak verification passed134 checks per mode over
three genuine scenes. Two successful scenes complete the initial contact pass,
clear26 inactive-view fields total, activate the actual first cameras and then
admit the first56-object update. Wrong actor pointers, stale owners, changed
collision topology, replay and skipped visibility refuse.

Independent DOL/registry comparison verified all32 outer-candidate gates,
the three actual empty registry prefixes, four current positive-health/in-bounds
actor returns and thirteen inactive-view clears. Unit/sanitizer tests for the new
tail gates, shared contact reset and initial shapes, plus combined headers,
passed. Actor-construction, shared class3-pool, view and plane-lifecycle
sanitizer regressions also passed. Receipts are under ignored
`local/recomp-evidence/contact-tail/verify-v2-spawn-boundary` and `comparison-v1`.
The final trace matches the independently compared `verify-v1` trace byte for
byte; additional checks exercise refusal at the later spawn boundary.
The camera/four-actor regression passed464 checks per native/sanitizer mode in
`camera-follow/verify-v5-contact-regression`, with byte-identical output to the
prior verified regression.
Final native/sanitizer diagnostics use an immutable source snapshot for both
builds, so edits during a long sanitizer compile cannot mix two trace formats.

Later contact updates, active-view visibility casts and camera-body retirement
remain unsupported. This joins the initial contact/visibility boundary only,
not the subsequent spawn-frame contacts, object draw callbacks, presentation
history or full frame dispatcher. There is still no valid rendered battle demo.
On the collider-opted-in path, the existing follow-camera adapter now refuses
advancement across the missing post-spawn collider/contact/active-view visibility
sweep; completing initial contacts is not authority to skip the next frame.

Next collider-sweep lead: the first0BDA4 allocates view bodies in0/1/2/3/6/7/8/9
order, then deactivates alternate parents6..9. The next group48 traversal reaches
those four inactive parents first and8A65C requests88E50 retirement. That class4
retirement unlinks the shared group and returns the cell through89814 without
class2/3 model or renderer cleanup; it does not clear the parent's86 flag.
Only a proven never-registered phase0 view body may take a narrow native cleanup
entry. The remaining four active view bodies must initialize from their actual114
matrices; existing actor bodies need their source0/2/4 update handlers, while
frozen stage bodies retain their shape fields and re-register. Do not retry
earlier allocation failures or blanket-reset the populated pools.

## First post-spawn collider sweep

The next scene entry now walks the actual48/49..54/55 groups after every actor
and side object has its current spawn world. It is separate from the initial
collider receipt and requires the completed initial contact/visibility path,
first camera allocation and first stage-object updates. Unknown occupied bodies
and stale owners refuse before the registry reset or any retirement.

Initial inactive alternate-view bodies take the narrow88E50 class4 branch:
their exact original leases, parent identities, uninitialized phase and absence
from the earlier registry prefixes are checked first. The shared group is unlinked
and89814 returns the cells through the existing FIFO. The generic public cleanup
refusal for bound bodies stays closed, and parent86 is retained.

Normal active view bodies initialize from their actual first-camera114 matrices.
The33 actor bodies consume the shared sphere/capsule/plane update kernels and
current logical/body matrices; the33 frozen stage bodies hold their previous
geometry but reapply the current frame-column filter and registry membership.
No allocation failure is retried. The new source-written registry prefixes replace
current authority without clearing or inventing the unobserved array tails.

A source quirk is explicit:3B304 at3BD54/3BD58 squares
`travel + extentLength*C0 + radius` after writing the broad-bound field. The native
adapter preserves that store rather than silently repairing it to a linear bound.
Sphere updates retain radius and center history; capsule endpoint history and
plane history remain distinct. Swept-effect flags1/80, shape3/5 and arbitrary
non-frozen box updates remain unsupported.

Shared step/initial-shape/plane-lifecycle/collision-pool sanitizer tests and
combined headers passed. Genuine-scene diagnostics include normal operation,
an unknown occupied group48 body, stale actors, camera class4 exhaustion followed
by a real free cell, and generic-cleanup refusal. Their receipts and source
comparison live under ignored `local/recomp-evidence/collider-spawn/`.

Native and ASan/UBSan/float-cast/leak verification passed126 checks per mode over
four genuine scenes. The three successful scenes retired eight inactive view
bodies, initialized eight active view bodies and updated/held99 actor/stage
bodies respectively. `verify-v2` uses one immutable source snapshot for both
build modes; the implementation hashes match the current native source.
The first-contact/camera-boundary regression passed134 checks per native/sanitizer
mode in `contact-tail/verify-v3-spawn-sweep-regression`, with unchanged output.

Independent DOL/asset comparison matched the full74-body traversal: four inactive
views retire first, four active views initialize next, then33 stage holds and33
actor updates follow the original group order. It matched2,094 numeric components
with zero error, all78 registry entries and the source FIFO result
head74/tail70/free314. First-camera matrices are also checked against the separate
camera-entry receipt. The exhaustion scenario contains no camera bodies and must
not retry initialization when a cell is later freed.

The first diagnostic retained an overly strict expectation that the shared
class2 snapshot would not change its group48 head during class4 retirement.
That expectation was corrected to the exact shared-list mutation; the failed
receipt is preserved. The native allocator/list operation was not replaced.

Spawn contact response and active-view visibility are still separate and remain
guarded. Following the camera or publishing a frame cannot skip those consumers.
The next contact lead has sixteen sphere/plane broadphase candidates, still
separated in a read-only numeric audit of the new trace. That audit is not an
executed second contact pass. The active-camera visibility casts remain real
nonempty3B7C shape queries even if the contact pass takes non-response branches.

## Spawn contacts and active-view visibility

The spawn contact owner now consumes the current post-spawn collider sweep and
the exact mutable actors. It shares2CDA4's reset and the actor scalar-prelude
kernel with the initial contact entry. Source scratch arrays retain explicit
active-slot prefixes and unknown inactive tails. The current actor/stage/view
contact fields, sixteen sphere/plane broadphase pairs and the remaining
non-response gates are checked before committing; general damage, attachment,
ground correction and movable-solid response still refuse.

Active36E24 visibility now walks the actual3B7C registry. Selected aliases3/1
cast against every eligible stage body; active unselected views0/2 cast only
against flag0x800 bodies. All thirteen view26F fields clear before results are
stored. Successful selected-view queries OR the source view bit into parent270,
and their source inside flag controls286 independently. Only descriptor flag800
hits set view26F. Unknown actor/view-specific owner filters remain unsupported.

The shared shape-query path covers the reached sphere0, box1, capsule2 and
plane4 cases. It reuses the existing plane/segment kernel and source float vector
operations. Source box clipping keeps its ordered plane traversal and distinct
only-end-clipped flag; sphere root selection retains the strict tangent fallback.
Capsule queries implement38C00's nonparallel projection/clamp branch, including
the source last-qualifying coordinate choice and its coincident-point X+=2
adjustment. Parallel closest-segment table paths and shape3/5 remain refused.
Unsupported queries cannot become invented unobstructed rays.

36E24's broad gate also preserves its reached source arithmetic:8020BC88's
**squared** eye-to-target distance is multiplied by0.5, not square-rooted into a
half-length. This is separate from the shape's physical radius and cannot be
"corrected" to an ordinary sphere fit without changing source behavior.

The genuine scene performs120 visibility candidates:60 box,48 capsule,8 sphere
and4 plane queries. There is one hit, setting class3 side-parent0's270 mask to2
for viewer1, while its286 mask stays0. All view26F flags remain0 because that hit
does not carry descriptor flag800. The following ordinary camera path does not
read26F in its admitted branch; it still performs the existing actor-world2
follow, source floor probe and generation2 pose publication.

Final native and ASan/UBSan/float-cast/leak verification passed88 checks per
mode over three genuine scenes. Two successful scenes execute240 visibility
candidates and two hits, then reach the existing follow-camera owner.
Independent DOL/shape comparison matched632 numeric components with zero error,
all120 ordered candidates,21 parent mask pairs,13 view flags and16 contact pairs.
Shared query, reset, actor-construction and contact-tail sanitizer tests and
combined headers passed. Final receipts are under ignored
`visibility/verify-v2-shared-scratch`; its implementation hashes match current
source, and its numeric trace is identical to the independently compared
`visibility/verify-v1` trace retained with `comparison-v1`.
The initial contact/camera-boundary regression passed134 checks per
native/sanitizer mode in `contact-tail/verify-v4-visibility-regression`, with
unchanged output. Active source scratch prefixes and unknown inactive tails are
tested explicitly; both contact entries reuse the same scalar-prelude kernel.

Object draw callbacks must now consume the real270/286 masks. In particular,
820C8 cannot be replaced with a generic normal-root draw for that occluded side:
its current per-view factor and prepared styled root must follow the source
material/alpha branch. Its initial factor1.0 takes the hidden>0.5 branch directly
to0.5; the ordinary gradual step is the separate source0.0075 value, with a0.25
floor. Those values come from80437AAC/7AC0/7AC4/7AC8, not an invented fade.
Presentation parity/history, remaining callbacks and the
enclosing frame dispatcher are still unjoined. There is no valid battle demo yet.

## Camera-invariant prop draw-callback prefix

The initial group0 draw list begins with all48 props in actual110-link order:
static records50→33, then ordinary records32→3. The qualified scene's prop270
masks are zero for every active view, so87C84/87B64 select the same normal E0
root for any reached viewer. This prefix can therefore execute without inventing
common28, framebuffer parity or a caller-selected camera.

A fresh collection entry registers the source stage callback first, then the
89EB4/89F28/89F9C prefix into callback groups0/1/2. Stage publication consumes
only the first callback;89EB4 then advances48 nodes inside its still-incomplete
callback. The source callback cursor stays1 and the object cursor becomes48.
The next actual object is side ordinal7, class2 index86. Actor publication remains
guarded; the side and child callbacks are not silently acknowledged.

Each prop publishes its existing exclusive normal root, descriptor0B=1 and
typed source queue links. Ordinary props register mask0x17; static props use0x14.
Together with the two stage roots, the resulting partial queue has114 visits.
Shadow roots remain unselected and unpublished.

All48 authored prop records have zero Euler rotation and unit scale. The new
bounded authored-tree adapter uses the shared translation/matrix kernels for
their59 joints, preserving original local metadata and flags. It does not import
actor076D0/user-matrix flags, request animation, fit a pose or admit arbitrary
hierarchy/constraint transforms. The material/source-root/lifetime checks remain
in the existing scene-model binding owner.

Independent DOL/HSD comparison matched all48 descriptors,59 joint identities,
944 matrix components with zero error, source root-selection/order/masks,114
queue visits and the next side-object identity. Shared profile/model-binding/
rigid-renderer/model-queue sanitizer checks and combined headers passed.
Receipts are under ignored `local/recomp-evidence/prop-publication/`.

Native and ASan/UBSan/float-cast/leak verification passed528 checks per mode across
three genuine scenes. Two successful scenes publish96 prop roots/118 joint
worlds total. Wrong recipes, a stale actor, callback/root replay, a skipped stage
callback and premature actor publication refuse. Moves retain the scoped graph;
scene retirement closes it. `verify-v2` uses one immutable source snapshot per
normal/sanitizer pair, and `comparison-v1` retains the independent source result.
The earlier compiler-formatting warning is preserved in `verify-v1`; no failed
receipt or original input was overwritten.
The contact/visibility/follow-camera regression passed88 checks per
native/sanitizer mode in `visibility/verify-v3-prop-regression`, with unchanged
output. Implementation hashes match the source used by the final diagnostic.

This is world/model-queue publication, not skin/geometry preparation or a draw.
The multi-joint and material-bearing prop geometry still needs the shared
renderer adapter. Side820C8 is genuinely viewer-dependent for the previously
occluded parent0, so this entry stops before any side callback. Presentation,
side/child continuation and complete frame dispatch remain unjoined; no valid
battle demo has been launched.

## Published prop geometry and immutable material reduction

The shared `AttachmentModelScene` now has an explicit published-static hierarchy
entry in addition to its unchanged legacy single-joint/supplied-transform entry.
It takes the exact retained source bank and the existing published root, remaps
the source joint/palette identities, and evaluates its vertices/normals with the
shared `hsd_skin_sample` kernel. Draw submission still uses `MenuScene` and the
shared model/render engine; no parallel battle/Story renderer was added.

The entry preserves source PObj/material identities and skin bindings, checks
cross-record influences, and retains the existing skin, normal, texgen and
material refusals. The published root's local metadata is not applied a second
time: evaluated world geometry is submitted through the existing identity-geometry
path, with per-mesh world origins. Legacy hierarchy/alternate/material entries
have not been broadened.

Published-world receipts now borrow both the exclusive root identity and a
per-publication epoch plus its existing world scope. Copies cannot keep retired
roots or expired/replaced publications valid. Prepared constructor metadata cannot
mint such a receipt. Native scene geometry inspection also retains its actor/frame
currency gates; scene moves preserve ownership and retirement closes it.

Preparation reached two source boundaries rather than bypassing them:

- Record35's TObj uses one colour-only custom TEV expression:
  `white - mix(one, black, TEXC)`.8023F280's selectors and immutable constants reduce
  that exact normalized RGB expression toTEXC; alpha activation is absent.
  An explicit published-static policy admits only this exact single stage,
  unchanged weight/UV transform/default class and no global expression. Partial
  weights, changed inputs/constants, alpha activation, additional stages and
  animation requests still refuse. Default/legacy decoding continues to mark it
  unsupported. The original bank/root material metadata is not rewritten.
- Records9/10/11/16/18/26/33 are authored empty static records. The shared bank's existing
  `is_empty_static_record` proof is required to acknowledge zero geometry, and no
  triangles or texture uploads are invented. These seven records are distinguished
  from the41 records with geometry. Other unexplained empty selections
  still refuse.

An independent DOL/container audit verifies record35's selectors/constants,
colour-only activation and the identity for all256 channel values. This is a
bounded renderer-expression proof, not original hardware TEV timing or pixel
equivalence. The first refused-material and empty-record diagnostics remain
retained under ignored `local/recomp-evidence/prop-geometry/`; no receipt/input was
overwritten. Shared identity-colour, rigid-model, joint-instance and skin
sanitizer checks and combined headers passed.

Native and ASan/UBSan/float-cast/leak verification passed791 checks per mode over
three genuine scenes. Two successful scenes prepare96 prop records,126 meshes
and21,906 vertices, including eight multi-joint records and fourteen explicitly
empty records total. The default published policy still refuses the custom
material; source descriptors, worlds, model-pass order, allocators and RNG remain
unchanged during preparation. Scene moves retain the borrowed world epochs and
retirement invalidates them.

Independent source display-list/palette decoding matched48 records,59 joints,
63 meshes and10,953 vertices for the selected scene. All131,436 source/world
position and normal components matched with zero numeric error. Every reached
PObj is rigid, including meshes owned by the multi-joint trees; no new blended
skin coverage is claimed. Exactly one identity-colour reduction was applied,
to record35, without changing its original metadata. Receipts are in ignored
`prop-geometry/verify-v3-empty-record`, `comparison-v1` and
`identity-colour-comparison-v1`.
The prop-publication regression passed528 checks per native/sanitizer mode in
`prop-publication/verify-v3-geometry-regression`.

The prop hierarchy/skin source comparison is recorded separately from callback
publication. Side/child callbacks, presentation history and the complete native
frame controller remain unjoined. Geometry preparation is not a rendered battle
frame or an actor substitute; there is still no valid battle demo.

## Camera-invariant side callback and geometry prefix

After the48 props, the actual89EB4 list visits side ordinals7→0. The first seven
have source270=0 for every active viewer and every per-view factor already1.
820C8 therefore leaves the selected factor at1, selects the normal E0 root,
applies7CAC alpha1 and copies the current object114 through076D0, independent
of which active viewer will be presented. These idempotent choices do not mint
common28/parity or acknowledge the final viewer-dependent side.

The existing native model/queue owner now publishes those seven exact roots.
Static variants retain mask0F; ordinary variants use47. With stage and props,
the scoped partial queue has135 visits. The89EB4 object cursor becomes55 while
its callback cursor remains1. Next is the actual occluded side0, class3 index0.
Both of its prepared root alternatives remain unpublished; actor callbacks still
cannot skip it or the following child callbacks.

The world adapter checks the exact original one-root source, parent matrix and
material context. A plain default material or actual ScopedCache with source
RGB0 is required; equal values in a different context do not substitute ownership.
A dedicated publication marker and world epoch authorize the shared renderer's
explicit one-root matrix entry. Legacy static and actor-fade entries retain their
refusals. In particular, the source object matrix replaces record1's authored
Y=1 rather than adding that translation twice.

The four static side roots use the same colour-only identity TEV expression as
prop35, with ordinary texture colour/alpha interpolation modes3/2 at weight1.
The opt-in identity proof now accepts exactly that additional flag/mode pair and
preserves the ordinary modes. Default/legacy decoding, partial weights, alpha
activation, extra stages and changed expressions still refuse. The first failed
strict-material diagnostic is retained, not bypassed.

Independent DOL/side-bank comparison matched seven root identities/banks,
fourteen meshes,2,332 source/world vertices and14,104 numeric components with
zero error. It also matched135 source model-queue visits and the next object
identity, and verified the static-side TEV inputs/constants directly from both
original banks. Shared side-profile, rigid-model, material-proof, model-binding
and skin sanitizer tests and combined headers passed.

Final native and ASan/UBSan/float-cast/leak verification passed329 checks per
mode across three genuine scenes, publishing fourteen side roots and preparing
28 meshes/4,664 vertices across two successful scenes. Bad profiles, stale actors,
geometry replay and premature actor dispatch refuse; all physical pool state,
source callback lists and RNG are retained. The same scene also prepares both
stage roots and all48 prop geometry records after the side cursor advances.
Final receipts are in ignored `side-publication/verify-v3-combined-geometry`;
its trace matches the independently compared `verify-v2-material-identity`
trace byte for byte, retained with `comparison-v1`. Tested implementation hashes
match current source. The prop geometry/material/lifetime regression passed791
checks per native/sanitizer mode in `prop-geometry/verify-v4-side-regression`,
with unchanged output.

This seven-side entry remains camera-independent. The following presentation
fragment supplies the first viewer-dependent continuation; child callbacks,
history capture and whole-frame coordination remain unsupported.

## Initial presentation fragment

The presentation owner now represents only the initial horizontal-split graphics
fragment:2AAAC's1MiB zero history allocation/common31 reset, the later common33
zero write and8CEC8's first common34=0 pass. It does not acknowledge286D8 or the
enclosing frame dispatcher.

At the current generation2 camera boundary,8CBD4's first parity0 draw chooses
alias0, hence actual view3. The shared camera uses source projection row4 and
viewport7, not caller selection or a fitted scene camera. The shared renderer
uploads the640×224 RGBA8 history and queues2ACD0's white, non-blended restore
quad only in the opposite half(Y224..448), then restores caller render state.
Raw PPC FIFO stores, rather than the incomplete decompiler's quad output, fix
that rectangle. The1MiB cleared allocation and0x8c000 image/copy extent are
distinct.

The first presented side callback uses that owned viewer and an active weak
render-frame receipt. View3 takes side0's unoccluded E0/alpha1 branch; the
occluded viewer1 E4/alpha0.5 branch still refuses. Its source queue prefix has
138 visits and object cursor56, but callback cursor1 is deliberately retained:
unknown non-stage collection and child callbacks cannot be skipped.

Render-frame receipts expire at frame end, replacement, reinitialization,
shutdown and destruction. They authorize an active graphics scope only, never
a successful/composed world frame. Presented publication inspection additionally
requires the current scene, actor and camera owner graph.

Source-profile/frame-receipt sanitizer tests and combined headers passed.
Final genuine-scene verification passed196 checks per native/sanitizer mode
across six scenes, preparing side0's two meshes/722 vertices in five successful
scenes. It covers bad profiles, missing frames, wrong canvases, missing copy
capability, insufficient texture limits, reserved shared textures, active model
passes, invalid lighting, replay and late upload failure. Frame end/replacement,
shutdown and stale actor owners close presentation-dependent inspection.
Independent comparison matched4,341 numeric components with zero error,
including all source/world vertices and the separately verified generation2
view3 camera. It matched138 source queue visits and raw PPC retained-half quad
operands. Receipts are in ignored
`initial-presentation/{verify-v1,comparison-v1}`. Final combined geometry,
scene move/retirement, edited-side-recipe refusal, physical-list/RNG invariance
and frame-lifetime checks passed in
`initial-presentation/verify-v2-combined-lifetimes`. Its trace matches the
independent comparison byte for byte, and all snapshotted source hashes matched
that tested checkpoint. All stage/prop/seven-invariant-side geometry remains preparable
after the final side advances the object cursor to56. The seven-side combined
stage/prop regression passed329 checks per native/sanitizer mode with unchanged
output in `side-publication/verify-v4-presentation-regression`.
Combined headers, seven renderer/projection/profile sanitizer executables and
the native host build passed; the earlier startup-feedback and legacy-copy
compatibility sanitizer checks also passed.
History capture2AB70, common31 increment, world rendering coordination and the
enclosing transitions remain unimplemented. No battle demo has been launched.

## Actual non-stage occupancy and first child update

A read-only genuine-scene audit traversed every typed group chain, checking
previous/next links, cycles, tails and parent ownership. It found209 grouped
physical records,153 outside stage group40, including25 owned actor children.
Those153 records are not153 renderable objects: collider/preparation records
and callback-pending children remain distinct. The audit passed822 checks per
native/sanitizer mode without changing any pool, callback list or RNG state.
Receipts are in ignored `nonstage-collection/audit-v1`.

89A94(0)'s first active actor-controlled group is the actual slot0 group1,
whose class2 order is61→60. These are070B's type3 point follower and field
follower, respectively. The new narrowly owned continuation initializes only
the first point follower, before the first89554 object-world boundary. It
cannot be invoked retroactively on the already prepared stage-only scene.

The point child registers47AA4 in the shared scheduler's actual callback0,
sets source84=54/bone89=1 and binds two separate07030 record1 roots through
the parent's existing archive and scoped cache. Only the second root receives
07C54/07CAC(.4); source textures and diffuse colours remain. These are prepared
child roots, not published child worlds or actors. No model descriptors or
physical cells are allocated. First FCEFC smooths the referenced Point150 into
the cleared14C using source46588's separate scale/add operations, then adds
parent1DBC as a vector. Flag169=1 does not enter the high80 effect branch or
clear the flag. B4/B8 inherit the parent scale;58/5C/60 retain source1.

The child render selector is49, not65: its eventual source draw belongs to
8A010's child list, whereas the following field follower's selector65 belongs
to slot0's actor-controlled list. Neither list may be replaced with the four
selected actors. The first-child receipt records next class2 index60 and keeps
all-child-updates and render-collection completion false. The stage-only89554
entry now refuses when this real callback0 is occupied, rather than skipping
the pending child world callback. No source frame has been acknowledged.

Source-profile/numeric and shared pool/model/geometry sanitizer tests and
combined headers passed. Genuine first-child verification passed70 checks per
native/sanitizer mode across four scenes. Two successful scenes initialized one
point child each and prepared four roots total; stale parents, bad recipes,
replay and retroactive insertion after89554 refuse. Actor/scene moves preserve
ownership, and parent/scene retirement closes inspection or prepared identities.
Independent original-bank/DOL comparison matched record1's one joint/four
meshes per root, all eight normal/styled material states, descriptor indices
251/252, source order61→60 and six numeric components with zero error. It also
verified46588's raw split-scale/add calls and the source phase/type tables.
Receipts are in ignored `initial-child-updates/{verify-v2,comparison-v1}`.
The exporter also refused existing outputs without modifying them.
Presentation and combined stage/prop/side regressions passed196 and329 checks
per native/sanitizer mode, respectively, with byte-identical output. Receipts:
`initial-presentation/verify-v3-point-child-regression` and
`side-publication/verify-v5-point-child-regression`. Final source confirmation
matches current engine/test/diagnostic C++ hashes against all three immutable
snapshots and matches the independent point-child trace. This preserves the
earlier bounded presentation entry; it does not make the child update prefix
compatible with silently skipped sibling/world callbacks.

The next sibling, class2 index60, uses source row3 at803052FC, selecting
record1/bone12, local offset(0,30,0), zero angular offsets, no auxiliary child,
zero follow/bob/angle-step and no flag80 effect. Its C6544 body is recoverable
from raw PPC despite the decompiler's truncated entry; preserve its separate
46588 smoothing and final FMADD, not a generic interpolation replacement.
Its callback flags1E require an explicit world-position-plus-scale47AA4 branch;
the existing0E adapter must retain its refusal. The following continuation joins
its initialization, not the pending world callback or all of group0.

## Initial070B field-follower sibling

The owned sibling continuation requires the completed point-child receipt,
actual group1 tail60 and the existing callback0 singleton61. It initializes
only the genuine Flag168 follower, appending60 after61 in that same scheduler
list. It binds two independent record1 roots with the parent's existing archive
and cache, reusing the point child's scoped binding/material adapter; only the
second root receives source07C54/07CAC(.4). No pool cells or model descriptors
are allocated, and these prepared roots publish no child world.

The child takes actual parent world1/logical bone12, not a caller-provided
matrix. C63E8 transforms row3's local(0,30,0) and initializes148. The immediate
C6544 repeats the source transform/split-scale-add follow, copies parent angles
plus the row's zero offsets and preserves the separate FMUL/FMADD bob operation.
Source follow/bob/angle-step are zero for this exact row.58/5C/60 copy parentB4;
B4/B8 retain their constructor copies. Fresh parentA8 is copied as zero.
Flag168=1 neither clears the flag nor enters the high80 effect path.

Render82=65 is distinct from the point child's49; the field child eventually
belongs to slot0's actor-controlled render list. The update cursor advances to
the actual next active actor group: slot1/group2, head class2 index67. It does
not declare all group0, all children, render collection or callback execution
complete. Unjoined child updates and the pending61→60 world-callback list still
block the stage-only89554 adapter.

The shared attachment numeric kernel now has a separate, explicit1E
world-position/rotation/scale entry. It shares the existing Z/Y/X math rather
than inventing a parent matrix. The old0E, copied-bone and local-position
adapters retain their refusals. This numeric entry does not mint a child world
receipt or execute89554.

Combined headers and six source-profile/transform/pool/model sanitizer
executables passed. Genuine sibling verification passed73 checks per
native/sanitizer mode across four scenes. Two successful scenes initialized
one field follower each, preparing four field roots total. Callback order61→60,
next slot1/head67, parent immutability, no allocation/RNG changes, replay and
stale-owner refusals, actor/scene moves and retirement are covered.
Independent DOL/model-bank comparison matched both record1 roots, their one
joint/four meshes per root, all eight material states, descriptors249/250,
source group order61→60→67→66 and25 numeric components with zero error.
The numeric comparison uses the actual owned parent world1 bone12 as its input;
it is not an independent reconstruction or original-hardware proof of that
parent world. No child world or full frame was published.
Receipts: ignored `initial-child-updates/{field-verify-v1,field-comparison-v1}`.
Bounded read-only extraction also refused existing outputs without changes.
Point/presentation/stage regressions passed70/196/329 checks per native/sanitizer
mode with unchanged trace and diagnostic output. Receipts are
`initial-child-updates/verify-v3-field-sibling-regression`,
`initial-presentation/verify-v4-field-sibling-regression` and
`side-publication/verify-v6-field-sibling-regression`. Source confirmation
matches the tested C++ hashes against all four immutable snapshots and matches
the field-child comparison trace.

Next source children are Sasuke type7 at67, then type6 at66. Reuse
`startup_attachment_state_step` and the shared attachment transform rather than
introducing another attachment engine. Their model records are14/11; source
mode0 rows use bones11/7, positions(20,-20,0)/(-8,-3.2,6.2), and zero angles.
Recover/consume actual parent6E8 from the accepted VM's `actor.control0[2]`
before selecting a mode; these mode0 facts are not permission to assume it.
The isolated `StartupActorBankAttachments` owner declares parent fields and
does not own this physical scene, so it cannot substitute for the native child,
callback registration, descriptor or parent-lifetime receipt.

## Initial Sasuke attachment children

The owned continuation now executes the actual group2 order67→66 (type7 then
type6), after both070B children. It reuses the existing GFAT parser,
`startup_attachment_state_step` and attachment transform, rather than the
isolated `StartupActorBankAttachments` owner. A sealed source envelope pins
the unchanged Sasuke model hash and all imported mode rows; edited operands,
wrong banks, stale parents and out-of-order dispatch refuse.

Mode6E8 comes from the accepted current VM's `actor.control0[2]`, not a default
or remapped actor index. The genuine scene observes mode0. Source record14's
type7 initial draw branch is hidden in that mode; record11's type6 branch is
visible. These are read-only branch observations: neither draw callback has run,
and both normal/styled roots are still prepared, not published child worlds.
The second root alone receives source07C54/07CAC(.4), retaining its texture and
diffuse data. Both children receive D850's unit B4/B8/58 scale stores.

89100 stores actual source selector11 in byte85 and appends67→66 to scheduler
list index7; do not confuse that index with the source selector. All other
callback lists, physical cells, model descriptor free state, parent program/world,
shared cache and RNG are retained. The first-child update cursor becomes4 and
continues to actual slot3/group4 head80; slot2's group3 is genuinely empty.
There is no acknowledgement of all group0, remaining children,89554, render
collection or a battle frame.

Owned source-profile/shared-state/transform/model sanitizer tests and combined
headers passed. Genuine verification passed132 checks per native/sanitizer mode
across four scenes: five successful child initializations (including one
interrupted between siblings) and ten prepared roots. Stale owners before and
between siblings, replay, source cleanup refusal, actor/scene moves and native
retirement are covered.

Independent DOL/model-bank comparison matched both source records, all twelve
normal/styled material states, both normal joints, original descriptor lineage,
actual mode0 and source order67→66→80→79. It matched38 numeric components with
zero error. Numeric transform inputs are the actual owned parent world1 bones,
not an independently rebuilt parent-world/original-hardware proof. Receipts are
ignored `initial-child-updates/{sasuke-verify-v1,sasuke-comparison-v1}`. The
read-only bounded exporter also refused existing outputs unchanged.
Earlier child/presentation/stage regressions passed73/70/196/329 checks per
native/sanitizer mode for the retained Sasuke checkpoint, with byte-identical
output and intact immutable snapshots. This predates the following G-Red
extension, which is verified separately. Sheet02's four fresh native captures
matched all RGB pixels against the approved Sheet01 baseline, with zero changed
pixels in every view. Sheet02 was shown in Review and the user confirmed that
it still looked good, with no new visual flaws. The captures still exclude
actors/children and do not acknowledge battle-frame composition.

Next are G-Red type25 at80, then type24 at79. Reuse the shared GFAT/state/transform
logic, but preserve the existing isolated helper's parent-index0/1 refusal:
actual G-Red is slot3 and must not be remapped to a preview actor index to pass
that gateway. Any native-slot extension needs a separate explicit source-bound
entry and tests.

## Initial G-Red slot3 children and empty update groups

The new entry keeps the legacy isolated attachment helper restricted to
parent indices0/1. A separate sealed fresh G-Red entry requires actual slot3,
unchanged model0615 and the exact type24/25 GFAT rows. Both entries call the
same numeric implementation; the native path never substitutes preview index1
for source slot3. Parent6E8 is the accepted VM's full control byte, not the
low7-bit Sasuke mode.

The scene-owned continuation follows actual group4 order80→79 (type25 then24),
after both Sasuke children. It registers selector13 in byte85 and appends those
nodes in scheduler list index9, retaining all earlier lists. Render82 is68.
Both children bind independent normal/styled roots through the existing owned
archive/cache and shared model adapter: records9/10, with source .4 alpha on E4.
Their draw-only144 scale is distinct from callback114's unit-scale bone basis;
zero draw scale must not collapse a pending callback matrix or mint an actor.

After the six actual group0 children, a separately profiled gateway checks the
real remaining group0/7 heads/tails and every bucket in groups8..23. Only genuinely
empty lists may cross the contiguous source group10/8 calls. Occupied—even
inactive—test records refuse without being relabeled empty. Successful source
cursor continuation selects the actual next group18/slot0 head59. It does not
acknowledge all children, render collection,89554, collisions or a battle frame.

Combined headers and eight source-profile/shared-state/transform/model sanitizer
executables passed, including unchanged legacy slot restrictions. Genuine
G-Red/empty-group verification passed208 checks per native/sanitizer mode across
five scenes, initializing seven children and preparing fourteen roots (including
an interrupted sibling pair). It covers actual slot3/render68/selector13, parent
and scene moves/retirement, replay, unsupported cleanup, no allocation/RNG/cache
or earlier-child changes, and an actually allocated group16 test node. That
occupied inactive node refused the empty-group gateway; only releasing the
diagnostic's own unregistered node allowed it to continue. Three successful empty
continuations reached group18/slot0 head59 without executing callbacks.

The genuine VM mode is0, so both initial144 draw scales clamp to zero. Their
callback bases remain noncollapsed; neither child world is published.
Independent DOL/model-bank comparison matched records9/10, both normal joints,
all eight normal/styled material states, actual descriptor lineage, source
order80→79→59→58 and40 numeric components with zero error. Transform inputs are
the actual owned parent world1 bones, not independently reconstructed parent
worlds or original-hardware equivalence. Receipts:
`initial-child-updates/{gred-verify-v1,gred-comparison-v1}`.
The retained G-Red checkpoint passed source confirmation and132/73/70/196/329
native/sanitizer regression checks with unchanged component output. Those
immutable snapshots predate the following light/shape extension. No new renderable world or
new visible actor capture is claimed; the approved stage-only contact-sheet
baseline remains separate.

Next lead: shape59 uses1C42C/1C468, registers1C5AC and eventually draws through
1C70C/52214's shared graphic path. Its constructor-direction read depends on
the actual80436208 light alias.08A94 selects the last infinite light after the
first loaded light, not an arbitrary convenient light;40E94/2442DC returns the
normalized interest-minus-position vector with its original fallback. Reuse
the existing stage light/archive owner and preserve that alias/lifetime before
joining this branch. Do not assume the global is null or substitute a viewer
direction. Stage/global light construction and any intervening updates must be
qualified at this exact boundary.

## Initial light-constructor alias and shape59

The stage owner now retains default light-constructor inputs separately from
the generic archive lighting vector.08A94/245F20 creates fresh chains for each
root load, including repeated descriptors. Linking a later root overwrites only
the prior head's next link; any now-unreached constructor nodes remain retained.
The primary infinite alias is selected only from later returned root heads,
not root0 or a convenient descendant in a flattened list. No light animation or
SDK cleanup is acknowledged.

The first pending070B shape at59 is source type0/bone19, not the type1 branch
that overwrites direction from parent facing. Its native entry requires the
actual group25 head and source sibling58, unchanged model hash, owned initial
stage2/main2 light constructors and the exact source profile. It reads the
constructor-owned interest-minus-position direction through the shared
normalization helper, preserving the original zero-vector fallback and refusing
unsupported nonzero underflow instead of silently repairing it. It appends
1C5AC under selector10 (scheduler index6), sets phase/render1 and144=1, then
retains1C524's fresh parent/model branch.

The first genuine preflight correctly refused a profile accidentally bound to
main payload445. The selected stage2/main2 main payload is473 (model hash
BDF8E542), so the profile was corrected rather than relaxing identity checks.
The failed profiles/diagnostics are retained in ignored evidence. The source
light positions happen to match across those variants; that is not permission
to substitute one archive owner for another.

Owned constructor tests cover caller relinking, retained unreachable chains,
fresh repeated-descriptor instances, root-only alias selection, unrequested
animation, malformed/custom/unsupported lights and WObj ranges. Shape-profile
tests cover the original fallback and unsupported tiny nonzero normalization.
Corrected genuine native/sanitizer verification passed40 checks per mode across
four scenes, initializing two shapes, preserving source pools/RNG/earlier
callbacks and covering replay, stale parents, source order and owner moves.
Independent selected-main2/DOL comparison matched two constructor nodes, both
root heads, exact primary archive descriptor0x77658, bone19/type0, source59→58 order and16
numeric components with zero error.

Receipts: ignored `initial-child-updates/{shape-inputs-v2-main2,shape-verify-v3-main2,shape-comparison-v1}`.
Corrected-profile combined headers and all six light/shape/shared-state
sanitizer executables passed. Earlier component regressions passed208/132/196/329
checks per native/sanitizer mode with unchanged output at the preflight
checkpoint. Their only later engine differences are the corrected shape-profile
identity and error text; current shape and GPU sources are separately
snapshot/hash-verified. Final receipts explicitly retain that version boundary
rather than claiming all snapshots are byte-identical current source.
Sheet03's four fresh native GL captures matched every RGB pixel against the
user-approved stage baseline and were opened in Review. Actors/children remain
absent from those stage-only images.
Shape59's world callback and graphic draw remain unexecuted; no new actor/child
image or battle frame is claimed.

## Following type0 shape constructors

The continuation now uses a sealed four-parent model/hash/bone table with the
same initial light recipe. All imported source rows are type0, argument0,
unit B4 and bones16/19. It reuses the first shape's constructor math and native
parent checks; the first-shape gateway retains its exact entry restriction.
No type1 parent-facing branch or arbitrary parent slot is admitted.

The actual cursor follows58→65→64→71→70, with parent slots0/1/1/2/2. Callback
lists preserve their real existing tails: selector10 is59→58; selector11 is
67→66→65→64, retaining Sasuke's bank children before its shapes; selector12 is
71→70. The continuation stops at the actual slot3/group28 common-auxiliary
node78 (C7664), rather than skipping it to G-Red's later shapes76/75.

Only constructor/runtime fields and typed callback links are written. Models,
physical allocation, source group links, parent program/world, stage-light
generation and RNG remain untouched. Parent/scene/child/light lifetime gates
remain distinct, and later callbacks cannot bypass pending source consumers.
Combined headers and five source-profile/light/shared-lighting sanitizer
executables passed. Genuine native/sanitizer continuation verification passed134
checks per mode across five scenes, including twelve additional shape
initializations, two complete five-shape continuations and interruption between
Sasuke's siblings. Bad recipes, early dispatch, stale frame parents, source-tail
preservation, moves/retirement and no allocation/clock/RNG changes are covered.
Independent DOL/model-bank comparison rehashed all four parent banks and matched
all five source rows, the retained mixed callback chains and20 numeric components
with zero error. It also matched the pending C7664 node78 without executing it.
Receipts: ignored
`initial-child-updates/{shape-continuation-verify-v1,shape-continuation-comparison-v1}`.
The retained continuation checkpoint also passed40/208/196/329 native/sanitizer
regression checks with unchanged output and matching immutable source hashes.
There is no new visible capture yet:
89554 shape worlds and52214 graphic draws are still unexecuted.

Next C7664 lead: source C76A0 initializes exactly two inline matrix slots
(`count*64==128<129`), rather than allocating a scratch block. Its source54
relationship must be native typed inline storage, never a host pointer encoded
as a guest word. It binds two common-effect record51 roots with06FB4/default
material context, not the parent's07030 colour-cache hook. Reuse the existing
auxiliary constructor/state math without remapping actual G-Red slot3 to an
isolated index0/1, and do not execute C79F8's world/matrix work as part of the
phase0 initializer. The current effect-resource owner exposes prepared model
bytes but its archive lease is not yet bound: join that source resource/archive
ownership at the actual load boundary, not by inventing a late child-owned
archive or substituting the isolated auxiliary-scene owner.

## Ordered graphics archives and common-auxiliary initialization

Source inspection establishes42954 as the effect-model archive publication;
42878 only queues its bytes/texture callback.42954 follows425B8's cockpit
archive/light-input setup in286D8, after resource drain and before actor dispatch.
The new bounded archive fragment acquires/binds those two actual pool1 headers
in that order. It cannot run retroactively at child78, cannot replay, and does
not acknowledge42300 relocation, the enclosing transition or a battle frame.

Split display uses cockpit member2708 and effect model block2, not the isolated
preview bank. Their exact parsed model hashes are2FBC80C9/6DD5D3B2, with118/157
model records. Selected record51 is root25AC8 with three joints/four meshes,
distinct from the old preview's bank hash, root and geometry counts.

Cockpit08B90 retains four independent constructor-input groups, each loading a
fresh first ambient chain and its selected second root. It neither replaces
the stage primary-light alias nor creates a renderable point-light capability.
The fourth cockpit group includes a point light: its bounded constructor
parameters are retained under an explicit input-only entry, while the stage
constructor and shared renderer retain their point/spot-light refusals.

The common-auxiliary entry requires the earlier same-scene archive fragment and
actual group28 order78→77 (types41→40). It reuses the source initializer's
constant field updates, not the isolated auxiliary owner or its0/1 index limit.
Actual slot3 registers selector13 in the retained G-Red callback list. Each child
binds two normal/default-context record51 roots through the effect archive;
there is no07030 actor-cache or alternate-style substitution.

The count is exactly2, so128 inline bytes fit the source `<129` branch. Field54
is represented by a typed self/generation/field144 reference, never a host
pointer packed into a guest word. Fresh matrix storage is zeroed and marked
invalid, not presented as identity worlds. No scratch allocation, immediate
C77EC update or C79F8 matrix callback is performed. Source cursor continuation
stops at G-Red shape76, with bar/HUD/world/graphics consumers still pending.

Combined headers and seven archive/auxiliary/light/shared-resource sanitizer
executables passed. Genuine verification passed326 checks per native/sanitizer
mode across three archive and four child scenarios. It checks ordered headers,
real zero/one-free-header exhaustion with consumed prefixes retained, bad
profiles, no retroactive initialization/replay, stage-alias isolation, actual
slot3 inline storage, moves/retirement and no late allocation/RNG/parent-clock
changes. Four auxiliary children initialized across two successful scenes,
preparing eight roots without executing world callbacks.

Independent DOL/selected-bank comparison matched archive order425B8→42954
(pool1 indices9→10), the four distinct cockpit light-input groups/eight nodes,
record51's three joints/four meshes, all twelve prepared joint states and
original model-descriptor lineage353/354 then351/352. It matched child order
78→77→76 and the inline128-byte branch. Receipts:
`initial-child-updates/{common-aux-inputs-v1,common-aux-verify-v1,common-aux-comparison-v1}`.
Current-source component and native-pixel regressions are in progress.
Sheet04 has since captured four fresh native GL views with this ordered archive
fragment actually enabled before actor dispatch/presentation. Every RGB pixel
matched the user-approved stage baseline, and the sheet was opened in Review.
The stage primary-light alias remained separate from the four cockpit input
groups. Actors/children remain absent from those component images.
The retained archive/auxiliary checkpoint also passed134/196/329
native/sanitizer regression checks with unchanged component output. The following
shape/bar work is tested separately.

## G-Red shapes and first bar scratch ownership

A separate post-auxiliary shape entry reuses the same source shape implementation
for actual nodes76→75; the older continuation still stops at common-auxiliary78.
The retained selector13 chain is80→79→78→77→76→75. The following node is the real
bar74, not a selected subset or an inferred empty tail.

06DC8 consumes the existing pool3 records, not a parallel scratch allocator.
The bar requests5×36=180 bytes, rounded to192. Native storage is32-byte aligned,
opaque/uninitialized and owned by a generation-checked weak lease. Pool2's
header hooks remain unsupported. Individual pool3 release preserves the source
retained0C marker, while no stale lease or copied snapshot can keep the buffer
current. Allocation/free history also invalidates bootstrap freshness even when
the free count is restored.

Actual heap failure writes null0C and returns the consumed pool3 descriptor via
the source free-list path; pool exhaustion does not call the heap. Neither
outcome is supplied by a caller flag. The bar follows source phase2/render0 on
those known failures rather than failing the whole scene. Successful allocation
stores typed54/DC references, initializes phase/render1, counter/mask1C/1E=0 and
selector89=3 from model615's high byte06. It appends13C144 after the real class3
stage-side callback1 tail, retaining a mixed-class chain.

No scratch payload contents or identity/world matrices are fabricated. The
registered zero-mask bar's eventual world/draw callbacks must still be dispatched
in source order. Consumer-bound scratch cannot be released without its cleanup
consumer, and retirement during a heap callback cannot publish a late bar.
After only genuine empty unparented24/31 lists, the source cursor reaches
post-class group38 head82; HUD/world/graphics consumers remain pending.

Combined headers and seven scratch/profile/pool/transform sanitizer executables
passed. Genuine shape/bar verification passed615 checks per native/sanitizer mode
across six scenes. It covers both G-Red shape constructors, successful scratch,
actual pool3 exhaustion, a real null-returning heap service, stale parents,
retirement during a heap callback, source mixed-class callback order, replay and
an occupied unparented31 test node. Four bar callbacks completed, two allocated
scratch, and all four reached the actual post-class head82 through empty-tail
guards. Known allocation failures retain source phase2/render0 without a false
whole-scene failure.

Independent DOL/source-list comparison matched requested180/rounded192 bytes,
pool3 descriptor0/generation1, zero initialized-payload bytes, source high-byte
selector3 and the full callback1 chain(class3 indices0→1→2→3→class2 bar74).
The first1E mask remains0; source world/draw callbacks have not been executed.
Receipts: ignored `initial-child-updates/{bar-inputs-v1,bar-verify-v1,bar-comparison-v1}`.
Current-source regressions passed326/196/329 checks per native/sanitizer mode;
the finalizer confirmed current C++ matches all four immutable snapshots and
earlier component output is unchanged. There is no newly rendered
actor/child or battle frame yet; Sheet04 remains the last visual checkpoint.

## Fresh cockpit HUD child initializers

The source HUD order is the actual group38 chain82→81→73→72→69→68→63→62.
The one-descriptor children use81478/818CC and cockpit record56, including
086B8's null-material-table branch: record56 has no animation attachment or
time0 request.07C30 is AnimAll, not a flag setter; the fresh unconstrained root
has no AObjs and its flags do not trigger a dirty change. The six-descriptor children
use808B4/80F88, initially binding cockpit records81/81/50 in slots2/3/4 with
green/red diffuse changes on the first pair. Do not use side-shadow texture
removal or actor-cache styling as a substitute, and retain genuinely unbound
descriptor slots. Reuse the newly owned cockpit archive at its original scope.

The new initializer entry follows this actual occupied chain, preserving group56
kind83=14(hex), the parent constructor and same-domain cockpit2708/hash2FBC80C9
archive. The first full-scene verifier caught an overstrict zero-kind check; the
guard was corrected to the exact source14 rather than removed. The failed
`hud-verify-v1` snapshot is retained separately from its corrected rerun.

The model-bank metadata query now retains all three animation-table pointer
identities as well as clip counts. Owned null and nonnull-empty fixtures prove
those states do not collapse. The shared binding helper admits only a fresh
single unconstrained/default root with null tables and no AnimAll dirty
predicate; nonnull-empty/nonempty tables and dirty branches remain refused.
It does not request a clip, tick a player or publish an identity/world matrix.

Six-descriptor children retain exact144..15F sentinels/clears, yaw74=0 and
independent default record81 roots in slots2/3. The shared material binding
writes only diffuse RGB green/red, retaining alpha, category, texture and geometry
flags. Slot4 receives default record50 without attaching its authored material
animation. Slots0/1/5 remain unbound. All eight initializers leave render82 at
its unchanged fresh0 and append selector21 to actual callback list5; no HUD
callback or draw has run.

Combined headers and seven owned sanitizer executables passed, covering the
new table identity/profile/material helpers and shared animation/pool bindings.
Genuine scene verification passed596 checks per native/sanitizer mode across
five scenes:20 HUD initializers and40 prepared roots, including a partial prefix,
stale-parent refusal, moved owners, retirement, an unrelated prepended group38
head and a genuinely occupied suffix. No model/archive/scratch allocation, RNG,
parent program/world advancement or unrelated object/callback-list mutation
occurred. The corrected `hud-verify-v2-source-kind` snapshot supersedes only the
failed HUD attempt, not the historical bar regression checkpoints.

Independent DOL/store/asset comparison passed. It matches both phase0 dispatch
tables, source group56 kind14, exact initializer stores/call arguments, all16
baseline cockpit roots,12 genuinely unbound slots, null animation tables,
record50's present-but-unattached table and diffuse-only green/red RGB stores.
Receipts: ignored `initial-child-updates/{hud-inputs-v1,hud-verify-v2-source-kind,hud-comparison-v1}`.
The earlier bar/presentation/side regressions passed615/196/329 checks per mode;
the HUD finalizer confirmed all four tested C++ snapshots still matched their
current files before the subsequent briefing work.
World dispatch remains refused:
source group3A→39 tails, occupied callback lists, graphics collection and the
enclosing transition still need their own joins. Sheet04 remains the latest
visual checkpoint; these changes do not yet draw a HUD, actor or battle frame.

The retained source audit has an empty group3A but **58 occupied group39 widgets**
at indices0..57, all using199C0C. The existing shared preparation-widget owner
still marks its models/callbacks pending. Completing the eight HUD nodes cannot
skip those widgets, decrement counter1653, or acknowledge the whole29434/89554
boundary. Reuse the shared preparation/model/animation systems for that next
join; do not create a separate battle-widget renderer.

## First preparation-widget callback: pre-input fragment

The source group3A guard now refuses any occupied head/tail, including an inactive
diagnostic node, before reaching actual group39. Its first node is the original
type4/argument3 widget0, not a new actor child. Scene45 remains1, so199C0C selects
fresh phase0→199C74→19B030 rather than the scene>1 phase2 branch.

The fragment reuses the existing resource-first briefing owner
(hash22E678B4,27 records) and shared normal-root attachment. Its original three
descriptors bind records18/16/17 independently. Records16/17 have real material
tables with one texture AObj each and eight channels2..9; they must not use
HUD82's null-table shortcut. A narrow same-descriptor/root/archive owner retains
the existing shared record player and genuine time0 texture callbacks. Copied
poses or surviving immutable bytes cannot keep retired archive/root ownership
current. Root18 remains unrequested; roots16/17 carry actual request markers.
Authored mesh metadata and root flags are not replaced with a sampled static
model, and no world/identity matrix is published.

19B030 immediately calls19B094. Before its controller branch the source timer
is179(from3×60−1), pitch/yaw72/74 areA0/80, and time44 is the source f32
0.1 after both material requests at the OLD time0. The fragment stops here:
no controller word is supplied as an implicit zero, counter1653 stays58, the
source cursor stays on widget0, and the other57 widgets remain unexecuted.
803C7204 is the controller-word **array base**, not an observed input value:
with the genuine human mask0A the first selected port would read803C7224.
Neither read has run.

Combined headers and seven owned sanitizer executables passed, including515
source-profile/arithmetic checks, retained animation ownership, stale source
refusals and shared texture/HUD/model/pool regressions. The real briefing bank's
existing shared player probe independently admitted both eight-track texture
trees and produced the actual initial UV state with signed-zero offsets.
Genuine scene verification passed273 checks per native/sanitizer mode across
six scenes:three pre-input fragments, nine actual bound roots and six material
requests. Coverage includes stale actors/archive owners, an occupied group3A,
an unrelated group39 head, exact root/descriptor lineage, actual texture-player
ownership, replay refusals, moved scenes and retirement. Only widget0 and its
three original descriptors change; no allocation, source RNG, callback-list or
actor program/world mutation occurs.

Independent source comparison passed for both dispatch tables, group57 kind15,
records18/16/17, all16 time0 texture-track callback values, timer/angle/time stores,
counter58 and the unchanged cursor. It decodes initial packed track values
separately from the native shared player. The historical verifier field
`first_controller_word` denotes the array base; the comparison explicitly
distinguishes that base from the first port selected by mask0A.
Receipts: ignored `initial-child-updates/{briefing-inputs-v1,briefing-verify-v1,briefing-comparison-v1}`.
HUD/presentation/side regressions passed596/196/329 checks per mode. The briefing
finalizer confirmed the tested C++ files and earlier component outputs were
unchanged before the subsequent input join.
World dispatch and drawing remain unsupported; Sheet04 is still the latest
visual checkpoint, not a capture of these new widget roots.

The next input join must preserve MAIN's pressed-word producer8B650:
held803C71FC, previous+4, pressed+8=(held&~previous), released+C and a SEPARATE
repeat word+10.19B094 masks the pressed word with1F70, so a menu repeat sample or
an already-held confirm button cannot substitute for a new press. The existing
portable input router and digital-pad mapping can feed that bounded digital
subset, with explicit prior/current history; analogue calibration, repeat-word
production and whole-input/hardware equivalence remain separate.

## First preparation-widget input completion

The MAIN digital-input owner now reuses the existing input router and digital-pad
mapping for only mask1F70. It requires an explicit previous snapshot and the next
explicit current sample; it never seeds history to an implicit released pad.
Unreported device fallbacks remain unknown, not known neutral input. Both
snapshots must cover each requested logical port before its pressed word can
authorize the source branch. Native alias/touch routing remains the existing
router's policy, not a new battle input backend.

Each sample has a weak owner/serial receipt. The scene pins the exact sample at
the pending widget0 boundary without consuming a controller word. Equal values
or serials in another owner cannot substitute for it. Advancing or retiring a
pinned producer prevents late completion/rebinding; copied state is inspection
data, not input authority. Once the source stores have completed, a later input
sample does not retroactively invalidate those stores or the retained widget
roots. It does not authorize later callbacks to read a different input frame;
the pinned sample cannot be rebound mid-dispatch.

The new completion entry ORs only the genuine human ports' pressed words, masks
1F70, and follows19B094's one-shot branch: a new selected press changes19 from0
to1 and decrements the actual counter1653 once. No press, an already-held button,
or a new press on an unselected port does not decrement it. Multiple buttons or
ports still decrement only once. Only widget0 returns; the child-update count
becomes26 and the cursor reaches the actual type0 widget1. The other57 widgets,
world dispatch, graphics callbacks and the enclosing transition remain pending.

Combined headers and six owned sanitizer executables passed. The new producer
suite has609 checks and the first-dispatch arithmetic suite has772, including
all256 supported digital-bit combinations. Existing input-map/router, retained
material-animation and HUD-model suites passed too. Genuine scene verification
passed248 checks per native/sanitizer mode across13 controlled native scenes.
Eight first-widget dispatches completed; four decremented the counter. The five
refused scenarios cover missing prior-port history, a superseded pinned sample,
a retired producer, a stale actor and a retired archive. Equal-value foreign
samples, bad profiles, absent receipts, replay and late rebinding are refused
without source object/model/counter effects. Texture callbacks are not repeated
or ticked by completing the input branch.

Independent source comparison passed: it matches8B650's actual held/previous
loads, `andc` and pressed+8/released+C stores, the separate repeat word,
19B094's1F70 mask and counter/latch stores, and32 native port-state rows. Actual
human mask0A selects only ports1/3; both simultaneous port/button presses still
produce one decrement. The retained source cursor reaches widget1 with26 known
updates, not a completed group39/frame.
Receipts: ignored `initial-child-updates/{main-inputs-v1,briefing-input-verify-v1,briefing-input-comparison-v1}`.
Earlier briefing/HUD/presentation/side component regressions passed273/596/196/329
checks per mode. The input finalizer confirmed every previously tested C++ file
still matched its immutable snapshot and the component outputs were unchanged
before the subsequent type0 work.
The input samples in these tests are controlled native snapshots, not observed
physical-controller polling or a claim that actor movement/combat works.
No new actor/widget draw or battle frame exists; Sheet04 remains current.

## Type0 preparation-widget initializer

The next entry now initializes actual type0 widget1 through199C74→199DC0.
Its two original descriptors3/4 bind default briefing records2/3 from the same
resource-first archive. The asset audit corrected an overstrict single-joint
assumption BEFORE output publication: record2 has rootE88/flags20080000 and
childEC8/flags80000, each with its own DObj/material. Record3 is the separate
root1208/flags10040000. Both records have null animation tables.

07CAC delegates through091E4 to2437C8's direct float-alpha store. The shared
default-material helper preserves the full hierarchy and sets alpha0 on BOTH
descriptor0 materials, leaving descriptor1's authored alpha1 intact. It does not
change diffuse RGBA bytes, texture/category/blend/cull/geometry flags, joint
flags, source contexts or animation/world state. Cached actor material owners,
styled roots, requested/evaluated roots and unsupported scalar inputs remain
refused.

The exact13 fields144..174 retain the source zero/−75/−301/one values.
Timer1E stays10, rate3C is source f32 0.2 and rate40 is−22.5, with the source's
separately rounded product/subtraction/divisions. Phase18=1 and render82=11(hex)
do not imply a drawn widget. The phase1 body199EA8, its timer updates, sound and
counter decrement remain unexecuted. This initializer does not read or rebind
input, tick widget0's texture players, consume RNG or register a world callback.
It advances only the actual cursor to type1 widget2, leaving56 widgets pending.

Combined headers and six owned sanitizer executables passed, including261
source-profile/field/rate checks and147 default-alpha checks across complete
owned hierarchies, independent instances, signed-zero/scalar refusals, stale
descriptors and contextual-owner refusals. Genuine scene verification passed125
checks per native/sanitizer mode across six scenes. Three type0 initializers
completed, binding six actual roots and changing six descriptor0 materials.
Stale actors/archives and an unrelated prepended group39 head refuse without
source root/descriptor effects. Successful initialization retains the completed
input branch, either counter57/58, widget0's texture-player state, all actor
program/world fields, source RNG, callback lists and source pool occupancy.

Independent DOL/store/asset comparison passed. A bounded straight-line audit of
199DC0's actual stores, conversion stack and float operators matches all13
field words, timer10 and both rate bit patterns. All three actual joint and
material rows match the original records, with alpha0 on both descriptor0
materials and alpha1 on descriptor1. Phase1 and its counter/sound work have not
run. Receipts: ignored `initial-child-updates/{briefing-type0-inputs-v1,briefing-type0-verify-v1,briefing-type0-comparison-v1}`.
Earlier input/briefing/presentation/side component regressions passed248/273/196/329
checks per mode. The type0 finalizer confirmed the previously tested C++ files
still matched their snapshots and component output was unchanged before type1 work.
World/draw callbacks and the battle demo remain unsupported; Sheet04 is still
the latest visual checkpoint.

## Type1 wave-label widget and fresh mode provenance

Actual widget2 now follows the normal Challenge branch of19A108. The source
dispatch table at802CFDC4 uniquely maps **mode1**, not mode2, to195F2C. A separate
96-byte entry-profile V3 opts into that lineage at the existing fresh setup
producer, before any setup callback. V1/V2 keep common32 UNKNOWN. No late mode
setter or upgrade is provided; changing a copied inspection state cannot
authorize this widget. The initial-only AI planner already passes wave0 to
1962C4; its scene46 fact is retained privately in the party candidate and becomes
usable only after the same party/RNG publication commits.

The initializer binds all three original descriptors5/6/7 to records0/1/1.
Record0 keeps its two-joint background hierarchy; the two record1 digit roots
have distinct mutable identities. Leading tens remain **bound but hidden** by
descriptor+10=0, not by deleting a root or changing JObj flags. Only the ones
root attaches the actual material texture tree and requests selector1 followed
by its first AnimAll. Its eight packed FObj tracks are evaluated by the existing
shared player at time1, not replaced with the time0 sample or a static digit.
The old time0-only helper still requests time0.

The shared binding helpers now separately admit bounded integer texture
selectors0..9 and fresh default descriptor visibility. Cache/style/world/request
refusals remain intact. The source target64..6C values, rate40, timer1E=15,
digit-count13=1, flags84=1 and render82=11(hex) are retained. Selector20 appends
the actual node2 to the genuinely empty callback list4 with key47AA4, but that
world callback has NOT run. Counter1653, source RNG, previous widgets and actor
program/world fields do not advance. The cursor reaches actual type2 widget3
with28 known updates and55 widget dispatches still pending.

Combined headers and seven owned sanitizer executables passed, including195
fresh-entry/legacy profile checks,520 first-wave digit-state checks and24
visibility/retained-selector checks. Genuine scene verification passed232 checks
per native/sanitizer mode across eight scenes, with four initialized widgets and
12 actual bound roots. It covers V3 lineage, V2 UNKNOWN-mode refusal, copied
inspection-state refusal, late-upgrade refusal, stale actor/archive owners,
unrelated occupied heads, pending type0 refusal, move/retirement and independence
from the already-consumed input producer.

Independent source comparison matched the mode table, scene46 producer store,
actual four joint rows, descriptor visibility/request states, target/rate/timer,
list selection and all eight time1 texture-track results against a separate
packed FObj evaluator. Receipts: ignored
`initial-child-updates/{briefing-type1-inputs-v1,briefing-type1-verify-v1,briefing-type1-comparison-v1}`.
Earlier type0/input/presentation/side regressions passed125/248/196/329 checks per
mode; the type1 finalizer confirmed the tested source snapshots and unchanged
component output before type2 work. Type2/name/card
widgets, typed world dispatch and drawing remain unjoined at this checkpoint; no new battle frame
or visual capture exists, so Sheet04 remains current.

## Complete initial type2 widget group

The next entry now visits ALL18 actual source type2 objects, indices3..20, in
occupied-list order. The normal first-party recipe keeps mode1/wave0, active0F,
human0A and the owned team member lists. Slot96 is the original argument modulo6;
the actual mutable argument12 becomes its quotient/variant.19BF18's team-list
lookup retains its original not-found→0 fallback. The target rows come from the
complete18-vector source table, including unused ordinal rows; team flips retain
negative zero in field44 and source-rounded rates.

Twelve active nodes append47AA4 to actual selector20/list4, retaining the
already-registered type1 node2. Six inactive nodes run their phase2 branch,
decrement the actual counter once each, register no callback and consume no name
RNG. Crucially,199C74 has already bound default record0 in descriptor2 for EVERY
node, including those inactive nodes. Unused descriptors remain genuinely
unbound. Active names bind independent record14 roots in slots0/1; active badges
and team marks bind the actual selected records5/7/15/22/23.

Source05630 is NOT the party byte generator. It reads/writes the independent
halfword at804360C4, truncates the product through the actual halfword/byte
sequence, and returns the new full16-bit value. The shared recurrence is reused,
but the native owner and receipt type are separate from055FC's C6/C7 owner.
An explicit current halfword stream must be bound at the fresh V3 setup entry
BEFORE setup callbacks. No seed is inferred from the byte state, clock or widget
number, and no late binding/reset is allowed. Equal state in a different owner
does not authorize a draw; legitimate intervening draws in the same stream are
preserved.

Each active variant0 name consumes its two actual halfword calls at the source
sites, masks each return with7 and requests that selector through the shared
texture player.07CD0's following white/black RGB write changes the actual default
MObj constants AND the retained player's constant base colour. It does not
replace the selected UV sample, tick/replay callbacks, alter alpha or mutate
another instance. The constant base survives later texture ticks/re-requests and
is cleared by a new binding; material/joint-animation and cached/styled/world
owners remain refused by this narrow helper.

Combined headers and seven owned sanitizer executables passed. The halfword
suite has65,614 checks, exhaustively comparing all65,536 inputs to an independent
reproduction of the actual PPC instruction sequence. Source type2 arithmetic/
profile coverage passed1,047 checks, and post-selector RGB/base ownership passed24.
Genuine scene verification passed1,470 checks per native/sanitizer mode across
eight scenes: three full18-node groups plus a valid one-node continuation after
a refused foreign stream. They bind105 actual roots overall. Each full group
binds34 roots, runs12 active/six inactive updates and consumes exactly eight
halfword calls, while leaving the separate byte generator unchanged.

The source cursor now reaches actual type5/widget21 with46 known child updates
and37 widget dispatches remaining. No world callback, card initializer or draw
has run. The separate actual bidirectional-list audit passed326 checks per
native/sanitizer mode, walking every forward/back link after every one of the18
updates. It matched node2→3→4→5→6→9→10→11→12→15→16→17→18, with no skipped
inactive update or hidden callback suffix.

Independent source comparison passed for all18 state rows,34 actual bound roots,
52 joint rows, eight full halfword returns,64 packed-track callback values and
the selected UV states. It matched the target table, record selection, source
signed-zero/rate operations, counter52 baseline and the actual callback order.
Receipts: ignored `initial-child-updates/{briefing-type2-inputs-v1,briefing-type2-verify-v1,briefing-type2-links-v1,briefing-type2-comparison-v1}`.
Earlier component regressions passed232/248/196/329 checks per mode. The type2
finalizer confirmed both primary/link snapshots and unchanged component output
before card work. Sheet04 remains the last visual
checkpoint, not a capture of these new roots, and the battle demo remains unready.

## Force cards, final widget and initial-update boundary

All36 actual type5 widgets, indices21..56, now follow19B358 in source order.
The source argument decodes slot96=argument/6. Active slots replace argument12
with its page remainder; inactive slots4/5 retain the original raw argument24..35
and the freshly clear team byte. Twenty-four active pages register47AA4, including
empty pages. Twelve inactive pages decrement counter1653, register no callback
and keep all five descriptors unbound. Empty pages are not absent objects.

Each nonempty page binds up to five genuine current force members from the
private published party, using the existing shared `ForceEditorArt::icon` lookup.
The complete206-row mapping is checked against the same catalog and the sealed
source mapping hash, not a selected four-Borg shortcut. All206 lookups independently
match80350D0C's original group/model tables. Icons attach to the actual owned
unitall archive(hash87D2F859,202 records), not the briefing or actor model bank.
Repeated model IDs retain separate mutable root identities; no icon is labelled
as a battle actor and the existing edited-Borg restrictions remain unchanged.

07D68(-1) clears only both PObj cull bits throughout each default hierarchy.
The shared helper does not remove textures, alter material/category/alpha, apply
side/shadow styles, request animation or publish matrices. Source card counts,
page offsets, separately rounded spacing/rates, scale0.4/0.4/1 and mode1 team
targets remain exact. Neither RNG stream is consumed.

The original final type6/widget57 argument0 branch is joined through19B8A4:
counter1653 decrements once, default briefing record21 binds to its original
descriptor, flags84=19/render82=12(hex), timer1E=8, angular rate80=444(hex),
scale0.8/0.8/1 and target0/0/−400 are retained. The argument!=0 random/19BB34
branch remains refused. Source47AA4 is registered but NOT executed.

Only after the actual group57 tail and empty suffix are qualified does the new
end entry mark **all83 initial child updates complete**. An added source suffix
refuses this acknowledgement; it is never silently skipped. This closes the
initial group-update segment only. Counter1653 remains39(or38 after the earlier
skip press), not zero.89554 world dispatch, UI's later89658 dispatch, graphics
collection/presentation and the enclosing29434/286D8 transition remain unjoined.

Combined headers and seven owned sanitizer executables passed, including848
card/final-widget arithmetic/profile checks and18 hierarchical cull-only checks.
Genuine scene verification passed2,160 checks per native/sanitizer mode across
eight scenes: four full card groups and one one-page continuation after malformed
mapping refusal,145 card-node visits,101 actual icon roots and72 empty active
pages. Stale actors/archives, unrelated heads and an occupied suffix are refused
at their exact boundaries. The actual38-node callback chain retains all prior
nodes, active empty pages and final node57.

Independent comparison matched all36 page state rows, all206 source mappings,
the canonical24 actual icon roots/joint/PObj rows,38 callback nodes, final-widget
fields and the counter39 baseline. Both RNG streams are unchanged. Receipts:
ignored `initial-child-updates/{briefing-cards-inputs-v1,briefing-cards-verify-v1,briefing-cards-comparison-v1}`.
Earlier type2/type1/presentation/side regressions passed1470/232/196/329 checks per
mode. The card finalizer confirmed current tested source snapshots and unchanged
component output before the subsequent world-prefix work. At this card checkpoint
no world/draw callback or battle frame had run; Sheet04 remains the last visual
checkpoint.

## First89554 world-callback prefix

The new first-world cursor follows the actual89554 list order
0→1→2→3→6→7→8→9→10→11. UI lists4/5 are not part of this call;89658 traverses
those later. The old stage-only adapter retains its occupied-list refusals.
The typed cursor instead dispatches the actual initialized follower/side/bar
objects, retaining head, predecessor, tail, class, callback and owner checks.

List0's point61 uses47AA4 flags54: copy the actual current070B logical bone1
BASIS, replace translation directly from the child's20/24/28, rotate localY,
then apply58 scale. It does NOT transform a fabricated local64 offset or add the
parent bone's translation. A separate narrow direct-bone entry reuses the shared
matrix helpers; existing copied-local-bone41/identity0F/scaled-direct1E refusals
remain unchanged. Field60 reuses the existing1E direct-world-position kernel.
Both retain exact current parent/field/child ownership and first world generation.

The next four actual class3 side callbacks0→1→2→3 reuse the SAME stage82060
preparation/commit path as the older stage adapter. No parallel stage or render
engine was added. Bar74 then executes its genuine13C144 mask1E==0 no-op:
no object matrix, descriptor matrix or scratch payload is fabricated/written.
The genuinely empty40/41 lists are qualified, then the cursor stops at actual
shape59/1C5AC on list6. It cannot skip that occupied suffix.

These callbacks write ONLY the source object114 matrices/positions. HSD model
roots remain unrequested/unpublished as appropriate, with no root world/render
receipt. In particular the aggregate stage world generation stays0, even after
its four per-object matrices reach generation1. It will not release the collider/
camera boundary until the ENTIRE89554 dispatcher is joined. Partial matrices,
copied matrix values or the completed83 initial updates cannot authorize a later
camera, collision, model collection or enclosing transition.

Combined headers and six owned sanitizer executables passed, including275
direct-bone numeric/refusal checks and322 exact list-order/UI-exclusion profile
checks. Genuine scene verification passed228 checks per native/sanitizer mode
across six scenes, executing24 actual callbacks and21 matrix writes overall.
Each complete supported prefix has seven callbacks and six matrices. It covers
stale/retired/moved parents and scenes, archive retirement, invalid recipes,
unknown occupied suffixes, source bar no-op/scratch preservation, both RNG
streams, unchanged callback lists and unchanged parent actor/program/model roots.

Independent comparison matched all96 matrix components against a separate
float-order evaluator, the original side-placement payload, exact source input
branches and actual callback order. Receipts: ignored
`initial-world-dispatch/{inputs-v1,verify-v1,comparison-v1}`. Earlier card/
presentation/side regressions passed2160/196/329 checks per mode; the finalizer
confirmed unchanged outputs and matching source snapshots before shape-world edits.
At that checkpoint shape projection/world work and the following bank/auxiliary
callbacks remained pending; world dispatch, graphics,
history composition and the battle demo remain incomplete. No new visual
capture is warranted yet; Sheet04 remains current.

## Static type0 shape and interleaved bank worlds

An explicit shape-world opt-in continues the initial89554 cursor through
59→58→67→66→65→64→71→70→80→79. Six type0 shape callbacks and four normal bank
attachments follow the actual occupied lists; the legacy seven-callback entry
still refuses shape59. The new prefix stops at actual auxiliary78/C79F8 on list9,
with17 callbacks and16 object matrices total. It cannot skip that auxiliary to
reach G-Red's later shapes76/75 or acknowledge the dispatcher.

Shape1C5AC reads its actual parent's current logical bone19/16 and the retained
stage light direction. The static3F688 branch requires the reached3CC24 attachment
flag and55DCC cached87 both zero. Its3E0A8 height+normal query reuses shared static
cell/projection/plane/ceiling operations, retaining last-wins ties and the chosen
best/lowest/highest normal. Outside the source bounds it returns−5000, unlike the
older height-only sibling's pointY return. Moving-platform/capsule branches remain
refused.

The mode4 basis uses normal asZ, light hint asX, thenY=Z×X andX=Y×Z, with the
source threshold bits2EDBE6FE. The degenerate retained/unwritten-basis branch
refuses instead of inventing identity. Shared basis/transform helpers build the
matrix with scale(2×base,base,1), where
base=childB4×(size7FC×parentB4). Translation is restored from the projected point.
Visibility144 uses squared distance and range(100×size7FC)×parentB4; childB4
does not enter that fade range. The bank callbacks reuse the existing47AA4
copied-bone/local-offset/rotation kernel, without ticking the parent or animation.

Combined headers and five owned sanitizer executables passed:267 shape-world
profile/arithmetic/refusal checks,22 selected-normal/tie/gate/span/bounds checks,
321 existing static-ground checks,546 attachment checks and322 dispatcher-profile
checks. Genuine native/sanitizer verification passed259 checks per mode across
six scenes, executing31 additional callbacks overall. An independent original
payload473/STIH and numerical comparison matched202 components with zero numeric
error;196 were bit-identical, the remainder differed only in zero sign.
Parent bones are actual owned first-world inputs, not a new independent proof of
the complete actor-world calculation.

Only object-world values and source shape fade fields change. Both RNG owners,
scratch, callback links, parent journals and HSD model bindings remain unchanged.
No HSD model/root world or render receipt is published. Aggregate stage generation
stays0, and the later collider/camera boundary remains withheld. World dispatch,
UI worlds, graphics collection, history composition and the battle demo remain
incomplete. Sheet04 remains the last visual checkpoint.

Receipts are ignored
`initial-world-dispatch/{shape-inputs-v1,shape-verify-v3,shape-comparison-v2}`.
The retained v2 JSON's decimal next-callback address contained a reporting typo;
v3 explicitly checks the actual stopped800C79F8 key and corrects the report.
Both immutable runs have identical numerical traces. Earlier world/card/
presentation/side regressions passed228/2160/196/329 checks per native/sanitizer
mode with unchanged outputs. The shape finalizer confirms the primary's current
C++ snapshot and the regression snapshots, except the separately rerun shape
diagnostic's report correction/stopped-key assertion (not compiled by those
regressions). This finalizes the seventeen-callback checkpoint, not a complete
world or graphics frame.

Next audit lead: C79F8 first tests parent byteA8+argument12 (AA for auxiliary78,
A8 for77). Its inactive branch writes no inline matrices. The shared
`startup_auxiliary_step` combines update and world work, so calling it here would
incorrectly increment the already initialized child's counter. Any continuation
must qualify the actual parent channel producer and dispatch only the world
fragment, retaining the active40 direction/history refusal.

## Initial89554 tail and separate end check

The new explicit tail entry admits only C79F8's zero selected-parent-channel
branch. Actor-owned A8..AD counters now represent the six actual537A8 clears,
instead of retaining only A8. The admitted first phase0 constructor/program/world
path does not run5568C's later channel decrement/reload work. Auxiliary78/type41
reads AA;77/type40 reads A8. A nonzero selected byte still refuses, even for the
otherwise supported non-turn preview recipe.

The source zero branch jumps directly to the epilogue before any inline matrix,
history or position writes. The dispatcher records the callback visit but changes
no auxiliary object fields, never calls the combined update/world preview helper,
and never increments the child's counter. The original inline words remain
positive zero, not identity/current worlds.

After these two callbacks, the tail reuses the same static shape kernel for
G-Red76→75. End-of-list traversal has a separate exhausted state; it does not
itself release aggregate stage worlds. A separate end entry rechecks every actual
occupied source chain/head/tail/key/registration against its ordered visits and
confirms every written object matrix and stage-side matrix. Only then may it
acknowledge89554 and release aggregate stage generation1 without replaying the
four side callbacks. UI lists4/5 remain outside this traversal.

Combined headers and four owned sanitizer executables passed, including1,494
inactive-channel/profile/no-write/refusal checks and341 common-auxiliary storage
checks. The integrated tail verifier passed238 checks per native/sanitizer mode
across nine scenes, including25 tail callback visits and six completed
dispatchers. Each completed dispatcher has21 callbacks and18 object matrices.
The tests cover active-AA refusal, exact parent/archive retirement, scene/parent
moves, partial-end/replay refusal, and fault-injected changes to prior callback
keys, late list10 occupancy and an appended list9 suffix. End checking rejects
all changed chains before releasing aggregate stage generation1.

The independent source/STIH calculation matched all46 additional G-Red shape
components bit-for-bit and all64 untouched auxiliary inline words. It also
matched the complete21-visit source order and retained list endpoints. List4
contains38 briefing callbacks; list5 contains eight HUD callbacks, not an empty
list. Both remain unexecuted. Component regressions passed259/228/2160/196/329
checks per mode for shape, earlier world, cards, presentation and sides. The tail
finalizer confirmed immutable-source integrity and unchanged output. This is a
historical tail checkpoint: the later caller join changes the bootstrap and
entry-owner headers, and is verified separately below rather than attributed
retroactively to the tail snapshot.

Receipts: ignored
`initial-world-dispatch/{tail-inputs-v1,tail-verify-v1,tail-comparison-v1}`.

The next source caller operation after89554 is the commonB0 counter increment
inside29434 at294D0..294D8, followed by8A28C/89AEC/2BB14,36E24,8972C and89658.
The following collision branch reads common66, not common64; the counter's32-bit
wrap/store and its same-entry owner still need a caller join.8972C re-traverses
lists2/3;89658 then runs lists4/5. This next boundary was audited read-only from
the original instruction bytes in ignored
`initial-world-dispatch/post-world-audit-v1`; none of those operations was
executed or implied by the new world-end acknowledgement. The enclosing
transition, UI worlds, graphics
collection/history composition and battle demo remain incomplete. There is no new
visible capture; Sheet04 remains current.

## Post-world caller/collision join

The new same-entry29434 counter entry requires the completed21-callback world
dispatcher, aggregate stage generation1, unconsumed B0=0, unchanged owners and
the distinct fresh common66=0 field. It stores B0=1 in the actual retained entry
AND scene state, increments their native revisions once, and records the next
source callback8A28C. Copied counter/phase observations cannot mint this private
entry identity or resume a changed/retired owner. The numeric seam preserves
addi/stw's low32 bits without signed host overflow; the native first-frame entry
does not accept a caller-supplied late counter seed.

The existing shared collider/contact functions now advance that typed caller
through its actual collider sweep, reset/sphere prefix and contact tail. The
typed world path cannot bypass the B0 store. Legacy isolated stage/contact
entries retain their earlier behaviour; this does not replace their kernels
with a second collision engine. The intended stopping boundary is36E24, before
the occupied UI callbacks, camera activation, spawn updates or rendering.

Combined headers,65,813 caller-profile/wrap/phase sanitizer checks and195
mode-lineage/legacy-refusal checks passed. Genuine integrated verification passed
294 checks per native/sanitizer mode across nine scenes: eight actual counter
fragments, seven collider sweeps and six completed contact passes. Unsupported
common66, stale actors, changed entry counters, unknown actual collider-list
occupants, swapped actor pointers and invalid profiles refuse at their own
boundaries. Scene moves preserve identity; copied observations cannot retain
retired ownership. Completed fragments are not rolled back or replayed.

Independent comparison matched the actual B0=0→1 store and both owner revisions,
the complete earlier source-compared66-collider trace (2,789 independently
checked numeric components),74 registry entries, all16 sphere/plane broadphase
pairs and37 ordered reset owners. The same27 flagged/five frozen source solid
exits and four ordinary actor-tail returns complete the supported initial contact
pass. No general collision response was added. The next source remains36E24;
the occupied UI lists still prevent visibility completion, camera activation,
spawn continuation and drawing.

The earlier tail snapshot does not cover these later header changes. Tail/
legacy-contact regressions passed238/249/134 checks per mode with unchanged
output. The caller finalizer confirms immutable snapshot integrity; subsequent UI
world work changes the actor/bootstrap/caller-phase headers and is not attributed
retroactively to this historical294-check checkpoint.
Caller receipts are ignored
`initial-world-dispatch/{post-world-inputs-v1,post-world-verify-v1,post-world-comparison-v1}`.

The following UI audit found an important distinction: the38 briefing callbacks
use47AA4, but the eight HUD callbacks do not. Four are the genuinely empty818CC
callback; four use80F88's parent7D2 bit0/phase1 target gate. That parent-byte
producer still needs native ownership before admitting an inactive gate.
Receipts: ignored
`initial-world-dispatch/{post-world-inputs-v1,ui-world-audit-v1}`. No UI world or
new renderable frame has been produced.

The next briefing transform helper passed298 native and sanitizer owned checks under shared
attachment/transform systems. It admits only identity-base/local64 flags01/11/19,
consumes scale58 and Z angle74 only when their source bits request them, and does
not widen the existing0F/1E/copied-bone entries. It is numeric only: it executes
no UI callbacks and publishes no model/root world authority.

## Initial visibility and UI worlds under verification

A new explicit entry performs36E24's13 inactive-view26F clears and qualifies
8972C's actual empty lists2/3 before beginning89658. It retains the real4→5
callback head/predecessor/tail links, rather than treating either list as empty.
The older empty-list visibility entry keeps its occupied-list refusal.

List4's38 briefing callbacks reuse the shared attachment/local-transform kernel.
Only actual privately owned initialized widgets may supply local64, conditional
scale58 and type6's actual angle74; rate80 is not substituted or ticked. All24
active card pages run, including18 pages without icon roots. Unregistered inactive
pages and widgets0/1 are not fabricated as callbacks. The object-world writes
carry generation1 and no parent-actor generation; HSD/root worlds stay unpublished.

List5 retains all eight actual HUD children in82→81→73→72→69→68→63→62 order.
The four818CC callbacks are genuinely empty. The four80F88 callbacks read the
actual parent's owned7D2 bit0 and phase18 before admitting the inactive return.
The7D2 field retains537A8's clear on this initial path:29434 calls6AE60 before
the first constructor dispatch, and the admitted constructor/world/contact
fragments do not rerun that target-display producer. An active target gate still
refuses; no target actor, selected model, material animation or matrix is invented.

Traversal exhaustion alone does not complete the entry. A separate end check
revalidates both actual lists, recomputes briefing matrices from their current
owned inputs and requalifies HUD gates. It still stops at53A94. That actor-late
callback and group3B precede0BDA4, so the typed path explicitly refuses premature
camera activation even after UI worlds complete.

Combined headers and three owned sanitizer executables passed, including1,379
UI-profile/HUD-gate/caller-boundary checks,298 shared local-transform checks and
65,813 counter checks. The genuine integrated UI verifier and current caller/HUD
regressions are running. No complete UI callback traversal or source-equivalent
UI matrices are claimed yet. No renderer frame or new contact sheet has been
produced; Sheet04 remains current.

## Visual review contact sheets

User requested contact sheets during continued testing, before the battle demo
is ready, and will point out obvious visual issues. Keep posting labeled actual
native-renderer captures at visible checkpoints; do not substitute generated art,
preview actors or fitted cameras for source viewer selection.

Sheet01 (`contact-sheets/stage-prefix-v1/contact-sheet-01.png`, ignored) contains
four actual SDL/OpenGL captures made through the shared `RenderEngine` and
`AttachmentModelScene` paths. Each is an independent genuine stage-prefix scene
with both stage roots,48 prop records and eight side roots:58 model records and
138 unchanged source pass visits. Source materials, textures and culling were
retained. CameraA uses the owned source row4/viewport7 view3; its lower black
half is the initial retained history. B–D are explicitly diagnostic inspection
poses of the same view3-selected world geometry, not alternative source viewers.
They neither authorize callbacks nor change any object transform or queue.

The captures recorded63 draw submissions in A and92 each in B–D. Seven authored
empty prop records remain empty. Actors/children, source-global frame
coordination and complete presentation are intentionally absent. This is a
visual component checkpoint, not the battle demo or original-hardware
equivalence. The user-facing sheet was opened in Review.
User reviewed Sheet01 and reported that stage rendering looked right with
nothing obviously wrong visually. Treat that as approval of this stage-prefix
baseline only, not actors, child effects, frame composition or equivalence.

## Next

### Spawn/wave recovery lead

User gameplay recollection (not yet source-verified): most maps appear to use
fixed authored Borg spawn locations. Some Story missions may override the first
wave's initial placement—an optional mission remembered as approximately
“Defeat the Hero Borg Unit!” is a candidate. Later spawns reportedly return to
the map's normal fixed locations. Enemy wave order and spawn-location patterns
appear identical on replays, suggesting authored wave-to-spawn selection.

The recovered Challenge path already indexes fixed stage/team XZ coordinates
with a four-entry cursor and advances that cursor. Its four initial team cursors
are seeded by actual RNG draws. That evidence covers the joined Challenge
placement path; Story overrides and subsequent wave/respawn sequencing remain
to be recovered.

When recovering Story/waves/respawns, trace the existing stage spawn tables and
team cursors through wave/member selection, first-wave overrides and cursor
reset/advance rules. Check whether replay-stable ordering is table-driven or
depends on a deterministic RNG stream. This recollection does not yet establish
the exact mission name, override conditions or enemy spawn-selection mechanism.

### Render-scene integration

Continue from the now-complete four-actor constructor pass using its current RNG
and physical slots.
Stage roots, all props, all eight side roots and the first-view source projection
are joined. Next establish actual non-stage render110 collection and8A010's child
lists, then dispatch8A0A8's actor-controlled lists (including their children)
without resetting the existing model queue.
The enclosing29574 order is8A16C reset,53928 actor collection, then89CD0 groups
28/20/00/10/08/18 and89C00 groups3A/38/39. The bounded collection currently
qualifies the actors/stage subset only; do not infer that the other genuinely
occupied groups are empty.288FC registers8A010 between89EB4 and8A0A8.
Join the remaining first-view coordination and2AB70 history capture before
acknowledging composition/common31 advancement. Consume the scoped model-pass
plan through the shared renderer. Preserve
source callback order and frame generations rather than supplying a free camera
or preview actor. Preparation, widget/stage/child callbacks and full scene-pass
coordination also remain unjoined.
Program continuations and collider parameter-data/update consumers retain their
unsupported-feature refusals; no valid battle demo exists yet.

Join the enclosing29434 callback/frame controller in its source actor-before-
stage order, preserving current RNG, descriptor and collision occupancy. Active
battle-camera selection/projection is still separate from initial view matrices.
The first bounded demo should be shown when those actual scene inputs can draw;
full combat/waves/all-Borg support need not be finished first.

Show the first bounded demo as soon as valid stage, camera and normal-actor
frames reach the shared renderer. Prepared resources are not a substitute for
that demonstration; no battle demo has been launched yet.

No original assets or private data are distributed, and no new commits or pushes
are implicit in continuing this implementation.
