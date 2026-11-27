# GG4E08 retail package track

## Current priority: roster conversion before exact move recovery

Use the [deterministic roster extractor](retail-roster-extraction.md) for the
current baseline. The catalogue-only baseline and individual witnesses below
are historical. Machine Red's isolated loader research is parked; it is not a
dependency for extracting the roster, decoding common assets, or mapping basic
movement and moves by shared evidence.

## Roster baseline

The local source-derived `catalog.gfbc` contains **206** distinct retail collection
definitions and a bijection over source ordinals 0–205. The installed desktop
packages used by this project already cover five IDs:

| Stable ID | Existing package capability |
| --- | --- |
| `gg4e08:0615` G-Red | Qualified normal movement practice and bounded Battle Test; test-authored combat |
| `gg4e08:000a` Sasuke | Normal preview and opt-in, authored Battle Test |
| `gg4e08:0005` Teleport Ninja | Normal preview and controller review only |
| `gg4e08:0009` Switching Ninja | Normal preview and controller review only |
| `gg4e08:0d06` Death ICBM | Normal static preview and controller review only |

The remaining **201** IDs get local two-file placeholders: a `GFBORG 2`
manifest and its required catalogue. Preserve the stable ID if a folder is
renamed or a placeholder is replaced. In an empty install root, the exporter
instead creates all 206. It skips existing identities, not known folder names,
and refuses conflicts before writing anything. It never rewrites a package.

```sh
python3 -B -m tools.extract.retail_borg_placeholders /path/to/catalog.gfbc borgs --dry-run
python3 -B -m tools.extract.retail_borg_placeholders /path/to/catalog.gfbc borgs
gf_native --list-borgs
```

The required input is the local GFBC v1 export from the GG4E08 Force Editor
recovery (`catalog-inputs-v1/catalog.gfbc`), not a wiki scrape and not an
unrelated region's catalogue. The exporter qualifies that exact input. The
source's encoded name is normalized to printable ASCII, the collection number
is the source ordinal plus one, and the normal Force cost comes from lane 0.
The tribe key preserves the numeric source tribe without guessing a label.
The default `normal` means source variant byte **0**, not supported gameplay.
The six source inventory lanes don't prove six usable appearances for every
Borg; no non-normal variant is declared by this baseline. It records
unresolved icons, gameplay data and capabilities without inventing a controller,
stats, colour overrides, costs for unverified variants, or a data-derived
playability claim. `--list-borgs` is the expected check: `--check-borg-catalog`
refuses the intentionally unresolved editor icon, and preview, practice and
Battle Test are not admitted. Keep the local outputs out of Git.

## First population: Double Ninja, normal (`GG4E08:0003/0`)

`gg4e08:0003` remains the same stable definition; only the normal variant has
been reviewed for this package. AFS member 488 (`pl0003.pzz`), PZZ slot 4
(normal wrapper) and slot 3 (local motion bank) supply its native body preview.
The shared whole-body preview reads raw bindings `0:0`, `0:1`, and `0:2`.
`idle` is the preview's required starting clip label; the three samples are
not claims about movement dispatch or upper/lower lanes in battle.

The source catalogue verifies #18 and cost 210. The original normal HP display
rows give 150/195 at levels 1/10, and the member's authored specification
gives DEF/SHT/ATK/SPD `2/3/3/5` plus four move display slots. The
[archived Double Ninja profile](https://gotchaforce.fandom.com/wiki/Double_Ninja?oldid=6397)
supplies their readable English names and displayed air-jump label, not
runtime move behavior. The package uses a labelled generic editor tile: it
does not extract the nameplate and claim it is the editor icon. It includes no
playable rules, non-normal asset fallback, controller, damage, ammo, or
clone behavior.

```sh
python3 -B -m tools.extract.retail_borg_review double-ninja /path/to/catalog.gfbc \
    /path/to/main.dol /path/to/afs_data.afs local/double-ninja-review
```

To install the review, first move the `gg4e08:0003` placeholder **out of**
`borgs/`, then move the new review folder into `borgs/`. Do not install both:
discovery refuses both copies of a duplicate ID. The exporter never performs
the replacement automatically. The native `--check-borg-catalog`,
`--check-borg-moves`, and headless `--preview-borg gg4e08:0003 --borg-variant normal`
commands are the checks for this slice. Battle Test must still refuse.

### Engine handoff: Double Ninja's charged X

The original member proves a charged-X display slot. The supplied Move List
describes extra projectiles, no additional melee damage, and disappearance on
knockdown. The July 2022 wiki names the move **Ninpo Kage Bunshin** and mentions
clones. The user observed mirrored actions and projectile releases without a
second stock debit.

Static inspection of the pinned GG4E08 executable joins model `0003` through
`80056CE0` to installer `8006F60C` and class dispatcher `8006FACC`. It finds
these separate paths. No original execution was run for this follow-up.

| Source path | Finding and limit |
| --- | --- |
| `80071750`, `80071858`, `80071938` | Charged-X state visits keep a clone flag and offset **on the caster**. State 2 is a transitional path; it can shrink the stored vector to zero or follow a phase-derived scalar. State 1 is maintained from a current owner matrix's forward basis times that scalar. The pre-hook normalizes/re-extends the existing vector against owner scale; it is not a fixed model-space attachment. |
| `8006F708 → 80141A40` | The normal model's class initialization requests an owner-linked presentation child **before** X. A failed allocation skips child publication. It does not request a second fighter at the charged-X marker. |
| `80141C30`, `80141DE4 → 80141C8C → 8020B4AC` | The child sets its visibility selector to zero at clone state 0. Its transform reader copies each referenced current owner logical matrix, preserving the basis and adding the **same world translation** to X/Y/Z. It chooses presentation variants for state 2 versus the ordinary active route; related equipment paths also consume the offset. These are owners and data flow, not original draw timing or an independent pose clock. |
| `80055568 → 8006F88C` | The class post-hook runs after the lower and captured-upper visits and clears clone state on `5E0 & 0x02000000`, without zeroing the vector there or freeing the child. `8005C290` can set the bit on its gated recovery path; the Move List associates disappearance with knockdown. Which accepted hit receipts set it, reactivation in the same update, and the first rendered disappearance are not verified. |
| `80070558` | The checked shuriken branch spends channel 0 once. Model `0003` first requests emission selector 0; if its clone flag is nonzero it also requests selector 2, regardless of the primary allocation result. The second constructor path adds the owner's clone offset. |
| `800715F8` | The checked bomb branch spends channel 2 once. Normal model `0003` requests selector 1, then selector 2 if the clone flag is nonzero. The second call does not depend on the outcome of the first. The constructor retains the same caster, with its own selected emission parameters. |

Both extra-release gates accept any nonzero clone state, including state 2.
The steady offset operand is 150 times the owner scale in the pre-hook;
the retract branch multiplies the carried world vector by 0.92. The
extra shuriken descriptor uses owner logical key 7 and local `(-50, 0, 20)`;
the extra bomb uses key 11 and `(55, 0, 0)`. The constructors add the
separately tracked world vector after the owner-local transform. These raw
operands do not prove the native clip frame, coordinate conversion, or
clock-to-update cadence.

Those B/X paths support the single-debit claim for the checked routes, not
every context or projectile. Clone appearance quality, collision or
targetability, post-creation damage credit, shutdown ordering, and live
timing remain unverified. This path supports an owner-derived render
instance, not a second independently simulated fighter.

The [roster Battle Test](retail-roster-extraction.md#try-the-opt-in-authored-b-shot-battle-test)
admits only the ordinary B shot with clone state inactive. It binds the
exact local throw clip and first joint socket but uses an authored
release marker; it does not implement any of the charged-X behavior above.

The following-emitter API requires a **successful primary allocation** and
uses a fixed owner-relative mount. The new `release-extra` handoff separates
the allocation attempts and preserves the single debit, but does not gate
the extra by clone state or offset the release. The next contract should
prepare an owner-owned child representation, maintain an inactive/transition/
active state and world offset from accepted simulation data, and publish the
duplicate from the owner's accepted logical pose. The eligible extra emission
must sample the **simulation pose and offset**, not its last draw or
successful primary allocation. Keep visual visibility, release eligibility,
knockdown clearing, defeat/round retirement and any future contact separate.
Do not copy Sasuke's behavior because the normal motion-bank bytes match.

Next source check: a bounded join of charged-X activation, the active pose
publication, and a status-driven clear, before qualifying any clone lifetime
or presentation. A separate allocation/refusal check should observe both
release attempts and the single debit rather than infer success from the
first constructor call.

## Second population: Gatling Gunner, normal (`GG4E08:0102/0`)

The original `pl0102.pzz` is AFS member 500; PZZ slot 4 (normal wrapper)
and slot 3 (local bank) supply raw native previews `0:0`, `0:1` and `0:2`.
They are inspected whole-body samples, not a controller map or a weapon cue
timeline. The source catalogue and authored specification give #116, cost
420, long-range role, HP 250/295 at levels 1/10 and DEF/SHT/ATK/SPD `3/6/0/4`.
The two move slots read **Special Gatling** and **Hand Grenade** in the
original English specification. The normal level rows give Special Gatling
100/100 stock at levels 1/10. This is review metadata, not live stock
configuration. Grenade stock and behavior are not admitted.

The [archived profile](https://gotchaforce.fandom.com/wiki/Gatling_Gunner?oldid=6629)
reports an Air Jump level of 3, but the checked source specification does not
display a jump-count glyph for this Borg. The package leaves the displayed
jump field unresolved; neither source provides a checked live movement map.
There is only a normal variant preview and a labelled generic icon.

```sh
python3 -B -m tools.extract.retail_borg_review gatling-gunner /path/to/catalog.gfbc \
    /path/to/main.dol /path/to/afs_data.afs local/gatling-gunner-review
```

Install it by moving the `gg4e08:0102` placeholder out of `borgs/` first,
as with Double Ninja. The native catalogue, move and preview checks use
the ID `gg4e08:0102`; Battle Test must refuse B and X along with the actor.

### Engine handoff: Gatling Gunner's held B

The [360 bounded source checks](borg-controller-audit.md#gatling-gunner-held-b-gate)
establish a held-B repeat gate checking a repeat clock and available stock
without spending it. The subsequent shot waits for a positive pose cue and
aim approval or a local timeout, then performs one channel-0 debit. The
checked normal level-1 stock row seeds a 480-source-unit refill on depletion;
the source-unit to live-update relationship is not established here.

Static follow-through joins model `0102` to installer `80134D68` and its
dispatcher `80134FF8`. The neutral repeat-tail branch (`80135408`) visits
the clock and check-only stock; the subsequent neutral cue route
(`80135954`) enters the separate paid function (`80135B10`), which requests
emission selector 29 only after the stock visit accepts. Do not extrapolate
that neutral route to the alternative action contexts. For X, the static
positive-cue branch around `80135D38` spends channel 2 and requests grenade
selector 4, but that does not qualify its complete timing or contact rules.

`held-fire` in the [current Battle Test API](borg-gameplay-api.md#variable-rate-held-fire)
uses its declining interval to attempt payment and emission directly. `repeat`
does separate a stock query and later release marker, but restarts on a fixed
action boundary and cannot wait for the distinct qualified cue and aim/timeout
branch. The missing contract is an engine-owned pending repeat request with
separate eligibility, authored action cue, and bounded aim/timeout admission;
only its accepted release may pay and request a projectile. Pending-cue
cancellation policy and actual cadence, including quick release/re-press,
need separate witnesses before binding retail values. The supplied Move List
also reports no receiver invincibility from these shots. That is a
hit-recovery lead, not evidence to disable immunity or hit reactions in the
package. Grenade release, prop contact and blast rules still need separate
verification beyond that static cue branch.

## Reviewed batch: Revolver Gunman and Death Borg Beta II

| Normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Confirmed B stock L1/L10 |
| --- | --- | --- | --- | --- |
| `GG4E08:0100/0` Revolver Gunman, AFS 498 | #21 / 250 | 150/195 | 2/4/2/4 | Fast Shooting 20/20 |
| `GG4E08:0105/0` Death Borg Beta II, AFS 503 | #79 / 100 | 50/68 | 1/2/0/3 | Shot 5/14 |

The packages preserve each original member's normal body, local bank,
display names and normal level rows. Both support inspection samples
`0:0`, `0:1`, `0:2` and `3:0`; their separate controller rows record the
reviewed type-0, first-32 selector cohort with contact and actual air
resource policy unresolved. The raw bank's group numbers and the class
program request's group numbers are not interchangeable: host preview
refuses raw `2:4` on Gunman and `2:0` on Beta II, even though the original
class selects those **program** requests at entry.

The [44 bounded source and 114 detached native checks](borg-controller-audit.md#revolver-gunman-and-death-borg-beta-ii-shared-movement-distinct-shots)
establish fresh conditioned B → class selection **without debit** →
positive signed program event → one channel-0 debit → constructor request.
A repeat check only observes availability. Gunman's selectors 0/1 and
Beta's 56/57 must not be shared by analogy. The basic native `repeat` and
marker `release` have the right high-level debit split. The missing
retail binding is a qualified program-to-current-pose/event path and the
per-side projectile handoff to portable assets, launch, and accepted
contact. Do not assign a guessed frame or use an authored Scout's hit policy
as that evidence. The 120/60 refill values are source units, not established
live tick durations. Other B contexts, X, melee and hit/damage remain
unadmitted; Beta II's initial-AI evidence does not qualify human control.

## Reviewed batch: Normal Knight and Normal Samurai

| Normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Qualified review scope |
| --- | --- | --- | --- | --- |
| `GG4E08:020A/0` Normal Knight, AFS 519 | #26 / 180 | 180/225 | 4/1/3/4 | Shield Slash owner channel-2 capacity 1/1; no live guard or damage |
| `GG4E08:0700/0` Normal Samurai, AFS 606 | #39 / 300 | 220/310 | 2/2/4/5 | Ordinary close-B and neutral X registration, no live damage |

Both have separate controller reviews with the checked three-use jump
resource; this is distinct from the catalogue display fields and does not
qualify all movement settings. Knight previews raw `0:0`, `0:1`, `0:2`,
`4:0`. Samurai previews raw `0:2`, `3:0`, `4:0`. The host refuses Samurai
`0:0` and `0:1` with unsupported AObj flags/object/track binding. `0:2`
is the explicitly selected starting preview sample, not gameplay idle or a
fallback. Neither definition declares a playable controller, another variant,
or a qualified motion-program mapping.

- **Knight Shield Slash:** The [169 bounded offline and 106 detached checks](borg-controller-audit.md#normal-knight-and-normal-samurai-shared-movement-distinct-melee)
  distinguish a negative program event arming the resident shield from a
  positive event that spends stock and requests moving hit spheres. The
  directional `guard=` cone is not that shield plane. The contract needs
  separate registration/lifetime for an owner-bound plane and moving
  shapes, a signed cue that does not spend on arming, and a separately gated
  owner-stock restore. Do not infer reflection or blanket allocation refund.
- **Samurai Nitoryu and Rolling Slash:** Close B uses conditioned/cached
  context to select an action, not to authorize a hit. A lane-gated
  program command requests one actor-owned hit group without charging
  the first strike; a later opcode-17 cue is visual only. The
  [neutral-X comparison](borg-controller-audit.md#demon-samurai-and-normal-samurai-shared-slash-different-shapes)
  separately verifies program `4:1` requesting one sphere on Samurai,
  not Demon Samurai's three capsules. The current body-forward `melee=`
  proxy does not express typed, pose-owned, lane-event shape groups.
  Require independent visual cues, accepted current pose, shape activation
  and a contact receipt from the collision owner. End-to-end hit/receiver
  timing and interruption are still unqualified.

All four regenerate with `tools.extract.retail_borg_review`:

```sh
python3 -B -m tools.extract.retail_borg_review normal-samurai /path/to/catalog.gfbc \
    /path/to/main.dol /path/to/afs_data.afs local/normal-samurai-review
```

Other keys: `revolver-gunman`, `death-borg-beta-ii`, `normal-knight`.
Replace only the matching placeholder, as above. `--check-borg-controllers`
also validates the non-executable review rows. Battle Test and undeclared
variants must still refuse.

## Reviewed batch: Normal Tank and Battle Girl

| Normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Package |
| --- | --- | --- | --- | --- |
| `GG4E08:0C00/0` Normal Tank, AFS 666 | #64 / 450 | 300/390 | 4/5/0/3 | Catalogue and controller/move review only |
| `GG4E08:0300/0` Battle Girl, AFS 525 | #34 / 230 | 130/175 | 2/3/2/6 | Same review facets, plus raw previews `0:0`, `0:1`, `0:2`, `4:0` |

The [320 bounded source and 170 detached native checks](borg-controller-audit.md#normal-tank-and-battle-girl-shared-metered-routes)
verify common handlers but not a common controller: the source movement
type differs (Tank `1`, Battle Girl `0`). Both check previous cached
grounding for refill rather than refilling at the floor snap. A lower-lane
control gates the ground release clock (10 source units for Tank, 20 for
Battle Girl); the active air-dash latch (60/30) opens a possible transition,
not an unconditional expiry. Depletion can enter recovery earlier. The
verified ordered ascent/dash costs are `200/90/150/50` versus
`150/50/450/50`. Neutral B retains Tank's lower selector; Battle Girl
replaces both lanes. The package metadata records the shared boost-meter
family and grounded recovery, not the original timing or floor policy.

**Movement API gap:** A portable controller needs a declared, per-actor
meter profile consumed by the shared accepted movement/contact owner, a
lower-program-gated ground timer, explicit air release/landing eligibility,
and distinct upper-only/both-lane action requests. The existing authored
grounded or flight mode is not this qualified lane/contact sequence.
Detached scalars alone don't publish a live floor result or compatible
pose. B and X remain non-executable on both.

**Tank presentation gap:** The exact normal wrapper's record 0 (root
`0x104c8`) enumerates 32 ordinary POBJs. Published record 1 (root
`0x1358c`) has a POBJ at `0x13564`, flags `0x9000` (shape type `0x1000`)
and a binding at `0x13548`. Offsets are relative to the HSD data block.
The current shared skin loader refuses the archive with
`shape/unknown POBJ type requires separate evaluation` before it can
preview any compatible clip. No source bytes were removed. The engine
must either validate and bind that record's shape evaluation in the shared
record/attachment owner, or, for preview only, validate explicit selection
of record 0 after proving record 1 is not required by that preview path.
This says nothing about whether a live Tank can omit record 1.

Use generator keys `normal-tank` and `battle-girl`. Tank's generated
manifest declares no model, bank, or preview path. Its controller review
notes the original member's local bank without claiming the package
provides it. Both must refuse Battle Test.

## Reviewed batch: Magnet Robots and Thunder Robot

| Exact normal source | Catalogue / Force cost | B-shot capacity L1/L10 | Other inspected move names |
| --- | --- | --- | --- |
| `GG4E08:0405/0` Magnet Robot (+), AFS 544 | #61 / 400 | Machine Gun 10/10 | Magnet Arm, Magnet Scope |
| `GG4E08:0409/0` Magnet Robot (−), AFS 548 | #62 / 410 | Beam 4/4 | Magnet Arm, Magnet Scope |
| `GG4E08:0404/0` Thunder Robot, AFS 543 | #60 / 400 | Thunder Beam 4/4 | Thunder Claw, Thunder Wire |

All three have HP 350/440 at levels 1/10 and authored DEF/SHT/ATK/SPD
`4/3/5/4`. Each uses its own pinned original member. The static installer
join (`800DA9FC` for the Magnets, `800D7C7C` for Thunder) exposes movement
type byte 0 and identical first-48 selector pairs across the two tables;
the package label `machine-04` names only this comparison. It does not
claim shared movement, resource or action logic. The boost-jump label
comes from the archived descriptions, not a live controller binding.

All three normal whole-body previews sample `0:0`, `0:2`, `3:0` and
`4:0`. The host explicitly refuses `0:1` with unsupported AObj
flags/object/track binding. All 20 bound AObjs in that exact binding carry
`0x20000000` (Thunder motion archive at bank offset `0x1040`, both Magnets
at `0x1060`). The shared sampler currently requires zero flags. Preserve
the refusal until that flag's lifecycle is qualified; the generated preview
set does not declare or silently substitute `0:1`. Each package remains
nonplayable and has no alternate variant fallback.

### Magnet Scope: keys and polarity are distinct

The normal source level rows give both versions 1/1 display stock for X.
Static follow-through of `800DB0C8` finds a positive cue cleared before
a channel-2 debit; only an accepted debit reaches `800DD47C`, which
requests a type-45 object with key **0** for `0405` and **1** for `0409`.
Neither key alone establishes the effect's polarity.

The [bounded type-45 audit](borg-attack-visual-recovery.md#movement-contact-and-attraction-leads)
observed both keys requesting inward movement in the supplied unit-scale,
identity-pose path. The six-slot actor scan excluded the caster, used a
sphere 2,000 forward from the sampled emitter socket with radius 2,000,
and separately checked the 150-unit minimum from the socket. The accepted
on-axis request was magnitude 25 before the receiver's own placement
and contact path. This did not enumerate projectiles. The player's
observation is **plus attracts, minus repels**; it remains a recollection
in conflict with the narrow inward witness for minus. Neither the guide's
actor-pull descriptions nor the authored polarity option settle that.

**API gap:** The current field primitive offers `away` as an authored
grounded pull mode, but limits the step to 16, uses the two-actor test,
and does not implement the source receiver's 3D accumulator/consume flow.
A retail binding needs a pose-sampled **separate detection volume and
effect origin**, a bounded eligible actor roster with owner exclusion,
and a typed 3D request consumed by each actual receiver's movement and
contact owner. Do not scale the retail operands just to fit the authored
limits or choose a final polarity for minus without a resolving witness.
Activation, direction, interruption and stock return are still separate.

### Thunder Wire: retained endpoint vs fixed points

The source catalogue calls `0404` **Thunder Robot** and the original
specification calls its X **Thunder Wire**. There is no separate
“Lightning Robot” catalogue ID. This is the evidence-backed candidate for
the recalled fence, not proof that the recalled mechanics belong to it
exactly. The [archived Thunder Robot profile](https://gotchaforce.fandom.com/wiki/Thunder_Robot?oldid=6730)
instead describes a wire attached to an ally.

Static inspection of the positive X cue (`800D8470`) finds a channel-2
debit before the type-43 request (`800D9FBC`). The factory's allocation
failure calls `8006DE10` to return one unit to that channel, capped at
its capacity. In the linked path `800DA374`, the object reads a
contact-side reference and retains it after a mode gate; it selects
stored lifetime operands 1200 or 2 on same/different team comparison.
`800DA6B8` refreshes the caster socket and the contacted reference's
logical point separately; an ineligible reference or an exhausted retry
path ends that stage and requests stock return. These are static source
paths, not observed live linkage, time in ticks, or proof that the
contacted object is always an allied fighter. The review leaves X
stock timing, damage and connectivity unresolved.

**API gap:** `anchor-fence` and `fence-recoil` are authored fixed-point
operations. The observed source branch needs an owner-held link with a
contact-qualified endpoint identity, current pose samples at both ends,
separate arming and distance/eligibility gates, and explicit retirement
and stock-return reasons. Link overlap, damage cadence and push must
consume accepted contact receipts; no authored fence hit or render line
establishes those rules. Verify the stationary/retraction branch and any
actual point-to-point route separately before deciding whether both
policies belong to the retail move.

Regenerate with keys `magnet-robot-plus`, `magnet-robot-minus` and
`thunder-robot`; do not install a second package named “Lightning Robot”.

## Reviewed batch: Kung-Fu Master, Angel Nurse, Patra Witch

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Recorded normal stock L1/L10 |
| --- | --- | --- | --- | --- |
| `GG4E08:0800/0` Kung-Fu Master, AFS 620 | #41 / 260 | 180/270 | 2/1/4/7 | Spirit Shot 5/5 |
| `GG4E08:0900/0` Angel Nurse, AFS 634 | #47 / 240 | 150/195 | 2/1/1/3 | Nurse Shot 10/10, Healing 1/1 |
| `GG4E08:0902/0` Patra Witch, AFS 636 | #48 / 150 | 150/195 | 2/1/1/3 | Magic Shot 10/10, Big Spell 2/2 |

Each package uses its own original normal wrapper and local motion bank.
Kung-Fu Master previews `0:0`, `0:1`, `0:2` and `3:11`; Nurse and Patra
sample `0:0`, `0:1`, `0:2`, `4:0` and `4:1`. Raw samples are not
program execution. The `moves.txt` sidecars record the verified action
phases and recipient roles, not a gameplay profile or an event evaluator.
Other attacks and all non-normal variants remain unadmitted.

### Kung-Fu Master: next candidate for a narrowly verified shot

The [185 bounded source and 121 detached native checks](borg-controller-audit.md#kung-fu-master-close-b-priority-and-paid-spirit-shot)
distinguish a near-context candidate from the accepted shot. Close B
has priority in its reviewed context, but a near classification never
proves contact. The selected neutral shot claims both lanes, checks
stock without spending, and later processes event `(3,1)`. This event
attempts one debit and advances the subphase even on refusal. A later
signed `-1` is recovery, not another launch.

Static inspection joins debit acceptance at `80076C04` to projectile
selector 12 of `80082824`. Its selected descriptor at `802D7B10 +
56 × 12` gives logical key **7**, local origin `(0,0,0)` and basis
column **0**. The constructor's ordinary branch consumes those via
`80082B78`. This establishes an initial socket/basis operand, not full
aim, transformed direction, successful allocation, damage, or contact.
Do not assume the default straight/targeted aim policy reproduces it.

**API gap:** Live package stock needs an explicit bounded mode-1
**add-on-debit** policy. Each accepted debit adds a preset to the running
clock; the shared refill helper restores intervening counts from the
remaining clock and preserves a negative remainder on crossing zero.
The new `stock-clock=id|add-on-debit` handoff expresses the arithmetic,
not the source's per-visit update rate. Preserve read-only availability
versus debit and keep the clock separate from projectile life; both
observed presets are 300 source units, not interchangeable native ticks.
`entry-stock=move|stock|minimum` now supplies the separate read-only
availability check. It grants neither a later paid release nor a hit.

### Spirit Shot follow-up: program joins clip; contact remains open

Static source inspection follows the normal neutral setup
`80076AD8 → 8004BEB8`: group/entry `2:0`, mask 15 for the request,
no frame override. The installed local program root `802C1C40`
resolves to `802C1F20`. Descriptor `2:0` at `802C1B0C` resolves to
local raw clip **`3:11` on both reviewed lanes**, with start operand 0,
mode 2, and blend-duration operand 4. The program after a flags-2
wait with threshold **9** writes `(3,1)` through upper controls at
`802C1F3C`. A later flags-2 threshold **31** writes signed recovery;
following completion it requests the separate base descriptor. These
are static program links and clock operands, not elapsed native ticks.
The added `clip-3-11` preview proves the raw clip can be sampled, not that
it produces the cue alone or at a specific displayed frame.

For the recipient join, selector 12 is an ordinary type-0 allocation from
`80082824`. Its initializer samples the owner key-7 basis, then runs its
own aim/normalization and shape-registration path; the descriptor supplies
registration selector 1 and independent axis dimension operands
`0.75/0.75/0.75` before owner scale. The active callback at `80082FC4`
has an already-latched-contact retirement check (`80083028`) separate
from movement/static-segment work. Thus the accepted hit cannot be
inferred from allocation, launch aim, lifetime or the carrier retiring.
The next scoped check needs registered shape ownership, an actual overlap
with an eligible ordinary recipient, class/row acceptance and the
resulting damage/reaction receipt, with a miss/refusal control. The
required supported movement mapping and same-update pose publication
also remain open. No executable package is installed.

### Angel Nurse and Patra Witch: same input, different owners

The [39 bounded source and 55 detached native checks](borg-controller-audit.md#patra-witch-and-angel-nurse-distinct-x-contact-outcomes)
establish distinct paths despite an audited shared movement prefix.

- **Healing:** Nurse's fallback requests program `4:0` and attempts a
  channel-2 debit during setup. A lane-gated command requests owner-bound
  shape group 3; the later visual event cannot synthesize its contact.
  The checked same-team ordinary recipient row heals **200 HP**, capped
  at that recipient's own maximum, preserves previous HP, and refuses
  zero HP, unsupported recipient mode, and the tested global inhibit.
  **API gap:** independent setup payment and a pose-owned support-contact
  producer, a qualified living ally receiver with a real contact receipt,
  and a bounded vital write. The two opposing actors in Battle Test are
  not a same-team test; a self-heal or damage with a negative number is
  not the new behavior.
- **Big Spell:** The checked Patra program `4:0` consumes exactly event
  pair `(1,1)`. A successful channel-2 debit requests a caster-owned
  type-60 carrier. Its qualified row-5 contact can request a type-33
  effect on the **actual contacted actor**, not on the press-time target
  hint or caster. **API gap:** a signed cue-to-paid carrier path and a
  second, contact-driven recipient-owned effect registration. Actual
  size mutation, refresh, duration and full team eligibility are not
  qualified; do not use a walk-speed modifier or assume enlargement or
  shrinkage solely from the move label.

Regenerate with keys `kung-fu-master`, `angel-nurse`, and `patra-witch`.
`--check-borg-moves` validates the phase/outcome shape without applying
stock, collisions, or recipient effects. Battle Test still refuses all
three.

## Reviewed batch: Acceleration Ninja and Pop Honey

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Preview samples |
| --- | --- | --- | --- | --- |
| `GG4E08:0004/0` Acceleration Ninja, AFS 489 | #19 / 380 | 200/245 | 3/3/4/9 | `0:0`, `0:1`, `0:2`, `4:0` |
| `GG4E08:0904/0` Pop Honey, AFS 638 | #50 / 250 | 150/195 | 2/1/1/3 | `0:0`, `0:1`, `0:2`, `4:0`, `4:1` |

The generator keys are `acceleration-ninja` and `pop-honey`. Both use
their exact normal body and bank. Their review sidecars record verified
move-phase boundaries, not runnable behavior:

- **Ninpo Full Throttle** (`0004/0`): [146 source and 119 detached checks](borg-controller-audit.md#acceleration-ninja-steerable-stance-with-actor-bound-contact)
  verify a stockless stance on the existing actor, with startup program
  `4:0`, conditioned steering and a separate trail request. A lane
  command requests two colocated-with-actor group-10 shapes; animation
  progress gates activation. The ordinary air dash uses separate group 11.
  Fresh X after lockout, timeout, and insufficient movement can request
  recovery. **API gap:** a steerable owner stance feeding normal contact
  and explicit pose-clock shape registration/retirement. Neither a new
  form, a timed forward `melee` proxy, nor the trail establishes live
  contact. No final damage or same-update interruption ordering is admitted.
  A later authored Battle Test (`tools/analyze/retail_charge_field.py`)
  expresses the stance as the native dash plus an owner-following field:
  the field stays on the actor like the recovered colocated shapes, but its
  radius, damage, interval and lifetime are authored, and it does not
  implement animation-progress shape registration/retirement. The
  geometric contact and final damage remain unclaimed.
- **Lovely Voice** (`0904/0`): [138 source and 101 detached checks](borg-controller-audit.md#pop-honey-contact-delivered-targeting-status)
  distinguish a paid setup from a later wake of a preallocated carrier
  and its timed type-62 note request. Row-5 note contact applies/refills
  **641 eligible receiver updates** on the actual contacted actor. The
  checked targeting path clears its target snapshot and swaps sectors
  2/8; an audited pose path offsets drawing yaw without rewriting actor
  heading. This is not full control inversion. **API gap:** independently
  owned carrier wake, elapsed note request, and contact-driven target
  status with a receiver-owned refresh/pause/expiry clock, plus a
  separately qualified pose offset. A shot-speed or movement modifier
  cannot substitute; live carrier/recipient joining and full eligibility
  remain unresolved.

No one in this batch is admitted to Battle Test or another variant.

## Reviewed batch: Roach, Death Bomb, and Blue Striker

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Recorded stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0F05/0` Roach, AFS 691 | #189 / 30 | 60/78 | 1/1/0/2 | Bomb 1/1 at normal L1/L10 |
| `GG4E08:0D07/0` Death Bomb, AFS 679 | #108 / 20 | 30/39 | 1/0/3/5 | Reviewed stockless X |
| `GG4E08:0D00/0` Blue Striker, AFS 672 | #70 / 200 | 150/168 | 2/4/0/10 | Vulcan 40/40, Missile 4/4 at normal L1/L10 |

The exact normal wrappers validate as static body previews. Slot 3 of all
three source bundles is actually empty: the generated manifests declare no
bank or gameplay and use `idle=none`. This is not a zero-frame synthetic
clip, an actor that does not move, or an implied empty class controller.
The generator refuses a substituted nonempty bank for these presets.

### Roach: state updates do not require a local clip

The [167 original and 89 detached cases](borg-controller-audit.md#roach-one-use-jump-control-only-programs-and-bomb-stock)
separate takeoff from the accepted ascent that spends the only use,
and floor snap from later cached-grounded bookkeeping that restores it.
The ordinary ground and air dash requests refuse without spending the
flick. Source lane programs still issue controls and waits without a
local clip or descriptor root. `grounded-v1` charges at native takeoff,
already owns different dash/contact behavior, and cannot run that
verified control-only path. **API gap:** admit a bounded control lane
and its clocks independently of pose assets, with explicit per-family
request/debit/reset gates; do not invent an idle descriptor or active dash.

The [same-carrier bomb audit](borg-controller-audit.md#roachs-bomb-timer-and-proximity-are-separate-exits)
separates paid type-16 allocation, `300`-count timer or contact latch,
`25`-count inner phase with a separate area-key-26 request, the 3D
six-slot proximity exit, and capped return to the credited owner before
physical release. The scoped proximity callback admitted distances
through **150** and can alter a travel operand outside that radius;
it did not establish the same blast as timeout/contact. Current
`bomb-proximity`/`bomb-attraction` operate on a landed supported bomb
and a single opponent proxy. **API gap:** independent, roster-filtered
3D sensing/motion and separate exit-specific area, return and retirement
requests. Do not turn the source return into timed refill or grant its
area damage from the request alone.

### Death Bomb: descent, impact allocation, and defeat are independent

The [211 original and 191 detached cases](borg-controller-audit.md#death-bomb-steerable-fall-and-stockless-drop)
join the ordinary fall controller, a distinct stockless descent on fresh
X, and the independent late contact gate. The original specification
uses **Fall**; the archived profile calls X **Drop**. The recorded
3,000 floor-relative units are a **pre-projection** entry operand,
not a verified native spawn clearance or final Y. Entry code 2 selects
fall separately. The late gate requests **type 5 / argument 17** and
queues **self-defeat even if allocation fails**. Ordinary impact is not
an X input side effect; already committed defeat refuses a new request.
**API gap:** a stage-owned floor/placement receipt and distinct
steerable fall and Drop controls, followed by qualified contact that can
independently request an area object and one self-defeat. A self-hitting
authored blast cannot replace that ordering. Final impact recipients,
allocation/defeat ordering in live play and effect life are unresolved.

### Blue Striker: boost inactivity is not grounding

The [294 original fragments and 301 detached comparisons](borg-controller-audit.md#normal-blue-striker-cruise-boost-and-hud)
separate cruise from boost and pre-selection refill from the accepted
boost step. A fresh airborne press and positive meter can enter boost.
The `30`-source-unit phase opens a release/depletion exit latch, not
an automatic state transition. Cruise entry clears the boost flag;
the earlier refill pass can still see that flag on the same visit.
From delay 16, the `1 → 0` visit still does not add fuel, and the next
eligible inactive visit can refill. A sampled floor snap did not select
landing or refill. **API gap:** a qualified continuous-flight controller
profile with per-move debit, a separate release latch, pre-selection
not-boosting recovery, and an owner meter readout. Do not route it
through a ground-jump or G-Red budget. Pose/trajectory, live contact and
weapon lifetimes are not joined; the display stock rows do not bind a shot.
The authored `cruise-boost` handoff exercises inactive-refill arithmetic.
It does not qualify this actor's pose, trajectory, source clock or floor
continuation, and it does not stand in for the Wing/Quick glide rules.

Regenerate with keys `roach`, `death-bomb`, and `blue-striker`.
All three pass `--check-borg-catalog`, `--check-borg-moves` and
`--check-borg-controllers`, and remain unadmitted to Battle Test.

## Reviewed batch: Wing Soldier, Quick Valkrie, and Beam Satellite

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked presentation and stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0A00/0` Wing Soldier, AFS 647 | #52 / 200 | 150/195 | 2/2/2/8 | `0:0`, `0:1`, `0:2`, `0:12`; Arrow Shot 10/10 |
| `GG4E08:0B00/0` Quick Valkrie, AFS 658 | #53 / 230 | 120/165 | 2/2/2/8 | `0:0`, `0:1`, `0:2`, `4:0`; Magic Shot 10/10 |
| `GG4E08:0E01/0` Beam Satellite, AFS 681 | #148 / 800 | 1000/1450 | 9/6/0/2 | Static exact normal wrapper; Reflector Beam 8/8 |

The normal level rows supply capacity endpoints, not weapon behavior.
Each preview uses its own original asset. Wing Soldier's raw `4:0` is
absent; do not look it up in Quick Valkrie's bank. Beam Satellite's local
bank is genuinely empty. It has a static **preview**, not a stationary
Battle Test controller.

### Wing Soldier and Quick Valkrie: floor snap is not automatically landing

The [type-2 glide audit](borg-controller-audit.md#normal-wing-soldier-and-quick-valkrie-glide-and-contact)
matches 282 scoped original and 420 detached comparisons across both
normals. They share movement type 2 and resource policy 2, not a live
pose or class action. Glide 18 can install a continuation for a valid
floor snap to choose landing 7. Steering 19 clears that continuation,
allowing a floor snap with no landing selection. Landing entry clears
logical airborne status and leaves `0x80` intact. Refill runs before
selection; it holds meter and delay while that flag is set and does
not test grounding or ask the floor snap to refill.

Wing Soldier also has a **scheduled** alternate dash witness:
`18 → 22 → 23 → 24`, with the upper lane lagging the lower, the
landing continuation suppressed, and no meter debit on those visits.
The observed recovery pose uses local clip `0:12` at sample 0.
That supports the selected raw review sample, not a full dash clock
or the same binding for Quick Valkrie. Both X rows named **Fly**
remain unresolved; do not infer their entry path from jump/glide.

**API gap:** an accepted locomotion owner that can keep floor snap,
optional landing continuation, and logical air state separate;
per-lane glide/dash requests and waits; and a pre-selection resource
visit with its own inhibit gate. An authored perpetual-flight boost or
ordinary ground jump cannot express this sequence. Next witnesses:
a scheduled steered near-floor snap and continuation refusal, the
recovery tail after sample zero, and each model's own Fly
input → program → refill-inhibit writer.

### Beam Satellite: selected stage placement is not a universal altitude

The [type-9 audit](borg-controller-audit.md#normal-beam-satellite-placement-stance-and-support)
matches 314 scoped original and 790 detached comparisons. Stage code,
team, and retained side select complete XYZ operands **before** stage
projection. Fresh placement advances a team cursor; reused placement
does not. The two raw stage-0/team-0 Y samples (1243 and 399) are not
runtime spawn heights, and the separate floor+2000 fallback does not
choose a table row. None of those readings grants final body clearance.

Idle's return-only phases do not prohibit held steering and a later
movement/braking sequence. The class reasserts logical airborne status,
ordinary landing is disabled, and cache-driven refill is separate from
floor correction. Three body descriptors carrying a support bit are
eligible candidates, not registered rider surfaces.

**API gap:** a stage-owned placement service that can validate
per-stage/team choices and commit the cursor only with accepted fresh
placement; an aerial stance with explicit turn/drive/brake transitions
and logical contact policy; and separate support relations after actual
registration/contact. A stage-centre spawn, fixed height, or
`stationary-v1` is not this behavior. Next witnesses: fresh versus
retained side with live static/dynamic projection and clearance, then
held/released steering with root and air-state receipts. A separate
support-contact witness and both beam lifecycles are still required.

Regenerate with keys `wing-soldier`, `quick-valkrie`, and
`beam-satellite`. Catalogue/controller/move checks do not admit the
packages to battle.

## Reviewed batch: Gold Hero, Titan Robot, and Copy Man

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Package scope |
| --- | --- | --- | --- | --- |
| `GG4E08:0802/0` Gold Hero, AFS 622 | #122 / 420 | 200/290 | 2/2/3/4 | Base raw preview; temporary-entry/expiry form review |
| `GG4E08:0604/0` Titan Robot, AFS 578 | #166 / 1000 | 450/540 | 5/3/5/5 | Metadata only; two-way selected cycle and a flagged return |
| `GG4E08:0806/0` Copy Man, AFS 625 | #44 / 350 | 200/290 | 1/1/0/6 | Base raw preview; donor-contact reload review, no fixed form graph |

Gold Hero and Copy Man preview normal samples `0:0`, `0:1`, `0:2`,
`4:0`; these are not game action maps. The form checker validates
Gold Hero's and Titan Robot's declarations without running their
transitions. Live model, movement, collision and transfer bindings in
both graphs remain unresolved. Neither `0803` nor `0605` is a new
catalogue ID or variant. Retail Titan Tank `gg4e08:0619` is a separate
definition; see its independent review below.

### Gold Hero: paid request, committed entry, deferred return

The [84 bounded source and 187 detached checks](borg-controller-audit.md#normal-gold-hero-temporary-form-and-deferred-return)
verify a successful channel-2 debit staging state 4, then lower-lane
completion gating the rebind prefix from `0802` to `0803` in the same
actor and variant. The powered initializer seeds **1800** qualifying
class visits. An expiry with the guard open can stage a return; otherwise
the negative pending sentinel retries. Return uses state 5 and does not
debit X again. Base/powered air-use limits differ (2/4); stock setup is
rerun. Do not claim complete HP, root, used-count or collider transfer
from the preserved fields in the rebind prefix alone.

**API gap:** a staged same-actor form transaction with payment,
committed lower-lane completion, destination-specific resource validation,
and a separately guarded retry/return clock. Next witnesses: both live
rebind directions with actual pose and collider retirement/replacement,
air-use and stock state, and blocked-then-retried expiry. The review does
not invent a refund or an interrupted-return policy.

### Titan Robot: cyclic selection is not timed expiry

The [43 bounded source and 134 detached checks](borg-controller-audit.md#normal-titan-robot-cyclic-form-and-the-separate-titan-tank)
verify stockless fresh X staging the opposite form from either
`0604` or internal `0605`; both wait for lower completion. The movement
type changes 0 ↔ 1 and destination stock setup runs again. The separate
`0x02000000` branch on internal `0605` requests return; the review does
not label the source status as generic stagger. The other state-1
return leaf still lacks a verified live trigger and priority.

**API gap:** use the same actor transaction but support a persistent
selected cycle with no assumed expiry, a separately qualified status return,
and form-specific movement, rig and collider binding. Independently, the
shared skin loader rejects the exact normal wrapper on record 7:
root `0x1aae4`, POBJ `0x1aabc`, flags `0x9000` (type `0x1000`),
binding `0x1aaa0`, all relative to the HSD data block. Record 0 has
47 ordinary POBJs; no evidence lets a live Titan omit record 7.
The package supplies no model or preview until the required shape
semantics, or a narrowly qualified explicit preview record path, are
supported. Next witness: accepted cycles in both directions with
floor-result and lane completion, then a separately observed flag return.

### Copy Man: contact chooses the destination, not the caster's aim

The [56 bounded source and 95 detached checks](borg-controller-audit.md#copy-man-contact-derived-same-slot-reload)
verify charged X event `(1,1)`, a separate caster-owned type-85 head,
and an accepted ordinary contact path that captures the donor's
**current model and variant**. It queues reload of **Copy Man's slot**,
retaining its own level and saved original identity. The checked
Sasuke donor witness uses the caster's current absolute HP capped at
the destination maximum, not donor HP, a ratio, or restored lost HP.
The **900**-visit copied clock and blocked-expiry retry restore the
saved definition, not a generic base that can replace it.

**API gap:** a contact-qualified dynamic definition selection with
variant/resource admission and atomic same-slot retirement/republication,
while preserving caster ownership and regenerating stock/pose/collision
from a validated destination. A finite static form graph cannot pick a
donor, and the one exact Sasuke witness isn't a donor whitelist. Next
witnesses: accepted contact versus missed/ineligible donor, isolated
outbound and return resource publication including active-object handling,
and blocked expiry before guarded retry.

Regenerate with `gold-hero`, `titan-robot`, or `copy-man`.
Use `--check-borg-forms` for the fixed form reviews. All three still
refuse Battle Test and alternate variants.

## Reviewed batch: Flame Dragon, Ice Dragon, and Demon Samurai

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0500/0` Flame Dragon, AFS 553 | #149 / 1500 | 1000/1450 | 8/7/2/1 | Flame Bomb 5/5, Flame Breath 1/1 |
| `GG4E08:0516/0` Ice Dragon, AFS 573 | #113 / 1050 | 500/680 | 5/6/1/1 | Ice Bomb 5/5, Ice Breath 1/1 |
| `GG4E08:0701/0` Demon Samurai, AFS 607 | #186 / 480 | 220/310 | 3/3/6/5 | Soul Ball 1/1; audited neutral X is stockless |

Both Dragons sample local `0:0`, `0:1`, `0:2`, `4:0` with their own
normal wrappers. Slot 3 happens to hash identically across the pair;
that neither licenses exchanging body wrappers nor proves matching
movement or damage. Demon Samurai samples `0:2`, `3:0`, `4:0`; exact
raw `0:0`, `0:1`, and `4:1` are refused by the shared AObj binding
check. The explicit preview start `0:2` is not an inferred idle role.

### Exact takeoff and beam seams

The [268 original and 242 detached Flame Dragon movement cases](borg-controller-audit.md#flame-dragon-type-4-takeoff-and-airborne-dash)
verify type 4 and meter policy 1 for **Flame**. Both lanes can request
selector 4; program completion, not startup, hands off to ascent.
Ground flick dash refuses without consuming its direction; the airborne
path separately pays wind-up handoff/active costs. The exact known
ascent entry/update and air-dash handoff/active costs are
`100/25/200/15`. Cached grounded state, not inactive boost, gates
refill; a floor snap without a continuation does not select landing.
The source pre-projection +1000 operand is not a portable final height.
Ice has no controller review row in this batch; it does not inherit
Flame's movement from a matching bank or installer.

The [type-46 and type-47 attack audits](borg-attack-visual-recovery.md#steerable-sustained-beams-dragon-borgs-and-ultimate-cannon)
separate B bomb contact/terrain tails from X breath contact children.
The supplied terminal B path requests area key **3** for Flame and
**23** for Ice, without the additional type-30 requests verified on
other Dragon rows. The X follow-up has a stockless setup cue, paid
branches and a separate hold-budget transfer. It does not settle the
ordinary-X versus charged-X coverage or active cadence. On the checked
key-0 substate, the visual requests a type-46 contact child **before**
advancing its own angle; the child samples the previous angle and
logical key 12 with local `(0,150,30)` for Flame or `(0,75,15)` for Ice.
Shape refusal or parent phase 2 leads to teardown requests. A child
request and a visible beam do not establish accepted damage.

**API gaps:** a program-completion takeoff and ordered air-meter/landing
controller for exact Flame, plus a phase-scoped visual owner and
independently validated contact child that samples the appropriate
accepted socket and retires with its parent. Authored `sustained-beam`
and `impact-children` do not represent this exact lifecycle or prove
damage/steering. Next witnesses: a live startup/completion/contact
movement join, and per-Dragon X cue/hold selection and allocated child
overlap on an eligible receiver versus a blocked or refused shape.

### Demon Samurai: visible orbit, shape contact, and award arrive separately

The [249 original and 142 detached slash cases](borg-controller-audit.md#demon-samurai-and-normal-samurai-shared-slash-different-shapes)
verify neutral X program `4:1` requesting **three** group-7 actor-bound
capsules on Demon Samurai, unlike Normal Samurai's sphere. The word
“Random” in the display name does not prove RNG. The separate
Soul Ball follow-up found one debit for two indexed orbit children
and confirmed a return path that clears only the returning child's
flag. A distinct owner hook restores stock when **both flags are clear**;
an outbound allocation refusal also clears its flag. Do not require a
hit or use one generic retirement credit per object.
The `release-return=move|frame|all-retired` handoff being tested covers
a single debit with a return token for two accepted carriers. It does
not qualify allocation at orbit initialization, later orbit-to-flight
handoff, flag clearing on a refused outbound allocation, or the retail
owner hook.

**API gaps:** pose-clock-owned multi-shape groups, and an owner-scoped
two-child activation/return barrier independent of a hit. The separate
blade growth lead needs a qualified defeat observer and a successfully
arriving child before applying a capped stat; the source scalar does not
prove any growth in visual length, collider reach or damage. Next
witnesses: full paid orbit-to-outbound-to-return ordering including
one-leg failure/interruption; accepted pose-owned slash contacts with
real recipients; and a joined blade-arrival and geometry result.

Regenerate with `flame-dragon`, `ice-dragon`, and `demon-samurai`.
All three remain review-only and refuse Battle Test.

## Reviewed batch: Shuriken Ninja, Beam Gunner, and Neo G-Red

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0001/0` Shuriken Ninja, AFS 486 | #16 / 310 | 200/245 | 2/2/6/5 | Shuriken 30/30 |
| `GG4E08:0104/0` Beam Gunner, AFS 502 | #154 / 650 | 300/345 | 3/6/2/6 | Beam Shot 5/5, Giga Beam Cannon 1/1 |
| `GG4E08:0629/0` Neo G-Red, AFS 604 | #14 / 500 | 400/490 | 5/5/6/6 | Beam Gatling 10/13 |

Each package has six raw preview samples on its own exact normal
art. None claims the raw preview samples are selected by an input
controller. Beam Gunner's raw `0:2` archive (local bank offset
`0x2e00`) has non-null AnimJoint constraint pointers on logical
slots 5, 6, 7 and 10. The shared sampler refuses that bank entry;
it is omitted, not patched or replaced behind the same label.

### Shuriken Cutter: the two hand records and the mounted child are separate

The [Cutter presentation audit](borg-attack-visual-recovery.md#shuriken-ninja-cutter-hand-parts-and-mounted-child)
joins a supplied `P4:0` start to the hand-record change from `0/0` to `2/2`
after the cursor-9 wait. The registered records become `[6,3,0]` instead
of `[4,1,0]`, and hand groups map to logical keys **7/11**. This doesn't
change the authoritative logical pose. The separate mounted child starts
with record **7**, can select record **9** with material clip 0, and
samples logical key **3** with local offsets differing by phase. Its
checked X angle uses a clamped `546`-unit step; the Y angle in applicable
phases integrates a changing step and has its own hold/reset behavior.

`part-swap`, fixed `part-material` sampling and `part-turn` cover portions
of this in authored packages, not the one-to-one same-update transfer
from VM/child events. **API gap:** phase-driven child presentation with
independent mount, rotation accumulation and material selector, without
giving the visual child actor-bound contact. Next witness: one accepted
entry/phase/return sequence with actual scheduler ordering and separate
hit-group registration and contact. Do not use a fixed native frame
derived from the isolated program visit count.

### Giga Beam Cannon: weapon deployment is not beam emission

The [Beam Gunner audit](borg-attack-visual-recovery.md#beam-gunner-weapon-child)
separates `P4:0` setup and a check-only stock gate from later paid
type-32/key-1 emission and a stockless `P4:2` exit. Its separate
weapon child shrinks **record 9/key 7**, switches to **record 10/key 1**,
grows it, and performs an independent return sequence. The beam origin
is key **1**, local **`(0,25,350)`**; its own active callback may correct
the direction toward a copied target and hands segments to type-11/key-10
children. The positive segment producer did not trim on terrain; that
does not qualify downstream target or prop contact or commit timing.

**API gap:** child shrink/switch/grow stages keyed to accepted parent
phase, and a separately qualified beam aim-cutoff and segment/receiver
policy. `part-swap-bone` and `aim-lock` express mount replacement and
committed aim as authored operations without establishing their retail
schedule. Next witness: weapon phase through the paid beam cue, then
accepted segments with both moving/off-line recipients and obstructing
props. No generated profile fixes an aim-lock frame.

### Neo G Crash: a tested lunge opening is not its full steering or hit

The [Neo G-Red comparison](borg-attack-visual-recovery.md#deployment-leads-beam-gunner-and-neo-g-red)
records a stockless `P4:1` request, advancement at authored cursor 40,
and exits on supplied contact or clock. The opening uses descriptor
`4:3` and requests actor-owned group **29**; G-Red's distinct `P4:0`
opening uses descriptor `4:0` and group **15**. The report of a
back-to-hand sword transfer and range-dependent homing is not a measured
socket/steering/contact result. Neo's checked charged B instead requests
type 32/key 15; it is not the sword lunge.

**API gap:** accepted action locomotion with per-context heading
commitment, independent equipment transfer, and current-pose melee
shape registration. Next witness: inside/outside the lunge's range
gate, an accepted transfer, and accepted contact versus timeout with
the real shape/recipient. Do not reuse G-Red's qualified movement pack
or substitute a generic attack proxy.

Regenerate with `shuriken-ninja`, `beam-gunner`, and `neo-g-red`.
The sidecars record the source phase distinctions without applying them.
All three remain unadmitted to Battle Test.

## Reviewed batch: Machine Head, Mechanical Dragon, and Machine Dragon

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Scope |
| --- | --- | --- | --- | --- |
| `GG4E08:0511/0` Machine Head, AFS 569 | #198 / 300 | 250/340 | 3/4/0/4 | Static body, B 10/10 and X 1/1 display stock, fusion participant review |
| `GG4E08:0512/0` Mechanical Dragon, AFS 570 | #199 / 1200 | 1000/1450 | 8/6/1/1 | Normal raw preview, B 5/5 and X 8/8 display stock |
| `GG4E08:050F/0` Machine Dragon, AFS 567 | #200 / 1500 | 1250/1790 | 8/7/1/1 | Separate catalogue body and bank, no battle-result alias |

Machine Head's local motion bank is empty. Mechanical Dragon and the
catalogue Machine Dragon have normal preview samples `0:0`, `0:1`,
`0:2` and `4:0`. Machine Dragon has authored specification text entries,
but all five display-slot selectors in its exact actor record are
disabled (`0xff`). It does not declare a move file: unused text cannot
qualify a live button binding, and missing review is not explicit
`moves=none`.

The [38 bounded original and 53 detached Machine Dragon cases](borg-controller-audit.md#machine-dragon-battle-result-versus-catalogue-entry)
show that two eligible normal owners stage internal primary `0513/0`
and companion `0514/0`, both backed by AFS 571. Do **not** substitute
the standalone catalogue member `050F/0` or create a selectable
package for either internal role. Machine Head's `fusions.txt`
records the exact normal participants, linked readiness and stage-seven
gates, but leaves the result **unresolved** because the current checker
can describe only one result package. It doesn't assign both slots
one controller. No inventory, crystal assembly or availability rule is
inferred.

**API gap:** an engine-owned two-slot transaction with separate primary
and companion resources, both readiness and commit receipts, the team
expiry clock, and individually guarded restoration of each saved original
identity. The primary's channel-15 completion or local 120-count fallback
is separate from the 1800 eligible team visits. On mode-4 cleanup,
nonzero `48A` skips the checked cleanup path; zero clears role/link
before requesting single-slot reload. Do not use timer expiry as a
guaranteed simultaneous split.

Next witnesses: exercise both participating owners with real approach
and readiness, record paired publication with the correct internal
resources, then observe both permitted and blocked restore with link
and object/weapon ownership checks. Compare a standalone `050F` load
without routing it through that battle transaction.

Regenerate with `machine-head`, `mechanical-dragon`, and `machine-dragon`.
With both participant packages installed, the Machine Head fusion check
reports **one unresolved party**, as intended. All three refuse Battle
Test and undeclared variants.

## Reviewed batch: Flying Saucer, Death Head, and Galactic Emperor

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Review data |
| --- | --- | --- | --- | --- |
| `GG4E08:0D03/0` Flying Saucer, AFS 675 | #195 / 600 | 250/295 | 3/4/0/10 | Maser 10/10, Devastation Laser 1/1; separate scratch/segment phases |
| `GG4E08:0505/0` Death Head, AFS 558 | #202 / 500 | 500/590 | 5/5/0/4 | Death Beam 10/10, Death Breath 1/1; separate accumulator/segment phases |
| `GG4E08:0E04/0` Galactic Emperor, AFS 684 | #205 / 3000 | 2000/6500 | 10/8/0/0 | Type-8 class/controller review; no move or spawn binding |

Each exact normal wrapper passes static `idle=none` preview; none has a
local motion bank. Static initializer readback finds distinct movement
type bytes (**18**, **2**, and **8**, respectively). The bytes do
not admit a movement policy: do not reuse Blue Striker's flight for
the Saucer, Wing/Quick's glide for Death Head, or Beam Satellite's
steering for the Emperor.

### Two beam collision paths, not one authored sweep

The [bounded X source checks](borg-attack-visual-recovery.md#swept-beams-flying-saucer-and-death-head)
separate type-32/key **8** (Saucer) from key **16** (Death Head).
The factory/active callbacks differ. Saucer has an explicit scratch
refusal path. Death Head's paid request is guarded by a source
accumulator exceeding **8192**, adding **682 per checked callback**;
this is not a portable action frame or a verified sustained hit rate.

The [type-32 census](retail-code-atlas.md#type-32-roster-census-carrier-and-segment-ownership)
separates the borrowed-endpoint family's positive-mode segment path at
`801DF2B0` from Saucer key 8's zero-mode branch, which binds group 1
on the carrier itself. Death Head's model-relative path at `800E20C0`
requests a type-11 child with two transformed endpoint addresses
after a rate-scaled clock. That request does not prove an accepted
terrain crossing, a visual explosion, or a damaging blast.

**API gap:** independently qualified endpoint generation and
segment emission on accepted simulation visits, with ownership/cleanup
and terrain/receiver result paths that can refuse. `beam-sweep` is
authored translation plus visual terrain impacts, not either factory's
source trajectory. Next witnesses: accepted versus refused scratch for
Saucer and the numeric threshold crossing for Death Head; segment
publication with actual terrain and receiver results; then demonstrate
which terrain crossings create visuals versus damaging effects.

### Galactic Emperor: role-gated pose write before stage projection

The [116 bounded original and 422 detached cases](borg-controller-audit.md#normal-galactic-emperor-class-pin-and-placement)
find three role-0 class prefixes writing `(0,2000,0)`, heading 8192,
and logical airborne status. Nonzero roles skip the coordinate write.
Shared type-8 stage/team/side placement and static/dynamic projection
can run **after** the initializer, so the pin is not the final spawn
position or proof the body fits. A distinct zero-authored-Y fallback
adds 1000 to a queried floor; it is not the table-height rule. Steering
1–3, ordinary jump/landing, and plane 27/28 are refused by the checked
setters. Five support-eligible body descriptors do not establish live
registration or a rider.

The exact actor record disables all five move display selectors even
though the specification text contains names. The package leaves moves
unresolved rather than binding that text or declaring that the class
has no actions. **API gap:** constructor role and stage-owned placement
receipts plus explicit class-pose hooks in their real order, with body
clearance and contact/support owners independently qualified. Next
witness: initial root/heading before and after both class and live
projection services, and accepted body registration in the actual
encounter. A generic fixed position or `stationary-v1` cannot supply
those results.

Regenerate with `flying-saucer`, `death-head`, or `galactic-emperor`.
No source type, display row, or raw preview in this batch grants gameplay.

## Reviewed batch: Jack and Jet Hero

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:080E/0` Jack, AFS 632 | #11 / 280 | 200/245 | 3/2/4/5 | Jelly Shot 5/8, Jelly Scope 1/1 |
| `GG4E08:0A01/0` Jet Hero, AFS 648 | #46 / 350 | 160/205 | 3/2/4/8 | Blade Gun Shot 10/10; checked charged-B Power Scope cue is stockless |

Both previews use the exact normal body and local `0:0`, `0:1`, `0:2`,
`3:0` and `4:0` samples. The host refuses raw `4:1` for both. There
is no fallback clip or borrowed Fly mapping.

### Jelly Scope: binding and following are not effect acceptance

The [scoped original checks](borg-package-requirements.md#projectile-and-movement-fields-are-not-smoke)
join Jack's X to type **58/key 1**. Accepted group **9** binding seeds
**1140** source count and samples logical key **12**, local `(0,0,0)`.
The next active sample from another supplied bone matrix updates the
position; a refused initial group bypasses that anchor visit. That
supports the sampled following prefix, not the player's reported
fixed sphere for its whole life. The stock row does not tell when
debit, re-entry or removal occurs.

`field-follow` plus `field-socket` can represent the documented
accepted anchor sample. They don't prove the matching group gate,
the source count's cadence, or a move binding. The player's accounts
of traversing-shot slowdown and broken locks remain distinct from
verified actor or projectile effects. **Next witness:** accepted
versus refused group through live following and exit, then one actor
and one passing object with normal/no-effect controls to distinguish
contact policy, guidance cancellation and any overlap persistence.

### Power Scope: initial placement, startup travel, and later contact

The checked charged-B cue at cursor **19** requests type **3/key 0**
without a debit. Static follow-through maps the key-0 descriptor
(`802DA028`) through the initializer `800857D4` to logical **key 3**,
local `(0,0,100)` and basis column 2. The accepted initialization
uses a shape-registration request; refusal takes a distinct phase-2
exit. The accepted object's startup branch at `80085B64` subtracts
**4** from a speed seeded at **80** and integrates the normalized
basis at the resulting speed while positive. On the nonpositive
branch it sets the later phase and updates the object's registration
mask. This is static evidence for startup world travel, not a fixed
sphere and not proof of its later overlap shape. The later branch
counts a nonzero contact-side byte and can enter teardown at **8**
such visits; neither the eligibility that writes that byte nor damage
or shot size is joined. Do not convert counts to native frames.

**API gap:** an accepted, phase-scoped travelling field plus a
separate contact/retirement policy. The existing fixed or
owner-following `field` and authored power-field boost do not encode
this startup or establish doubled shot damage. Verify the object
phase, world position/radius, effect eligibility, recipient credit and
repeat/stack rules with real actor, shot and beam crossings before
binding it. The checked cursor does not set a native activation
frame; Jet Hero's Fly and unrelated moves remain unadmitted.

Regenerate with `jack` and `jet-hero`. Their `moves.txt` inspections
show source-backed phase boundaries only, not executable fields.

## Reviewed batch: Normal Ninja and Command Girl

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0000/0` Normal Ninja, AFS 485 | #15 / 150 | 150/195 | 2/2/3/5 | Shuriken 10/10, Shinobi Bomb 5/5 |
| `GG4E08:0306/0` Command Girl, AFS 531 | #36 / 330 | 200/245 | 2/5/0/6 | Beam 10/10, Buster Laser 3/3 |

Both have separate exact normal wrappers and four-plus-one raw samples
`0:0`, `0:1`, `0:2`, `4:0`, `4:1`. Both installer configurations
include movement type 0, but the reviews do not qualify current
lanes, floor policy, or the portable player controls. Normal Ninja
and Sasuke have the same local motion bank, not the same bomb
selector or qualified controller.

### Normal Ninja: separate bomb owner, area request, and hit

Static inspection of the reviewed bomb leaf `800715F8` and the
model-0 branch confirms one channel-2 debit and a call to `8007DB5C`
with selector **0**. This requests a type-15 carrier. Its own normal
descriptor at `802D6D68` uses logical key **11**, local `(55,0,0)`,
and area selector **0**. The initializer uses its own selected-source
basis and copies aim-related state; it has a conditional solve for
the launch vector. Do not borrow Double Ninja or Sasuke launch tuning
or convert static table values into a native ballistic arc.

The active callback (`8007E200`) has a latched-contact branch to
`80085E00`, the separate **type-5** area factory, followed by carrier
retirement. A later request path uses the shared `80084170` response
test before the same factory. These are requests and gate shapes,
not a comparison of live events. Do not infer equal actor, floor and
prop behavior solely because a factory is shared.

**API scope:** the authored carrier and area rules cover selected
launch and impact mechanisms without asserting source pose timing
or live admission. The verified no-target handoff and portable
body-heading option cover one launch prerequisite.
The [paid-release floor replay](retail-code-atlas.md#ordinary-ninja-bombs-floor-response-and-same-pass-area-bind)
now checks real triangle response for keys 0 and 3. The floor branch
keeps its opening shape registered through the impact visit and
the same-pass area bind. The [matching actor-hit replay](retail-code-atlas.md#ordinary-ninja-actor-hits-unlink-request-versus-slot-return)
marks unlink before the area bind, but the parent still occupies
its slot until the separate shape pass. Neither branch can spend
the parent's slot for that area initializer.
The [floor-contact replay](retail-code-atlas.md#ordinary-ninja-floor-blasts-growth-timing-and-hp)
also separates direct damage from growth-dependent area damage.
The [authored `impact-area` rule](borg-gameplay-api.md#static-stage-impacts)
expresses delayed growth and independent hit identity. Its optional
`impact-area-shapes` rule preserves the verified no-borrow order
for a bounded native budget. It does not reproduce retail capacity,
body-slot admission, or the parent shape's post-impact contact on
terrain.
The [ordinary-X cue replay](retail-code-atlas.md#ordinary-x-installed-program-vm-cue-and-paid-bomb-release)
now joins the original phase handler to its VM-generated cue, stock,
and the constructor's later key-11 sample. The separate
[targeted launch replay](retail-code-atlas.md#ordinary-x-bombs-copied-aim-live-gate-and-gravity-refit)
distinguishes copied aim, the live launch gate, and later gravity
correction. The replays supply the pose and target selection; they
do not run live controls or qualify a full intercept. The
[authored ballistic aim and refit](borg-gameplay-api.md#ballistic-aim-and-gravity-refit)
use native target selection, body centres, and ticks. The
[grounded X Workshop opt-in](retail-roster-extraction.md#normal-ninja-and-sasuke-grounded-x-bomb-test)
uses them for two authored test profiles only. It preserves their B
moves and does not enable X by default. Live scheduling and prop
response remain separate checks.

### Command Girl: B and X do not request the same family of beam

The source specification names **Beam** (B) and **Buster Laser** (X).
Static inspection of the checked B leaf `80109190` finds a positive
program byte, one channel-0 debit, and `80109710 → 801DEB68` selecting
type **32/key 5** for model `0306`. The checked X branch
(`801083EC`) consumes a positive byte and checks caster state before
spending one channel-2 unit. State 0 requests `800A6108`, type
**35/key 0**. The separate state-1 branch instead schedules
state 2 for an existing-object path; the type-35 updater can
consume that owner state to reload a local count. No second factory
is shown at the state-1 debit. Neither route proves which attack in
the player's account explodes against a target, or that X is merely
a faster travelling B.

**API gap:** a stateful owner/object release or re-entry with accepted
stock and phase receipts, plus independently qualified beam/scratch or
type-35 contact policy. A one-off `blast=…|actor` supplies neither
policy. Next witness: original B and each X state separately, with
cue, debit, accepted object state, distinct movement and contact
results, and any follow-up area request. The portable profile should
refuse an unqualified route rather than choosing one from the name.

Regenerate with `normal-ninja` and `command-girl`. Both packages remain
review-only; no source address is a callable package operand.

## Reviewed batch: Plasma Dragon and Thunder Dragon

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0509/0` Plasma Dragon, AFS 561 | #151 / 1600 | 1000/1450 | 8/7/2/1 | Plasma Bomb 5/5, Plasma Breath 1/1 |
| `GG4E08:0515/0` Thunder Dragon, AFS 572 | #112 / 1100 | 500/680 | 5/6/1/1 | Thunder Bomb 5/5, Thunder Breath 1/1 |

The exact normal wrappers preview raw `0:0`, `0:1`, `0:2`, `4:0`.
The local bank matches the Flame/Ice archive, but neither inherits
Flame's controller. The B review adds separate emitter and descendant
contact outcomes, with damage payload unresolved. No stock, shape,
movement or breath rule executes from those rows.

### B impact: the emitter itself can make contact

The [original impact and shape comparisons](borg-attack-visual-recovery.md#type-30-collider-follow-up)
provide a narrower seam than the recollection of airborne spawned
bullets. On the checked terminal paths, the **type-47** carrier
requests area key **24** (Plasma) or **25** (Thunder) and separately
requests a **type-30** emitter in mode **2** or **3**. The area still
runs when the emitter allocation or bind refuses.

Read-only extraction of each exact slot-2 collision bank confirms
that group 7 lists row **9**, a radius-10 sphere at local origin,
and group 8 lists row **12**, a radius-110 capsule with local
endpoints at Y=0 and Y=400 after the checked unit-scale translation.
The first shape belongs to the emitter, not to beam/particle art.
A supplied type-30 group-7 acceptance samples the carrier matrix
on the emitter's first update, not the impact point captured by
the area path. The spawn pattern rotates around that fixed seed in world axes: offset
starts at 200, increases by 15 before each request, and advances
8° per visit with a three-way 120° phase selection. Up to 30
checked emitter visits can request descendants, and a refused
request does not rewind its counters. The further child can bind
group 8 and request model 83 independently; a rendered child is
not a contact receipt.

The static terrain wrapper writes the accepted crossing into the
carrier root; later emitter binding uses the carrier's matrix
translation. A supplied actor latch uses the carrier root for the
area path without that terrain lookup. One checked list sequence
spans pass 1 (impact), 2 (emitter bind), 3 (first child request),
4 (child bind), and 5 (first active descendant update). This is
scoped owner-list evidence, not universal global frame offsets.

Supplied Sasuke overlaps admit the expected tangent distinction
and credit the Dragon, with repeated-pair and separated controls.
At the reviewed level-0 receiver/amount call path, raw **10**/ **5**
produce actual deductions **9**/ **4** HP respectively. Guard/reaction
and scene holds can suppress results. Never turn 9/4 into global
package damage constants or skip the owner's receiver reaction.

**API gap:** `impact-pattern-space=…|world` and
`impact-pattern-timing` can express the authored axis and delays, and
`impact-hazard` approximates a descendant capsule/tangent policy.
They do not independently register a collidable sphere emitter, seed
its matrix at bind time, request/retire its deferred children with
that ownership, or execute the retail receiver/damage policy.
A grounded or authored flight controller also cannot stand in for
either exact base controller.

**Next narrow witness:** carry the accepted type-47 move cue through
resolved carrier contact and both adjacent class-3 visits, using the
actual matrix seed and stage result; then verify group-7 and group-8
overlaps, rejection, and recipient health with registration and object
retirement observed together. Keep the area request independent of
failed emitter bind. Only then consider a qualified live binding.

Regenerate with `plasma-dragon` and `thunder-dragon`. These packages
refuse Battle Test and undeclared variants.

## Reviewed batch: G-Black and Ultimate Cannon

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- |
| `GG4E08:062A/0` G-Black, AFS 605 | #206 / 600 | 350/440 | 4/5/6/6 | Beam Gun 5/8 |
| `GG4E08:0C04/0` Ultimate Cannon, AFS 669 | #145 / 1500 | 500/590 | 6/10/0/1 | Vulcan & Missile 200/200; ordinary-X channel 1/1 |

G-Black's exact normal wrapper and bank preview `0:0`, `0:1`, `0:2`,
`3:0`, `4:0`, `4:1`, `4:3`. Ultimate Cannon previews `0:0`, `0:1`,
`4:0`, `4:1`; raw `0:2`, `3:0`, and `4:3` are absent and still
refuse. The raw clip labels are not gameplay roles. The display
stock values do not change the executable profile's limits.

### G-Black: do not swap the two charged routes

The [scoped source checks](borg-attack-visual-recovery.md#beam-gunner-weapon-child)
separate **Buster Laser** (charged B, type **89/key 16**) from
**Black Buster** (charged X, type **32/key 17**, checked at cursor 12).
Neo G-Red's type-32/key-15 charged-B request cannot substitute for
either. Static inspection joins the common installer `8018CCFC`
to a G-Black-specific override (`8018CE08–8018CE14`) of the record
and configuration roots. Its local motion bank also differs from Neo
G-Red's. Shared class logic and the player's comparable-control report
are not exact movement or attack bindings.

The [processed-input fixture](retail-code-atlas.md#processed-input-and-charge-operands)
now verifies a prefix of branch selection. With an idle actor and
both buttons held past their supplied limit then released together,
the actual B-only profile selects charged B and the profile with
both modes selects charged X. The [isolated installer run](retail-code-atlas.md#standalone-profile-selection)
now identifies the dual-charge config as the selection for standalone
model `062A`; the B-only config was an explicitly supplied comparison.
With that installer selection, charged B reaches an allocator and
charged X reaches a world query before either reaches a program. No
service result or cue is supplied.

**API gap:** separate, qualified accepted charge/pose routes and
object-specific emitter/receiver transactions. `aim-lock` can cover a
committed direction only after its clock and sample owner are proved;
neither type establishes obstacle policy, damage, or a native commit
frame. Next witness: follow each exact charged cue independently
through allocation, first accepted contact and a miss/obstruction,
and observe which object and owner commit or retain target correction.

### Ultimate Cannon: ordinary X is not the charged beam

The [source attack audit](borg-attack-visual-recovery.md#steerable-sustained-beams-dragon-borgs-and-ultimate-cannon)
separates the ordinary X fragment from charged X. Static inspection
of the checked ordinary leaf `800B1918` finds that a zero local
count is reset to **90** before trying a single channel-2 debit.
On acceptance, the source calls the same one-object factory
(`800C3BE0`, type **2**) four times with keys **4, 5, 6, 7** in
order. It does not gate a later request on a prior allocation
result. A nonzero local count decrements instead. This is not proof
of four accepted objects or that the 90 operand is a native duration.
The source config has movement type **13**, not Normal Tank's
type-1 route. Charged X **Ultimate Beam** remains unresolved.

**Engine handoff:** `stock=` now accepts up to **255** units, so
the exact **200** can be represented without splitting a source pool.
`release-followup` can express up to five ordered independent
requests after the primary request behind one debit. Neither API
supplies the count/eligible visit or admits an Ultimate Cannon move.
That count remains separate from stock replenishment.
Twenty-one bounded checks ran the ordinary leaf, debit, and
type-2 factory with supplied stock mode and allocator. Zero count
reset to 90 even when the debit refused; a nonzero count only
decremented. All 16 allocator acceptance masks still attempted
4–7 in order behind one debit, and each accepted object retained
the caster's credit. This doesn't establish how the live scheduler
visits the count or what any child hits. Charged-X steering/width,
receiver recovery and actual damage cadence remain independent
work; the authored sustained beam does not establish them.

Regenerate with `g-black` and `ultimate-cannon`. Neither has an
executable profile or accepts an undeclared variant.

## Reviewed batch: Cyber Girl Super and Cyber Girl Hyper

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked display stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0305/0` Cyber Girl Super, AFS 530 | #120 / 590 | 250/295 | 3/6/2/6 | Quaternary Beam 4/4, Remote Beam 6/6 |
| `GG4E08:030A/0` Cyber Girl Hyper, AFS 535 | #157 / 620 | 300/345 | 3/6/2/6 | Quaternary Beam 8/8, Remote Beam 12/12 |

Both exact normal wrappers preview `0:0`, `0:1`, `0:2`, `3:0`,
`4:0` and `4:1`. The local motion banks match, but the bodies and
original level rows differ. Super's stock comes from the pinned
normal level rows, not the archive's incomplete Remote Beam entry.
Neither bank supplies a movement controller or an animated deployment
timeline. The common installed movement-type byte is 0; actual
movement, contact, and resource policy are left unresolved.

### Dependent initialization and remote release are distinct

The [requirements lead](borg-package-requirements.md#observed-attacks-with-secondary-emitters-and-anchor-paths)
describes inactive mounted guns, deployment, and coordinated shooting.
Hyper's archived account mentions a target-surrounding route.
These reports do not choose a stock/emission contract.

Read-only DOL follow-through resolves both normal installers to
`8012F204` and class initialization `8012F330`. Super makes
**three** indexed dependent allocation attempts (`0–2`) via
`80131C14`; Hyper makes **five** (`3–7`). Both happen at
initialization before a move. The parent then seeds its mask byte
at `+0x144` to **3** or **15** respectively. These counts are
allocation attempts, not verified visible guns, and the mask is
not a success receipt for the dependent pool.

The dependent constructor records a distinct owner reference and
index. Its later update `80131ECC` has index-specific branches
reading parent controls at `+0x6E8`/`+0x6E9`, or masks against
parent `+0x144`, then writes a child scale. That is a static
presentation handoff, not proof of an attachment's playable
activation timing. A separate actor cue path `8012F9B8`
requires a positive program byte and another owner guard before
consuming the cue. It scans four low mask bits and selects one
enabled index. The checked factory call `8013212C` uses that
index for Super or index **+6** for Hyper; accepted allocation
creates a separate **type-36** object. It does not prove the
object fired from current mounted or deployed pose, that X paid
at that call site, or that the main B already allocated a beam.

**API gap:** an indexed, owner-scoped dependent lifecycle with
accepted mount/activation distinct from visuals, and a semantic
remote request with its own eligibility and actual object credit.
The authored owner-following emitter currently begins at a deploy
marker and listens to **successful main allocation**; that ordering
is not established for these exact Borgs. Do not emit all child
indices from a single mask scan or assume Super and Hyper have
the same seat/target behavior. Next witnesses: qualified input
and debit to parent cue, an accepted and refused dependent
transition, and the actual remote object's spawn/aim/contact across
both variants. Neither game-activation policy nor target-seeking
behavior is bound by these packages.

Regenerate with `cyber-girl-super` and `cyber-girl-hyper`. Both
remain nonplayable.

## Reviewed batch: Wire Gunner and Wire Girl

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked normal stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0103/0` Wire Gunner, AFS 501 | #23 / 270 | 220/265 | 3/3/2/5 | Assault Shot 20/20, Set Wire 4/4 |
| `GG4E08:0301/0` Wire Girl, AFS 526 | #35 / 220 | 140/185 | 3/3/2/6 | Charge Gun 10/10, Anchor Wire 4/4 |

Wire Gunner previews `0:0`, `0:1`, `0:2`, `3:0`, `4:0`, `4:1`.
Wire Girl previews `0:0`, `0:1`, `0:2`, `4:0`; raw `3:0` refuses
on the AObj binding check and `4:1` is absent. The exact source
installer configurations supply movement-type byte 0 for both
(`80335E30`, `80322F40`), but the packages do not bind either
movement/contact policy. The normal X stock rows both have mode **2**,
with different stored preset operands (480 versus 0); these are not
timed refill declarations.

### Wire Gunner: a paid shot is not evidence of a sealed path

Static readback of the exact `0103` branch at `801314D8` finds a
channel-0 debit and, on acceptance, one key-**38** request to
`80082824`. The leaf also updates its own remaining-fire counter;
it is not proof of which charged cue carries a wire, the contact
receiver, or the path handoff. The [review leads](borg-package-requirements.md#observed-attacks-with-secondary-emitters-and-anchor-paths)
report an ordered world-space anchor set, including midair points,
and a charged hit dragging the **actual recipient** newest-first.
The supplied Move List separately reports a damage-only shot
without a wire. Do not infer that the checked key-38 leaf is the
same charged path, that either one uses a timed timer refill, or
that a miss consumes/restores the stored sequence.

**API gap:** the authored recipient-rewind engine service seals at
allocation and only supports same-height ground traversal. It can't
establish the retail seal/clear boundary, off-floor traversal, or
recipient motion eligibility. Next witnesses: charged release and
ordinary B with absent versus present accepted anchor objects,
including a later placement before contact; then actual contact
versus miss, route order and the first blocked/height-changing leg.

### Wire Girl: anchor debit and owner return are not a gesture

The exact normal initialization at `80106F38` calls `80106BB8`, which
seeds separately allocated owner scratch with the current root and
initializes its point records. A checked zero-cooldown owner leaf
in `80106FF0` sets a local count of **15**, tries one channel-2 debit
and, on acceptance, requests the separate type-**34** object via
`801073F8`. A further paid path at `80107094` increases an indexed
owner count. These paths do not verify a native double-tap window
or prove which one is the observed gesture. `80107510` has a
qualified lifecycle branch clearing an owner field and calling
`8006DE10` to return one channel-2 unit; `80107B34` has another
return in a qualified distance branch. Do not treat this as a
stock refund on every exit.

In **six bounded placement checks**, allocation refusal after a
successful debit restored the supplied capacity (4), not simply
the one debited unit. **Thirty-six point-return checks** ran the
separate distance branch with supplied scaled distances and
point counts. Distance at or below **25 × object scale** returned
one channel-2 unit up to capacity, without changing the timer. The
last point entered phase 2 and cleared the owner flag; an earlier
point decremented both recorded counts. Outside the threshold none
of those writes occurred. These cases are not a complete traversal
or the live gesture. The authored `anchor-return=…|on-reach` awards
one unit per accepted self-rewind arrival but does not adopt that
threshold, allocation reset, or original route.

**API gap:** a qualified owner anchor object and indexed point set,
separate pay/return events, and a self traversal (never the aimed or
hit recipient). The authored `tap`/`double-tap` and `rewind` choices
cover the guide's shape but do not establish its original input
window, point acceptance, sideways hold, or route ordering. Next
witnesses: one accepted placement, refused allocation and owner
stock return, then the separate multi-point placed/midair route
and input-to-accepted-self-traversal order. Do not copy Wire
Gunner's recipient flow.

Regenerate with `wire-gunner` and `wire-girl`. Neither preview or
move review executes placement/traversal or alters stock.

## Reviewed batch: Akuma Samurai and retail Titan Tank

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0708/0` Akuma Samurai, AFS 614 | #185 / 480 | 250/340 | 3/3/6/5 | Akuma Beam 5/5 |
| `GG4E08:0619/0` Titan Tank, AFS 591 | #66 / 620 | 400/490 | 5/3/5/5 | Titan Beam 5/5, Javelin Shot 3/3 |

Akuma's exact normal bank previews `0:2`, `3:0`, `4:0`; raw `0:0`,
`0:1` and `4:1` refuse on AObj binding. The explicit inspection
start `0:2` is not gameplay idle. The checked configuration at
`8032D2A8` has movement type 0 but does not qualify live movement.
Titan Tank's exact normal wrapper refuses on published record 7,
root `0x14e44`, POBJ `0x14e1c`, flags `0x9000` (type `0x1000`),
binding `0x14e00`, all relative to the HSD data block. The generated
Tank package deliberately declares no model, motion bank or preview.

### Akuma Beam: timed stock, not Demon Samurai's return

The [shared blade-growth and distinct B audit](borg-controller-audit.md#follow-up-soul-ball-return-and-blade-growth)
joins the ordinary defeat observer and arriving soul to a capped scalar
for Akuma and Demon Samurai, but not a measured render length or hit
reach. Static readback of Akuma's exact B leaf `80122BDC` shows
one channel-0 debit and, on success, a type-2/key-**59** request
through `800C3BE0`. Akuma's source level rows use mode 1 with preset
300; the detached debit advances that clock rather than relying on
the two-orb owner return. The portable `stock-clock=…|add-on-debit`
can express the arithmetic, not the accepted rate/pose sample or
contact. The X display label **Random Slash** does not qualify
Demon Samurai's hit group for Akuma.

**Next witness:** accepted Akuma B cue through the actual muzzle,
launch and contact with one rejected control, including the owner
stock clock and current pose. Independently join an incoming soul's
arrival to blade geometry/contact before exposing any growth operation.

### Titan Tank: catalogue row, installed actor record, and Javelin owner

The exact `pl0619.pzz` contains **two** 432-byte actor-data records.
The recovered specification row matches the first; the live installer
at `80126EC4` advances the source actor pointer by **432**.
Its `0619` override installs configuration `803305F8` (type 6),
not the internal `0605` type-1 path. This is why catalogue stats
and the visible move rows cannot substitute for the live installation.
The package preserves `gg4e08:0619` without a form graph or alias.

The [scoped Titan comparison](borg-controller-audit.md#normal-titan-robot-cyclic-form-and-the-separate-titan-tank)
identifies the exact `0619` X weapon branch, independent of the
Robot cycle. Static readback at `801275AC` spends one channel-2
unit then calls the type-**92** factory `80182FCC` with indices
**2**, then **3**, mode **1**. It doesn't inspect the first
allocation before requesting the second. The `0605` branch instead
passes indices 0/1 in mode 0. The ordinary paid-release followup
API now covers independent requests behind one debit, not the type-92
initializer's authored attachment, path, collision, or physical
retirement. Don't identify a Javelin with either isolated effect
allocation receipt or generic beam hits.

**Next witness:** the scheduled cue, accepted stock, partial
allocation and each child from current owner pose through the actual
target/stage contact (including a miss); also qualify the record-7
shape/evaluation or a narrowly valid explicit preview record. No live
profile is installed.

Regenerate with `akuma-samurai` and `titan-tank`. Both refuse Battle
Test and undeclared variants.

## Reviewed batch: Time Bomber and Remote Bomber

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0106/0` Time Bomber, AFS 504 | #24 / 220 | 220/265 | 2/4/0/5 | Set Bomb 10/10, Bomb Shot 3/3 |
| `GG4E08:0107/0` Remote Bomber, AFS 505 | #25 / 180 | 220/265 | 2/4/0/5 | Set Bomb 8/8; checked X command is stockless |

Time Bomber previews `0:0`, `0:1`, `0:2`, `3:0`, `4:0`, `4:1` with
its own normal wrapper. Remote Bomber's body fails the shared
material check. Readback of record 0 (root `0x12bc8`) finds MObj
`0xa540` binding TObj `0xa4d0` with flags **`0x30082`**,
including generated texture coordinate mode **2**. Offsets are
relative to the HSD data block. The package stays metadata-only:
do not erase that stage or replace it with ordinary UV coordinates.
The same installed movement-type byte (0) does not qualify either
live locomotion or controller timing.

### Timed and remote bombs share a factory, not the trigger

Static inspection joins both model IDs to installer `801301F8`
with separate model configuration overrides. Owner hook `801304B8`
has a conditional B placement path with its own current count gate.
The inspected Time branch spends channel 0 and calls `800E058C`
with selector **2/key 10**. The Remote branch spends channel 0,
uses the **post-debit count parity** to update an owner bit and
requests selector **3/key 11 or 12**. Both requests are type **41**.
Both normal B stock rows use mode 2; that does not authorize a
generic timed refill. Time's X leaf `80131AAC` separately spends
channel 2 and requests `800FE668` with `2,2`. Do not turn that
request into an alias of the B object or Remote's command.

For Remote, the separate owner X check at `80130608` requires
an X pulse and channel-0 count below the maximum. It sets
`+0x6F7` without spending on that route. The active type-41
callback (`800E0A30`) checks for selector 3 and nonzero owner
latch, then **clears the latch before** provisional teardown
and a separate type-5 area request (area selector **20**).
It does not broadcast to every matching bomb. The checked
contact-side latch also has a separate area request; a Time
selector-2 row names area **19**. Later phase-2 lifecycle
`800E0DB8 → 800E0E68` uses an object's nonnegative bound
channel to request a capped stock return. Neither stock return
nor allocation is a recipient/damage receipt. Exact lifetime,
shape/contact, object-list order and initial binding remain open.

**API gap:** The default `detonate` broadcasts to all qualifying
authored charges in one visit. A portable exact pilot needs an
owner-scoped consumable request read by eligible active charge
visits, distinct retirement/area allocation and a separate
channel-bound stock return. Do not claim a global instant
detonation, a fixed target order, or a broad refund on failure.
Next witnesses: two eligible Remote objects in original list
order consuming one command, actual accepted area and receiver
contact versus refusal, and return on the bound channel after
retirement; separately witness Time B expiry/contact and its
paid X carrier. The missing movement bindings and Remote material
remain independent gates.

**Follow-up:** Sixteen bounded source cases run two active callback
prefixes in both supplied orders. The first eligible selector-3
charge consumes the latch; selector 2 doesn't consume it. The
successful area allocation binds type 5/key 20 at that charge's
root. Refusal still consumes the latch and leaves the other charge
active. The command itself does not return stock. The separate
phase-2 callback returned one unit only with the supplied bound
channel, leaving the clock unchanged. These cases use a supplied
visit order, not a verified scheduler or area damage path.
`detonation-policy=…|first-eligible` now covers consumption in the
authored projectile pass. It does not adopt the retail retirement
delay or contact policy.

Regenerate with `time-bomber` and `remote-bomber`. Neither package
admits gameplay or an alternate variant.

## Reviewed batch: Cosmic Dragon and Space Dragon

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Checked stock |
| --- | --- | --- | --- | --- |
| `GG4E08:0504/0` Cosmic Dragon, AFS 557 | #201 / 1700 | 1300/1750 | 9/8/2/1 | Cosmic Beam 5/5, Cosmic Breath 1/1 |
| `GG4E08:0510/0` Space Dragon, AFS 568 | #197 / 1200 | 650/740 | 6/7/1/1 | Space Beam 5/5, Space Breath 1/1 |

Both exact normal wrappers preview `0:0`, `0:1`, `0:2`, `3:0`,
`4:0`, `4:1`. The matching local motion banks and shared installer
do not qualify either live controller. The configuration bytes
for these exact normals report movement type 1; do not borrow
Flame Dragon's type-4 movement or its boost/refill semantics.

### Black Hole: charged-X identity, travelling startup, and a separate registration

The GG4E08 display data puts **Black Hole** in the charged-X slot
for each, matching the [scoped source charged-X checks](borg-attack-visual-recovery.md#movement-contact-and-attraction-leads).
The Move List's charged-B claim about projectile pull is preserved
as a source conflict, not used to rebind the display name or establish
which object applies the reported effect. Static inspection of
`8010A468 → 8008672C` confirms type **7/key 0** for Cosmic and
**7/key 1** for Space. A separate positive-cue, channel-2-paid
path at `80109F70` requests type **32/key 6** or **18** respectively;
it does not pay for the checked type-7 request. The complete stock
policy of Black Hole remains unresolved.

On a successful group-2 bind the type-7 initializer `8008688C`
samples logical key **2** with local `(0,380,610)` for Cosmic or
`(0,190,305)` for Space, using basis column **2**. Distinct
table rows supply startup travel and growth operands. Start
does not immediately publish the owner-slot pointer. After a
travel phase and growth gate, `80086CF0` advances subphase and
writes that slot's active flag and pointer. The checked active
clock initializes from **600** and a replacement pointer can
trigger teardown; teardown clears the slot only if it still
names that exact object. This is static ownership/lifetime
evidence, not a scheduled result or a live pull receipt.

Sixteen bounded checks then executed the publication and retirement
callbacks with supplied objects and slots. Publication copied the
supplied settled scale into the active axes and replaced the slot
pointer without immediately changing the older object's phase. The
old object retired on its next active visit, but did not clear the
new pointer or flag. Expiry without a replacement did clear its own
pointer and flag. Thus the authored instantaneous replacement
boundary models exclusivity, not original teardown timing.

**API gap:** a registered, owner-slot travelling/growing field
with separate bind and publish/replace/retire visits. The authored
`field-slot=…|on-ready` can defer replacement until a new drift
settles. It doesn't reproduce the growth gate, binding, or owner
pointer callbacks. The authored
`field-shot-pull` and `field-core-damage` only express possible
effects, not this source phase order or any real projectile or
actor recipient policy. Next witness: accepted charged X through
bind, travel and first pointer publication with a bind/refusal
control, then type/owner-filtered shot and actor visits to
distinguish deflection, capture/retirement, and damage. The
source count is not a native tick duration. Neither dark field,
ordinary breath, nor the separate beam route is admitted.

Regenerate with `cosmic-dragon` and `space-dragon`. Both reviews
leave hit payloads and executable rules unresolved.

## Reviewed batch: Machine Red, Machine Blue and Cyber Machine Seiryu

| Exact normal source | Catalogue / Force cost | HP L1/L10 | DEF/SHT/ATK/SPD | Displayed finite stock L1/L10 |
| --- | --- | --- | --- | --- |
| `GG4E08:0600/0` Machine Red, AFS 575 | #164 / 900 | 500/590 | 5/5/5/5 | Beam Rifle 5/5, Buster Laser 2/2 |
| `GG4E08:0601/0` Machine Blue, AFS 576 | #165 / 850 | 550/640 | 6/5/4/5 | Arm Missile 4/4, Shoulder Cannon 5/5 |
| `GG4E08:0602/0` Cyber Machine Seiryu, AFS 577 | #176 / 1750 | 1050/1230 | 9/7/7/5 | No moves declared; all five normal actor display selectors are `255` |

The original English specification supplies the names, including Red's
**Beam Sword** and Blue's **Punch Kick**. Seiryu's specification text
does have five strings; their presence does not override the disabled
selectors on the checked actor record or bind the live fused actions.
Each native preview uses its own normal wrapper and local motion bank.
All three preview `0:0`, `0:1`, `0:2`, `3:0` and `4:0`. Blue and the
Seiryu primary additionally preview `4:1`, which Red's bank refuses as
missing. Those sample names are not battle cue labels. All three checked
installer configurations have movement-type byte 0. The
[Machine Red fall/landing witness](borg-movement-families.md#ordinary-fall-roster-checkpoint)
is a separate movement fragment, not an idle-to-action controller map.

### Standalone attack candidates

- **Red, Beam Rifle:** scoped channel-0 debit `800C9F78` accepts before
  the `0600` branch requests one type-2/key-21 object at `800C9F98 →
  800C3BE0`. The initializer at `800C3D2C` uses the key-21 descriptor
  (`803036CC`), owner matrix key **7**, local `(-249.4, -9, 64)`,
  and a separate group bind. Do not substitute the authored generic
  projectile: first witness the accepted B entry, release and refill
  timing, bind/refusal, active carrier contact and retirement, plus the
  close-range selection control. This is the narrowest standalone
  candidate in the batch, not a qualified playable subset.
- **Blue, Arm Missile:** class init `800CE858` requests two indexed
  owner children through `80136880`. The checked B release
  `800CF6FC` accepts a channel-0 debit, requests type **52** through
  `80128DF8` with the current index at owner `+0x06E8`, then
  writes the corresponding owner byte at `+0x014A + index` without
  checking the allocation result. This is not a generic unindexed
  missile or proof that each shot came from a rendered launcher.
- **Blue, Shoulder Cannon:** the checked channel-2 debit at `800CFA70`
  precedes two type-1 requests (`80084600`, keys **7 and 8**);
  the first request's result does not gate the second. Verify accepted
  bind, origin, contact and stock independently before mapping this to
  a portable paired release. Red's Buster Laser and both melee routes
  stay display-only.

### Seiryu is a paired rebind, not a third combatant

Machine Red's `gg4e08:seiryu-battle` review links exact normal Red
and Blue to the named catalogue primary `0602/0`, with `result-use=fusion-only`.
The [original-source audit and bounded comparisons](borg-controller-audit.md#cyber-machine-seiryu-two-slot-battle-fusion)
also require internal companion `0603/0` on the other owner slot.
Do not install `0603` as a new retail identity or run a second copy
of the primary in that slot. Both slots must reach stage seven before
the paired rebind; the primary role can change when Blue is human and
Red is AI. The 1800-count team clock advances on eligible updates;
expiry and the guarded per-owner restore are separate from resource
retirement and link cleanup.

The native fusion check resolves the package references, not the
companion binding, live controller, state transfer, or split. The
[portable paired ledger](borg-packages.md#standalone-battle-fusion-check)
does not load the two retail roles or transfer HP/stock. The next
fusion witness needs both existing slots through commit and guarded
split, including a pre-commit interruption and the human-Blue role
case. This battle review makes no inventory or crystal claim.

Regenerate with `machine-red`, `machine-blue` and `cyber-machine-seiryu`.
All three refuse Battle Test and undeclared variants. No original
replay was run for this batch.

### Machine Red Beam Rifle: source preflight for a single-move comparison

Scope: `0600/0`, ordinary grounded ranged B, one paid release. This
follow-up uses the pinned GG4E08 DOL, the saved movement witness above,
and the retained `deep-0600-machine-red`, `bulk-actions-machine-volley`
and `0600` animation request receipts. The detached release/refill
comparisons supply context, cursor, aim, and allocator results; the
retained `P2:0` VM runs only through visit 13. None joins a fresh B
press, its actual cue, a hit, and rifle stock refill.

| Boundary | Evidence and remaining gap |
| --- | --- |
| Program request | The [scoped original-input fixture](retail-code-atlas.md#from-the-installed-row-to-the-first-program-request) joins fresh ranged B to class state `+0x580 = 0`, family `800C9510`, then `800C9590` and entry `800C95D8`. The idle pre-callback zeroes `+0x6EE`, and the leaf requests local `P2:0` at `8030903E` through `800C9634`. The fixture stops before executing it, with a declared actor/scene and no live terrain witness. |
| Exact cue and guard | `P2:0` first executes upper-control command `80309046` (`1CEF = 0`). After waits and a directional descriptor change, `8030906A` writes `1CEF = 1` via opcode 2 (`8004C424`), provided the effective mask includes the upper lane. At `800C96C0` a pending cue temporarily zeroes the step for the VM call. The checked release branch requires a nonzero cue **and** either a passing aim test (`8006D0DC`) or local clock expiry. Neither physical B release nor clock expiry alone fires. No original visit number or native frame is inferred from the program bytes. |
| Debit, refill and movement | All ten normal level rows give channel 0 initial/max **5**, mode **0**, preset **300**. `800C9F78` spends one before requesting type 2/key 21. Shared `8006DBE0` only arms mode 0 on an emptying debit; the refill visit `80055D08 → 8006DCC0` checks the channel inhibit mask at `+0x597`, decrements by the effective stock step, then restores the cap and clears the clock at expiry. **300 is a source clock value, not a native tick count.** The saved Machine Red fall trace establishes fall, landing, neutral and fresh walk from supplied terrain, including a separate cache-based movement refill probe. It does not establish the weapon's inhibit/step schedule or admit idle-to-B in the native controller. |
| Carrier and contact | Key 21 specifies owner matrix key 7 and local `(-249.4, -9, 64)`. The paid request precedes allocation and a separate resource bind; neither implies contact. The live accepted pose, carrier direction and sweep, receiver/HP result and retirement have not been joined. |

**Shortest qualification path:** start with one reserved original replay
from an exact-normal, settled ground checkpoint with a registered
stationary receiver outside close-B selection. Feed a single fresh B
pulse and observe selector/substate, requested program, upper mask,
zero-to-nonzero cue, aim/timer gate, debit, key-7 pose, allocation/bind,
and receiver contact or refusal. Include a same-checkpoint two-stock
versus one-stock fork to distinguish partial and emptying debit; on the
emptying path log `+0x597`, effective stock step and the first eligible
restore to 5. Use a close-range selection control if the checkpoint
supports it; stop before exercising the sword. A missing exact
checkpoint or failed producer/bind is a blocker, not a reason to seed
a release cue or synthesize a hit.

Compare only that accepted move transaction in an isolated native
fixture. `entry-stock` (if fresh entry proves it), ordinary timed stock
and logical socket origins are candidates. A fixed native release
marker is not the source cue/aim gate by itself, and `grounded-v1`
is authored, not Machine Red movement. Any missing native owner or
fixture support is an engine handoff; coordinate a single compiler job
only after the original receipts define that comparison. Keep close B,
X, repeat, aerial movement and fusion unqualified, and leave
`gameplay=` absent even if the rifle comparison passes.

#### Reserved original replay: exact-rig cue prefix

The loaded movement harness supplies outcomes at `8005614C` (weapon
stock setup), `800C92B8` (class initializer), and `800699D8` (action
arbitration). It has no joined grounded actor/receiver checkpoint for
this shot. The reserved track therefore used the retained request
arguments (`mask 0x0F`, local `P2:0`, effective rig mask `0x07`) on
the rebuilt exact normal rig, with no scheduler, cue, or aim result
injected. The original opening left `1CEF` at 0. On VM visit **16**,
`8030906A` changed it **0 → 1** on lane bit `0x01`; the same opcode
visits on bits `0x02` and `0x04` left it unchanged. The upper lane
then held wait `8030907E`, descriptor `2:6`, exact local clip `3:11`
at cursor 1. All 18 lookups resolved without fallback. This is an
original VM receipt for a supplied request, not a frame count from B.

The later [input-to-VM fixture](retail-code-atlas.md#first-cues-in-the-original-vm)
uses the actual installed ranged-B row. The original request clears the
marker and installs its first wait before the action's world query.
Advancing only that resident VM with a supplied step, without a loaded
rig or world-query result, reaches the same cue at visit 11. This is a
separate clock comparison, not a replacement for the rig-backed count
or evidence of a tick-11 live release.

The rig-backed run stopped before the class cue consumer and made no
weapon debit, refill, allocation, or recipient claim. The exclusive
local checkpoint is `retail-0600-rifle-vm-prefix-v1.json` under
`build/borg-ordered-audit/` (SHA-256
`5b0ccaeb4fe065c630d3ac642fd7e0f745e6330cc8b071c0027fd415dac2d476`).
Wall time was **28.610 s**; launch availability was **5,935,560 KiB**,
with a **1 GiB** address-space cap. No successor was run. The next
owner boundary remains the original grounded input/scheduler and
initialized weapon/receiver setup; a separate native fixture cannot
replace that source join.

#### Prepared next witness: post-VM consumer and first bind

The saved cue checkpoint is a receipt, not a resumable actor image.
The private `retail-0600-rifle-consumer-v1.py` fixture under
`build/borg-ordered-audit/` translates only its cue byte (`1` from
visit 16 or `0` from the opening as a refusal control) to fresh
minimal actor forks. It uses the pinned DOL, AFS 575/normal actor
block, level-1 stock operands and original collision bank group 0
(one descriptor, index 15). No VM cursor, pose, target, stock,
or scheduler state is restored from the receipt.

| Boundary | Fixture input and intended source observation |
| --- | --- |
| Post-VM aim | Enter `800C9704` with supplied class state 3/subphase 1, a minimal unregistered peer and exact cardinal target point, body yaw 0 or a quarter turn, clock 2 or 1, and step 1. Only the cardinal `atan2(0,1024)=0` leaf is supplied. The original aim conversion, turn-limit test, countdown and cue guards execute. Compare aim-pass, aim-miss before expiry, aim-miss at expiry, and expiry without cue. |
| Debit versus constructor | Supply 2, 1 or 0 current units, previous count equal to current, and clock zero, with original mode 0/cap 5/preset 300. Execute the real `8006DBE0` debit, not a stub. The partial and emptying controls check whether a clock is armed; the empty control must never reach the constructor. `800C3BE0` writes the header on a supplied cell or sees a supplied null result after debit. |
| First bind versus pool refusal | Directly visit the constructed header's callback once, without a Scene list or a scheduler. `8008AC80` executes group 0 selection from the exact member bank and writes the shape header on a separately supplied cell. The null shape-cell control checks phase 2 and no refund. Stop successful bind at `800C3DAC`, before reading or copying key 7. No matrix is supplied. |

The carrier and shape allocator outcomes and self-credit pointer are
fixture operands, not live ownership or registration receipts. A
passing track can qualify the original consumer, paid request and
first bind **under those operands**. It can neither time the cue from
B nor qualify live targeting, weapon refill cadence, Scene allocation,
origin publication, retirement, contact or Battle Test. Only syntax,
input hashes, source branch edges and exact AFS fields were checked
during prep. The checkpoint filename is reserved by exclusive creation
on the authorized run.

**First launch:** the authorized process opened
`retail-0600-rifle-consumer-v1.json` exclusively, then stopped before
`track()` because the launcher called `signal.signal(...)` after
importing `signal` as a function. The zero-byte file is preserved as
an interrupted launch, not a receipt. `MemAvailable` immediately
before launch was 5,649,012 KiB, the address-space cap was 1 GiB,
and measured wall time was 0.085 s. No original instruction or fork
ran, no peak RSS was recorded, and there was no retry or successor.

**Corrected launch, partial receipt:** under a fresh reservation, the
first fork entered `800C9704` with the translated cue byte 1 and
supplied cardinal aim inputs, local clock 2 and two rounds. Original
aim accepted with the clock unchanged; the channel-0 debit changed
stock **2 → 1**, stored previous count **2**, and left the refill
clock at **0**. The original constructor wrote the type-2/key-21
header into the supplied, unregistered carrier cell. Its first bind
requested a group-48 shape cell, and the harness supplied one. The
evaluator then stopped on an unmapped read before the bind returned.
The receipt does not capture the failing guest address, a bind
result, or key-7 consumption. The six later controls did not run;
this is a partial positive-case receipt, not a refusal comparison.

The exclusive `retail-0600-rifle-consumer-v2.json` receipt has SHA-256
`793170adb217cc24a482168d1ea02f11046fd794ac2aedc33a1dc9c036ccdb5d`.
Measured wall time was 0.170 s; peak RSS was 31,496 KiB, launch
availability 5,775,260 KiB, and the cap 1 GiB. No retry, successor,
or package admission followed.

The authored `release-aim-gate` now separates a release marker from
its aim-or-timeout permission. Its angle, timeout and native frame are
package choices. The visit-16 result does not select any of them for
Machine Red.

#### Post-VM consumer: accepted aim and debit, bind unresolved

The corrected bounded run used the saved cue byte `1` as provenance,
not as a restored actor image. With the supplied target, yaw, class
state and clock, the original aim check passed without advancing the
clock. The debit changed current B stock **2 → 1**, stored previous
stock **2**, and kept the refill clock at **0**. With a supplied pool
cell, the constructor created a **type-2/key-21** header owned by the
original caster.

The first bind requested a shape cell, then stopped on an unmapped
read before returning. That is a harness stop, not a retail shape
refusal. No completed bind, key-7 matrix read, launch vector,
recipient contact, or timeout control was observed. The other six
forks did not run. The exclusive private `v2` receipt is preserved;
no gameplay admission or first-release frame follows from it.

#### Static audit of the v2 bind stop

The likely first unmapped read is `8008AD74`, `lbz r0,0x1CEC(r25)`.
The binder's `r25` is the **carrier**, not the credited Machine Red
actor. v2 supplied a zeroed carrier cell `[0x120000,0x120400)` and
returned it without the real allocator's header write. The resulting
zero `carrier+0x83` sends `8008AD48` down the actor-layout branch.
That branch computes a shape matrix pointer at `carrier+0x8D4`,
then reads `0x121CEC` and, if reached, `0x121CED` at `8008AD78`.
Neither address belongs to the carrier or shape fixture. This
identifies the first hole on the statically reconstructed path; v2
did not record an executed PC/address pair for the fault.

The missing byte is a source-pinned allocator output, not pose or
scheduler state. In `80088AA0`, the eligible slot remap at
`80088B2C..80088B40` maps base bucket 8 to 9 for the declared
ordinary slot-zero credit. `80088B54..80088B58` reads the byte
from `802DA700 + bucket` and stores it at returned cell `+0x83`.
The exact table yields `0x10` for carrier buckets 8–14 and `0x1A`
for shape buckets 48–54 (base 48 remaps to 49 at slot zero).
With the carrier tag present, `8008AD48` takes the object branch
and writes `shape+0x60 = carrier+0x114`. It does not read those
actor-layout fields or consume a matrix on that branch.

**Safe next fixture:** in a new private revision, explicitly declare
ordinary slot zero. At the two supplied-success allocator seams,
derive and check the effective bucket and write the exact table tag
before returning the scratch cell. Label the cells, tags and dispatch
as supplied/source-derived fixture state, not live allocations.
Do not enlarge the carrier to cover `+0x1CEC`, add substitute matrices,
or write a live list. Trace the branch and first-bind return separately
using the already-pinned AFS group-0 descriptor 15; stop at
`800C3DAC` before the actual parent key-7 matrix copy. If the seam
cannot prove the tag, stop at `8008AD40` and do not report a bound
shape. Keep null carrier and null shape controls distinct: in the
shape-null fork the original branch skips the header writes and
requests phase 2, with stock checked for no refund. Observe the
aim/timeout and null controls before the positive bind, and log the
guest PC, address and width of any missing read.

Real pool availability, list insertion, and current pose are separate
dependencies. This plan cannot supply their scheduler receipts.
Both v1 and v2 artifacts are unchanged; no replay or compile was run.

#### Reserved v3 replay: post-VM gate, debit and first bind

All seven scoped forks completed. They translated the saved cue byte
and supplied class state, target/yaw, timer and step, starting stock,
pool outcomes, and the pinned slot-zero carrier/shape tags `0x10` and
`0x1A`. The original gate, debit, constructor and first bind executed
where reached.

| Fork | Observed original outcome |
| --- | --- |
| Aim miss, clock not due | Clock 2 → 1; no debit or constructor. |
| Timeout with opening cue 0 | Clock 1 → 0; no debit or constructor. |
| Aim pass, empty stock | Aim gate passed; debit refused, no constructor. |
| Null carrier cell | Debit 1 → 0 armed clock 300; no header and no refund. |
| Null shape cell | Debit 1 → 0 armed clock 300; carrier created, bind returned 0 and selected phase 2, no refund. No retirement visit. |
| Aim pass, partial stock | Clock unchanged; debit 2 → 1 left refill clock at 0; type-2/key-21 header and group-0 bind accepted. |
| Aim miss, timeout with cue 1 | Clock 1 → 0; debit 1 → 0 armed clock 300; type-2/key-21 header and group-0 bind accepted. |

Both positive binds took the nonzero-tag branch and stored the
carrier's inline matrix pointer in the shape. Both stopped at
`800C3DAC` before copying the parent's key-7 matrix. No live pool
insertion, current muzzle pose, release frame from B, refill visit,
contact, or Battle Test admission follows from the result.

The later [attack-transaction fixture](retail-code-atlas.md#paid-release-and-pair-allocation)
joins the VM-produced cue to the paid rifle handler and executes the
actual allocator with declared pool records. It independently verifies
the empty-carrier and empty-descriptor refusals without a refund, and
observes the original carrier list insertion on success. It stops at
the next world query and does not replace this older bind witness or
establish a scheduled carrier visit.

Exclusive private receipt: `build/borg-ordered-audit/retail-0600-rifle-consumer-v3.json`,
SHA-256 `87c3182900e0bda387ac335f0e86abdc1de42878e3f4be97b6e9aa0c271a8f33`.
One process, soft/hard 1 GiB address-space cap, 58 s outer timeout;
launch `MemAvailable` 5,621,392 KiB, peak RSS 31,608 KiB, wall time
0.203 s. No retry, successor, compile or package change.

#### Launch witness: supplied rigid pose

The exact-rig VM receipt stores the visit-16 cue, descriptor, clip and
cursor, not a current logical key-7 matrix or a pose publication
epoch. v3 stores bind outcomes, not registers or actor memory. The
missing ownership join is the original post-actor world publication
for the same actor/resource generation, followed by the accepted
carrier's first scheduled visit. Neither receipt can supply that
matrix or the credited actor's simultaneous key-0 bearing.

The private `build/borg-ordered-audit/retail-0600-rifle-launch-v1.py`
prepares one bounded case, with fresh output
`build/borg-ordered-audit/retail-0600-rifle-launch-v1.json`. It rebuilds
only v3's aim-accepted partial-stock fork, compares debit/header/bind
to its receipt, and continues the **same in-memory carrier** at
`800C3DAC`. The receipt supplies no pose. Instead, the fixture declares
two rigid matrices with different forward columns for parent key 7
and credit key 0, with no carried launch target. It tests the source
key-7 copy and descriptor-offset transform, the null-target credit
bearing at `80083E64`, the key-21 speed operand (80), and the resulting
scratch carrier matrix. Narrow cardinal math adapters stand in for
normalization, scaling and cross/concat helpers; no hardware
floating-point parity or exact action timing is asserted.

The positive bind's shape pointer must still reference this carrier,
but the witness stops before the `800AEDC0` effect request at
`800C3FE8`, the subsequent late registration, and any shape callback.
It does not run an isolated shape callback ahead of its scheduler
owner or claim first shape publication, contact or damage.

**Reserved launch result:** the rebuilt fork matched v3 through
group-0 bind, then copied the supplied key-7 matrix and transformed
the original local muzzle offset to **`(74, 11, 279.3999938964844)`**.
The null-target branch read the supplied credit key-0 forward
column, not key 7's forward column, giving unit direction
**`(0, 0, 1)`** and velocity **`(0, 0, 80)`** through the declared
cardinal math seams. In `800C44A8`'s mode-5 basis path, the first
cross-product seam (`80046408 → 8020BB78`) refused its operands
before writing a result. The receipt does not record the rejected
vectors; no completed carrier matrix, effect, late registration,
shape publication, contact, or admission follows.

One reserved process ran with a soft/hard **1 GiB** address-space
limit, **58 s** outer timeout, **5,794,636 KiB** launch availability
and **31,504 KiB** peak RSS. Measured wall time was **0.186 s**.
The new exclusive launch receipt's SHA-256 is
`c915726f910188e2361c586a33a94c0641ad684b2c541f085a58119f242b5392`.
Earlier artifacts are preserved. No retry, successor, compile, or
admission change.

#### Static audit of launch-v1 basis stop; launch-v2 prepared

The v1 assertion expected the first cross product to receive the
**speed-scaled velocity** `(0,0,80)`. That is the wrong operand.
Before scaling the direction, `800C3ED0` adds its unit step to the
muzzle position. Later `800C44C8` subtracts the saved muzzle from
that stepped position. Mode 5 uses the resulting **displacement**
`(0,0,1)`. For the same explicitly supplied rigid pose, static
instruction order gives:

| Original call | Expected operands → result |
| --- | --- |
| `80046408 → 8020BB78` | Up `(0,1,0)` × displacement `(0,0,1)` → right `(1,0,0)` |
| `8004644C → 8020BB78` | Displacement `(0,0,1)` × right `(1,0,0)` → up `(0,1,0)` |

This is a static reconstruction, not operands logged by v1. The
new private `build/borg-ordered-audit/retail-0600-rifle-launch-v2.py`
keeps the same v3 fork and supplied pose. It logs both vectors and
their pointers before the operand guard, then executes the original
cross-product and matrix concatenation helpers through the existing
paired-single interpreter. Its remaining normalization and speed
seams accept only the scoped cardinal operands and log before
rejecting; no unexpected vector is replaced. The success cut is
`800C3FD0`, immediately after the matrix helper returns, before
effect argument setup, registration or shape callback. The fresh
output path is `build/borg-ordered-audit/retail-0600-rifle-launch-v2.json`.
Syntax, source operands and parent pins were checked statically;
no replay was launched and neither poses nor gameplay were admitted.

#### Reserved launch-v2 result: carrier matrix, no shape update

The replay matched launch-v1's supplied-pose muzzle, credit bearing
and velocity. Both original cross-product calls then matched the
static operand prediction above (ignoring the sign of zero).
Original scale concatenation and translation produced this carrier
matrix at `800C3FD0`, in source units:

```text
1.6233766078948975  0                   0                    74
0                   1.6233766078948975  0                    11
0                   0                   0.04545454680919647  280.3999938964844
```

The fixture supplied the cue byte, actor gate state, pool cells and
tags, rigid key-7/key-0 matrices, null launch target, cardinal math
seams and direct callback dispatch. The original helper code
consumed those inputs through the bounded interpreter; this is
not a current-pose, scheduler, or hardware floating-point receipt.
The shape retained its bound pointer only. No effect setup,
registration, shape update, contact, or gameplay admission ran.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-launch-v2.json`
SHA-256: `980fb39697a3fde0d3fb169a2b9d7319bd55c260e3ee49cfbca09c90dce15494`.
One process, **0.187 s** wall, **5,752,380 KiB** available at launch,
**31,396 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. Prior receipts remain unchanged. No retry,
successor, compile or package change.

#### Prepared first-shape witness: geometry before activation (not run)

The private `build/borg-ordered-audit/retail-0600-rifle-shape-prefix-v1.py`
rebuilds one launch-v2 fork and checks its output against the pinned
receipt; it does not restore a suspended actor or copy a saved matrix
into a new shape. It uses the **same** bound class-4 cell, whose
`+0x60` points to that carrier's inline matrix. AFS 575 collision
group 0 selects descriptor **15**: sphere kind 0, flags `0x0001`,
local center `(0,0,0)`, base radius 20. The scoped direct visit at
`8008A65C` can test `8003A4B8`'s local center, scaled radius and
initial endpoint writes without mapping or seeding any Scene list.
This is local geometry, not proof of a scheduled or registered shape.

The negative control supplies the carrier's stopped byte (`+0x1D8=1`),
then observes whether the same shape takes the unlink request before
geometry. It stops before `80088E50`; the flag is not a contact
receipt and no unlink executes. The fixture restores its own scratch
state before the positive visit. That visit stops at **`8008A7BC`**
before descriptor-gated activation and registry writes. Static
inspection distinguishes three source owners: allocator insertion
(`80088AA0`, skipped by the supplied cell seam), the carrier's late
update registration (`80089100`, not yet reached), and the class-4
sweep (`80089AEC`) followed by attack-registry publication
(`8008A2BC`). None is implied by the local shape transform.

A later narrow contact comparison is defensible only after a
separate same-carrier witness produces actual list insertion and
visit, publishes that shape to the attack registry, and supplies a
separately qualified receiver with a miss/refusal control. Do not
insert a synthetic registry pointer or treat the unregistered shape
as a hit. Static fixture and descriptor checks passed; the fresh
`retail-0600-rifle-shape-prefix-v1.json` output is unused. No replay,
compile, package, or engine changes.

**Reserved first-shape result:** the launch-v2 prefix matched. With
the supplied stopped byte, the original guard requested unlink for
the same shape and stopped before `80088E50`; shape bytes and stock
were unchanged. The positive visit entered the sphere initializer
and reached its epilogue, then the evaluator refused `8003A5E0`.
That address is the original `blr`, excluded by the fixture's
half-open text span. The receipt records the initializer's final
instruction path but no center/radius/endpoint readback or return
to `8008A7BC`. It is not a completed geometry or activation receipt.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-shape-prefix-v1.json`
SHA-256: `a557179f81bdfcb033f0434f031ef47dd0e0a989d98d44f97c0530cbf182b11d`.
One process, **0.219 s** wall, **5,753,916 KiB** available at launch,
**31,040 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. No retry, successor, compile or admission.

#### Grounded B and rifle refill: shortest next source witnesses

The existing Machine Red movement checkpoints are not resumable
actor images. The static-height receipt stayed at selector 0 for
four visits from a supplied Y=150. The threshold-height receipt
used Y=235, observed floor contact on visit 18 and neutral on 44,
then settled idle on 45 and admitted a stick walk on 46. The
wider air-dash receipt also reached neutral and accepted a stick
walk (70/71). Use the threshold-height **visit-45 idle** boundary,
not the following walk, as a regression reference for a grounded
shot fixture. It must be recreated with a valid constructor; do
not restore state from these JSON receipts.

| Boundary | What the saved movement fixture actually owns |
| --- | --- |
| Constructor and stock | `bulk-animation-loaded-rig.py` runs loading, install and rig construction, but at `8005614C` executes only `800563D8..80056414` (scalar/divisor setup). It bypasses the level-row load `80056464..80056598`, which normally installs B count/previous/cap 5, mode 0, preset 300 and an unarmed clock on normal level 1. The loader also declares ancillary and body-binding seams; it is not a whole scene bootstrap. |
| Machine Red class init | The movement harness supplies the `800C92B8` result. The actual normal branch calls shared init `800C74EC` with key 25, requests keys 0–3 through `80185F6C`, and writes `+0x4A8=1`. The two factory allocation sites are `800C7544` and `80185FF8`; initializer execution with explicitly scoped null pool returns would be a refusal-context witness only. Successful dependents would need source-created cells/list ownership and their required update visits. The paired-animation harness also cuts those helper sidecars. |
| Input and arbitration | The fall harness reads original processed stick packets and runs the conditioner. It declares no competing action at `800699D8`, no class action at `800696F4`, and bypasses other action/status edges. `8005913C` checks real ground loss at `80059174` before calling `800699D8` at `80059184`. The saved stick walk does not establish B. The detached Red action tests cover table selection or a supplied class/cue, not this joined handoff. |
| Refill | Both movement receipts bypass `8005568C` at `80055590`. Their explicit post-visit refill probes cut at `80055A2C..80055AF8` for the movement meter, not rifle stock. The deep Red stock cases separately verify mode-0 arithmetic with a supplied step/inhibit state, not its scheduled cadence. |

**Shortest grounded B witness:** on the same loaded `0600/0`
normal level-1 actor, first run the omitted setup owners or stop
at the first unsupported call. At minimum, observe the actual
stock-row writes (not supplied counts), exact class-init request
results and a valid same-owner settled ground pose. Keep the
known source floor queries and explicitly label any supplied
placement, processed pad packet, roster or null child result.
`8006A104` clears a target bit through **all six** slot pointers
at `803C4E84` before installing the action; a singleton with
unmapped slots or six aliases is not a valid join. The fixture
needs a qualified roster or six distinct, declared scoped slot
records and no unproved target-producer result.

Feed one neutral packet then a fresh processed B packet (`0x200`,
zero stick), via `8010D6E8 → 80056900 → 800685C0`. Allow the actual
ordinary grounded caller to arbitrate. The pinned ranged row at
`8030A2A8` is `(44,3,0,0)` (selector, lane flags, class state,
substate), and selector 44 maps to upper 61/lower 0. Observe
the selected row and the entry's **check-only** B stock probe
at `8006A37C`, then let the original captured upper dispatcher
enter class state 0 (`800C9510`) and request the program from
its own state. Do not seed the VM request or start at the already
verified post-cue consumer. Continue with neutral/no-repeat input
only until the first actual upper `8030906A` store (or a prior
blocker), keeping selector, request, mask, VM step and pose
publication tied to that owner. A neutral/no-B packet control
should not select the shot. Close/charged B and movement during
the shot are separate. Count visits from the accepted packet;
the supplied-program 16-visit receipt is not that count.

**Separate stock-clock witness:** preserve an actual partial
debit (no clock arm) and emptying debit (`0x778 = 300`), without
filling in any failed emitter or refund. On subsequent actor
visits, observe the real `8005568C → 80055D08 → 8006DCC0` stock
call, before the later `80059068` input/priority work. The
updater checks rifle inhibit bit 0 in `+0x597`, positive clock
and mode 0, and reads the established `+0x768` stock step
(multiplied by 10 when `+0x6FC` is nonzero). Ordinary rate 1
is a pinned source operand at `8005A410`, not a supplied native
tick. Observe the actual rate producer and its carry into the
next visit, the first accepted decrement, any refusal, and
the first `0/300… → 5/0` restore. This stock gate is separate
from the movement air cache. If a full wait does not fit the
reserved budget, stop on the proven prefix; any supplied
near-expiry differential must remain a separately labelled
arithmetic check, not claimed elapsed refill timing.

Static checks pinned the three movement receipts and deep
stock receipt, exact stage edges and class/stock/arbitration
call targets, selector/row bytes and default stock-step operands.
No source-owned setup → grounded B → cue or scheduled refill
receipt exists yet. Outcome-v1's scoped amount 8 supplies none
of these inputs and does not admit the package. No new replay,
compile or engine change.

#### Machine Red stock/initializer prefix and class-2 continuation

The private `build/borg-ordered-audit/retail-0600-rifle-ground-init-v1.py`
reuses the exact threshold-fall parent's loader and normal `0600/0`
constructor path. It does not restore an actor from the fall JSON
or run the parent movement sweep. From the constructor's actual
`80054414` call it replaces the old `80055F90` and `8005614C`
substitutions with a byte-pinned instruction/read/write domain
and no service callbacks in that domain. It checks the loaded
AFS 575 actor data and actual level index, follows variant-map
byte `2` to row `802E6BD4`, and requires the original source
writes for max HP 500 and rifle count/previous/cap 5, mode 0,
preset 300 and clock 0. The receipt must distinguish those writes
from a successful return of `8005614C`.

The reserved v1 run stopped as predicted **before `80088AA0` at caller
`8001C334`**, inside the stock initializer's `80056834 →
8001C2C8` ancillary path. Exact family/model dispatch chooses
`802CA150`, whose first entry requests `(owner, 2, 24, 0, 0)`.
The fixture records that call without producing a cell or
returning null; any unreviewed edge or read/write stops earlier.
This is **not** the later Machine Red hook at `8005445C →
800C92B8`. Even after the ancillary service, the path still
crosses body/collision binding at `80054444` and actor setup at
`8005444C` before the shared key-25 and four model-specific
dependent requests. None of those outcomes is supplied by this
fixture.

Six distinct records are defensible **for the slot-mask leaf**:
`bulk-scene-roster.json` witnesses `800536B0` publishing six
distinct `0x1E00`-stride records, including inactive records.
That can back `8006A268..8006A2D8`'s six accesses to `+0x5E6`;
it does not establish the target producer or the optional set
through the live `+0xCC` peer. Separately, registry-v4 uses
declared class-3 and class-4 pools and a supplied render-descriptor
capacity operand. It does not own the class-2 pool or the descriptor
allocation and dependent updates for this constructor. Neither
receipt is imported as actor state or used to skip a call.

Accordingly the prepared fixture sends **no pad packets** and
does not run fall/landing, input arbitration, the program request
or cue. After constructor services are qualified, reuse the
same-owner threshold-fall setup and visit-45 idle control, then
publish neutral followed by one zero-stick `0x200` B edge. A joined
visit must also replace or stop at the inherited timer/priority
services rather than jumping directly to `800699D8`. Do not seed
the selector, target, `P2:0` or cue. Rifle refill remains a separate
fixture.

`retail-0600-rifle-ground-init-v1.json` verified those stock writes
without returning from `8005614C`. SHA-256
`5b398e66b122e0252b26758373928201db9d3095febacf4bd0ade3ffc04de3bc`.
One isolated process, 6.360 s elapsed, 5,660,096 KiB available at
launch, 150,520 KiB peak RSS, 1 GiB soft/hard cap, 54 s internal
and 58 s outer caps. No retry, install or admission.

**Class-2 continuation passed.**
`build/borg-ordered-audit/retail-0600-rifle-constructor-pool-v1.py`
rebuilds v1 on the same actor and stack; the pinned JSON is a
comparison, not a saved frame. Drill's
`bulk-scene-drill-first-constructor-allocation-v1.json` proves a
class-2 pop, source group-1 insertion and four descriptor pops.
`bulk-scene-flying-saucer-constructor-v1.json` proves repeated
class-2 group-25 insertion through `80088AA0`, including the
shared class helper at `800C7544`. These are allocator precedents,
not Machine Red's children or permission to reuse their state.

- The new fixture declares exactly two distinct free cells sized
  `0x1C4` and tagged `2` from `802DA660` / `802DA672`, with a
  cold group-25 list. The capacity query reads a declared empty
  row-0 descriptor header; both real `8001C334` requests have
  `r7=0` and need no descriptor pops. The original allocator
  must pop, clear and insert both objects and the original caller
  must write both type-6 headers (source keys at child `+0x89`:
  `16`, `19`). Neither returned pointer, linkage, header nor
  descriptor outcome is pre-seeded. Group 25's physical tag comes
  from source byte `802DA719`. No child is updated or retired.
- After the second header it requires the actual `8005614C`
  return, the original scalar/rig setup, and the `8003CC24`
  collision pointer handoff from `Scene+0x80` to actor `+0x3FC`.
  The pointer is checked against the original delivery of AFS
  575 role 2 (7,840 bytes); it is not a registered body. The
  later `8008AB30` binder is outside this fixture.
- **Next stop:** `8001C858 → 800055FC`, the first draw inside
  `8001C80C`, or any earlier unqualified edge. The fixture does
  not map, reset or provide a result for the byte RNG at
  `r13-21722/-21721`. A live join needs same-generation common
  startup (`8003FD58` is the candidate initializer call) or an
  ordered draw ledger. A narrower future comparison could
  explicitly declare a fresh isolated stream *before* any
  consumer, run its original initializer and draw, and stop
  before the profile helpers. A full ancillary return still
  needs the mode-owned slot values (`Scene+0x20/+0x32/+0x3E`),
  override selector `Scene+5597` and its selected slot values,
  and a qualified roster/target producer; v1's scratch Scene is
  not evidence for those inputs.

Only after that return does `8005445C` dispatch `800C92B8`.
For ordinary settings, source key 25 at `80307390` requests
one descriptor in shared `800C74EC`; `80185F6C` requests two
for each selector 0–3. The positive branch therefore needs
five further class-2 cells and nine descriptor pops from a
qualified row-0 free list, with live owner/list and update
ownership still to be shown. `+0x4A8=1` is the hook's write,
not an admissible initializer substitute.

`retail-0600-rifle-constructor-pool-v1.json` SHA-256
`32c30b49b35f003a19c7577ef02cfb80a792968f2be127e5cbe83ab7fdda8d50`
records original group-25 insertion and type-6 headers for
keys 16 and 19, stock-initializer return and the body pointer
handoff, followed by the planned stop before the first RNG draw.
One isolated process, 6.332 s elapsed, 5,706,312 KiB available
at launch, 150,860 KiB peak RSS, 1 GiB soft/hard address-space
cap, 54 s internal and 58 s outer limits.

**First constructor byte draw passed.**
`build/borg-ordered-audit/retail-0600-rifle-constructor-rng-v1.py`
rebuilds the exact pool prefix and verifies it against the
pinned receipt before continuing in the same frame. It
requires the explicit deterministic inlet seed `--byte-seed
01c3` (high:low); the same bytes are written by original
initializer `800055E0`, but this is a supplied **current draw
input**, not an assertion about the stream at this point in a
retail match. The fixture does not run the initializer at the
constructor cut or reset any previously mapped state.

- Original common startup calls `800055E0` at `8003FD58`.
  The separate `8001825C` path saves two prior live bytes and
  resets them temporarily; `80018270` restores the byte pair.
  An exact live join requires that upstream owner and all
  intervening draws. Antares' private witness executed the
  same RNG leaf before a later stage-selection stop; its state
  is not reused here.
- `800055FC` uses **low at `804360C6`, high at `804360C7`**.
  It stores the new high at `8000561C` and low at `80005624`,
  and returns the new low. `source_byte_rng_owner` and
  `source_byte_rng_step` provide a cross-check, not a source
  substitute. The halfword draw `80005630` owns `804360C4`
  and is not mapped or used.
- The fixture expects those two source stores to yield high
  `05`, low `C8`, return `200`. It then requires the natural
  `8001C85C` halfword store of `200` to actor `+0x33C` and
  the original clears of `+0x308`, `+0x328`, and `+0x30F`.
  It stops at **`8001C874 → 80021B9C` before reading
  `+0x438`** from the scratch Scene-derived actor. No second
  draw, profile table selection, Scene override, target result,
  allocation result, or class-hook output is supplied.

Going farther requires mode-owned `Scene+0x20/+0x32` for actor
`+0x438/+0x439` (plus any relevant `Scene+0x3E` slot state).
The five timer helpers consume five more draws: the first
selects its table at `802CFA58` with `+0x438` and indexes it
with `+0x439` and `draw&7`. The native timer-prefix planner
requires a declared model/input domain and does not grant
ownership for Machine Red's scratch Scene. The remaining
ancillary path reads `Scene+5597` and selected slot overrides,
crosses `8002532C` target selection and `80020FD4`, and draws
again at `8001C9B0`; none of those paths is covered by this
cut. The five class-2 cells/nine row-0 descriptors above are
for the class hook alone. Later group-56 setup and
`8008AB30` body registration have separate gates and resource
requirements.

`retail-0600-rifle-constructor-rng-v1.json` SHA-256
`be9219aac542d3bc8cad05e31b2d0502203a359adece6f4a292e0008cfdaa690`
verified the two RNG stores, return `200`, and the four constructor
writes, then stopped at the planned profile inlet. One process,
6.407 s, 5,720,388 KiB available immediately before launch,
150,844 KiB peak RSS, 1 GiB soft/hard limits, 54 s internal
and 58 s outer caps.

**Singleton Scene ancillary and class phase: stopped at first helper return.**
`build/borg-ordered-audit/retail-0600-rifle-constructor-class-v1.py`
rebuilds that exact RNG prefix on the same actor and stack.
The native timer/target audits in `docs/battle-scene-mvp.md`
give the service order and null-target model-domain precedent,
not Machine Red evidence. Drill's ancillary preflight stopped
before a mode/profile join. `bulk-scene-roster.json` supplies
the six physical slot/stride precedent, not the actor state or
target result for this fixture.

- **Declared inputs:** seed `01c3`; slot-0 `Scene+0x20/+0x32/+0x3E`
  and override selector `Scene+5597` all zero, settings `+0=0`,
  `+50=1`, and one active slot (the rebuilt Machine Red in slot 0).
  The fixture requires original stock writes of the first three
  Scene bytes to actor `+0x438/+0x439/+0x43A`; it does not
  write those actor fields itself. The five other `0x1E00`-stride
  slots expose only their inactive bytes. These are declared
  fixture inputs, not a claim about an authored retail mode.
- **Ancillary phase:** run the five original profile helpers
  against the pinned selector-0/level-0 rows and the carried
  byte stream. Then run the source override branch and
  `800263EC`, `8002532C → 80025944`, `80025A84`, `80020FD4`
  and the last draw. Require the original scan to read six
  activity bytes and return null; do not preload a query result.
  Expect eight additional draws, timer writes
  `304/614/612/580/556`, delay `15`, and both writes to `+0x328`
  (`82` then `5`). The changed-target path at `80025884 →
  80023BF4`, live-peer eligibility/distance, and the other
  Scene/profile branches are refused, not replaced.
- **Class phase:** allow actual `+0x4B8` dispatch into `800C92B8`,
  then the shared key-25 request and four dependent selectors `0–3`.
  Only free inputs are declared: five distinct class-2 cells
  (452 bytes) and nine distinct row-0 records (44 bytes, original
  `80006F60` initializer). Require original pops, group-25/group-1
  list insertion, descriptor linking, owner/credit links,
  initialized headers, and the class writes `+0x708=1` and
  `+0x4A8=1`. No descriptor or class result is provided.
- **Planned stop / next unsupported service:** `80054464 →
  800807E4`, after the real class returns to `80054460` and
  before the separate group-56 setup. That setup uses
  `Settings+0x40` and `Scene+0x45` gates and, on its allocation
  branch, a separate cell and six descriptors; the following
  `800813A8` service can request another cell and one descriptor.
  Neither is justified by the class pool alone. If an earlier
  instruction, read/write, or service leaves the pinned domain,
  the fixture stops there instead. Child updates, body registration,
  grounded B, and scheduled refill remain outside the phase.

The sole reserved run of `retail-0600-rifle-constructor-class-v1.py`
rebuilt the passing RNG prefix and verified the original
`+0x438/+0x439/+0x43A` zero writes from the declared Scene inputs.
It entered `80021B9C`, executed the next byte draw (both stores
and return), fetched the pinned row entry, and executed the
halfword store at `80021BEC` to actor `+0x32E`. The saved path
proves these instructions completed; the predicted values
(`217` return, state `11:D9`, timer `304`) are derived from the
pinned input and instructions, not a separate readback in
this receipt.

The actual stop was **`80021C04` before `blr`**: the fixture's
half-open text range excluded that return instruction. This
was a fixture range error, not a missing retail service.
The six-slot fixture inputs were set up, but the scan, remaining
timers, ancillary return and class hook did not execute.
No additional class object or descriptor inputs were installed,
and no class allocations or marker writes were observed. The
only observed allocations were the rebuilt prefix's two type-6
children. The group-56 planned stop was not reached.

Exclusive receipt `retail-0600-rifle-constructor-class-v1.json`,
SHA-256 `ee955e4e61cef5cd9eb609b63b6dde480970c28799aaca8abcd6eb69218932ef`.
Syntax and pins passed; one isolated Python process, 6.334 s
elapsed, 5,726,868 KiB available immediately before launch,
150,868 KiB peak RSS, 1 GiB soft/hard cap, 54 s internal and
58 s outer caps. Prior receipts and the fixture were preserved.
No retry, successor, compile, grounded B, or Battle Test admission.

**Constructor class v2 passed under the declared singleton Scene.**
`build/borg-ordered-audit/retail-0600-rifle-constructor-class-v2.py`
pins the failed v1 script and receipt without editing either. It
includes the single omitted instruction `80021C04` and narrows
the other text spans to the declared singleton path. The static
control-flow walk checks direct and indirect destinations and
matches calls to their continuation addresses. It covers 383
ancillary instruction sites, 395 edges and 19 return edges;
464 class/allocator instruction sites, 496 edges and 14 return
edges. These counts are static, not replay observations.

The audit checks access widths at all 167 ancillary and 209
class/allocator memory instruction sites, including byte
selectors and RNG stores, halfword table values and free
counts, word links and callback pointers, and stack saves.
The fixture checks reached edges and actual access widths
against these maps, with the existing memory bounds. The two
exact word store-update instructions in the cell clear retain
their pinned adapter. Off-scope branches refuse instead of
receiving an invented result, including a live peer/change of
target, enabled Scene overrides, other class branches, and
descriptor-initializer failure/unwind.

The reserved run read back **`+0x32E = 304`** after the first
helper returned to `8001C878`; the byte RNG state was low/high
`217/17`. Original ancillary code wrote the remaining timers
`614/612/580/556`, scanned all six activity bytes and derived
a null target, wrote delay `15` and `+0x328 = 82 → 5`, and
returned to `80054450`. The seed `01c3`, four zero Scene
operands, settings `+0=0`/`+50=1`, slot 0 as the only active
actor, and zero initial target history are fixture inputs,
not live retail mode or target evidence.

Original `800C92B8` popped and cleared all five supplied
452-byte class-2 cells and allocated/linked all nine supplied
44-byte row-0 descriptors. It inserted the type-14/key-25
child `00121000` in group 25 with descriptor `00130000`;
the type-0/keys-0–3 children `001211C4`, `00121388`,
`0012154C`, `00121710` were inserted in group 1 with two
descriptors each. Both free counts reached zero. No allocation,
target or header output was supplied. The hook set `+0x708`
and `+0x4A8` to `1`, returned to `80054460`, and the fixture
stopped at `80054464 → 800807E4` before the group-56 service.

Exclusive receipt `retail-0600-rifle-constructor-class-v2.json`,
SHA-256 `bf0b83f03e0cbc1d76eabcfb801a55f12b426504c5418325db79581a93a3aff3`.
Syntax and pin preflight passed. One isolated process,
6.316 s elapsed, 5,711,608 KiB available immediately before
launch, 151,612 KiB peak RSS, 1 GiB soft/hard limits, 54 s
internal and 58 s outer caps. Prior receipts were preserved.
No retry, successor, compile, installation, grounded B,
refill timing, or Battle Test admission.

#### Machine Red post-class construction and grounded input gates

Passed
`build/borg-ordered-audit/retail-0600-rifle-post-class-v1.py`.
It rebuilds class-v2 on the same actor and stack, compares the
passing receipt, then covers **both group-56 factories and the
entire body binder**. It stops before original placement
`800544D4 → 80056D28`, or the first earlier unqualified edge.
No position, pose, child return, body header, or target value
is injected as a result.

- `800807E4` / `800813A8` read the retained declared
  `Settings+0x40=0`, `Scene+0x45=0`. Original `8008893C`
  must pop two distinct class-2 cells into group 56 and take
  six then one row-0 descriptor. Verify kind `0x14`, parent
  aliases, and the exact update/draw pointers. The two
  allocation sites and their positive results are already
  demonstrated in the pinned Drill constructor-body-v2
  receipt; Emperor placement-03 is a **preflight**, not a
  second passing replay. Neither supplies state to Red.
- `8008AB30` must bind all 14 signed-list entries `0–13`
  from the already delivered AFS 575 role-2 bank (7,840 bytes;
  SHA-256 `89e4a919e5989aad5b2fb36985d55859b8cfdf9387bec0149265cbf716b9aec3`).
  The fixture declares fourteen free 448-byte class-4 cells.
  Original code must insert group-49 headers, keep the exact
  descriptor/matrix pointers, and publish only row 7 to the
  local `+0x530` pointer array (flag `0x200`). This scope
  allocates no descriptor for the bodies and makes no collider
  transform, sweep, or hit claim. No HUD initializer or draw
  runs. The supplied stock is separate from the existing
  occupied group-1/25 and active descriptor chains.

**What remains before the first update and grounded B:**

1. **Placement is the next unjoined constructor service.**
   `Settings+0x32` (decimal 50) is **1** in class-v2, so
   `80056D54 → 800572F4` takes the team/table branch. With
   Machine Red's source movement type 0 and first `+0x43B=0`,
   it uses `Scene+0x1C` (stage ID), retained actor team `+0x88`,
   and the current team cursor at `Scene+0xFE+60*team` to
   read `802DDF00` and advances the cursor. It then needs the
   same stage's `r13-21316` header and three qualified STIH
   grids for `8005744C` and `80057558`, plus fresh dynamic
   registry counts at `80436244/8043624C` for `800575A8`.
   The exact fall precedent has ST42 AFS 473 grids and a
   deliberately supplied Y=235 slot; that uses **the other
   settings branch**, not this table placement. Qualify a
   same-actor ST42/header/grid/cursor handoff (with face 1130
   still refused), source-owned empty counts before any
   collider update, and actual result/history writes. Do not
   toggle setting 50 to reuse the elevated fall result.
2. **Constructor return is not input admission.** After
   placement require actual selector 0 installation
   (`800545E4/800545F0`), the `80054610 → 8004BEB8`
   initial `(group 0, choice 8)` request on Red's loaded bank,
   constructor tail/return, and the enclosing first-world
   work (`80054140` bounds, `80054160 → 800465FC`,
   `8005418C → 80006284`). That is the next defensible
   complete update boundary after a stage join. The first
   phase-1/substate-0 appearance visit `8005475C → 800547DC`
   has its own colour/fade and Scene gates; it is not a
   `80055568` input/action visit. Do not seed the subphase
   forward or borrow the other models' first-world outputs.
3. **An actual human port and running phase gate B.** The
   retained singleton recipe has the clear human slot mask;
   `800543C0/800543C4` select nonhuman mode. In
   `80056900`, that mode bypasses the port packet load and
   can call `8001C9D0` instead. A fresh B witness therefore
   needs an explicitly human-enabled slot via `Settings+2`
   **before reconstruction**, with its own checked scalar/init
   results and a source-produced transition to pad-consuming
   `80054CBC/80054CF0`. Retain a valid ground contact and
   neutral/no-B control, then publish one fresh B. The old
   fall adapter's direction conditioner excludes the charge
   history tail and its scheduler masks arbitration/timer
   calls; none may stand in for the corresponding real B
   visit. For `8006A104` give six distinct slot records with
   `+0x5E6` in bounds; the post-class singleton only qualified
   the initial scan's activity bytes.
4. **HUD is not the rifle input or VM owner.** The child
   initializers `800808B4/80081478` need the owned cockpit
   archive and their callback-list custody when a scene
   scheduler actually dispatches them. They do not gate a
   bounded actor-only pad/arbitration/cue prefix. A full
   scheduler cannot silently skip them or body updates.
   Conversely, target absence does not inherently forbid the
   scoped shot: `8006C674` has an original null-target sector-0
   return, and `8006BDA8` has a null target path. Live selection,
   close B, and release aim remain separate; if a peer is
   introduced it needs real target producers and the `80023BF4`
   transition, not a seeded target or aim result.

The exclusive `retail-0600-rifle-post-class-v1.json` receipt
(`e168eb934cdbf74f7edea09b549e57c1d3372682572d1ff92944906382a433a0`)
records two original group-56 headers, `00122000` (six
descriptors) and `001221C4` (one), and all fourteen original
group-49 body headers; only row 7 entered `+0x530`. It stopped
before placement. One process, 6.524 s, 5,719,584 KiB available
at launch, 152,384 KiB peak RSS, 1 GiB soft/hard limits,
54 s internal and 58 s outer caps. No successor launched.

**Same-owner ST42 placement v1 stopped at the original table read.**
`build/borg-ordered-audit/retail-0600-rifle-stage-placement-v1.py`
rebuilds post-class-v1 on the same actor and stack. It declares
**row 2/style 2/AFS 473 (`st42.pzz`)**, the exact combination
from `bulk-scene-stage-ownership.json`. The row-0/473 control
is explicitly inapplicable (the loader would choose 471);
the elevated fall receipt supplies neither this table branch
nor a placement result. The fixture keeps `Settings+50=1`,
team 0, and initial team cursor 0. Original `80087718`
publishes the row-2 header on the retained Scene, and
`8003D4A0` relocates all three hash-pinned raw STIH roles in
bounded row slices. No retail stage model or render resource
is claimed. Original `8008A28C` clears the fresh count range
from canaries; no shape sweep owns any candidates here.

The unmodified constructor call enters `80056D28` and
reads team/table coordinates from `802DDF20/24` (800, 1400).
The corresponding STIH cell is 1031, with seven primary
face references and no secondary/tertiary references. Face
1130 remains refused regardless of the query. The fixture
requires original vertical floor, secondary-segment and
dynamic-query service returns; dynamic counts are read as
halfwords from `80436244/8043624C`, with no candidate array
supplied. Heading uses the scoped finite `atan2` numerical
ABI at `8023CB78`; original code still quantizes and writes
the heading. No pose, Y, query result, or phase is supplied.
Natural return readbacks check cursor increment and matching
position/history fields on the retained owner.

**Planned stop:** after placement returns to `800544D8`, or
earlier on an unsupported edge or access. This does **not**
claim a grounded update. Next comes the constructor's actual
selector/boot request and first-world work, including current
program/VM resource ownership (the existing staged pool
covers row 0, not arbitrary motion-descriptor outcomes).
Then the source appearance/phase and human-port gates above
still have to be joined before a B witness. No B, cue, pose
epoch, input phase, or Battle Test admission is seeded.

The one reserved v1 run passed the row-2 stage setup assertions:
original `80087718` published the header (height cap 5000,
fallback floor −55), original `8003D4A0` relocated all three
AFS 473 grids (215,748 / 126,948 / 36,484 bytes), and
`8008A28C` cleared all ten count halfwords from canaries.
The retained actor entered placement and reached the team
cursor store at `800573A0`, then stopped **before the 4-byte
table read at `800573BC`**. The receipt does not contain a
separate cursor readback.

The fixture pinned the wrong row address. The original
address is `802DDF00 + 4*(16*stage + 8*team + 2*cursor)`;
row 2 / team 0 / initial cursor 0 selects **`802DDF80/84`**,
not the v1 `802DDF20/24` pin. The correct source bytes decode
to **X/Z = (−800, −1400)** statically. That corrects the
address audit only: no coordinates were loaded, and no
floor, projection, history or placement return was observed.
The earlier cell-1031 cross-check does not apply to this row.

Exclusive receipt `retail-0600-rifle-stage-placement-v1.json`,
SHA-256 `875c2450b80167a2783c3f1914a7ae7625279b34b4a9232e7019876644f4829d`.
Syntax and pin preflight passed. One isolated Python process,
10.180 s elapsed, 5,674,632 KiB available immediately before
launch, 231,368 KiB peak RSS, 1 GiB soft/hard limits, 54 s
internal and 58 s outer caps. Earlier receipts and the failed
fixture are preserved. No retry, successor, compile, engine
or package change, grounded state, B/cue, or Battle Test.

**Same-owner ST42 placement v2 passed.**
`build/borg-ordered-audit/retail-0600-rifle-stage-placement-v2.py`
pins both v1 artifacts without changing them. It pins the
original row/team/cursor index instructions and the two
four-byte source reads at `802DDF80/84`. At `800573D4`,
before the floor service, it requires both source reads and
the source cursor stores `0 → 1`, then reads the cursor,
actor subselector and X/Z back from memory. Heading consumes
the actual negated source X/Z `(800,1400)` via the same scoped
finite `atan2` numerical adapter.

The three ST42 grids have 500-unit cells, 42×42 dimensions,
and X/Z origin `(−11000,−11000)`. Original lookup is
X-major: `(20,19)` selects cell **859**. That column has
**27 primary references, zero secondary, zero tertiary**,
with no face 1130. V2 limits query reads to those headers,
cell entries, reference lists and exact face records; it does
not supply a floor or projection result. Setup still runs the
real header publication, three relocations and ten count
clears. The two dynamic count reads remain halfwords at
`80436244/8043624C`; candidate-loop edges remain refused.
Actor writes are narrowed to the published position, angle,
history and placement fields; no writable mapping was added.

Static checks closed the full declared route through the
floor/secondary/dynamic returns, nine position-history words
and original `blr` to **`800544D8`**. This also pins the
math/return edges, accessed constant widths and out-of-scope
branches. Syntax, seven parent pins, 588 memory-width sites
and 36 query read ranges passed. `800544D8` is still the cut
before constructor boot and first-world work; none of those,
nor grounded input or cue timing, is claimed.

The one reserved v2 run published the row-2 header at
`007A3000` (cap 5000, fallback floor −55), relocated 1,764
cells per grid and cleared all ten registry halfwords.
It read `802DDF80/84`, selected X/Z `(−800,−1400)` and
read back cursor 1. Cell 859 returned floor
`17.673864364624023`; the original secondary and dynamic
projections both returned 0. The readback was
`(−800,17.673864364624023,−1400)`, raw yaw 5414, and
matching position histories at `+0x2C/+0x5E8/+0x354`.
The original helper returned to `800544D8`, with no actor
update, boot, shape/HUD update or input consumption.

Exclusive receipt `retail-0600-rifle-stage-placement-v2.json`,
SHA-256 `ee330844e4c0c0f0448e68e8ec2da282625ca5e15c194ffa42aeea626e92f0db`.
Syntax and pins passed; one isolated process, 10.140 s
elapsed, 5,681,916 KiB available immediately before launch,
231,636 KiB peak RSS, 1 GiB soft/hard AS, 54 s internal and
58 s outer limits. All seven parent pins remained unchanged.
No retry, successor, compile, grounded B or Battle Test.

#### Machine Red constructor boot and first-update prerequisites

Passed through the original boot request and descriptor pop:
`build/borg-ordered-audit/retail-0600-rifle-constructor-boot-v1.py`.
It rebuilds the passing ST42 placement receipt on the same
actor and stack, runs the original post-placement stores and
selector-0 installers, then follows the actual `80054610 →
8004BEB8` boot request. Normal Machine Red's mask 7 intersects
the request mask 15. Group 0, choice 8 selects **`80308DDE`**,
not the rifle program. Opcode 1 supplies local descriptor
group 0, choice 5; the lane-0 record at `803096DE` selects
AFS 575 role-3 **clip 0:9** at raw bank offset `0x13FC0`.
The fixture checks both the exact raw bank and the current
relocated slot; no common bank sample or clip is substituted.

`8004D548` strips the serialized clip tag and requests tag 2
at `8004D5F8`. Row 2 of `8036F644` has 16-byte descriptor
cells and initializer `80006E3C`. The fixture declares one
distinct free cell, its tag, link/count and the pinned row
header. Original code must pop it, link it as active and
hand it to the initializer. **Planned stop: `80006E54 →
8025AFA4`, before the 68-byte SDK heap request is serviced.**
Both loaded-rig and paired Seiryu evidence use declared
SDK/heap outcomes; neither owns the current Red heap
callback at `803FA748`, its allocation result, nor the
memory/descriptor lifetime. Row-0 class allocations are
not evidence for that service. The fixture adds no heap
callback and refuses an earlier unsupported access.

The audited continuation after that fence still needs:

1. The real row-2 header allocation and initialization, then
   `8004D60C → 8025B00C` archive relocation and publication
   into the exact bank/lane slot. Keep any additional lane
   binding, boot wait and SDK track/pose work tied to the
   current resource. The loaded-rig Red fall receipt
   demonstrates the pre-existing boot path **with** SDK
   seams, not current allocation or completion here.
2. Return from the boot request, execute the null-peer
   `+0x508` guard and ordinary mode path at `80054630..80054758`,
   and finish the enclosing `80054070` visit: bounds at
   `80054140`, world at `80054160`, colour tail at `8005418C`.
   Compare current root/contact results against the retained
   ST42 grids, not another actor's first world or a claim
   based on placement Y. No constructor return or pose is
   claimed by the boot-prefix fixture.
3. The first scheduler-owned visit is not another direct
   movement-frame call: the `80029434 → 80053A2C →
   80054070` slot visit and surrounding scene order need
   current owner/list/phase operands. With the retained
   phase 1/subphase 0 and fresh appearance state, the first
   actor dispatch is `8005475C → 800547DC`; it starts the
   actual appearance/colour path (`800548DC → 800061A8`)
   and is not `80055568` input. The existing ST42 unmasked
   startup audit and loaded movement visit are procedure
   references only. The former is a preflight; the latter
   declares the running subphase and substitutes several
   timer/arbitration services. Neither qualifies a first
   Machine Red scheduler visit.
4. A later neutral pad visit needs a **human-enabled
   constructor** and source-produced running phase. Existing
   `80056900`/`800685C0` processed-packet evidence is
   applicable to the corresponding human input domain, not
   to the retained nonhuman singleton or the appearance pass.
   Do not patch port/phase late, force grounding, or reuse
   the supplied arbitration answers.

Static preflight passed: 16 pinned text spans, 526
instruction sites, 544 edges, 17 call sites, 10 return edges,
214 access-width sites, and the exact Red boot bank and
descriptor inputs. The reserved run verified selector 0 on
both lanes and the original group 0/choice 8 request with
mask 15. It selected program `80308DDE`, descriptor
`803096DE`, and local clip 0:9. Original code popped the
supplied free 16-byte cell `00132000` from row 2, marked it
active and emptied the free list, then stopped at
`80006E54 → 8025AFA4` with request 68. No header, heap
callback or heap result was supplied. No constructor return,
scheduler entry, appearance or input was observed.

Exclusive receipt `retail-0600-rifle-constructor-boot-v1.json`,
SHA-256 `ef31fd40af2ffbfc32a5051ec58e806b80db4b4a2dff882fe20d062b10e8f3e5`.
One isolated process, 10.235 s elapsed, 5,699,588 KiB
available immediately before launch, 232,368 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer limits.
All four parent hashes remained unchanged. No retry,
successor, compile, engine/package change, B/cue or admission.

#### Boot heap ownership audit — constructor return still blocked

No pinned receipt qualifies an original HSD heap allocation
on this Machine Red generation. The bounded
`bulk-scene-challenge-dvd2499-functional-v1.json` (SHA-256
`022ed603ca15266e04419f8116269169b238ea1a4ea15a6c65ef5fdb92b37961`)
reached original OS heap creation at `801FFF30`, then stopped
at `8002A574 → 801FFFB0`. Its allocation and release arrays
are empty and `80088354` never published the game arena.
The exact 68-byte Seiryu, paired-rig and Scene archive
witnesses use declared storage providers. They pin the
ABI but not the original heap return. The ST42 stage-model
preflight (`bcb972e1ffa3f3f4f0380685fa99a1df9ebfac25cd3b91d53a9c300d359b97f3`)
and Antares case-20 audit locate the same unowned dispatch
read. No prior heap state or callback is transferable.

The original allocation path is:

- `8025AFC0` reads BSS `803FA748`, and `8025AFC8` calls its
  `+0` slot with `(68,32,0,803FA748)`. The default slot
  `80253F94` calls `802540D8` to read the current HSD handle
  (`80435F40`) and `801FFD38` to allocate in the OS table
  at `80436408`, with the descriptor count at `8043640C`.
  The allocator rounds the positive 68-byte request to a
  128-byte heap block including its 32-byte header; the
  source returns the aligned payload at block `+32`. A
  correctly linked active row-2 cell proves none of this.
- The original registration is not a pointer seed:
  `801FFEC0` publishes the OS descriptor array from arena
  bounds; `80253DF0`'s default branch installs the five
  slots via `80253E98 → 8025AE44`, which writes all 20
  bytes. It calls `801FFF30` for the HSD heap, writes its
  handle, selects it via `801FFEB0`, and initializes SDK
  statistics via `80257084`. Qualify the default-state
  word `804369D8`, arena bounds and optional pending bound
  pair `804369C0/C4`, `80204038`'s memory-size input at
  `80000028`, and exact range/list ownership together.
  The default table alone is insufficient; a later slot
  `+8` reset can destroy the backing with the same table.

**Shortest testable step:** a fresh, explicitly declared,
aligned guest arena with room for the OS descriptor table
and an HSD split (remaining free block at least 64 bytes).
Require non-overlap with the retained actor, ST42, existing
descriptor cells and resource banks. Run the original
descriptor-table initialization and default HSD registration
in the same evaluator, preserving the suspended constructor
frame. Verify the published dispatch, current handle and
empty allocated list before letting `8025AFA4` call the
original allocator. Require the source free-list split,
128-byte active block, aligned payload and source
`80006E5C` write of the result into `00132000+0x0C`.
Do not provide an SDK callback, heap pointer, heap result,
or copy a heap/list from the DVD receipt. This would prove
a **declared guest heap**, not retail startup.

The next owner after allocation is the boot archive, not
appearance. Exact clip 0:9 at `0x13FC0` is 7,656 bytes
(SHA-256
`d414a1cb532d6dc3d12bf429882d839ee4819743c148c20cb3ec2056512d3150`),
with 5,612 body bytes and 271 distinct aligned relocation
words, all with bounded target offsets. Original
`8004D60C → 8025B00C` clears and populates the new 68-byte
context and writes those relocations **in the retained
bank**. Its actual return must be checked before
`8004D610/614` publishes the descriptor to the bank slot
and current lane. Other lanes can reuse an already
published pointer only if they select that exact slot;
do not infer that the initial allocation completes all
lane work. Keep the backing live across all readers;
destruction uses row-2 `80006E8C` and the matching SDK
free path, not the row-0 descriptor recycle procedure.

Boot still crosses `8004A7DC → 8004B8F4` and
`8004A800 → 8004AAE0` for per-part track/pose work. A raw
archive-context allocation does not qualify cold SDK
AObj/FObj pools, live JObj roots, or the first pose. Only
after the actual `80054614` return can the ordinary
constructor tail run and return through `80054758`; the
outer visit still reads `8008B480` and then runs bounds,
world and colour work before `800541A8`. The first
appearance requires the next source-owned Scene slot
visit and its colour resources. These are separate from
human pad/grounded arbitration. The audit used read-only
source bytes, receipt pins and bounded relocation checks;
no replay, compile, gameplay change or admission.

**Original heap test passed under declared guest RAM.**
`build/borg-ordered-audit/retail-0600-rifle-boot-heap-v1.py`
pins and rebuilds the passing boot-v1 suspension, then declares
blank aligned guest RAM `00600000..00600400`, the 24 MiB
memory-size operand at `80000028`, two OS table slots and the
default/no-override HSD state. It rejects overlap with any
prior RAM mapping. In lower scratch it runs original `801FFEC0`,
the two arena setters and `80253DF0`, including the real
`8025AE44` dispatch publisher, `801FFF30`, current-heap selection
and stats. The only instruction adapter applies the exact
pinned non-stack `stwu` at `8025AE84`; no heap callback or
allocation result is provided.

After restoring the same suspended row-2 caller frame, the
fixture follows the original veneer, indirect dispatch and
`801FFD38`. Expected readbacks: handle 0, allocated block
`00600020` of size 128, payload `00600040`, remaining free
block `006000A0` of size 864. It then requires the actual
`80006E5C` pointer store into `00132000+0xC`, the active
row-2 link and original row-2 return to **`8004D5FC`**.
The bank slot must remain tagged and unchanged. This cut
precedes `8025B00C`, track/pose work, free/reset and the
constructor return. No source payload initialization is
claimed. Earlier unsupported code or access stops the fixture.

The one reserved run passed syntax/pin preflight and observed
original OS table initialization and the five-slot HSD dispatch
registration. Heap handle 0 began with a 992-byte free block and
no allocations. The 68-byte request split off 128 bytes including
the block header, returned payload `00600040`, and left the
864-byte free block at `006000A0`. Original `80006E5C` wrote
`00600040` into `00132000+0x0C`; the descriptor remained active,
and its caller resumed at the planned `8004D5FC` cut. The bank
was unchanged. No heap result or callback was supplied.

Exclusive receipt `retail-0600-rifle-boot-heap-v1.json`,
SHA-256 `ac572d400b90d3f4955b712702051129fc5087c6c52de79115a979d106041066`.
One isolated process, 10.276 s elapsed, 5,643,224 KiB available
immediately before launch, 232,668 KiB peak RSS, 1 GiB soft/hard
AS, 54 s internal and 58 s outer limits. All four parent hashes
remained unchanged. No retry, successor, compile, engine/package
change, archive relocation, track/pose, input or admission.

#### Machine Red boot archive publication passed

`build/borg-ordered-audit/retail-0600-rifle-boot-archive-v1.py`
rebuilds and compares the passing heap receipt on the same
Machine Red actor and stack. It keeps the active row-2 cell
`00132000`, original heap payload `00600040`, and current
tagged bank entry. It does not transfer an archive image,
allocate another cell, or supply a result.

- `bulk-animation-tagged.json` proves the source
  `8004D548 → 8025B00C` relocation/cache pattern and the
  repeated even-pointer lookup. Its clip `0:9` arms use AFS
  587 and 495, not Red, and supply allocator/clear/copy
  services. Red's exact `80308DDE` lane-clock entry in
  `bulk-animation-clock-coverage.json` is mask 2/group 0/
  choice 8, with owner branches explicitly not inherited.
  The exact normal Red fall receipt runs through loader/SDK
  seams and is not an allocation/track-owner receipt for
  the current boot actor.
- The new fixture pins the exact 7,656-byte local clip
  from AFS 575 role 3 at `0x13FC0`, all 271 distinct
  aligned relocation targets, 32 in-range named public
  entries and no externs. Original `8025B00C` must clear
  the 68-byte context, copy its 32-byte header, derive the
  section pointers, and perform exactly the expected writes
  into the current bank. Exact byte/word update-store
  adapters are limited to the clear/copy destination and
  produce no service return. The fixture checks context and
  relocation readback before `8004D610/614` publishes the
  active descriptor to the tagged cache slot and lane.
  No additional motion allocation or pose is supplied.
- **Planned stop:** `8004A800 → 8004AAE0`, before the first
  per-part track/pose call, after the actual publication and
  `8004B8F4` part-range helper return. Going farther requires
  current actor/style logical-to-JObj pointers at
  `+0x1DB4/+0x1DB8`, the appropriate relocated clip records,
  track attachment/sampling at `8004ACDC → 8004ADC8` and
  `8004B76C`, and independently owned SDK class/track storage
  and release paths. Inherited witnesses supply AObj/FObj
  allocation at `802572F0` and track teardown. The successful
  68-byte context heap split does not qualify those SDK
  pools or the old rig's supplied JObj results. Neither the
  remaining boot lanes nor constructor/appearance has run.

Syntax and pin preflight passed. The sole reserved run kept
the same actor, row-2 cell `00132000` and heap backing
`00600040`. Original code relocated all 271 words in local
clip `0:9` at `1048FBC0` and published the cell to bank slot
`1047BC64` and lane slot `00101AD8`. The context readback
verified body `1048FBE0`, relocation table `104911CC`,
public table `10491608`, and names `10491708`. It stopped
as planned at `8004A800 → 8004AAE0` with part key 2.
No additional allocation or completed pose was supplied;
the scoped clear/copy instruction adapters remained in use.

Exclusive receipt `retail-0600-rifle-boot-archive-v1.json`,
SHA-256 `e1c8ec65c464a8497d4e99529f43350841057a963e84b6879422666d4e4ef783`.
One isolated process, 10.676 s elapsed, 5,649,536 KiB
available immediately before launch, 235,628 KiB peak RSS,
1 GiB soft/hard AS limit, 54 s internal and 58 s outer
timeouts. All six parent hashes remained unchanged. No retry,
successor, compile, engine/package change, per-part track/pose,
constructor return, input, grounded B or admission.

#### Machine Red first per-part track inlet passed

`build/borg-ordered-audit/retail-0600-rifle-boot-track-inlet-v1.py`
rebuilds the passing boot archive and compares it on the same
actor, relocated clip, row-2 descriptor and original guest heap
payload. The style-0 key-2 JObj is the **retained scoped loader
outcome**, checked against the current `+0x1DB4` table and its
mapped class/empty-AObj fields. It is not a new SDK construction
or a claim that its full retail lifetime is owned.

Original `8004AAE0 → 8004ACDC → 8004ADC8` must select that
JObj and call the public-index and name-match leaves
`8025B178/8025B1B0`. For key 2 the exact clip entry names
`pl0600_b02_animjoint`, body offset `0x136C`, with its AObj
descriptor at `0x1A8` and bounded FObj chain `0x16C → 0x180 →
0x194 → null`. The fixture requires those real return operands
and the original `8004AFC4 → 8024DC04` attachment entry with
no stale JObj AObj. The only local arithmetic adapters are the
four pinned `subf.` compare instructions in the original SDK
name comparison. No name or match return is supplied.

**Planned stop:** `80243F38 → 802572F0`, with AObj pool
operand `803F8D90` and the exact current AObj descriptor.
Only stack saves are allowed. No actor, JObj, heap or track
write, allocation or pose publication is made in this slice.
An old-track removal edge, foreign JObj, name mismatch or
other unqualified branch/access refuses instead.

Going farther requires original AObj/FObj pool initialization
and access to their same-generation free/slab lists
(`803F8D90` / `803F8DE8`), matching allocation and release
paths, and qualified JObj methods and live track attachment.
The old Red rig and SDK track witnesses supply these storage
or teardown outcomes; the passing 68-byte archive-context
heap allocation is not an AObj/FObj pool receipt. Only after
the remaining per-part calls, lanes and boot tail return can
the constructor/world and later appearance work be joined.

Syntax and pin preflight passed. Original code selected the
retained style-0, key-2 JObj `02001800` and resolved
`pl0600_b02_animjoint` to `10490F4C`. It reached the planned
`80243F38 → 802572F0` fence with pool `803F8D90`, AObj
descriptor `1048FD88`, and FObj descriptor `1048FD4C`.
Only stack saves ran in the continuation. The JObj remained
a scoped loader outcome and no binding, pose, or allocation
result was supplied or published.

Exclusive receipt `retail-0600-rifle-boot-track-inlet-v1.json`,
SHA-256 `e9c948f0c2b792627c4dcce0c46a2fe3335c2b057449a0b2030e2233dfb837c7`.
One isolated process, 10.964 s elapsed, 5,656,612 KiB
available immediately before launch, 235,948 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All four parent hashes remained unchanged. No retry, successor,
compile, engine/package change, constructor return, appearance,
input, or admission.

#### Machine Red part-2 SDK track attachment — stopped after first AObj pop

No existing witness qualifies the allocator for this part. The
loaded-rig and paired G-Red/Sasuke witnesses return fixture cells at
`802572F0` and log release requests at `80248794`/`80257440`.
The ST42 BRIF material and Scene group-57 fixtures also use supplied
track storage, with per-record owner and retirement checks. They
establish the typed seam and the 28/48-byte clear sizes, not the live
SDK registry or free lists. The DVD2499 receipt ends after OS heap
creation, before any allocation. Machine Red boot-heap-v1 supplies
the usable precedent: original heap creation and one split under
declared guest RAM, rebuilt on the same actor. The later receipts
retain that heap, clip, row-2 descriptor and scoped loader JObj.

`build/borg-ordered-audit/retail-0600-rifle-boot-track-attach-v1.py`
scoped a single continuation:

- Static retail startup calls `80254298/9C` identify AObj
  `80243ABC` and FObj `8024873C`. Each enters original
  `80257468` to clear, size and link a 44-byte pool record:
  `803F8D90`/28 and `803F8DE8`/48, both alignment 4. The
  fixture declares the missing virgin BSS (the FObj record
  tail and list-head word `804369F0`) and reuses the parent's
  already mapped zero bytes. The two initializer calls run
  on a separate scratch frame in source order; then the
  suspended frame resumes. No other SDK startup, slab,
  free-list pointer, live count or returned cell is supplied;
  this is a scoped configuration, not a retail startup lease.
- Original `802572F0 → 802570A0` must grow each empty list
  by one cell via `8025AEB8 → 80253F94 → 801FFD38` and pop
  the cell it linked. This uses the same selected heap handle
  0 and existing 864-byte free block. The expected four
  payloads are `006000C0`, `00600100`, `00600160`, and
  `006001C0`, leaving 512 free bytes and preserving archive
  payload `00600040`. These are static arithmetic checks,
  not observed returns. Cell write access is opened only
  after source return and heap-link readback. The only new
  evaluator adapters are exact byte/word update-stores at
  the three pinned clear instructions; none returns a service
  result.
- Original `80243D88 → 8024A208` must build AObj descriptor
  `1048FD88` and the three authored FObjs rooted at
  `1048FD4C` (channels 1/2/3). Only after their data/link
  readback may `8024DC54` publish the AObj to `JObj+0x7C`
  on style-0 key 2 `02001800`. This is still the same
  inherited loader JObj, with the resource generation,
  style table and null old AObj checked. No JObj method,
  completed pose, callback result or replacement allocation
  is provided. **Stop at `8024DC58`**, before the remaining
  attachment tail (FObj scan, RObj/flags, child/model paths)
  and before sampling.

Release ownership was audited statically, not exercised:
`80243EB0` drops the FObj chain through
`80248794 → 8024A48C → 80257440` and returns the AObj via
`80243F8C → 80257440`. The latter is a same-pool push and
counter update, not a backing-heap free. A future detach
must start with the installed owner and live chain (plus
the actual post-attach path), check `8024D520/524/528` removes
and clears this JObj's pointer, and account for exactly one
push per live object before any reuse or backing retirement.
The old-track and external-object paths are not substituted.

Static syntax, source and parent pins, exact FObj chain and
heap arithmetic passed. Per initializer: 160 sites, 174
edges, five return edges, 38 access-width sites. Attachment:
392 sites, 405 allowed edges, 23 return edges and 182
access-width sites. Scoped release audit: 115 sites, 129
edges, ten return edges and 46 access-width sites, excluded
from the executable slice. Fixture SHA-256:
`2899894d26dc9c232b95385fea476e3646f5eedf0e90a3c44bb1214a8bcbfaa3`.

The one reserved run registered the source pool list
`803F8DE8 → 803F8D90` with strides 48 and 28. Original code
grew and popped one 28-byte AObj cell at `006000C0`, giving
the AObj pool one live cell and leaving 800 free bytes in the
same guest heap. The AObj constructor ran its clear and
default stores. On return, the non-null AObj took the
`80243DB4 → 80243DB8` fallthrough. The fixture's edge guard
had excluded that arm and stopped before executing
`80243DB8`. No FObj allocation, `JObj+0x7C` pointer store or
readback, or planned `8024DC58` stop was reached. No free-list
entries, allocation responses, binding or pose were supplied.

Exclusive receipt `retail-0600-rifle-boot-track-attach-v1.json`,
SHA-256 `2f69b243b8cc0eea04f0db7990a1b7d73d9f8d23ec1ff6a4ca94a0defb90b0ee`.
One isolated process, 11.455 s elapsed, 5,722,916 KiB available
immediately before launch, 236,100 KiB peak RSS, 1 GiB soft/hard
AS, 54 s internal and 58 s outer timeouts. All four pinned
parents were unchanged. No retry, successor, compile,
engine/package change, constructor return, appearance, input
or admission.

#### Machine Red part-2 SDK track attachment v2 — stopped at AObj epilogue

V1's refusal table had the polarity wrong at `80243DB4`
(`41820014`, `beq 80243DC8`). The v1 receipt confirms the
pop returned `006000C0` and reached `80243DAC → 80243DB0 →
80243DB4`. `mr. r31,r3` sets CR0 from the returned pointer;
the intervening `lwz r0,0(r29)` loads authored flags and does
not change CR0. The non-null pointer must fall through to
`80243DB8`. Original code reads the fresh AObj flags there,
ORs in the descriptor's masked flags (zero for this part) and
stores the result at `80243DC4`. V2 admits those four
fallthrough-path instructions and refuses the null arm
instead. No source instruction or evaluator result changed.

`build/borg-ordered-audit/retail-0600-rifle-boot-track-attach-v2.py`
pins the v1 script and refusal receipt and reuses the same
BSS, heap, descriptor, JObj and address-space limits. It
narrows the replay guard to the statically audited success
edges and compares the chronological trace. Static coverage
includes original pool registration, default heap split/return
and one-cell link/pop on each request, the exact three FObj
constructor return sites (`8024A228/23C/250`), the three
word/halfword/byte field copies and eight-byte stack spills,
the AObj chain store with no external object, and the
`80243EAC → 8024DC54` return to the sole `JObj+0x7C` store.
The branch ledger refuses seeded free-cell use, failed
allocation, a fourth FObj, stale tracks and post-store activity.

The operand pass caught a second v1 blocker: the epilogue
loads at `80243E94..80243EA0` from the original prologue's
saves at `r1+36/+28/+24/+20`. At that cut, `r1` is the
suspended allocator inlet SP plus `0x10`, so these reads
are above v1's general stack limit of inlet SP `+0x20`.
The inlet receipt shows the matching source spills at
`80243D90..80243D9C`. V2 opens only these four mapped
four-byte reads for the epilogue and disables all writes
while it runs. It neither widens the general stack window
nor supplies restored values.

Static preflight: 396 attachment instruction sites, 409
graph edges, 23 graph return edges, 184 access-width sites.
The ordered success path has 1,132 instruction visits and
470 mapped memory accesses. Syntax, source/parent pins,
exact authored clip chain and heap arithmetic passed.
Fixture SHA-256:
`8f6c297f91d2804c059c1a8be5f99caebbeb730364c9870757e2c889c8d00868`.

The one reserved v2 run registered both pools, passed the v1
fallthrough, and observed all four source allocations and pops:
one 28-byte AObj at `006000C0` and three 48-byte FObjs at
`00600100/00600160/006001C0`. The final heap readback verified
the 512-byte free block at `00600200`. Original code reached
the AObj chain store at `80243E04` and stopped at `80243E94`,
where the first four-byte stack read at `002FFEF4` was refused.
The v2 epilogue hook was keyed to a drive call starting at
`8024A44C`; a clear adapter split that call, so the resumed
segment ran without the epilogue hook. No JObj pointer store
or readback ran; `8024DC58` was not reached.

Exclusive receipt `retail-0600-rifle-boot-track-attach-v2.json`,
SHA-256 `be130970c64f9f27b670e6106eeb111b520330d9a523c667d267fd122b91c341`.
One isolated process, 11.828 s elapsed, 5,701,244 KiB available
immediately before launch, 236,260 KiB peak RSS, 1 GiB soft/hard
AS, 54 s internal and 58 s outer timeouts. All six pinned
parents were unchanged. No retry, successor, compile,
engine/package change, pose, release, constructor return,
input or admission.

#### Machine Red part-2 SDK track attachment v3 — passed at pointer publication

`build/borg-ordered-audit/retail-0600-rifle-boot-track-attach-v3.py`
preserves and pins the previous scripts and receipts. It uses the
same source, inputs, fresh receipt mode and runtime limits. From
the last FObj pop, v1's drive crosses five clear adapters: one
at `800054B8` and four at `800054CC`. Each resumes at the next
instruction. The last resume starts at `800054D0`, not
`8024A44C`; v2 only armed its hook on the latter. V3 arms the
`80243E94` fence on **every** segment of this drive and checks the
six ordered start/stop pairs and instruction sequences.

The read-only epilogue checks the observed-derived inlet SP
`002FFEC0` and epilogue SP `002FFED0`. Only these four
PC/address pairs are permitted, in order and four bytes each:
`80243E94/002FFEF4`, `80243E98/002FFEEC`, `80243E9C/002FFEE8`,
`80243EA0/002FFEE4`. The original saves supply the bytes;
there is no added stack mapping, restored value or write in
this phase. V3 checks `80243EAC → 8024DC54`, closes that
read window, and leaves the original single-word store guard
unchanged.

Static preflight checked the ordered tail against v2's observed
trace through the failed load, six helper return edges, 87
memory visits from the last pop including the store, and 43
pre-store writes limited to the existing stack and live track
cells. The JObj is writable only for the final four-byte
publication at `0200187C`. On failure, the receipt can record
segment start/end, last source PC, SP/LR and any rejected
epilogue access. Fixture SHA-256:
`c8783c89e8789f33d3018493d96754ddc6b71211856b5bd7e13660b8c63f5368`.

The one reserved v3 run reproduced both pool registrations,
the AObj pop at `006000C0` (28 bytes), and the three FObj pops
at `00600100/00600160/006001C0` (48 bytes each). The heap
retained a 512-byte free block at `00600200`, and the archive
payload remained `00600040`. The epilogue performed all four
original saved-word reads in order, with no writes, and
returned `80243EAC → 8024DC54`. Original `8024DC54` wrote
and the fixture read back `0200187C = 006000C0`, then
stopped at `8024DC58` before the rest of SDK attachment.
No allocation result, initialized binding, restored word
or pose was supplied.

Exclusive receipt `retail-0600-rifle-boot-track-attach-v3.json`,
SHA-256 `9d00131cb32e3afa0f9e536fc939417debcfb005b31e387e1d9a9afe89d82d40`.
One isolated process, 11.872 s elapsed, 5,716,728 KiB available
immediately before launch, 235,740 KiB peak RSS, 1 GiB
soft/hard AS, 54 s internal and 58 s outer timeouts. All
eight pinned parents were unchanged. No retry, successor,
compile, engine/package changes, later attachment, sampling,
pose, release or admission.

#### Machine Red boot after part-2 publication — static audit, no fixture

The passing v3 cut at `8024DC58` is a pointer publication,
not a return from `8024DC04` or a sampled joint. Preserve
the same style-0 key-2 JObj `02001800`, active record
`10490F4C`, row-2 cell `00132000`, archive payload
`00600040`, and source AObj/FObj chain. The v3 receipt is
comparison evidence, not an image to reload.

| Phase | Exact dependency and expected arm for key 2 |
|---|---|
| SDK attachment tail | `8024DC58..8024DCA8` walks the three channels `1,2,3`, so the type-12 splice should not write. `8024DCB4 → 802578C0` takes the null RObj arm only if the current `JObj+0x80` and record `+0x0C` agree. Both nulls have authored support, not just fixture zeros: exact AFS 575 role-4 key-2 JObjDesc (`0x13A88` body-relative) has null RObj and DObj operands, and role-3 clip `0:9` key 2 has a non-relocated null RObj word. The record flags are zero, so `8024DCDC → 80250650` clears bit 8. Key 2's authored flags are `10000000`; under the unchanged scoped loader flags, `flags & 4020` is zero and `8024DD20 → 8023D838` is not selected. Verify the live links and flags; no stub return or assumed null for another joint. The SDK returns to `8004AFC8`. |
| First sample | `8004B2F0` builds an authored-channel mask from the current FObjs. The rotation channels are present; missing scale channels take the default/dirty path, potentially walking this JObj's current children at `80251B80`. `8004B6EC → 8024D838 → 80243B90 → 802488B8` requests the lane's current frame and resets the FObj cursors; `8004B6F8 → 80243FC0` writes the actor's rate, not a tick. Use the reconstructed lane frame/rate, not the outer `-1` boot request sentinel or a seeded frame: `8004C37C..384` substitutes lane time plus actor `+0x1DC8`. `8004B700 → 8024EA64`, `8024EB84` loads the current class `+0x50` callback; `8024EB88 → 80243BD0 → 8024A198 → 80249AC4` is the actual sample path. The source channel table selects `8024E36C/8024E42C/8024E4C0` for `1/2/3`. The null RObj request/sample leaves are `802577D0` and `80257678`; do not replace them with unconditional returns. |
| Actor-local publication | `8004B768` returns to `8004AD30`; `8004AD6C → 8004B76C` copies current JObj T and converts its Euler values at `8004B864 → 8025ACFC` into the actor-local quaternion. It also reads the AObj's current/end fields for the lane. Qualify any later `8004ADB0` scale arm from current lane fields. The complete per-part return is `8004ACD8 → 8004A804`, not merely return from the SDK sampler. No world matrix or appearance is implied. |

The older loaded-rig path can identify the typed local JObj
fields and uses original `802524E8` to publish the class
slots, but it supplies `8025C984` registry completion and the
descriptor-to-JObj tree. The ground-init receipt records both
seams. The earlier G-Red/Sasuke channel proof covers source
stores on its own fixture class/instances, not Red's live
registry or tree teardown. The loaded-rig wrapper also
supplies `8025ACFC` with host Euler-to-quaternion conversion;
the older frame helper supplies host sin/cos for
`8023C480/8023C9E8`. None establishes the exact Red sample
through a source-produced actor-local quaternion. Do not
replace those gaps with a channel value, class table, completed
quaternion, or foreign JObj. Keep the scoped-loader origin
explicit even if its necessary slots and links can be checked.

**Independent boot budget blocker.** `8004A804..810` advances
the same lane-0 part loop and uses the authored cuts and order:
`[0,11)` selects keys `2..12`. The exact records for all 11
have one AObj and three FObjs. With the observed pool policy
(one-cell growth; no releases in this fresh attach loop),
another such part needs `64 + 3×96 = 352` OS-heap bytes
including headers. The current 512-byte remainder can hold
key 3, leaving 160. At key 4 the first FObj request would
take the whole remaining 96-byte block rather than the
qualified split path; the next would have no free head.
These are static capacity bounds, not observed requests.
Reusing the v3 heap and dropping tracks cannot prove a boot
return. A larger, separately reviewed arena must be declared
and created in the earlier heap frame, not grown or filled in
after v3. After lane 0 returns through `800487D0/800486EC`
to `8004C3A8`, the other mask-15 lane visits and opcode tail
still have to run before `8004C01C → 80054614`. Constructor
return at `80054758` is later still.

**Next useful witness:** finish key 2 on the same scoped JObj,
with no new allocations. First pin its post-attach flag and
child-walk branches, current class callback/slot, and the
authored FObj bytes and reset/sample path under bounded reads.
Then qualify the original conversion (`8025ACFC` and its
reachable scalar leaves) for the sampled Euler domain before
including `8004ACD8 → 8004A804` and actor-local readback.
No runnable follow-up was prepared: the existing evidence
does not cover that complete part without supplied pose math
or unqualified dispatch. Static checks matched 16 source
call edges, the exact class channels, role-3/4 records,
part order and resource arithmetic, plus v3 and all eight
parent hashes. No replay, compile, engine/package change,
input or admission.

#### Machine Red key-2 reset/sample/transform — owner gate

The [scoped joint-clock replay](retail-code-atlas.md#contact-rates-through-an-original-joint-sample)
now verifies original attachment, sampling, and local translation/quaternion
publication for this exact track on an independently constructed key-2
instance. It clears the copied descriptor's child and sibling links; it
does not complete the retained full-rig boot path audited below.

No runnable continuation was prepared. V3 remains valid for
the pointer publication on the scoped JObj. It does not
establish the initialization history needed to extend that
same pointer to a source-owned sample and child update.

- **Callback identity is known, instance ownership is not.**
  `8025258C` publishes `803ACC10+0x50 = 8024E120`; the
  sampler loads that slot at `8024EB84`. The authored
  channel table at `803ACCB8` routes `1/2/3` to
  `8024E36C/8024E42C/8024E4C0`. The standalone BRIF v3
  witness is evidence for the default-class registration and
  these slots, not for the current Red instance. In Red's
  ground-init receipt the registry call `8025251C → 8025C984`
  and tree factory `800070A4 → 8024F834` are supplied seams.
- **The source loader and the scoped loader do not establish
  the same initial state.** `80251EF0` initializes
  `JObj+0x14` with `0x40`; `8024F674` ORs in descriptor
  flags, and `8024F768` installs the descriptor backpointer
  at `+0x84`. The scoped loader copies descriptor flags
  directly, fills an identity matrix and constructs the link
  graph itself; it does not establish those source writes.
  This matters before conversion: absent scale channels and
  the Euler callback can enter `80251B80` depending on the
  current flags. Neither setting `0x40` now nor assuming it
  is clear proves the intervening lifetime. `+0x84` is
  useful ownership evidence, not a claimed sampler read.
- **Authored child links are pinned separately.** For normal
  AFS 575 role 4, key-2 JObjDesc at body offset `0x13A88`
  has flags `0x10000000`, parent key 1, first child 3,
  sibling 13 and authored null DObj/RObj. Starting at the
  child, the bounded descendant order is
  `3,4,5,6,7,8,9,10,11,12,20,21,22,23`; key 13 is
  key 2's sibling, not its descendant. This pins what to
  check, not a live child traversal. Do not replace child
  backing with the role-4 offsets, zero descendant flags
  or supply a dirty callback.

The exact key-2 FObj streams in role 3 are `(channel,
body offset, bytes) = (1,0xEC,29), (2,0x10C,45),
(3,0x13C,45)`. An offline decode of the first two calls
selects spline ops and suggests a small-angle conversion
domain; neither the decode nor the reconstructed boot frame
may supply the source sample. `8025ACFC` multiplies the
sampled Euler components by the DOL word `3F000000` at
`8043D498`, then calls `8023C480`/`8023C9E8` three times
each. For half-angles whose sign-cleared high word is at
most `3FE921FB`, these call `8023B044`/`8023BF8C`;
otherwise the finite range-reduction arm goes through
`8023ACA4`. Refuse the wider arm until independently
qualified. Use original bounded spills and coefficient reads
(`8043D030..8043D078`, `8043D0B8..8043D0F0`, and
the two zero doubles at `8043D150`/`8043D198`) and preserve
the single/double fused arithmetic where reached. Host
sin/cos or precomputed quaternion output is not source math.

| Pinned source range, end exclusive | SHA-256 |
|---|---|
| `8025ACFC..8025AE44` | `bd4ffdacfc871236f778f38d80b703de8faae90f86d4fa215edee3da470cc368` |
| `8023C480..8023C554` | `7191f0b4d2e319d285c8db0eb3c663aaef119792a2e0a66390ed24c441f18e0f` |
| `8023C9E8..8023CAC0` | `f5cca675e2bbd4c6f03e357e1551f901741db5fa9354b1a9d3e86b62fb65f2d7` |
| `8023B044..8023B138` | `e291a438fc827ef22d258b8b22405923970c977efcbfcad6171577ab95b2e6cd` |
| `8023BF8C..8023C02C` | `74d1575c2f44b98288faef0858309e3e440520c568d670b0ad1b946df7454f44` |

**Independent SDK owner prerequisite:** start with the exact
normal Red role-4 record-0 wrapper and relocated archive,
and follow original default-class registration, JObj
construction and descriptor load at `8024F834 → 802509C8 →
8025D088` / class `+0x3C → 8024F638`. Observe the returned
root and source publication of the style-0 logical table.
Account for source-produced initial flags and link maintenance
through the key-2 pre-sample boundary. Read back on one
generation the callback slot, key-2 descriptor/class identity,
parent, first child, descendant sibling/parent links, flags
that drive dirty propagation, and key-2 DObj/RObj state.
Require stable archive and instance ownership through the
visit; don't need draw or hardware pose equivalence. Compare
this evidence against the scoped path before extending it;
do not transplant a separately built tree, method table or
live-state snapshot into v3. A genuine-owner reconstruction
would start before the loader seam, not after v3's store.

If those predicates are qualified, the target is the complete
key-2 reset/sample/local T/Q visit, `8004ACD8 → 8004A804`
before the next key, on the same AObj/FObj cells and with
no new allocations. Math/decoder outputs remain expected
until a source witness runs. All v3 parent pins matched;
no replay, compile, heap change or engine/package change.

#### Machine Red independent JObj default/merge — passed

The exact BRIF 16/17 witness runs original default-class
registration and method publication, but uses raw object
backing in place of `8024F834`. The HSD size-class v3
witness executes `8025D088`, initialization hooks and true
returns for light, position and fog instances; its typed
slab/index/record backing does not qualify the 136-byte JObj
class (rounded bucket 4, 160 bytes) or any Red descendants.
Neither supplies a Red style-0 resource generation.

Prepared one independent narrow fixture:
`build/borg-ordered-audit/retail-0600-jobj-default-merge-v1.py`.
It declares fresh 136-byte **raw unpublished** backing at
`00680000`, the exact unrelocated key-2 role-4 descriptor
bytes at `00680100` (only the four-byte flag operand is
read), and original cold Base/Object/JObj class images.
There is no allocation, publication of an instance or its
class pointer, actor, held v3 heap, or supplied initialized
object.

1. Run original `802524E8 → 8025C984`; the two original
   parent dispatches initialize Object and Base. Original
   `800054F4` copies both inherited 20-byte method tails;
   the sole update-store adapter handles pinned
   `80005510` one byte at a time. Read back parent and
   child class links, sizes, and JObj slots
   `+0x2C/+0x3C/+0x40/+0x50`, including the expected
   sample callback `8024E120`. No class result is supplied.
2. Invoke original `80251EB4` from the published `+0x2C`
   slot on the declared backing. Source must dispatch to
   the inherited parent initializer `8025CF48` and return;
   expect `+0x14=0x40` and the three unit-scale defaults.
3. Enter the published `+0x3C` loader `8024F638` with the
   exact key-2 descriptor. Original code must clear the
   three link fields and merge `0x10000000` into the
   current flags. **Stop at `8024F678`**, immediately after
   the merge store `8024F674`, before the descriptor tail.
   Expected `0x10000040` is a comparison with the scoped
   loader's descriptor-only `0x10000000`, not a live pose,
   complete JObj, returned loader, or pointer to transplant.

Full tree remains blocked on original default JObj
allocation/init and recursive descriptor ownership. Although
key 2 has null DObj/RObj, its first child key 3 has DObj
descriptor `0xBBF8`; the exact wrapped load can enter
`8023DAE4` and material/PObj dependencies. Do not drop that
branch to claim a style-0 tree. The fastest expanded owner
step is one source-allocated 136-byte JObj return with the
bucket-4 lifetimes qualified, then the exact record-0
constructor including child/display branches. Observe root
publication at `800070A8` and `80053848 → 80067E18`
writing the style-0 table from the **returned tree**;
`80053888` installs that table on the actor. Check key 2's
parent/child/sibling, flags and callback on that generation.
Prior symbolic `8005380C` witnesses already cover the
table consumer, not the tree provider. Neither child links
nor style-table publication is executed in this narrow cut.

Static syntax and pin checks passed: registration 162 sites,
165 edges, 64 access-width sites, six return edges; default
27/27/12/two; merge 16/16/11/zero. Exact role-4 descriptor
and v3 parent hashes verified. Fixture SHA-256:
`4090b51a558161975732c2efc1f851b125418487710d02978e85ea3e08e3635e`.

The sole reserved run registered the original hierarchy,
copied both inherited method tails (40 bytes total), and
read back JObj slots `+0x2C = 80251EB4`, `+0x3C = 8024F638`,
`+0x40 = 8024D224`, and `+0x50 = 8024E120`. Original
initialization wrote `+0x14 = 00000040` and unit scale on
all three axes. The loader prefix cleared the three link
fields, then merged the exact key-2 flags to `10000040`
at `8024F674`. It stopped at `8024F678`, before the
descriptor tail. This confirms the difference from the
scoped loader's `10000000` on separate raw backing; no
allocated instance, tree, style table, pose or v3 state
transfer was produced.

Exclusive receipt `retail-0600-jobj-default-merge-v1.json`,
SHA-256 `1ff302aa7cedefad890b8022be52ed7c5a2834aab701957dea7694c09f9ceec1`.
One isolated process, 0.120 s elapsed, 5,699,740 KiB
available immediately before launch, 30,204 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All pinned parents, including v3 ancestry, were unchanged.
No retry, successor, compile, heap expansion, engine/package
change, input or admission.

#### Machine Red allocated JObj key-2 prefix — reached stop, readback refused

`build/borg-ordered-audit/retail-0600-jobj-allocated-key2-v1.py`
is an independent generation. It rebuilds original cold class
registration, OS table/heap/handle and HSD dispatch from source.
The declared 32-byte-aligned arena is `00600000..00602000`
(8 KiB, two OS table slots, 24 MiB memory-size operand,
no override), not an expansion or reuse of v3. BSS covers
only the pinned OS/HSD fields plus default JObj selector and
cold SDK size-class table/count. The read-only model input
is the exact key-2 role-4 record; its non-null child/sibling
offsets are not guest pointers and are not consumed in this cut.
No pointer, bucket record, initialized flags, allocation
result or helper return is seeded.

The default factory path `8024F834 → 802509C8 → 8025D088`
must allocate and return before the same cell is passed to
class loader `8024F638`. Expected cold requests are 128 bytes
for the 32-entry index, 20 bytes each for buckets 4 and 26,
and 1,024 bytes for the slab. Source rounds the JObj to 160
bytes and links the 864-byte slab tail into bucket 26. OS
requests enter the retained default dispatch as `(size,4,1)`,
then split the original aligned OS heap, with header/link
checks before the fixture enables each returned payload.
Expected payloads: `00600040`, `006000E0`, `00600120`,
`00600160`; expected OS free head `00600560`, 6,816 bytes.
The original JObj constructor must return from `8025D14C`
through `802509F4` and the factory must resume at `8024F874`
with the source-carved `00600160`. Expected defaults are
flag `40` and unit scale. None of these are replay results.

The loader runs the genuine null DObj/RObj helper returns,
writes the key-2 flag merge, copies the authored local fields,
and lets `8020AEFC` initialize the **default matrix** (not a
sampled pose). **Stop at `8024F764 → 802596A4`** before the
descriptor registry, with the loader frame retained and
expected flags `10000040`. The next prerequisite is the
source descriptor hash table at `803FA55C` and its distinct
12-byte registry-cell pool `803FA530`
(`80259738 → 802572F0` on a miss). Neither registry state nor
its result may be substituted. Even after that registry write,
child recursion and key 3's non-null DObj/material/polygon
path remain unqualified; this is not a tree return or style-0
publication. No detach, reset or descriptor lifetime beyond
this stop is claimed.

Static source and pin checks passed: the factory cut covers
527 instruction sites, 545 edges, 25 call sites, 24 return
edges and 223 access-width sites, in addition to the pinned
cold registration and heap setup graphs. Exact instruction
adapters handle the inheritance byte copy, HSD dispatch
publication, clear update stores and one positive `addze`
rounding instruction; no adapter returns a pointer.
Fixture SHA-256:
`d7327efa006e44237208fb8b899fc4dbea12b6e20d5893fa08ef303b66ade8a3`.

The one reserved run observed all four OS requests and
payloads as planned, with 6,816 free bytes remaining. The
JObj constructor returned the source-carved `00600160`
through `8025D14C → 802509F4` to `8024F874`; default flag
`00000040` and unit scale were checked. The same cell went
through the key-2 loader, reached `8024F764 → 802596A4`
without entering the registry, and passed the same-cell
link/flag/DObj readback, including merged flag `10000040`.

The fixture then refused at its local-field readback
assertion (line 394), not at a source branch. The assertion
treated rotation, scale and translation as nine contiguous
words starting at `+0x1C`. The original stores skip
`+0x28`, place scale at `+0x2C/+0x30/+0x34`, and include
translation Z at `+0x40`, which that read omitted.
The default-matrix helper ran but its separate readback
and the final SDK bucket readbacks were not reached.

Exclusive receipt `retail-0600-jobj-allocated-key2-v1.json`,
SHA-256 `8e4b4382828e8f25f0317257f56d982c726423c7fbf0b0d84dd6c74bd96d3c74`.
One isolated process, 0.170 s elapsed, 5,697,768 KiB
available immediately before launch, 30,576 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All pinned parents were unchanged. No retry, successor,
compile, v3 transfer, engine/package change, pose,
grounded B or admission.

#### Machine Red allocated key-2 descriptor registry v2 — stopped before hash multiply

`build/borg-ordered-audit/retail-0600-jobj-allocated-key2-v2.py`
pins the v1 script and refusal receipt. It rebuilds the
same standalone heap, class, returned cell and suspended
loader frame, with a single corrected host readback:
rotation `+0x1C/+0x20/+0x24`, scale `+0x2C/+0x30/+0x34`,
translation `+0x38/+0x3C/+0x40`. It leaves `+0x28` out
of the readback and does not write it. The default 3×4
matrix is checked against the six source GQR0 paired stores
at `8020AF04..20`, not a pose. The unchanged bucket checks
read the original 32-slot directory, bucket 4 (160-byte
live object, bucket-26 link), and bucket 26 (864-byte
free tail, null next). Neither readback ran to completion
in v1. No v3 object or heap is involved.

The continuation declares only virgin registry storage:
44-byte pool record `803FA530`, 404-byte default hash
table `803FA55C`, and the pool-list head `804369F0`.
Original `80259644` initializes the 12-byte/alignment-4
pool; original `80259674` clears the 101 hash slots. Their
source call sites are `802542A0` and `80253B98` respectively;
the fixture calls just the two leaves on bounded scratch
stack and restores the suspended loader frame. It does not
claim all SDK startup or an in-game reset. The only new
arithmetic adapter is exact unsigned `mulhwu` at
`802596DC`; the rest of the hash and selection is source.

Original `802596A4` must miss slot 15 for the declared key
`00680100`, then `80259738 → 802572F0` must grow and pop
one cell from the **same** OS heap. Expected cell `00600580`,
64-byte OS block inclusive of header, and 6,752-byte free
remainder. Source must write `(next=0, descriptor=00680100,
JObj=00600160)`, publish the cell at bucket `803FA598`,
return `8025978C → 8024F768` and write the same JObj's
descriptor backpointer at `+0x84`. **Stop at `8024F778`**
before the child descriptor load. No registry entry,
allocation response, repaired flags or child pointer is
supplied. Neither tree return nor display/pose is in scope.

Static syntax, parent/source pins, exact descriptor and
readback operands passed. Additional graph checks: pool
160 sites/174 edges/38 access widths, hash clear 56/60/17,
registry through stop 248/251/96 with 12 return edges.
Fixture SHA-256:
`73f9892a3cf7a08b174deb70f4792d4d4a0929dc56dba554cf9b2ed76839d3fb`.

The sole reserved run passed the corrected spaced readback:
zero rotation and translation, unit scale, identity default
matrix, merged flags `10000040`, and the bucket-4 and
bucket-26 checks on the source-allocated `00600160`.
Original code initialized the 12-byte/alignment-4 pool and
cleared all 101 hash slots. At the `802596DC` multiply
boundary the fixture refused on line 186: it expected
`44708657`, but the preceding `lis 0x4470; addi -31145`
decodes to `446F8657`. No multiply, hash lookup, registry
cell allocation or publication, JObj backpointer, or child
read ran. The observed heap remainder is still 6,816 bytes.
The planned stop at `8024F778` was not reached.

Exclusive receipt `retail-0600-jobj-allocated-key2-v2.json`,
SHA-256 `56207650de0e5db32c0dc514aaab8c130448d6b278740f4af680008a9d25f430`.
One isolated process, 0.208 s elapsed, 5,674,216 KiB
available immediately before launch, 30,636 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
Prior pins were unchanged. No retry, successor, compile,
engine/package edits, pose or gameplay admission.

#### Machine Red allocated key-2 descriptor registry v3 — passed

`build/borg-ordered-audit/retail-0600-jobj-allocated-key2-v3.py`
pins both the v2 script and its refusal receipt. It rebuilds
the independent allocated JObj and rechecks the same flags,
local-field/default-matrix readbacks, heap and virgin pool
initializers. It does not load a receipt as state or borrow
the earlier rifle-boot v3's scoped JObj.

The pinned instructions `3C604470 38038657` compute
`0x44700000 + sign_extend(0x8657) = 0x446F8657`. For
descriptor `00680100`, the exact `mulhwu` at `802596DC`
produces high word `001BCD93`; original remaining
instructions give difference `004C336D`, half `002619B6`,
mixed value `0041E749`, quotient `0001079D`, product
`006800F1`, and remainder `0000000F`. The bucket offset
is `0x3C`, address `803FA598`. The fixture records the
live descriptor, multiplier, table and unsigned high word
before checking them, then records each source intermediate.
The instruction adapter implements only the high multiply;
no hash index, hash entry, or allocation result is supplied.

The independent ordered audit covers 281 instruction visits,
107 memory visits, 11 helper returns and 128 bytes of maximum
stack depth after the registry inlet. Runtime gates separate
hash selection, raw OS split, pool link/pop, cell clear and
publication, and the final JObj write. They check the
12-byte request `(12,4,1)` on heap handle 0; the expected
returned payload is `00600580` from the 64-byte split at
`00600560`, leaving 6,752 bytes at `006005A0`. Only that
post-return cell may be linked, popped and then cleared.
The expected source writes are descriptor and JObj keys,
null next, one bucket publication, and finally
`006001E4 = 00680100` at `8024F768` after registry return.
Only that backpointer word is writable in the final segment.
**Stop at `8024F778`**, before executing the first child
descriptor read. No recursion, loader/tree return or pose.

Syntax, source and prior receipt pins passed static checks.
Fixture SHA-256:
`fd98584a4ea412ef0a76e790f7c435afb68ac19df4acd57e89f9f39fd8343784`.

The sole reserved run cleared all 101 hash slots and
initialized the original 12-byte, alignment-4 pool. The
live multiplier `446F8657`, high word `001BCD93`, quotient
`0001079D`, remainder 15 and bucket `803FA598` all matched.
Original code grew and popped cell `00600580`, leaving
6,752 free bytes at `006005A0`, then published the cell with
`(next=0, descriptor=00680100, JObj=00600160)`. The registry
returned to `8024F768`; the source store and readback
confirmed `006001E4 = 00680100`. It stopped at `8024F778`
before the first child descriptor read.

Exclusive receipt `retail-0600-jobj-allocated-key2-v3.json`,
SHA-256 `3ca6ce062d2f141bfe9694138f9269f422f3429926edc21932382f5de2bb4e0f`.
One isolated process, 0.214 s elapsed, 5,660,248 KiB
available immediately before launch, 30,636 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All nine direct parent pins and earlier receipt pins
matched. No retry, successor, compile, engine/package
change, rifle-boot v3 state transfer, pose or admission.

#### Machine Red first child JObj to DObj inlet — stopped at allocator prologue

`build/borg-ordered-audit/retail-0600-jobj-first-child-v1.py`
rebuilds the allocated-key2-v3 receipt, not the rifle-boot
rig. Exact normal AFS 575 role-4 wrapper/model hashes,
32 joint nodes and wrapper links, 638 distinct in-range
relocations, and zero externs pass static review. The
fixture declares two bounded descriptor views on one
address basis: key 2 remains at `00680100`, key 3 is at
`00680140`. Their explicit pointer operands are derived
from the authored relocation entries; the source does
not execute archive relocation in this witness. No other
descriptor (including the DObj) is mapped for execution.
Key 2's first child is key 3; key 3's first child is key 4
and its sibling is key 22. Parent key 2's own sibling is
key 13, not part of this child allocation.

`8024F778` reads the key-2 child pointer and the original
default factory at `8024F7A8` obtains key 3. The existing
8 KiB arena is sufficient: SDK bucket 26's 864-byte free
tail is carved into a 160-byte key-3 cell at `00600200`
and a 704-byte tail in bucket 21. Only a 20-byte bucket-21
record needs a new OS allocation (`006005C0`, 64 bytes
including the header); expected OS remainder is 6,688 bytes
at `006005E0`. No arena growth, seeded cell or allocator
result. The exact positive-rounding and clear instruction
adapters preserve the original helper returns. Source
default initialization and publication of the class pointer
must precede the virtual loader call with the original key-2
JObj as parent operand. Expected flags after the child merge:
`100501C0`.

**Stop:** `8024F6C8 → 8023DAE4` with first non-null DObj
descriptor `00678270` (body offset `0xBBF8`), before the
DObj constructor executes. Key 2's live child link remains
null. The original child loader has to return through
`8024F7EC` before `8024F804 → 8024FF64` attaches the
child; the fixture does not skip that return or substitute
display results.

| Key-3 draw order | DObjDesc | MObjDesc / mode | PObjDesc / authored flags |
|---|---|---|---|
| 0 | `BBF8` | `AAD4 / 201D` | `BBE0 / 80000025` |
| 1 | `BF98` | `AA34 / 2015` | `BF80 / 8000001B` |
| 2 | `C1B8` | `A994 / 201D` | `C1A0 / 8000000F` |
| 3 | `C218` | `A8F4 / 2001` | `C200 / 80000001` |

First DObj dependency is the default/override selection
`8023DBF0` (`r13−19704`), DObj class `803ABAE8`,
24-byte instance/bucket 0, and original `+0x40` loader
`8023D9D4`. `8023D9F4` recurses into the next DObj
**before** `8023DA00` calls MObj and `8023DA0C` calls
PObj; the last DObj's `A8F4` material is therefore the
first material demanded, not `AAD4`. The latter has a
nonnull TObj (`AA64`, image descriptor `AA4C`) and material
record `AAC0`; the first PObj has vertex layout `B6C4` and
borrowed display bytes at `B740`. Full construction needs
the same relocated archive lease, lifetime for all four
DObjs, and the exact material selector/context. The retail
wrapper selects MObj subclass `802DA7F0`; neither a default
MObj substitute nor a BRIF JObj/TObj result qualifies that
context. Default/derived MObj sizes 32/36, PObj 28, TObj
172 and the material/cache allocations need original class
registration, typed allocation and actual returns.

Practical next reusable witness: qualify original DObj
class/bucket-0 construction then a bounded tail-to-head
display/material chain, starting at the terminal DObj's
authored untextured material. Reuse the HSD allocator
arithmetic, BRIF class registration and wrapped material
provider contracts only for their checked inputs. The
`80242A74 → 80243468` material cache must really return
before consuming MObj or PObj links. No whole tree or
style table is qualified by an inlet.

Static syntax, source/parent pins, descriptor graph and
warm split arithmetic passed. Child graph: 408 sites,
420 edges, 16 return edges and 166 access-width sites.
Fixture SHA-256:
`257e4f8a1a5d4d1a8cd1a0a60009715cc57941a70554d29e9d43f08824432123`.

The sole reserved run reproduced key-2 registration and
the descriptor backpointer on `00600160`. Source read the
authored key-3 link and called `8024F7A8 → 802509C8`.
The fixture refused the original four-byte LR save at
`802509D0` to `003000D4`: that address is the exclusive
upper bound of the configured stack window. No child
allocation, flag merge, link publication or non-null DObj
inlet was reached. The last verified heap remainder was
6,752 bytes after key-2 registry allocation.

Exclusive receipt `retail-0600-jobj-first-child-v1.json`,
SHA-256 `635037cd0c19c6527f94586e9a3dcd0e398c2bdde0713392c804d062dad001fa`.
One isolated process, 0.221 s elapsed, 5,702,088 KiB
available immediately before launch, 30,348 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
Prior pins matched. No retry, successor, compile, pose,
engine/package changes, rifle-boot transplant, style-table
publication or admission.

#### Machine Red first child exact ABI frames v2 — passed

`build/borg-ordered-audit/retail-0600-jobj-first-child-v2.py`
pins the v1 script and refusal receipt and preserves the
same source-owned key-2 → key-3 path. The root factory's
16-byte frame and key-2 descriptor loader's 32-byte frame
leave the suspended parent at SP `003000D0`. Original
`8024F7A8` calls the child factory with that SP, and
`802509C8` lowers it to `003000C0`. `802509D0` saves LR at
`003000C0 + 0x14 = 003000D4`; `80250A0C` reads the same
word back. Caller LR for this call is `8024F7AC`. The
child descriptor loader later starts at `003000B0` and
`8024F640` reuses the same word at `+0x24` for `8024F7EC`.
The final loader return remains suspended at the stop.

There is no wider writable stack span. The corrected
runner replaces the v1 span check for every stack
instruction with an ordered PC/SP/address/width/direction
ledger. The three accesses above are the only permitted
accesses above the caller SP. Non-stack operations cannot
use the stack as a fallback. The ledger follows all
13 frames through the two size-class lookup calls, heap
dispatch and split, record and object clears, class
initializer, factory return and child loader prologue.
It checks 637 instruction visits, 81 stack accesses
(70 PC/SP rules), 16 helper returns, and lowest SP
`00300030`. The runtime records live and derived entry
SPs before checking them and logs each caller-word access
before validation. Restores use guest words originally
written by source, never supplied values.

**Same planned stop:** `8024F6C8 → 8023DAE4`, before
the first non-null DObj constructor. Child `00600200`,
merged flags `100501C0`, the 20-byte bucket record and
6,688-byte OS remainder are still expectations. The
parent's child link must remain null. The 8 KiB arena,
two authored descriptor views and all non-stack bounds
are unchanged. No supplied child pointer, flags, display
result or pose.

Static syntax, source/parent pins, ordered returns and
stack operands passed. Fixture SHA-256:
`66cff13a1cece1ae7f363698cca187a38bbf5f57c8d92997b0d680a73aeb9301`.

The sole reserved run matched all 81 stack accesses and 16
helper returns. Live SP was `003000D0` at resume and
`003000C0` for the factory LR save/readback at `003000D4`;
the child loader reused that word from SP `003000B0`.
Original size-class code carved child `00600200`,
allocated the 20-byte bucket record at `006005C0`, and
left 6,688 OS-heap bytes free. Original initialization
read back class `803ACC10`, default flags `00000040`,
and merged key-3 flags `100501C0`. The key-2 child
link remained null. The run stopped at `8024F6C8 →
8023DAE4` with DObj descriptor `00678270`; no DObj
constructor ran.

Exclusive receipt `retail-0600-jobj-first-child-v2.json`,
SHA-256 `8e225731c56bffcb85b9946580167d4f151caf0d1b49b61843df64e6f431a100`.
One isolated process, 0.263 s elapsed, 5,647,736 KiB
available immediately before launch, 30,732 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All prior pins matched. No retry, successor, compile,
engine/package changes, pose, rifle-boot transplant or
admission.

#### Machine Red key-3 display chain — static audit, no replay fixture

The passing first-child-v2 receipt stops at `8024F6C8 →
8023DAE4`, with child `00600200`, flags `100501C0`, a
null parent-child link, and 6,688 OS-heap bytes free.
`00678270` is the exact first DObj descriptor **operand**,
not a mapped descriptor or returned DObj. The witness
provides only the two declared JObj views: it has not
established a readable role-4 display lease, DObj selector
state or a registered DObj class on this generation.

**First unowned inlet.** The first descriptor use is
`8023DB08` (`lwz r3,0(r30)`). The cold normal record has
a genuinely null class-name field; all four 16-byte DObj
records can be pinned as read-only relocation-derived
views, but not as live DObjs. The next dependency is
`8023DB20 → 8023DBF0`, which reads the unmapped DObj
override word `804368A8` at `8023DC00`. An independent
default-only witness can explicitly declare a fresh
no-override input and run original cold class registration
`8023DE1C → 8025C984`, rather than seed `803ABAE8`'s
vtable. Then `8023DC18 → 8025D088` must really return
a 24-byte DObj, and `8023DB5C` must dispatch its actual
class `+0x40 = 8023D9D4`. The first-child receipt does
not qualify the combined default-class and bucket-0
allocation/recursive loader scope; no runnable follow-up
was prepared.

**Tail-first scope and frames.** The exact DObjDesc sequence
is body-relative `BBF8 → BF98 → C1B8 → C218 → null`.
Each factory allocates before calling its loader.
`8023D9F4` recurses before material or polygon loading;
the terminal null visit returns `0` through `8023DB78`
to the real `8023D9F8` sibling store. It is not an empty
display substitute. For completed calls, `8023DAE0 →
8023DB60 → 8023DB78` returns each DObj to its waiting
loader, then `8024F6CC` would install the head only
after the whole chain returns. The source child-link
publication at `8024F804` is later still.

| DObj | Entry SP | Factory SP | Loader SP | Factory LR / loader LR words |
|---|---|---|---|---|
| `BBF8` | `003000B0` | `003000A0` | `00300090` | `003000B4 / 003000A4` |
| `BF98` | `00300090` | `00300080` | `00300070` | `00300094 / 00300084` |
| `C1B8` | `00300070` | `00300060` | `00300050` | `00300074 / 00300064` |
| `C218` | `00300050` | `00300040` | `00300030` | `00300054 / 00300044` |

These are derived frame operands, not live trace results.
The terminal null prologue reaches `00300020`; the
deepest predicted DObj allocation helper frame through
default heap selection reaches `002FFFA0`. A future guard
must follow each PC/SP save/restore across recursion and
separate the terminal material/cache frame, not enlarge
the first-child stack range. For the existing cold
bucket-0 case the four 32-byte DObj slots can come from
the remaining 704-byte SDK tail. A source-owned split
would require five 20-byte OS requests (bucket 0 and
remainder buckets 20/19/18/17), at 64 bytes including
headers each. Expected OS remainder before the first
material is 6,368 bytes. This is a capacity calculation,
not a grant of those slots or a bound for the cache.

**Terminal material/cache.** `C218` references `A8F4`
(mode `2001`, no TObj/PE, material data `A8E0`) and
PObjDesc `C200` (flags `80000001`, vertex layout `B58C`,
borrowed stream `C1E0`). The vertex-layout relocation at
`B5A0` has target offset zero: it is the archive body
base, not a null pointer. After the actual null sibling
return, `8023DA00 → 802429FC` demands `A8F4`.
`80243850` selects the MObj class from `80436918`; the
retail wrapper normally selects `802DA7F0` and publishes
the context used by `8008D72C` and `8008D7EC`. That
wrapper-owned state was never produced by the independent
key-3 path. `802428EC` takes the true null-TObj arm,
allocates/copies the 20-byte material and ORs `0x1000`
into the mode. The resulting `3001` still selects
`80243508`'s texture-override lookup at `80436920`, so
authored no-texture does not authorize skipping cache or
assuming that global is null. `80243550` dispatches the
selected expression method and `8024355C → 802604F8`
must produce the actual cache before `802429FC` returns.
Only then can `8023DA0C → 8024A944` load and publish the
PObj with its real borrowed data.

**Reusable evidence, not results to lend.** BRIF v3
qualifies source default DObj class registration and
consumer slots on supplied model backing. HSD size-class
v3 covers source carve/return with scoped raw storage for
lights/position/fog, not this occupied DObj/registry heap.
ST42 side-material loader v2 reaches `80242A74` with
original wrapped MObj/TObj/material outputs for a
different textured record and raw storage service. It
stops before the cache; graph/drop observers are host
controls, not cache completion. The next useful bounded
display witness is the four source DObj allocations and
terminal null sibling return, stopping at
`8023DA00 → 802429FC` with the exact terminal MObjDesc.
Qualify the selector/BSS and warm bucket-0 schedule in that
same evaluator first. A separate exact untextured cache
witness must pin wrapper selector/context, texture override,
expression/cache allocation and ownership through the real
`80242A78` return. No DObj or material return should be
substituted to reach a later parent store.

Static-only checks matched the first-child-v2 receipt and
all thirteen direct parents, exact role-4 bytes and
relocations, source calls/returns, recursive stack
arithmetic and pre-material capacity. No new replay fixture
or receipt, replay, compile, engine/package edit, pose,
boot transplant, input or gameplay claim.

#### Machine Red key-3 terminal DObj material inlet — stopped during class registration

`build/borg-ordered-audit/retail-0600-jobj-terminal-dobj-v1.py`
rebuilds first-child-v2 on the same independent JObj and
heap. It maps only four exact, read-only, 16-byte authored
DObj descriptor views at the existing role-4 address basis.
All class-name, sibling, MObj and PObj operands are checked
against original relocation membership and bounds. The
terminal MObj/PObj bytes are checked statically but not
mapped. No DObj instance, live slot, or class method result
is declared. The only added guest RAM is the virgin
no-override selector `804368A8` and the 72-byte cold DObj
class image `803ABAE8` from the DOL.

Original `8023DC00` must use the declared no-override arm,
then `8025D088` must invoke and return from cold DObj
registration and allocate a 24-byte instance through
bucket 0. The fixture checks source publication of loader
`+0x40 = 8023D9D4` before each virtual loader call. The
four objects are allocated head first while earlier
loaders wait on their next-sibling calls. Warm splits
should return DObjs `006002A0/2C0/2E0/300` and original
20-byte records for buckets `0/20/19/18/17`. The five
OS splits should leave 6,368 bytes in the same arena.
These are checks on original returns, not injected
addresses. The null terminal sibling must return from
`8023DB78` to `8023D9F8`, which must itself write zero.

**Planned stop:** `8023DA00 → 802429FC` with the exact
terminal untextured MObj descriptor `00676F6C`, before
executing the material factory. No non-null DObj loader or
whole-chain return is claimed. Neither the DObj head nor
the key-2 child link is published. The cache, wrapper
selector/context and any later PObj work remain outside
scope as in the static audit above.

The ordered path audit covers 466 sites, 2,822 visits,
59 stack frames, 70 returns and 1,032 memory-access
visits. Lowest expected SP is `002FFFA0`. Runtime checks
the exact per-visit stack displacement, saved-word
readback, original branch successors and role-bounded
non-stack operands. Instruction adapters are limited to
positive `addze`, inheritance-copy byte store and byte
clear update stores; no function result is supplied.

Static syntax, source and prior pins, exact descriptor
views and ordered stack/path checks passed. Fixture
SHA-256 `3ab1d758bcf4f017995d48d3b75f6b3437817027813a84f11a0f81c626eadcaa`.

The sole reserved run rebuilt first-child-v2, retained
6,688 free bytes, and entered cold DObj registration for
the first descriptor `00678270`. The inherited 20-byte
method copy returned. The fixture then refused
`8025CA50`, the source store of the new class pointer
to the existing Base class's child-list head
(`803AD6B0+0x1C = 803AD6CC`). The guard treated Base as
read-only, so this store did not execute. Class
registration did not return; no DObj allocation, display
or child publication, null-sibling store, or material
inlet was reached.

Exclusive receipt `retail-0600-jobj-terminal-dobj-v1.json`,
SHA-256 `6ffec6a18d98d2a4dd96690386063e3979318eb012ce4df64d5dcad99e59578e`.
One isolated process, 0.351 s elapsed, 5,636,056 KiB
available immediately before launch, 31,892 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All fifteen direct pins and earlier ancestry matched.
No retry, successor, compile, engine/package edit,
boot transplant, pose or admission.

#### Machine Red terminal DObj class linkage v2 — passed

`build/borg-ordered-audit/retail-0600-jobj-terminal-dobj-v2.py`
pins v1 and its refusal receipt and leaves them unchanged.
The original class registration sequence is
`8025CA48` (load Base's previous first child),
`8025CA4C` (write it to the DObj class's next-sibling slot),
then `8025CA50` (`stw r30,0x1C(r31)`). The expected
live operands are `r30=803ABAE8` (DObj), `r31=803AD6B0`
(Base) and previous head `803AD548` (Object), with
DObj's parent and sibling already written by source.
V2 logs the live operands and old links before validation,
permits exactly this one four-byte Base write at
`803AD6CC`, and reads back the new head. It does not
widen the class mapping or initialize the head itself.
Final checks require Base → DObj → Object, both direct
parent pointers, and the unchanged Object → JObj link.

The ordered audit enumerates all 341 non-stack write
visits at 44 PCs, and all 18 indirect calls. It checks
actual address, width and value, including inherited
methods, class hooks/counts, five OS splits and bucket
record insertions, warm tail links, all four object
clears and class pointers, and the terminal null-sibling
store. It retains the v1 per-visit stack ledger, all 70
helper returns and the source default-class/loader
dispatches. The only change to Base access is the exact
head store; output values in the ledger are checked,
not passed to guest code.

**Same planned stop:** `8023DA00 → 802429FC` before
terminal material work. New Base head, DObjs
`006002A0/2C0/2E0/300`, bucket-0 records, null sibling
and 6,368-byte heap remainder are expected only.
MObj selector/context, texture override and cache
ownership still gate a real display return. No material
return, DObj head, child link, or pose is produced here.

Static syntax, exact descriptor views, source/parent
pins and write/dispatch ledger passed.
Fixture SHA-256:
`10ea5b4ea8640db52c79b3f602b71ae10a0c4375b052551f5fe3ce2cc3767257`.

The sole reserved run observed original `8025CA50`
change `803AD6CC` from Object `803AD548` to DObj
`803ABAE8`; final Base → DObj → Object and Object → JObj
links matched. The source returned four DObj allocations
at `006002A0/006002C0/006002E0/00600300`, with five
20-byte bucket records and 6,368 OS-heap bytes remaining.
The terminal null-sibling store and readback matched.
All 341 non-stack write visits and 18 indirect calls
matched the ordered ledger. Source reached `8023DA00 →
802429FC` with material descriptor `00676F6C`; neither
the DObj head nor the child link was published, and
material/cache code did not execute.

Exclusive receipt `retail-0600-jobj-terminal-dobj-v2.json`,
SHA-256 `ec84aab5c1d146e79c22e8077ac370c9a05aab0b3999c32a4e47e8faa1e00c94`.
One isolated process, 0.777 s elapsed, 5,651,292 KiB
available immediately before launch, 33,204 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All seventeen direct parent pins and earlier ancestry
matched. No retry, successor, compile, engine or package
edits, boot transplant, tree or gameplay admission.

#### Machine Red terminal untextured MObj — static owner gate

The passing terminal-DObj-v2 receipt stops at
`8023DA00 → 802429FC`, with the terminal DObj at
`00600300`, SP `00300030`, and material descriptor operand
`00676F6C`. The 8 KiB heap has 6,368 OS-free bytes, and
the four DObj cells are source-owned. No material or
cache code has run. **No runnable continuation was
prepared.** The first unowned read is `80242A18`, the
four-byte class-name read from `MObjDesc+0`; the current
fixture maps only four DObj descriptors, not `A8F4`.

The exact normal role-4 bytes establish a legitimate null
class name, TObj, render descriptor and PE, mode `2001`,
and one relocated data pointer (`A900 → A8E0`). A narrow
read-only view could cover the 24-byte descriptor
`00676F6C` and 20-byte data `00676F58` at the same address
basis. That does not establish the next owner dependency:
`80242A30 → 80243850` reads the MObj class selector
`80436918` at `80243860`. The independent JObj/DObj
fixture did not enter the retail model wrapper and has
neither that selector nor a wrapper context on this
generation. A zero selector specifically chooses
`803AC428` (32 bytes), not the wrapped class. Do not
declare it null merely because the authored name is null.

**Source path to qualify with the right wrapper context:**

1. `80041A8C → 80007030` obtains a context from the
   resource block at `+3112` (`80041A5C` writes `255`
   there and `80041A68` zeros the next word).
   `80007080 → 8024298C` selects `802DA7F0`;
   `80007094 → 8008D714` installs the context pointer at
   `804362B4`. Those calls precede `800070A4 → 8024F834`.
   The wrapper restores both after the real factory
   return at `800070B0/B8`. The local `modelDesc+24`
   fallback is not the demonstrated non-null Red caller
   input; stack scratch or a copied ST42 address is not a
   context. A future independent witness needs this
   source-owned lifetime before the factory, not an
   after-the-fact setter called to repair the stopped frame.
2. Original `80243A0C` registers the default MObj and
   `8008DA7C` registers its 36-byte subclass (84-byte
   class table). Rebuild both source links and methods,
   then observe `80243878 → 8025D088` and its real
   `8024387C` return, continuing to `80242A34`. The
   class `+0x40` loader call at `80242A6C` must reach
   `8008D72C`, which calls the base loader. The true
   null-TObj arm `8023E814` returns at `8023E8A8`;
   `802438AC → 8025CCD0` allocates the 20-byte
   material and `80242930` copies the exact source data.
   Authored null PE selects `80242968`. Source stores
   loaded mode `3001`; `80242988` returns into the
   subclass, which reads the current wrapper context
   at `8008D760` and writes `MObj+0x20` at `8008D764`.
   `8008D77C` returns to `80242A70`. This is a loader
   result, not a cache or DObj return.
3. `80242A74 → 80243468` consumes that same MObj. Mode
   `3001` avoids the `r13−19580` append arm but takes
   `80243508`, which reads texture override `80436920`
   (`r13−19584`). Authored null TObj does not prove a
   null override. The located zero store at `802439DC`
   belongs to a class-amnesia hook, not an initialization
   to invoke to make the input pass. A new isolated
   witness must explicitly qualify a cold no-override
   scope or carry the actual owner and its `+0x58` field.
   Under a checked null override, the original
   `80243534 → 802401B0` takes the null-list path.
   The `+0x44` call at `80243550` still reaches
   `8008D7EC`. Base expression `80242A9C` misses the
   textured branch, creates the 108-byte root, and uses
   `80242CDC` and `80242E2C` for the `data+4` and
   `data+12` constants. Its texture-list walk has no
   element; mode bit `0x4` is clear, so the extra
   expression arm at `80242FAC` is not selected.
   The subclass interns the **context address** at
   `8008D838`, allocates its own 108-byte expression at
   `8008D844`, and returns the actual root at `8008D8E4`.
4. `8024355C → 802604F8` consumes that root and writes
   the distinct `MObj+0x18` compiled-list and `+0x1C`
   graph slots. Its 1,504-byte frame alone places SP at
   `002FFA20` for the retained terminal stack (MObj
   frame at `00300020`, cache frame at `00300000`),
   before deeper helpers. Preserve the real expression-graph
   rewrites and reference counts, register assignment,
   120-byte per-stage allocations, and qualified
   `80260768/7C` graph retirement. The expected return
   route is `8026079C → 80243560`, then
   `80243584 → 80242A78`, then `80242A98 → 8023DA04`.
   Only then can source publish terminal `DObj+8` at
   `8023DA04`; stop at `8023DA0C` before polygon work.
   None of these returns or outputs has been observed
   for this Red descriptor.

With the stated wrapped/no-texture arm, assuming no
constant hit and no earlier collection, the pre-compiler
allocation projection from the 576-byte SDK tail is
`36,20,108,20,20,20,108` bytes (rounded
`64,32,128,32,32,32,128`). The expected new bucket records
are `1,15,14,3,10,9,8,7`, leaving 128 slab bytes and
5,856 OS-free bytes before the compiler. This is an
operand/capacity calculation, **not** a compiler bound:
the optimized stage count, cache and reclamation schedule
must be qualified on the retained graph. Do not transplant
the textured ST42 bucket order or grant fixed slot IDs.

**Practical next witness:** an independent wrapper-owned
load for the exact normal Red role-4 material, with
source-published selector/context and explicit cold
texture-override scope, through the real `80242A74`
inlet first. The ST42 loader-v2 witness qualifies original
wrapped class/loader methods and a material copy, but has
different authored texture data and raw-storage grants; it
does not run the cache. The recycler receipt qualifies
typed allocation/reuse for selected empty graphs, not
Red's live references and compiler output. Reuse their
guards and original paths, not their objects or results.

Static-only checks matched all seventeen direct parent
pins, exact terminal data/relocation, 22 call edges and
return/selector instructions. No replay, compile, engine
or package edit, head publication, pose, boot transplant,
input or admission.

#### Machine Red record-0 material wrapper prefix — passed

`build/borg-ordered-audit/retail-0600-material-wrapper-prefix-v1.py`
is a fresh **independent** source-wrapper witness. It uses
the Red normal role-4 record-0 pointer chain: `scene_data`
`2B66C → 2B5F4 → 2B524 → 13A08`. The fixture declares
four narrowly bounded pointer words (including the
header's `+0x10` view) and a cold local model descriptor.
The two authored material intervals are separately
mapped read-only: the 24-byte `A8F4` view at `00676F6C`
and 20-byte data at `00676F58`. None is an observed
archive parse or published model. No old child, DObj,
allocation, class result or heap is imported.

Original `80041A54..68` writes the two context words
(`255,0`) on a fresh resource block. The fixture enters
the **actual `80007030` wrapper** with that non-null
context operand. Cold original Base/default-MObj/wrapped
MObj class images are declared, not completed vtables.
`8024298C → 8025D150` must lazily register both MObj
classes and prove the wrapped class's ancestry; the
original copy is 20 bytes from Base and 44 bytes from
default MObj. Original selector store `802429CC` and
context store `8008D718` must publish `802DA7F0` and
resource `+3112`. Verify `+0x40/+0x44` of both classes
and the resulting Base/MObj/subclass links. **Stop at
`800070A4 → 8024F834`**, with the real record-0 root
descriptor operand and suspended wrapper frame. No
factory instruction, heap allocation or class/instance
consumer executes.

The first missing factory input beyond this cut is the
exact key-0 descriptor/load domain (and original
default-JObj override/heap state) in this new generation.
Record 0 builds key 0, then 1, 2 and 3, including the
earlier null-role registries and lifetimes, before the
terminal DObj under key 3. Neither the *independent*
key-3/DObj results nor a modified record root can be
substituted to reach `80242A74`. Reuse the earlier OS/HSD
allocation, class and descriptor-registry cuts in a fresh
wrapper-owned generation with an audited original heap
and bounded material ancestry. The original wrapped
class/loader code and exact Red terminal descriptor
are compatible at the ABI level; their joined original
allocation and loader return are still unproved here.

Texture override `80436920` is deliberately **unmapped**
and unclaimed: no prefix instruction consumes it.
For a later independent cold SDK owner, qualify its
initial BSS lifetime (or source-produced live binding)
from before wrapper entry through `80243508`, with no
texture owner or setter in between. Do not substitute
the class-amnesia clear, a post-stop null, or a ST42
texture result. The deferred factory, base loader and
cache/compiler ranges are pinned read-only for review;
they are not in the runnable text range. The previously
audited `80242A74` loader return, override check,
actual expression graph, `802604F8` compiler and
retirement, and the two distinct outputs at
`MObj+0x18/+0x1C` remain separate gates before the
real `80242A98 → 8023DA04` return. The terminal
material phase cannot yet produce a trustworthy cache
or ownership store.

Static syntax, source and receipt pins, exact record and
terminal material views, 571 instruction visits, 250 bus
visits, eight frames and eleven returns passed. Fixture
SHA-256 `8eeba87b0639017e7c1edd975748031676547a5f62a967f4fed3968bfded3983`.

The sole reserved run observed source context words
`255,0` at `803BC0E8`, the default and wrapped MObj
class registrations with both inheritance copies, and
the Base → default MObj → wrapped MObj links. Final
class readbacks verified default instance size 32
and loader/expression slots `802428EC/80242A9C`;
wrapped size 36 and slots `8008D72C/8008D7EC`.
Original wrapper readback confirmed selector `802DA7F0`
and context `803BC0E8`. It stopped at `800070A4 →
8024F834` with record-0 root operand `00680080`, before
entering the JObj factory. The local model descriptor
remained unpublished and the texture override unmapped.

Exclusive receipt `retail-0600-material-wrapper-prefix-v1.json`,
SHA-256 `f03e4c306af7827318da13d63e8823fdaa716298268f0df8db05926f2946e180`.
One isolated process, 0.163 s elapsed, 5,643,520 KiB
available immediately before launch, 31,268 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All pinned artifacts matched. No retry, successor,
compile, engine/package changes, context transplant,
material/cache execution, pose or admission.

#### Machine Red wrapper-owned root descent — static plan, no new fixture

The passing wrapper receipt ends at `800070A4 → 8024F834`
with root operand `00680080`, selector `802DA7F0`, context
`803BC0E8`, and the caller frame still active. That
generation **has no initialized OS heap**. It cannot
resume on the standalone key-2/key-3 heap. A new
continuation must declare and initialize its own arena
and registry in the same evaluator as the original
wrapper, before source consumes an object. The receipt
is a comparison, not a frame, heap or context image.

The first source dependency beyond the existing cut is
the root JObjDesc word at `8024F858` (`00680080`, not
currently mapped). The exact four 64-byte role-4 views
are pin-able from AFS 575's relocation data at the
existing address basis. Their class-name words are
authored nulls, so `8024F870 → 802509C8` is the default
JObj path provided its **own** override at `80436980`
is qualified. This is separate from the wrapped MObj
selector; do not clear or replace that selector. The
default constructor then needs source cold registration
and `802509F0 → 8025D088` original heap allocation.
Above the allocation, the Base link history is now
different: Object registers in front of the already
registered default MObj, retaining its existing link to
wrapped MObj, and JObj registers under Object. The
standalone cold class receipt does not authorize
replacing the Base head or restoring it to zero.

| Expected source visit | Descriptor | Children / DObj | Registry slot before descending |
|---|---|---|---|
| key 0 | `00680080` | key 1 / null | 89 |
| key 1 | `006800C0` | key 2 / null | 52 |
| key 2 | `00680100` | key 3 / null | 15 |
| key 3 | `00680140` | key 4 / body `BBF8` | not reached before DObj |

Keys 0–2 have null matrix-extension and RObj operands,
with authored flags `10000000` before the source merge.
Each must complete its own null DObj/RObj helpers
(`8023DB78 → 8024F6CC`, `80258CFC → 8024F6D8`), local
default matrix, `802596A4` registry grow/pop/return
and backpointer store at `8024F768` **before** reading
the next child at `8024F778`. Key 2's sibling key 13
is not read before the pending key-3 loader returns.
Key 3 has authored flags `10050180` and the first
non-null DObj (`00678270`), reached on the source
`8024F6C8 → 8023DAE4` edge before key 3's own registry
or child recursion. No parent link or root publication
precedes this cut: `8024F804 → 8024FF64` requires a
returned child and `8024F8BC → 8024F9D4` follows the
root loader's return. This proves static traversal
order, not original execution.

**Shared ownership and bounds to qualify together.**
The cold 8 KiB arena arithmetic is sufficient through
the first non-null DObj inlet. Predicted JObj cells are
`00600160/00600200/006002A0/00600340`. The 160-byte
carves successively leave free tails in buckets
26/21/16/11. Predicted OS requests in source order
are `128,20,20,1024,12,20,12,20,12,20`: the three
12-byte requests are fresh source registry pool growths,
not three registry entries from the old key-2 receipt.
This leaves 6,432 OS-free bytes at `006006E0` and a
384-byte slab tail. Exact registry hash and slot checks
must follow each live operand and retain all previous
keys; the previous single fresh registration does not
qualify those interleaved allocations. The wrapper-owned
loader SPs are predicted `003000B0/90/70/50`, 96 bytes
deeper at key 3 than at key 0. Check source branch,
SP, saved return, per-PC read/write widths, current
bucket/registry cells, stack dispatcher and frame
restoration; do not copy the standalone absolute stack
or slot ledger. Keep key-0..3 descriptors read-only and
gate writes to exact class links, OS headers, SDK bucket
records, allocated cells and the actual cold registry.
No permitted write to selector/context or texture
override is required in that descent. Existing receipt
guards establish the per-primitive behavior but not
the joined 4-node stack, relocated read set and
multi-owner write/return ledger.

**Next reusable witness:** qualify one cold,
wrapper-owned root-to-first-display runner as a unit:
original heap setup (`801FFEC0`, arena setters,
`80253DF0`), registry pool init and clear before the
wrapper, then the above exact three null-role
registrations and fourth source JObj initialization.
Stop at the key-3 DObj inlet, not a single intermediate
allocation. No runnable continuation was prepared:
the combined heap/class/registry schedule and its
guarded reads and writes have not yet been pinned as a
unit. The earlier DObj phase would need requalification
on this stack/heap: four 32-byte DObjs reduce the
384-byte tail to 256 and are projected to leave 6,112
OS-free bytes. That is not the old terminal v2 state.
The wrapped pre-compiler projection alone needs 448
slab bytes, so this branch must qualify slab growth
before the Red material/cache return; the unresolved
texture override is only consumed later at `80243508`.

Static checks matched the wrapper receipt and parents,
pinned factory/loader/registry ranges, eleven call
edges and the exact authored graph/hash/arena
arithmetic. No replay, compile, fixture launch,
engine/package edits, old-state transfer, tree or
pose publication, input or admission.

#### Machine Red wrapper-owned root to first display — stopped on JObj class accounting

`build/borg-ordered-audit/retail-0600-wrapper-root-first-display-v1.py`
turns the preceding static plan into one reservation-only
fixture. It supplies a fresh aligned 8 KiB arena, two OS
heap slots, the 24 MiB memory-size operand, cold registry
and JObj-override BSS, and original cold Object/JObj
class images. Original OS/HSD setup creates the heap and
dispatch; original registry leaves initialize the 12-byte
pool and 101-slot hash. All of this runs on the new
evaluator before replaying the passing record-0 wrapper
and checking its original selector/context publication.
Only the wrapper's context-store assertion is scoped to
writes after setup. No previous heap, actor, class output,
or registry entry is loaded.

Four 64-byte read-only JObj views at
`00680080/006800C0/00680100/00680140` use the exact
authored role-4 records and relocation membership.
They are declared views, not replayed archive relocation.
The old DObj, any cache or texture override and all
further JObj descriptors are outside the mapped inputs.
The original non-null display call is a stop, not a stub.

The ordered descent checks 4,567 instruction visits,
1,830 bus visits, 95 frames and 134 returns with lowest
SP `002FFFD0`. It includes three real null DObj/RObj
returns, three original hash/registry growths and
backpointer stores, and the fourth JObj's default
initialization and merge. Runtime memory guards distinguish
exact frame saves/restores (including the `r11` helpers),
read-only descriptor words, class links and methods,
current OS headers, source-returned bucket records, slab
cells and current registry writes. Register and source
readback checks pin all ten raw requests and the source
hash domains; only exact update-store, positive `addze`,
`mulhwu` and dispatch-publication instruction adapters
are used. No helper return is injected.

**Stop:** `8024F6C8 → 8023DAE4` with the key-3 DObj
operand `00678270`, before any instruction of that
non-null factory visit. The earlier three visits to the
same address are the authored null arms and must run and
return; the stop is ordered by its live frame/descriptor.
Expected readbacks: source JObjs
`00600160/00600200/006002A0/00600340`, registry slots
89/52/15 for keys 0–2, key-3 flags `100501C0`, and 6,432
free bytes at `006006E0`. Selector and context must
remain live and unchanged and all child links and the
local model's root slot must remain null. Display, cache,
texture override and tree return are not qualified.

Static syntax, source and receipt pins, views, cold/warm
allocation and hash arithmetic, ordered branches, frames
and widths passed. Fixture SHA-256
`ac5f09baf1ecec63bf1793a0b400dcda927c48c98a4be2cd1e18bfe79dc3575a`.

The sole reserved run initialized the fresh OS heap
and descriptor pool/hash, then matched the prior
wrapper readbacks: selector `802DA7F0`, context
`803BC0E8`, root operand `00680080`. Original root
allocation requested 128/20/20/1,024 bytes and
returned payloads `00600040/006000E0/00600120/00600160`,
leaving 6,816 OS-free bytes. It stopped at `8025CF1C`,
before the original four-byte store to JObj class
`803ACC10+0x20 = 803ACC30`. That live-instance count
write was missing from the operand guard. The root
constructor did not return; no JObj flag merge, object
registry insertion or display inlet was reached.

Exclusive receipt `retail-0600-wrapper-root-first-display-v1.json`,
SHA-256 `6dd7e026ad4c29f7905211118cb38547f4a7c8398d3682828a9f3188ae4f2815`.
One isolated process, 0.531 s elapsed, 5,664,004 KiB
available immediately before launch, 31,624 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
Prior pins matched. No retry, successor, compile,
engine/package change, old state transfer, pose or
gameplay admission.

#### Machine Red wrapper-owned root class accounting v2 — stopped at first loader entry

`build/borg-ordered-audit/retail-0600-wrapper-root-first-display-v2.py`
preserves and pins the v1 script and refusal receipt. The
observed v1 path returned a slot from `8025CE94 →
8025CF0C` and took the non-null arm at `8025CF10`.
`8025CF14` reads `JObjClass+0x20`, `8025CF18` adds one,
and `8025CF1C` (`901F0020`) writes the live count. The
original re-read and unsigned comparison at
`8025CF20..2C` select `8025CF30` (`909F0024`) only
when the new live count exceeds `+0x24`. The source
free helper decrements `+0x20` and the class-reset
hook clears both counters; neither path is in this cut.
The two stores account for allocated slots before
instance initialization, not completed tree nodes.

V2 permits only `8025CF1C/803ACC30/4` and
`8025CF30/803ACC34/4` at their ordered bus visits.
For each of keys 0–3 the fixture logs live operands
before checking class `r31`, the non-null source slot
in `r3`, the observed preceding allocation count and
exact compare/fallthrough. It requires paired transitions
`0→1→2→3→4`, reads each word back after its source
store, and checks final live/high-water values of four.
There is no range expansion or injected counter. The
cold class bytes and source initialization are unchanged.

The static re-audit covers all 667 non-stack write visits
at 101 PCs in the same ordered phase, including OS
headers, bucket records/carves, Object/JObj linkage and
inherited methods, three registry insertions, local
matrix/default writes and the fourth merge. The follow-on
high-water store was the other uncovered class write.
All other role, stack, method, branch and return guards
remain v1's; the audit is static, not a claim that those
visits have run. The source cursor must still stop at
`8024F6C8 → 8023DAE4`, before the non-null DObj. The
three registry insertions, fourth initializer, merged
flags `100501C0` and 6,432 OS-free bytes remain expected.
No child link or model root is supplied or expected.

Static syntax, source and prior pins, exact authored
descriptors and the write/counter ledger passed.
Fixture SHA-256
`5e09f3e4889fd019bf7f86b55dcb3de396af506a674134d617e218d8c91714d4`.

The sole reserved run reproduced the fresh heap/registry
setup and wrapper selector/context, and all four initial
OS requests, leaving 6,816 OS-free bytes. Source wrote
and read back both key-0 class counters `0→1` at
`803ACC30/803ACC34`, with allocated slot `00600160`.
The source constructor returned through `8025D14C`
and the factory executed the first loader dispatch at
`8024F8B0`. The fixture stopped on the host entry-operand
assertion at v1 line 458, before executing `8024F638`.
Static diagnosis: that assertion compares the pre-prologue
SP with the post-prologue loader SP (`003000B0`);
the original `stwu` at `8024F638` has not executed.
The receipt does not log the live operand tuple at this
assertion. No JObj loader flag merge, object registry
entry, later allocation, or non-null DObj inlet ran.

Exclusive receipt `retail-0600-wrapper-root-first-display-v2.json`,
SHA-256 `9036ace817423aced54659e1c9b9e2255931fa8edd90a293237350bfd266c55c`.
One isolated process, 0.616 s elapsed, 5,568,300 KiB
available immediately before launch, 33,260 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
Prior pins matched. No retry, successor, compile,
engine/package change, old state transfer or admission.

#### Machine Red wrapper-owned loader entry v3 — passed

`build/borg-ordered-audit/retail-0600-wrapper-root-first-display-v3.py`
pins v1/v2 and the v2 refusal. It corrects only the loader
entry assertion's SP basis: compare at `8024F638` before
its `9421FFE0` `stwu`, then check the actual post-prologue
SP at `8024F63C`. Pinned operand moves and the original
`bctrl` define the entry tuples:

| Key | Caller / LR | `r3` object / `r4` descriptor / `r5` parent | SP before → after |
|---|---|---|---|
| 0 | `8024F8B0 / 8024F8B4` | `00600160 / 00680080 / 0` | `003000D0 → 003000B0` |
| 1 | `8024F7E8 / 8024F7EC` | `00600200 / 006800C0 / 00600160` | `003000B0 → 00300090` |
| 2 | `8024F7E8 / 8024F7EC` | `006002A0 / 00680100 / 00600200` | `00300090 → 00300070` |
| 3 | `8024F7E8 / 8024F7EC` | `00600340 / 00680140 / 006002A0` | `00300070 → 00300050` |

Static re-audit derives every selected LR return from
the nested calls, saved LR stores and restores (all 134
returns). It checks 106 checkpoints as a unit: the root
and all child loader dispatch/entry frames, class and
parent init dispatches, the two inheritance copies and
13 clears, ten OS trips, warm bucket rounding, three
registry inlets/hash/lookups/pop returns, true nullable
DObj/RObj visits and the fourth non-null DObj call.
The live PC/SP/LR and relevant registers are logged
against the expected tuple **before** any checkpoint
rejection. The v2 counter gates, v1 ordered bus/width
and bounded writes, source wrapper, fresh 8 KiB heap,
registry, descriptor inputs and stop are unchanged.
No counter, register tuple or return value is seeded.

No later unowned dependency was identified in this
**pre-DObj** static cut. All post-v2 execution remains
expected, including the three registry insertions,
count/high-water `4/4`, key-3 flags `100501C0`,
6,432 free bytes and the final `8024F6C8 → 8023DAE4`
stop with the root and child links unpublished. Non-null
DObj loading, texture override and cache remain outside
scope. Static syntax, source and prior pins passed.
Fixture SHA-256
`b9814559aef6c956530c08a07457cdbb3091332279942b110929a96c07187ce8`.

The sole reserved run matched all 106 live checkpoints
and 1,830 bus visits. The four loader entry/prologue
tuples matched the table, with source JObjs
`00600160/00600200/006002A0/00600340`. The three null
DObj/RObj arms returned normally. Original code
registered keys 0–2 in slots 89/52/15 at
`803FA6C0/803FA62C/803FA598`, with cells
`00600580/00600600/00600680` and matching descriptor
backpointers. Counters read back `4/4`, key-3 merged
flags `100501C0`, and all ten OS requests left 6,432
free bytes at `006006E0`. The original wrapper
selector/context remained `802DA7F0/803BC0E8`.
It stopped at `8024F6C8 → 8023DAE4` with non-null
descriptor `00678270` and SP `00300050`. No key-3
registry insertion, display execution or child/root
publication; texture override remained unmapped.

Exclusive receipt `retail-0600-wrapper-root-first-display-v3.json`,
SHA-256 `14867d2576f58f62ff82d254692b5c6f4da7fc721c123f42f02f8a5fe51f6331`.
One isolated process, 1.284 s elapsed, 5,623,864 KiB
available immediately before launch, 34,700 KiB peak
RSS, 1 GiB soft/hard AS, 54 s internal and 58 s outer
timeouts. All prior pins matched. No retry, successor,
compile, engine/package change, pose or admission.

#### Machine Red wrapper-owned terminal material inlet — stopped in host handoff

`build/borg-ordered-audit/retail-0600-wrapper-terminal-material-v1.py`
rebuilds the passing wrapper-root-first-display-v3 in the
same evaluator and compares its heap, registry, class counts,
loader tuples and stop. It keeps the source-published
`802DA7F0/803BC0E8` selector/context and the existing 8 KiB
arena. Only four exact 16-byte DObjDesc views, a cold
72-byte DObj class, and a declared DObj no-override word
are added. The terminal 24-byte material descriptor and
20-byte authored data are the existing read-only wrapper
views at `00676F6C/00676F58`. No PObj view, texture
override binding or initialized object is supplied.

On this generation Base's original head is Object,
whose existing next sibling is default MObj. Original
DObj registration may write only its exact Base child
head store and must preserve the Object/MObj links.
The four 24-byte DObjs split bucket 11's 384-byte tail;
expected cells are `006003E0/00600400/00600420/00600440`.
The original terminal null-sibling return/store must
execute before `8023DA00 → 802429FC`. That factory
must select the live wrapped class and allocate the
36-byte MObj at `00600460`; the base loader must run
the true authored null-TObj return and allocate/copy
the 20-byte material at `006004A0`. The subclass
must write the source wrapper context to `MObj+0x20`
and return to `80242A70` with mode `3001` and empty
cache fields. None of these values is passed in as a
result.

Seven expected 20-byte raw records use new buckets
`0,10,9,8,7,1,5`; existing bucket 4 receives the
160-byte tail after the material copy allocation.
Expected OS remainder is 5,984 bytes at `006008A0`.
The ordered audit covers 3,660 instruction visits,
1,430 bus visits, 482 non-stack writes, 84 frames and
102 returns; lowest expected SP `002FFF20`. Runtime
checks the source successors and exact stack spills,
authorized descriptor/class/bucket/heap reads and
each non-stack write's PC/address/width/value. No
allocator, class, display or material return adapter;
only the pinned rounding and byte/word update stores.

**Stop at `80243508`**, before dereferencing texture
override `80436920`. Original cache prefix enters via
`80242A74` and checks empty old cache slots and the
`3001` flags before this read. The word is unmapped
and no null/live texture state is assumed. Remaining
expression ownership, compiler growth/retirement and
cache return remain unqualified; even the projected
pre-compiler expression set will not fit this tail
without source slab growth. The terminal `DObj+8`,
the DObj head on key 3, parent child links and the
model root must all remain unpublished; no PObj runs.

Static syntax, source/parent pins, original descriptor
views and exact path, return and write ledger passed.
Fixture SHA-256
`f3e4be3fd9c717a97d7f55494a0cd8e62b5947602dfcc2228bd0373d9601e2cc`.

The sole reserved run initialized the fresh 8 KiB OS heap
and descriptor registry and ran the original wrapper
through `800070A4 → 8024F834`. It stopped in the host
handoff before the first JObj factory instruction:
`continue_display` was not defined in the nested v2
runner's namespace when it built the descendant runner.
No root OS request, class count, JObj, DObj, MObj,
material copy or cache prefix ran. No later heap readback
was made; the source setup verified the initial 8,160-byte
free block. The planned override-read stop was not
reached and the override was not read.

Exclusive receipt `retail-0600-wrapper-terminal-material-v1.json`,
SHA-256 `e094dc3b42b708eb6eca2b3c97db0181c25ad150b9e13a2207e43dd90d95c3c5`.
One isolated process, 0.422 s elapsed, 5,627,692 KiB
available immediately before launch, 35,048 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
Earlier pins matched. No retry, successor, compile,
engine/package edits, old state transfer, tree/pose or
admission.

#### Machine Red wrapper-owned terminal material v2 — passed

`build/borg-ordered-audit/retail-0600-wrapper-terminal-material-v2.py`
preserves the v1 script and refusal. The traceback's
`wrapper-root-first-display-v2.py` `run` line 155 is the
host read constructing the inner `exec` globals. That
nested `run` shares the transformed V2 witness's globals
(the outer `exec` in root V3). V1 added the callback to
the inner mapping for the transformed V1 `descend`, so
V2 `run` could not resolve the name while building that
mapping, and the allocated root path never started.

V2 adds the callback to the **outer V2-witness globals**
and leaves the inner `descend` mapping unchanged. The
only invocation remains after the original ordered
descent and its final class-counter checks, on the same
evaluator. It first compares the proved pre-DObj
boundary, nodes, OS requests, counters, live checkpoints
and heap scope; it does not treat the receipt as a state
image. The later V3 checkpoint check uses the existing
journal and permits replacement of only the boundary
result. Exceptions still clear that result and preserve
failure evidence.

The inert host check parsed the actual scope mappings,
confirmed the V2 `run` insertion site and later receipt
assertions, reproduced the missing global at construction
of an inner mapping with a stand-in function, and reached
the synthetic callback exactly once after the proved
`8023DAE4` boundary with the corrected globals. It used
existing inert Python code objects, not compilation of a
transformed fixture, did not import the source evaluator,
did not execute original instructions, and opened no
output receipt. Parent and earlier receipt hashes matched.

The original continuation, PC/data/write guards and stop
at `80243508` are unchanged. All DObj and wrapped-MObj
allocations, the material loader return and 5,984-byte
remaining heap are expected only. Texture override
remains unmapped; no cache, polygon, child or root result
is supplied. Fixture SHA-256:
`f044aa9f9579d1c587d622485d3128884770a8371cb9eb53d520e3250cc3add7`.

The sole reserved replay reached the live callback on
the proved pre-DObj frame and completed the original
continuation. It observed four DObjs at
`006003E0/00600400/00600420/00600440`, the terminal
null-sibling return/store, seven source 20-byte bucket
records and 5,984 OS-free bytes at `006008A0`.
The original wrapped MObj loader returned to `80242A70`
with object `00600460`, exact material copy `006004A0`,
mode `3001`, null TObj/PE/graph/cache fields and the
original wrapper context at `+0x20`. All 1,430 bus
visits and 482 non-stack writes matched. The prefix
entered the cache builder and stopped at `80243508`
before reading unmapped `80436920`. No cache return,
PObj, DObj head, child link or model root was published.

Exclusive receipt `retail-0600-wrapper-terminal-material-v2.json`,
SHA-256 `5cdf06a4f698d46feffedcd55afbd8f3a1df2d66dc5a3e265c51e639d2a86e66`.
One isolated process, 2.086 s elapsed, 5,586,088 KiB
available immediately before launch, 36,364 KiB peak RSS,
1 GiB soft/hard AS, 54 s internal and 58 s outer timeouts.
All prior pins matched. No retry, successor, compile,
engine/package change, tree/pose or admission.

#### Machine Red cache override — cold startup static check

The pinned DOL has no initialized file bytes for the
override word `80436920`. Original startup takes
`8000316C → 80003340`; the clear-table record at
`800055B8` covers `[804360C0, 80436A1C)`, including
the whole override word. The table loop passes fill
byte zero to `8000540C` at `800033D8`. This establishes
the cold-start value by static inspection, not the
value at a later material load.

A scan of aligned DOL text for direct `r13−19584`
memory operands found four loads (`80243508`,
`80243528`, `802436B8`, `802436D8`) and one store
(`802439DC`, the class-amnesia clear). This scan
does not exclude writes through computed addresses.
A cold-cache witness must carry or verify the startup
zero through its bounded setup; it must not call the
class-amnesia hook to manufacture a null input. No
cache output nor the terminal DObj material store is
observed.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-cold-override-v1.py`,
SHA-256 `0141863c169c69e42a948f2aceb1b0a1a6e60750d3d6a4cd7bfe42e32f046687`.
Before heap initialization and wrapper entry, it calls the
original fill routine with the pinned clear-table operands
on the same evaluator. It checks every store over the
canary-filled 2,396-byte range, retains only the source-written
four-byte override as read-only, and rolls back the scratch
frame and other temporary state. This is an isolated startup
fragment, not a whole retail boot or a statement about the
override in an arbitrary running match.

After rebuilding the observed v2 material state, the fixture
requires the original override read at `80243508` and the
null-list return from `802401B0`. It stops at `80243550`
before calling the wrapped expression method `8008D7EC`.
The zero read, null-list return and dispatch operands are
expected, not observed. No expression graph, compiled cache,
polygon work or material attachment is supplied or inferred.
Static source and parent pins, branch and memory-site checks,
and the parent's inert callback check passed. The new receipt
is unused; prior artifacts were preserved.

The single reserved v1 replay verified **599 original zero
word stores**, including `80436920`. It reached the planned
pre-expression dispatch: the guarded override read returned
zero, the original null-list helper returned zero, and the
wrapped expression target and arguments passed the continuation
checks. The outer host comparison nevertheless failed on
`declared` because its baseline did not include the extra
four-byte read-only view. The receipt therefore has no success
boundary; the expression itself never ran. Receipt
`retail-0600-wrapper-cold-override-v1.json`, SHA-256
`d4464faa0de5700d27c77df31059090ba7b784223ed1a91134682a9a941a30d0`.
One isolated process, 2.125 s elapsed, 5,746,572 KiB available
at launch, 43,284 KiB peak RSS, 1 GiB soft and hard address-space
limits, 54 s internal and 58 s outer timeouts.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-cold-override-v2.py`,
SHA-256 `d9f3f52a8a75b70f8d648ea50d5f067e48289c9dcd7cb4e42e5ee0863ce93901`.
It verifies that the only added view is `80436920`, width four,
then compares the inherited metadata separately from the full
scope. The original replay and stop are unchanged, and the new
receipt retains a separate dispatch record before outer host
checks. Static pins and an inert handoff check passed; no new
receipt was opened during preparation.

The single reserved v2 attempt opened its exclusive receipt,
then failed before executing any original instruction. The
inner witness repeated the fresh-path assertion after the
runner had already created the receipt. This was a host
lifecycle failure, not a different override result. Receipt
`retail-0600-wrapper-cold-override-v2.json`, SHA-256
`f822ea6813c6dcc04648cc2290e5aacda44a218b2c7b468e66d059fdaf4bc772`.
One isolated process, 0.003 s elapsed in the runner,
5,666,356 KiB available at launch, 42,768 KiB peak RSS,
1 GiB soft and hard address-space limits, and a 58 s outer
timeout. Both earlier receipts are preserved.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-cold-override-v3.py`,
SHA-256 `3f44fbef0cb8d8270534384f06ffc84f3d9b41337cdb2547b901cfd55d036622`.
It completes freshness and pin checks before the exclusive
open and passes the prechecked inputs to the inner witness.
An inert test exercised the metadata comparison and dispatch
journal while a disposable exclusive receipt was open. The
real receipt is unused. The original route and pre-expression
stop are unchanged; no automatic retry was launched.

The sole reserved v3 replay **passed**. The isolated original
startup fill made all 599 zero-word stores. The wrapper then
loaded the same material with 5,984 heap bytes free. Original
code read `80436920 = 0`, returned an empty texture list, and
selected `8008D7EC` as the wrapped expression method. The
replay and final host checks completed at `80243550`, before
calling that method. No cache output, polygon, or terminal
material attachment was produced. This qualifies the scoped
cold route, not the override in a running match.

Exclusive receipt `retail-0600-wrapper-cold-override-v3.json`,
SHA-256 `8a910605ee0c7f58d1ae0abef7244797194de0da207573975d3af345d4c32e91`.
One isolated process, 1.999 s elapsed, 5,674,992 KiB available
immediately before launch, 43,876 KiB peak RSS, 1 GiB soft/hard
address-space limits, 54 s internal and 58 s outer timeouts.
Prior receipts were preserved; no retry, successor replay,
compile, package change, or Battle Test admission.

#### Machine Red first material expression — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-expression-root-v1.py`
continues the passing cold-override v3 route on the same
evaluator. Original dispatch at `80243550` enters the wrapped
method and calls its base expression method. For mode `3001`,
the selected path skips the texture-list walk, clears the
graph head, and requests the first 108-byte node at
`8025DFBC → 8025CCD0`. The old 160-byte slab tail can
supply the rounded 128 bytes and leave a 32-byte remainder.
The new size-class record itself requires one 20-byte request
on the existing source heap; no new slab or allocator result
is supplied.

The fixture requires the original `0xFF` fill, kind-1 fields,
and publication at `8025E000` into `MObj+0x1C`. It stops at
`80242CDC → 8025E048` before interning the first material
constant. Node `006004C0`, bucket-3 record `006008C0`, and
5,920 OS-free bytes are expected, not observed. The compiled
list at `MObj+0x18` must still be null. No expression graph
completion, compiler output, material attachment, or Battle
Test admission follows from this checkpoint.

Static pins and checks covered 502 instruction visits, 208
memory accesses, 11 frames, 13 returns and 83 non-stack
writes. An inert test passed the same owner through the nested
handoff and preserved the inherited and new receipt boundaries.
Fixture SHA-256
`4caa81f9635aa237634d97b8ea4a8ee11f3cdd45e0f5e73a0488f59605f54765`.
The exclusive receipt is unused. No original replay or
compile ran; prior artifacts were preserved.

The sole reserved v1 attempt reproduced the cold override
prefix and stopped at its host dispatch check before the first
expression instruction executed. V1 expected the cache
caller's return address (`80242A78`) in LR. The pinned
null-list call at `80243534` sets LR to `80243538`; its
epilogue restores that address and returns, and the next
instructions up to `80243550` leave LR alone. The v1 receipt
did not log the live tuple. No expression allocation, heap
split, initialization, or publication was observed. Receipt
`retail-0600-wrapper-expression-root-v1.json`, SHA-256
`f1d3dd74a7b9d83c7350892417ed8c712278f19546de2ea5088a8bc84a2ebb26`;
2.036 s elapsed, 5,648,848 KiB available at launch, 45,884 KiB
peak RSS, one isolated process with 1 GiB soft/hard
address-space caps and a 58 s outer timeout.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-expression-root-v2.py`,
SHA-256 `fa736c5053f765d5fdd5174e0b9e23726296281e97dc12f8ca5a7ec5a3daae87`.
It derives the expected LR from the pinned prior call and
journals all six entry operands before checking. The inert
control accepted `80243538` and rejected `80242A78`, without
executing original instructions. The original expression
path, allocator guards, memory scope, and pre-constant stop
are unchanged. Prior artifacts were preserved; no retry,
compile, or gameplay admission.

The sole reserved v2 replay **passed**. At dispatch, LR read
back `80243538` and CTR `8008D7EC`. All 502 instruction
visits, 208 memory accesses, and 83 non-stack writes matched.
Original code allocated the bucket-3 record at `006008C0`,
initialized the 108-byte kind-1 node at `006004C0`, and
published it to `MObj+0x1C` at `0060047C`. The 32-byte slab
remainder and 5,920 OS-free bytes both read back. `MObj+0x18`
and the terminal DObj material slot remained null.

The replay stopped at `80242CDC → 8025E048`, with operand
`006004A4`, before executing the first constant intern.
Expression completion, compilation, and material attachment
remain unverified. Exclusive receipt
`retail-0600-wrapper-expression-root-v2.json`, SHA-256
`9c7b35fb734e2bc6d1d729d45573567271a385d78150885ed396865fef135e39`.
One isolated process, 2.075 s elapsed, 5,635,468 KiB available
at launch, 44,616 KiB peak RSS, 1 GiB soft/hard address-space
limits, 54 s internal and 58 s outer timeouts. No retry,
successor replay, compile, package change, or admission.

#### Machine Red first material constant — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-expression-constant-v1.py`
rebuilds the passing first-expression checkpoint before entering
the original constant intern at `8025E048` on the same evaluator.
The expected path finds no matching constant on the one-node list,
pops the remaining 32-byte bucket-0 slot at `00600540` for the
20-byte kind-4 object, and publishes the borrowed material key
`006004A4` (`material+4`) and the original node as its successor.
No new slab, heap service, allocator result, or graph node is
supplied. The fixture also verifies the base node's source stage
setup and the constant pointer passed via the caller's stack.

The planned stop is `80242D20 → 8025E8F0`, before the operand
setter. The new list head, zero reference count, and unchanged
5,920 OS-free bytes are expected, not observed. Publishing the
constant on the allocation list is distinct from retaining it as
an operand; the setter, later constants, compiler outputs, and
terminal DObj attachment remain unverified.

Static pins, 185 instruction visits, 78 accesses, four frames,
eight returns, and inert same-owner handoffs passed. Fixture
SHA-256 `06bb730b0b165243eecc3630c806630e57a71aeaa16a6038e6d71647a893e405`.
The exclusive receipt is unused. No original replay, compile,
package change, or Battle Test admission.

The sole reserved replay **passed**. Original code popped the
remaining bucket-0 slot at `00600540`, created the kind-4
constant with borrowed material key `006004A4`, and linked it
ahead of the existing kind-1 node. It configured the base stage
and wrote the constant pointer to the operand setter's caller
frame. The constant reference count remained zero, the compiled
list remained null, and the OS heap still had 5,920 bytes free.
All 185 instruction visits and 78 accesses matched. It stopped
at `80242D20 → 8025E8F0`, before the setter executed.

Exclusive receipt `retail-0600-wrapper-expression-constant-v1.json`,
SHA-256 `79b10c451b6e05fcd5718eb50fe488d4221328533173fafeab58d4f844fcee07`.
One isolated process, 2.147 s elapsed, 5,637,716 KiB available
at launch, 45,632 KiB peak RSS, 1 GiB soft/hard address-space
limits, 54 s internal and 58 s outer timeouts. Prior receipts
were preserved. No retry, successor replay, build, or admission.

#### Machine Red first material operand — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-expression-edge-v1.py`
rebuilds the original first constant and enters the four-operand
setter on the same evaluator. The selected path must initialize
the three null operands and attach constant `00600540` to operand
3 on base node `006004C0`, with selector 1. It checks the four
null displaced operands, the source pointer write, and the
original retain at `8025E704` changing the constant's byte count
from zero to one. Nothing is freed or newly allocated in this cut.

The stop is `80242E2C → 8025E048`, before the second constant
intern. The next key is expected to be `006004AC` (`material+12`).
The first operand and reference increment are expected, not
observed; this does not supply a second constant, compile the
cache, attach the material, or admit gameplay.

Static pins and inert same-owner handoffs passed; the audit
covers 538 instruction visits, 221 accesses, nine frames, and
27 returns. Fixture SHA-256
`a6f21a17e44bd8e49ff09f18b8f51a51b886ccaf2f24cd4ab340786fbe3371f6`.
The exclusive receipt is unused. No original replay, compile, or
package change; prior receipts were preserved.

The sole reserved v1 replay rebuilt the first constant and
checked 31 setter accesses. It stopped before the first null
operand finished: `8025E3E4` selected `8025E3F8`, which v1's
trace excluded. No retain or second intern was observed. Receipt
`retail-0600-wrapper-expression-edge-v1.json`, SHA-256
`3a7f7d8df396656b9f2e550c6c18a9ab1b3451f7b9aedabc1f575c8a3b38a1ae`.
One isolated process, 2.127 s elapsed, 5,629,344 KiB available at
launch, 46,272 KiB peak RSS, 1 GiB soft/hard address-space caps
and a 58 s outer timeout. Prior artifacts were preserved.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-expression-edge-v2.py`,
SHA-256 `034d7016ffa8f9bfecc38bff997dfd804dab52641c31bab1d206b18fa733c036`.
The pinned `cmpwi r25,7` followed by `bf 0` selects the direct
null initializer for all three selector-7 inputs. The selector-1
constant still takes the kind-4 retain path. V2 checks the
updated path (514 instruction visits, 218 accesses, nine frames,
27 returns) and keeps the same pointer, reference count, and
pre-second-intern stop. Static and inert checks passed. The
exclusive receipt is unused; no retry or compile was launched.

The sole reserved v2 replay **passed**. Original code
attached `00600540` to operand 3 of node `006004C0` with
selector 1, and the constant's reference count went **0→1**.
All four displaced operands were null. The 218 checked accesses
matched; no allocation or retirement occurred and OS-free
space stayed at 5,920 bytes. The compiled-list slot remained
null. The replay stopped at `80242E2C → 8025E048` with
material key `006004AC`, before the second constant intern.

Exclusive receipt `retail-0600-wrapper-expression-edge-v2.json`,
SHA-256 `46eba3d9ac44fd1e8cb6ff32bbbe3c11fbecdf7cd4e4456e0bc02511b6166e39`.
One isolated process, 2.234 s elapsed, 5,601,392 KiB available
at launch, 45,040 KiB peak RSS, 1 GiB soft/hard address-space
limits, 54 s internal and 58 s outer timeouts. Prior artifacts
were preserved; no retry, successor replay, build, or admission.

#### Machine Red second material constant — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-expression-second-constant-v1.py`
rebuilds the verified first operand and reaches the second intern on
the same evaluator and heap. The selected lookup misses the retained
constant because its borrowed key is `material+4`, not `material+12`.
No existing bucket is expected to have a free donor. Original code
must request the 20-byte bucket-30 record, then a 1,024-byte slab, on
the retained OS heap. The fixture supplies no new storage or results.

The expected second constant is `00600940`, with key `006004AC`,
kind 4, selector bytes `(6,3)`, and zero references. It must link ahead
of the first constant without changing the verified first edge. The
expected original remainders are 992 bytes in the slab and 4,800 bytes
free in the OS heap. The stop is the original return to `80242E30`,
before second stage setup or operand attachment. These results are
not observed. There is no compiler run or material attachment.

Static pins and inert same-owner handoffs passed; the ordered audit
covers 551 instruction visits, 214 accesses, 11 frames, 16 returns
and 57 non-stack writes. Fixture SHA-256
`1a4f5f253fb88baa4bdcf32fa524aa86f2f189adc20345192c9e543e96e44781`.
The exclusive receipt is unused. No source replay, compile, package
change, or admission.

The sole reserved replay **passed**. Original code allocated the
bucket-30 record at `00600900` and requested the 1,024-byte slab on
the same heap. It interned the second constant at `00600940` with
borrowed key `006004AC`, selector bytes `(6,3)`, and zero references.
The first operand stayed attached with one reference. Readback
confirmed 992 free slab bytes and 4,800 OS-free bytes; all 214
accesses and 57 non-stack writes matched. Original code returned to
`80242E30`, before second-stage setup or operand attachment. The
compiled-list slot stayed null.

Exclusive receipt
`retail-0600-wrapper-expression-second-constant-v1.json`, SHA-256
`d69f9b22c33384db0711caa6b4aa9b5d918e6a1b898f2d65f2d62ce49e23651f`.
One isolated process, 2.180 s elapsed, 5,657,680 KiB available
at launch, 46,728 KiB peak RSS, 1 GiB soft/hard address-space
limits, 54 s internal and 58 s outer timeouts. Prior receipts
were preserved; no retry, successor replay, build, or admission.

#### Machine Red two-operand base expression — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-expression-base-return-v1.py`
rebuilds the passing second-constant receipt on the same heap. It
requires source to configure the second channel, attach and retain
`00600940` without changing the first edge, and return the original
base node `006004C0` through `80243464 → 8008D820`. The scoped null
texture list means the base method selects the same node for both
paths; its final checks must return that node without allocating a
combiner. The second reference count is expected to go from zero to
one; both operands should then have one reference.

The stop is `8008D838 → 8025E048`, before the wrapper interns the
source context pointer `803BC0E8` as a constant. No wrapper node,
cache compilation, or terminal DObj material attachment is supplied
or inferred. The base return and retain remain expected, not observed.

Static pins and same-owner handoff controls passed; the audit covers
598 instruction visits, 234 accesses, 10 frames and 34 returns.
Fixture SHA-256
`3affca7bfa4a596f7a690a5148cc3b8b1012c841bd14edad2c3af9687321cec4`.
The exclusive receipt is unused. No replay, compile, package
change, or admission.

The sole reserved v1 replay verified the second retain **0→1**
and the base expression return `006004C0` at `8008D820`. It
checked all 234 accesses and reached the wrapper context-intern
inlet without executing it. The host's final channel readback
comparison failed: it expected `040606FF`, but the audited
stores leave the third byte of operand 3 unchanged (`FF`), so
the source-derived expectation is `0406FFFF`. The v1 receipt
did not log a separate channel readback and has no success
boundary. Receipt
`retail-0600-wrapper-expression-base-return-v1.json`, SHA-256
`ce530dad16a892a52be764e9282ccf052c2fce69d2484c720ba5b293ad09b20b`;
2.217 s elapsed, 5,637,184 KiB available at launch, 47,116 KiB
peak RSS, one isolated process with 1 GiB soft/hard address-space
caps and a 58 s outer timeout.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-expression-base-return-v2.py`,
SHA-256 `3dba706800cdd56caa4d50e443b7deb3ca98c8c81aab00a03edd7743217b8a3c`.
It derives all four expected metadata words from the initialized
bank and the exact guarded source stores, and logs the actual
words and pointers before comparing them. The execution, memory
guards, and stop are unchanged. Static and inert checks passed.
The new receipt is unused; no retry, compile, cache output,
material attachment, or admission.

The sole reserved v2 replay **passed**. Original code attached
the second constant `00600940`; its count rose **0→1**, and the
first constant stayed at one. The second channel read back three
null operands and `0406FFFF` with pointer `00600940`. Original
code returned base expression `006004C0` without a combiner,
then reached `8008D838 → 8025E048` with the wrapper context
pointer `803BC0E8`. It did not enter that intern. All 234 guarded
accesses matched and the compiled-list slot remained null.

Exclusive receipt `retail-0600-wrapper-expression-base-return-v2.json`,
SHA-256 `f3ee6aad0d134f45b28fce0d4993351d2a1319de8b74f38397c5bba6be941264`.
One isolated process, 2.326 s elapsed, 5,641,988 KiB available
at launch, 46,112 KiB peak RSS, 1 GiB soft/hard address-space
limits, 54 s internal and 58 s outer timeouts. Prior receipts
were preserved; no retry, successor replay, build, or admission.

#### Machine Red wrapper context constant — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-context-constant-v1.py`
rebuilds the verified two-operand return and enters the original
wrapper context intern on the same evaluator. The lookup must miss
both material keys and preserve their references. Its borrowed key
is the **address** `803BC0E8`, not either context word or an address
on the wrapper stack. The existing 992-byte tail is expected to
supply the 20-byte constant with 32-byte rounding. Original code
must obtain one new 20-byte bucket-29 record on the retained heap;
no callback, allocator outcome, or new slab is supplied.

The proposed output is kind-4 constant `00600960`, selector bytes
`(1,0)`, zero references, with both prior operand counts still one.
Expected remainders are 960 slab bytes and 4,736 OS-free bytes. The
fixture follows the original return and register-only handoff to
`8008D844 → 8025DF88`, before wrapper-node allocation. No wrapper
expression, cache output or material attachment is supplied.

Static pins and inert same-owner handoffs passed; the audit covers
483 instruction visits, 192 accesses, eight frames, 12 returns and
51 non-stack writes. Fixture SHA-256
`25a67bcdfab0a5a6ee2aa3a5526afdcd4d076729af6a33b05aaae463dd5e6526`.
The exclusive receipt is unused. No replay, compile, package
change, or admission.

The sole reserved replay **passed**. The original intern stored
the borrowed wrapper context address `803BC0E8` in kind-4 constant
`00600960`, with zero references, and kept both earlier operand
counts at one. It obtained the new bucket-29 record on the same
heap, leaving 960 free slab bytes and 4,736 OS-free bytes. All
192 accesses and 51 non-stack writes matched. The wrapper then
reached `8008D844 → 8025DF88`, before allocating its result node.
No new slab, wrapper expression, compiled list, or DObj material
attachment was produced.

Exclusive receipt `retail-0600-wrapper-context-constant-v1.json`,
SHA-256 `ba7d2c1aee972bc60298434e7a8dd0344838bb55ccc603ee6a94b428d66efd15`.
One isolated process, 2.234 s elapsed, 5,657,108 KiB available
immediately before launch, 47,348 KiB peak RSS, 1 GiB soft/hard
address-space limits, 54 s internal and 58 s outer timeouts.
Prior receipts were preserved; no retry, successor replay, build,
or admission.

#### Machine Red wrapper expression node — prepared, not run

`build/borg-ordered-audit/retail-0600-wrapper-expression-node-v1.py`
rebuilds the passing context intern and runs the original 108-byte
wrapper-node request on the same evaluator. The expected source
split takes 128 bytes from the 960-byte tail and needs one 20-byte
bucket-25 record, not another slab. Original code must initialize
node `00600980`, prepend it to the material graph list ahead of the
context constant, configure its first channel, and put the context
pointer in the exact outgoing argument slot at `8008D860`.
Both earlier material operands must remain retained.

The planned stop is `8008D884 → 8025E8F0`, before the first
wrapper operand setter. The new node, 832 free slab bytes and
4,672 OS-free bytes are expected, not observed. Prepending the
node to the allocation list is not attaching the base expression
or its context constant, nor returning a finalized wrapper to the
compiler. Cache output and material attachment remain unverified.

Static source pins, 584 instruction visits, 231 accesses,
10 frames, 15 returns, 86 non-stack writes, and inert same-owner
handoffs passed. Fixture SHA-256
`d6db0dea75d01691d98873202440a66f1179e2804813848ee80cb1db3b51577f`.
The exclusive receipt is unused. No replay, compile, package
change, or admission.

The sole reserved v1 attempt reproduced the context intern,
then stopped at the wrapper allocator's donor branch `8025CD48`
after 49 checked accesses. The static plan overcounted empty
records before the bucket-29 donor. There were no wrapper
allocation, initialization, or other non-stack writes. Receipt
`retail-0600-wrapper-expression-node-v1.json`, SHA-256
`58bfbf9dcf22e0d43dbccbce311b466181d503c902ea41c11926d48fda9491b5`;
2.296 s elapsed, 5,634,632 KiB available at launch, 47,756 KiB
peak RSS, one isolated process with 1 GiB soft/hard address-space
caps and a 58 s outer timeout.

Prepared, **not run**:
`build/borg-ordered-audit/retail-0600-wrapper-expression-node-v2.py`,
SHA-256 `bfb1e80369768c186be45370c3c13ca507da7eb95b88b0a6c924078447fa8f62`.
It derives the donor walk from the linked size classes above
bucket 3: ten empty records, then bucket 29. It logs the live
record and head at each guard. No allocator result or write is
added; the original stop is unchanged. Static pins, 578
instruction visits, 229 accesses, and inert handoff checks
passed. The new receipt is unused; no retry was launched.

The sole reserved v2 replay **passed**. Original code checked
ten empty donor buckets before using the bucket-29 tail. It
allocated and published the 108-byte wrapper node `00600980`,
configured its first channel, and placed the context constant
in the outgoing argument slot. All 229 accesses and 86
non-stack writes matched. Readback confirmed 832 free slab
bytes and 4,672 OS-free bytes. The earlier two material
operands stayed retained. No wrapper operand was attached;
the base-node and context references from the wrapper were
still zero and the compiled-list slot was still null.

The replay stopped at `8008D884 → 8025E8F0`, before
the first wrapper operand setter. Exclusive receipt
`retail-0600-wrapper-expression-node-v2.json`, SHA-256
`49cb64b518e7222d585bbe955a8ae8168d2a29b6e0ce1883fa7219decdad5e34`.
One isolated process, 2.392 s elapsed, 5,641,280 KiB available
at launch, 46,896 KiB peak RSS, 1 GiB soft/hard address-space
limits, 54 s internal and 58 s outer timeouts. Prior receipts
were preserved; no retry, successor replay, build, or admission.

#### Prepared Machine Red reaction and HP comparison (not run)

The private `build/borg-ordered-audit/retail-0600-rifle-outcome-v1.py`
rebuilds receiver-v2's positive contact in the same source frame.
Before continuing at `8002E660` it compares the new receipt to
receiver-v2 and retains the recipient/credit pointers, token
history, contact point, carrier fields and reaction registers.
It does not enter at `8002F108` with a supplied reaction result.
The G-Red outcome probe supplies the reviewed reaction/amount/HP
boundary pattern; the Dragon probe supplies a contrasting detached
amount path, which this fixture does not use as a substitute.

Additional inputs are explicit:

- Machine Red credit: active actor phase 1 and source level-1
  max HP 500. Sasuke receiver: ordinary phase 1, slot 2, max HP
  180, prior-HP sentinel `0xFFFF`, unit scale and the pinned AFS
  495 actor block at `+0x4AC` (damage selectors `1,1,0`).
  Teams 0/2, receiver HP 180, and the carrier's surviving `+0xCC`
  peer distinct from the recipient carry over from the prefix.
- Fresh scratch status/ledger baselines are read and checked, not
  rewritten: ordinary tags, zero AI-kind/status/guard selectors
  and flags, no linked receiver, zero credited damage/reaction
  ledgers and carrier bookkeeping byte; the fixture lists the
  exact offsets and widths in `STATUS_ZERO`. The only fork
  override is receiver `+0x5A0 = 4` to mask reaction class 2
  (ordinary fork 0) after the same accepted contact.
- Settings byte 50 and indexed difficulty bytes are 0; Scene
  combat-hold byte 31 remains clear. Both supplied team current
  and reference totals are 100 (teams 0/2), with HP snapshots
  500/180 (slots 0/2). The global reaction byte and the single
  mapped team-zero tally start at zero. These are match operands
  supplied at the boundary, not observer receipts from that scene.

The exact mode-2 reaction angle path reads coincident endpoints
and falls back to the observed `(0,0,80)` velocity. The bounded
math seams supply only zero length and cardinal `atan2` results;
the original angle conversion, curve write, flags, ledgers,
amount and HP code execute. The adapter additionally bounds one
indexed team halfword store and three signed divisions on
declared positive operands. All reached indexed float-table
addresses are pinned and checked before original loads; the
actor, stack and Scene spans stop on unexpected access. No
amount or HP result is injected.

Only the ordinary surviving path and the masked amount-zero
path are proposed. The latter should keep HP 180 and damage
ledgers unchanged, but the HP routine can still write previous
HP; it is not a rejected contact or a no-write claim. The exact
cut is **`8002F1DC`**, after the shared HP call returns and before
any remaining reaction tail, KO, effect, or scheduled follow-up.
The unpublished receipt path is
`build/borg-ordered-audit/retail-0600-rifle-outcome-v1.json`.
Syntax, pins, declared-operand control flow and ranges passed
static checks. No replay or compiler ran; no result, native
damage constant, or retail admission follows.

**Reserved outcome-v1 result:** the accepted receiver-v2 contact
was rebuilt and compared before continuing in the same source
frame. Both forks entered reaction class 2, set receiver flags
to 1, and retained the original carrier backlink and token
history. Under the declared ordinary match state, the original
code assigned curve `0x802C3494`, evaluated amount **8**, and
wrote HP **180 → 172**, previous HP 180 and damage ledgers **8/8**.
With class-2 binding masked, the curve remained unset; original
amount returned **0**, HP stayed **180**, damage ledgers stayed
zero, and the HP routine still wrote previous HP 180. That is
damage masking after accepted contact, not a contact refusal.
Both forks stopped at `8002F1DC` after the shared HP routine;
no subsequent reaction, KO or scheduled follow-up ran.

Actor status/phase, level data, team totals and HP snapshots,
difficulty/settings and scoped math remained supplied fixture
inputs. Amount 8 is not a general retail or native damage
constant. This is not a live battle or package admission.

The [recompiled rifle fixture](retail-code-atlas.md#machine-red-from-registered-rifle-contact-to-hp)
later reproduced the ordinary and masked outcomes with the
original installed input, VM cue, allocations, registry passes,
and HP writer. It uses teams 0/1 and different supplied poses with
null target pointers, and also checks a miss and a duplicate
token. Neither setup qualifies live hit timing or retirement.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-outcome-v1.json`
SHA-256: `c336d89bc4d157c9cf3fa3394b404d17ed53e8a7e6766631e97699c92c3d9270`.
One process, **0.337 s** wall, **5,635,004 KiB** available at launch,
**32,916 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. No retry, successor, compile, or admission.

**Reserved shape-prefix v2 result:** extending the geometry span
by the pinned `blr` at `8003A5E0` let the initializer return.
The same bound descriptor-15 sphere reached local phase 1 with
center **`(74, 11, 280.3999938964844)`** and radius
**`32.467533111572266`**. The fixture directly checked the endpoint
at shape `+0x98` against that center. The other endpoint at `+0x8C`
has a matching source store sequence but was not separately read
back in this replay. The stopped-carrier control again requested
unlink for the same shape before `80088E50`, with shape bytes and
stock unchanged. The positive visit stopped at `8008A7BC` before
descriptor-gated activation, with no registry or contact claim.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-shape-prefix-v2.json`
SHA-256: `7c79a368cc2b2596623c0b1df40099883d2f7ab338a7815398d62da787119d5b`.
One process, **0.202 s** wall, **5,769,728 KiB** available at launch,
**31,240 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. v1 remains unchanged. No retry, successor,
compile or admission.

#### Prepared singleton attack-registry witness (not run)

`build/borg-ordered-audit/retail-0600-rifle-registry-v1.py` is one
bounded source fixture with an unused `retail-0600-rifle-registry-v1.json`
output. The Dragon harness separates pool insertion from shape
publication; the G-Red harness separates carrier dispatch, late
registration and the shape sweep. This fixture uses those boundary
patterns only. It retains Machine Red `0600/0`, type-2/key-21, and
AFS 575 group-0 descriptor 15. Prior receipts are comparisons,
not restored world state.

The fixture supplies two **free** cells, initial free counts and
empty live lists, one active roster slot, clear Scene inhibit bytes,
an empty attack registry and a starting credited serial. Small
array extents fail closed beyond this singleton. The original
allocator and free-pool pop (`80088AA0 → 80088D98`) must write the
slot-zero buckets 9 and 49, their tags and links; neither live-list
pointer nor attack entry is seeded. The exact pool clearing leaf
is bounded host zeroing. Render-descriptor capacity and its null
allocation result are fixture inputs, and presentation-only
effect, art and sound calls are omitted at the three pinned sites.
The supplied pose and scoped geometry math retain launch-v2's limits.

The original `8008997C` carrier pass must select the inserted
carrier and run its initializer, then execute the original late
insert (`80089100`) and dispatcher post-call before the slot loop
advances. The original `80089AEC` shape pass must select that
carrier's bound shape from bucket 49. It then tests shape-prefix v2
geometry on the same record and looks for the actual publication
store `8008A478` to `803C477C`, with count 1. The negative fork
sets the carrier's `+0x272` attack-suppress bit on the fixture
snapshot: the same shape should still be dispatched and initialized,
but the count and entry must remain zero. Both cuts stop at
`80089B94`, before the slot loop advances or a contact consumer
runs. Unexpected dependencies stop the witness; no substitute
list insertion or dispatch is supplied.

Static syntax, pins, source callsites and descriptor checks passed.
No source replay or compiler ran. The nearest **observed** boundary
remains shape-prefix v2 at `8008A7BC`; the singleton schedule and
registry above are proposed checks, not results or retail admission.

**Reserved registry-v1 result:** the source debited B **2 → 1**
with refill clock zero, popped the class-3 carrier from the supplied
singleton free pool, and inserted it into original bucket **9**.
The original carrier dispatcher selected that cell and reached the
first-bind class-4 allocation. The class-4 pop reached its clear
leaf (`80088E24 → 8000540C`) but the fixture still mapped only
`0x100` bytes for the shape. The pinned class-4 pool clear is **448**
bytes (`0x1C0`), so the bounded zero adapter refused at the mapping
edge. This is a fixture extent failure, not source allocation
refusal. Class-4 insertion, the shape visit, suppression and attack
registry writes did not run. The late carrier insert and contact
were not reached either. The fixture's inherited “unregistered
carrier” string is not a live-list readback; the explicit original
singleton insertion probe supplies the narrower bucket-9 receipt.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-registry-v1.json`
SHA-256: `33705e3331df038e078b112c73e342c8a69898a14892e8b833ab329330272ecb`.
One process, **0.204 s** wall, **5,769,472 KiB** available at launch,
**31,832 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. Prior receipts are preserved. No retry,
successor, compile or admission.

**Reserved registry-v2 result:** source pool pops and insertion
completed for the same class-3 carrier (`0x120000`, bucket 9) and
class-4 shape (`0x121000`, bucket 49). Group-0 bind and the carrier
matrix matched the earlier scoped receipts. The original carrier
late-list insert and dispatcher post-call reached `80089A70`.
In the supplied suppression fork the original shape dispatcher
reached the class-4 activation tail and called `8008A5D0` from
`8008A830`. The evaluator refused the callee entry, which is
excluded by the fixture's half-open text span. This is a harness
stop, not an observed suppression or activation result: the
token/serial helper and attack-registry publication did not run,
and the eligible fork was not attempted. No contact was reached.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-registry-v2.json`
SHA-256: `3e18f227b58af93c9f13ccfeab9d326bcb0c2a5caa74295d93cfb404a3b636ee`.
One process, **0.232 s** wall, **5,760,456 KiB** available at launch,
**32,116 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. No retry, successor, compile or admission.

#### Registry-v3 static activation audit (prepared, not run)

The scoped descriptor-15 tail is now split into pinned instruction
ranges, rather than extending a catch-all range past `8008A5D0`.
The static control-flow walk covered both declared branches from
the original class-4 sweep to the exact pre-contact cut `80089B94`.
The two alternatives differ at the carrier `+0x272` bit-0 branch;
the following other-shape, degenerate-matrix, target and registry
alternatives remain out of scope:

| Reached source boundary | Pinned input and bounded writes |
| --- | --- |
| `80089AEC`, `8008A65C`, indirect `8003A4B8 → 8020B7A8` | Existing singleton slot-zero list, active class-4 cell and its own callback; collision row 15, flags 1, sphere kind 0; original center/radius/endpoints in that cell. |
| `8008A7BC → 8008A830 → 8008A5D0` | Shape group-0 hit-table row, ordinary credit, token 0, serial 17, null carrier target. Both forks are expected to write shape `+0x11/+0x4E`, carrier latch `+0x1CA`, token `+0x26C`, clear byte `+0x2AD`, increment the global token and credit serial to 1/18, then return on the null-target branch. |
| `8008A84C → 8008AA40 → 8020BB14` (three calls) | The original matrix guard reads the three columns of the bound carrier matrix. The existing axis-only length adapter checks them before returning lengths; original comparisons use pinned epsilon 0.01. Invalid columns are not repaired. |
| `8008A85C → 8008A2BC` | Ordinary credit activity scalar 0, active tagged carrier, shape flags exactly 1. The eligible branch alone should increment `r13-21328` and write the same shape pointer at `803C477C`; suppression bypasses both stores. No other registry slot is mapped for that write. |
| `8008A860 → 8008AA20 → 80089B94` | Original callback epilogue and singleton next link, before advancing the roster or visiting a contact consumer. |

The new private `build/borg-ordered-audit/retail-0600-rifle-registry-v3.py`
pins registry-v1/v2 and their receipts, checks token and matrix
guard returns separately, and adds no writable data span. It uses
the same supplied Scene context, pool cells, rigid poses and narrow
helper seams as v2. The unused output is
`build/borg-ordered-audit/retail-0600-rifle-registry-v3.json`.
Syntax, pins, branch coverage and mapped operands were checked
statically only. No activation or candidate publication is claimed
until a fresh authorized replay; no engine or package change.

**Reserved registry-v3 result:** the class-3 and class-4 pool
inserts, group-0 bind, carrier matrix and late carrier-list insert
matched v2. The suppressed shape visit returned from the sphere
helper and reached `8008A7BC`, then the fixture rejected its
three-way zero assertion at `shape_sweep` line 358 (carrier `+0xCC`,
credit tag and global token). The receipt did not log those
three values. Static readback explains the likely mismatch:
`800C3C7C..800C3C80` copies the caster's supplied peer pointer
to carrier `+0xCC`, whereas the launch path clears `+0xC8`,
not `+0xCC`. The null-target assumption in the audit above
therefore does not cover this fixture's token-helper branch.
No token or serial result, suppression outcome, attack entry,
eligible fork or contact was reached.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-registry-v3.json`
SHA-256: `752d6e399f580100801c650b285248abaa2c2a1fb43ce9e81b46f81bc9ff84eb`.
One process, **0.227 s** wall, **5,719,692 KiB** available at launch,
**32,536 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. Prior receipts are unchanged. No retry,
successor, compile or admission.

#### Registry-v4 static audit: carry `+0xCC`, do not clear it

In this source slice, actor `+0xCC` names the target/peer used by
aim validation (`80066620`) and copied by the constructor into the
carrier's related-actor field. Carrier `+0xC8` is the launch
position/geometry reference read by `80083E84` (the nonnull branch
reads its `+0x64/+0x68/+0x6C` point). Both words are copied
independently. This fixture has the existing supplied peer in
`+0xCC` and no launch geometry reference in `+0xC8`; clearing
`+0xC8` does not clear `+0xCC`.

With descriptor 15, token 0 and ordinary credit, `8008A5D0`
should write token 1 and increment credited serial 17 → 18. It
then reads carrier `+0xCC`, finds the existing peer, and reads
its supplied nonzero `+0x83` tag. The expected return is
`8008A630`; there is no team read or peer tally write on that
branch. The ordinary-peer branch after `8008A630` would require a
different actor fixture and remains unmapped, not forced to pass.
The expected matrix guard, suppression fork, eligible registry
store and `80089B94` stop otherwise remain as in v3.

Private `build/borg-ordered-audit/retail-0600-rifle-registry-v4.py`
pins the carried-peer read and return (12 extra text bytes, no new
writable mapping). It logs both carrier pointers, the peer and
credit tags, the token/serial and actual peer accesses, plus
matrix-guard and registry operands before checking them. Neither
carrier pointer is altered to satisfy an assertion. The unused
output is `build/borg-ordered-audit/retail-0600-rifle-registry-v4.json`.
Static syntax, pins, data bounds and both branch paths to the
pre-contact cut passed. No replay or compiler ran; no activation,
registry result or admission is claimed.

**Reserved registry-v4 result:** both forks completed at `80089B94`.
The carrier retained `+0xC8=0` and `+0xCC=0x110000`. The original
token helper read only the supplied peer's `+0x83=1`, made no peer
writes, and returned at `8008A630`. Both forks set token 1, advanced
credited serial 17 → 18, passed the matrix guard, and initialized the
same shape (`0x121000`) with descriptor 15 and both endpoints at
`(74, 11, 280.3999938964844)`, radius `32.467533111572266`.
The suppressed branch left registry count and slot zero. The
eligible branch wrote that same shape pointer at `803C477C`
through `8008A478` and raised the count to 1. No contact consumer
or later roster slot ran.

The active slot, supplied free cells and initially empty lists,
tagged peer, rigid poses, presentation omissions and narrow math
seams remain fixture context. Original code performed the pool
insertions, dispatch, activation and publication. This is a
singleton source result, not a live retail actor or hit receipt.

The [later recompiled bucket fixture](retail-code-atlas.md#machine-red-from-paid-carrier-to-attack-registry)
joins the installed B row and VM cue to those original allocation
and publication boundaries. It uses different supplied rigid poses
and a null target, and independently checks slot inhibition, shape
pool refusal and attack suppression. It does not replace the earlier
receiver or HP comparisons or admit the move for retail gameplay.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-registry-v4.json`
SHA-256: `8725a07aa3c0ac6a60764d94ac5a8d674c5d44bf18e1fe30ce058341fc972cdd`.
One process, **0.227 s** wall, **5,642,184 KiB** available at launch,
**32,564 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. Prior receipts are unchanged. No retry,
successor, compile or admission.

#### Prepared Machine Red receiver comparison (not run)

The private `build/borg-ordered-audit/retail-0600-rifle-receiver-v1.py`
rebuilds registry-v4's original carrier/shape insertion, dispatch
and publication, then compares the eligible result to that receipt.
For the receiver, the fixture pins the exact `000A/0` AFS 495
collision bank, body-9 descriptor and its hit-table row against
the independent Sasuke body observations. It supplies a separate
ordinary owner/header, distinct team, HP 180 and rigid key-2 pose.
A direct original class-4 visit, not a receiver scheduler or
initializer, must write the receiver/support registry entries.
No receiver or attack pointer is inserted into a registry by hand.

Three forks start from the same rebuilt attack: a receiver **256
units away on X** (broad-phase miss), a coincident receiver with
its first history slot explicitly seeded with attack token **1**
(duplicate prefilter), and coincident spheres with clear history
(positive source geometry and reaction handoff). Coincidence is a
supplied placement, not a promised hit: the original pair filters,
overlap test and accepted prefix decide the outcome. The pair
fixture uses a separately supplied scratch stack, not the
uncompleted Scene coordinator; only cardinal unit/zero lengths
are supplied at their reviewed callsites.

The pinned pair spans include the sphere-only indirect helpers,
two 360-byte copies within the 448-byte cells, 48 history slots,
and the accepted token/recipient writes. Both misses must return
without recipient writes; the positive stops **before `8002E660`**
after the original recipient/credit write and token-history update.
All cases require HP unchanged. An amount/HP continuation is not
admitted here: it would need a separately qualified post-reaction
state and amount/HP owner context, rather than treating this
handoff as a completed reaction.

Static syntax, source/receipt pins, mapped ranges and the three
declared pair paths were checked without running the evaluator.
The unused output is `build/borg-ordered-audit/retail-0600-rifle-receiver-v1.json`.
No comparison, replay, compiler, package or engine change ran.

**Reserved receiver-v1 result:** registry-v4 rebuilt its one attack
entry, and original class-4 code published the supplied Sasuke
body-9 receiver and support entries. The clear miss passed the
48-slot token scan, then failed broad overlap at the 256-unit
separation. The coincident duplicate control refused at the
first history slot before broad phase. Neither wrote a recipient
or token cursor; HP remained 180 in both completed controls.

The fresh-history coincident fork reached the broad-phase accepted
branch and entered the first 360-byte scratch copy. The evaluator
refused `8002DD1C` (`stwu r0,8(r5)`): the inherited scalar store
decoder only admits the stack-pointer form of `stwu`. Its asserted
failure is not source contact refusal. Narrow overlap, accepted
recipient/token writes, reaction entry and a positive-fork HP
readback did not occur. This receipt does not qualify a hit.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-receiver-v1.json`
SHA-256: `9ee570319774c06adf70d7d3872db5f9e24a12a08a50d6dfb2ab364b0cd7cff8`.
One process, **0.278 s** wall, **5,552,664 KiB** available at launch,
**33,524 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. Prior receipt pins are unchanged; no retry,
successor, compile, or admission.

#### Receiver-v2 static copy audit (prepared, not run)

The four fixed-size update-address sites also reviewed by the
Dragon `PairSource` are reached on Machine Red's sphere/sphere path:
`8002DD14/DD40` are `0x84040008` (`lwzu r0,8(r4)`), and
`8002DD1C/DD48` are `0x94050008` (`stwu r0,8(r5)`). The
proposed adapter executes only those words, verifies the live
attack and receiver record registers, biased pointers and CTR
for each of 45 iterations, and stays inside the two 448-byte
shape records and two 360-byte scratch buffers. The original
non-updating stores and loop branches still execute. At the
positive cut it checks both complete scratch copies against
their original shape words; it never supplies copied geometry.

`8002DE24` (`0x94A30720`) is pinned but intentionally not admitted.
That publication belongs to the deferred pair-selection branch.
For this sphere the pair routine takes the immediate acceptance
route, and the exact coincident narrow helper `80030500` writes
the point through `80030578/7C/84`. The verified static path to
`8002E660` then includes the original recipient/credit writes
and token history update, not the reaction or HP write. The two
miss controls must match v1 and perform zero copy updates.
HP must remain 180 in every fork.

The private `build/borg-ordered-audit/retail-0600-rifle-receiver-v2.py`
uses a fresh `retail-0600-rifle-receiver-v2.json` output and pins
the prior script and receipt. Syntax, source words, copy bounds
and positive control flow passed static checks only. No replay,
compiler, package or engine change ran.

**Reserved receiver-v2 result:** the clear miss and duplicate
prefilter matched v1 and did not enter either copy loop. The
coincident clear-history fork performed 45 updates at each of
the four pinned sites, and both 360-byte scratch copies matched
their original shape bytes. The original sphere overlap accepted
and wrote point `(74, 11, 280.3999938964844)`. The carrier and
credited owner named receiver `0x130000`, carrier contact bits
became `0x10`, and receiver history recorded token 1 with cursor 1.
The cut was **`8002E660` before reaction**; HP was 180 in all
three forks. No actual reaction or damage comparison ran.

Original insertion, publication and pair logic were rebuilt
against the prior pins. The separate ordinary body-9 receiver
header, pose, starting HP/history, scratch stack and scoped math
leaves remained fixture inputs, not a live roster or actor update.

Exclusive `build/borg-ordered-audit/retail-0600-rifle-receiver-v2.json`
SHA-256: `e91176310fb54577760c5ad8fbf7ef76aef1e983de1e72ef210a448fe0e49eba`.
One process, **0.296 s** wall, **5,684,524 KiB** available at launch,
**33,860 KiB** peak RSS, **1 GiB** soft/hard address-space cap and
**58 s** outer timeout. No retry, successor, compile or admission.
