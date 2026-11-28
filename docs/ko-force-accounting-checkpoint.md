# Opt-in KO Force accounting

Standalone package Battle Tests can now track **KO cost** independently
of HP damage. Every fighter needs a `ko-force-cost` in its normal gameplay
profile, and the caller must explicitly supply both team pools. A profile
with a cost but no session opt-in behaves as before.

On an admitted HP write that takes a living fighter to zero, the engine
debits that fighter's team once, clamping the remaining pool at zero.
Each defeated seat keeps a round-local record of the configured cost,
actual debit, team, and whether the debit crossed zero. Nonlethal hits
leave both pools unchanged. Restarting the round restores the declared
budgets and clears the records.

The first retail binding is **normal Tau (`0807/0`)**, whose cached
90-point cost is verified in the [loaded KO replay](world-allied-ko-checkpoint.md).
The [earlier knockout replay](api-map/gg4e08-body-knockout.json) verifies
the original clamp when a cached cost exceeds the available pool. The
Tau extractor emits just `ko-force-cost=90`, not a different attack or
new default battle policy.

| Battle Test case | HP behavior | Opt-in Force behavior |
| --- | --- | --- |
| Nonlethal hit | Existing native HP loss and credit | No debit |
| KO with 180 Force remaining | Existing authored defeat | 90 debited, 90 remains |
| KO with 90 Force remaining | Same authored defeat | 90 debited, zero crossing recorded |
| KO with 40 Force remaining | Same authored defeat | 40 debited, zero crossing recorded |

**Accounting only.** Team Force does not decide the round, change a hit's
damage or admission, or schedule replacement. The opt-in does not
populate original HP-request or KO-credit ledgers, enable friendly fire,
or bind Story, Challenge, reserves, or saves. The existing legacy HP-based
Force ledger is separate and cannot be mixed with this option.

Two fresh 206-package extractions differed only in Tau's gameplay row and
two metadata files. The installed Tau smoke held the native hit trace fixed
with and without accounting. Native sanitizer controls also covered
multiple victims sharing a pool and a surviving teammate after exhaustion.

## Run the normal Tau Battle Test

Generate a fresh local package set:

```sh
python3 -B -m tools.extract.retail_borg_roster "$GF_CATALOG" \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  local/tau-ko-force --role-clips --source-pace --action-clips \
  --basic-shot-test --basic-melee-test --ko-force-cost-test
```

With that package available under the installed `borgs/` root, run:

```sh
gf_native --battle-borg gg4e08:0807 --against-borg gg4e08:0807 \
  --package-battle-distance 90 --package-ko-force 180,90 --trace-package-battle
```

`borg-ko-force` trace rows distinguish cost, actual debit, and before/after
balances. To run the installed-package smoke against a generated roster
instead of an installation:

```sh
PYTHONPATH=. python3 -B tests/host/retail_ko_force_smoke.py \
  /path/to/gf_native local/tau-ko-force local/content/native-battle-mvp-v1
```
