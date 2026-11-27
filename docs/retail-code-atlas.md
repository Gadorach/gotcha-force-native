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
  and the available config and program-table identities.
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
