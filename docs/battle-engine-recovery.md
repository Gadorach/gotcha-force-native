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
| Pair/collision/amount functions | amount translated; pair/collision not yet reimplemented |

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
geometry, 24 apply and 23 post. The apply band ends at the pair-loop call
`8002C4CC -> 8002DB58`, which is the entry to the reaction scaffold. The map is
[`api-map/collision-walk-map.json`](api-map/collision-walk-map.json).

The pair loop is **target selection**, not a blind pair walk:

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
call. The 47 direct calls are ordered in
[`api-map/reaction-scaffold-map.json`](api-map/reaction-scaffold-map.json):
16 pre-amount calls, the amount call and HP write, then 29 reaction and
presentation calls. The first reaction-band call (`8002F8DC`) is the force
ledger: it subtracts the amount from the team force record, adds the per-actor
ledgers and a six-slot hit-history ring. The rest are impact effects, object
spawns and color.

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
[`api-map/damage-amount-model.json`](api-map/damage-amount-model.json). It is a
19-stage multiply chain over one integer base:

```text
base (record+0)
  x carrier_relative
  x credit_difficulty(side, settings[attacker+136])      CE94
  x attacker_rank(attacker+1082)                         CFC8
  x conditional_half                                     CEB8
  x attacker_hp_row(record+6, HP index)                  CF48
  x team_force_row(record+7, force index)                CFC0
  x matchup[model_index(defender)][model_index(attacker)] D024
  x defender_class_row(model+156, settings[defender+136]) D100
  x relative(defender+180)                               D16C
  x defender_hp_row_157/158(post-hit HP index)           D1EC/D260
  x receiver_rank(defender+1082)                         D270
  x tier(defender+1738)                                  D290
  x defender_1504_half / same_player_quarter / scale_divisor_40 flags
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
The class row is `80433600[side][selector][settings[slot]]`; the two HP rows
use `+157` and `+158` with the post-hit HP index. The per-slot settings byte is
written by `80006ABC`: state 1 sets slot 0 = `d` and slot 1 = `31 - d` (with
`d` from `*80433934 + 0x46`), state 2 sets slots 0..3 to 7. The observed
defender slot values 16 and 30 correspond to d = 15 and d = 1; that is
consistent with, not proof of, that writer for those battles.

`tools/analyze/damage_content_row.py` turns the tables and the public selector
artifact into the native content row. The known hit reproduces the fixture line
exactly:

```sh
python3 -B -m tools.analyze.damage_content_row <dol> \
  docs/api-map/borg-damage-selectors.json \
  --attacker-model 0615 --defender-model 000a --attacker-side 0 \
  --defender-side 1 --attacker-settings 0 --defender-settings 16 --base 20
```

```text
damage-vs=10|0.867|1.13|1.15|1.0   product 1.126666   amount 22
```

## Next targets

1. Emit the eight recovered table roles into packages: credit difficulty and
   attacker rank for the attacker, class row and receiver rank for the
   defender, plus the two HP/force rows where the packager can supply them.
   The mapper, per-Borg selectors and the `damage_content_row` builder are now
   exported; the remaining choice is how the packager binds the battle settings
   index.
2. `8002E2A8`: reimplement the pair/reaction scaffold, reusing the matched
   helpers, the new amount routine and the published call map.
3. `8002BB14`: recover the branch conditions inside the collision walk; the
   call map is already published.
4. The action-class table entries: reimplement the shared class handlers as
   native move behaviours, with the class selected by content.

The credit-difficulty pair is exported as
[`api-map/contact-side-rows.json`](api-map/contact-side-rows.json) by
`tools/analyze/contact_side_rows.py`. The engine supports the attacker half with
`damage-side=team|...` (selected after a receiver `damage-vs` row and before the
default). The remaining roster step is emitting both variants into each package,
rather than binding one side.
