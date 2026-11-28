# First shared combat checkpoint: contact and carrier slot return

The [original type-0 receipt](api-map/gg4e08-type0-contact.json) stops the attack
on its first follow-up visit and returns the carrier and descriptor on visit
three. A miss or seeded duplicate leaves it flying. The
[Machine Red receipt](api-map/gg4e08-rifle-next-receiver.json) instead shows one
paid carrier dealing damage to two distinct receivers without accepting the
same token twice on the first body.

The [headless comparison](api-map/native-projectile-contact-comparison.json)
uses an authored Workshop with two stationary recipients. It seeds the
observed **17** and **8** damage amounts, so this checks custody and recipients,
not the retail damage evaluator.

| Native trial | Accepted hits by recipient | Slot return |
| --- | --- | --- |
| Ordinary default | First body only | Hit update |
| Ordinary opt-in | Same first-body hit | Three updates after the hit |
| Ordinary opt-in, miss | None | Normal expiry, no contact tail |
| Rifle continuation | One hit to each body with the same object and debit | Survives both hits, then expires |
| Rifle continuation, second body offset | First body only | Normal expiry |

The first mismatch was **slot availability**, not whether an ordinary shot
stopped hitting. The [portable opt-in](borg-gameplay-api.md#stop-on-contact-return-the-slot-later)
closes native contact immediately and makes the inactive slot unavailable to
other releases. A native unit test offers a release on the old slot's return
update and checks later reuse with a fresh allocation ID. That test also
refuses a socket emission before publication, with no hit or held slot, and
checks that HP and credit are not delayed.

The [retail extractor opt-in](retail-roster-extraction.md#type-0-ordinary-b-contact-tail-test)
selects only normal `0800`/key 12, `0801`/key 125 and `0802`/key 45. No normal
Machine Red B rules or default type-0 packages change. The native slot count
does not reconstruct the original descriptor pool, exact unlink timing,
or original graphics. Native updates in the comparison are not original
frames, and damage on the installed packages remains authored.

```sh
python3 -B -m tools.analyze.native_projectile_contact_compare /path/to/gf_native \
  --output /tmp/opencode/native-projectile-compare.json
python3 -B tools/build_native.py --tests native_projectile_contact_tail,borg_gameplay --jobs 2
```

The earlier host and the updated host produced identical normalized default
and rifle results. `--baseline-only` runs those controls on a host that does
not yet implement the new opt-in. With local copies of the already extracted
normal packages, the same binder preserved all three positive B hits and
delayed slot return; Machine Red still hit one recipient by default and two
with continuation. Those installed checks used temporary copies, not a new
full extraction from `GF_CATALOG`.

The native contact-tail test passed both ordinary and sanitizer builds; the
existing Borg gameplay suite passed. The Python suite ran with the local
retail inputs: **1,059 passed, 129 skipped**.

The pending launcher-install audit later ran **201** subjects on each
host with the same aggregate counters. Both refused the group containing
borrowed-controller G-Red, so it was not a completed 206-subject regression
comparison. A [separate current-host audit](melee-interruption-checkpoint.md#workshop-triage)
staged an explicitly authored normal G-Red package and admitted all 206.
