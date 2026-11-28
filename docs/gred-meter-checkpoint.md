# Normal G-Red: lift, air dash and grounded refill

No new movement binding is needed for the tested meter route. The existing
normal G-Red (`0615/0`) native source controller already spends and refills
the meter at the recovered boundaries. It does not share Sasuke's jump count
or the authored cruise refill gate.

The comparison reuses the qualified
[lift operands](g-red-animation-controller.md#earlier-neutral-vertical-jump-milestone),
[dash debit sites](g-red-animation-controller.md#combined-wind-up-entry-floor-response-gfmm9),
and [scheduled original observations](borg-movement-families.md#recovered-examples-and-remaining-limits).
Those observations include depletion during held lift, refusal of an
otherwise ready relaunch at zero, five `+2000` grounded visits, and a
separate buffered-flick dash that consumed `2000` before grounding. No new
original replay was needed. The original events and native updates below
are compared by role and amount, not by absolute visit number or trajectory.
The existing movement-input extractor reproduced the fixture's GFMI profile
byte-for-byte from the qualified DOL and normal G-Red setup profile
(`96039c7257fb2c177c093826dfe48e86ea13fa34087b9a6b70fff3c270d8b6ee`).

| Meter boundary | Original evidence | Running native observation |
| --- | --- | --- |
| Takeoff and ascent | Takeoff does not debit; ascent uses `150` on entry and `100` on later updates at the qualified unit rate. | Takeoff stays at `10000`. Ascent starts at native update 18: `10000 → 9850`, then `−100` per continued lift update. |
| Air-dash wind-up | Entry is free. The final wind-up charges `450` after movement and before contact; active charges `50` before movement. | Entry 46: `7150 → 7150`; handoff 51: `7150 → 6700`; active entry 52: `6700 → 6650`. The 31 active debits and one handoff cost `2000` total. |
| Floor and landing | A floor query or snap without the landing callback does not imply grounding or refill. Landing entry clears the airborne cache; the next resource update can refill. | Touchdown 113 keeps the airborne cache and `5150`. Landing entry 114 clears the cache, not the meter. Update 115 refills `5150 → 7150`, before landing animation completion. |
| Depleted relaunch and refill | A fresh conditioned pulse at zero does not relaunch, despite readiness. Empty landing entry is followed by five `+2000` visits. | Held lift exhausts the meter and enters falling. An isolated fresh press at update 120 yields the same falling motion as neutral. Landing entry 217 still has `0`; updates 218–222 refill to `10000`. A separate admission test holds the conditioned pulse, readiness and locks constant while changing the meter. |

The native fixture uses the same `prepare_battle_tick` interface and
`GroundWindupEntryPush` mode used by the package host, in the headless
two-actor native stage fixture. It checks that planning and world acceptance
alone do not publish the meter: the source owner commits after the matching
world receipt. A separate headless package smoke passed with the mandatory
landing entry and a later fresh ground jump.

**Deliberate scope difference:** the original can take off on the update
that would otherwise run landing entry, and it can keep a flick buffered
behind a simultaneous ground jump. Battle Test instead drops jump and dash
intent at its landing-entry barrier and does not promise the
original jump/flick buffer ordering. No admission was widened. The comparison
does not validate original pad polling, slope and prop contact, HUD pixels,
combat interruptions, or whole-frame equivalence.

To reproduce the native trace with the locally prepared normal G-Red stage,
rig and GFMM9 inputs:

```sh
python3 -B tools/build_native.py \
  --tests native_gred_meter_checkpoint,gred_movement_input,gred_landing_input \
  --jobs 2
build/direct/native_gred_meter_checkpoint
```
