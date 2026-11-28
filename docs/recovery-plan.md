# Gotcha Force native port: recovery and delivery plan

**Baseline:** `08cd0f5`, GG4E08 (NTSC-U), Linux reference host. This is the
current work plan. The [retail atlas](retail-code-atlas.md) and older recovery
notes are evidence and history; their original “next steps” are not a priority
queue. The [architecture](architecture-directions.md) and
[content contracts](portable-content-v1.md) govern shipped code.

## What we are recovering

Recover the original game’s **observable rules and the data that selects them**:
how an exact Borg and form move, select and execute attacks, interact with
other actors and the stage, spend and recover resources, enter and leave battle,
and affect match and campaign state. A faithful ruleset needs the order of
these events, not just the right damage number or animation.

The deliverable is a standalone native engine that runs independently authored
content and a separately supplied GG4E08 content set through the same portable
APIs. Keep the playable authored Battle Tests working while introducing exact,
explicitly scoped compatibility features. Modern controls, presentation, and
community content are separate goals; they need not reproduce GameCube SDK
internals. A matching decompilation, original address layout, opening sequence,
online play, and hardware/pixel equivalence are not prerequisites for the next
playable milestone.

## Current project state

| Area | Working or verified at this baseline | What is not established |
| --- | --- | --- |
| **Desktop and tools** | The Linux C++20/SDL/OpenGL host uses shared model, material, animation, input and audio systems. The standalone G-Red/Sasuke MVP and package Battle Tests are playable. The latter accept up to six fighters and one or two locals, including authored Workshop presets. Offline disc, AFS, PZZ and HSD tooling is bounded and read-only on inputs. | A complete public disc-to-installed-content workflow, a full retail battle, physical-controller and audible-output qualification for the recovered routes, or support on other platforms. |
| **Borg catalogue and packages** | All **206 normal Borgs** have authored-playable Battle Test packages (documented install v62, **222 combat selections**). The input-to-payload sweep is a native smoke, not a complete moveset test. Exact normal assets, stats, indexed animations and **1,030** alternate wrapper candidates are extracted. [The Borg API map](borg-api-map.md) records authored fields and has **zero** whole-Borg `retail_behavior_verified` flags. | Alternate looks, levels, exact variant admission, temporary forms, and fusion are different questions. Previewable art is not an actor, a collectible, or a playable variant. |
| **Original input, movement and action** | The [atlas](retail-code-atlas.md#what-it-recovers) indexes **13,303 functions** and **4,142 static selector rows**. All 206 normal installers select **158 profiles** in the bounded standalone inlet. There are scoped input/charge and VM replays, and a deep opt-in normal G-Red movement lab. | Static rows are not reachable-move counts. Most per-family input priorities, action exits, locomotion bindings and resource/contact semantics have not been joined into a running native ruleset. |
| **Original combat and match lifecycle** | Bounded receipts cover all normal body queues, initialized stock at all ten levels, many stock and carrier lifetimes, ordinary/alternate hit paths, damage and receiver factors, reaction clocks, KO cost and replacement. Two loaded actors share the tested readiness barrier; six seats have separate readiness controls. Native play has authored contact and opt-in recovered operands or behaviors, e.g. [Machine Red’s rifle continuation](retail-code-atlas.md#machine-red-one-carrier-two-receivers). | Bounded contacts, constructor prefixes and scoped native options do not establish a full input-to-exit move, live release-pose sweeps, loaded-body teardown, inactive-seat reuse, or uninterrupted normal match scheduling. |
| **Stage and support** | Real `st42` static walls and shared roster wall/reference behavior have original receipts. All **61** side archives / **360** placements were replayed through initial publication. The fixtures exercised **181** support cells, **172** timed recoveries and **8** terminal owners. The latest [scheduled comparison](retail-code-atlas.md#scheduled-side-collision-old-support-survives-geometry-catch-up) distinguishes pre-world carry, stale geometry, and retained versus fresh contact on one plane and one box. | Those cuts do not establish a full normal actor/world/collision visit in a loaded arena or authorize moving props in the portable stage contract. Most break triggers were declared zero HP; the Tau-to-prop join is one exact attack. The native Workshop wall is an authored partition, not the retail stage solver. |
| **Modes, saves, presentation** | Portable stages, roster presets and a shared duel request path exist. Story and Challenge have hosted, **read-only duel** slices; Story can progress in memory. Collection/Force editor, save transactions, and portions of retail scene bootstrap and presentation exist independently. | Sequential or reinforced Forces, retail Story/Challenge data and campaign rewards, a complete native front door, and complete menu/HUD/localization/audio integration. Original cue requests, effect allocations and draw-list entries do not establish on-screen output or audible playback. Networking has framing infrastructure, not multiplayer simulation. |

The latest changes to `engine/` or `platform/` at this baseline were at
`f3078e2` (the scoped Ninja X Workshop opt-in). Later evidence checkpoints
should not be read as changes to the playable ruleset. The authored
[mixed Workshop audit](retail-roster-extraction.md#mixed-workshop-roster-check)
admitted all 206; its latest documented 240-tick snapshot had
**191 starts**, **183 damaging hitters**, and **five interrupted melee
subjects with no payload**. These are matchup observations, not missing
packages or a target to maximize at the expense of correct interruptions.

## Method: evidence first, then portable integration

Keep two independent statuses for each feature of an exact route:

| Original evidence | Native implementation |
| --- | --- |
| **Unresolved / observation only**: player report, visual classification or plausible candidate. | **Absent / refused**, with the reason visible. |
| **Static**: exact member, descriptor, table row, initialized operand or dispatch candidate. | **Authored**: playable policy, including rules merely informed by static or executed evidence. |
| **Bounded execution**: original code runs from a documented inlet with positive and refusal controls, stopping at a named boundary. | **Bounded match**: portable feature and exact content binding reproduce the stated source observations in tests. |
| **Joined original execution**: original producers and consumers run together with qualified owners, scheduling and state for that case. | **Integrated match**: the same bounded behavior also passes through the actual native session on the supported inputs. Hardware, graphics and audio still have separate acceptance tests. |

These columns are not a one-to-one ladder. For example, a joined original
route can remain unimplemented, while an authored route can be fully playable.
Grade **features**, not whole Borgs. Keep separate
`revision / Borg / variant or form / input context / move / feature / scenario`
keys and link the best receipt, native test, remaining assumptions and
blocking question. Track counts of admitted packages, reviewed routes,
matched features and fully joined routes independently. Do not use static
candidate counts or the number of replays as conversion progress.

For each recovery ticket:

1. **State the decision.** Name the gameplay consequence, exact scope, current
   native behavior and smallest observation that would change the next
   implementation. Search the atlas and existing fixtures first. Reuse an
   existing witness if it answers the question; don't replay adjacent code for
   its own sake.
2. **Pin the source and inlet.** Establish revision and input digests,
   original entry and caller, owner lifetime and permitted writes. Distinguish
   guest-produced state from declared rig, pose, pad, timer, HP, pool or stage
   setup. Execution stays bounded and stops at unsupported calls; a missing
   hook is not a negative result. Keep executable, assets, reconstructed
   source, saves and raw/private traces out of Git.
3. **Compare causally relevant cases.** Trace request → admission → commit/debit
   → allocation → pose/shape publication → pair selection → HP/credit/effects
   → unlink/retire/refund. These are distinct events; some may not occur.
   Preserve separate team settings/Force pools and actor ranks/explicit banks
   (omitted banks follow team lookup). Keep raw power, requested amount, HP
   removed, positive credit and KO cost separate. Track the actual owner,
   receiver, hit token and allocation lifetime—not just a reused address,
   descriptor, key or aimed target. Compare hit/miss, duplicate/fresh token,
   capacity failure, interruption and relevant boundaries. A visit count is
   not a frame count until the caller's schedule is joined.
4. **Port only the proved contract.** Use `engine/gameplay/` for shared rules
   and capability validation, `platform/` for host/device work, and
   `tools/analyze/` and `tools/extract/` for retail identities and conversion.
   Add portable operands or a named capability only if needed. Prove it with
   authored data as well as the exact retail binding. No retail-ID gameplay
   branches, parallel renderer/controller stack, or new fallback that weakens
   a refusal. Keep the existing authored rules as a separate baseline.
5. **Close the loop.** A promoted feature needs a deterministic, asset-free
   receipt and replay instructions; native positive, negative and unchanged
   baseline tests; and a declared next stop. Compare normalized events rather
   than pointer values. Use O0/O1/O2 for source-boundary work where the modules
   exist; use native headless/unit and sanitizer tests for shared changes,
   then appropriate desktop/device tests for visible or audible claims.
   Publish only reviewed source, tests and metadata. Extraction remains
   read-only, bounded and refuses existing outputs. At most two compiler jobs.

## Work order and exit criteria

| Order | Work | Exit criterion |
| --- | --- | --- |
| **0. Establish the conversion baseline** | Start a small route ledger from the Borg API map, installer and receipt indexes. Record the authored defaults, missing joins and the actual native owner of each first candidate. Capture the 206-package input-to-payload sweep separately from the mixed Workshop matrix. Document a reproducible local import recipe and identify the remaining private-profile dependencies; don't block rule work on converting every historical lab profile. | We can compare authored and compatibility runs against unchanged, named baselines and tell **which feature** changed. A refused route, a late start, an interruption, a miss and a tool stop are different results. |
| **1. Close one shared combat seam in the running native battle** | Start at published shape → accepted contact → HP/credit → retirement. Use the [three ordinary type-0 contact keys](retail-code-atlas.md#type-0-contact-hp-and-retirement) as one shared family and the exact [Machine Red continuation](retail-code-atlas.md#machine-red-one-carrier-two-receivers) as a contrast: its carrier continues after an accepted hit and can hit a second recipient. Reuse the shared targeting, damage and projectile systems. Follow with one existing ordinary melee route, comparing an uninterrupted start against interruption where supported by evidence. Then look upstream only for missing input/pose/debit joins needed to admit the route. | Controlled native traces agree on miss, fresh and duplicate tokens, exact recipient, HP/credit, closure and reuse. Both stop-on-hit and continued-contact policies are explicit. The authored Workshop behavior and unrelated routes don't silently change. A factory-supplied key remains a factory-scoped match, not a verified original button-to-hit move. |
| **2. Qualify shared movement across the roster** | Start with separate normal G-Red meter and Sasuke air-jump witnesses, then one verified representative each for fixed airborne, hover-floor contact, and perpetual-flight boost. Trace input priority, entry/exit, velocity/heading, floor/support state and resource spending/recovery. Automatically index clips and use existing labels/player review as cross-checks only. Keep camera-relative commands, body heading and temporary upper-body aim separate. Expand by exact branch and operand identity rather than by appearance, tribe or a copied G-Red program. | Each admitted family has an entry → airborne/travel → exit/refill trace and a rejection control, a portable binding, and a running native test against a real moving opponent where supported. Hover-floor reset is not perpetual-flight recovery; supported floor and permitted refills are independently recorded. Publish coverage gaps by Borg, not as universal defaults. |
| **3. Join normal battle scheduling, stages and match lifetime** | Use the [loaded Sasuke/Time Bomber inlet](retail-code-atlas.md#dual-scene-entry-two-loaded-actors-share-the-readiness-barrier) and known normal post-world/collision cuts. Aim for consecutive normal visits with an actual moving actor pose, shape republish and accepted/missed contact, then a KO → unlink → archive handoff → ready replacement or end. Resolve the next narrow missing caller or model-refresh input, not all scene setup or rendering. For moving stages, require a normal join across actor-before-world, publication, contact/anchor refresh and removal before binding carry. | A scoped original sequence and native counterpart preserve order across adjacent visits and release/reuse. The native stage/match capability has an authored control and refuses unsupported transitions. Current static stages remain usable. No hardware, full arena, or campaign claim from this slice. |
| **4. Expand exact combat/move and form coverage** | Batch by shared dispatch/factory and failure behavior. Promote other projectile/beam/area, guard, heal, tether, return and reaction profiles only when each binding's exact context is known. Resolve unusual level/variant/form ownership and paired fusion only after the ordinary actor, effect, stock and replacement boundaries are stable. Track each alternate look's art, eligibility and gameplay separately. | A published supported-route manifest lists the tested contexts and outcomes, with refusals for every unbound context in the counted scope. Forms and fusion prove who owns body, resources, effects and control during entry, interruption, return and KO. |
| **5. Ship a complete playable loop for a declared content set** | Use the existing portable battle request and result path to extend sequential Forces/AI, then Challenge and Story content, progress and rewards. Add menu/HUD, fonts/strings, opening and input/audio polish through shared services. Validate result provenance before engine-owned save transactions; stale/edited/unsupported copies, cancel, draw or failed admission must not produce rewards. Keep scripts bounded and capability-based. | A clean, asset-free native build plays authored content; a fresh read-only local import yields the declared retail content set. Players can select a Force, run battles, resolve outcomes and return to the host without diagnostic paths. Optional hardware/display/audio checks have separate results; broaden release claims only to what those checks cover. |

Steps 1 and 2 can share fixture data but should land in small independent
changes. Stage or scene evidence can proceed sooner if it blocks one of those
features. Resuming broad bootstrap, graphics or per-Borg SDK archaeology is
not a dependency of an ordinary combat/movement checkpoint.

### Immediate queue

- **Landed:** the [normal-contact trace comparison](projectile-contact-checkpoint.md)
  separates immediate contact closure from slot return. It retains authored
  defaults, checks the rifle continuation and absent-emission controls, and
  binds the new opt-in only on the three reviewed normal type-0 B routes.
  This does not close the retail geometry, amount or release-timing gaps.
- **Landed:** the [Tau interruption comparison](melee-interruption-checkpoint.md)
  shows original allocation before publication: a pre-publication hit
  cancels the attack and its exact cell returns. Native authored melee also
  cancels before payload, but it never allocated a window at entry. The
  refreshed Workshop audit ran all 206 on authored movement and found four
  pre-payload melee interruptions and one after Roach's emission.
- **Next:** start with Sasuke's jump-point spend, air dash and grounded reset.
  Compare accepted, refused and refill visits with native movement before
  adjusting the package. Keep air-dash admission, jump points, and landing
  distinct. Then take the next shared movement family from step 2.
- **Hold until a decision depends on it:** another side-prop census; the
  absent-support terrain fallback past the unresolved external read;
  live group 26 (direct bind is verified, no proved trigger);
  type-25/type-80 accepted-contact retraction; Worm tether/poison tails;
  type-52 socket coverage; Thunder Wire linked contact; mixed-power hits,
  mixed receiver audio bands and unjoined effects; the three outer
  collision-reset callers; loaded rig teardown and inactive-seat reuse.
  Promote one of these when it blocks a shared feature and there is a
  discriminating witness. Keep the negative result and refusal otherwise.

The next milestone is **a playable, evidence-matched shared battle slice**,
not complete retail admission. If a replay cannot settle a branch safely,
record the open boundary and switch to the next high-coverage decision rather
than supplying the missing state and treating the result as live behavior.

## Review checks

- Python suite with local GG4E08 inputs: **1,054 passed, 129 skipped**.
- Fresh desktop host build via the direct C++20 runner (CMake unavailable in
  the review environment); asset-free three-frame headless launch passed.
  The authored one-local Workshop and the portable Story host smokes passed
  headless, including the Story refusal/no-save-write checks.
- Baseline source checkpoint: ten scheduled side-contact probes and two
  normal prefix cuts matched across O0/O1/O2. This review did not re-run the
  full source replay catalog, the full C++ suite, or device tests.

Baseline setup and commands are linked from the [README](../README.md#build)
and the [atlas reproduction notes](retail-code-atlas.md#generate-locally).
