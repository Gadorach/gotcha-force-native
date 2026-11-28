# Ordinary melee interruption checkpoint

Two independently loaded normal Tau actors (`gg4e08:0807`) run the original
processed close-B path. Both scenarios start the subject's B on visit **8**:

| Bounded original visit | Uninterrupted | Opponent starts earlier |
| --- | --- | --- |
| 8 | Subject allocates group 1 / step 2, but no attack is published | Same; the opponent already owns a separate cell |
| 19 | Subject's opening continues | Opponent removes **17 HP** from subject at pose **5**; subject's lower-lane selector clears |
| 20 | Subject still has the cell | Subject's exact cell is on the free list; it never publishes or receives an outgoing token |
| 27 | Pose **10**: subject publishes, obtains token 1, and removes **17 HP** | No subject hit |
| 32 | Published cell has returned | No replay of the interrupted attack through visit 40 |

The [original receipt](api-map/gg4e08-melee-interruption.json) checks the exact
amount record, actual contributor and HP recipient. The first actor
presentation call is a cutoff, not a stubbed return. The stage and initial
positions are declared and the processed B packets are supplied. Each visit
composes bounded actor, list-rebuild and collision calls; it is not a full
battle frame. Do not infer how original recovery admits the next press.

The authored native Tau control agrees on **no payload from the interrupted
start**, but it reserves no melee object before the active window. Its hit
interrupts on the next native update, and a later fresh press arms a new
strike. It keeps the authored 12-HP amount and its own clock. These are
different allocation mechanisms; no native timing or gameplay was changed.

## Workshop triage

The [authored-roster audit](api-map/native-workshop-interruptions.json) admitted
and ran all **206** normal packages. The audit used the local v62 test packages
with an authored normal G-Red package explicitly staged for this run. The
launcher install's borrowed G-Red controller is correctly refused by explicit
rosters; no runtime refusal was relaxed. This run is not a byte-for-byte
before/after comparison of the 206 installed default packages.

At update 240, **191** subjects had started a move and **183** had damaged an
opponent. Five started, suffered an interruption and recorded no enemy hit:

| Subject | Observed outcome |
| --- | --- |
| Command Girl `0306`, Star Hero `0804`, Cyber Hero `0808`, Tar Diver `080d` | An earlier accepted hit was followed by an authored `opening-slash` interrupt before frame 9; no payload. |
| Roach `0f05` | `mine` emitted at 230, incoming hit at 231, interrupt at 232. This is **after** payload and is not a missing melee window. |

These five outcomes have trace explanations under the **current authored**
rules. That does not independently prove all five are correct retail
behaviour; only the normal Tau case above has the bounded original comparison.
Do not shorten wind-ups or tune the CPU merely to raise these totals.

## Reproduce

Compile [`retail_dual_scene_entry_fixture.cpp`](../tests/host/retail_dual_scene_entry_fixture.cpp)
with the [phase fixture flags](retail-code-atlas.md#shared-action-gate).
All **80** original observations matched across O0/O1/O2, and the older
dual-scene-entry receipt reproduced byte-for-byte.

```sh
python3 -B tools/build_native.py --tests native_melee_interruption --jobs 1
python3 -B -m tools.analyze.retail_melee_interruption \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /tmp/opencode/dual-scene-entry "$GF_RECOMP_MODULE" \
  build/direct/native_melee_interruption /tmp/opencode/gf-melee-interruption.json

# On an installed authored Workshop content root:
python3 -B -m tools.analyze.native_roster_matrix /path/to/gf_native /path/to/content \
  --frames 240 --checkpoint 120 > /tmp/opencode/gf-workshop-matrix.json
python3 -B -m tools.analyze.native_workshop_interruptions \
  /path/to/gf_native /path/to/content /tmp/opencode/gf-workshop-matrix.json \
  /tmp/opencode/gf-workshop-interruptions.json
```
