# Borg attack pipeline recovery

Where a contact comes from, traced from the input down to the data, and what
remains undecoded. This is a recovery record, not engine content.

## Where damage is not

Damage was searched for in the two projectile row tables and is in neither. The
56-byte child rows at `802D7B10` for `0f01` key 74 carry `51.8, 40.0, 100.0,
0.75, 15.0`, and the 80-byte rows at `802D9758` for `0f04` key 23 carry `-35,
100, 60, 0.3, 7.5, 25`: speed, lifetime, scale and timing, with no `8` among
them. It cannot be derived from the attack rating either — `0f01`'s ratings are
`[1,2,0,2]`, so its attack stat is zero while its authored damage is 8.

So damage lives in the program/cue path that the projectiles do not describe,
and recovering it means decoding that path rather than reading the row. Until
then every pilot's damage stays explicitly authored.

The static route to that path is now exhausted. The HP writer at `8003D358`
(the `HP - r4` site found during the Roach work) has no `bl`, no `b`, no 32-bit
pointer word in any data section, and no `lis`/`addi`/`ori` materialisation of
its address anywhere in either text section. The direct `+0x1C6` HP writes that
do exist are initialisation sites (`HP = +0x1C4`), such as `8004E318` and
`80081CFC`, not damage application.

The project does have a gated original-execution harness: receipts such as
`build/borg-ordered-audit/retail-0600-rifle-consumer-v3.json` record forked
observations under a 1 GiB address-space cap, an outer timeout and exclusive
creation. That harness can execute a **named** span with supplied operands, and
that is exactly what this case lacks: the damage writer has no static caller, so
there is no root to enter from and no operand context to supply.

Settling the amount therefore needs one capability the harness does not have
today — a dynamic trace of a live battle that records the caller and `r4` at the
damage site for a known attacker and defender. Until that exists, authored
damage stays labelled rather than guessed.

## One dispatcher, one missing subsystem

Melee, shots and breath all reach the same entry. The close-B request that the
knight pilot records as `800FEEA4` is
`8004BEB8(actor, 15, group, choice, -1, -1)`, and `8004BEB8` is the shared
cue/program request: it masks the request against `actor+0x579`, writes
`actor+0x709`, and starts a program object from `actor+0x1D84` on the context at
`actor+0x1CD4`.

So the difference between a sword swing, a bullet and a breath is the program
contents, not a per-family mechanism. `tools/analyze/lane_descriptors.py` already
decodes clip lanes from those programs, but it exposes **no** shape, collision,
hit or damage event.

That is the single missing subsystem behind three separate gaps: the melee
geometry for 91 Borgs, exact damage for all of them, and the attack timing that
the authored window proxies stand in for. Porting the lane-event to shape/damage
path is the next real unit, and it starts at `8004BEB8` and the program at
`actor+0x1D84`.

One concrete obstacle found while trying to start it: `descriptor_lanes` decodes
a per-lane `event` and `flags` byte, but `program_candidate` reduces each lane to
`lane/group/choice` and the export never persists the event. The shared
`shared-action-program-controls.json` stores programs, routes and wait commands,
not descriptor lanes. So before the shape path can be classified, the export
needs a lane-event index — a per-Borg list of `(action group, lane, group,
choice, event, flags, bank)` — which does not exist yet. Attempting it as a
silent enrichment of `program_candidate` produced no observable output, so that
change was reverted rather than left as dead metadata.

The index now exists: `tools/analyze/retail_action_links.py` exposes
`lane_events(roots, mask, group, choice)` and the extractor writes
`shared-lane-events.json`, copied here as
[`lane-events.json`](api-map/lane-events.json). It covers 147 of 206 Borgs and
records, per resolved entry clip, each lane's `group`, `choice`, `event` and
`flags`.

Its first result is a **negative** one, and it redirects the work:

| Group | Most common `(event, flags)` |
| --- | --- |
| `B-close` | `(-1,1)` 311, `(1,0)` 227, `(1,255)` 116, `(9,1)` 48, `(6,1)` 30, `(5,1)` 25 |
| `B-shot` | `(-1,1)` 214, `(-1,4)` 96, `(0,0)` 95, `(-1,0)` 50, `(0,4)` 48, `(3,0)` 37 |
| `X` | `(-1,1)` 67, `(4,0)` 39, `(-1,5)` 26, `(0,0)` 19, `(8,1)` 16 |

The event values overlap heavily across groups, so `event` is a lane action type
(play clip, sound, and so on), **not** an attack class.

The lane **group** does separate the families cleanly:

| Group | Dominant lane target |
| --- | --- |
| `B-close` | local group `1` (582), then local `2` (64), `7` (43), `6` (32), `0` (27) |
| `B-shot` | local group `0` (284), then local `3` (134), `2` (121), `5` (36) |
| `X` | local group `4` (112), then local `2` (36) |
| `X-charge` | local group `4` (88) |

So melee plays local group `1`, shots groups `0`/`3`, and X group `4`. Those
groups are animation bank selectors, though: they choose which clip a lane
plays, not which collision shape an attack uses. That places the hit-shape
request inside the **program bytecode** at `actor+0x1D84`, not in the descriptor
lanes at all — which is the next thing to decode, and the reason this index was
worth building even though its first answer was negative.

## The melee contact opcode

Decoding the exported programs (`shared-action-program-controls.json`, 245
programs: 226 `terminal_wait`, 19 `yielding_loop`) and cross-tabbing opcodes by
action group isolates one:

| Group | `actor_event_10` | `class_event` | `directional_select` |
| --- | --- | --- | --- |
| `B-close` | **85** | 31 | 0 |
| `B-shot` | 1 | 0 | 32 |
| `X` | 13 | 11 | 6 |

`actor_event_10` is the melee contact request: 85 occurrences in close programs
against a single one in the shot programs, which use `directional_select`
instead.

Its encoding is `0a AA BB ff`, four bytes, where `AA` ranges over **0..7** and
`BB` is `00` or `7f`. The `AA` range matches a per-Borg attack slot rather than a
shape id, and `BB` behaves like a side or height flag.

The dispatch path is now traced. The interpreter `8004C050` selects handlers from
`802D0DA0 + opcode*4`; opcode 10's handler is `8004C6B4`, which calls
`8008AC80(actor, AA)` and also stores `AA` at `actor+0x744`. `8008AC80` is the
slot lookup:

```text
owner = *(actor + 0x90)
block = *(owner + 0x3FC)
entry = *(block + 0xA20 + slot*4)   // 8 pointers, slot 0..7
child = 80088AA0(actor, 4, 0x30, 0, 0)
```

So each contact resolves through a per-model array of eight pointers at
`block+0xA20`, and the pointed-to record initialises a 48-byte child.

Following the child initialisation shows what those records hold:

```text
byte  = *(slot_record + 0)          // first byte of the slot record
row   = block + 0xAA0 + byte*80     // 80-byte stride
child+0x28 = row
child+0x0C = 0x8009A65C             // contact update
child+0x14 = slot
child+0x20 = actor
```

The slot record's first byte indexes an **80-byte row**, and that is the same row
format the projectile pilots already read from the shared table at `802D9758`
(speed, lifetime, scale, timing). So melee contact and projectiles share one
attack-row format; they differ in which table and which row they select.

That makes the remaining work a single decode of the 80-byte row fields, plus
finding where damage lives in that row — and it applies to melee (91 Borgs) and
projectiles together rather than to each family separately.

The block itself is placed by `800544A0`:

```text
block = *(actor + 0x9C) + index*7840      // 0x1EA0 bytes per class block
*(owner + 0x3FC) = block
```

So `actor+0x9C` is an array of 7840-byte class-data blocks, selected by a signed
index and installed as `owner+0x3FC`. That is not the actor record — the
extractor's actor stride is 432 bytes — so these blocks are built or copied
separately, and finding where `actor+0x9C` is filled is the last link in the
chain. A second writer, `8003CC48`, installs a pointer from a small table at
`index*4+0x80`, which looks like the simpler class-selection path.

Summary of the remaining decode, in order: fill source for `actor+0x9C`, then
the eight pointers at `+0xA20`, then the 80-byte rows at `+0xAA0` including the
damage field. Everything above that is traced.

## The contact child is visual

One more check changes what binding those rows would buy. The child's update
`8009A65C` is a matrix and transform pass: it calls `8020B238` and `8020AF5C`
for vector maths and submits the child's model through `800076D0`/`800097B4`
using its render object at `+0xE0`. There is no collision or HP write in it.

So the opcode-10 child that carries the 80-byte row is the **swing effect**, not
the hit. The actual contact is resolved by the collision service against the
actor's own shapes, and its damage goes through the unreachable HP writer. That
matches the melee pilots' description of a body proxy: the geometry they stand
in for is real, but the amount is not in the visual path.

Consequently, recovering the `+0xAA0` rows would give the **effect** asset,
timing and scale for melee — genuinely useful for presentation — but it would
not by itself yield damage. Damage still needs the dynamic trace of the
collision receiver described earlier.
- `audio` — the landing candidate (bank, tone, whether the trigger is
  qualified, and whether the PCM file is present), the `other_sfx` state,
  whether a launch sound was authored, and how many action cues and routes
  the shared `shared-action-sfx.json` recovers for that Borg.
- `preview_declaration` — whether a preview is declared.
