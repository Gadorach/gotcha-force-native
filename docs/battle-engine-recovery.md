# Original battle engine recovery

This note maps the shared original battle engine from the private matching
decompilation and the byte-verified recompiled chunks. It records addresses,
status and architecture only; matching source stays local.

## Evidence available

- The private matching decompilation tracks **13,039 named functions**, of which
  **7,907 are matched** byte-for-byte. Metadata is summarised in
  [`api-map/decomp-battle-inventory.json`](api-map/decomp-battle-inventory.json).
- The recompiled module under `recomp/generated` is generated from the same DOL
  and chunk-verified by the runtime.
- [`api-map/retail-battle-engine-map.json`](api-map/retail-battle-engine-map.json)
  follows 14 curated roots into **213 functions and 412 direct calls**, with the
  dispatch tables it meets.

## The shared engine is a class table, not per-Borg code

The action entry is a single dispatch:

```c
void fn_8006FACC(void *actor) {
    ((void (*)(void *))lbl_802D3B68[actor[0x580]])(actor);
}
```

`lbl_802D3B68` at `0x802D3B68` is the action-class table; `actor+0x580` selects
the class. The first entries are matched handlers:

| Class | Handler | Role |
| --- | --- | --- |
| 0 | `8006FB08` | B-shot family |
| 1 | `80070604` | B-close family |
| 2 | `80071598` | X family |
| 3 | `800716D8` | class 3 |
| 4 | `80071A84` | class 4 |
| 5+ | `8006FB44`, `8006FDB8`, `80070024`, … | remaining classes |

This is how the original handles "all those Borg functions": a shared class
handler table, with per-Borg data selecting the class. It is not a branch on the
Borg ID.

Each family handler dispatches a **second** time on `actor+0x581`:

| Family | Sub-table | Notes |
| --- | --- | --- |
| `8006FB08` | `802D3B7C` | B-shot sub-classes |
| `80070604` | `802D3BE8` if model `0x000A`, else `802D3BD4` | B-close, two variants |
| `80071598` | `802D3C48` | X sub-classes |
| `800716D8` | `802D3C5C` | class 3 |
| `80071A84` | `802D3C7C` | class 4 |

The full map is published in
[`api-map/retail-action-class-map.json`](api-map/retail-action-class-map.json):
32 family entries, 28 matched. The sub-tables contain the small shared state
machines (`8006FB44`, `8006FDB8`, `80070024`, `80070670`, `80070C7C`,
`800710D8`, `80071714`, `80071AC0`, …), and most are byte-matched. A handful of
sub-handlers remain open (`8006FF00`, `80071750`, `80071858`, `80071938`, …).

There is a **third level**: those handlers dispatch again on `actor+0x540`
through phase tables. The generic dispatcher is `800FED6C`:

```c
void fn_800FED6C(void *p, const char *desc) {
    if (p[0x541]) *(int *)(p + 0xCC) = 0;
    lbl_8031EFF0[(signed char)p[0x540]](p, desc);
}
```

Phase tables such as `802D3C2C`, `802D3C14`, `802D3C3C` and `802D3C90` hold the
per-frame windup/active/recovery handlers. The published class map now includes
those phase tables and the dispatcher.

So the original action engine is a **four-byte dispatch hierarchy**:

```text
actor+0x580 -> family   (802D3B68)
actor+0x581 -> sub-class (802D3B7C, 802D3BD4/802D3BE8, 802D3C48, ...)
actor+0x540 -> phase     (802D3C2C, 802D3C14, 8031EFF0, ...)
actor+0x541 -> phase timer reset flag
```

Per-Borg data selects the family/sub-class/phase; the handlers are shared.

### The close-phase state machine

The close family's phase handlers are still open, but their partial sources show
the shared machine:

| Phase | Handlers | Role |
| --- | --- | --- |
| 0 | `80070CCC` / `800706E8` | windup; timer from the actor's 3x3 parameter table; freezes velocity and requests the next program |
| 1 | `80070DE4` / `800707B8` | active; integrates velocity and timer, then advances |
| 2 | `80070F18` | recovery; clears flags and requests the next program |

The fields are stable across both variants:

| Field | Meaning |
| --- | --- |
| `actor+0x540` | phase index, incremented on each transition |
| `actor+0x541` | phase reset flag; writes `actor+0xCC = 0` |
| `actor+0x558` | phase timer |
| `actor+0x868` | per-mode 3x3 parameter table (windup/active/recovery values) |
| `actor+0x584` | mode selector; index is `mode % 3` |
| `actor+0x44/48/4C/50` | velocity components |
| `8004BEB8` | action/program request used at the phase exits |

The native move model currently covers the active volume (melee window) but not
this windup/active-velocity/recovery machine.

**The phase gate is animation-driven, not a tick counter.** The open helper
`fn_80066838(actor, threshold)` compares a transformed vector magnitude against
`threshold * actor[180]` and returns 1 while it exceeds the threshold. The
`+0x868` values (G-Red 1000/1200/1300, Sasuke 1000/800/1000) are displacement
thresholds for the class/mode, not phase durations. The phase advances when the
animated displacement falls below them.

So an `action-phase` tick key would **not** be faithful. The native clip and
duration model stays the approximation until the animation displacement is
recovered. Exact phase timing needs the retail animation/pose data, not another
tick table.

The scene/actor layers use the same pattern:

- `fn_80028264`: `lbl_802CFDE0[scene_state]()` then advances a frame counter.
- `fn_80029164`: `lbl_802CFE3C[actor_state]()` then movement/effect updates.
- Actor update chain: `80028264 → 800288FC → 80029164 → 80029434`.

## Matched versus open

| Range | Functions | Matched | Open | Largest open |
| --- | --- | --- | --- | --- |
| Scene/actor `80027000-8002B000` | 69 | 46 | 23 | `800289FC`, `800297C8`, `80027764` |
| Collision/pair/reaction `8002B000-80031000` | 39 | 14 | 25 | `8002E2A8` (5,672 B), `8002BB14` (3,712 B), `800305D8` |
| Damage `8003C000-8003E000` | 16 | 1 | 15 | `8003CD5C` (amount), `8003C080`, `8003C614` |
| Action `8006E000-80073000` | 132 | 84 | 48 | `8006EE14`, `80072BF0`, `800707B8` |

The **action and scene scaffolding is largely matched**; the **collision and
pair core remains open**. The amount routine (`8003CD5C`) is still an open
decompilation entry, but its behaviour is now translated from the static tables
and validated against two live hits; the pair (`8002E2A8`) and collision
(`8002BB14`) remain unimplemented.

## Native mapping

| Original | Native |
| --- | --- |
| `lbl_802D3B68[actor+0x580]` class dispatch | `moves` selected by button, priority, range and state |
| Shared action handlers | shared melee/projectile/field systems |
| `8003D344` HP writer | `damage()` and actor health |
| Recovered scale rows | `damage-factors` / `damage-vs` / `damage-side` / `damage-chain` profile keys |
| Attacker credit difficulty and rank | `damage-vs` / `damage-side` credit factors |
| Defender class row and receiver rank | `damage-vs` receiver factors / `BattleMvpConfig::damage_scalar` |
| `8002DB58` direct/nearest selection | `battle_pair_select` in `engine/gameplay/battle_pair_targeting.hpp` |
| `8002FD7C` relation filter | `battle_pair_relation_ok` in the same header |
| `8002F8DC` force/ledger | actor `damage_taken`/`damage_credited`/hits, eight-entry history ring, `team_force` and `team_tally` |
| Pair/collision/amount functions | amount and selection translated; the wider pair/collision walk is not yet reimplemented |

The native pair seam implements the recovered order: the direct path collects
every accepted pair, the indirect path keeps the nearest candidate with the
`400000000.0` sentinel and last-wins ties, and the `0.1` sphere gate is shared.
The relation filter (`8002FD7C`) is reimplemented as
`battle_pair_relation_ok` over declared actor/record fields and is applied at
the MVP hit boundary through the `damage-attack-key`/`damage-mask`/
`damage-immunity` content keys. The category dispatch (`80030490`) remains a
`BattlePairFilter` supplied by the caller, so no retail-id branches enter the
engine.

The MVP roster now holds up to six actors. `BattleMvpConfig::declared_actors`
defaults to 2, and `teams`/`teams_declared` assign up to two teams. `step_roster`
takes per-actor pads and package inputs; four-actor spawn, team assignment,
nearest-enemy targeting, idle stepping, a beam hit and a team-elimination win
are tested. The duel
`step` wrapper keeps the two-pad signature. Non-package weapon choice and the
source-contact path still assume the duel's first two actors, and the ledger
supports two teams.

The MVP now tracks the accepted-hit ledger: per-actor damage taken and
credited, hit counts, an eight-entry history ring, a per-team force pool
(`team_force`) and a per-team tally (`team_tally`). The ring writes type 3 for
another team's slot and type 4 for a same-team slot other than the defender;
the duel has only the attacker slot, so it records type 3.

The opt-in `force_policy` / `team_ledger` / `actor_flag3e6` /
`actor_flag1ea` config reproduces the `8002F8DC` mode 0/1/2 force gates, the
`+0x109` counter branch and the last-attacker/receiver writes. The default
policy keeps the simple always-decrement pool. A nonzero defender
`damage-kind` skips the ledger, matching the source's early return, while the
health damage still applies.

The native engine is already data-driven in the same sense as the original: a
package declares moves, buttons and ranges, and shared systems execute them. The
remaining work is to move the **open** collision/pair/reaction logic into shared
native systems, emit the recovered amount rows as content, and keep per-Borg
choices out of engine branches.

## Open core decomposition

The call graph splits the open core into a small amount routine and a broader
reaction fan-out:

- `8003CD5C` (amount, 1,512 B) calls only `80066298` (the model-to-matchup
  index mapper). The rest of the factor math is inline. Both are now
  translated in `tools/analyze/damage_amount_model.py`: the mapper is a
  256-entry pointer table at `802F2E28` (G-Red `0615 -> 4`, Sasuke `000a -> 0`,
  cell `[0][4] = 1.0`). Emitting the eight table roles into packages is the
  next step.
- `8002E2A8` (pair/reaction, 5,672 B) calls 17 functions. The matched ones are
  `800061A8`, `80006C24`, `80019550`, `8002F8D0`, `8002FCBC`, `8003D344`
  (the HP writer), `8013F300`, `80176EC0`, `801DFF24`, `801EC1BC`. The open
  ones are `8002F8DC`, `8002FC18`, `800300BC`, `8003CD5C`, `800DB250`,
  `8010E158`, `8018214C`.
- `8002BB14` (collision, 3,712 B) is the walk above the pair loop. With a
  branch-aware scan it calls 32 functions, including `8002DB58` and the
  `8002C994`–`8002E1C8` group.
- `8002DB58` (pair loop) calls `8002E2A8` (the reaction), `8002FD7C`, `80030348`,
  `80030490`, `80039F6C` and the vector helpers.

The full chain is therefore:

```text
8002BB14 collision walk
  -> 8002DB58 pair loop
     -> 8002E2A8 pair/reaction
        -> 8003CD5C amount
        -> 8003D344 HP writer
        -> matched reaction/effect helpers
```

The collision walk has 68 direct calls in four phases: 14 broad-phase, 7
geometry, 24 apply and 23 post. Its 98 conditional branches are decoded in
[`api-map/collision-branch-map.json`](api-map/collision-branch-map.json):
10 broad-phase, 14 geometry, 27 apply and 47 post, all with a compare or mask
condition (`cmpi` 29, `fcmpo` 28, `extsb.` 21, `rlwinm.` 11, plus `cmp/cmpl`).
Each branch also records the last load/immediate that fed the tested register
(field offset, sign extension, mask, indexed load or call result). The
broad-phase and geometry bands are fully named (24 gates): actor active/timer
2001/global flag 1078, the two u16 peaks and s16 timers, the 6-actor,
12-entry and geometry loops, geometry active/subactive-477, height+475 and the
height limit, the actor+460 vector gates, and the pair pass (actor/flag475/
linked/model table, the 0x1652 table and the f32 order). Float provenance is
the nearest load, not the whole expression. The apply band ends at the pair-loop call
`8002C4CC -> 8002DB58`, which is the entry to the reaction scaffold. The map is
[`api-map/collision-walk-map.json`](api-map/collision-walk-map.json).

The pair loop is **target selection**, not a blind pair walk. Its 32
conditional branches are decoded in
[`api-map/pair-loop-branch-map.json`](api-map/pair-loop-branch-map.json)
(prepare 2, inner 20, select 4, publish 6). The named gates are the
direct-category/record-bit mode, the carrier flags, the `8002FD7C` and
`80039F6C` validity results, the partner checks, the two matrix copies, the
`80030490` category handler (48 categories indexing 15 handlers, bit `0x80`
swaps the argument order; the call site uses `(slot << 3) | record+0x58`), the
direct-vs-nearest choice, the nearest
distance, the aim record/length and the owner callback. Provenance now
confirms the field reads: `[outer+0x88]`, the carrier `node+2` masks, the
record bits at `+0x10`/`+0x12`, receiver `+0x474`, partner link `record+0x24`
plus `+0x83/+0x1DB/+0x3E8/+0x1E4`, team `+0x88`, the aim `lhax` record and the
owner `+0x100` callback.

- outer list `803C477C` (count at `r13-21328`), inner list `803C417C`
  (count at `r13-21330`);
- direct mode (`[actor+88] == 2` or `record+16` bit 0) calls `8002E2A8` for
  every valid pair;
- otherwise it keeps the nearest valid target (distance `8020BC88`, initial
  sentinel `400000000.0`) and calls `8002E2A8` once with that target;
- validity filters: carrier node flags `0x0800/0x1000/0x2000`,
  `receiver+474` bit `0x02`, then `8002FD7C` (different actor,
  owner-invulnerability, same-team flag `0x80`, attack-class flags
  `0x3C0/0xC00`, six-eight-entry immunity keys at `+0x20C`, target mask at
  `+0x272`), `80039F6C` (both radii > `0.1` and centre distance <= radius sum)
  and `80030490` (dispatch `(outer category << 3) | inner category`; `0xFF`
  rejects), with `80030348` running the two `+88` category setup handlers;
- both actor matrices are copied to scratch `803B04D0`/`803B0368`, the aim
  vector is stored at `803B0720`, and an owner callback at `owner+256` runs
  before the reaction call.

So the reaction path is mostly a scaffold of matched helpers around the open
amount and a handful of open per-class effect functions.

### Native roster contact selection

An aim target and a body that intersects an attack are separate. In native
package battles with more than two actors, ordinary direct free-flight shots and pure melee
windows test the existing proxies against all living enemies that pass the
pair-relation gate. The first sweep entry wins; an exact tie keeps the later
actor slot. An ineligible fighter cannot spend a guard or hide a valid hit.
Guards and receiving damage rows use the actual selected body. Tied or earlier
projectile stage/floor contact keeps precedence.

Aim and move-entry range checks still use the existing target selector.
Duel contacts and detached children are outside this direct-attack
change. The native check uses the
existing finite cylinder and melee capsule, not the source's shape-bank
transforms, nearest-distance metric or every-pair mode. The CPU fixture places
the aim target on a raised floor and checks the other bodies, a tied wall,
immunity, allies, and guards.

Main-move sustained beams also use the first eligible body on each
roster damage pulse, within the existing stage-clipped segment. A
recovering or guarding body consumes the pulse; the beam does not
pierce to another target. Relation-refused bodies are excluded before
contact, and a tied wall still wins. Beam length, aim, timing, power
fields, and owner interruption use their existing rules.

### Native roster body motion

Native walking, dashes, air travel, cruise, and grounded forced steps
test the current positions of **all living peer bodies** in a package
roster. Allies and damage-immune actors remain solid. The shared
movement resolver clips each candidate path at the first overlapping
body; this also applies when probing the two axis-slide alternatives.
Vertical separation, arena bounds, terrain queries, and actor visit
order are unchanged. A grounded pull or rewind still refuses instead
of committing a clipped step.

This is the existing native circle-and-height proxy extended to the
whole roster, not the retail body solver. It does not introduce
depenetration or change the checked single-peer path for external
source-motion offers, duels, or legacy non-package battles. Regressions
cover the last slot of a six-actor roster, a vertically separated aim
target, allied contact, a defeated body, and forced-step refusal.

### Native roster fences

The authored electric fence no longer borrows the nearest aim target.
Each owner expires its own anchor sets; each eligible enemy has a
separate inside latch per set. A fighter can leave and reenter without
resetting another fighter's latch. Rejected and guarded crossings do
not queue recoil. The presenter submits each valid segment once, with
storage bounded by the full actor roster, not duel size. This extends
the existing authored fence, not Lightning Robot's retail scheduler.

### Native roster walking pulses

A walking pulse in a package roster tests each eligible enemy against
the existing annulus and height tolerance. It considers the smallest
radial entry fraction within the current expansion slice, with the
later actor slot winning an exact tie. One accepted contact or guard
spends the wave; the other bodies don't inherit the same hit. The wave
origin, cadence, pool budget, and visual lifetime are unchanged. This
does not add moving-body sweeps or retail wave collision.

### Native roster charges and blasts

Landed, supported, armed charges sense the nearest living enemy to
the device's own 3D position, not the caster's aim target. Proximity and
attraction sample that same body centre. Obstructed planar attraction
still refuses, and the sensor retains priority over expiry on a tied
visit. This does not move detonation authority away from the original
owner or change fuse, stock, or support rules.

The sensor and damage query are separate. An `opponent` blast excludes
the whole owner's team, including allies in range; `all` still includes
the owner and allies. These are authored native policies, not a claim
about the retail area object's recipient scheduler.

## Applying the recovered scale to content

The `damage-vs` row matches the **receiver** model, so it belongs in the
attacker's package. The current demo profiles make the mapping easy to get
wrong:

- G-Red's test profile (`g-red`) has only the `beam-test` move; it has no close
  melee move.
- `close-b-test` is Sasuke's move, so adding `damage-vs=10` there would match
  Sasuke-model receivers, not G-Red's opponent.

The correct content step is a dedicated G-Red close move on an unused button
(for example `x`), with `damage-model=10` declared on the Sasuke package (the
motion model is untouched):

```text
move=close-b-test|x|30|grounded|18|0:2
range=close-b-test|145
melee=close-b-test|4|8|115|12|60|24|20
damage-vs=10|0.867|1.13|1.15|1.0
damage-chain=1.0|0.8|0.5|0.25
```

That keeps G-Red's `b` beam behaviour intact and applies the recovered contact
only against the Sasuke package, which now declares damage-model `0x000A`.

The shared package-battle script is **not** a safe place for this yet: it presses
`x` at frames 41 and 126-138, so adding the close move to the shared G-Red
profile would add an unscripted hit and change the verified `shots=2 hits=2`
traces. A dedicated fixture was added instead:

```sh
PYTHONPATH=. python3 -B tests/host/gred_sasuke_contact_fixture.py
```

It appends the close move and `damage-model=10` only inside the temporary
packages, builds the `damage-vs` row from the recovered tables and selector
artifact (it needs the original DOL), and asserts the real host applies the
chained amount:

```text
beam hit tick=23
close hit tick=46 value=18
20 x 0.867 x 1.13 x 1.15 x 0.8 = 18.03 -> 18
```

## Reaction scaffold boundary

`8002E2A8(r3 = attacker side object, r4 = receiver side object)` is the
reaction entry one level above the amount routine. The fields the entry reads
are verified from the static instruction stream:

| Field | Use |
| --- | --- |
| `side+17` | attack-record index |
| `side+44` | attack-record table base; row = base + index * 24 |
| `side+32` | actor pointer |
| `side+36`, `+40` | linked objects used by the reaction helpers |

The amount call at `8002F108` receives exactly:

```text
r3 = [attacker side+44] + attacker side[17] * 24   credit row
r4 = [attacker side+32]                             attacker actor
r5 = [receiver side+32]                             receiver actor
```

It is reached only when `receiver+474 & 0x02 == 0` or `receiver+131 != 0`;
otherwise `r4` is forced to 0 and the call is skipped. The HP writer remains
`8003D344`.

The hit-acceptance exits that branch to the function epilogue are:

| Site | Condition | Effect |
| --- | --- | --- |
| `E34C` | `receiver+131 == 0`, `record+16 & 0x0800`, `receiver+1436 & 0x0800` | OR flags at `+1433`/`+474`/`+1789` from `record+11`; return |
| `E374` | `record+16 & 2`, `receiver+1436 & 2` | return |
| `E398` | `record+16 & 0x0800`, `receiver+131 == 16`, receiver-linked model `770` | return |
| `E72C` | `record+18 & 0x0400`, `receiver+1438 & 0x0400` | return |
| `E738` | same pair, receiver model `2053` | return |
| `E740` | same pair, receiver model `2061` | return |
| `E748` | same pair, receiver model `2062` | return |
| `E754` | same pair, `receiver+1788 != 0` | return |
| `E760` | same pair, `receiver+1184 != 0` | return |
| `E774` | same pair, `receiver+1804 < record+5` | return |
| `E77C` | after storing `record+5` into `receiver+1804` | return |
| `E790` | `record+18 & 0x0800`, `receiver+1438 & 0x0400` | return |
| `E79C` | after setting `receiver+1805 = 1` | return |
| `F51C` | `receiver+131 == 32` (post-effect) | return |
| `F8A0` | `receiver+131 != 0` (end gate) | return |
| `F8AC` | `receiver+1184 != 1` (end gate) | return |

These are the direct branches to the epilogue; the meanings of the flag bits and
the special model ids are not recovered, so they are recorded as numeric gates.

The reaction class pair is verified. `receiver+0x5A0` is a class bitmask and
`receiver+0x71A` the class index. In the amount routine a nonzero mask with bit
`1 << class` set returns 0, which suppresses duplicate damage for a pending
reaction class. The live fixture sets `+0x5A0 = 4` to suppress class 2, exactly
this pair. `8002E2A8` uses the same test before running the reaction branch.

The live pair entry `r3=80681A40 r4=8067FAC0` carried `+0x2C`/`+0x1EC` links to
`80365EC0` and `802D4160`, but the X hit applied `80366058` instead. Those links
are side defaults, not the applied credit; the credit must be read at the amount
call. The 29 reaction-band calls and their decoded argument registers are in
[`api-map/reaction-effect-map.json`](api-map/reaction-effect-map.json). The
matched/open helpers resolve to:

| Helper | Role |
| --- | --- |
| `800DB250` | allocate an effect at the `803B0720` vector, copy `values[0..2]` to `+0x20/+0x24/+0x28`, callbacks `800DB378`/`800DB838`; the nine `8031227C` indices map to definition ids `2,1,1,1,1,1,1,1,2` |
| `8010E158` | spawn effect type `0x15`, copy the entry fields and owner, set `owner+0x8C4` when `entry[1]==1` |
| `800300BC` | reaction direction: modes 0–4 initialize a vector; writes receiver yaw `+0x284` and pitch `+0x282` (can copy to the linked body when `+0x1DA` bit 1) |
| `8018214C` | per-battle reaction-ID allocator, random 1–16 delta |
| `8002FCBC` / `8002FC18` | fixed sound ID 38 / size-indexed impact sound pair |
| `80019550` | visual descriptor selector (`+9`); `802C7ED0` is not an RGB table |

The scaffold itself is ordered in
[`api-map/reaction-scaffold-map.json`](api-map/reaction-scaffold-map.json):
16 pre-amount calls, the amount call and HP write, then 29 reaction and
presentation calls. The first reaction-band call (`8002F8DC`) is the force
ledger. It reads the recipient's cached amount, updates force and actor
totals, and appends a record to each applicable observer's ring. The rest of
the band includes impact sounds and visual effects.

### Hit-history observers

[`battle-hit-ledger.json`](api-map/battle-hit-ledger.json) pins the
`8002FB14..8002FC10` loop. It walks **six physical actor slots**, not a
six-entry history. Each active nonrecipient has a separate
**eight-entry ring** at `+0x464`, with its cursor at `+0x484`. Entries
contain relation `4` for the same player slot or `3` for a different
player slot, plus the actual recipient's physical slot. Only type
`4` advances the observer's same-player amount and count.

Native records the final accepted package-hit amount using its existing
team analogue for that relation. All admitted nonrecipient slots now
observe hits, including previously defeated slots while they remain in
the round. Rings wrap at eight and reset with the round. This does not
change which hits enter the ledger, damage, force gates, credit, or AI.
Roster Battle Test reports the final round's per-slot cursor, ally
totals, and most recent relation/recipient under `--trace-package-battle`.
Relation `0` means the observer has no record.

### Optional ordinary contact audio

[`contact-sound.json`](api-map/contact-sound.json) records the lookup.
`8002FC18` reads attack byte `+10`: `0xFF` is silent; for the reviewed
`0x80`/`0x84`/`0x88` codes, the low group selects a block of four sound
pairs. Code `0` uses one pair regardless of size. The band increases at
sizes strictly above 1.5, 2.5, and 3.5.
`80068004` writes size (`receiver+2044`) as the max of config floats
`+76/+80` divided by the source constant (about 167.76). This is not
sound volume. `800F088C` reads the even ID and uses the low bit of its
counter for the odd ID, then dispatches audio at the hit position.
The fixed helper's size branches all request the same sound, ID 38.

In `--contact-audio`, the converter requires normal receiver records
and agreement across installer size bands (184 qualify). It binds
168 verified direct moves; two zero-power, two special-status and two
unsupported-code moves stay unbound. The native cue uses the shared
PCM mixer and a four-tick per-receiver cooldown. Alternation is
receiver-local rather than a shared retail sound-slot counter. Special
status/model branches, the fixed alternate sound, live source size
switches, spatialization and visual objects are not emulated here.

The native package mixer now consumes the later roster actors' cues as
well as the first two slots. Slot bindings stay fixed through a round;
mirrored actors borrow the same PCM but advance independent cursors.
The shared 32-source block limit still applies, with bound movement
lanes reserved and excess new one-shots dropped in event order.

### Optional ordinary contact art

[`contact-impact-visuals.json`](api-map/contact-impact-visuals.json)
pins the attack byte `+9` dispatch, the receiver's separate visual
gate at `+687`, and the common HSD record references. Selector 1 uses
source slot record 2 or 3 and clones it into two randomized particle
groups. Selectors 2–4 are variants of one family which draws **three**
sublayers: records 144, 145, and 146. These are not one record per
selector. Selector 8 references records 53 and 54; the source can
delay the second layer.

`--contact-art-test` uses the exact local common archive, through
the same static impact presenter used for projectile retirement art.
The reviewed roster binds 164 moves (73 projectile, 91 melee) and
188 single normal receivers. It uses one representative record by
attacking team for selector 1, or three simultaneous records for
selectors 2–4. The static
placement, scale and one-particle reduction are authored; the layered
recipe uses the source lifetime fields as native tick counts. It does
not run retail particle, pose, matrix, or material-curve callbacks.
Six moves with already-authored actor-retirement art (including the
selector-8 Dragon pilots) keep it rather than stacking a second effect.
Zero-power and special-status moves and unqualified receiving records
do not acquire this presentation.

### Optional contact pose hold

[`contact-pose-pause.json`](api-map/contact-pose-pause.json) pins the
attack-record byte `+8` timer. The ordinary hit path writes
`max(existing, byte)` to body `+1820`. `8002E628` tests the attacking
**collision object**, not its credit link, for body kind 0; the
receiving body write is at `8002E6A8` under its own flag gate.
Opcode 10 supplies the fighter itself on reviewed melee routes,
whereas a projectile does not pass a hold back to its launcher.
After decrement at `8005A520`, a still-positive count sets the
two local time factors (`+1524` and `+7624`) to `0.03`. Seed 1 has no
slow tick on this branch.

`--contact-pause-test` converts agreeing seeds to `seed - 1` held
native render ticks for 83 melee moves and 34 projectile moves.
There are 188 qualified receiving bodies. Another 53 reviewed moves
had no slowed tick; the zero-power and special-status routes remain
unbound. This is a **skeletal presentation hold**, not
simulation slow motion or frame-exact retail hitstop. It reuses
the previous sampled pose at the live root while the authoritative
bone and socket samples, reaction angles, move progression, collision,
and stock continue on their existing clocks. Neither unbound
receivers nor borrowed source-motion poses use this approximation.

`800300BC` reads a signed mode at attack-table entry `+0x0E` (call site
`8002F064`). It applies the `(0,0,-1)` fallback when `|v|² < 0.01`, writes
`atan2(x,z)` and `-atan2(y,x*x+z*z)` to the receiver's `+0x284` and `+0x282`,
then subtracts the signed `+0x14`/`+0x15` offsets in `0x100` units. Native keeps
receiver-relative angles and an authored eight-tick upper-body reaction. It
does not copy angles to a linked body.

The engine accepts content modes 0–3 on ordinary moves. For mode 0,
`80089910` and `80089A1C` snapshot the object origin from `+0x20` to
`+0x2C` before its update; the body snapshot
also has a writer at `80056874`. Mode 0 reads current minus previous object
origin. Mode 2 instead reads the attacking pair's `+0x30 - +0x3C` centre
displacement. `8008A728` seeds both centres from the selected world matrix
and the step's local point, and `8008A864..8008A888` rolls the previous
centre and recomputes the current one. Both modes use object `+0x38` only
for zero displacement, before the separate `|v|² < 0.01` direction check.

Ordinary native move projectiles now supply those inputs from the existing
free-flight contact path. Mode 0 takes actual projectile motion up to the
native hit fraction; mode 2 takes the whole centre sweep tested for that
hit. Both use live projectile velocity for the zero case. This is an
analogue at the native proxy boundary, not recovered retail shape poses
or clipping. Melee deltas, sustained rays, fields, detached emitters and
spawned children still use the previous planar caster approximation where
an explicit motion sample is absent. The published [effect
map](api-map/reaction-effect-map.json) pins the producer and consumer sites.

Mode 1 at `80030160..8003016C` subtracts the attacking object's origin
from the receiver object's origin. It follows the pair's object at
`+0x20`, not the credit link at `+0x24`. Mode 3 at `800301B8..800301C4`
instead subtracts the pairs' current geometry centres at `+0x64`.
The ordinary sphere callbacks (`8003A4E0`, `8003A634`) transform the
step point using the pair's selected matrix to populate that centre.
On the guarded nearest-contact route, `8002DE90` adjusts the attacking
object origin and `8002DEB0` separately adjusts its pair centre before
damage; these are not interchangeable fields.

At an accepted native direct projectile hit, an explicit mode 1 uses
the carrier's actual clipped origin and the receiver's body root. Mode 3
uses its clipped proxy centre and the receiver's cylinder midpoint; the
direct melee proxy supplies its capsule midpoint at the authored height
for mode 3. This does not reproduce the source's transformed shape
bank or nearest-contact correction. Unsampled paths and the undeclared
mode-1 default keep the previous body-origin reaction.

Mode 4 reads the third column of a 3×4 matrix: `+0x8DC/+0x8EC/+0x8FC` from
the body or its link, or `+0x11C/+0x12C/+0x13C` from a non-body object when
neither link is a body. The body writer `80046BD0` builds the world root at
`+0x8D4` from actor position, Z/Y/X binary rotations and scale, then copies
it to `+0x114` before the child-joint pass. It has a guarded yaw adjustment;
this is not an animated bone or the aim ray. The [reaction effect
map](api-map/reaction-effect-map.json) pins the producer and reader.

Native mode 4 uses the planar +Z of a scoped body root, not retail pitch,
roll, scale or status pose. Direct melee supplies the same direction used
by its collision proxy. A direct projectile-only move can explicitly bind
`reaction-root=move|linked-owner`: it follows the shot's original round
owner and move, and reads the living owner's current body heading when
the shot hits, not when it launches. If that owner is defeated before
contact, the shot retains its damage behavior but uses the unbound native
reaction default, without the mode-4 offsets. Fields, blasts, spawned
children and mixed moves cannot inherit that root. Values above 4 remain
refused; there is no separately defined mode-5/default vector in the
reviewed source routine.

`8008AC80` stores the attacking object at pair `+0x20`, its link at `+0x24`,
the queue's chosen collision step at `+0x28`, and the linked owner's attack
table at `+0x2C` (copied via object `+0x27C`). The normal `pl####hit.bin` is
7,840 bytes: 32 four-byte queues at `+0xA20`, then 64 steps of `0x50` bytes at
`+0xAA0`. A negative signed queue byte terminates; each preceding byte
selects a step. The low byte of the step halfword at `+4` is copied to pair
`+0x11` and selects a 24-byte table entry. The owner's installer sets the
table, including model-conditional overwrites for e.g. Dragon, Worm and
Machine Head. `8006F5EC` stores `802D4160`, not a global fallback table.
See [`reaction-attack-table.json`](api-map/reaction-attack-table.json) for the
pointer chain and [`reaction-action-map.json`](api-map/reaction-action-map.json)
for the pilot queue inventory.

**Correction:** the controller input context `(choice, sector)` is not the
hit-queue index. For example, `8006F11C` is the type-14 projectile
initializer; it reads the selector from the projectile's factory row, not
an input-context row. [`reaction-projectile-map.json`](api-map/reaction-projectile-map.json)
replaces the earlier speculative context map with the factory operand,
initializer, allocator call and resulting entry. G-Red uses key 43 → group 0
→ step 15 → table entry 0; Sasuke uses key 4 → group 0 → step/entry 0;
Proto King uses key 85 → group 9 → step/entry 9. All three select mode 2 and
offsets `0|24`.

The roster packager uses that chain for reviewed ordinary projectile
allocations (types 0, 2, 14, 15, 47, 49 and 94), including ground pairs,
Tanks, Dragon/Worm shots and the static-flight charge. A move gets a row
only when every launch and every step resolves and agrees on mode/yaw/pitch.
No first-step guess or descendant projectile substitution is used. A fresh
roster generated with the qualified GFBC catalog is exercised by
`tests/host/retail_projectile_reaction_smoke.py`.

The ordinary type-0/49/94 constructors copy the input fighter's `+0x90`
self link into the non-body carrier. `8008AD1C` caches it at pair `+0x24`,
and `800301F4..80030204` follows that link to the zero-tagged fighter's
root. This is not the carrier's `+0x8C` parent, launch basis or flight
direction. The qualified fixture checks the fighter self link, ordinary
carrier pool tag and copy sites in the DOL. The extractor emits the explicit
linked-root binding for the seven agreeing mode-4 shots. Reassignment,
form changes, and neither-link-is-a-body cases are not mapped.

For authored neutral close-B, the existing pose auditors also preserve the
exact selected program addresses. The packager verifies a direct lane-zero
request to `8004BEB8` and lane-zero availability in each audited normal
config (`config+2` seeds `actor+0x579`). A positive signed `AA` from opcode
`0x0A` reaches `8008AC80` at `8004C6F0`. Every direct contact in the bounded
opening program set, and every step in its queues, must agree on
mode/yaw/pitch. The `BB` state operation runs after allocation and is not
part of the mapping.

For 14 pilots the neutral lead-in is pose-only. The verified relay seeds
actor `+0x6EA` to 0. The phase-1 request uses the old byte as a group-3
choice and increments it, so the first continuation at phase 2 requests
`[3,1]` when its guard passes. The two proved request sites are
`8010B5E0` and `801616D4`. The extractor requires each opening program to
remain pose-only and each corresponding continuation to start with an
opcode-10 contact; it keeps all installer roots and rejects disagreement.
This binds the first direct scripted contact, not a live hit, later combo
or indirect effect. The continuation also selects exact local clip `1:1`
from frame 0 on both body lanes for all 14 pilots. Native plays the existing
windup, then switches to that clip at authored tick 6, three ticks before
its tick-9 melee arm. No source timer, blend, collision shape or combo
behaviour is imported; the hitbox, active window and damage are unchanged.

The conversion now emits rows for **91** close-B pilots (77 entry contacts
and 14 first continuations). All select mode 4 with their signed source
offsets. See
[`reaction-melee-map.json`](api-map/reaction-melee-map.json) and
`tests/host/retail_melee_reaction_smoke.py`.

The native pitch bone axis is X; a
rendered `reaction-offset=beam-test|0|64` capture tilts the torso without
yawing it. A rendered comparison is under
`../local/recomp-evidence/contact-sheets/reaction-direction-v1/`: the default
beam hit turns the receiver away from the attacker, while
`reaction-offset=beam-test|-128|0` turns it back toward the attacker. The
per-actor `+0x2AE` impact state is decremented by `8002CE04`; native uses the
recovered 4-tick window to gate the direct hit ring, so the fixture's two
bullets at tick 2 and beam at tick 4 share one ring. The contact sheet also
holds the pitch capture.

The ledger fields are:

| Field | Meaning |
| --- | --- |
| `actor+0x4AA` | applied amount (s16) |
| `actor+0x424` | damage taken total (int) |
| `actor+0x434` | hits taken (byte) |
| `attacker+0x420` | credited damage total (int) |
| `attacker+0x434` | hits dealt (byte) |
| `team_base+team*0x3C+0x118` | team force pool, decremented by the amount |
| `team_base+team*0x3C+0x108/0x109/0x10A` | force flags/count |
| `lbl_80436154[team]` | team tally, `+= 100` on a cross-team hit |
| `lbl_803C4E84 + slot*0x1E00` | six-slot actor array |
| `slot+0x464`, `slot+0x484` | eight-entry hit ring and its index |
| `slot+0x430`, `slot+0x437` | ring total and count |

The amount routine's force/HP rows read per-frame mirrors refreshed at the top
of the collision walk:

- `803B068C[team]` <- `team_base+team*0x3C+0x118` (force current), at
  `8002BB5C/BB74/BB80/BB8C`;
- `803B069C[slot]` <- `actor+454` (current HP) for the six physical slots, at
  `8002BBEC` with a two-byte stride.

The force cap is `team_base+team*0x3C+0x114`; the HP cap is `actor+452`. The branch
conditions inside those bands remain open; the collision walk map is published
as [`api-map/collision-walk-map.json`](api-map/collision-walk-map.json).

## Amount pipeline

The amount function `8003CD5C` is translated in
`tools/analyze/damage_amount_model.py` and exported as
[`api-map/damage-amount-model.json`](api-map/damage-amount-model.json). It has
19 conditional scale stages between two early zero exits and the final
HP rounding:

```text
base (record+0)
  x carrier_relative
  x attacker_1788                                        CE48
  x credit_difficulty(side, settings[attacker+136])      CE94
  x attacker_conditional_half                            CEB8
  x attacker_hp_row(record+6, HP index)                  CF48
  x team_force_row(record+7, force index)                CFC0
  x attacker_rank(attacker+1082)                         CFC8
  x matchup[model_index(defender)][model_index(attacker)] D024
  x defender_1788                                        D0B4
  x defender_class_row(model+156, settings[defender+136]) D100
  x defender_conditional_half                            D128
  x relative(defender+180)                               D16C
  x defender_hp_row_157(projected HP from raw base)      D1EC
  x defender_force_row_158(receiving slot force index)   D260
  x receiver_rank(defender+1082)                         D270
  x tier(defender+1738)                                  D290
  x defender_1504_half / same_player_quarter / scale_divisor_40 flags
  -> trunc(max(1, product))                              D300..D310
```

The two captured hits reproduce exactly:

```text
Kakeru boost  20 x 1.02 x 0.85 x 1.13 x 1.15        = 22.533 -> 22
Kakeru X      30 x 1.02 x 0.85 x 1.13 x 1.15        = 33.800 -> 33
Kakeru X t1   33.8 x 0.8                            = 27.040 -> 27
Kakeru X t2   33.8 x 0.5                            = 16.900 -> 16
Mana           5 x 0.92 x 0.85 x 0.5 x 1.40 x 1.15  =  3.148 -> 3
```

`0.867` is therefore `1.02 x 0.85`, not a stored float. The `1.13` is
`802C6820[16]`, the defender class row selected by `model+156 = 1` and the
per-team settings byte; it is not a battle-wide scalar. The `0.85` is the
attacker difficulty rank at `804335F8`, separate from the `804335E0`
credit-difficulty pair. The D024 cell is
`matrix[model_index(defender)][model_index(attacker)]`; the mapper (the only
callee of the amount routine) is a pointer table at `802F2E28`.

The defender selectors are per-Borg bytes at `plXXXXdata.bin +156..158`
(Sasuke `1,1,0`, G-Red `2,2,2`), exported as
[`api-map/borg-damage-selectors.json`](api-map/borg-damage-selectors.json)
together with the 13 combined forms, whose `data2`/`data3` blocks carry one
triple per component.
The class row is `80433600[side][selector][settings[slot]]`; the HP row
uses `+157`, and the receiving force row uses `+158` with a different
index. The per-slot settings byte is
written by `80006ABC`: state 1 sets slot 0 = `d` and slot 1 = `31 - d` (with
`d` from `*80433934 + 0x46`), masking both cells to five bits and leaving
slots 2 and 3 untouched. State 2 sets slots 0..3 to 7. The observed
defender slot values 16 and 30 correspond to d = 15 and d = 1; that is
consistent with, not proof of, that writer for those battles.

`tools/analyze/damage_content_row.py` turns the tables and the public selector
artifact into native content rows. A battle can pass up to 16 defender models
and receive a ready-to-append `damage-vs` block; the known hit reproduces the
fixture line exactly:

```sh
python3 -B -m tools.analyze.damage_content_row <dol> \
  docs/api-map/borg-damage-selectors.json \
  --attacker-model 0615 --defender-model 000a --attacker-side 0 \
  --defender-side 1 --attacker-settings 0 --defender-settings 16 --base 20
```

```text
damage-vs=10|0.867|1.13|1.15|1.0   product 1.126666   amount 22
```

A second defender uses its own selector; G-Red's class row at the same
settings gives `damage-vs=1557|0.867|1.05|1.15|1.0` (amount 20).

### Paired contact-scale test profile

[`damage-contact-roles.json`](api-map/damage-contact-roles.json) records
an opt-in snapshot: setting 7 on both teams, rank 0. The state-2 arm of
`80006ABC` writes 7 to all four setting bytes. The exporter qualifies that
writer and reads the two credit rows, eight class rows per side, and rank
rows from the DOL. This is an authored choice for Battle Test, not a claim
that the native host follows the retail battle-mode/rank scheduler.

`--contact-scale-test` replaces the older fixed training-target overrides
with `damage-credit` on the attacker and `damage-receive` on the recipient.
For example, team 0 credit is `1.0 × 0.85`; team 1 receivers use class
`1.04` for Sasuke or `0.33` for the class-7 cohort before the common
`1.15` receiver rank factor. Rank indices outside the eight qualified
entries are refused by the analyzer. A receiver row is published only
when its single normal actor record agrees with the published selector.
Combined/multi-record actors do not get a guessed component row.

The native engine combines the factors for direct package move hits
when both actors declare the appropriate role. Unbound receivers retain
the authored base; legacy rows, indirect effects and blast scopes are
unchanged.

The opt-in profile also binds `8003D024`'s directional matchup term.
`80066298` maps the two source models to indices and the multiplier is
`matrix[receiver][attacker]` at `802C5D60`, 20×20. Its recovered branch
requires a live linked attacking fighter and a live body recipient in
their ordinary active phase. The native scoped analogue uses living
actors only, with no retail phase/ID test. The extractor resolves named
groups from the verified single normal records and emits the 26
non-neutral cells as sparse receiver-side bindings. Unknown or combined
groups are neutral. For example, group 4 → group 19 uses `1.1`,
while the reverse uses `0.9`. These 20 matchup groups are separate from
the eight defence class selectors in the receiving role. Live settings
and dynamic rank selection remain outside this test snapshot.

### Optional indexed round selection

[`damage-contact-indices.json`](api-map/damage-contact-indices.json)
exports all 32 setting cells and all 8 rank cells. They have different
inputs: the setting array is indexed by `actor+136`, while
`800561E4..800561F8` initializes actor rank `+1082` from
`Scene[62 + s8(actor+996)]`. The reviewed active-slot setup at
`80173710` writes rank 3. This does not establish a rank scheduler for
other modes; the table side flag at `actor+998` is also a distinct
operand in the source.

`--contact-index-test` includes the complete curves for the optional
portable round selection. The host takes settings by native team and
ranks by actor slot. It replaces just the fixed difficulty/class and
rank factors on the existing split-role path. Without a selection,
the generated setting-7/rank-0 snapshot still applies. Indirect hits
and unqualified receivers do not get the new scaling. Native does not run
the source scene writer, rank scheduler, or extra conditional amount
flags. The indexed test covers 206 credit roles and 188 verified
normal receiver roles.

### Optional receiving HP and force ladders

[`damage-receiver-ladders.json`](api-map/damage-receiver-ladders.json)
pins the two 32-cell lookups. `8003D1EC` uses the receiver's selector at
`+157`. It subtracts the literal attack base from mirrored HP, floors
that probe at zero, skips the lookup above 200, and otherwise indexes
`min(31, 32 - floor(32 * probe / min(maxHP, 200)))`. The signed multiply
constant `0x51EB851F` with high-half and shift 6 divides by **200**, not 25.
This is not the HP after writing the scaled damage.

`8003D260` instead uses `+158` and the receiving slot's force ratio,
even when the HP term was skipped. On the opt-in native path
(`--contact-ladder-test` with `--contact-scale-test`), it uses the actual
receiving team's force ledger **before this hit**. An untracked pool is
neutral. A projectile snapshots its base at emission, before any native
power-field buff; the HP probe uses that base. Both factors are applied
between receiver class and receiver rank. Only verified normal
recipients on split-role direct hits receive these curves. No source
settings scheduler, active-component inference or force policy is added.
This uses native per-hit state, not the retail collision walk's frame-wide
mirror ordering.

### Optional outgoing HP and force ladders

[`damage-attacker-ladders.json`](api-map/damage-attacker-ladders.json)
records `8003CF48` (attack-record `+6`) and `8003CFC0` (`+7`). The former
uses current attacker HP before the hit, with the same 200 ceiling and
divisor cap as the receiver lookup but **without** subtracting the base.
The latter uses attacking slot force divided by its cap. Both are read
before attacker rank, with an active linked fighter in the reviewed
source path.

With `--contact-ladder-test`, the current audited cohort resolves both
selectors to 0. The extractor binds that shared pair only to the
**172 positive-power direct routes** (including the two melee moves
whose raw bases differ but whose selectors agree). The two zero-power
Worm routes stay unbound. Disagreeing selectors or different qualifying
pairs within a package do not pick a winner. Native uses the originating
move, the living attacker's HP and its pre-hit team force, not the
attacker's current action or a detached child's state.

### Optional receiver-owned contact tier

[`damage-contact-tiers.json`](api-map/damage-contact-tiers.json) records all
64 factors in `802C7CA0`. In the reviewed normal receiver path, `8003D290`
uses the **existing** tier (`actor+1738`), unless attack-record `+18` has
bit `0x4000` set. The hit handler then adds the unsigned byte at record
`+5` to the receiver's meter (`actor+1736`). At 100 or more it clears the
meter (discarding overflow) and raises the tier once, to a maximum of 63.
The counter belongs to the receiver, not to a pair of combatants. The
source refreshes a 60-unit timer and subtracts the receiver's update step
until expiry clears both fields. The handler also has a separate
stagger-gauge reset branch before adding charge.

`--contact-tier-test` binds only verified direct routes with unanimous
charge and scale operands. The reviewed roster binds 172 moves
(81 projectile and 91 melee) and 188 normal receivers. All bound moves
use the scale path, including two with a valid zero charge. The two
zero-power Worm routes remain unbound. The test advances the idle
timer once per native battle tick. Unbound and indirect hits do not
advance the meter. This policy is separate from `damage-chain`.

### Optional contact-tier reset gauge

`80056414..80056428` initializes the gauge and its capacity from the
signed halfword at offset zero in the active actor data record. The
shared hit handler subtracts attack-record byte `+4` from that gauge at
`8002F3C0..8002F3CC`. At zero or below it refills the gauge, clears the
tier and charge, and then processes byte `+5` for the same hit. Damage
was already calculated at the old tier. The handler raises bit `0x80`
at `actor+1789`, which suppresses further wear while set but not charge;
the collision walk clears the byte at `8002BC74`. The separate 60-unit
idle timer at `actor+1668` also refills the gauge without forcing a tier
reset.

`--contact-tier-reset-test` binds the capacity only where the installer
proves it selected the single normal actor record. The qualified
receiver set has 180 capacities of 500, seven of 700, and one of 30000.
All 172 eligible direct moves agree on wear across their selected steps.
Native scopes the wear gate to one receiver reset per battle tick
and uses a separate native idle countdown. It does not replay the retail
collision walk, phase gates, stagger action or scheduler.

### Optional one-HP contact floor

`8003D300..8003D310` clamps the final product to at least 1 **before**
truncating toward zero. A zero base returns at `8003CDB4`, and the
body recipient's indexed suppression mask can return zero at
`8003CDEC` before scaling. Neither reaches the floor. The conditions
at `CEB8` and `D128` use the outer `Settings+50` state (0 or 1), not
setting index 0. `D2A4` halves for a **set** receiver flag;
`D2FC` checks the actual recipient kind, not the attacker.

`--contact-floor-test` adds the floor to 172 qualified direct moves
whose audited source bases are all positive. The two moves with mixed
positive source bases can keep authored HP power while still using
the floor; the zero-power Worm routes stay unbound. Native applies it
after all active scale stages on those direct moves, without replacing
its relation filter, adding the source suppression mask, or importing
any of the conditional state/flag multipliers above.

### HP base of an exact direct route

The pair's attack index (`+0x11`) selects the 24-byte record passed by
`8002F108` to `8003CD5C`. `8003CD88` loads the unsigned HP base at record
`+0x00`. `8002E3C4` reads the different halfword at `+0x02` for an earlier
contact calculation; do not use that value as HP damage.
[`damage-contact-power.json`](api-map/damage-contact-power.json) records
the distinction for representative projectile and melee routes.

`--contact-power-test` populates `contact-power` from that first halfword
only if all qualified steps and siblings agree on one positive value.
The current roster yields 81 projectile and 89 melee overrides. Two
melee routes mix 30 and 40, while two Worm orb routes have zero HP base;
those four keep their authored behavior with an explicit refusal in the
receipt. The option does not bring unsupported contact effects or
collision geometry into the runtime. It can be used with or without
the separate contact-scale test snapshot.

## Per-actor source movement

Source-owned motion now has a per-actor engine path:

- `step_roster` takes a `source_offers` span (one pointer per actor);
- the validator is `prepare_source(index, offer, actor, other, pad, other_pad,
  contact)` and runs for every offered actor;
- contacts, receipts, producer bindings and `observe_motion`/`motion_receipt`
  are indexed by actor;
- the step refuses any source package that has no offer instead of falling
  back to legacy motion.

The view holds maxima-sized Borg/effect slots and accepts a package span. The
host now runs the four-actor roster in both variants (`--demo-roster-battle`,
teams 0/1/0/1):

- authored packages;
- a G-Red/Sasuke source roster with two demos, per-actor offers and one
  batched source publication. `allow_multi_source` admits the second
  `NormalGRed` package; the single-source duel keeps the stationary-opponent
  admission rule.

The host drives the second G-Red with a forward AI pad through a per-actor
camera frame, and gives actors 2/3 periodic B attack input. The roster sets
`source_allow_active_opponent`, so the source validator no longer requires the
opponent to be idle or pad-free; authored opponents can use ports 1/3. The host smoke
asserts the authored roster `actors=4 ... shots=2 hits=2 reactions=2` and the
source roster `actors=4 ... shots=5 hits=4 reactions=4`; `reactions` counts
hits that opened a reaction window, so it verifies the new direction stage in
the real host. The roster demo's frame-85 restart is driven by P2's Y/N, so the
smoke also guards the second local player's session control. The same smoke passes with `--opengl` under
`SDL_VIDEODRIVER=offscreen`, and the 4-actor source roster renders 150 frames
with the same counts and `reactions=4`. The HUD draws
one bar per actor, and the camera frames actor 0 against its nearest enemy.

The [explicit package roster](borg-gameplay-api.md#choose-a-roster)
uses the same host with up to six individually admitted actors. The
first two slots are local by default; a preset may keep just P1 local.
CPU slots use an authored input planner: ready B first,
X as a fallback, an approach based on the declared range and
melee or damaging-field reach, simple hops for air-only attacks, and
held/released samples for charge windows. Read-only stock checks cover
entry gates, the first projectile release cost and attacks whose only
payload is a paid field. The field marker still owns its debit, and an
empty field pool does not suppress another payload on the same move.
A grounded roster CPU can step out when a confirmed hostile field hit
interrupts its melee windup before the window arms. It uses ordinary
walking, releases the interrupted input, and avoids returning through
that same active field. On a stage that admits CPU aim hints, it can
pursue another grounded rival along a clear path outside the field.
It waits outside if there is no such path and another damage pulse
is pending. It can re-enter during the harmless tail of the field;
the native world removes a field before damage on its expiry tick.
Damage, interruption, field lifetime, and movement acceptance still
belong to the native simulation.
A melee-only CPU can also notice a stationary hostile field or a
plain owner-following field carried by the rival it is approaching.
It uses the current footprint only, if the direct approach would
enter it before reaching strike range and a pulse remains. A ready
strike takes precedence. Drifting fields, moving sockets and fields
carried by other fighters still require a confirmed hit before the
CPU reacts to them.
Simple slow ground drops with an opponent-only expiry blast use the
declared blast radius for a finite flat-floor approach. The planner
discounts body-centre height and does not count launch offset or
forward travel as extra reach. This only applies when speed times
lifetime is at most a quarter of the blast radius; it is not a
landing-point prediction. Thrown, guided, remote, and mixed payloads
keep their existing behavior. Actual landing, blasts, and stock
returns remain in the ordinary projectile system.
A grounded move-charge that carries only an owner-following field can
start from farther out. The planner checks the projected body path
and field reach at the first damage pulse, without extending an
authored entry range. Floor and body collisions, field placement,
guard, and damage still use the normal live simulation.
A simple field-only attack can hold fire if its aimed rival is at a
sustained height outside the damage core, unless the field could reach
another enemy nearby. Ordinary jumps, socket-driven fields, and moves
with other payloads are not refused by this check. Aim and field
recipients remain separate.
Empty held weapons stop receiving the held bit. Direct melee planning
checks the native height spans and can ask for an ordinary hop for
air-capable attacks. It does not forecast
powered lift or replace the actual active-frame overlap. The planner does not bypass stock or
collision, recover retail AI, or alter the duel bridge. The shared camera
fits living collision proxies together with each current posed body
mesh. It falls back to the proxy when pose bounds are unavailable and
keeps the direction set by the two local spawn positions. It does not
fit attached weapons or effects. Both local pads use that view's
movement basis; CPU inputs stay arena-relative, and the camera never
forces either Borg's body heading. The roster HUD groups health and KOs
by actual team instead of using duel bars. It also reads each local
fighter's live stock pools, deduplicating B/X uses without estimating
a reload deadline or modifying stock. The local movement readout keeps
powered lift, jump points, shared aerial actions and cruise boost
separate; it does not replace the borrowed source-motion meter. An authored stage
supplies one validated marker per actor and uses the same floor,
projectile collision, and renderer as an authored duel. Borrowed source
motion remains on its separately checked duel and mirrored-demo paths.
The shared roster viewport evaluates authored obstacle fades for
both living locals, with one fade state per obstacle and viewport.
This does not change stage collision or recover retail visibility.
Roster presets may put both locals on team 0 for co-op. With allied
locals or a single local, the camera uses the opening opponent placements
for its direction,
and the existing team result logic keeps play active when only one
local fighter has been defeated.
Optional first-to-N presets count completed native rounds from the
current battle scope only. Q/N retains the score until the match is
clinched, then starts a fresh series. This is session bookkeeping, not a
retail event scheduler.
Optional roster clocks count accepted simulation ticks and settle only
the standalone round: resolve native KOs before comparing the remaining
team HP fractions at expiration. Story and Challenge timeout semantics
do not change.
An authored stage can also opt in to full-height planar body partitions.
These use the shared native movement sweep, including dash and forced-step
acceptance. They are neither inferred from projectile grids nor a recovery
of the retail solid-geometry rules.
Individual partitions can also flag direct melee occlusion. The native
probe filters actual contact candidates before applying guard, damage,
or credit. This is a centre-line check against the authored footprint,
not weapon-mesh collision or a retail wall-effect callback.
For CPU intent, a blocked padded body path does not veto a ready direct
strike on the nearer rival if that bare melee line is clear. The CPU
holds position for the strike instead of taking a detour or changing
rivals. A flagged partition across the strike line still refuses it.
Short-range roster CPUs use the same planar sweep to find routes around
those footprints. On a long, clear leg, the planner can request an
ordinary grounded dash if its full authored travel fits before the
corner. Short legs, sealed routes and dash-disabled packages do not
get that request. The planner supplies inputs only; floor, other
bodies and actual dash acceptance remain in the battle engine.
For a direct close-range approach, the burst must leave room to slow
down near the rival, and a ready attack takes priority over dashing.
A slow walker can extend a forward dash straight past a corner
waypoint when walking that leg would take at least four dash durations.
The full dash must fit over flat support, clear walls and the current
bodies, and shorten the remaining route. The planner holds the checked
stick direction; the mover still owns the dash duration and cooldown.
If a living ally at body height blocks a grounded CPU's direct approach
or a leg around a partition, the planner checks two short lateral legs
and either sidesteps or waits. If neither full-width probe fits, it
can check a shorter step away from an off-center ally. This does not
change the target or grant passage through the ally; native contact
still resolves actual movement against all solid fighters.
A planted roster CPU can turn at the normal angular rate when its
stick requests movement but a solid peer prevents translation. The
turn uses the stick direction, not the aim target. It does not move
either body, apply during a hit reaction, or alter local controls.
Short-range continuous-cruise CPUs also route around full-height
partitions. They only request a new boost when aligned on a clear leg
with room and meter for the authored minimum. A short corner leg can
extend straight past a waypoint if the whole boost fits on flat
support, clears walls and bodies in the current snapshot, and shortens
the remaining route. The CPU keeps that stick direction for the
minimum boost; the normal movement controller still owns thrust,
collision, spending, release and recovery. The CPU does not climb over
the partition or change the package's cruise height.
When an authored partition blocks the approach to the nearest enemy,
the CPU normally prefers a clear rival, except when a valid detour to
the nearer rival is less than half the clear walking distance. When
neither approach is straight, the CPU compares valid route lengths at
the usual replan points, not every tick. A one-body-radius preference
favors the current route, or the nearest rival on the initial plan, so
a small difference doesn't flip sides.
On these stages, CPUs limited to
direct melee or simple damaging fields can also pursue a lower rival
if a cruiser holds height beyond their attacks. A field that already
covers another enemy keeps its original aim. Briefly airborne targets
and ranged fallbacks do not trigger the height-based switch. The
choice is a validated one-tick hint, not an override of the nearest
body, contact, guard or credit.
A ground melee-only CPU can also switch off a faster, grounded runner
if another rival has a substantially easier clear approach. The
planner uses a short estimate from accepted movement, checks the
runner's projected runway, and keeps a ready strike ahead of the
switch. This is native pursuit behavior, not a retail AI claim.
A direct melee CPU can also dash earlier toward a grounded runner
when the short forecast leaves normal stopping room and puts the
rival in range. Both projected lanes must be clear. The forecast
does not steer the rival, change the dash duration, or accept a hit.
The two locals can cycle their attack aim among living enemies. This is
a native Battle Test control, not retail lock-on recovery. It feeds
move range, aim and direct launches, but not body heading, recipient
selection, CPU decisions, or deployed follower targeting. Committed aim
and launched guidance keep their captured targets.

## Next targets

1. Move beyond caller-selected round tables: qualify live retail mode
   and rank scheduling, combined-form receiver records, and the remaining
   amount conditions. All eight static table roles and the contact tier
   now have opt-in bindings for reviewed routes. The legacy fixed training
   row and `damage-side` fixtures still serve as controls, not as a
   scheduler or a substitute for an unqualified receiver.
2. `8002E2A8`: the force ledger, amount/HP boundary, reaction direction
   (modes 0..3 and scoped body-root mode 4), signed offsets, impact-record cooldown and pitch axis are
   native. Ordinary direct impact sound, static contact art and skeletal
   pose holds are opt-in. Full actor-local slowdown, alternate sounds
   and retail visual object callbacks remain
   outside the native presenter. Direct projectile and
   neutral close-B mode/offset binding is implemented for the reviewed
   routes; non-body carrier transforms and indirect contact paths remain
   unbound.
3. `8002BB14`: all 24 broad-phase/geometry gates are named and the apply/post
   branches carry operand provenance (`api-map/collision-branch-map.json`).
   `8002CAA8`'s scale rule is in `api-map/collision-walk-map.json`; the
   remaining work is the apply/post handler semantics, not more gate names.
4. The action-class table entries: reimplement the shared class handlers as
   native move behaviours, with the class selected by content. The 15
   category handlers are `80030500..80032E44`; twelve are open and large, and
   the three small matchers (`80032DA4`/`80032DF4`/`80032E44`) call
   `80030A3C`/`80031E50`/`80031F30` plus the `80032E94` geometry check, so
   this needs handler translation before a content mapping.

The credit-difficulty pair is exported as
[`api-map/contact-side-rows.json`](api-map/contact-side-rows.json) by
`tools/analyze/contact_side_rows.py`. The engine supports the attacker half with
`damage-side=team|...` (selected after a receiver `damage-vs` row and before the
default). The separate `--contact-scale-test` profile now binds the actual
receiver for qualified normal records; source mode scheduling and combined-form
component selection remain separate work.
