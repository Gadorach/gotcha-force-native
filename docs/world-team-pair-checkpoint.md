# A loaded Tau strike can hit an opponent and an ally

The [three-body receipt](api-map/gg4e08-world-team-pair.json) extends the
[two-body Tau comparison](world-moving-receiver-checkpoint.md). Three normal
Taus (`0807/0`) load independently on the original `st42` grids. Seat 0 attacks
with close B. The other two seats get the same processed movement input; their
team assignments are declared before opening. Each publishes two receiving
shapes. Identical input does not guarantee identical geometry.

On visit **27** the three actor poses publish before the receiving list is
rebuilt. When **seat 2 is allied**, both other actors overlap the unmodified
strike. The original pair pass tries one narrow collision per recipient. One
strike token produces **two HP writes in that visit**:

| Recipient | Requested and removed | Attacker ledger |
| --- | ---: | --- |
| Seat 1, opponent | 17 HP | 17 opponent damage |
| Seat 2, ally | 4 HP | 4 ally damage |

Each recipient records token 1. Neither is hit again by that strike, and its
exact cell returns on visit **32**. In the other full-world case, seat 1 is
allied and is eligible just the same, but its two shapes miss the broad
overlap. Only opposing seat 2 takes damage. No loss of HP is not, by itself,
evidence of team exclusion.

## Same geometry, separate relationship controls

The controls start at the original visit-27 pair inlet, after shape
publication. They are **pair-only forks**, not changes to the returning
dispatcher:

| One change to the overlapping case | Original pair result, in observed write order |
| --- | --- |
| Relabel the ally as an opponent | Both still pass narrow collision. Requests are 17 and 17; opponent credit is 34. |
| Relabel the opponent as an ally | Both still pass narrow collision. Requests are 4 and 4; ally credit is 8. |
| Set the original `0x0080` exclusion bit in the attack-rule row | The ally still overlaps, but is rejected before narrow collision. Only the opponent takes 17. The stock row is `0x0001`, without that bit. |

This verifies a relationship-dependent amount and separate credit ledgers
for this close-B shape, not a universal friendly-fire policy, multiplier,
recipient order, or retail moveset match. None of the hits was lethal, so
this replay does not qualify allied KO or Force handling.

**No gameplay change.** In the native authored control both body proxies
overlap, but only the opponent takes damage regardless of seat. Any retail
opt-in would need to describe both relationship admission and per-recipient
strike history; flipping the authored team filter alone would not reproduce
this result.

All 22 original observations matched across O0/O1/O2, and five earlier
receipts reproduced byte-for-byte. The native team-pair, moving-receiver,
and roster-series tests passed under sanitizers.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_world_team_pair --jobs 1
python3 -B -m tools.analyze.retail_world_team_pair \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_world_team_pair /tmp/opencode/world-team-pair.json
```
