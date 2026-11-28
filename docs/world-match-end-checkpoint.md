# One-row Tau KO: Force result and ending frontier

The [receipt](api-map/gg4e08-world-match-end-frontier.json) uses the same
loaded Tau hit as the [queued handoff](world-ko-handoff-checkpoint.md), but
side 1 has only its current Tau (`0807/0`). The fixture declares **90 Force**,
equal to the loaded receiver's KO cost. There is no reserve actor or next
archive. The two lethal branches differ only in whether the Force debit
is enabled.

| Case | Complete combat visit 27 | Later boundary |
| --- | --- | --- |
| Survives | Requests and removes 17 HP; 43 HP and 90 Force remain | No result or retirement through visit 35 |
| Lethal, no Force debit | Requests and credits 14, removes **1 HP**; 90 Force remains and there is no result | Receiving shapes disappear at 28, three old cells return at 63, and phase 3 starts at 64. A 60-step wait starts at 65; the seat becomes inactive at 125. No next archive request, winner, or old archive release through 130. |
| Lethal, Force exhausted | Same HP write and credit, then **90 → 0 Force**; the scene selects side 0 and schedules its ending handler | The first ending visit stops at a missing audio table. The defeated seat and its archive are still bound. |

In both lethal arms the HP leaf returns while the actor is still in phase 1,
the Force pool is unchanged, and no result is set. The returning combat
dispatcher selects the result **after** the world pass, before the defeated
body's receiving shapes have disappeared. In the no-debit arm, an inactive
seat is not proof that the old archive or its four live view wrappers returned.

## Exact stop, not a completed match

Visit 28 in the Force-exhausting arm reaches the ending handler, sets its
status to **5** and clock to **90**, and requests audio entry **118**. The
CPU-only fixture has a null table pointer at `803D3E44`. The first external
read is `00000766` at `pc=800F0FDC`, `lr=800EFC94`. The fixture refuses it and
does not resume execution. No ending dispatcher return, further retirement,
match-end return, or save handling was observed on that arm. Visits are
counted from scene opening, not display frames.

**No gameplay change.** The authored native duel still ends on team KO. Its
portable result bridge reports a win for the lethal control and no completed
outcome for the surviving control. It does not emulate the original Force
gate or ending schedule.

All 15 original observations matched across O0/O1/O2. The dual-scene entry,
melee interruption, moving-receiver, and queued KO handoff receipts reproduced
byte-for-byte. The native body-exit, roster-series, and battle MVP controls
passed under sanitizers.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_world_body_exit --jobs 1
python3 -B -m tools.analyze.retail_world_match_end \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_world_body_exit /tmp/opencode/world-match-end-frontier.json
```
