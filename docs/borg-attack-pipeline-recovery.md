# Borg attack pipeline recovery

Where a contact comes from, traced from the input down to the data, and what
remains undecoded. This is a recovery record, not engine content.

## Where damage is not

Damage was searched for in the two projectile row tables and is in neither. The
56-byte child rows at `802D7B10` for `0f01` key 74 carry `51.8, 40.0, 100.0,
0.75, 15.0`, and the 80-byte rows at `802D9758` for `0f04` key 23 carry `-35,
100, 60, 0.3, 7.5, 25`: speed, lifetime, scale and timing, with no `8` among
them. It cannot be derived from the attack rating either — `0f01`'s ratings are
`[1,2,0,2]`, so its attack stat is zero while its authored damage is 8.

So damage lives in the program/cue path that the projectiles do not describe,
and recovering it means decoding that path rather than reading the row. Until
then every pilot's damage stays explicitly authored.

The static route to that path is now exhausted. The HP writer at `8003D358`
(the `HP - r4` site found during the Roach work) has no `bl`, no `b`, no 32-bit
pointer word in any data section, and no `lis`/`addi`/`ori` materialisation of
its address anywhere in either text section. The direct `+0x1C6` HP writes that
do exist are initialisation sites (`HP = +0x1C4`), such as `8004E318` and
`80081CFC`, not damage application.

The project does have a gated original-execution harness: receipts such as
`build/borg-ordered-audit/retail-0600-rifle-consumer-v3.json` record forked
observations under a 1 GiB address-space cap, an outer timeout and exclusive
creation. That harness can execute a **named** span with supplied operands, and
that is exactly what this case lacks: the damage writer has no static caller, so
there is no root to enter from and no operand context to supply.

Settling the amount therefore needs one capability the harness does not have
today — a dynamic trace of a live battle that records the caller and `r4` at the
damage site for a known attacker and defender. Until that exists, authored
damage stays labelled rather than guessed.

## One dispatcher, one missing subsystem

Melee, shots and breath all reach the same entry. The close-B request that the
knight pilot records as `800FEEA4` is
`8004BEB8(actor, 15, group, choice, -1, -1)`, and `8004BEB8` is the shared
cue/program request: it masks the request against `actor+0x579`, writes
`actor+0x709`, and starts a program object from `actor+0x1D84` on the context at
`actor+0x1CD4`.

So the difference between a sword swing, a bullet and a breath is the program
contents, not a per-family mechanism. `tools/analyze/lane_descriptors.py` already
decodes clip lanes from those programs, but it exposes **no** shape, collision,
hit or damage event.

That is the single missing subsystem behind three separate gaps: the melee
geometry for 91 Borgs, exact damage for all of them, and the attack timing that
the authored window proxies stand in for. Porting the lane-event to shape/damage
path is the next real unit, and it starts at `8004BEB8` and the program at
`actor+0x1D84`.

One concrete obstacle found while trying to start it: `descriptor_lanes` decodes
a per-lane `event` and `flags` byte, but `program_candidate` reduces each lane to
`lane/group/choice` and the export never persists the event. The shared
`shared-action-program-controls.json` stores programs, routes and wait commands,
not descriptor lanes. So before the shape path can be classified, the export
needs a lane-event index — a per-Borg list of `(action group, lane, group,
choice, event, flags, bank)` — which does not exist yet. Attempting it as a
silent enrichment of `program_candidate` produced no observable output, so that
change was reverted rather than left as dead metadata.

The index now exists: `tools/analyze/retail_action_links.py` exposes
`lane_events(roots, mask, group, choice)` and the extractor writes
`shared-lane-events.json`, copied here as
[`lane-events.json`](api-map/lane-events.json). It covers 147 of 206 Borgs and
records, per resolved entry clip, each lane's `group`, `choice`, `event` and
`flags`.

Its first result is a **negative** one, and it redirects the work:

| Group | Most common `(event, flags)` |
| --- | --- |
| `B-close` | `(-1,1)` 311, `(1,0)` 227, `(1,255)` 116, `(9,1)` 48, `(6,1)` 30, `(5,1)` 25 |
| `B-shot` | `(-1,1)` 214, `(-1,4)` 96, `(0,0)` 95, `(-1,0)` 50, `(0,4)` 48, `(3,0)` 37 |
| `X` | `(-1,1)` 67, `(4,0)` 39, `(-1,5)` 26, `(0,0)` 19, `(8,1)` 16 |

The event values overlap heavily across groups, so `event` is a lane action type
(play clip, sound, and so on), **not** an attack class.

The lane **group** does separate the families cleanly:

| Group | Dominant lane target |
| --- | --- |
| `B-close` | local group `1` (582), then local `2` (64), `7` (43), `6` (32), `0` (27) |
| `B-shot` | local group `0` (284), then local `3` (134), `2` (121), `5` (36) |
| `X` | local group `4` (112), then local `2` (36) |
| `X-charge` | local group `4` (88) |

So melee plays local group `1`, shots groups `0`/`3`, and X group `4`. Those
groups are animation bank selectors, though: they choose which clip a lane
plays, not which collision shape an attack uses. That places the hit-shape
request inside the **program bytecode** at `actor+0x1D84`, not in the descriptor
lanes at all — which is the next thing to decode, and the reason this index was
worth building even though its first answer was negative.

## The melee contact opcode

Decoding the exported programs (`shared-action-program-controls.json`, 245
programs: 226 `terminal_wait`, 19 `yielding_loop`) and cross-tabbing opcodes by
action group isolates one:

| Group | `actor_event_10` | `class_event` | `directional_select` |
| --- | --- | --- | --- |
| `B-close` | **85** | 31 | 0 |
| `B-shot` | 1 | 0 | 32 |
| `X` | 13 | 11 | 6 |

`actor_event_10` is the melee contact request: 85 occurrences in close programs
against a single one in the shot programs, which use `directional_select`
instead.

Its encoding is `0a AA BB ff`, four bytes, where `AA` ranges over **0..7** and
`BB` is `00` or `7f`. The `AA` range matches a per-Borg attack slot rather than a
shape id, and `BB` behaves like a side or height flag.

The dispatch path is now traced. The interpreter `8004C050` selects handlers from
`802D0DA0 + opcode*4`; opcode 10's handler is `8004C6B4`, which calls
`8008AC80(actor, AA)` and also stores `AA` at `actor+0x744`. `8008AC80` is the
slot lookup:

```text
owner = *(actor + 0x90)
block = *(owner + 0x3FC)
entry = *(block + 0xA20 + slot*4)   // 8 pointers, slot 0..7
child = 80088AA0(actor, 4, 0x30, 0, 0)
```

So each contact resolves through a per-model array of eight pointers at
`block+0xA20`, and the pointed-to record initialises a 48-byte child.

Following the child initialisation shows what those records hold:

```text
byte  = *(slot_record + 0)          // first byte of the slot record
row   = block + 0xAA0 + byte*80     // 80-byte stride
child+0x28 = row
child+0x0C = 0x8009A65C             // contact update
child+0x14 = slot
child+0x20 = actor
```

The slot record's first byte indexes an **80-byte row**, and that is the same row
format the projectile pilots already read from the shared table at `802D9758`
(speed, lifetime, scale, timing). So melee contact and projectiles share one
attack-row format; they differ in which table and which row they select.

That makes the remaining work a single decode of the 80-byte row fields, plus
finding where damage lives in that row — and it applies to melee (91 Borgs) and
projectiles together rather than to each family separately.

The block itself is placed by `800544A0`:

```text
block = *(actor + 0x9C) + index*7840      // 0x1EA0 bytes per class block
*(owner + 0x3FC) = block
```

So `actor+0x9C` is an array of 7840-byte class-data blocks, selected by a signed
index and installed as `owner+0x3FC`. That is not the actor record — the
extractor's actor stride is 432 bytes — so these blocks are built or copied
separately, and finding where `actor+0x9C` is filled is the last link in the
chain. A second writer, `8003CC48`, installs a pointer from a small table at
`index*4+0x80`, which looks like the simpler class-selection path.

Summary of the remaining decode, in order: fill source for `actor+0x9C`, then
the eight pointers at `+0xA20`, then the 80-byte rows at `+0xAA0` including the
damage field. Everything above that is traced.

## The contact child is visual

One more check changes what binding those rows would buy. The child's update
`8009A65C` is a matrix and transform pass: it calls `8020B238` and `8020AF5C`
for vector maths and submits the child's model through `800076D0`/`800097B4`
using its render object at `+0xE0`. There is no collision or HP write in it.

So the opcode-10 child that carries the 80-byte row is the **swing effect**, not
the hit. The actual contact is resolved by the collision service against the
actor's own shapes, and its damage goes through the unreachable HP writer. That
matches the melee pilots' description of a body proxy: the geometry they stand
in for is real, but the amount is not in the visual path.

Consequently, recovering the `+0xAA0` rows would give the **effect** asset,
timing and scale for melee — genuinely useful for presentation — but it would
not by itself yield damage. Damage still needs the dynamic trace of the
collision receiver described earlier.

## The melee window is authored because the source has none

Decoding the exported close-B programs shows the contact command is the **first**
command in the stream, before any descriptor select or wait:

```text
0x8030dc5c actor_event_10   0a0700ff
0x8030dc60 select_descriptor 0103030104000000
0x8030dc68 upper_controls   0200000000000000
0x8030dc78 wait             00020004
```

So the source raises the contact request at program start rather than at a
timed frame, and the actual overlap is decided by the animated body shapes
during the swing. Summing the waits before the contact therefore always gives
frame 0, which is why the native pilots' `melee=...|9|14|...` window is authored:
it stands in for shape overlap that the program does not describe.

That closes the last cheap route to melee timing. Exact melee geometry, timing
and damage all reduce to the same missing piece — the animated shape data behind
`8008AC80` — and every path to it now ends at either `actor+0x9C` or the
collision receiver.

The rendered model was checked as a last alternative and does not carry them.
Across the roster its PObj types are only `skin` (7,052), `envelope` (2,680) and
`shapeanim` (15); `0001` individually shows flags `a001` (18), `8000` (48) and
`0000` (1) with no third collision type. So the melee contact volumes are not
DObj/PObj geometry in `body.hsd`; they belong to the `owner+0x3FC` class block
that is still unreached.

That makes the order unambiguous for the next pass: the dynamic collision-
receiver trace is the only item that can move melee geometry, melee timing and
damage at once, and nothing left in the static data substitutes for it.

The class blocks were also searched for directly. Their decoded signature is
distinctive — eight pointers at `+0xA20` into the same 7,840-byte block, whose
records' first bytes select 80-byte rows at `+0xAA0` — so both data sections
were scanned for it. The only hits were overlapping sliding windows in texture
and palette regions (`0x802dc998` decodes to colour values `0.046`/`0.747` and
`00FA`-style indices), not class blocks. The blocks are therefore built at run
time rather than stored in `main.dol`, which matches `actor+0x9C` being filled
by a loader.

Do not repeat that scan; it produces false positives on palette data.

The packaged wrapper headers were checked too. `visual/body.prefix` is 256 bytes
and is the `rig-prefix` joint remap — pairs such as `01 0E 00` and `0F 11 0D`
mapping logical joints to JObj indices — not class data. So the class blocks are
not in the DOL, not in `body.hsd`, and not in the wrapper prefix: they come from
another archive member or are synthesised at load, and only the loader tells us
which.

Every packaged file has now been examined. Further progress on melee geometry or
damage needs the dynamic trace, not another guess at an archive region.

## The trace harness exists

`tools/analyze/damage_observer_mod.c` is a read-only ModernGekko mod: a single
`RECOMP_HOOK` entry hook at `8003D358` that prints the target pointer, the amount
in `r4` and the type in `r5`. It patches nothing and writes no register.

`tools/analyze/damage_trace.py` builds it against the sibling checkout's
`external/ModernGekko/include`, refuses a missing runtime, module, header or
`main.dol`, refuses an existing run directory, and runs

```sh
moderngekko-run --game original/extracted   --module build/moderngekko-module/gGG4E08_recomp.so   --mods <dir> --graphics Null --audio Null --user-dir <dir>
```

Two things are established by running it:

- The recompiled original **does** boot headless: `--graphics Null --audio Null`
  completed 12,394,857 native dispatches and 2,359,304,477 cycles in 60 s, where
  `--graphics null`/`software`/`opengl` all failed to initialise and the
  documented `--headless` path aborts on an ImGui assertion.
- The run is bounded and repeatable.

A first version of this runner claimed the observer loaded while reporting zero
observations. That was wrong, and the cause is worth recording because it looks
exactly like a hook that never fires:

- a dynamic mod is only discovered when its file name ends in **`.mgm.so`**
  (`IsPackagedLibrary`); `damage_observer.so` was never enumerated;
- the descriptor `version` must be a **three-part** version (`1.0.0`); `"1"` is
  rejected with `mod rejected: ... invalid mod version`.

The runner now builds `damage_observer.mgm.so`, prints `mod loaded` / `mod
rejected`, and fails if the observer was not loaded. Battle entry is no longer
missing either: the bundled runtime's `--automation-dir` exposes frame-counted
pad input, pause/resume and `read_memory`, and the sibling checkout already keeps
battle savestates, so a recorded battle can be resumed and driven without any
guest-memory write. The remaining work is choosing the right observation point,
not reaching a fight.

Where pad injection would have to attach is now known, and it is not the game
code. The GameCube SI registers sit at `0xCC006400`, so the DOL was scanned for a
`lis rD,0xCC00` / `ori rD,rD,0x6400` pair that would mark the SI driver: there
are **zero** such sites. The module's generated tables likewise name no HLE
entries, and `decomp/symbols/functions.csv` holds six hand-curated rows with no
pad symbols. So the pad path is HLE'd in the runtime, and injection has to hook
that layer or write the guest pad buffer directly — both are runtime work in the
sibling checkout, not in this repository.

Do not scan for SI register materialisation again; it is absent by design.

Two further facts bound the route. The sibling checkout **does** bundle a working
emulator — `local/dolphin/usr/bin/dolphin-emu` (Dolphin 2609, needs
`LD_LIBRARY_PATH=local/dolphin/usr/lib`), and it boots the desktop disc image
offscreen until killed. But its input path (`tools/verify/reference_window.py`,
driven by `keyboard-inputs.jsonl`) injects **host** keys through X11/xcb, and
`Xvfb` is not installed here, so that harness cannot reach a menu in this
environment.

So the remaining options are narrow and all outside this repository: a Dolphin
movie (`.dtm`) played back headless, guest pad writes through the Dolphin GDB
stub, or a runtime HLE hook in ModernGekko. Each needs work in the sibling
checkout before this repo's observer can see a single damage event.

## The runtime already has pad input, memory reads and battle savestates

A cheaper route was under the earlier work all along: the bundled runtime's
`--automation-dir <dir>` is a file-based controller and observation interface.
It consumes command files from `commands/` and rewrites `status.txt` every
200 ms. The commands that matter here are `pad`, `pad_frames` (frame-counted
input on port 0–3), `pause`, `resume`, `read_memory`, `save_state`,
`load_state`, `screenshot` and `stop`. It is the documented controller path,
not a guest-state patch.

Measured with the recompiled original:

- `--graphics Null --audio Null` boots headless. `--graphics null`, `software`
  and `opengl` fail to initialise, and `--headless` aborts on an ImGui
  assertion.
- `pad_frames port=0 frames=45 main_y=1` moved the player from
  `z = -2437.18` to `z = -1937.41`, exactly 500 units, so the pad reaches the
  game's own input path.
- The sibling checkout already keeps battle savestates (`first_battle`,
  `kakeru_battle`, `mana_battle`, `mana_combat_hud`, …). They load into a new
  runtime with `--load-state` and resume the battle.

That removes battle entry from the blocked list. The observation points that
are now known:

- The HUD status object sits at `0x803C5500` with `0x3C`-byte entries. Each
  entry has an HP triple at `+0x34`, `+0x38`, `+0x3C`. Entry 0 is the ally
  (300/300/300) and entry 1 the enemy. Observed values: 540/78/540 in the first
  battle, 100/100/100 in the Kakeru battle, 580 in the Mana battle.
- A scan for `lha → subf → sth` sequences that load a signed field, subtract a
  register and store it back gives 44 distinct sites. The known `HP - r4` site
  `8003D380`/`8003D388` (offset 454) is one of them. Hooking those sites with
  `RECOMP_HOOK` makes the runtime print `mod fallback: chunk [...]`, and the
  hooks do fire, so mid-chunk observation works without a patched runtime.

What is still missing is a **damage event inside the observation window**. The
savestates available here are either at battle start (enemy out of reach for
the tested inputs) or after a combo (the enemy is already at 78 and stops
responding). Replaying the recorded pad stream that followed a savestate is the
next step: the recorded runs reached a WIN screen, so their input stream
contains at least one damaging sequence if it was the automation rather than
manual input that produced it. `tools/analyze/damage_trace.py --state <file>`
now drives a short script for exactly this purpose and reports `mod loaded`,
`mod rejected` and every observer line.

## The damage fields are already pinned by a bounded fixture

`build/borg-ordered-audit/retail-0600-rifle-outcome-v1.py` already executes the
damage tail with supplied operands and asserts the exact write. Its facts:

- the HP field is `actor + 0x1C6` (454) and the previous value goes to
  `actor + 0x1C8` (456);
- the applied amount is also recorded at `actor + 0x41C` (1052) and in a global
  ledger (`state word 1048`), and both equal the amount;
- the write is `HP_after = HP_before - amount`, and the receipt records
  `amount = 8` on its ordinary path (`0` on the binding-mask refusal) with
  `HP_before = 180`;
- the amount itself comes from `8002F108`, which performs three integer
  divisions; the receipt supplies `(3200, 100)`, `(5440, 180)`, `(3200, 100)`
  because it has no live Scene.

So the writer is no longer the unknown. What is missing is a **live hit** that
supplies the native operands, and the observation points are now explicit:

| Address | Meaning |
| --- | --- |
| `8002F108` | shared amount computation (three divisions) |
| `8002F110` | HP tail caller; `r4` is the final amount |
| `8003D344` | HP subtract tail; `r3` actor, `r4` amount |
| `actor+0x1C4` | maximum HP |
| `actor+0x1C6` | current HP |
| `actor+0x3E8` | model ID (halfword) |
| `actor+0x41C` | last applied amount |
| `actor+0x4AC` | actor data pointer |

`tools/analyze/damage_path_mod.c` is a read-only mod that hooks the pair entry
and those three damage entries and prints the caller (LR), actor, model ID, HP
and ledger. It loads, and
the runtime prints `mod fallback: chunk [...]` for the two containing chunks, so
the hooks are armed.

It has now observed a live application. Loading `kakeru_battle`, moving forward
45 frames and then holding boost+forward produced:

```text
damage-path: apply #1 caller=8002F1DC actor=805DDBC0 amount=22 hp=180 ledger=22
damage-path: apply #2 caller=8002F1DC actor=805DDBC0 amount=22 hp=180 ledger=22
```

A longer run continued with contact damage of **1 per frame**, HP falling
158 → 137 and the ledger 23 → 44. Three facts follow:

- `8003D344` is reached with `caller=8002F1DC`, which is exactly the audit
  fixture's `STOP` address for the same tail. The static fixture and the live
  battle therefore meet at the same call site.
- `hp` is read from `actor+0x1C6` and `ledger` from `actor+0x41C`; the ledger
  increments by the amount, matching the receipt's `damage_ledgers` assertion.
- The damaged actor is `0x805DDBC0`. Reading `actor+0x3E8` gives **model 0x0A**,
  i.e. `GG4E08:000A` SASUKE, and `actor+0x1C4` gives its maximum HP, 180. So the
  damaged actor is Sasuke, and it took 22 as a discrete hit and 1 per frame while
  contact continued. Its HUD entry (entry 1, the 100-HP target) never changed in
  the same window, so the HUD copy is not a reliable damage oracle when several
  actors are present.
- The apply's `r7` points at `0x802C7C80`. That is not a move ID: the bytes are
  the float ladder `1.15, 1.1, 1.05, 1.0, 0.95, 0.9, 0.85, 0.8`, and
  `0x802C7CA0` (the receipt's last indexed record) is a second ladder
  `1.0, 0.8, 0.5, 0.25, 0.1, 0.05, 0.02, ...`. They are damage multipliers, so
  the base amount still comes from `8002F108` and the actor stats.
- A control run with the same savestate, forward movement and **no boost**
  produced zero applies over 100 frames. The 22 and the 1-per-frame contact are
  therefore caused by the boost input, not by passive enemy action.
- The pair is the existing table's own case. The actor at `0x805DBDC0` has
  maximum/current HP **200**, and `0x805DDBC0` is Sasuke with 180. The native
  `engine/gameplay/borgs/normal_gred_beam_sasuke_damage_tables.hpp` already
  expects exactly a **credit cap 200** (G-Red) and a **receiver cap 180**
  (Sasuke), so the live hit is G-Red damaging Sasuke, and **22** is the first
  live value that can be compared against that table.
- The pair entry `8002E660` did not fire for this contact, so the hit is not
  routed through the shape-pair scan; it arrives at the reaction tail directly.

The amount arithmetic is now visible. With both chunks forced to the
interpreter, the same hit logs:

```text
division site=CF70 caller=8002F10C r3=0000012C r4=00002580 ...
return   site=D00C r3=00000000
division site=D1B8 caller=8003D00C r3=807469C0 r4=00000001 r5=805DDBC0 ...
division site=D214 caller=8003D00C r3=807469C0 r4=00000064 r5=805DDBC0 ...
return   site=F10C r3=00000016   (22)
```

`8003CF70` is `divw r3, r4, r3` followed by `subfic r9, r3, 32`, clamped to
31. So the factor index is `32 - (r4 / r3)`: live `9600 / 300 = 32`, index 0.
The index selects the first float of the `0x802C7C80` ladder (1.15), and the
base damage times that factor gives the observed **22**. The two later
divisions return 0 and are the other factor rows (force/rank), which is the
same shape as `GRedSasukeDamageTable` (32-entry health/force rows, 3/4-entry
difficulty/rank/tier rows). This is the first live confirmation of that
index formula outside the supplied-operand fixture.

The call that produces the amount is `8003CD5C(credit, ?, receiver)`, reached
from `8002F108`. Its live arguments name the three records:

```text
amount-call r3=80365EC0 r4=80636130 r5=805DDBC0 r6=3F46FE14
            | r5 id=10 hp=180
```

- `r5 = 0x805DDBC0` is SASUKE (model 0x000A, HP 180), the receiver.
- `r3 = 0x80365EC0` is the credit/attack record. It is not an actor — its
  `+0x3E8` halfword is not a model ID — and its words are small operand-like
  values (`0x00140032`, `0x00180000`, `0x001E0064`, …).
- `r4 = 0x80636130` is the carrier/hit object; it holds three pointers
  (`0x8066140C`, `0x806369DC`, `0x80199C0C`) and the usual position/matrix
  floats.

So the full live chain is now:

```text
8003CD5C(credit=80365EC0, carrier=80636130, receiver=805DDBC0)
  -> divw index 32 - 9600/300 = 0
  -> factor 1.15 from 0x802C7C80
  -> amount 22 at 8002F10C
  -> 8003D344 applies it to actor+0x1C6 and actor+0x41C
```

The credit record is not a mystery address: `0x80365EC0` lies inside G-Red's
static action/movement configuration block. `tools/analyze/gred_movement.py`
uses `CONFIG = 0x80365AC8`, the block runs to the program root at
`0x80366220`, and `0x80365EC0 - 0x80365AC8 = 0x3F8`. So the credit side of the
hit is G-Red's own config record, which names the attacker without needing the
pair scan.

That closes the observed chain for this pair: **G-Red's boost/contact on
Sasuke** computes factor index `32 - 9600/300 = 0`, selects the receiver-rank
`1.15` entry, and applies **22** to Sasuke's HP and ledger.

The base amount is also already extracted. `tools/extract/gred_sasuke_damage.py`
guards `read(0x80365EC0, 2) == 20` and calls it the **source beam row amount**;
that is the first halfword of the very credit record the live call passes in.
The extracted GFDA table
(`fb220836…` for this DOL) has the matching factor rows:

| Row | First entries |
| --- | --- |
| CreditDifficulty | 1.02, 1.02, 1.01 |
| CreditRank | 0.85, 0.9, 0.95, 1.0 |
| ReceiverDifficulty | 0.9, 0.9, 0.91 |
| ReceiverRank | **1.15**, 1.1, 1.05, 1.0 |
| ReceiverTier | 1.0, 0.8, 0.5, 0.25 |

So the live value is `base 20 × factor product`, and `20 × 1.1 = 22` matches
exactly. But the full beam formula in
`engine/gameplay/borgs/normal_gred_beam_sasuke_health.hpp` multiplies **all**
nine factor rows (difficulty, health, force, rank, tier) and also applies a
projectile relative adjustment; with the live indices the full product lands
around 17–21, not 22. The boost/contact path therefore applies a **reduced
factor set** rather than the beam's full chain. The shared facts are the base
`20`, the credit record and the `1.15/1.1` receiver-rank row; the exact
per-hit factor selection is still open.

That distinction matters for shipping: the live 22 is a genuine contact amount,
but it must not be fed into the beam table as if it were the beam result.

The move-level name still needs the carrier row, but the attacker, receiver,
base amount, factor row and HP write are all identified.

So the first live amounts exist: **22 discrete** and **1 per frame contact** in
this state. They are not yet attributed to a Borg or move — the apply tail only
receives the target and the amount. The next observation is the pair entry
(`8002E660`) and the reaction inlet (`receive: r3` hit table, carrier, target),
which names the attacker and lets the amount be bound to the shared move rather
than to one battle.

The recorded pad streams, replayed without the observer, produced no HUD HP
change; that confirms the earlier note that the recorded WIN screens came from
manual input, not the automation stream alone. The live damage event above came
from the boost/contact path, which is exactly the shared contact semantics the
native port already models.

- `audio` — the landing candidate (bank, tone, whether the trigger is
  qualified, and whether the PCM file is present), the `other_sfx` state,
  whether a launch sound was authored, and how many action cues and routes
  the shared `shared-action-sfx.json` recovers for that Borg.
- `preview_declaration` — whether a preview is declared.

## Why recovered action cues do not play

`shared-action-sfx.json` recovers cue sets for 13 Borgs (each with a bank, tone,
site and route list), and `tone_candidate` resolves them to a tone record. None
of that reaches the player.

The reason is structural rather than a missing bind: launch sounds are not
played by the simulation. It records `BorgBattleEventKind::LaunchSound` and the
host plays the PCM, driven by a `launch-sound=projectile|path` rule. There is no
per-move sound key and no corresponding event, so a recovered action cue has
nowhere to attach.

Binding them needs four coordinated pieces:

1. a profile key such as `action-sound=move|path`;
2. a new engine event raised on the move's accepted start;
3. host playback mirroring the launch-sound path;
4. extraction emitting the PCM for the 13 Borgs whose routes already resolve.

That is one unit, not a one-line change, and it is the honest prerequisite for
"action SFX" beyond the landing candidate.

