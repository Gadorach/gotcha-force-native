# A loaded Tau can exhaust its own team's Force

The [allied-KO receipt](api-map/gg4e08-world-allied-ko.json) reuses the
[three-Tau close-B hit](world-team-pair-checkpoint.md). Seat 0 attacks;
seat 1 is an opponent and seat 2 is an ally. Both are hit on visit 27.
The lethal controls lower one receiver's current HP to **1**, preserving
its max HP of 60 and original cached KO cost of **90**.

| Receiver at 1 HP | HP requested / removed | Attribution after the complete visit | Result when the victim's team has exactly 90 Force |
| --- | ---: | --- | --- |
| Opponent | 14 / 1 | 14 opponent damage, one opponent KO credited to the attacker, and one KO notification | Team 1 loses; team 0 wins |
| Ally | 3 / 1 | 3 ally damage and a 90-cost own-team KO observation; no opponent KO credit or KO notification | Team 0 loses; team 1 wins, even with the attacker alive at 60 HP |

With **180 Force** on each team, either KO debits **90** only from the
victim's team; no winner is selected in the tested continuation through
visit 35. The defeated body enters phase 2 before its receiving shapes
drop from the next list. An opposing KO debits Force before the same
strike's second HP write, but the damage evaluator's Force mirrors still
hold the start-of-visit values. When a team exhausts its Force, the source
result is selected on the returning combat visit. The ending handler and
later archive retirement are not run in those two cases.

## Why the allied hit requests 3 HP, not 4

The amount-only controls use the same published seat-2 body and the stock
attack row with raw power 20. When current HP is declared **before** the
scene refreshes its HP mirror, the amount entry gives:

| Current HP and refreshed mirror | Opponent request | Ally request |
| ---: | ---: | ---: |
| 1–23 | 14 | 3 |
| 24–29 | 15 | 3 |
| 30–34 | 16 | 4 |
| 35–60 | 17 | 4 |

The original amount routine multiplies a same-team amount by **¼** before
its positive minimum and truncation when the row's `0x1000` bit is clear.
Setting that bit in a disposable amount-only control restores the opposing
amount throughout the sweep. A separate late edit leaves the mirror at
60 while changing current HP to 1; its allied amount is still 4. Neither
control edits the stock world runs.

**No gameplay change.** The authored native control still skips an
overlapping ally at 1 HP. In its opposing KO case it removes and credits
1 HP, debits 1 Force, and ends the duel on actor elimination even though
89 Force remains. It is not a match for the original KO cost or result rule.
No allied admission, original ending sequence, save behavior, or whole-game
equivalence is claimed.

All 76 original observations matched across O0/O1/O2, and six prior
receipts reproduced byte-for-byte. The native team-pair, body-exit and
roster-series tests passed under sanitizers.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_world_team_pair --jobs 1
python3 -B -m tools.analyze.retail_world_allied_ko \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_world_team_pair /tmp/opencode/world-allied-ko.json
```
