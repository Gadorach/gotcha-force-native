# Normal Blue Striker: boost exit and delayed refill

The [source/native receipt](api-map/gg4e08-blue-striker-boost.json) matches
**353** bounded visits for normal Blue Striker (`0D00/0`) across three inputs:
release before readiness, release after readiness, and held depletion followed
by a fresh press on the first refill visit. The O0/O1/O2 receipts matched
byte-for-byte.

| Boundary | Original result at the declared unit rate |
| --- | --- |
| Entry | A fresh conditioned pulse selects boost. The lower handler spends `50`, reloads delay `16`, and seeds clock `30` without decrementing it. |
| Early release | Visits 2–31 continue boost. Visit 31 reaches zero clock and opens the latch; selection first uses the opened latch on visit 32. That exit neither spends nor refills. |
| Late release | Holding past readiness does not time out. Releasing on visit 37 exits at once, with `8200` meter. |
| Held depletion | Visit 200 spends the last `50`. The still-held input exits on visit 201 because the latch is open and meter is zero. |
| Refill and re-entry | Visits 202–217 only decrement the delay, including `1 → 0`. A fresh pulse on 218 can spend the first `+50` on that same visit. With no meter left, this boost still waits until its own latch opens on 248 and exits on 249. |

The original probe uses the normal class installer and its exact archived
record. Each visit runs the real pre-input bookkeeping, the selected cruise
or boost gate, and the lower handler through its meter and clock writes.
It supplies conditioned input, unit rate, and an already-held cue slot
that refreshes without playback. It cuts before heading, movement and floor
contact; it does not qualify the outer actor scheduler, earlier action
arbitration, hit interruptions, physical controller input, or the HUD.

No new runtime policy is necessary. The existing
[`cruise-boost` row](borg-gameplay-api.md#profile) counts **handler visits**,
including entry. The [opt-in extraction](retail-roster-extraction.md#blue-striker-boost-test)
uses `240|10000|50|50|16|31` instead of `240|240|12|12|8|4` only for this
package. The first field is still authored height. Speed, turning, terrain
following, charge-shot timing, and damage rules are unchanged by the binding.
Copying the original `30` into the native minimum exits one visit too soon;
the trace includes that negative control as well as the unchanged default.
The native test runs with an independently moving authored opponent.

Fresh baseline and opt-in extractions each produced 206 normal packages.
Of 2,910 file pairs, only Blue Striker's gameplay and source index and the
top-level extraction receipt differed. Both generated packages ran on the
headless host; the same scripted input reported 10 versus 31 boost updates.
That smoke did not emit a shot and does not qualify hit or charge timing.

Build the [original fixture](../tests/host/retail_blue_striker_boost_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B tools/build_native.py --tests native_blue_striker_boost,normal_blue_striker_boost --jobs 2
python3 -B -m tools.analyze.retail_blue_striker_boost \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/blue-striker-boost-fixture "$GF_RECOMP_MODULE" \
  build/direct/native_blue_striker_boost /tmp/opencode/blue-striker-boost.json
```
