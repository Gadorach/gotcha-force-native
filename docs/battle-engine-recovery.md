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
  follows 12 curated roots into **240 functions and 403 direct calls**, with the
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

The **action and scene scaffolding is largely matched**; the **collision, pair,
reaction and amount core is still open**. That matches the live traces: the
reaction tail (`8003D344`) is matched, while the pair (`8002E2A8`), collision
(`8002BB14`) and amount (`8003CD5C`) are open.

## Native mapping

| Original | Native |
| --- | --- |
| `lbl_802D3B68[actor+0x580]` class dispatch | `moves` selected by button, priority, range and state |
| Shared action handlers | shared melee/projectile/field systems |
| `8003D344` HP writer | `damage()` and actor health |
| Recovered scale rows | `damage-factors` / `damage-chain` profile keys |
| Pair/collision/amount functions | **not yet reimplemented** |

The native engine is already data-driven in the same sense as the original: a
package declares moves, buttons and ranges, and shared systems execute them. The
remaining work is to move the **open** collision/pair/reaction/amount logic into
shared native systems and content data, not to add per-Borg branches.

## Next targets

1. `8002E2A8` / `8002BB14` / `8002DB58`: pair enumeration, collision and
   reaction. These are the largest open functions and the gate for hit geometry,
   contact timing and per-move damage.
2. `8003CD5C`: the amount function and its factor-row selection.
3. The action-class table entries: reimplement the shared class handlers as
   native move behaviours, with the class selected by content.
