# GG4E08 original-code atlas

The [atlas](api-map/gg4e08-code-atlas.json) maps the original executable and
the installer-selected action profiles for **all 206 catalogued Borgs** in
one pass. It is a recovery index, not recovered source or a runtime
implementation. Use it with the [battle engine recovery notes](battle-engine-recovery.md)
and the [native Borg API coverage map](api-map/gg4e08-borg-api-map.json).

## What it recovers

- **Original executable:** 13,303 symbol-bounded functions across both text
  sections. Entry walks found 40,541 direct call sites and 2,692 unresolved
  indirect call/branch sites; code reachable only via unrecognized indirect
  transfers is not counted as visited. Of the bounds, 7,884 exactly join an
  address/size entry marked `matched` in the local matching database. This is
  reported evidence, not a new compile or a statement about the rest of the
  executable.
- **Shared engine:** All 14 previously reviewed battle anchors are located.
  Seven are interior addresses, not distinct function starts. For example,
  the HP reaction tail at `8002F110` is inside `8002E2A8`, and collision
  service `8002C4D0` is inside `8002BB14`. These sites should still be
  traced individually, but should not be counted as separate routines.
- **Borg dispatch:** All 206 resolve an installer and one reaction table;
  they share 119 distinct installers. The installer walks produce 317
  profile references to 170 distinct profiles. The full six action groups
  retain the static controller contexts, explicit refusals, main handlers
  and the available config and program-table identities. The
  [standalone installer run](#standalone-profile-selection) separates the
  observed normal selections from this static superset.
- **Action code:** All 4,142 selector candidate rows (counting profile
  references) now identify a first code entry: 4,140 via a family table and
  2 via a direct main handler. 2,833 rows have a recognized substate entry
  and 1,787 have a reviewed early phase candidate. Neither count means
  those code paths were taken in play.

### Dispatch differences the neutral map missed

| Scope | Observed original dispatch | How the atlas records it |
| --- | --- | --- |
| `gg4e08:000a` in the close-B wrapper at `80070604` | The model comparison selects `802D3BE8` instead of `802D3BD4`. | The actual model selects the subtable; the rest of the roster does not inherit the special case. |
| `gg4e08:0205` | Main handler `80156FFC` uses either `803429C4` or `803429D4`, according to `actor+0x582 == 1`. | Both arms appear for 25 rows; the atlas does not guess the runtime value of the byte. |
| `gg4e08:0d06`, `gg4e08:0d07` | Their X row reaches `801AE7F4` directly, without a family table. | A direct entry, not an absent move or a borrowed sibling route. |

Table lookups stop at the supplied data-object extent. A missing recognized
substate table is not proof that a handler is a leaf: some families use
other dispatch patterns. The original shares executable code between
Borgs; referencing a function does not make it exclusive to that Borg or
establish what action or payload it produces.

## Standalone profile selection

The [installer execution map](api-map/gg4e08-standalone-installers.json)
runs the original installer for all **206** model IDs against isolated
actor records with zero variant/form fields and a declared slot-0
resource pointer. Each execution joined exactly one atlas profile by
its main handler and seven installed action-table pointers: **158**
distinct profiles. **82** Borgs had more than one static candidate.
The actor-data offsets also agreed with the independent static slot-0
record analysis for every model (200 select record 0, five record 1,
and one record 2).

For this inlet, **62** Borgs select a profile with a charge mode,
compared with 78 if all static alternatives are counted. Of the
selected profiles, 44 enable B-charge, 25 enable X-charge, and seven
enable both. In particular, Sasuke selects `p013`, and G-Black's
actual `062A` installer selects `p089` (dual charge). `p088` was a
useful explicit control in the earlier fixture, **not another observed
selection for normal standalone G-Black**.

The report was byte-identical across the original O0, O1 and O2
recomp modules. The CPU fixtures check each module's declared text
chunk hashes against the DOL, and a copy with one changed instruction
byte is refused. No constructor, scene, slot population, combined
form, or full battle was run. The map is not imported as native
gameplay or used to relax package refusals.

To reproduce, compile [`retail_installer_execution_fixture.cpp`](../tests/host/retail_installer_execution_fixture.cpp)
using the same flags and headers as the [phase fixture](#shared-action-gate),
then pass the compiled fixture and the local module to:

```sh
python3 -m tools.analyze.retail_installer_execution \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_CATALOG" "$GF_SYMBOLS" \
  /tmp/opencode/gf-installer-execution-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-standalone-installers.json
```

## Shared action selector

The [action selector map](api-map/gg4e08-action-selector.json) separates
normal lookup (`80069DD0`) from explicit group/choice/sector selection
(`8006A3D0`) and the shared row commit (`8006A104`). These operate on
actor state after an input or script has requested a group; the map
does not yet assign those flags to controller gestures.

- **Mode 1** uses `16 × choice + 4 × sector`, with six possible choices.
  Sector uses a signed angle tested against four per-spec thresholds;
  with no target the sector is zero. **Mode 0** uses `4 × choice`.
  It contains six rows, but the normal selector only chooses 0–4.
  An explicit request can still supply a flat choice of 5.
- When `+0x595` is zero and `+0x694` is positive, normal mode 1
  refuses. Mode 0 still permits choice 3. Mode refusal, row refusal
  (`selector == -1`), and a disabled individual handler are distinct.
- Row commit writes state/substate to `+0x580/+0x581`, always attempts
  the first handler slot, and attempts the second only if the row's
  lane byte is exactly `3`. A lane mask alone does not tell you which
  handlers will execute later.
- Tick dispatch (`80055568`) runs the installed pre-callback, snapshots
  both slots, runs the second then the saved first, and finally calls
  the installed post-callback. Both 64-entry handler tables are now
  mapped. First slot `61` relays to the installed Borg main callback
  at `+0x4B4`; second slot `0` is a no-op.

Across all 206 Borgs and 317 profile references, the tables contain
**14,880 raw cells**. The normal flat lookup cannot select **364**
of them, including 360 previously indexed selector candidates. The
published exception list identifies the exact model, profile, group,
choice and sector. This rules out a normal lookup path, not access
from a continuation or other explicit request.

The zero-filled rows are data, not synonyms for explicit refusal.
In the original recomp module, normal lookup on `0205`'s B-shot
`3:0` and an explicit request on `0207`'s flat B-close `5:0` both
committed state `0:0` and scheduled first handler `1`, leaving the
second handler untouched. In the dispatch table, first slot `1` is
the common handler that requests or resets a program, not the
slot-61 Borg callback. Neither case proves a strike or a live controller
trigger. The portable moves and existing preview refusals remain unchanged.

To reproduce the map, use the same DOL, catalog and symbol inputs as
the atlas, with `python3 -m tools.analyze.retail_action_selector`.
The CPU-only test uses [`retail_action_selector_fixture.cpp`](../tests/host/retail_action_selector_fixture.cpp)
in the same compile command as the [phase fixture](#shared-action-gate).
It covers normal and explicit lookup, guards, signed sector bands,
commit vs. refusal, the two rows from the unmodified DOL, and the
slot-61 relay: 36 checks passed on each of the unmodified O0, O1,
and O2 recomp modules. It does not run the whole live tick dispatcher.

### Processed input and charge operands

The [input map](api-map/gg4e08-action-input.json) joins the shared
processed-pad consumer (`80056900`), conditioner (`800685C0`) and
action arbiter (`800699D8`). In a scoped human-source visit, the
consumer derives pressed and released edges from the current and previous
held masks. A source-switch visit explicitly clears those edges.
Fresh B (`0x200` in the processed packet) generates two candidate
bits; it does not by itself choose ranged or close-B. Fresh X
(`0x400`) supplies its own bit. Holding either button does not recreate
the ordinary fresh-press request.

The original arbiter stops at the first nonzero probe: linked action,
charged X, charged B, ordinary X, close B, then ordinary B. The normal
X or close-B branch can refuse and let the later B branch try. The
linked-actor probe can consume priority without installing a row; that
live path was not tested. The conditioner can also produce requests
that are not acted on by this arbiter. None of this determines when
other handlers run before or instead of the arbiter.

The charge updater reads held buttons and produces separate release
requests when the prior accumulator has reached the actor-spec limit.
The map resolves the standalone slot-0 record for **all 206 Borgs**.
Across 317 profile references, **78 Borgs** have at least one enabled
charge mode: 59 have B-charge, 30 have X-charge, and 11 have both
somewhere in their installer profiles. Group 4 has no enabled profile
in this atlas. The three recorded floats per Borg are source limits,
not measured frames or guarantees that a particular profile is active.

The [bounded original-input fixture](../tests/host/retail_action_input_fixture.cpp)
feeds processed packets into the unmodified functions with declared
actor/scene state and six separate slot records. It reaches Machine
Red's ranged-B and X rows, Sasuke's close-B row, and both G-Black charge
profiles when supplied explicitly. With the same sufficiently long B+X hold and release, profile
`p088` commits charged B (`group 3`, state `3`); `p089` generates both
requests and commits charged X (`group 5`, state `4`). All 12
sequences passed on the original O0, O1 and O2 recomp modules. These are
isolated profile comparisons with a supplied clock step and no live
move during the hold. They neither pick a profile at runtime nor prove
cancel timing, actual release payloads, or a hit.

### From the installed row to the first program request

An extension of the input fixture runs the actual installers, carries
each committed row through the installed pre-callback and slot-61 relay,
and stops before the program or a required service executes. These were
the first observed boundaries for the declared actor state:

| Model and action | Boundary |
| --- | --- |
| Machine Red (`0600`), ranged B | Call `800C9634` to `8004BEB8`: program `2:0` at `8030903E`; requested lane mask `15`, intersecting the config seed to `7`. The idle pre-callback zeroed the old `+0x6EE` choice. |
| Sasuke (`000A`), close B | Call `800FEEA4` to `8004BEB8`: program `3:1` at `802BE3B6`; mask `15` intersects to `3`. The selected close handler supplies choice 1; the fixture does not seed the choice. |
| G-Black (`062A`), charged B | Call `8016CC50` to allocator `80088AA0` with operands `(actor, 3, 8, 0, 1)` before a program request. No allocated object is supplied. |
| G-Black (`062A`), charged X | Call `800677F0` to world-query entry `8003E0A8` before a program request. No world-query result is supplied. |

The program addresses are checked by following the original local-bank
tables with the observed arguments. This first-request check stops before
`8004BEB8` runs; it does not claim the engine accepted that mask or began
executing the program. All four boundaries matched across O0, O1 and O2.

### First cues in the original VM

The [program-cue index](api-map/gg4e08-program-cues.json) follows the
observed standalone profiles of all **206** Borgs into **114** nonzero
local or common banks and **3,798** distinct program addresses. It
preserves aliases and records only the first reviewed cue of each kind
in a decoded prefix:

| Cue candidate | Programs | Before the first wait |
| --- | ---: | ---: |
| Positive upper-lane marker (opcode 2) | 969 | 241 |
| Positive direct melee queue (opcode 10) | 765 | 657 |
| Class event, callback not qualified | 378 | 45 |

These are static program choices, not moves proven to select or fire.
One group's choice table is refused without losing its bank's other
groups, and 19 programs stop at an unsupported opcode. A missing cue
past one of those boundaries does not mean the original has none.

The CPU fixture now follows two observed first requests further:

- **Sasuke:** The same installed close-B path runs `3:1` and reaches
  `8008AC80` from `8004C6F0` with queue **2** before any wait. It stops
  before allocation or the actor's queue-byte write. With only lane
  bit `0x02`, a separate original-opcode check advances the cursor
  but leaves the queue byte alone.
- **Machine Red:** The joined ranged-B path runs `2:0`, clears an
  old marker and installs the upper wait at `80309056` before a
  world query. The fixture stops the action there without supplying
  a query result. It then advances only the resident VM on the
  scratch actor with a declared step of `1`. That reaches positive
  marker `8030906A` on VM-only visit **11**; driving the other two
  active lanes alone did not set the marker in 16 visits, even
  though their interpreter state advanced.

Both checks matched across the unmodified O0, O1 and O2 recomp
modules. The 11 updates are not a native release constant: the
separate rig-backed fixture took 16 VM visits from a supplied
request. Neither cue check measures elapsed battle time. The
transaction checks below continue into allocation, not contact.

```sh
python3 -m tools.analyze.retail_program_cues \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_CATALOG" "$GF_SYMBOLS" \
  docs/api-map/gg4e08-standalone-installers.json /tmp/opencode/gf-program-cues.json
```

### Paid release and pair allocation

The [attack transaction receipts](api-map/gg4e08-attack-transactions.json)
continue from the installed actor and VM-produced rifle cue. The
fixture invokes the original release handler with declared stock, a
null target, and isolated pool lists. It stops at the next world query
without supplying that query's result.

| Rifle case | Original stock result | Carrier result |
| --- | --- | --- |
| Cue, two rounds, both pools available | `2 → 1`, clock `0` | Type 2/key 21 published in bucket 9; its descriptor was popped and linked. |
| Cue, last round, carrier pool empty | `1 → 0`, clock `300` | Allocation attempted; no carrier or descriptor pop. |
| Cue, last round, descriptor pool empty | `1 → 0`, clock `300` | Allocation attempted; the offered carrier remains free. |
| Cue, stock empty | No debit, clock `0` | No allocation attempted. |
| Cue absent, two rounds | No debit, clock `0` | No allocation attempted. |

Both last-round failures retain the debit. In separate visits to the
shared refill helper on the same depleted stock state, one inhibited
visit left the clock alone, then 300 supplied unit steps restored the
cap of five and set both count mirrors. This is not a live reload time.

With Sasuke's original hit block loaded, the same close-B input leaves
queue byte `2` in both pool cases. A free class-4 cell is popped and
published to bucket 49 with physical tag `0x1A`. The record references
step 2, attack table index 2, and the actor's matrix slot 7; it
retains the actor/carrier and credit links. An empty pool publishes
no pair even though the queue byte was written. The fixture does not
populate that matrix, run the pair callback, or accept a collision.

The allocator's own code writes the headers, tags, and lists; the
fixture only supplies the free-list capacity. No carrier launch,
shape activation, collision-registry insertion, contact, or retirement
is claimed. All seven cases produced identical reports on O0, O1
and O2. Native behavior is unchanged. To replay, compile
[`retail_action_input_fixture.cpp`](../tests/host/retail_action_input_fixture.cpp)
using the [phase fixture](#shared-action-gate) build flags and run:

```sh
python3 -m tools.analyze.retail_attack_transactions \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-attack-transactions.json
```

### Machine Red: from paid carrier to attack registry

The [rifle bucket receipt](api-map/gg4e08-rifle-bucket.json)
continues the actual Machine Red ranged-B allocation above. The
fixture supplies an ordinary slot-zero owner, rigid matrices, and
the qualified collision block. `80089A94(8)` walks the original
carrier list; it selects the carrier in bucket 9 and calls its
phase-zero initializer. The original binder uses group 0 to allocate
and link descriptor 15 in bucket 49. Neither live list is seeded.

| Fork after debit `2 → 1` | Carrier | Shape and attack registry |
| --- | --- | --- |
| Scene inhibits slot zero | Phase 0; no initialization | No bind; registry stays empty |
| Slot admitted, no free shape cell | Phase 2; timer initialized to 300 | No bind; registry stays empty |
| Shape pool available, carrier attack bit suppressed | Phase 1; late list inserted | Sphere initializes; registry stays empty |
| Shape pool available, no publication inhibit | Phase 1; late list inserted | Same sphere; original code publishes one attack |

With the supplied identity bases, key-7 translation
`(100, 200, 300)`, and null launch target, the initial sphere
center and both endpoints are approximately `(-149.4, 191, 365)`.
Its source descriptor radius is 20; the original transformed
radius is **32.46753**. The carrier velocity is approximately
`(0, 0, 80)`. The carrier and shape runs use the original
math code, not a host-supplied geometry result.

No effect cell or visual payload is available. The original code
registers the carrier in its late list and reaches the audio call,
where the fixture stops. It then invokes the independent class-4
pass `80089AEC` over the same lists without resuming the stopped
stack. No contact consumer, HP write, retirement, or live pose
schedule is qualified. Native behavior is unchanged.

All four cases matched across O0, O1 and O2. Use the same binary
and inputs as the [transaction fixture](#paid-release-and-pair-allocation):

```sh
python3 -m tools.analyze.retail_rifle_bucket \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-rifle-bucket.json
```

### Machine Red: from registered rifle contact to HP

The [rifle contact receipt](api-map/gg4e08-rifle-contact-hp.json)
adds an original Sasuke body allocation to the same paid rifle
fixture. Only Sasuke's body list is narrowed to descriptor 9;
the rigid placement and ordinary actor/Scene state are supplied.
The original class-4 sweep publishes both shapes and the original
pair loop selects the receiver. No registry entry or hit is supplied.

| Receiver control | Accepted contact | Requested damage | Sasuke HP |
| --- | --- | ---: | ---: |
| Centers 256 units apart | No | No writer call | `180 → 180` |
| Coincident, token already in history | No | No writer call | `180 → 180` |
| Coincident, clear history | Yes | 8 | `180 → 172` |
| Coincident, clear history, reaction class 2 masked | Yes | 0 | `180 → 180` |

Both accepted cases set contact flag `0x10`, record token 1, and
link the carrier and credited Machine Red to the same receiver.
The raw attack record at `8030A398` has power 10. The amount call
receives the **rifle carrier**, not Machine Red's actor pointer;
the HP transaction still credits Machine Red and records 8 in
both damage ledgers in the ordinary case. Masking reaction class 2
leaves the curve unset and the ledgers unchanged, but the HP
writer still records previous HP 180. That is not a rejected hit.

All four cases matched across O0, O1 and O2. The fixture executes
the original HP writer and stops on return without resuming
reaction scheduling. It does not qualify live projectile sweep,
retirement, full hit reactions or native move timing. Native
gameplay is unchanged. Use the transaction fixture binary:

```sh
python3 -m tools.analyze.retail_rifle_contact_hp \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-rifle-contact-hp.json
```

### Machine Red: rifle continuation and pool return

The [rifle lifetime receipt](api-map/gg4e08-rifle-lifetime.json)
follows the same original carrier and shape through separate calls
to the carrier bucket pass, late pass, registry reset, and class-4
sweep. The fixture supplies a bounded empty terrain grid and stage
limits. Visit counts refer to that sequence, not live battle frames.

| Fork | Attack removed | Carrier and descriptor returned |
| --- | --- | --- |
| Timer expiry | Visit 300; timer reaches zero, phase `2:2` | Visit 335, after phase 3 begins at 334 |
| Forward stage-bounds exit | Visit 237; timer still 64, phase `2:2` | Visit 272 |
| Declared carrier stop bit `0x02` | Visit 1, phase `2:1` | Visit 4 |
| Shape pool empty during binding | Never published; starts in phase `2:0` | Visit 2 |
| Accepted rifle hit, contact flag `0x10` | Still registered after one following visit | Not tested past that visit |

For the after-hit control, the original carrier advanced from
`z=365` to `z=445` and the timer went from 300 to 299. The accepted
hit flag alone did not shut down flight or registration. This
does not demonstrate another contact or penetration.

On the stopped paths, the class-4 sweep unlinked and recycled
the attack shape before the carrier was recycled. The later
carrier release removed both its gameplay and late-list links
and returned the descriptor. The fixture did not repair the
lists or refund stock.

All five cases matched across O0, O1 and O2. No live terrain or
full Scene scheduling ran, and native gameplay is unchanged.
Use the same fixture binary and inputs:

```sh
python3 -m tools.analyze.retail_rifle_lifetime \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-rifle-lifetime.json
```

### Machine Red: one carrier, two receivers

The [second-receiver receipt](api-map/gg4e08-rifle-next-receiver.json)
continues the verified first rifle hit. In the same geometry, the
first receiver's token 1 blocks a repeat. Clearing only that
history word in a discarded replay reopens the original amount
call; neither history control writes HP.

The fixture then binds a second original Sasuke body-9 shape to
slot 2 on the same opposing team. Its pose is declared before
the next carrier visit. The original passes publish both
receivers, keep the same attack shape and token 1, and move the
carrier from `z=365` to `z=445`. The two receiver histories
remain independent.

| Second receiver | HP after pair pass | Machine Red's damage ledger |
| --- | ---: | ---: |
| 256 units beyond the next position | 180 | 8 |
| At the next position, token 1 already in history | 180 | 8 |
| At the next position, fresh history | 172 | 16 |

The fresh second contact uses the same amount record and costs
no extra stock. The first receiver stays at 172 HP in all three
forks. Before the second amount call the original contact code
places the carrier at `z=423`; the HP writer does not move it.
The earlier `z=445` sample is a flight result, not its accepted
contact position.

All three cases matched across O0, O1 and O2. This is a second
recipient on a subsequent isolated visit, not simultaneous pair
dispatch, live timing, or a rule for other carrier keys. Native
direct shots still retire on a body hit by default. The authored
[continuation option](borg-gameplay-api.md#travelling-shots-that-continue-after-contact)
uses per-recipient history instead of globally ignoring the next
contact. Only the explicit
[Machine Red Workshop extraction test](retail-roster-extraction.md#machine-red-projectile-continuation-test)
selects it for a retail-derived profile.

```sh
python3 -m tools.analyze.retail_rifle_next_receiver \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-rifle-next-receiver.json
```

### Type-2 carriers across the roster

The [type-2 census](api-map/gg4e08-type2-roster.json) checks all
**123** factory rows at `80303138` and scans all **206** profiles
selected by the bounded standalone installers. Of **103** direct
calls to `800C3BE0`, the static action linker associates **58**
calls and **56** keys with **30** Borgs. The remaining **45** calls
stay unjoined. A preceding literal is not treated as proof of
the argument along every incoming branch.

The same carrier dispatch reads **15 distinct speed triplets**.
There are 13 parent-root selectors, distinct from joint zero.
The bearing helper is used even without a target for 64 rows,
only with an available target for 30, and not at all for 29.
These describe the branch after target validation, not a
qualification of the actual target or rendered muzzle.

| Checked static call | Key | Initial / added / cap | Source group | Raw power |
| --- | ---: | --- | ---: | ---: |
| Machine Red ranged B | 21 | `80 / 0.5 / 160` | 0 | 10 |
| G-Red ranged B | 43 | `80 / 0.5 / 160` | 0 | 20 |
| Proto King ranged B | 85 | `160 / 1 / 160` | 9 | 50 |

The census identifies 36 checked normal-record call candidates
with positive raw power, and withholds the queue join for 18
combined-form calls and four alternate-spec calls. It does not
admit a move, infer a single hit amount, or assign Machine Red's
lifetime to another key. The carrier lifetime receipt above is a
separate bounded original execution.

The [speed-order replay](api-map/gg4e08-rifle-speed.json) checked
162 active key-21 updates on a declared empty stage. The first
visit moves 80 units before writing 80.5. Visit 160 moves 159.5
before writing the cap 160, and visits 161–162 move 160. O0, O1
and O2 agree. These are cardinal source visits, not a conversion
to native units or a live battle clock. The native
[speed ramp](borg-gameplay-api.md#travelling-projectile-speed-ramp)
expresses the order without copying these numbers into a package.

```sh
python3 -m tools.analyze.retail_type2_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json \
  docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type2-roster.json
```

The speed-order replay uses the transaction fixture binary:

```sh
python3 -m tools.analyze.retail_rifle_speed \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-rifle-speed.json
```

### Ordinary type-0 carriers across the roster

The [type-0 census](api-map/gg4e08-type0-roster.json) covers
**129** rows and **139** direct calls to ordinary factory `80082824`.
Of those, **58** call sites and **35** keys join to static candidates
on **27** of the 206 standalone profiles. The remaining **81**
calls stay unjoined. Violet Attacker's one-bit selector accounts
for two key candidates (16 and 17) at a single call site; the
index does not choose a repeat-shot key. Four alternate-spec
candidates keep their queue joins withheld.

The review also corrected Death Eye's model gate in the action
linker. Model `0F06` reaches key 122 at `801E4754`. Key 123 at
`801E4764` is the **not-equal** arm, not a second Death Eye
shot. That call remains in the inventory without an inferred owner.
The negative basis selector on key 114 remains unresolved.

Ordinary type-0 flight decrements its timer before the bounds
and contact checks. A nonzero contact byte selects phase 2.
The [bounded Gold Hero control](api-map/gg4e08-type0-flight.json)
calls the original factory directly with key 45 and a rigid pose.
With its shape registered and the byte clear, the next update
moves from `z=350` to `z=370` and keeps the attack registered.
With a supplied `0x10` byte, it stays at `z=350`, returns the shape
on visit 1, and returns the carrier and descriptor on visit 3.
Both forks decrement the timer from 300 to 299 first.

This differs from the [type-2 Machine Red comparison](#machine-red-rifle-continuation-and-pool-return),
where the same bit value from the pair loop did not stop the next
update. The supplied-byte control matched across O0, O1 and O2.
The [joined type-0 contact test below](#type-0-contact-hp-and-retirement)
then checks what three source keys actually produce. The
alternate `8008296C` constructor is still outside scope.

```sh
python3 -m tools.analyze.retail_type0_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json \
  docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type0-roster.json
python3 -m tools.analyze.retail_type0_flight \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type0-flight.json
```

### Type-0 contact, HP and retirement

The [type-0 contact receipt](api-map/gg4e08-type0-contact.json)
uses the ordinary factory and exact hit blocks for Kung-Fu
Master (key 12), Tao Master (125), and Gold Hero (45). The
original allocation and sweep register their attack spheres and
the same Sasuke body-9 singleton. The fixture supplies rigid
poses, unit scales, settings/ranks/banks at zero, and an empty
stage. It supplies the factory key directly, not the original
input, cue or stock debit.

| Control across all three keys | Original contact and HP | Following carrier update |
| --- | --- | --- |
| Centers 256 units apart | No contact; 180 HP | Continued and stayed registered |
| Coincident with token already in history | No new contact; 180 HP | Continued and stayed registered |
| Coincident with clear history | Set `0x10` and recorded token 1; raw power 20 became 17 damage, `180 → 163` HP | Entered phase 2 and returned the attack shape without moving |

The original amount call receives the carrier and the distinct
record for each key. The HP writer credits the real owner; neither
amount nor carrier flag is supplied. Its caller is not resumed.
In the independent continuation, all three accepted shots enter
phase 3 on visit 2 and return the carrier and descriptor on visit
3. This differs from the tested type-2 Machine Red shot, which
stays registered after an accepted `0x10` contact.

All nine cases matched across O0, O1 and O2. This does not
qualify moving-body collision, full attack timing, other keys,
KO, or the live Scene. No native runtime rule changed.

```sh
python3 -m tools.analyze.retail_type0_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type0-contact.json
```

### Worm paid release, refunds and contact follow-up

The [Worm contact receipt](api-map/gg4e08-worm-contact.json)
enters the original paid dispatch `80119848` with two rounds.
The model branch, debit, allocation, shape bind, and Sasuke
body-9 contact all run in the original code. The fixture does not
run the upstream B cue or the live Scene.

| Borg | Carrier | Empty carrier or shape pool | Opening raw power → damage |
| --- | --- | --- | --- |
| Alien Worm | Type 49, key 0 | Refunds the debit | `0 → 0` |
| Venom Worm | Type 94, key 0 | Keeps the debit | `30 → 26` |
| Claw Worm | Type 49, key 1 | Refunds the debit | `0 → 0` |
| Poison Worm | Type 94, key 2 | Keeps the debit | `15 → 13` |

The two type-49 openings set `0x10`, record token 1, and reach the
HP writer, but remove no HP. On the next isolated visit the
carrier retains the struck actor and unlinks the first shape.
On visit 2 the same carrier binds **group 7 / descriptor 10**
and registers token 2. With the same coincident receiver and
its token history intact, the second original contact deals
**1 HP** for each Worm, using raw power 2 for Alien and 1 for
Claw. Neither carrier spends another round. The HP writer
returns without resuming its caller on either contact.

All 16 cases and both follow-ups matched across O0, O1 and O2.
The fixture supplies rigid poses, unit scales, ordinary damage
inputs, an empty stage and no class-2 effect capacity. It does
not qualify full tether movement, later callbacks, poison effects,
or live timing. Native zero-power opening routes remain unbound:
binding the follow-up power to the opening would change the move.

```sh
python3 -m tools.analyze.retail_worm_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-worm-contact.json
```

### From shape publication to the amount call

The [shape sweep receipt](api-map/gg4e08-shape-sweep.json) carries the
allocated Sasuke close-B pair into the original class-4 pass `80089AEC`.
It also lets the original body allocator create one receiver sphere
from Sasuke's body descriptor 9. The receiver's scratch body list is
narrowed to that descriptor, and both rigid poses and the attack cursor
are supplied. The fixture does not seed either registry.

At cursor **18** the pass registers only the receiver. At **19** it
transforms and registers the attack, unless the owner's bit for
matrix slot 7 is clear. The source attack and receiver radii are
**60** and **22**. The original pair loop accepted distances **81**
and **82** (exact tangency) and rejected **83**. At 81, clearing the
attack's matrix mask, setting the receiver's relation inhibit, or
setting receiver `+0x1DA` bit `0x02` each kept the pair from the
narrow phase. The latter two controls were applied after
publication; `+0x1DA` rejects the pair before the relation helper.

For both accepted fixtures, the unmodified pair loop reached
`8002F108 → 8003CD5C` with the attack record `802D4190` and the
expected attack and body actors. The fixture stops before computing
an amount or writing HP. All six cases produced the same report on
O0, O1 and O2. The cursor values are not battle frames, and the
singleton comparison does not qualify full body geometry, Machine
Red's projectile update, guard, or other receiver types.

Use the same binary and qualified inputs as the transaction fixture:

```sh
python3 -m tools.analyze.retail_shape_sweep \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-shape-sweep.json
```

### From registered contact to HP

The [contact-to-HP receipt](api-map/gg4e08-contact-hp.json) extends the
same Sasuke pair through the original amount routine and HP writer.
Both fighters use the exact normal actor spec and level-one HP cap.
Rank, relative scales, force mirrors, controller source and team
settings are declared inputs, not outputs from a running match.
The fixture stops at the HP writer, then executes that same call
with its original arguments, stack and return address.

| Scoped case | Requested amount | Receiver HP | Credit and receive ledgers |
| --- | ---: | ---: | ---: |
| Overlap or exact tangency, settings 0 | 35 | `180 → 145` | 35 |
| Miss or any of the three publication/receiver gates | No call | `180 → 180` | 0 |
| Overlap with scene HP protection | 35 | `180 → 180` | 0 |
| Receiver starts at 1 HP | 34 | `1 → 0` | 34 |
| Settings 7, receiver controller source 0 | 35 | `180 → 145` | 35 |
| Settings 7, receiver controller source 1 | 40 | `180 → 140` | 40 |

The raw attack row says **40**. That is not necessarily the computed
amount or the HP actually removed. The low-HP clamp control
records 34 in both ledgers with only 1 HP removed. Zero HP is the
writer's return condition, not a test of subsequent KO scheduling.

In the last two controls, only receiver `+0x3E6` changes. The receiver
remains on team 1, both team settings stay 7, and the same contact is
accepted. The original uses that controller-source byte to select
the damage table bank (receiver class factor **0.92** versus **1.04**).
A caller can now supply a third group to native `--package-damage-indices`
to select per-actor banks independently of teams. The asset-free native
fixture reproduces **35** and **40** with the same teams, recipient, and
raw power. Omission preserves the older team-keyed selection. This does
not infer live controller source or admit Sasuke's original melee move.

All ten cases produced the same report on O0, O1 and O2. This does not
exercise the rest of the reaction loop, live rank/force generation, or
other attack and receiver families.

```sh
python3 -m tools.analyze.retail_contact_hp \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-paid-release-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-contact-hp.json
```

### Original damage setup writers

The [damage setup receipt](api-map/gg4e08-damage-setup.json) runs the
original settings writer, the ordinary constructor's controller
selection slice, and the rank initializer, using declared Scene,
mode and slot inputs.

- `80006ABC` in mode 1 writes `bias & 31` and `(31 - bias) & 31`
  to the first two cells, leaving the other two alone. Mode 2
  writes four `7`s. The tested modes 0, 3 and 255 do not write.
- `8005438C` tests the Mode input mask against the actor's assigned
  slot bit. An intersection writes bank and input mode 0; otherwise
  the slice writes 1 to both. It does not change the actor's team.
- The rank initialization slice copies `Scene[62 + slot]` to the
  actor. The byte is copied without a clamp. Values outside the
  table were tested only at the writer, never fed into damage.

Keeping team 0 attacking team 1 with the exact same Sasuke attack
record, the joined original writers produced these results:

| Declared setup | Observed settings / ranks / banks | Amount |
| --- | --- | ---: |
| Mode 2, both slots in the input mask | `7,7:0,0:0,0` | 35 |
| Mode 2, only the attacker in the mask | `7,7:0,0:0,1` | 40 |
| Same mask, attacker rank 3 | `7,7:3,0:0,1` | 47 |
| Mode 1, bias 0, rank 0, same mask | `0,31:0,0:0,1` | 79 |

The asset-free native CLI fixture reproduces these amounts with
explicit rows and authored contact geometry. No native runtime
assignment rules changed. O0, O1 and O2 produced identical reports
for 139 writer checks and all four joined cases. The test doesn't establish who
produces the live mask or ranks, run the separate special
constructors, schedule a hit, or write HP.

Compile [`retail_damage_setup_fixture.cpp`](../tests/host/retail_damage_setup_fixture.cpp)
with the [phase fixture](#shared-action-gate) build flags, then run:

```sh
python3 -m tools.analyze.retail_damage_setup \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-source-damage-setup-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-damage-setup.json
```

### Producers of the source scene inputs

The [scene-input receipt](api-map/gg4e08-scene-damage-inputs.json)
goes one step earlier than the setup writers, on three original
paths:

- The four-slot automatic setup `80018494` runs to completion
  using the original RNG from a fresh declared seed. It writes
  Mode state 2 and **input mask 0**, but the resulting Scene
  **active mask is `0x0F`** with count 4. The settings writer
  supplies 7, and the Scene rank bytes are zero. Passing those
  outputs to the ordinary actor slices yields bank 1 and rank 0
  on both teams. `damage-indices=7,7:0,0,0,0:1,1,1,1` expresses
  that damage selection for a separately authored four-fighter
  preset; it does not reproduce the original lineup or match.
- The admitted-slot slice at `80173680` uses a declared port map
  and tests the mapped port's `0x40` status bit. When clear, it
  adds the slot to the **Scene active mask**, increments the
  count, and writes **rank 3**. A blocked slot leaves those fields
  alone. Neither path rewrites the **Mode input mask**. These
  checks stop before the next random draw.
- The port helper `8019052C` scans four declared port words for
  the first requested-bit match excluding the current port.
  It ORs that port into the Mode input mask, then counts the
  low four bits. For example, selecting port 1 with prior mask
  `0x01` produces `0x03` and count 2. The ordinary source bank
  slice then selects bank 0 for slots 0 and 1, and bank 1 for
  slots 2 and 3, regardless of their team labels.

The receipt matched across O0, O1 and O2. It does not identify
hardware polling, front-end mode transitions, or every rank
producer. Native presets remain explicit; no live policy or
automatic bank assignment was added.

Compile [`retail_scene_damage_inputs_fixture.cpp`](../tests/host/retail_scene_damage_inputs_fixture.cpp)
with the [phase fixture](#shared-action-gate) build flags, then run:

```sh
python3 -m tools.analyze.retail_scene_damage_inputs \
  "$GF_RETAIL_ROOT/sys/main.dol" \
  /tmp/opencode/gf-scene-damage-inputs "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-scene-damage-inputs.json
```

### Processed pad to menu enrollment

The [menu-input receipt](api-map/gg4e08-menu-port-input.json)
joins the processed-pad buffer at `803C727C` (four 32-byte
records) to the original menu update `80190038`. The update calls
`8016E694` to transform the packet's button word, gate it by the
status byte at `+21`, and write button edges against the previous
menu-held word. The battle input consumer reads the same buffer;
it does not use these per-menu edge records.

- In menu branch 2, the update clears the current port's edge
  word and collects the other ports' edges. A held `0x100` does
  not produce a fresh `0x100` edge. Status zero clears the held
  word; both `1` and `255` took the nonzero branch in the fixture.
- Of the tested menu edge bits, `0x200` takes precedence over
  `0x100`, which takes precedence over `0x80`. The `0x200` path
  records a separate port without enrolling it. The other two
  paths call `8019052C` with their respective bits; the first
  matching port in scan order wins.
- Menu branch 1 reads only the current port. The same `0x100`
  and `0x80` edges advance the menu step without updating the
  input mask. The branch selector is not battle Mode state.
- With prior mask `0x01`, enrolling port 1 produces mask `0x03`,
  count 2, and banks `[0, 0, 1, 1]` for four declared ordinary
  actors on teams `[0, 1, 0, 1]`. Re-selecting a port whose bit
  was already set does not increase the count.

All 13 cases matched across O0, O1 and O2. They start from
declared processed packets and menu history and stop before
audio/transition work, or return without it. No hardware polling,
full menu transition, actual actor scheduling, or native runtime
assignment is qualified by this test.

Compile [`retail_menu_port_input_fixture.cpp`](../tests/host/retail_menu_port_input_fixture.cpp)
with the [phase fixture](#shared-action-gate) build flags, then run:

```sh
python3 -m tools.analyze.retail_menu_port_input \
  "$GF_RETAIL_ROOT/sys/main.dol" \
  /tmp/opencode/gf-menu-port-input "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-menu-port-input.json
```

Generate the metadata locally using the qualified AFS as well as the
atlas inputs. Compile the fixture in place of the phase fixture in the
command below; it takes the same DOL and recomp-module arguments.

```sh
python3 -m tools.analyze.retail_action_input \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  "$GF_CATALOG" "$GF_SYMBOLS" /tmp/opencode/gf-action-input.json
```

## Shared action gate

The [phase-gate map](api-map/gg4e08-action-phase-gate.json) isolates
`80066838` across **250 direct call sites in 249 functions**. It compares
horizontal distance against an actor-scaled threshold. The old notes
reversed its return value: for finite inputs it returns `0` beyond the
threshold and `1` at or below it. A zero actor counter takes priority and
returns `-1`; a separate actor flag can force `1`. Callers test four
different conditions: `> 0` (165), `<= 0` (51), `== 0` (32) and `!= 0` (2).
These branches cannot all be modeled as one Boolean phase delay.

The original helper passed ten synthetic actor inputs on each of the
local unmodified O0, O1 and O2 recomp modules, covering equality, scale,
vertical projection, both early exits, and their precedence. An exploratory live battle run
loaded the read-only hook but captured no call to this helper, so there
is no live phase/tick claim. To repeat the bounded helper check:

```sh
python3 -m tools.analyze.retail_action_phase_gate \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_SYMBOLS" /tmp/opencode/gf-phase-gate.json

c++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I "$GF_RECOMP_HEADERS/GXRuntime/include" \
  -I "$GF_RECOMP_HEADERS/Source/Core/Core/PowerPC/StaticRecomp" \
  tests/host/retail_action_phase_gate_fixture.cpp -ldl \
  -o /tmp/opencode/gf-phase-gate-fixture
/tmp/opencode/gf-phase-gate-fixture "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RECOMP_MODULE"
```

`GF_RECOMP_HEADERS` points to the local ModernGekko `vendor/dolphin`
tree; `GF_RECOMP_MODULE` is a trusted local unmodified module. The fixture
executes one entry with a dispatch limit and rejects external IO, fallback
and guest exceptions. No retail module is a dependency of the native build.

## Generate locally

```sh
python3 -m tools.analyze.retail_code_atlas \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_CATALOG" "$GF_SYMBOLS" \
  /tmp/opencode/gg4e08-code-atlas.json --db "$GF_DECOMP_DB"
```

Use the qualified GG4E08 DOL and GFBC catalog. `GF_SYMBOLS` is the local
matching project's analysis `symbols.txt`; `GF_DECOMP_DB` is an optional
local SQLite snapshot. Inputs are read-only, revisions and sizes are
checked, and the command refuses an existing output. The symbol input is
identified by digest; omitting the database omits match labels. No retail
executables, assets, instruction bytes, generated or matching source, or
private function names are in the atlas.

## Remaining verification

Use the original actor/action and collision anchors for replay comparisons
before turning these routes into portable behavior. The immediate questions
are which contexts and first payloads fire, whether a move is canceled or
just held on a pose, and when the actor can move or act again. Later phases,
actor-local slowdown and reaction locks remain unqualified. This atlas does
not change native controller timing, collision, or damage.
