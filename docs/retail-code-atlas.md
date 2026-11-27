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
expresses the order without copying raw source speeds into a
package. The separate
[authored key-21 test](retail-roster-extraction.md#machine-red-projectile-speed-ramp-test)
keeps native launch speed 32 and scales the proportions by 0.4.

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

### Five additional projectile factories and their action arms

The [small-carrier census](api-map/gg4e08-small-projectile-roster.json)
checks the selected profiles for all 206 Borgs and keeps call sites
without a qualified action association in the inventory.

| Family | Rows inspected | Direct sites → action candidates |
| --- | ---: | ---: |
| Type 14 throw | 5 | 8 → 6 |
| Type 15 bomb | 10 | 10 → 10 |
| Type 47 Dragon | 6 | 1 → 1 |
| Type 49 Worm | 2, reviewed prefix | 2 → 2 |
| Type 94 Worm | 3, reviewed prefix | 4 → 2 |

One site can select different keys. For the Dragons, the single
model-gated call selects six distinct rows. The static opening
records have raw power 80 for three models and 40 for the other
three. These are not the power of later area or descendant hits.
The Worm entries cover stationary B only. Type 49's zero-power
opening stays separate from its group-7 follow-up below.

The type-15 review resolves a previously unindexed action branch.
Powered Gunman (`0101`) uses keys **8, 9, 6, 7** in that order on
**charged X, state 3**. State 2, ordinary X, takes a different branch.
The debit at `80167138` belongs to that state-2 arm, so no debit is
assigned to the four-call arm. This does not establish the earlier
charge or stock schedule. Double Ninja's optional second bomb
requires a nonzero actor byte at `+0x7C8`; its queue is withheld
rather than borrowed from the normal actor.

Together with the type-0 and type-2 maps, these receipts inventory
267 direct call sites and associate 137 with static action candidates
on 75 Borgs. The other calls and other carrier families are not
assigned by analogy. No gameplay is emitted or changed.

```sh
python3 -m tools.analyze.retail_small_projectile_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-small-projectile-roster.json
```

### Shared type-5 areas: common records and scoped ancestors

The [type-5 census](api-map/gg4e08-type5-roster.json) inventories all
27 rows and **45 calls** across two allocation entries. The main
entry requests a bucket relative to the credited actor. The other
uses a fixed bucket and a different bind submode, but still copies
the parent and credited actor. Both request two descriptor cells.

Thirty sites have a literal key in `r5`; 13 read a key directly from
a bounded parent table. The remaining two read the type-89 cache at
`+0x17D`; all 17 reviewed type-89 rows write area key 16 there. This
resolves key sources for all 45 sites and accounts for all 27 keys.
A literal key does not identify its owner.

The 27 rows select 21 groups in the scene common collision bank.
Each descriptor is a local-origin sphere with raw radius 310, but
their common reaction records carry raw power **20, 30, 40 or 100**.
The growth and cap also vary by key: the six Dragon areas share
group 60 and raw power 40, but not one common scale curve. These
are neither measured live radii nor HP amounts.

The census ties scoped predecessors to 24 Borgs without calling
the downstream area a direct cast. For instance:

- The normal Ninja type-15 selectors use area key 0; the conditional
  extra Double Ninja bomb remains conditional. The negative selectors
  on the Powered Gunman charged carriers go to a different child path.
- Time and Remote Bomber's reviewed B placements create type-41
  carriers. Their later area keys are 19 and 20. The owner-latch
  call is restricted to parent row 3; it is not a broadcast.
- Roach pays for a placed carrier at `801FAD10`. Its later inner
  phase calls the area factory with **key 26**, not key 0, even
  though both area rows use common group 3. The separate proximity
  exit is not assigned this area.

No gameplay is emitted or changed. The census is static. The
separate bounded replays verify shape registration, recipients,
or damage only on their declared inputs.

```sh
python3 -m tools.analyze.retail_type5_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type5-roster.json
```

### Type-5 sibling exits: contact wins, and type 77 uses the root

Revision 4 joins seven more static routes across six Borgs. These
are the separate area branches of the type-33, 52, 74, and 77
parents, not their zero-HP or expiry type-4 effects. Time Bomber's
type-33 route and earlier type-41 placement both select key 19,
but have different predecessors and callbacks.

| Parent | Scoped area key | Position used on actor contact |
| --- | ---: | --- |
| Type 33, Time Bomber | 19 | Saved hit point (`+0x2A0`) |
| Type 52, Machine Blue / Proto Blue | 11 | Saved hit point (`+0x2A0`) |
| Type 74, Blue Striker | 6 | Saved hit point (`+0x2A0`) |
| Type 77, Red / Violet Attacker | 7 | Carrier root (`+0x20`) |

The [exit replay](api-map/gg4e08-type5-sibling-exits.json) supplies
phase one, a contact latch, HP, and timer values after the original
constructor. A set latch picks the area even with zero HP and
an expired timer. With only one descriptor, the area refuses and
the parent still closes; no type-4 fallback allocates, although
its pools are available. Without the latch, zero HP or expiry
requests type 4/key 0 at the root. An out-of-bounds terminal
control closes with neither request.

The separate stage-probe branches use `+0x20` on all four parents
in the static census. This bounds control is not a terrain probe;
the stage replay below exercises that branch separately. All 36
cases matched across O0, O1, and O2. This replay stops at allocation.

Compile [`retail_type5_sibling_exit_fixture.cpp`](../tests/host/retail_type5_sibling_exit_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type5_sibling_exit \
  "$GF_RETAIL_ROOT/sys/main.dol" docs/api-map/gg4e08-type5-roster.json \
  /tmp/opencode/area-exit "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type5-sibling-exits.json
```

### Type-5 sibling follow-up: shared damage, different reach

The [follow-up replay](api-map/gg4e08-type5-sibling-followup.json)
continues the allocated areas for all six scoped Borgs against a
stationary Sasuke. Visit 1 initializes the area and binds a radius
**3.1** sphere, but does not publish it for attack. Offering a slot
on visit 2 did not make a refused area retry; it retired on visit 3.

| Area key | First published radius (visit 2) | First hit at offset 70 / 90 / 420 | Peak radius | Shape / carrier return |
| --- | ---: | --- | ---: | --- |
| 6, 11, 19 | 34.1 | Visits 3 / 4 / miss | 387.5 | Visits 18 / 51 |
| 7 | 65.1 | Visits 2 / 3 / 8 | 1240 | Visits 22 / 55 |

All four keys hit at offset 22 on visit 2. An accepted hit removed
**17 HP** using that area's common amount record. The same token
wrote nothing more. The original parent returned on visit 2; the
admitted area kept its own shape and resources until its later
return. The parent cell was not reused.

All 30 cases matched across O0, O1, and O2. The parent latch and
initial positions were supplied. This does not replay the
parent's hit, live reaction tail, whole move, or native gameplay.

```sh
python3 -m tools.analyze.retail_type5_sibling_followup \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-type5-roster.json \
  /tmp/opencode/area-exit "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type5-sibling-followup.json
```

### Type-5 sibling stage exits: wall effect before area

The [stage replay](api-map/gg4e08-type5-sibling-stage.json) advances
the six scoped parents from `z=900` toward `z=1100` in one
declared active visit. The clear control keeps `z=1100`. With a
wall triangle at `z=1000`, the original intersection moves the
root to `z=1000`, and the area requests that corrected point.
Clipping the probe endpoint to `z=950` instead misses the wall;
the carrier still moves to `z=1100` on that visit.

The wall branch requests a separate **class-2 effect in fixed
bucket 31** before the type-5 area. It takes one descriptor,
the area takes two, and their carrier cells are separate:

| Spare descriptors | Class-2 cell supplied | Wall effect | Area |
| ---: | --- | --- | --- |
| 3 | Yes | Allocates first | Allocates second |
| 2 | Yes | Allocates | Refuses |
| 2 | No | Refuses | Allocates |
| 1 | No | Refuses | Refuses |

Even when both allocations refuse, the root correction and stop
remain in effect. A supplied contact latch bypasses the probe
and uses the previously verified contact origin instead. The
wall helper is not the separate type-4 expiry effect.

All 42 cases matched across O0, O1, and O2. The wall was a
synthetic triangle, and the children were observed only through
allocation. No terrain mesh equivalence, live scheduling, child
damage, or native gameplay was tested.

```sh
python3 -m tools.analyze.retail_type5_sibling_stage \
  "$GF_RETAIL_ROOT/sys/main.dol" docs/api-map/gg4e08-type5-roster.json \
  /tmp/opencode/area-exit "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type5-sibling-stage.json
```

### Wall-effect census: wrapper arguments are not effect selectors

The [wall-effect census](api-map/gg4e08-wall-effect-census.json)
accounts for **55 direct stage-wrapper calls** and **11 direct
effect-helper calls**. The wrapper's callers pass `0`, `1`, `8`,
or `255` in `r4`, but it overwrites that argument before the
probe. On a hit it supplies effect selector **0** in `r5`.
Ten helper calls resolve to selector 0. The contact resolver's
other call reads an amount-record byte at `+9`. The census
below inventories scoped source values without qualifying the
live contact gate.

Ten constructor-to-callback chains trace **72 scoped static
routes across 35 Borgs**. Type 14 calls the effect helper from
a different terrain kernel; the other nine traced parent types
use the shared stage wrapper. This does not qualify live terrain
admission for those routes.

In the selector-0 [lifetime replay](api-map/gg4e08-wall-effect-census.json),
the helper allocates one class-2 cell in fixed bucket **31** and
one descriptor. The original initializer chooses visual record
**21**. With initialization counted as visit 1, the scale samples
are **1**, **2**, and **3** on visits **1**, **5**, and **21**.
Both cells return on visit **23**; no collision shape publishes.
Missing either cell refuses allocation. Owner clock step `0.5`
and an inactive parent with retained storage did not move the
retirement visit.

All six cases matched across O0, O1, and O2. The replay supplies
the effect request directly and uses empty render records. It
does not bind the dynamic contact selector or qualify retail
drawing, damage, live scheduling, or native gameplay.

Compile [`retail_wall_effect_fixture.cpp`](../tests/host/retail_wall_effect_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_wall_effect_census \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/wall "$GF_RECOMP_MODULE" /tmp/opencode/gf-wall-effect-census.json
```

### Contact-effect selectors: scoped records and whole descriptor allocations

The [contact-effect census](api-map/gg4e08-contact-effect-census.json)
reads amount-record byte `+9` for the attack steps in seven pinned
queue receipts. After deduplication, **69 of 73** actor records
carry selector **1** and **four** carry selector **8**, across 86
normal Borg models. All 22 reviewed common type-5 and type-9
records carry selector **0**. The two type-76 receiver-only steps
are excluded from outgoing evidence.

Those bytes are not observations of the hit gate at `8002F87C`.
The isolated allocator replay supplies selectors directly:

| Selector | Descriptor cells requested |
| --- | ---: |
| 0, 1 | 1 |
| 2, 3, 4 | 3 |
| 8 | 8 |

All tested requests need one class-2 cell in fixed bucket 31.
One descriptor short means no allocation and no partial debit;
surplus descriptors stay free. The constructor keeps the
effect family and variant separate and consumes no shape slot.
Selectors 2–4 are direct fixture controls, not assignments to
the inventoried actor records.

All 24 cases matched across O0, O1, and O2. This replay starts
at the helper; the accepted-pair replay below checks the gate
separately. Neither changes native contact-visual support.

```sh
python3 -m tools.analyze.retail_contact_effect_census \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/wall "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-effect-census.json
```

### Contact-effect gate: HP survives effect refusal

The [accepted-pair replay](api-map/gg4e08-contact-effect-tail.json)
runs original published shapes against stationary Sasuke through
the full pair call, with declared state and empty audio/render
support. The amount, helper-entry, and allocator observations
fork from one saved inlet rather than counting separate hits.

| Tested attack | Selector at `8002F87C` | HP removed |
| --- | ---: | ---: |
| Red Attacker / Orange Fighter, type 76 | 1 | 35 each |
| Flying Saucer, type 32/key 8 | 8 | 26 |
| Standalone common type-9/key-4 effect | 0 | 17 |

An open gate writes receiver `+0x2AF = 4` before requesting
the visual. With no class-2 cell or one descriptor short, the
gate still closes, HP and ledgers still reflect the hit, and no
effect allocates. A seeded held gate suppresses the request but
not damage. Reopening just the gate after a hit does not make
the same token apply damage or request another effect.

All 20 cases matched across O0, O1, and O2. The type-9 source was
constructed directly, not emitted by a live floor hit. This
replay does not tick the gate or initialize the new effect. The
separate clock replay below checks the countdown. The joined
replay continues the effect allocated by the accepted hit.

Use the same type-76, type-32, and type-9 fixture executables
as their earlier replays:

```sh
python3 -m tools.analyze.retail_contact_effect_tail \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type76-paid /tmp/opencode/type32-bind /tmp/opencode/type9-bind \
  "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-effect-tail.json
```

### Dynamic contact visuals: separate storage admission and refusal cleanup

The [initialization replay](api-map/gg4e08-contact-effect-init.json)
starts selectors **1** and **8** at the original helper, after the
selector choice already verified in the accepted-pair replay.
Both allocate in bucket 31, then try to obtain a separate type-3
visual object before reaching the heap. The battle descriptors
do not satisfy that later request.

- **Selector 1** stops before advancing the RNG if the scene already
  holds 32 bursts or 512 entries. With space available, a seeded
  request for 19 entries splits **10 + 9**; the same seed at 496
  and 511 entries is trimmed to **8 + 8** and **1 + 0**. Each entry
  requests 56 bytes. The RNG advances even if visual storage is
  refused.
- **Selector 8** requests **256 bytes** regardless of those two
  scene counters in the tested inlet. It does not advance the RNG at
  this allocation boundary.

In a separate fork, supplying one type-3 cell reaches the original
heap entry with the request rounded up to 32 bytes (56 → 64 and
1,064 → 1,088). That fork stops before allocating. With the
type-3 pool empty, or when the selector-1 caps refuse the request,
the effect enters phase 2 on visit 1 and phase 3 on visit 2. Visit
3 returns its class-2 cell and all original descriptors. These
paths do not charge the scene counters or publish an attack
shape, even when a shape cell is available.

All eight cases, including six heap-entry forks, matched across
O0, O1, and O2. This does not qualify successful emitter
initialization, drawing, live scheduling, or native gameplay.
The separate storage replay below continues past the heap entry.

Compile [`retail_contact_effect_init_fixture.cpp`](../tests/host/retail_contact_effect_init_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_contact_effect_init \
  "$GF_RETAIL_ROOT/sys/main.dol" /tmp/opencode/impact-init "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-contact-effect-init.json
```

### Dynamic contact visuals: successful storage and update lifetimes

The [storage replay](api-map/gg4e08-contact-effect-storage.json)
runs the original OS heap initialization and default SDK dispatch
registration on a declared guest arena. The original allocator
splits a free block, returns the aligned payload, and attaches it
to the type-3 visual object. No heap callback or result is supplied
by the fixture. The type-3 row has one declared cell and uses the
source destructor from row `8036F60C`.

| Replay inlet | Heap block including header | Scene budget returned | Heap block and object cells returned |
| --- | ---: | ---: | ---: |
| Selector 1, 16 entries | 928 bytes | Visit 18 | Visit 20 |
| Selector 1, only one entry left | 96 bytes | Visit 17 | Visit 19 |
| Selector 8, empty or full scene budgets | 288 bytes | No debit | Visit 13 |

Selector 1 charges the scene before requesting visual record 2 and
creates one child joint per admitted entry. In the 16-entry case,
eight entries are active on visit 1 and the next eight join on visit
6. The scene counters return when the last entries expire, **two
visits before** the type-3 payload, visual object, and class-2 battle
resources return. Separate HSD object-pool reservations are not
counted as part of that returned payload.

Selector 8 uses the same activity-mask sequence at both tested
scene occupancies: two pieces on visit 1, a peak of six on visits
5–6, and none on visit 11. This replay supplies empty records 53/54
for selector 8 and a single-joint template without animation or
geometry for selector 1. Only the fixed-bucket updater runs; no
collider publishes and no drawing runs.

Four cases and 65 visits matched across O0, O1, and O2. These
observations qualify allocation and update visits under declared
inputs, not retail boot, loaded-match timing, drawing, or native
gameplay. The separate preparation replay below supplies joint
records for both selectors and checks what they enqueue.

Compile [`retail_contact_effect_storage_fixture.cpp`](../tests/host/retail_contact_effect_storage_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_contact_effect_storage \
  "$GF_RETAIL_ROOT/sys/main.dol" /tmp/opencode/impact-storage "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-contact-effect-storage.json
```

### Dynamic contact visuals: per-view model preparation and draw lists

The [preparation replay](api-map/gg4e08-contact-effect-render-prep.json)
loads geometry-free joint templates under the original record IDs:
2 for selector 1 and 53/54 for selector 8. It runs the original
bucket update, early transform walk `80089554`, effect collection,
and selected-view callback `80089EB4`, without running a full scene
frame.

- **Selector 1** writes the active child joints' world translations
  in the matching view callback and enqueues **one group descriptor**
  in draw lane 5, even when all 16 children are active.
- **Selector 8** prepares the active pieces' matrices in the early
  transform walk. The matching view callback does not rewrite them;
  it enqueues **one descriptor per active piece** in lane 5, with
  higher piece indices first. Six descriptors were queued at the peak.

The fixture's effect enters view list 0. With the context byte set
to 1 instead, neither selector queues a descriptor, although selector 8
still prepares its matrices. At phase 2 the effect can still be
collected, but its zero activity mask leaves the draw lane empty.
Phase 3 excludes it from collection. None of these passes changes
the tested scene budgets or publishes a collider.

All **66 view observations** matched across O0, O1, and O2.
The 27 nonempty lists reached the original lane consumer and stopped
at `8025A210`, before light/fog setup touches GX. No joint submission,
pixel output, live camera behavior, or native gameplay is qualified.

```sh
python3 -m tools.analyze.retail_contact_effect_render_prep \
  "$GF_RETAIL_ROOT/sys/main.dol" /tmp/opencode/impact-storage "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-contact-effect-render-prep.json
```

### Dynamic contact visuals: exact common archives and first model installs

The [source-art replay](api-map/gg4e08-contact-effect-source-art.json)
replaces the joint templates with exact local `efct.pzz` blocks
from AFS member 697. The original staging function selects slot
**1** for the declared single view and slot **2** for split.
The fixture stops before DVD, supplies the qualified decompressed
block to that destination, and runs the original owner allocation
and HSD relocation. Both archives publish 157 scene records; all
4,705 single and 4,682 split relocations were checked.

| Selector / declared input | Models loaded on the first update, in either archive |
| --- | --- |
| 0 | Record 21 |
| 1, parent lane 0 / credited actor lane 1 | Record 2 |
| 1, parent lane 1 / credited actor lane 0 | Record 3 |
| 1, same lanes, scene mode byte 3 | Record 2 |
| 8 | Records 53 and 54, both in the same update |

Selector 1 uses the byte copied from the **attacking parent**, not
the credited actor. The scene-mode override is separate from the
archive selection byte. Each selector-1 case leaves only one entry
available, bounding the replay to one real model child. The original
joint loader returned live joints from the selected records and the
matching view queued their descriptors in lane 5.

Ten cases and twelve model installs matched across O0, O1, and O2.
This does not qualify DVD/decompression execution, texture-atlas
upload, later animation and rendering with these models, or a live
split-screen match. The replay below follows the real models through
update, retirement, and reuse, still without drawing. Native bindings
are unchanged.

```sh
python3 -m tools.analyze.retail_contact_effect_source_art \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/impact-storage "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-effect-source-art.json
```

### Dynamic contact visuals: real-model retirement and resource reuse

The [lifetime replay](api-map/gg4e08-contact-effect-source-lifetime.json)
runs two consecutive effects per selector in each qualified common
archive. The first update installs the real models. The fixture then
moves the parent and marks it inactive, but retains its memory. The
effect keeps its saved origin and outlives that change.

| Selector | Last visit queued in lane 5 | Resources returned |
| --- | ---: | --- |
| 0 | 21 | Visit 23: class-2 cell and one descriptor |
| 1, one entry | 16 | Visit 19: class-2 cell, one descriptor, and type-3 storage |
| 8 | 10 | Visit 13: class-2 cell, eight descriptors, and type-3 storage |

Selector 0 still queues its model on its phase-2 visit. Selector 1
returns its scene-budget debit on visit 17 but still refuses a new
effect while the only class-2 cell and descriptor are occupied.
Selector 8 installs records 53 and 54 on visit 1, then installs
one additional record-54 model on each of visits 2–5.

Both phases 2 and 3 refuse a competing request in the one-cell
fixture. After retirement the original helper reuses the battle cell
with a new parent and origin; the selector-1 lane change loads record
3 instead of record 2. The archive owner stays active, and all
relocated archive bytes remain unchanged across both generations.
The replay verifies effect cleanup and reuse, not archive teardown.

All **12 lifetimes and 220 bucket visits** matched across O0, O1,
and O2. GX drawing and live scene scheduling remain unverified.
Native gameplay is unchanged.

```sh
python3 -m tools.analyze.retail_contact_effect_source_lifetime \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/impact-storage "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-effect-source-lifetime.json
```

### Accepted hits through original model preparation

The [joined replay](api-map/gg4e08-contact-effect-join.json)
advances the effect actually allocated by the original pair
call. There is no second helper request or reset between the
accepted hit and its visual update. The fixture stages the
qualified common archive and SDK state after the hit, and
overwrites the shared collision scratch as a control. The
allocated effect still uses the saved hit point.

| Accepted attack | HP lost | Selector | First update models with storage available |
| --- | ---: | ---: | --- |
| Red Attacker B / Orange Fighter X (type 76) | 35 each | 1 | Record 2 |
| Flying Saucer carrier (type 32) | 26 | 8 | Records 53 and 54 |
| Standalone type-9 key 4 | 17 | 0 | Record 21 |

When the later type-3 pool is empty, selectors 1 and 8 queue
no model. Offering a cell after that first refusal does not
restart them; their class-2 cell and descriptors return on
visit 3. Selector 0 still queues record 21 without a type-3
cell. HP, ledgers, token history, gate and shape counts stay
unchanged across these updates. The selector-1 inlet leaves
one scene entry available, so it loads one real child joint.

All 16 cases matched across O0, O1, and O2, including the
single and split common archives. The type-9 attack remains
a standalone inlet, not a live floor follow-up. The replay
stops at draw-list preparation and does not qualify full
frame scheduling, GX drawing, or native gameplay.

```sh
python3 -m tools.analyze.retail_contact_effect_join \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type76-paid /tmp/opencode/type32-bind /tmp/opencode/type9-bind \
  "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-effect-join.json
```

### Contact-effect countdown: four collision-reset visits

The [clock replay](api-map/gg4e08-contact-gate-clock.json) places
the reducer at `8002CDA4`, before pair resolution in `8002BB14`.
An idle collision walk decrements the gate for active actors,
eligible bucket objects, and the 12 view records. The tested
unregistered child, child of an inactive seat, and inactive actor
retain their values. Starting at 4 gives **3 → 2 → 1 → 0**, then
stays at zero on the fifth visit. The idle test has no published
pairs and stops at the later `8005A580` call.

A separate paired replay starts with a genuine type-9 hit that
sets the gate to 4 and removes **17 HP**. It then calls the same
receiver reset leaf between hits. A second original constructor
publishes a fresh token after the first shape is unlinked. At
every delay, the fresh hit removes another **17 HP**; it only
requests and allocates a visual after **four** reset calls.
Reopening the gate naturally still does not admit the old token.

The same results held at declared local clock steps 1 and 0.5:
these count **collision-reset visits**, not elapsed seconds,
live frames, or hit-stop time. Both sets of ten observations
matched across O0, O1, and O2. The combined hit fixture does not
run the entire collision walk with a loaded match or qualify
effect lifetime, live scheduling, or native gameplay.

Compile [`retail_contact_gate_prelude_fixture.cpp`](../tests/host/retail_contact_gate_prelude_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags and reuse the
type-9 fixture executable:

```sh
python3 -m tools.analyze.retail_contact_gate_clock \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json /tmp/opencode/gate-prelude \
  /tmp/opencode/type9-bind "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-gate-clock.json
```

### Contact pose clock: receiver timer and linked rate copying

The [body-clock replay](api-map/gg4e08-contact-pose-clock.json)
separates arming a timer from consuming it. The original pair
has already written the receiver timer at the amount entry,
before writing HP or changing either rate. An existing timer
of 0, 2 or 6 is replaced only by a larger attack-record seed.
The credited fighter never acquires that timer from the tested
non-body collision objects.

| Tested attack | Source seed | Slow body-clock visits from a clear timer |
| --- | ---: | ---: |
| Red Attacker B / Orange Fighter X (type 76) | 1 | 0 |
| Flying Saucer type-32 carrier | 4 | 3 |
| Standalone type-9 key 4 | 0 | 0 |

`8005A378` decrements before testing the remaining timer. A
positive remainder writes **0.03** to both local rates. On
expiry the primary rate restores the base; the secondary rate
restores the base times both local multipliers. For a declared
base of 1.5 and multipliers 0.5 and 0.75, that is **0.5625**.
These visits do not decrement the separate contact-visual gate.

With link kind `+0x4A0 = 2`, the follower reads the linked
body's current rates and leaves its own timer untouched. Reading
before and after the leader's visit gave the old and new rates,
respectively. The separate `+0x4A1` override takes precedence:
it writes three unit rates without using the link or ticking the
timer. Clearing the link resumes the follower's own countdown.
These are tests of declared body fields, not a replay of a live
linked form.

The static census found seeds 0/1/2/4/8 in the 73 scoped actor
records and only seed 0 in the 22 common records. None of these
95 records uses the receiver-timer bypass mask. Twelve accepted
pair cases and 25 isolated clock observations matched across
O0, O1, and O2. Melee timer playback, original frame scheduling,
pose sampling, and native mechanics remain outside this replay.

Compile [`retail_contact_pose_clock_fixture.cpp`](../tests/host/retail_contact_pose_clock_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_contact_pose_clock \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/pose-clock /tmp/opencode/type76-paid /tmp/opencode/type32-bind \
  /tmp/opencode/type9-bind "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-pose-clock.json
```

### Opcode-10 melee arms both body clocks

The [melee replay](api-map/gg4e08-melee-pose-clock.json) uses
the exact opening commands and hit groups for **Tau** (`0807`)
and **G-Red** (`0001`), with declared joint matrices and a
stationary Sasuke receiver. The original opcode consumes four
bytes even when lane zero is disabled. With the lane enabled,
the published shapes keep the executing fighter as the attack
object and the credited actor, rather than using a projectile.

| Original opening | Published steps | Source seed | Slow visits per body from clear timers | HP removed |
| --- | --- | ---: | ---: | ---: |
| Tau group 1 | 2 | 4 | 3 | 20 |
| G-Red group 4 | 6, 5 | 8 | 7 | 41 |

Both timers are armed at the amount entry, before HP or rate
writes. Each body keeps its own longer timer; holding the
attacker longer does not extend the receiver's timer, and vice
versa. A whiff arms neither. Re-running the retained pair after
expiry does not re-arm either body.

G-Red's two steps share amount record `8031B008`. Both overlap
in the control case, but only step 5 reached amount calculation
in that placement. Isolating either step still produced one
41-HP hit and the same two timers. This does not qualify general
shape order or attribution between steps with different amounts.
These direct-pair amounts use attacker phase byte 0. The
[frame-tail replay](#actor-tail-attack-admission-and-the-next-list-rebuild)
separately checks the active-phase amount.

The nine-row reviewed melee map statically references ten
distinct amount records: two with seed 0, seven with seed 4,
and one with seed 8. Only the two openings above were replayed.
All 12 cases matched across O0, O1, and O2. This does not run
live B input, clip progression, whole-frame hit-stop scheduling,
or native gameplay.

Compile [`retail_melee_pose_clock_fixture.cpp`](../tests/host/retail_melee_pose_clock_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_melee_pose_clock \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/melee-pose-clock "$GF_RECOMP_MODULE" /tmp/opencode/gf-melee-pose-clock.json
```

### Contact clock consumers: movement, waits, and clip requests

The [consumer replay](api-map/gg4e08-contact-clock-consumers.json)
takes the Sasuke receiver from the exact Tau and G-Red hit and
whiff cases. It runs the original receiver clock and then calls
four original helpers with declared inputs. The tested hit-stop
does not freeze these helpers:

| Original helper | Rate read | First clear-timer visit | First slowed visit |
| --- | --- | ---: | ---: |
| Planar advance `800676D8` | Primary | 100 units | 3 units |
| Heading step `80066E40` | Secondary | 15 angle units | 1 angle unit |
| Finite wait `8004CD24` | Secondary | 0.625 remaining | About 0.97 remaining |
| Relative-frame request `8004C298` | Secondary | 6.375 | About 6.03 |

The original interpreter installs the indexed one-unit terminal
wait at `802BD766`. The excluded lane does not spend it; with
lane zero enabled it completes on visit **3** in the whiffs,
**6** after Tau's hit, and **10** after G-Red's hit. Normal
secondary rate is 0.375 here because the fixture declares
local multipliers of 0.5 and 0.75. An explicit-frame request
stays at **2.5** in both clock states. Both clip probes stop at
the original loading call, before sampling a pose.

The wait and clip operands are original indexed probe commands,
not claims about what Sasuke's controller selects. The atlas
lists 1,128 direct calls to the wait updater; that census does
not qualify their scheduling. All 22 observations matched
across O0, O1, and O2. No moving-body sweep, full actor frame,
live pose sampling, or native gameplay is qualified.

Use the same melee-pose-clock fixture:

```sh
python3 -m tools.analyze.retail_contact_clock_consumers \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/melee-pose-clock "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-clock-consumers.json
```

### Actor update prefix: incoming and refreshed contact rates

The [frame-prefix replay](api-map/gg4e08-contact-frame-prefix.json)
retains the Sasuke receiver from the Tau and G-Red hit and whiff
cases. It runs Sasuke's original installer, then enters the original
actor wrapper `80055568`. The selected lower slot **3** calls the
original wait updater; the fixture stops at Sasuke's post callback,
after the lane visit and before the remainder of the actor update.

The early float counter `+0x694` spends the **incoming** secondary
rate before `80059068` refreshes the body clock. The scheduled lane
wait spends the **refreshed** rate. On the first visit after an
accepted hit, the early counter spends **0.375** while the later
wait spends **0.03**. On expiry those rates are reversed. The
early integer counter `+0x6B8` sees the pre-refresh timer; it
stays held on expiry and resumes on the following visit.

The original scheduled wait completes in **3 visits** after a
whiff, **6** after Tau's hit, and **10** after G-Red's hit, as
in the isolated helper replay. The fixture declares a settled
prepass mode and lane choice and suppresses the separate input
accumulator. These results do not show how the live controller
selects those modes or how a full scene schedules the actor.

All 22 visits matched across O0, O1, and O2. World updates,
original controller polling, and native gameplay remain outside
this replay.

```sh
python3 -m tools.analyze.retail_contact_frame_prefix \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/melee-pose-clock "$GF_RECOMP_MODULE" /tmp/opencode/gf-contact-frame-prefix.json
```

### Actor tail: attack admission and the next list rebuild

The [frame-tail replay](api-map/gg4e08-actor-frame-admission.json)
runs the complete `80055568` actor wrapper with Tau's and G-Red's
original installed callbacks, neutral lanes, and a declared stage.
It begins with their original melee shapes already published.
The fixture sets bit `0x20000000` in actor `+0x5E0`, then clears it
on the next visit. The original late helper `80055EE4` rewrites
`+0x276` to **1** and then **0**, despite the deliberately stale byte
at each inlet.

The wrapper does **not** retroactively edit the attack list. The
next original list rebuild omits the attack shapes while closed;
Sasuke's receiving shape is still present. No amount call, HP
write or body timer follows. After reopening, the next rebuild
re-admits the **same cells and token**, and one hit resolves. This
qualifies original attack-list admission, not a live guard or the
source of the status bit.

Separate forks changed **only** attacker phase `+0x18` before
the original pair. Tau went from **20 to 17 HP**, and G-Red from
**41 to 35 HP**, when the byte changed from 0 to 1. The active
phase amounts match the reopened hits; the change is not an
effect of toggling the admission byte.

All four actor visits and four phase-only controls matched across
O0, O1, and O2. The other explicit `+0x276` accesses, complete
scene scheduling, live input, and native gameplay remain outside
this replay.

```sh
python3 -m tools.analyze.retail_actor_frame_admission \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/melee-pose-clock "$GF_RECOMP_MODULE" /tmp/opencode/gf-actor-frame-admission.json
```

### Collision reset and the alternate contact lists

The [alternate-contact replay](api-map/gg4e08-alternate-contact.json)
checks the other `+0x276` store and load. On the declared slot-0
and slot-5 reset probes, `8003CC24` rebinds the slot's hit bank,
clears all 48 token-history entries, clears `+0x276`, and writes
**`0xEF`** to the separate role-exclusion mask `+0x272`. HP, the
body timer and the visual gate stay unchanged. These resets run
before shapes exist; they do not qualify any of the three outer
reset callers or resetting an active pair.

Across **206** normal hit blocks, the static census found **60**
queue-referenced role-`0x80` descriptors on **55** Borgs. Of these,
**46** are the same group-26 step-44 shape bytes; groups 3 and 4
account for the other 14. Sharing those bytes does not establish
the same damage record or a controller route.

The original direct binder loads that exact group-26 shape on
Machine Red and Drill Robot. Setting either its role-`0x80`
exclusion bit or the late outgoing byte withholds it from the
alternate attack list without returning the cell. Independently,
the receiver's role-`0x02` mask removes Sasuke from the receiving
lists while the attack stays published. After both roles reopen,
the ordinary pair pass still does nothing. The alternate pass
`8002DF08` accepts the retained attack once: **26 HP** for Machine
Red and **7 HP** for Drill Robot. Both local body timers arm in
each accepted case.

All **12 observations** matched across O0, O1, and O2. The replay
does not identify a live group-26 trigger, guard behavior, or
whole-scene scheduling. Native gameplay is unchanged.

Compile [`retail_alternate_contact_fixture.cpp`](../tests/host/retail_alternate_contact_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_alternate_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/alternate-contact "$GF_RECOMP_MODULE" /tmp/opencode/gf-alternate-contact.json
```

### Alternate VM contacts: Nurse healing and Patra damage

The [alternate VM replay](api-map/gg4e08-alternate-vm.json)
joins **23** indexed program choices on **nine** Borgs to
role-`0x80` group-3 or group-4 shapes. The bounded scan of 3,798
indexed programs found no group-26 opcode-10 request. No group-26
constant was proved at the 102 direct binder sites either, but
91 arguments remain unresolved. This does not rule out a live
group-26 trigger.

Three exact Angel Nurse and Patra Witch programs run in a
declared lane and pose. Their shapes publish only to the
alternate attack list. With the contacted Sasuke at 90 of
180 HP, the original signed HP writer gives:

| Tested command | Ally | Opponent |
| --- | --- | --- |
| Angel Nurse, groups 3 and 4 | Requests +200 HP; gains 90 HP and reaches cap | Requests and gains 50 HP |
| Patra Witch, group 3 | Removes 2 HP | Removes 8 HP |

Nurse bypasses the damage calculator and still requests healing
on the declared zero-HP receiver, but the writer does not revive
it. Patra calls the damage calculator. Its positive ally or
opponent ledger is already written at the HP entry; the
zero-HP control keeps that distinction between requested damage
and HP removed.

Nurse's group-4 command writes secondary state bytes
`81/7F/FF` even when shape allocation fails. With a cell,
the original VM returns with its first wait installed, and the
shape stays unpublished at frames 0 and 8. The fixture supplies
frame 9 to test admission separately from live pose progression.

All **12 cases** matched across O0, O1, and O2. The original HP
writer completes, but its caller is not resumed. This does not
qualify a live X cast or stock debit, the Patra type-60 Big
Spell route, later effects, or native gameplay.

Compile [`retail_alternate_vm_fixture.cpp`](../tests/host/retail_alternate_vm_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_alternate_vm \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/alternate-vm "$GF_RECOMP_MODULE" /tmp/opencode/gf-alternate-vm.json
```

#### Angel Nurse fallback: shape request precedes stock debit

The [Nurse setup replay](api-map/gg4e08-nurse-setup.json) starts at the
original class substate dispatcher `8007BB6C` with no selected target.
The normal installer supplies the bank; substate 0 selects program `4:0`
at `8007BD84`. An entry cut at the debit called from `8007BDC4`
observes the group-3 bind before the channel-2 debit changes stock.
A separate run completes setup and visits the next fallback substate once.

- **No shape cell:** the bind is refused, but mode-0 stock still falls
  from one to zero and starts the declared refill clock.
- **No stock:** the debit leaves the old stock fields unchanged, leaves
  latch `+0x548` at zero, and overwrites the VM cue at `+0x6E8` from
  `4` to `8`. The admitted shape survives the next fallback substate.
- **Shape lane masked:** the VM does not request group 3, even with a
  free cell. This does not prevent the debit.

The retained shape stays unpublished at the supplied frame 21 and
publishes at frame 22. With or without a successful debit, the
separately scheduled alternate pair requests 200 HP for the declared
ally, raising it from 90 to its 180 cap. This does not establish that
a zero-stock X press starts a cast or that live animation and collision
reach this schedule. Directed setup, later substates, and native gameplay
remain outside the replay.

The same fixture reproduced all **eight cases** across O0/O1/O2:

```sh
python3 -m tools.analyze.retail_nurse_setup \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/alternate-vm "$GF_RECOMP_MODULE" /tmp/opencode/gf-nurse-setup.json
```

### Type-60 casts: zero damage and a separate recipient effect

The [type-60 receipt](api-map/gg4e08-type60-contact.json) joins all eight
indexed direct calls to `8014452C` to two X release leaves in each of two
normal controller families. The first leaf of each family was executed;
the second was checked statically. Program `4:0` supplies the indexed
`(1,1)` event, which the replay declares at the release inlet.

| Normal model | Carrier key | Amount index | Type-33 arguments at contact |
| --- | ---: | ---: | --- |
| `0902` | 0 | 5 | `(1,1)` |
| `0909` | 2 | 6 | `(1,1)` |
| `0907` | 1 | 5 | `(2,1)` |
| `090C` | 3 | 6 | `(2,1)` |

The wrong event leaves stock alone. The exact event is consumed even if
the debit fails. A successful debit is not refunded if the carrier
cell, its four descriptors, or the later shape cell is unavailable.
An admitted carrier binds group 6, step 7, at its caster-relative
socket origin and publishes a radius-50 ordinary attack shape.

The tested ally and opponent overlaps both call the amount calculator
and reach the HP writer with **zero**. They leave HP and the tested
damage ledgers unchanged. The type-33 request is separate and names
the struck actor, not the caster or its stored target. It needs its
own cell and three descriptors. Even when those resources are refused,
the retained pair does not request another effect when resources arrive.

All **40 cases** matched across O0/O1/O2. The supplied pose and release
cue do not qualify live casts, continued flight, complete team eligibility,
effect lifetime, or native gameplay. The next replay joins the separate
size-change path.

Compile [`retail_type60_fixture.cpp`](../tests/host/retail_type60_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type60_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type60 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type60-contact.json
```

#### Type-60 size changes: receiver mask, scene consumer and expiry

The [size replay](api-map/gg4e08-type60-scale.json) joins a signed recipient
delta at `8043612C + slot` to the original scene consumer `800681E4`.
The pair only queues it; the scene chooses a target scale and starts a
**900-visit** timer on the receiver. The tested ally, opponent, and
opponent-without-an-effect-cell cases gave the same size result:

| Normal caster | Signed size steps | Sasuke target scale from 1.0 |
| --- | ---: | ---: |
| `0902` | +1 | 1.40 |
| `0909` | +2 | 1.80 |
| `0907` | −1 | 0.81 |
| `090C` | −2 | 0.70 |

The normal actor initializer copies spec `+0xA8` to receiver `+0x59C`.
Bits `0x04` and `0x08` **exclude** the corresponding size change.
Both are set on **17 of 206** indexed normal records. In the `0602`
replay, the overlap spawned a type-15 fallback instead of type 33;
it queued no size delta and did not change HP.

The size consumer separates the raw index from its clamped lookup.
For example, the `0909` cast moved Sasuke's raw index from 31 to 33,
but the lookup stayed at 31. Applied scale does not jump at the
consumer: `8005BAB8` moves it toward the target in 0.05 steps.
On visit 900 the temporary offset clears and the return ramp begins
on that visit. The number refers to isolated callback visits, not
seconds or verified live frames.

All **20 cases** matched across O0/O1/O2. The scene, pose and initial
size are bounded inlets. Live movement, type-33 lifetime and rendering,
and native gameplay remain unqualified.

```sh
python3 -m tools.analyze.retail_type60_scale \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type60 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type60-scale.json
```

#### Type-60 repeat contacts: stacking and cancellation

The [repeat-contact replay](api-map/gg4e08-type60-refresh.json) uses
separate original caster, carrier and shape cells for each fresh
contact on Sasuke. The two contacts do not depend on a type-33 cell.

- **Separate scene passes:** after three isolated size ticks, a second
  `+1` or `−1` adds to the existing offset and restarts the timer at
  900. A `+1` followed by `−2` crosses neutral to `−1` and also
  restarts it. A second contact that exactly cancels the offset
  sets the target back to neutral but leaves the timer at 897.
  Applied size starts returning on the next size tick, not at the
  scene write.
- **One scene pass:** two accepted tokens contribute to a single signed
  sum and one size-consumer call. Equal opposing deltas cancel in
  either tested order without starting a timer. Two `+1` deltas start
  at `+2`; `+1` and `−2` start at `−1`.
- **Same retained token:** no new size-consumer call and no refresh.

All **10 cases** matched across O0/O1/O2. HP stayed unchanged and the
isolated callback returned applied scale to its starting value in
each case. This does not qualify live recast timing, order in a full
match, additional recipients, or visual-effect refresh.

```sh
python3 -m tools.analyze.retail_type60_refresh \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type60 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type60-refresh.json
```

#### Size tables: collision radius, movement and secondary clock

The [size-consumer replay](api-map/gg4e08-size-consumers.json) audits all
206 indexed normal specs. **186** use size table `802DD5A0` and
**20** use `802DD720`; the specs contain **15** table/minimum/maximum
combinations. The same signed index change need not select the same
scale. The `0909` cast's `+2` targets **1.80×** on Sasuke (`000A`)
and **1.40×** on `0502`.

The original sphere publication uses applied scale `+0xC0`, not the
target `+0xB8`. The size write alone leaves the hurt radius unchanged.
After the ramp settles, detached queries against the same radius-50
cast sphere pass at axis distance **89** and fail at **90** for Sasuke
(neutral: 72/73). For `0502` they pass at **106** and fail at **107**
(neutral: 90/91). These are coarse sphere checks, not second hits.

In separate forks, the original body clock honors the contact hold
before using the target row's secondary-rate operand `+0x1DD0`.
After that hold, primary rate is **1.0** and secondary rate is **0.96**
for the tested `+2` case. An isolated movement visit with a declared
axial direction and speed 10 consumes the independent `+0x5F8`
operand and travels about **17.46** or **13.58** units respectively.
The rate and distance operands switch with the target row, before
the applied radius finishes ramping; they also reset first on expiry.

All **48 observations** matched across O0/O1/O2. Each clock test
forks from the retained hit timer; isolated size ticks do not advance
it. This does not qualify elapsed live frames, exact moving-body
contact, full poses, input scheduling, or native gameplay.

```sh
python3 -m tools.analyze.retail_size_consumers \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type60 "$GF_RECOMP_MODULE" /tmp/opencode/gf-size-consumers.json
```

### Normal startup: stock channels and size defaults

The [startup replay](api-map/gg4e08-stock-startup.json) runs each original
normal installer and then `8005614C` at each of its ten levels, with
the exact role-0 actor records from the archive. It stops at the first
equipment request or the later material initializer. All **2,060**
visits matched on O0/O1/O2.

The level row writes **channels 0 and 2**. Sentinels for channel 1's
stock and definition fields stay untouched. Channel 0's level-10
`0C06` row retains current 23 and limit 22. The startup previous-count
field receives the limit, not a second copy of current count.
Of 4,120 stock rows, 250 start with nonzero clocks; these are empty
mode-0 or mode-1 rows with nonzero presets. This is not a refill replay.

The original size initializer also selects raw and clamped index
16 and neutral 1.0 scale for every checked normal visit. The original
stock-side prefix copies both recipient masks from the selected actor
record. Neither result qualifies variants, fusion, or a complete actor
constructor. The extraction fix changes evidence links, not native moves.

Compile [`retail_stock_startup_fixture.cpp`](../tests/host/retail_stock_startup_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_stock_startup \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/stock-startup "$GF_RECOMP_MODULE" /tmp/opencode/gf-stock-startup.json
```

### Normal stock lifecycle: checks, debits and refill

The [stock lifecycle receipt](api-map/gg4e08-stock-lifecycle.json)
takes **4,120** exact initialized normal channel/level rows through
one-unit check-only and committed requests, then forks the result
into refill controls. **2,920** requests pass both checks; check-only
writes nothing. The refill wrapper `8005568C` calls the original
`8006DCC0` with a base rate produced by `8005A378`. The declared
zero and nonzero `+0x6FC` cases consume steps **1** and **15**,
respectively. Each inhibit bit holds its own channel without holding
the other. The uninitialized channel 1 is masked in these controls.

For Angel Nurse (`0900`), the emptying debit arms **180** and the
isolated 180th step‑1 visit restores the stock and clears the clock.
For Kung-Fu Master (`0800`), two debits separated by one step‑1 visit
leave **599** on the clock. With step 15, the 20th visit restores one
round; the 40th restores the limit and retains a **−1** clock.
Modes 2 and 3 skip timed refill even with a declared positive clock;
a successful debit clears it. A separately invoked `8006DE10` returns
one stock in those controls, but no event that triggers it was replayed.

The replay also exposes **60 zero-limit mode‑1 rows on six Borgs**.
On the first refill visit the original writes **−1** to both count
fields; the `0705` timeline shows that a one-unit check and debit
still refuse, and expiry restores zero. This is source evidence, not
permission to widen the native stock domain.

All rows and five timelines matched on O0/O1/O2. The requests, clocks
and wrapper visits are bounded; they establish neither live input
or actor ordering nor elapsed wall-clock refill duration.

```sh
python3 -m tools.analyze.retail_stock_lifecycle \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/stock-startup "$GF_RECOMP_MODULE" /tmp/opencode/gf-stock-lifecycle.json
```

### Stock returns: owner gates and Thunder Wire exits

The [return receipt](api-map/gg4e08-stock-returns.json) indexes **54**
direct calls to `8006DE10`: 17 pass literal channel 0, 36 pass
literal channel 2, and one receives a signed selector via `800E0E68`.
For the four indexed type-41 factory keys, table keys 0 and 1
supply `−1` (no return), while keys 2 and 3 supply channel 0.
The initializer keeps that selector for the separate retirement
leaf. The [next replay](#type-41-bombs-bucket-order-area-bind-and-stock-return)
executes the qualified keys 2 and 3; keys 0 and 1 remain static.

The original post callbacks for `0200`, `020A`, and `020F` restore
depleted channel 2 when their tested owner gate is open, without a
carrier. The `0403` pre callback instead restores channel 0 and
checks both of its owner bytes. Filling the stock prevents another
grant on the next tested visit; the seeded clock stays unchanged.

Exact normal Thunder Robot `0404` starts with one channel-2 stock.
The bounded X release spends that unit before the type-43 request.

| Boundary | Observed return |
| --- | --- |
| Missing cue or empty stock | No carrier and no return |
| No carrier cell or descriptor | Factory returns one before a carrier exists (`800DA0AC`) |
| Carrier exists but opening shape is refused | Initializer returns one and starts retirement (`800DA274`) |
| Admitted carrier leaves stage bounds | Active callback returns one (`800DA4CC`) |
| Owner phase invalid or model changes | Carrier and shape retire, but stock is not returned |

Returns name the paying actor, not the stored target. Shape, carrier,
and descriptor resources are released at their own boundaries, with
no repeat stock write on cleanup. All **21 cases** matched on
O0/O1/O2. The cue, pose, and stage controls are declared; linked
Thunder Wire contact, live scheduling, and native gameplay remain
unqualified.

```sh
python3 -m tools.analyze.retail_stock_returns \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/stock-startup "$GF_RECOMP_MODULE" /tmp/opencode/gf-stock-returns.json
```

### Type-41 bombs: bucket order, area bind and stock return

The [type-41 replay](api-map/gg4e08-type41-bucket.json) runs the
normal owner hook `801304B8` with declared eligible B/X inputs and
the exact initialized stock. Time Bomber spends channel 0 and
requests key 2, pose 10. Remote Bomber spends the same channel
and requests key 3, with poses 12 then 11 from post-debit parity.
Carrier or descriptor refusal returns one immediately through
`800E0E68`. Opening-shape refusal instead returns on the next
phase‑2 visit, after the actual initializer has stored channel 0.

Two paid Remote bombs share the original bucket. One X command
with stock below cap arms the latch without spending stock. The
bucket walk selects the first placed bomb in both tested cell
orders and clears the latch before the area request. Cell or
descriptor refusal does not move the command to the sibling. The
command visit leaves stock at 6; the selected bomb's phase‑2 visit
returns it to 7 while the sibling stays active. At full stock the
tested X request does not arm the latch.

Time's declared near-expiry inlet requests area **19** without
consuming the remote latch. Remote requests area **20**. Both use
the parent root and preserve caster credit. With a separate shape
cell, their areas bind common groups **179** and **180** respectively.
Area allocation, shape bind, and stock are separate outcomes. The
tested invalid-owner phase reaches the type‑41 return on that same
visit, unlike the [tested Thunder Wire exit](#stock-returns-owner-gates-and-thunder-wire-exits).

All **16 cases** matched on O0/O1/O2. The original bucket is tested
in isolation; roots, the sibling substate, and Time's near-expiry
counter are supplied. The randomized initialization seeds do not
measure live lifetime. The next replay checks area damage;
live controller scheduling and native gameplay remain unqualified.

Compile [`retail_type41_bucket_fixture.cpp`](../tests/host/retail_type41_bucket_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type41_bucket \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type41 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type41-bucket.json
```

#### Type-41 area HP: credit, role gate and exact tangent

The [area HP replay](api-map/gg4e08-type41-area-hp.json) follows one
paid Time or Remote bomb into the original child initializer, sphere
publication, ordinary pair and HP writer. Time uses common amount
record `802C49D0`; Remote uses `802C49E8`. Both have raw power 20.
In the tested settings both removed **17 HP** from opposing Sasuke
and **4 HP** from allied Sasuke. The caster's opponent or ally ledger
and Sasuke's receiver ledger record the corresponding amount.

The terminal bucket initializes the child. The next bucket returns
stock **before** the first close hit, and the following visit returns
the parent carrier. Only the child owns the accepted token, which
does not hit the same recipient again. Refusing the opening shape
or masking Sasuke's receiver role prevents damage without preventing
the stock return.

The spheres reach radius **387.5** against Sasuke's radius 22.
At the tested stationary axis separation, **409.5 hits** (exact
tangency), while **410 misses**. The child shape returns on visit
18 and its carrier on 51. Time key 19 also matches the earlier
type-33 sibling replay's shape-refusal, near and middle cases, without
equating their parent routes or stock handling.

All **16 cases** matched across O0/O1/O2. Visit numbers start at
the terminal bucket and use separate original registry/pair passes.
The root and receiver pose are declared; moving-body sweeps, full
scene/input order, rendering and native gameplay remain unqualified.

```sh
python3 -m tools.analyze.retail_type41_area_hp \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type41 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type41-area-hp.json
```

#### Type-41 parent contact: bomb HP, exit priority and two hit tokens

The [parent-contact replay](api-map/gg4e08-type41-parent-contact.json)
holds the initialized bomb at a declared root for two publications.
Its radius-20 role-3 sphere enters both ordinary lists. The bomb's
own contact removes **26 HP** from opposing Sasuke and **6 HP**
from allied Sasuke, under the parent's token. At axis separation 42
it hits; 43 misses.

A separate type-9/key-4 projectile requests 17 HP from the bomb and
removes its only **1 HP**. Neither fighter loses HP or gains a damage
ledger entry. The bomb's outgoing latch stays clear; its zero-HP
branch requests the area instead. For Remote, an armed command takes
priority over either the outgoing latch or the zero-HP branch. Stock
still returns only on the later phase‑2 visit.

After a completed outgoing pair, the child hits the same Sasuke
under a fresh token. The original per-pass reset preserves the first
token, and both entries remain in history. The child deals **14 HP**
to the opponent or **3 HP** to the ally, for totals of **40** and
**9**. The parent leaves receiver byte `+0x6CA` at tier 1. Clearing
only that byte in a disposable calculator fork restores the isolated
child amounts of 17 and 4. The retained run uses the original tier.

All **14 cases** matched across O0/O1/O2. The declared roots,
isolated parent bucket, and stationary receiver do not qualify live
input, moving-body collision, full scheduling, or native gameplay.

```sh
python3 -m tools.analyze.retail_type41_parent_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type41 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type41-parent-contact.json
```

### Receiver damage tier: normal capacities and independent expiry

The [tier replay](api-map/gg4e08-damage-tier-clock.json) observes the
original reset gauge initialization for all **206** normal records:
**198** initialize to 500, **7** to 700, and **one** to 30,000.

The two exact area pairs use the **old tier** at the HP writer. On
the surviving tail they subtract 100 gauge wear and add 50 tier
charge. Crossing 100 advances one tier and discards overflow;
tier 63 stays capped. Depleting the gauge refills it and clears the
old tier and meter **before** adding the new charge. Receiver flag
`0x80` skips wear, not charge. The scene HP hold leaves HP and the
tested damage ledger unchanged but still runs those tier writes.

The original wrapper checks actor bit `0x01000000`, which can hold
both clocks. Otherwise the gauge and meter idle clocks subtract
raw actor `+0x1DCC`, not the refreshed secondary rate at `+0x1DC8`.
With a declared step of **0.5**, both begin at 60 and expire on
visit **120**. They still spend 0.5 when the separate contact hold
sets the secondary rate to 0.03. Independent near-expiry forks
refill the gauge or clear the tier without expiring the other.

All capacity rows and **16 contact cases** matched across O0/O1/O2.
This is a bounded wrapper replay, not a wall-clock conversion,
full reaction animation, KO schedule, or native gameplay change.

```sh
python3 -m tools.analyze.retail_damage_tier_clock \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/stock-startup /tmp/opencode/type41 "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-damage-tier-clock.json
```

### Normal receiver factors: active records feed portable extraction

The [normal receiver proof](api-map/gg4e08-normal-receiver-factors.json)
executes the original installer and initialization prefix before
four receiver-only amount probes per model. Across **206** normal
Borgs, **200** select record 0, **five** select record 1, and **one**
selects record 2. All **824** probes matched on O0/O1/O2.

The probes exercise both receiving banks with exact archived selector
triples, setting 7, rank 0, and declared high/low HP and force mirrors.
The inactive credit owner deliberately skips outgoing factors and
matchups. This is not a live damage-settings or matchup replay.

The opt-in roster extractor validates the installed record's digest
and selectors before using that record for the existing portable
receiving rows and tier gauge. The new proof covers **18** normal
models missing from the previous loose-file map. It neither changes
the default export nor resolves combined-form components, admits
newly playable actors, or enables unverified moves.

The full opt-in export emitted the receiving, indexed and gauge rows
for **206** normal Battle Test packages, including all six nonzero
record ordinals. The existing headless tier-gauge smoke passed on
generated content, with hits **40/32/32** versus **40/32/20** in
the no-reset control. This smoke does not admit every exported actor.

```sh
python3 -m tools.analyze.retail_normal_receiver_factors \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/stock-startup "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-normal-receiver-factors.json
```

### Contact rates through an original joint sample

The [joint replay](api-map/gg4e08-joint-clock-sample.json) attaches
Machine Red's exact clip **0:9**, logical joint **2**, to an
isolated instance. Original HSD code relocates the model and
motion archives, constructs the joint, and allocates the AObj
and its three rotation tracks. Only the copied joint descriptor's
child and sibling links are cleared. This does not load the
whole actor hierarchy or qualify its boot.

The original rate writer does not advance the track. The first
sample uses the requested frame; later samples advance by the
current secondary rate. Both clocks give identical Euler
angles and a local quaternion on the first explicit-frame
sample at **2.5**. The second sample diverges at **2.875** and
about **2.53**, respectively. The relative requests instead
differ from the first sample: **6.375** and about **6.03**.
After expiry, the rate returns to **0.375** and advances from
the held track's current frame, without jumping ahead.

Original `8004B76C` copies the local translation and publishes
the quaternion via `8025ACFC`. All **16 samples** matched across
O0, O1, and O2. The timer seed and lane frame are declared, and
the indexed clip request is a probe, not Machine Red's live
controller choice. No world pose, skinning, rendering, full
frame scheduling, or native gameplay is qualified.

Compile [`retail_joint_clock_sample_fixture.cpp`](../tests/host/retail_joint_clock_sample_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_joint_clock_sample \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/joint-clock-sample "$GF_RECOMP_MODULE" /tmp/opencode/gf-joint-clock-sample.json
```

### Contact sound: strict size bands and shared counter bytes

The [sound replay](api-map/gg4e08-contact-sound-replay.json)
executes the original lookup with playback disabled. Size-indexed
codes move to the next band **strictly above** 1.5, 2.5 and 3.5;
the next representable float above each threshold confirms it.
Code 0 stays on IDs 48/49 regardless of size. Code `0xFF`
selects nothing and does not arm the sound gate.

Original alternation is partitioned by the receiver's source
byte `+0x96`, not by receiver object. Two declared receivers in
the same partition share a counter; a held gate leaves it
unchanged. The counter wraps from 255 to zero. The isolated
special-group probe also found an alias: code `0xC8` at band 2
in partition 0 uses counter byte **22**, as does ordinary `0x88`
at band 1 in partition 1. The selected IDs were **92** and **67**
in that sequence.

In the accepted type-9 pair, holding sound `+0x2AE` did not stop
the visual request, and holding visual `+0x2AF` did not stop the
sound counter increment. All four gate combinations removed
**17 HP**. The isolated sound replay observes the selected ID
left in `r3` by the disabled mixer path, not an audible result.

Five of the 73 scoped actor amount records use special codes
`0xC0` or `0xC8`. Those probes do not qualify live special
routes or extend native sound bindings. All 54 cases matched
across O0, O1, and O2; playback, mixing, live timing, and native
gameplay remain outside this replay.

Compile [`retail_impact_sound_fixture.cpp`](../tests/host/retail_impact_sound_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_contact_sound_replay \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/impact-sound /tmp/opencode/type9-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-contact-sound-replay.json
```

### Contact sound: installed normal receiver configurations

The [normal receiver sound replay](api-map/gg4e08-normal-receiver-sound.json)
checks all **206** normal installations. The config at actor `+0x4B0`
survives the level-1 startup prefix. Calling `80068004` directly
then writes `+0x7FC` from the larger of the two config dimensions
divided by the source constant. The replay checks the exact binary32
result before passing the same actor to the original sound helper.

The installed configurations produce **136 / 26 / 19 / 25** receivers
in bands 0–3. For codes `0`, `0x80`, `0x84`, and `0x88`, all **824**
counter-lane and even-tone checks matched across O0/O1/O2. Each helper
call starts with a clear gate, partition zero and zero counter parity.
The previous sound and stock-startup receipts reproduced byte-for-byte.

The `--contact-audio` extractor now requires this exact normal record
and a matching configuration. In the full opt-in export, all 184
previous bands stay unchanged and **18** more qualify, including
six with a nonzero record ordinal. The four mixed-band receivers
remain unbound. The headless native smoke exercised a band-1 cue
on the newly bound `061B` receiver (record 1), alongside existing
bands 0 and 3. This does not run a full original constructor,
choose live transformed sizes, or qualify audible original playback.

```sh
python3 -m tools.analyze.retail_normal_receiver_sound \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/stock-startup "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-normal-receiver-sound.json
```

### Type-89 exits: cached area key 16

`8016CC24` creates a type-89 carrier with update `8016CD1C`.
The initializer writes row byte 3 to object `+0x17D` at `8016CE48`,
after the carrier's opening bind succeeds. Both exit calls in
`8016D2D0` consume that byte. The bounded rows all select key 16,
whose area uses common group 110, descriptor 150.

The [allocation replay](api-map/gg4e08-type89-area.json) uses exact
normal Nu III key 11 and G-Black key 16. An empty opening shape pool
leaves the byte unwritten. With an initialized carrier, a supplied
contact latch requests the area at `+0x2A0`; the alternate flag
alone requests it at the carrier root. Contact wins if both gates
are set; neither gate means no request. A missing area carrier
or too few descriptors leaves the request visible but creates
no area. Successful requests bind a separate common-group shape.

All 14 cases matched across O0, O1 and O2. This allocation replay
invokes the constructor directly and supplies the exit gates. The
area cue is observed before playback. It does not qualify paid-action
timing, beam damage, or area overlap and HP. No native gameplay changed.

Compile [`retail_type89_area_fixture.cpp`](../tests/host/retail_type89_area_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags, then run:

```sh
python3 -m tools.analyze.retail_type89_area \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type89-area "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type89-area.json
```

### Type-89 timed stage probe

For normal Nu III key 11 and G-Black key 16, both timer pointers
select `80350598`, with phase durations **25, 6, 10**. Each active
update advances the clock by the owner's `+0x1DC8` value. Upon entering
the last timed phase, `8016D2AC` runs the stage-segment test and
`8016D2B0` stores its return at carrier `+0x17C`.

The [timed replay](api-map/gg4e08-type89-timed.json) calls the original
initializer at visit 0, then advances the carrier without seeding its
subphase or either exit latch. A synthetic intersecting wall clips
the root to `(100, 200, 1000)` and sets the stage latch. The next
update requests and allocates key 16 at that corrected root. An
empty stage and an offset-wall miss still execute the exit leaf,
but allocate nothing.

| Owner clock step | Middle phase | Stage probe | Exit leaf | Main phase 2 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 25 | 31 | 32 | 41 |
| 0.5 | 50 | 62 | 63 | 82 |

All 12 cases matched across O0, O1 and O2. These are carrier-update
counts, not game frames. This qualifies the alternate latch producer,
not the actor-contact latch, downstream area scheduling, HP, or full
disposal. The clip result applies to the tested carrier segment, not
to beam visibility or occlusion in live gameplay.

Use the same fixture executable and inputs as above:

```sh
python3 -m tools.analyze.retail_type89_timed \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type89-area "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type89-timed.json
```

### Type-89 roster census: guarded requests and ordered casts

The [type-89 roster census](api-map/gg4e08-type89-roster.json) accounts
for all **17 carrier keys** and **11 direct factory call sites**:
nine literal keys and two reads from a paired selector. It records
**36 static action candidates on 12 Borgs**, including model-guarded
X routes for `0304` and `0306` and the shared key-2 X route for
`0602`, `060a`, `060c`, and `060e`. The two neighboring sibling X
routes `0302` and `0308` do not get those type-89 joins.

The shared phase-two handler `800B8D6C` reads a timed cursor at
actor `+0x6EB`. For `0503` and `050f`, charged B starts with keys
**9 then 10 at the same cursor**, then requests **3, 6, 4, 7, 5, 8**
on separate cursor steps. For `0506` and `0512`, ordinary B uses the
same later sequence without the initial pair. A negative first
byte ends the sequence; a negative second byte suppresses only
the second request. This establishes request order, not simultaneous
allocation or hit order. Opening group 3 has raw power 20 on
`0503`/`0506` and 10 on `050f`/`0512`; those operands are not HP.

Four literal calls remain **unjoined**. The paid model-guarded leaf
`800CBA38` needs index 7 at actor `+0x581`, but the installed X
rows of `0600`, `0608`, `0616`, and `061c` select indices 0–4
and reach a different leaf. The census does not promote those
four model checks to a release claim.

Keys 1 and 15 read phase durations **25, 4, 10**; the others read
**25, 6, 10** across two timer programs. Equal durations do not make
the timer programs interchangeable. This is static evidence; the
timed replay above still covers only keys 11 and 16. No gameplay,
damage, or alternate forms are bound by the census.

```sh
python3 -m tools.analyze.retail_type89_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type89-roster.json
```

### Type-89 indexed B replay: cadence and pool refusal

The [indexed cast replay](api-map/gg4e08-type89-cast.json) runs
`800B8D6C` for all four model guards with a supplied phase-two
actor, cursor zero, clock 2.5 and a clock step of 1. The first
request is on visit **4**, not on visit 3 when the countdown crosses
zero. Each timed step reloads **10**, so the next request is **11**
updates later. The negative-pair visit issues no request and
reaches the original actor-return helper at visit **81** for
the paired bank and **70** for the single-only bank.

With eight carriers and descriptors, the first paired step
allocates **9 then 10**. With just one carrier **or** one
descriptor, it still asks for both but allocates only **9**.
With either pool empty, no carrier is created. Refusal does not
delay the next cursor step. If one carrier and one descriptor are
made available after the refused first step, the next step
allocates key **3** for the paired bank and key **6** for the
single-only bank; the failed requests are not retried.

All 24 cases matched across O0, O1, and O2. These are carrier
requests from an actor update, not input admission, a stock
debit, or actually scheduled carrier updates. The allocated
objects stay uninitialized; no shapes, contacts, or damage were
tested. The replay stops at the actor-return call, not after it.

Compile [`retail_type89_cast_fixture.cpp`](../tests/host/retail_type89_cast_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type89_cast \
  "$GF_RETAIL_ROOT/sys/main.dol" /tmp/opencode/type89-cast "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type89-cast.json
```

### Type-89 opening shapes and timed registration

The [opening replay](api-map/gg4e08-type89-opening.json) takes the
first cast into the original bucket visitor, using each model's
normal group-3 descriptor. Allocation and visitation order are
different:

| Paired cast resource limit | Carrier allocation | Shape bind attempts | Shapes admitted | Later registry |
| --- | --- | --- | --- | --- |
| One carrier or one descriptor | 9 | 9 | 9 | 9 |
| Two carriers, no shapes | 9, 10 | 10, 9 | none | none |
| Two carriers, one shape | 9, 10 | 10, 9 | 10 | 10 |
| Two carriers, two shapes | 9, 10 | 10, 9 | 10, 9 | 9, 10 |

The single-cast controls request only key 3. The paired shapes'
centers reflect different socket offsets, not two hits from one
shape: the fixture sees centers `(800, 600, 300)` and
`(1200, 600, 300)` for keys 9 and 10 respectively.

The initializer writes the exclusion mask `0xFFFF` to carrier
`+0x272`. In the clear-stage, unit-clock replay it clears the mask
on bucket visit **32**, counting initialization as visit 1. The
original shape pass then registers the admitted shapes for that
visit. Visit **33** unlinks them, returns their shape slots, and
requests no area. Shape admission is not attack eligibility.

All 20 cases matched across O0, O1, and O2. The replay does not
schedule input, paid admission, recipients, damage, or full carrier
disposal. Its socket and stage inlets are declared, and the late
transform gets an inert render node rather than retail model data.

Use the same fixture executable as above:

```sh
python3 -m tools.analyze.retail_type89_opening \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type89-cast "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type89-opening.json
```

### Type-89 swept contact and contact-point area

The [contact replay](api-map/gg4e08-type89-contact.json) leaves one
attack shape slot at cast time. It tests key 10 for `0503`/`050f`
and key 3 for `0506`/`0512` against one stationary Sasuke body.
On bucket visit **32**, the registered attack sweeps between the
previous and current positions. The axes differ with the declared
socket pose. The narrow radius is **30**, not the enclosing
broad-phase radius (about 5,030 in this clear-stage setup).

Using Sasuke's radius-22 body, transverse offsets **30 and 51**
from the sweep midpoint hit; **52 and 53** missed. This qualifies
the midpoint comparison, not a general moving-target sweep rule.
The original HP writer removed **17** HP for `0503`/`0506` and
**8** for `050f`/`0512`. Re-resolving the same registry refused the
recorded token without another HP or ledger write.

On visit **33**, actual accepted contact took `8016D30C` and
allocated area key 16 at the saved point. The point was roughly
5,000 units from the carrier root along the tested sweep, so this
was not the alternate root-position branch. Misses made no area
request. The replay ends before the area initializer; it does not
establish area damage, simultaneous recipient order, or native
gameplay. All 16 cases matched across O0, O1, and O2.

Use the same fixture executable as above:

```sh
python3 -m tools.analyze.retail_type89_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type89-cast "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type89-contact.json
```

### Type-5 key-16 follow-up: shape budget, growth, and separate HP

The [follow-up replay](api-map/gg4e08-type89-followup.json) starts
from the accepted type-89 hit on visit **32**. The area attempts
its **group-110** bind on visit **33**, before the original class-4
pass returns the parent's shape. With no spare it refuses that
bind and returns its carrier and two descriptors on visit **35**.
The newly freed parent shape does not trigger a retry.

With a spare shape, the area starts at radius **3.1** with a fresh
token, but is not in the attack registry yet. It uses common
record `802C4988` and removes **17 more HP**, independently of
whether the direct sweep removed 17 or 8:

| Receiver pose after the direct hit | First area hit | Radius at hit |
| --- | ---: | ---: |
| Unchanged, center 22 units from the saved point | 34 | 34.1 |
| Repositioned once, center 100 units from the saved point | 36 | 96.1 |

Re-resolving the area after the hit refuses its recorded token.
The area survives the parent object's return on visit **44**,
peaks at radius **387.5**, unlinks its shape on **50**, and
returns its carrier and descriptors on **83**. Key 16 shares
the reviewed key-0 scale curve but uses a different group and
reaction record.

All 12 cases matched across O0, O1, and O2. This is the bounded
contact-point descendant path, not the wall-root branch or a
claim about other area keys, live match scheduling, or native
gameplay.

Use the same fixture executable as above:

```sh
python3 -m tools.analyze.retail_type89_followup \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type89-cast "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type89-followup.json
```

### Type-32 roster census: carrier and segment ownership

The [type-32 census](api-map/gg4e08-type32-roster.json) resolves the
selector source for all **24 direct calls** to three factories:

| Factory | Update | Bounded rows | Collision path in reviewed code |
| --- | --- | ---: | --- |
| `800E19A8` | `800E1AB4` | 20 × 32 bytes | Model-relative endpoints; timed type-11 child request |
| `801DEB68` | `801DEC90` | 10 × 40 bytes | Borrowed endpoints; carrier bind or timed type-11 child request |
| `80165184` | `8016528C` | 1 × 24 bytes | Separate class-2, three-part route; collision unqualified |

Twenty-two call sites use literal keys and two use model guards.
The keys are local to each factory. Five rows in the model-relative
table have no resolved direct call source. Eight call sites join to
13 static action candidates on 12 normal Borgs; 16 sites have no
qualified installed action join.

For the borrowed-endpoint family, a zero mode byte at row `+0x24`
selects the **carrier's opening bind**. A positive byte instead
allows the timed type-11 request. The signed byte at row `+0x0E`
selects the group in the branch taken. Flying Saucer's normal
X/key 8 is the **carrier-bind** arm, not the periodic segment
arm. Death Head's X/key 16 uses the separate model-relative
family and requests a type-11 child. Both use group selector 1,
but neither the credited hit queue nor the collision owner is
interchangeable.

Type-11 factory `80139A6C` retains both endpoint addresses and
the parent and credited owner separately. Its opening bind uses
the signed child key as the group selector. The census records the
normal source queues without inferring HP, terrain occlusion,
action timing, or native admission.

```sh
python3 -m tools.analyze.retail_type32_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type32-roster.json
```

### Type-32 borrowed endpoints: carrier bind versus segment bind

The [branch replay](api-map/gg4e08-type32-bind.json) compares normal
Flying Saucer `0d03`/key 8 and `0e02`/key 9 inside factory
`801DEB68`. In both cases the constructor publishes a carrier
before requesting 64 bytes of scratch. An empty scratch pool
returns null from the factory but leaves the published carrier
in phase 2; it is not a refusal to allocate the carrier.

With a supplied scratch block, key 8 attempts group 1 on the
carrier itself. If that bind succeeds, the scoped active
callback clears its attack mask and requests no segment. If the
shape pool is empty, the initializer takes phase 2.

Key 9 skips the opening carrier bind. From a declared active
subphase, it requests a type-11 child with both carrier endpoint
addresses. The child attempts group 1 with the original actor's
reaction table, independently of the parent. A missing carrier
blocks creation; a missing shape allows creation but blocks
binding. A bound child samples the first point during init and
reads the changed second point on its next update, so that point
was not snapshotted at allocation.

All seven cases matched across O0, O1, and O2. Successful scratch
allocation, continuous phase timing, registry insertion, overlap,
damage, and the separate model-relative factory remain outside this
replay. No native gameplay changed.

Compile [`retail_type32_bind_fixture.cpp`](../tests/host/retail_type32_bind_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type32_bind \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type32-bind.json
```

### Type-32 borrowed endpoints: terrain writes and effect refusal

The [terrain replay](api-map/gg4e08-type32-terrain.json) uses the
same borrowed-endpoint modes with a real synthetic floor query.
At the declared active subphase, Saucer key 8 passes its borrowed
second point as both query input and output. The floor crossing
changes `(220, -200, 500)` to `(160, 0, 400)` in the actor's
borrowed storage, then the carrier copies the corrected point.

The worker requests a type-9/key-4 effect at the corrected
point, with the credited actor as its parent. If the effect lacks
a carrier or descriptor cell, its
allocation fails **after** the endpoint write; the correction
still stands. Terrain alone does not request the actor-contact
area. This replay stops at effect allocation; the
[standalone type-9 contact replay](#type-9-census-shared-shape-admission-and-contact)
starts at the effect's constructor.

The exact `0e02` key-9 control does not take the key-8 worker,
even when an independent original query hits the same floor.
It binds its type-11 child with the original far endpoint. This
is a distinction between the two reviewed routes, not a
universal terrain policy for type 32.

All six cases matched across O0, O1, and O2. This replay supplies
scratch and the active subphase, as above. It does not qualify
live action timing, actor contact, HP, or native gameplay.

```sh
python3 -m tools.analyze.retail_type32_terrain \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type32-terrain.json
```

### Flying Saucer key 8: carrier contact and independent descendants

The [contact replay](api-map/gg4e08-saucer-contact.json) publishes
normal Flying Saucer's carrier-bound group 1 and resolves it
against a stationary normal Sasuke on a clear stage. The original
amount record `80362C40` has raw power 30; the accepted hit removes
**26 HP** (180 → 154) and adds 26 to both damage ledgers. Resolving
the retained registry again does not apply the same token twice.

The original contact saves `(160, 200, 900)` on the carrier. On the
next callback, a supplied downward endpoint crosses the synthetic
floor at `(160, 0, 400)`. The type-9/key-4 effect uses the floor point
and the credited actor as its parent. The type-5/key-0 area uses
the saved contact point and the type-32 carrier as its parent.

| Follow-up inlet | Terrain effect | Contact area |
| --- | --- | --- |
| Contact, no floor | No request | Allocates |
| Floor, no contact | Allocates | No request |
| Floor and contact, two carrier and three descriptor cells | Allocates first | Allocates second |
| Floor and contact, one carrier or two descriptor cells | Allocates | Requests but cannot allocate |

All six cases matched across O0, O1, and O2. Scratch and the
active subphase are fixture inputs; the contact latch and saved
point come from the original resolver. The requests are observed
from the same saved follow-up inlet. Neither descendant is
initialized, so this does not qualify descendant damage, live
action or reaction timing, or native gameplay.

```sh
python3 -m tools.analyze.retail_saucer_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-saucer-contact.json
```

### Type-9 census: shared shape admission and contact

The [census and bind replay](api-map/gg4e08-type9-roster.json)
account for five direct calls to the type-9 factory and three
to its type-8 predecessor. Type 8 forwards its own key on
initialization and on a separate active branch. Type-76 rows
0 and 2 select type 8/key 0; rows 1, 3, 4, and 5 select type
9/key 1. The two borrowed-endpoint workers request type 9
with key 3 (type-32 rows 0 and 7) or key 4 (row 8). Only the
row-8 Saucer route has a scoped installed action join here.
Key 2 has a table row but no resolved type-9 call key in this
census.

All seven type-9 rows attempt common group **84**. Its sole
descriptor, **122**, is a sphere with raw radius **150**. In the
14 bind cases, the constructor took one carrier and one
descriptor cell. A spare shape cell let the opening leaf bind
descriptor 122 under the credited actor. With no shape cell,
the same leaf returned in phase 2.

The 28 contact cases instantiate type 9 directly under normal
Saucer and complete initialization with empty stage and render
records. Every key published an opening radius of **150**
despite the differing scale operands. Against stationary Sasuke
(radius 22), offsets **0** and **171** hit, while **172** and
**173** missed. A hit removed **17 HP** and added **17** to each
damage ledger. Resolving the retained registry again did not
apply the same token twice.

**Correction to revision 1:** descriptor +4 is amount index **9**,
selecting common record `802C4838` with raw power **20**. The zero
at descriptor +6 is the first step, not an index into `802C4760`.
The original resolver reached `802C4838` before the HP writer.

All cases matched across O0, O1, and O2. The contact runs start at
the effect constructor, not Saucer's terrain callback, and stop
before the reaction tail. The carrier hit and type-5 contact area
still have separate damage paths. The lifetime replay below
checks cleanup without a recipient.

Compile [`retail_type9_bind_fixture.cpp`](../tests/host/retail_type9_bind_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type9_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-type32-roster.json \
  /tmp/opencode/type9-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type9-roster.json
```

### Type-9 lifetime: shape returns before the carrier

The [lifetime replay](api-map/gg4e08-type9-lifetime.json)
advances each of the seven keys in the original actor-relative
bucket. Visit 1 is the initializer. The late transform and
registry passes follow each visit.

| Inlet | Published attack visits | Shape returns | Carrier and descriptor return |
| --- | ---: | ---: | ---: |
| Owner clock step 1 or 0.5 | 1–30 | 31 | 33 |
| No spare shape at opening | None | No shape allocated | 3 |
| Owner set to phase 2 before visit 2 | 1 | 2 | 3 |

In the normal expiry cases, the sphere stayed at radius **150**
and kept the same token through the last publication. At age 30,
the active callback requested shape unbind and ended visit 31 in
phase 2. On visit 32 the effect entered phase 3 while still
holding the carrier and descriptor. The owner-exit guard uses a
different unbind site and dispatches phase 2 on the same visit,
so its first post-exit sample is already phase 3.

All 28 cases (504 bucket visits) matched across O0, O1, and O2.
These are counts of callback visits at two supplied clock steps,
not a measurement of match time or pause behavior. No recipient,
HP, or native gameplay path ran.

```sh
python3 -m tools.analyze.retail_type9_lifetime \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type9-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type9-lifetime.json
```

### Type-8 trails: distance markers survive child refusal

The [type-8 replay](api-map/gg4e08-type8-trail.json) runs keys
0, 5, and 6 through the original actor-relative bucket on an
empty synthetic stage, without a copied target. The parent
uses no descriptor or attack shape. It requests a type-9 child
with its own key at initialization, then requests another when
its distance from the last request reaches the row threshold.

| Parent key | Travel per active visit | Distance threshold | Request visits (including initialization) | Spawn Z |
| ---: | ---: | ---: | --- | --- |
| 0 | 40 | 250 | 1, 8, 15 | 400, 680, 960 |
| 5, 6 | 50 | 200 | 1, 5, 9 | 400, 600, 800 |

Carrier and descriptor shortages did not change the request
points. When a carrier became available just after the first
spaced request failed, the parent did not retry immediately:
it waited for the next distance threshold. The saved spacing
point had already advanced.

Each allocated child initialized in the same bucket visit.
With one shape slot, the opening child kept it; the two later
children allocated carriers and descriptors but did not publish.
Each refused child returned those resources two visits after its
request. The published shapes belonged to the type-9 children,
not the type-8 parent.

All 12 cases (156 visits) matched across O0, O1, and O2. These
are direct constructor inlets, not installed Borg actions.
Homing, obstacle termination, parent expiry, damage from
overlapping children, and native gameplay were not tested.

```sh
python3 -m tools.analyze.retail_type8_trail \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type9-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type8-trail.json
```

### Type-76 roster: paid socket ordinals and separate exit children

The [type-76 census](api-map/gg4e08-type76-roster.json) accounts for
all six direct constructor sites. Two sites join normal action
routes; the other four test specific model constants in a separate
phase-two leaf. The selected Victory Jet `0620` record is spec 2.
That route and the other three guarded constants remain unbound
for active-form ownership and hit queues.

| Normal route | Parent row | Sockets at ordinals 0 and 1 | Opening carrier group |
| --- | ---: | --- | ---: |
| Red Attacker `0d01`, B-shot / B-close | 0 | 2, 3 | 2 |
| Orange Fighter `0d04`, X | 2 | 8, 10 | 1 |

Both normal queues contain raw powers **40 and 10**. The ordinal
selects a socket, not a different queue. The census alone does not
assign landed steps. The exit selector is also separate: after the
contact-latch or stage-probe stop branch, rows 0 and 2 request type 8/key 0;
rows 1, 3, 4, and 5 request type 9/key 1. The zero-HP and expiry
branches call a different helper, not this selector.

The [paid replay](api-map/gg4e08-type76-paid.json) ran the original
pre-callback and supplied phase one at each visit. Red Attacker
debited channel 0; Orange Fighter debited channel 2. In both, stock
was spent and the ordinal advanced even when the constructor ran
out of carriers or descriptors. Restoring a carrier after the
first refused request admitted ordinal 1, not ordinal 0. Empty
stock caused no allocation request and did not advance the cursor.

All eight cases (24 declared visits) matched across O0, O1, and O2.
The replay stops at carrier allocation; initialization, exit
execution, HP, controller timing, and native gameplay remain
unqualified for these routes.

```sh
python3 -m tools.analyze.retail_type76_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type76-roster.json

python3 -m tools.analyze.retail_type76_paid \
  "$GF_RETAIL_ROOT/sys/main.dol" /tmp/opencode/gf-type76-roster.json \
  /tmp/opencode/type76-paid "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type76-paid.json
```

Compile [`retail_type76_paid_fixture.cpp`](../tests/host/retail_type76_paid_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags.

### Type-76 opening: attack and receiver shapes are separate

The [opening replay](api-map/gg4e08-type76-opening.json) carries
both paid routes through the original initializer for both ordinals.
Distinct declared socket translations verify the lookup. With one
shape slot, the **40-power** descriptor publishes a radius-30 attack
sphere, 25 units ahead of the carrier origin. With two slots, the
**10-power** descriptor publishes a radius-55 sphere at the origin
in the **receiver registry**, not the attack registry.

Stationary Sasuke at the attack sphere's center lost **35 HP** with
either one or two slots. Only the 40-power record reached the amount
routine; a duplicate token did not write again. A position inside
only the carrier's receiver sphere did not reach the amount routine.
This assigns the bounded outgoing hit, not any reaction to an
incoming attack on the carrier.

The real contact latch drove the next callback: it stopped the
carrier and requested **type 8/key 0** at the carrier origin.
The request still occurred without a free carrier cell, and both
type-76 shape cells returned in either case. The type-8 child did
not initialize in this replay.

All 24 cases matched across O0, O1, and O2. Socket poses, phase
entry, pool pressure and recipient position are declared. No terrain
exit, incoming carrier damage, live scheduling, alternate form,
whole-move damage, or native gameplay is qualified.

```sh
python3 -m tools.analyze.retail_type76_opening \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type76-paid "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type76-opening.json
```

### Type-76 incoming hits: carrier HP and the zero-HP exit

The [incoming-hit replay](api-map/gg4e08-type76-incoming.json)
uses the same normal parent rows and both socket ordinals. A
separately constructed type-9/key-4 source, credited to Sasuke,
hits the level carrier receiver while Sasuke's body stays 800
units away. This is not a Sasuke action assignment. With only the
carrier's attack shape admitted, the colocated source did not
hit it. The two-shape control 400 units away also missed.

With both carrier shapes present and the source centered on
the receiver, common record `802C4838` reached the writer with
amount **17**. The carrier had **10 HP**, so the write removed
**10**, not 17. Neither fighter's HP or damage ledger changed.
The carrier recorded the incoming token and receive flag, but
its outgoing contact latch stayed clear.

The next callback used the zero-HP helper and requested
**type 4/key 0**, not the outgoing hit's type-8 child. Both parent
shapes returned on that callback. The carrier and its descriptor
returned on the third follow-up, even if the type-4 effect
could not allocate a class-2 cell or descriptor.

All 20 cases matched across O0, O1, and O2. This replay stops the
type-4 effect at allocation; the standalone lifetime replay below
checks its update. Nonlethal hits, incoming reactions after the
writer, terrain behavior, and native gameplay remain unqualified.

```sh
python3 -m tools.analyze.retail_type76_incoming \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type76-paid "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type76-incoming.json
```

### Type-4 key 0: shared effect, no collision publication

The [type-4 census and lifetime replay](api-map/gg4e08-type4-roster.json)
accounts for 33 direct sites in 20 functions calling `8001BDA8`.
All pass literal key 0 and a `+0x20` position pointer. Revision 2
traces **13 sites** to **15 scoped action routes on nine Borgs**.
The other 20 sites remain unassigned.

The new joins pin each constructor's written type tag and update
pointer, then its exact phase-table entry:

| Normal Borgs and installed contexts | Parent | Type-4 branch |
| --- | --- | --- |
| Time Bomber `0106`, B-shot / X | Type 33, key 2 | Zero HP or expiry |
| Machine Blue `0601` and Proto Blue `0617`, B-shot | Type 52, keys 0 and 1 respectively | Zero HP or expiry |
| Blue Striker `0d00`, X | Type 74, key 0 | Zero HP or expiry |
| Red Attacker `0d01` and Violet Attacker `0d05`, X | Type 77, keys 0 and 1 respectively | Zero HP or expiry |
| Antares `0e03`, X | Type 98, keys 0 or 1 | Phase-two exit with `+0x15B` clear |

These are static routes, not new live releases. The type-52 socket
ordinal remains unbound. The zero-HP and expiry branches are not
the separate actor-contact area branches. Powered Gunman's charged
type-15 negative-selector routes and the two normal type-76 exits
from revision 1 are unchanged.

The object takes one class-2 cell and one descriptor. Counting
initialization as visit 1, it selects visual record **67**, switches
to record **23** on visit **9**, and returns both cells on visit
**41**. It never takes a shape slot or publishes collision in this
replay. Owner clock steps 1 and 0.5 gave the same visit counts.

The original initializer used the parent scale of **2** from
retained storage, not the **0.75** supplied at allocation. A later
parent change to **7** did not override the child's curves; measured
scales at initialization, the visual switch, and phase 2 were
**2**, **2.5**, and **3.125**. Marking the parent inactive before
initialization gave the same result while its storage remained
intact. Parent cell reuse was not tested.

All four cases (164 visits) matched across O0, O1, and O2. The
render records were empty stand-ins. This is key-0 lifecycle and
collision evidence, not a retail draw comparison or a claim about
every object tagged type 4. Native gameplay is unchanged.

Compile [`retail_type4_fixture.cpp`](../tests/host/retail_type4_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type4_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/type4 "$GF_RECOMP_MODULE" /tmp/opencode/gf-type4-roster.json
```

### Type-32 model-relative segments: copied aim and live gate

The [shared replay](api-map/gg4e08-type32-shared.json) runs the
original constructor and initializer for Beam Gunner `0104` key 1,
Death Head `0505` key 16, Machine Head `0511` key 19, and G-Red
`0615` key 15. None requires a shape at parent initialization.
Key 1 skips the scratch request; keys 15, 16, and 19 still emit
segments after their scratch request returns null in this replay.

The carrier copies the owner's target pointer at construction.
Changing the owner's pointer did not retarget any carrier. Moving
the original target changed the emitted direction for keys 1 and
15 when the dot gate passed. A target behind the launch axis did
not. Keys 16 and 19 stayed on the declared axis in all four tests;
their row threshold is 1 and the callback requires a larger dot.
This qualifies a copied pointer with live position sampling,
not an unconditional aim lock or a live-match aim cutoff.

At owner clock step 1, the tested emission requests occurred on
active visits **1 and 2**. At step 0.5 they occurred on **1 and 3**.
The first request bound a type-11 shape with the credited actor's
queue; the second could not allocate another carrier. It still
refreshed the endpoint that the first child read on its next update.
Closing the producer byte stopped requests and took the carrier
to phase 2.

All 16 cases matched across O0, O1, and O2. Matrices, target
positions, and the producer gate are fixture inputs. No terrain,
receiver, or damage pass ran. This does not establish wall
occlusion, paid action admission, or native behavior.

Use the same fixture executable as above:

```sh
python3 -m tools.analyze.retail_type32_shared \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type32-shared.json
```

### Type-11 segment contact beyond a stage wall

The [contact replay](api-map/gg4e08-type32-contact.json) follows the
four model-relative examples above into the original class-4 pass.
With no target correction, a type-11 child publishes its near
sphere first, then sweeps to the refreshed far endpoint. Its
narrow radius comes from the shape scale at creation, not the
enclosing broad-phase radius. The stationary Sasuke body hits
one unit inside the sum of narrow and body radii, and misses
one unit outside.

A separate original terrain query hits the synthetic stage
triangle at `z=1000`. With the triangle installed, the segment's
endpoints and HP outcomes match the empty-stage cases: the
on-line receiver beyond the triangle takes the hit and the
off-line receiver still misses.

| Source move | First segment key | HP removed in either hit case |
| --- | ---: | ---: |
| Beam Gunner `0104` X | 10 | 2 |
| Death Head `0505` X | 1 | 3 |
| Machine Head `0511` X | 1 | 1 |
| G-Red `0615` charged B | 27 | 1 |

Only the segment gets the contact latch; the fighter gets damage
credit. The same token cannot hit again. The next segment update
unlinks the shape; the following one returns the segment while
the type-32 carrier stays live.

All 16 cases matched across O0, O1, and O2. This tests one
declared stage triangle and a stationary body, not prop handling,
live aim timing, whole-move damage, or native gameplay.

Use the same fixture executable as above:

```sh
python3 -m tools.analyze.retail_type32_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type32-contact.json
```

### Type-11 segment stream under pool pressure

The [stream replay](api-map/gg4e08-type32-stream.json) runs the
original actor-relative bucket for the carrier and its attached
type-11 children. The producer gate is open for six unit-clock
visits and closes before visit **7**. The stationary body at
`z=1000` is beyond each opening sphere but on the tested sweeps.
The body occupies its own slot, separate from the attacker
shape capacity in the table.

| Spare segment cells | Attacker shape cells | Visits that hit |
| ---: | ---: | --- |
| 1 | 1 | 2, 6 |
| 3 | 3 | 2, 3, 4, 6, 7 |
| 4 | 1 | 2, 5 |
| 4 | 2 | 2, 3, 5, 6 |
| 4 | 3 | 2, 3, 4, 5, 6, 7 |

New segments allocate and attempt their first bind before older
segments return their slots that visit. With three segment cells,
visit 4 refuses allocation even though a cell frees later that
visit. With two shape cells, visit 3 allocates a segment but
refuses its bind before the old shape returns. Neither failure
becomes a retry.

Where both a near sphere and a sweep register, the new near
sphere appears first. Only the sweep reaches this receiver.
Fresh tokens land on distinct visits; replaying each token does
not write HP twice. Closing the parent gate stops new requests
but does not cancel a segment already in flight: the ample-pool
case lands token **6** on visit **7**.

All 20 cases (140 traced visits) matched across O0, O1, and O2.
For the four tested pilots, per-hit loss still matches the contact
replay. This does not qualify re-hit cadence with live recipient
updates, whole-move damage, or native gameplay.

```sh
python3 -m tools.analyze.retail_type32_stream \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type32-stream.json
```

### Type-11 roster census: six parent callbacks

The [type-11 census](api-map/gg4e08-type11-roster.json) resolves
the key source at all **six** direct calls to `80139A6C`.
It records **21 static action candidates across 16 normal
Borgs**, including the existing type-32 segment routes.

| Parent factory | Segment call | Selector scope |
| --- | --- | --- |
| `800E19A8` | `800E20C0` | Type-32 key row cached at `+0x14C` |
| `800EC1F4` | `800EC75C` | Type-71 key 0, child group 2 |
| `8018EEA4` | `8018F384` | Type-55 key 0, child group 8 |
| `801DEB68` | `801DF2B0` | Positive-mode type-32 rows only; Saucer key 8 excluded |
| `801F2BF8` | `801F33A4` | Indexed `+0x13` rows 0–7, all child group 1 |
| `801F3784` | `801F3E80` | Indexed `+0x13` rows 0–7, all child group 2 |

Ultimate Cannon `0c04` requests the type-71 carrier from both
reviewed charged-X entry arms. Its normal group-2 descriptor
contains **raw power 5000**. That is not an HP measurement; the
native beam's authored damage, cost, timing, and geometry remain
unchanged. `0704`'s separate type-55 route starts on the scoped
charged-X cue and selects a four-descriptor normal group 8.

For `0e00` and `0e05`, ordinary X remaps the actor row, while the
reviewed charged-X arm uses ordered row pairs whose negative
bytes suppress individual requests. Both reach the same parent
factory and group 1. `0e03`'s paid B route instead uses its own
direction-selected rows and group 2. The last two factories do
not establish a numeric parent type, and zero rows outside the
reviewed actor inlet are not admitted.

These joins resolve a segment request and a normal queue, not
shape admission, contact, active timing, or whole-move damage.

```sh
python3 -m tools.analyze.retail_type11_roster \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  docs/api-map/gg4e08-code-atlas.json docs/api-map/gg4e08-standalone-installers.json \
  /tmp/opencode/gf-type11-roster.json
```

### Ultimate Cannon type-71 segment: amount versus HP

The [contact replay](api-map/gg4e08-type71-contact.json) starts the
original type-71/key-0 carrier directly. It passes through the
real initializer with no effect scratch, then drives the owner
byte at `+0x6EA`: zero stays in warmup, one enters emission, and
two in the emitting callback sets phase 2 **but still issues that
visit's segment request**. The test supplies one segment cell;
the existing child can read the refreshed far point even without
a slot for a second child.

The type-11 child binds normal group 2. On the first tested
sweep, its raw radius 2000 is scaled to about **133.33**. The
in-line Sasuke body hits; a body outside the combined radii
misses. Missing either the segment cell or the shape cell stops
the path before any HP test.

| Boundary | Accepted overlap |
| --- | ---: |
| Raw power in `802FC380` | 5000 |
| Amount passed to original HP writer | 4307 |
| Receiver HP | 180 → 0 |
| Actual HP removed | 180 |
| Credited / received damage ledgers | +4307 each |

The same token does not write again. The write clips HP to zero
but does not clip the measured ledgers to the receiver's remaining
HP. The replay finishes the HP writer, not KO scheduling. All five
cases matched across O0, O1, and O2. Actor launch timing, debit
policy, live-match damage cadence, and native gameplay remain
outside this result.

```sh
python3 -m tools.analyze.retail_type71_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type32-bind "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type71-contact.json
```

### Type-55 segment: partial shapes and a shared hit token

The [type-55 replay](api-map/gg4e08-type55-segment.json) starts
`0704`'s key-0 carrier directly. With no spare segment cell,
it warms up for **10** callbacks, requests group 8 on visits
**11–69** (59 requests), then enters phase 2 on visit **70**.
Owner clock steps 1 and 0.5 produced the same counts in these
controls. These are callback visits, not game frames.

With a spare segment cell, its four-descriptor group binds in
order **13, 28, 29, 30** until the shape pool is exhausted.
The registry lists the admitted shapes in reverse. Additional
slots supplied before the next segment update do not add the
missing descriptors or retry an empty bind.

The four radius-30 spheres sweep at vertical offsets **30, 90,
150, and 210**. The upper receiver is hit only with the fourth
shape admitted. A receiver between the two lowest spheres
overlaps both, but the original resolver removes **4 HP** once
from the group-8 record (raw power 10). Re-resolving the same
registry refuses the shared segment token without another
HP or ledger write.

All nine cases matched across O0, O1, and O2. The owner pose is
declared and the late transform stops at the renderer write.
The replay does not qualify paid action entry, live hit cadence,
multiple recipients, whole-move damage, or native gameplay.

Compile [`retail_type55_segment_fixture.cpp`](../tests/host/retail_type55_segment_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_type55_segment \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/type55-segment "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-type55-segment.json
```

### Direction-row sweeps: floor area before the sibling segment

The [sweep replay](api-map/gg4e08-sweep-area-routes.json) runs
the original `801F2BF8` parent at row 0 for `0e00`/`0e05` and
`801F3784` at row 2 for `0e03`. With the declared socket poses,
the floor probe runs on visit **35** for the first family and
**34** for the second, counting the initializer as visit 1.

The source clips the terrain probe at `z=6000`. A real crossing
of the synthetic floor requests type-5 key **26** at the
intersection. The sibling type-11 request still uses its
untrimmed endpoints, and clearing the floor or changing pool
pressure does not change them. Those siblings bind the normal
credited group **1** for `0e00`/`0e05` or **2** for `0e03`.

| Inlet on crossing visit | Area | Segment |
| --- | --- | --- |
| Clear stage | No request | Allocated and bound |
| Floor, two cells and two descriptors | Allocated first | Allocated second and bound |
| Floor, one cell and two descriptors | Gets the only cell | Still requests, cannot allocate |
| Floor, two cells and no descriptors | Requests, cannot allocate | Allocated and bound |

The type-5 census now records six scoped action routes on these
three Borgs. These are later floor requests, not direct casts;
Roach's separate placed-carrier route still uses its own
predecessor proof for the same key.

All 12 cases matched across O0, O1, and O2. Parent constructors
and socket poses are declared inputs. The replay stops before
area initialization or segment contact, so neither damage path
or native gameplay is qualified.

Compile [`retail_sweep_area_routes_fixture.cpp`](../tests/host/retail_sweep_area_routes_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags:

```sh
python3 -m tools.analyze.retail_sweep_area_routes \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/sweep-area "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-sweep-area-routes.json
```

### Floor-sweep siblings: shape priority and separate HP

The [follow-up replay](api-map/gg4e08-sweep-area-followup.json)
continues the saved floor crossing through the original bucket
visitor. The floor area allocates its carrier first, but the
segment is inserted before it in the update list. The segment
therefore attempts its shape first.

| Spare carrier cells | Attacker shape cells | Shapes bound | HP removed on next visit |
| ---: | ---: | --- | ---: |
| 2 | 0 | None | 0 |
| 2 | 1 | Segment only | 26 |
| 2 | 2 | Segment, then area | 43 |
| 1 | 1 | Area only | 17 |

The stationary receiver's center is 22 units above the saved
floor point. The area binds there at radius **3.1**, but does not
enter the attack registry until it grows to **34.1** on the next
visit. With both shapes, that registry lists the area first.
Successive resolutions against it write **17 HP** for the area
using token 1 and **26 HP** for the segment using token 2.
Resolving again writes nothing. The area uses common record
`802C4778`; the segment uses its own credited actor record.

All 12 cases matched across O0, O1, and O2. Each accepted hit
finishes the original HP writer, then the resolver is entered
again on the retained registry. This is not a continuous
pair-pass tail, live hit-reaction schedule, or whole-move
damage measurement. Native gameplay is unchanged.

Use the same fixture executable as above:

```sh
python3 -m tools.analyze.retail_sweep_area_followup \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/sweep-area "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-sweep-area-followup.json
```

### Ordinary X: installed program, VM cue, and paid bomb release

The [X-cue replay](api-map/gg4e08-bomb-x-cue.json) starts at the original
ordinary-X handler with state 2, substate 0 declared. The installed class
requests local program group 4. Actor flag `+0x5E0 & 0x40` selects choice
0 or 1; both choices clear a stale cue at entry. No positive cue is
supplied by the fixture.

| Normal owner | Positive cue, both choices | Factory key if paid |
| --- | ---: | ---: |
| Normal Ninja | Phase-one visit 21 | 0 |
| Double Ninja, clone flag clear | Phase-one visit 21 | 1 |
| Sasuke | Phase-one visit 21 | 3 |
| Alpha II | Phase-one visit 22 | 5 |

On that visit the original VM writes `+0x1CEF = 1` and the **same
handler visit** attempts the channel-2 debit. Empty stock still
advances the phase but never reaches the factory. With stock and no
carrier cell, the factory refuses and the debit remains. Masking the
already-started upper lane leaves its cursor at the first wait through
24 visits: no positive cue, debit, or factory call.

The paid constructors use a later supplied logical-key-11 matrix,
not the matrix present at program entry. All 32 cases matched across
O0, O1 and O2. Movement inhibition, unit VM time and the matrices are
declared inputs. The visit counts are not native release frames, and
this does not qualify live controls, pose evaluation, the clone-active
branch, or target-dependent launch. No native packages changed.

```sh
python3 -m tools.analyze.retail_bomb_x_cue \
  "$GF_RETAIL_ROOT/sys/main.dol" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-x-cue.json
```

### Ordinary X bombs: copied aim, live gate, and gravity refit

The [aim replay](api-map/gg4e08-bomb-aim.json) checks keys 0, 1, 3
and 5 for the same four normal owners. It declares the selected
target references, lets the original refresh cache the target point,
and then runs the paid release and launch. Three inputs are separate:

- **At release**, the factory copies the aim point and both references.
  Overwriting the caster's point after release does not turn the bomb.
- **At launch**, the caster's current `+0xC8` reference gates the
  target solve. That solve uses the *copied* point and the *later*
  logical-key-11 socket. Removing the gate produces body-heading
  launch even if the carrier still has tracking. Adding the gate
  after release enables aimed launch without supplying a tracking
  reference. The measured speed is 40; the high-point case reaches
  the 0.8-radian upward cap.
- **In flight**, a separate validity reference can clear tracking.
  Restoring it did not reacquire tracking through visit four. With
  valid tracking, the fourth visit can refit gravity from the
  tracked actor's current point without changing horizontal heading.
  Moving that point before the fourth visit changed gravity from
  the initial 0.3 to 0.15 in the tested case.

All 40 cases matched across O0, O1 and O2. The replay uses declared
target selection and socket matrices and observes four active
visits, not a verified intercept or live lock-on scheduler. It
does not bind a native launch rule or a retail move.

```sh
python3 -m tools.analyze.retail_bomb_aim \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-aim.json
```

### Type-15 bomb requests under pool pressure

The [bomb release receipt](api-map/gg4e08-bomb-release.json) runs
Powered Gunman's original state split at `8016711C`, after the
cue and movement producers. On state 3 it observes **all four**
factory calls in order **8, 9, 6, 7**, whether current stock is one
or zero. In state 2 it sees the ordinary debit instead. With stock
available, that branch reaches `8016D584` and does not request a
type-15 bomb before the cut.

| State-3 capacity | Factory requests | Published in request order | Active list, head to tail |
| --- | --- | --- | --- |
| Four carriers and descriptors | `8, 9, 6, 7` | `8, 9, 6, 7` | `7, 6, 9, 8` |
| Capacity for two in either pool | `8, 9, 6, 7` | `8, 9` | `9, 8` |
| Either pool empty | `8, 9, 6, 7` | None | Empty |

The ordinary Ninja and Alpha II paid leaves are separate
controls. With Double Ninja's extra byte clear, one debit
requests key 1. With it set, that same debit requests keys 1
and 2. A one-descriptor pool accepts the first and does not
refund the round when the second fails. Sasuke likewise keeps a
last-round debit on empty-carrier refusal. Offering Powered
Gunman's key 8 to Sasuke directly consumes neither pool.

All 16 transactions and the wrong-parent control matched across
O0, O1 and O2. The factory reads declared rigid sockets and the
original lists; the fixture runs no later list visit, shape bind,
bomb impact, or effect. It does not qualify the upstream charged-X
cost or cue schedule. No native package changes.

Compile [`retail_bomb_release_fixture.cpp`](../tests/host/retail_bomb_release_fixture.cpp)
with the [phase fixture](#shared-action-gate) flags, then run:

```sh
python3 -m tools.analyze.retail_bomb_release \
  "$GF_RETAIL_ROOT/sys/main.dol" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-release.json
```

### Type-15 first visit: shape priority and registration

The [first-bind receipt](api-map/gg4e08-bomb-first-bind.json) starts
from the verified release paths, then runs the original bucket visitor
and attack registration passes. It uses the exact normal AFS hit
blocks for Powered Gunman and Sasuke.

| Powered Gunman capacity | Published, in request order | Shapes bound, in visit order | Attack registry order |
| --- | --- | --- | --- |
| Four carriers, four descriptors, two shapes | `8, 9, 6, 7` | `7, 6` | `6, 7` |
| Room for two carriers or descriptors, one shape | `8, 9` | `9` | `9` |
| Four carriers, four descriptors, four shapes | `8, 9, 6, 7` | `7, 6, 9, 8` | `8, 9, 6, 7` |

A scarce carrier or descriptor pool favors the earliest **requests**.
A scarce shape pool favors the newest **published carriers** instead.
Each accepted shape is also prepended, so the registry order is not
the order that decided which bombs got shapes.

With no shape cells at the first visit, both pilots enter phase 2
without a shape. Supplying a free cell before visit two does not cause
a retry: the carriers enter phase 3, then return their carrier and
descriptor cells on visit three. Sasuke's paid release keeps its debit.
The registered shapes identify Powered Gunman's group 15, descriptor 8,
raw power 20, and Sasuke's group 1, descriptor 1, raw power 10. These
are opening records, not verified damage amounts or blast effects.

All eight cases matched across O0, O1, and O2. The fixture declares
an empty stage and unavailable visual/effect
resources; it does not run contact, HP writes, impact effects or live
battle scheduling. No native package changes.

Use the same fixture binary as above:

```sh
python3 -m tools.analyze.retail_bomb_first_bind \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-first-bind.json
```

### Type-15 direct contact and follow-up allocation

The [bomb contact receipt](api-map/gg4e08-bomb-contact.json) joins
two exact opening records to the original pair loop and HP writer.
The receiver is a normal Sasuke with 180 HP and one original body
descriptor at a declared pose. A miss or seeded duplicate token
leaves the bomb flying and produces neither an HP write nor a
follow-up request.

| Bomb key and owner | Opening power → HP removed | Child requested on the next visit | Pool entries consumed when the child publishes |
| --- | ---: | --- | --- |
| Powered Gunman key 8 (only one carrier available at charged release) | 20 → 17 | Class 2, type 4, key 0, bucket 25 | One object and one descriptor |
| Sasuke key 3 (ordinary paid X) | 10 → 8 | Class 3, type 5, key 0, bucket 9 | One object and two descriptors |

Accepted overlap records `0x10` on the parent. The next bomb visit
requests the child at the parent's position, retaining separate
immediate-parent and credited-actor links. The shape pass unlinks
the opening shape. If the child cell is unavailable or there are
too few descriptors, the request publishes nothing. Direct damage,
stock, and retirement are unchanged: the parent returns its carrier
and descriptor on the third visit after the hit.

All ten cases matched across O0, O1, and O2. The replay stops before
Sasuke's same-bucket child callback; it never visits Gunman's child
bucket. This verifies allocation, not child shape binding, blast
damage, terrain impact, or live battle timing. Native gameplay is
unchanged.

```sh
python3 -m tools.analyze.retail_bomb_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-contact.json
```

### Sasuke's type-5 area: growth and a second hit

The [area replay](api-map/gg4e08-bomb-area.json) follows Sasuke's
type-15 key-3 bomb into its type-5 key-0 child. The child binds
**group 3, descriptor 4** from `cmn_data.pzz` block 0. This is the
scene's common collision bank, not Sasuke's normal attack bank.
The common reaction record is `802C4778`, raw power 20.

In the unit-scale fixture, the initial **3.1-unit** sphere binds
but fails the original scale gate, so it is absent from the
attack registry. After one growth visit it registers at **34.1**
units, then reaches **387.5** before unlinking. The same receiver
goes from **180 → 172 → 155 HP**: 8 from the direct hit using token
1, then 17 from the area using token 2. Two later checks of token
2 do not write HP again, including one after the original bomb
has retired.

Counted after area initialization, the successful shape unlinks
on visit 17, and the area returns its carrier and two descriptors
on visit 50. If no shape cell is free at initialization, the
area neither requests its cue nor retries when a cell becomes
free. It returns its resources on visit 2, leaving the direct
hit and ammo debit intact.

Both cases matched across O0, O1, and O2. The successful replay
cuts at the sound request, then runs separate bucket and shape
passes. It does not establish live frame timing, terrain-triggered
areas, other owners' area damage, or audio/visual playback. No
native gameplay changes.
The replay initializes the area separately after removing the opening
shape. It does not imply early shape reuse in the original bucket.

```sh
python3 -m tools.analyze.retail_bomb_area \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-area.json
```

### Ordinary Ninja actor hits: unlink request versus slot return

The [same-pass replay](api-map/gg4e08-bomb-actor-area.json) runs Normal
Ninja's key 0 and Sasuke's key 3 through a direct hit, then leaves the
area in the original bucket pass. Both bombs remove 8 HP. Before the
area tries to bind, the parent has requested unlink, but its attack
shape is still linked and its slot is still occupied. Only the
separate class-4 pass returns that slot.

| Spare shape slots when the area binds | Result | Receiver HP after two growth visits |
| ---: | --- | ---: |
| 0 | Bind refuses; no cue and no retry after cleanup | 172 |
| 1 | Area binds; first growth visit removes 17 more HP | 155 |

Actor and floor impacts therefore both need a spare slot at area
initialization. They differ in when the parent is removed: the
actor hit unlinks in the impact visit's shape pass; the floor hit
keeps the shape registered until the following visit. The optional
[native shape budget](borg-gameplay-api.md#static-stage-impacts)
models the allocation order for authored areas, not the original
pool, source body shapes, or continued contact by that parent.

All four cases matched across O0, O1 and O2. No native packages
or gameplay changed. The fixture uses a declared fixed receiver
and stops at the sound request rather than playing it.

```sh
python3 -m tools.analyze.retail_bomb_actor_area \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-actor-area.json
```

### Ordinary Ninja bombs: floor response and same-pass area bind

The [floor replay](api-map/gg4e08-bomb-floor.json) runs Normal Ninja's
key 0 and Sasuke's key 3 from the original paid X leaves, with
declared sockets, no target and a synthetic collision grid. Both
cross the `y=25` floor on visit 35. The original triangle query
corrects the proposed point `(165, 16.25, 1684.49)` to
`(165, 25, 1651.79)`. The type-5 key-0 request uses that
corrected point. Shifting the triangle off the path exercises the
same geometry query but keeps the bomb flying, as does the empty
grid. The separate stage-bounds exit unlinks without an area request.

Unlike the actor-contact route, the floor route leaves the
parent's attack shape registered until visit 36; the parent
returns its carrier and descriptor on visit 37. The area can
initialize in the **same** bucket pass as impact, while the
parent still holds its shape slot:

- With no spare shape slot, the area refuses the bind. It does
  not retry after the parent frees a slot on visit 36.
- With one spare slot, the area binds on visit 35, then enters
  the attack registry at radius 34.1 on visit 36.

All 14 cases matched across O0, O1 and O2. This checks the real
segment/triangle query and the pool handoff, not live Stage
geometry, actor HP on the terrain branch, or target-dependent
aim. The portable Battle Test is unchanged.

```sh
python3 -m tools.analyze.retail_bomb_floor \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-floor.json
```

### Ordinary Ninja floor blasts: growth timing and HP

The [floor-contact replay](api-map/gg4e08-bomb-floor-contact.json)
follows both verified floor crossings through area damage. A
normal Sasuke body sphere (radius 22, 180 HP) is placed at a fixed
horizontal offset from the corrected crossing after the flight
has ended. The still-registered opening bomb misses that body
in all six cases.

| Shape pool and offset | HP after visit 36 | HP after visit 37 |
| --- | ---: | ---: |
| No spare shape, 56 units | 180 | 180 |
| Spare shape, 56 units | 163 | 163 |
| Spare shape, 57 units | 180 | 163 |

With a spare cell, the registered area radius is **34.1** on visit
36 and **65.1** on visit 37. The first radius plus the receiver's
22-unit radius separates the 56- and 57-unit cases. Both owners
use the common amount record `802C4778`: the area takes 17 HP
once, writes token 2 without a prior parent hit, and credits the
original caster. Duplicate checks leave HP and stock unchanged.

The six cases matched across O0, O1 and O2. They verify the
floor branch through HP on this fixture, not moving receivers,
multi-recipient order, live Stage scheduling or native timing.

```sh
python3 -m tools.analyze.retail_bomb_floor_contact \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/gf-bomb-release "$GF_RECOMP_MODULE" \
  /tmp/opencode/gf-bomb-floor-contact.json
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
