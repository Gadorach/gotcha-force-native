# Normal Sasuke: jump points, air dash and landing

The [loaded normal `000A/0` replay](api-map/gg4e08-sasuke-air-points.json)
distinguishes four events: requesting a jump, entering the jump, entering an
air dash, and resetting the used count. This is a three-point **jump** count,
not a shared jump/dash budget and not a flight meter.

| Boundary | Original observation | Native Battle Test |
| --- | --- | --- |
| Ground takeoff | Selector 4 leaves 0 used; ascent entry spends the first point | Accepted jump moves and spends on the same update |
| Air relaunch | A conditioned pulse below the cap can request selector 8; entry, not the request, spends. Ordinary fall also requires readiness; active dash does not. | A fresh native jump spends immediately below the cap, including from an active dash. This does not reproduce the source readiness gates. |
| Air dash | Buffered flick admits wind-up at each count. The request leaves the count alone. Wind-up changes 0 to 1 and leaves 1, 2 and 3 unchanged. The meter stayed at 10,000. | An opted-in accepted air dash seeds 0 to 1. It spends no additional point and can still dash at the jump cap if the authored dash gate allows it. |
| Landing | Landing entry clears airborne state but preserves the count. The first following grounded resource update clears it. | With the opt-in, native touchdown preserves the count. The next grounded update clears it before native input processing. |

The original fixture opens two archive-loaded normal actors, then uses bounded
branches of the loaded Sasuke actor. The matrix declares the current selector,
input, and readiness state. The dash and relaunch cases run the original
request gates; accepted cases then enter the relevant lower handler up to a
verified call boundary. The landing case tests resource logic before and after
landing entry, not floor collision. The
[earlier scheduled witnesses](borg-controller-audit.md#takeoff-relaunch-and-landing)
provide the separate walk-off and lower-floor landing evidence. All 67 new
observations matched O0/O1/O2; the prior dual-scene entry and melee receipts
also reproduced byte-for-byte.

The [extractor opt-in](retail-roster-extraction.md#sasuke-air-points-test)
replaces only the authored `air-action-budget=3` on the qualified normal
Sasuke package. The native tests keep the combined-budget control, the
jump-only control without seeding, and a grounded dash that cannot seed.
They also check a relaunch from an active native dash and refusal at the jump
cap. Neither matching account transitions nor loading the generated package
matches the full original controller: input conditioning, pose clocks, dash
budget/chaining/height, support terrain and live HUD drawing remain outside
this binding.

Fresh baseline and opt-in extractions each produced 206 normal packages.
Of 2,894 file pairs, only Sasuke's gameplay and source index and the
top-level extraction receipt differed. On the built headless host, both
generated Sasuke packages produced the same eight grounded-B events,
including a 7 HP authored hit. Native movement assertions ran separately
in the unit fixture, including under sanitizers. This was not a 206-Borg
runtime comparison.

Build the original fixture with the [phase fixture flags](retail-code-atlas.md#shared-action-gate),
substituting [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp).
Then run against a qualified local DOL, AFS and unmodified recomp module:

```sh
python3 -B tools/build_native.py --tests native_air_point_policy,normal_sasuke_air_points --jobs 2
python3 -B -m tools.analyze.retail_sasuke_air \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/dual_scene_entry_fixture /path/to/gGG4E08_recomp.so \
  build/direct/native_air_point_policy /tmp/opencode/sasuke-air-points.json
```
