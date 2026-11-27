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

The census ties scoped predecessors to 19 Borgs without calling
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
