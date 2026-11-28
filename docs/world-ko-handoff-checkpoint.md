# A loaded Tau KO through Time Bomber readiness

The [joined KO receipt](api-map/gg4e08-world-ko-handoff.json) follows the same
loaded physical seat from an ordinary melee hit, through body and archive
release, to a new normal actor. It uses the
[moving Tau world setup](world-moving-receiver-checkpoint.md) on the original
`st42` grids. The fixture declares side 1's two-row queue, Tau `0807/0` then
Time Bomber `0106/0`, without creating a reserve actor.

## Hit, unlink and request

The two cases share the earlier actor and world visits. Before contact on
visit 27, the lethal fork changes only the receiver's current HP, from
60 to 1. The original amount evaluator still runs; its answer changes too.

| Boundary | Surviving hit | Lethal hit |
| --- | --- | --- |
| Original HP writer | Requests and removes 17, leaving 43 | Requests 14, removes **1**, leaving 0. The two damage ledgers already contain 14 at the writer inlet. |
| Writer returns | Body is still in phase 1, old archive still bound | Same: the HP leaf does not unlink the body, consume Force, or request a replacement |
| Complete world visit 27 | Phase 1 retains its two receiving descriptors | World continuation enters phase 2 and sets mask `0443`. The already rebuilt list still has the old receiver this visit. |
| Visit 28 | Receiver remains published | Receiver is no longer published, but its three old body cells remain allocated |
| Visits 63 → 65 | No retirement in the surviving control through visit 35 | All three cells return at 63; phase 3 begins at 64. At 65 the loader returns the **loaded** Tau archive owner and its four active view wrappers before requesting AFS member **504**. |

The request is cut at the original disc call. It does not fabricate a
successful disc return. Dispatcher calls from that retained state complete
the 60-step wait, then leave the seat in phase 0 without a replacement owner.
The old ready byte is still set at that point, but the seat does not read
input, publish receivers, or advance generation. A ready byte alone is
not an admission receipt.

## Declared delivery, observed readiness

The fixture supplies the exact four decompressed archive roles at the original
request destinations and invokes the original completion callback, `800412C4`.
The allocator reuses the **same physical header** only after the observed old
owner return. The actor remains in the same physical seat, but the constructor
advances its generation from 1 to 2, clears 48 collision-history entries,
initializes **220 HP** and **10 / 3** stock, and binds 16 new body descriptors.

Visits below are counted from the declared delivery:

| Visit | Original result |
| --- | --- |
| 81 | New ready byte set; ordinary role mask still `00C3` |
| 82 | Role mask opens, but no replacement pad read or ordinary receiver publication |
| 83 | First replacement pad read. A separate fresh-B fork reaches key 2/pose 10 and spends one stock (`10 → 9`); it cuts before the bomb factory runs. |
| 140 | Nine ordinary receiving shapes publish in the neutral continuation |

This verifies the loaded Tau release and exact Time Bomber startup in this
declared queue, not physical I/O, asynchronous timing, arbitrary rig teardown,
inactive-seat reuse, campaign progression, or the no-reserve match-end route.
Visit counts are not display frames.

**No gameplay change.** The authored native control ends the round immediately
on its lethal hit and does not reload or advance the seat. Native damage
credit in that control is the **1 HP removed**, not the original request.
The surviving native control continues and retires its attack window normally.

All 28 original observations matched across O0/O1/O2. The earlier
dual-scene entry, melee interruption, and moving-receiver receipts
reproduced byte-for-byte. The native body-exit contrast passed under
sanitizers.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_world_body_exit --jobs 1
python3 -B -m tools.analyze.retail_world_body_exit \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_world_body_exit /tmp/opencode/world-ko-handoff.json
```
