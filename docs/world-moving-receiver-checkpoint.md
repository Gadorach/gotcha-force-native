# Moving receivers in the original world schedule

The [loaded Tau replay](api-map/gg4e08-world-moving-receiver.json) joins ordinary
actor motion, pose publication, receiver-list rebuild, the world collision
pass, and HP on the same loaded owners. It no longer needs the
[declared matrix translations](retail-code-atlas.md#moving-receivers-narrow-tests-use-the-receivers-final-pose)
used by the earlier geometry probe.

Two normal Taus (`0807/0`) load through the original scene. Both cases share
the first 21 complete dispatcher visits after opening, including the same
attacker B packet on visit 8. On visit 22 the receiver starts a dash from
either `+63` or `−63` processed stick X. Both receivers are moving in selector
13 when the attack publishes on visit 27. No fixture writes a root, matrix
or shape after opening. The changed input can also affect the attacker's
motion in the original world; this is a matched schedule, not an experiment
that holds every other coordinate fixed.

| Boundary | `+63`, hit | `−63`, miss |
| --- | --- | --- |
| Visits 26 → 27 | Both original receiver matrices advance; the two spheres retain their previous centers and publish new ones in the same cells | Same |
| Ordinary world pair, visit 27 | The same close-B sphere publishes in slot 6. Both receiver descriptors pass the relationship check and the broad test; descriptor 2 reaches the narrow kernel and overlaps. | The same attack publishes. Both descriptors are relationship-eligible, but only descriptor 1 passes the broad test. Its current sphere reaches the narrow kernel and misses. |
| Complete world visit 27 | One writer removes **17 HP**, from 60 to 43. The receiver records token 1; the attacker credit and receiver damage ledgers each record 17. | No HP write, accepted receiver token, or credit. |
| Follow-up | Visit 28 does not write HP again; slot 6 returns on visit 32. | Same slot return, with HP unchanged. |

The observation forks stop after the actor pass, after the rebuild, and at
the ordinary pair inlet. Both cases check that the narrow scratch data uses
the **current** published center. The receiver owner displacement `1CC` is
zero at these sampled pair inlets. The main runs return through the real
scene dispatcher, with matching HP and ownership results. The fixture
uses the three unmodified `st42` collision grids and declared initial slot
coordinates, with the stage world handler on its no-op branch. Moving
props, owned support, nonzero pair correction, hardware polling and
presentation devices are not qualified. Visit numbers are not display
frames.

**No gameplay change.** The authored native control already samples the
receiver after movement. On its miss update, a stale receiver proxy at
Z `165.5` would overlap; the new proxy at Z `170.5` does not. Both cases
arm and retire the same authored window, and the accepted native contact
removes the authored **12 HP**, not the retail amount. The native controller
does not gain the original skeletal sphere or its allocation timing from
this comparison.

All 23 original observations matched across O0/O1/O2. Four earlier
dual-scene receipts reproduced byte-for-byte. The native moving control
passed under sanitizers, the existing melee interruption and partition
tests passed, and the installed authored Tau still hit for 12 HP in a
headless smoke.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_world_moving_receiver,native_melee_interruption --jobs 2
python3 -B -m tools.analyze.retail_world_moving_receiver \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_world_moving_receiver /tmp/opencode/world-moving-receiver.json
```
