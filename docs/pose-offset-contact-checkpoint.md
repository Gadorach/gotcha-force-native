# Angel Nurse and Wing Soldier: pose height is not a hover floor

The [loaded original comparison](api-map/gg4e08-pose-offset-contact.json) did
not identify an above-floor grounding or refill rule. It did verify a
nonzero logical contact offset for normal **Angel Nurse** (`0900/0`) and
show what it does: the source subtracts it from the floor height at a snap.
Normal **Wing Soldier** (`0A00/0`) is the zero-logical-offset control.

| Boundary | Angel Nurse | Wing Soldier |
| --- | --- | --- |
| First ascent | The live lower entry uses one jump point. The pose offset is already nonzero. | The lower entry also uses one point. Logical offset is zero, despite a separate class pose-height value of `24`. |
| Next airborne visit, after declaring meter `4000` | Meter stays `4000`; the cached airborne flag suppresses recovery. | Meter becomes `6000` while the root is at `58`, above the floor at `10`. The type-2 branch checks the inhibit flag, not contact. |
| Static snap and real callback | At visit 77 the root is about `−0.089012`. Adding `6D0 ≈ 10.089012` puts the contact point exactly at `10`. | At visit 71 the root snaps directly to `10`, because `6D0` is zero. |
| Logical landing | The snap selects landing on both routes but leaves the airborne cache set. Nurse's next visit clears it, leaves one used point, and ends with root `10` and zero offset. | The next landing visit clears its airborne cache too. The separate `24` pose output is unchanged. |
| Same `4000` meter control at the landing-entry inlet | Landing entry keeps `4000/1 used`. The following grounded visit reaches `6000/0 used`. | Landing entry reaches `6000/1 used` and the following visit `8000/1 used`. That is not a point reset or a floor-gated refill. |

Both normal actor archives are loaded through the original scene path on
declared stage code 2 and relocated original `st42` grids. After opening,
a supplied processed jump packet stays held for 18 visits on the subject
seat only. The original takeoff, ascent, fall, snap, landing and return to
neutral run in order. The source prepass calls the actual pose-offset
producer at contact; a fork cuts at the real landing continuation and
checks the selected up-facing static floor and `rootY + 6D0`. The ordinary
visits cut before the scene world tail. No external support owner is
supplied or found at the snap. Visit counts are after opening, not
hardware ticks; no body-to-body clearance, moving support or rendered
hover height is qualified.

Meter `4000` is a **declared sentinel**, not a measured consumption. It is
seeded once after ascent entry, and separately in a fork at the
landing-entry inlet. Angel Nurse's observed air resource is an actual
jump count, not evidence of an ordinary flight-meter spend. Wing's
previously qualified [zero-offset glide](borg-controller-audit.md#normal-wing-soldier-and-quick-valkrie-glide-and-contact)
is a separate route; this new joined sequence uses ordinary fall.

**No gameplay change.** The detached native recovery kernels reproduce
the two landing-fork results. In a running authored native battle,
a pose-only vertical offset did not move the floor or alter touchdown,
and the authored combined action budget still reset immediately. That
contrast is not a port of either retail controller. A new hover-floor
binding needs an exact producer of above-floor logical contact, not
only a posed height or a refill observed near landing.

Both unchanged installed packages still ran their authored movement in the
headless host smoke. This was not a source-matched move or contact test.
All 28 original observations matched across O0/O1/O2, and four previous
dual-scene receipts reproduced byte-for-byte. The native contrast passed
under sanitizers.

Build [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_hover_floor_contrast,normal_type2_glide --jobs 2
python3 -B -m tools.analyze.retail_hover_floor \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual-scene-entry-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_hover_floor_contrast /tmp/opencode/pose-offset-contact.json
```
