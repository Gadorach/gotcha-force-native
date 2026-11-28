# Normal Galactic Emperor: placement before the airborne pin

The [loaded source replay](api-map/gg4e08-emperor-pin.json) shows that normal
Galactic Emperor (`0E04/0`) does **not** spawn at the position his ordinary
class hooks later enforce. The Emperor and Time Bomber load into separate
physical seats through the original archive delivery and constructor paths.

| Boundary | Per-slot fallback, empty grid | Stage code 2, original `st42` collision grids |
| --- | --- | --- |
| Class initialization, before shared placement | `(0, 2000, 0)`, heading `8192` | Same |
| Before the shared static projection | `(0, 1000, 0)`, heading `0` | `(-3648, 1985, -3800)`, heading `17913` |
| Dynamic query inlet and constructor return | Same as the placement above | Same as the placement above |
| First restoring class update, visit 22 | First pre-hook sees the placed position; the visit ends at `(0, 2000, 0)`, heading `8192` | Same ordering, starting from the table position |
| Open scene, visits 174–176 | Pin retained; the ordinary wrapper calls both hooks | Same |

The st42 case uses the original table row and all three unmodified collision
grids, with declared stage bounds. Neither projection changed this particular
placement. The first dynamic query had empty candidate lists; these cases
do not establish a rule for a different floor or any overlap.

At visit 175 an X-axis stick sample of `63` reaches the processed input
path and yields direction buffer `0022`. The two movement lanes stay on
idle state `52` and the root and heading match the no-input fork. Direct
requests for steering `1–3`, jump `4`, landing `7`, and plane `27/28` refuse
on both installed lanes. Idle `0` is the positive control.

The separate original receiver rebuild after visit 176 publishes **25**
Emperor descriptors from its own bank and **9** Time Bomber descriptors.
It does not alter the pin or install a saved support owner. This is registry
publication, not accepted overlap, a clearance test, or support. Scheduled
visits stop before the scene world tail; class-child capacity/lifetimes and
live contact corrections remain open. Visit numbers are fixture observations,
not retail display frames.

**No new native binding.** The current authored flying-boss profile follows
floor height plus `50`, moves forward on neutral input, and can turn and boost.
The running native contrast reproduced these behaviors against a moving peer.
`stationary-v1` is not an alternative: it stays grounded and has no moves.
Neither can represent both stage-owned initial placement and the later
class-owned airborne pin. An exact binding still needs a stage-owned
placement decision and a collision/contact witness around the pin; assigning
the pin at spawn would contradict both loaded original cases. The existing
authored package and its rules are unchanged.

All 64 original observations matched O0/O1/O2. The previous dual-scene,
Sasuke air-point, and melee receipts also reproduced byte-for-byte. The
native contrast passed under sanitizers, and the unchanged installed
Emperor package still ran its authored cruise in a headless host smoke.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py \
  --tests native_emperor_movement_contrast,normal_galactic_emperor,normal_beam_satellite --jobs 2
python3 -B -m tools.analyze.retail_emperor_pin \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_emperor_movement_contrast /tmp/opencode/emperor-pin.json
```
