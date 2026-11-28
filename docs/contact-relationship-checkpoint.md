# Ally admission belongs to the collision row

The [stock-row receipt](api-map/gg4e08-contact-relationship.json) compares
matched overlap with an opponent and an ally. Only the receiver team changes
at the pair inlet; the stock attack row, published shapes, carrier and
credited owner stay the same. These are bounded original pair replays, not
full matches.

| Original route | Stock flags | Opponent | Ally | Carrier follow-up |
| --- | --- | --- | --- | --- |
| Tau close B (`0807`) | `0001` | 17 HP | 4 HP | The [loaded world strike](world-team-pair-checkpoint.md) can hit both on the same visit, once each. |
| Machine Red rifle (`0600`, key 21) | `0000` | 8 HP | 2 HP | Keeps its shape on the next update after either hit. Its separate [second-receiver replay](api-map/gg4e08-rifle-next-receiver.json) checks fresh and duplicate tokens on opposing bodies. |
| Ordinary type-0 shots (`0800`/12, `0801`/125, `0802`/45) | `0000`, `0000`, `0001` | 17 HP | 4 HP | Closes attack publication on the next update after either hit. |
| Alien Worm and Claw Worm (`0501`/0, `050D`/1), opening group 6 | `0000` | Accepts, 0 HP | Accepts, 0 HP | The retained opposing opening returns its shape and then rebinds. |
| Same Worm carriers, follow-up group 7 | `00A1` | Accepts, 1 HP | **Excluded before narrow collision** | The ally keeps its opening token only; no new token, HP write, or credit. |

Positive damage goes into the original opponent or ally credit ledger
respectively. Zero-power opening contact still reaches the original HP writer
and advances history. That is distinct from an excluded follow-up. The follow-up
fork starts from the retained *opposing* opening and the original republished
shape: it does not qualify the complete lifecycle of a tether attached to an
ally, late repeat pulses, or poison.

## Static reachability and native decision

The scoped static check indexed 3,798 program prefixes and found 1,244
owner-to-opcode-10 edges for 200 zero-spec-record owners. Three edges had no
qualified steps; the others yielded 2,123 descriptor-step edges. None uses
`0x0080`. Neither do the 55 and
36 qualified type-0 and type-2 candidate edges, respectively. These counts
describe indexed data, not unique moves or whole-roster reachability. The
positive stock exclusion case comes from a carrier rebinding its shape, not
a direct opcode-10 request.

**No gameplay change.** One move can publish different relationship rules
over its lifetime, even while retaining its carrier and credited owner. A
move-wide team switch cannot express the Worm example. Authored native
projectiles still skip allies: a front ally does not consume an ordinary
or continuing projectile, and the opponent behind it can take damage.
No general allied multiplier, KO policy, or exact native contact binding
is inferred from the scoped damage amounts above.

All 34 original observations matched across O0/O1/O2, and five earlier
carrier receipts reproduced byte-for-byte. The native projectile contact
test, including both authored ally controls, passed under sanitizers.

Build [`retail_action_input_fixture.cpp`](../tests/host/retail_action_input_fixture.cpp)
with the [host fixture flags](retail-code-atlas.md#shared-action-gate), then:

```sh
python3 -B -m tools.analyze.retail_contact_relationship \
  "$GF_RETAIL_ROOT/sys/main.dol" "$GF_RETAIL_ROOT/files/afs_data.afs" \
  /path/to/action-input-fixture "$GF_RECOMP_MODULE" \
  /tmp/opencode/contact-relationship.json
python3 -B tools/build_native.py --tests native_projectile_contact_tail --sanitize --jobs 1
```
