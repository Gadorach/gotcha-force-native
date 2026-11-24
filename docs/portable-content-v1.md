# Portable content v1 contracts

These are implementation contracts, not current feature or retail-admission
claims. The existing `GFBORG 2` manifest, catalogue and `GFBORG-GAMEPLAY 1`
remain the Borg formats. Do not convert the legacy retail menu profiles into
mandatory inputs for custom content.

## Shared rules

Discover one directory below each kind's root: `borgs/`, `stages/`,
`challenges/`, `stories/`. A package directory is not an identity. Use
case-sensitive lowercase namespaced IDs with the existing Borg ID grammar.
References are typed by context; a stage and a Borg may have the same text
ID. Resolve a Borg as an **explicit** `(package ID, variant ID)` pair. Never
infer a variant from a title, directory position, or default selector when
resolving a saved or authored battle reference. Retail import is a separate
adapter; it supplies stable portable IDs, not runtime branches by model.

New manifests are UTF-8, with a version header on the first line, single
`key=value` rows, `|` separators in compound fields, and `#` comment lines.
Reject unknown keys, duplicate scalar/row IDs, malformed UTF-8, out-of-range
numbers, links and path traversal. No absolute paths, eager model decoding,
online discovery or fallback to a different asset or definition. Byte and
row bounds below are ceilings, not permission to allocate unbounded assets.
Follow existing content loading patterns: parse first, resolve and validate
named dependencies second, bind runtime capabilities third. A package can
be listed without being usable for a battle. Duplicate IDs refuse, including
across packaged retail and custom content. Diagnostics distinguish malformed,
missing, and unsupported. Pin accepted data for a session; replacing files
on disk is not an instruction to change a running session.

Only engine code prepares an encounter and validates the stage, typed
spawns, exact Borgs, current Force and edited-copy eligibility, rules and
permissions before publishing a battle. Story and Challenge both produce
a **portable battle request**: content ID and encounter ID, stage ID,
ordered player and enemy spawn marker IDs, roster source (explicit authored
entries or an engine-confirmed saved Force), AI/rule keys, and a tick limit.
The first running adapter supports one actor per side, existing duel AI
and defeat-opponent rules. It refuses unsupported roster sizes/capabilities
without truncating, substituting, consuming progress, or writing a save.
A battle result must belong to the accepted request and session. Browsing,
previewing and practice never produce campaign completion.

## Stages

`stages/<folder>/stage.manifest` (`GFSTAGE 1`, max 16 KiB):

```text
GFSTAGE 1
id=example:workshop
name=Workshop
arena-half=1800
model=art/stage.hsd
floor=collision/floor.stih
projectile-stage=collision/bounds.gfsr
secondary=collision/walls.stih
tertiary=collision/props.stih
view-fade=art/view-fade.txt
spawn=west|-240|0|300
spawn=east|240|0|-300
```

`model`, `floor`, `arena-half`, and at least two uniquely named `spawn`
rows are required; at most 32 spawns. Spawn fields are `id|x|floor query
y|z` in world units. The engine resolves support, spacing and initial
facing, not the manifest. IDs use the bounded local variant-token grammar.
`projectile-stage`, `secondary`, and `tertiary` are optional as a group
only; use current GFSR/STIH validation and the shared stage renderer and
projectile segment query. `view-fade` is independently optional and visual
only. V1 uses the existing HSD stage record layout. A portable stage is
not an override for the pinned retail source-ground qualification or a
claim that moving props, hazards or general traversal work. Additional
capabilities should be typed declarations referencing engine-owned services.

The native Package Battle Test accepts an exact stage and ordered marker pair
via `--battle-stage ID --stage-spawns PLAYER ENEMY`, with explicit variants
for both Borgs. It does not combine a portable stage with legacy stage paths,
debug spawn distance, or G-Red movement practice. On Linux, `--list-stages`
reads the bounded manifests and checks the declared paths; admission pins
the selected bytes and validates the actual stage. `arena-half` and horizontal
spawn coordinates are bounded by the native arena domain (10,000 world
units), and floor-query Y is bounded to ±20,000. The selected spawn markers
must have support and leave room for both duel bodies.

For an authored example, generate content into a new local folder from
the repository root:

```sh
python3 -B -m tools.extract.demo_borgs local/workshop-example --swept-beam
python3 -B -m tools.extract.portable_stage_example local/workshop-example
cd local/workshop-example
/path/to/gf_native --battle-borg example:flight-stick --borg-variant normal \
  --against-borg example:jump-stick --against-variant normal \
  --battle-stage example:workshop --stage-spawns west east
```

## Challenge

`challenges/<folder>/challenge.manifest` (`GFCHALLENGE 1`, max 64 KiB):

```text
GFCHALLENGE 1
id=example:workshop-trials
name=Workshop Trials
encounter=round-one|example:workshop|west|east|duel-ai-v1|defeat-opponent|3600
enemy=round-one|example:jump-stick|normal
```

Maximum 64 encounters, in declared order; at most 30 ordered enemy rows
per encounter at the format boundary. An encounter names `id|stage
ID|player spawn|enemy spawn|AI|win rule|tick limit`. A zero tick limit
means untimed; otherwise cap at 216000 ticks. One or more `enemy` rows
per encounter are required. The launcher supplies an explicitly selected,
validated player Force; the UI cannot hand the enemy or a replacement copy
to the player. V1 has no implicit random encounters, AI profiles as code,
rewards, or save writes on selection. The engine advances the Challenge on
a matching win; loss/cancel/invalid result never advance it. Later variants
can add explicit deterministic selection tables (ordered choices + run
seed), typed rule/profile assets and enemy reinforcement without changing
the identity model. Don't claim Battle Test can play sequential Forces
until it can spawn and advance all the actual members.

On the Linux desktop host, use `--list-challenges` to see discovered IDs
and encounter IDs without admitting a battle. Copy the authored
`examples/challenges/workshop-trials` package to the generated example's
`challenges/` directory, then run from that content root:

```sh
/path/to/gf_native --run-challenge example:workshop-trials round-one \
  /path/to/engine-save-root ACCOUNT_ID SAVE_ID FORCE_NUMBER
```

`FORCE_NUMBER` is one-based. The current hosted slice requires a single
encounter, one eligible packaged copy in the selected Force, and one
exact authored enemy variant. It runs the existing native package battle
loop with duel AI. It never writes progression or rewards to the selected
save. `Q` cancels; a draw, tick-limit expiry, or early host exit does not
complete the encounter. For headless runs, `--frames N` controls when the
host exits, not whether the player wins.

## Story and its map

`stories/<folder>/story.manifest` (`GFSTORY 1`, max 128 KiB):

```text
GFSTORY 1
id=example:workshop-story
name=Workshop Story
start=arrival
map-art=art/map.tpl
node=arrival|text.arrival|240|370|example:workshop-trials|round-one
node=ending|text.ending|750|410|none|none
edge=arrival|win|ending
```

Nodes (`id|label key|x|y|challenge ID|encounter ID`) use integer normalized
map coordinates 0–1000, at most 128 nodes. Node and encounter IDs use the
bounded local variant-token grammar; label keys start with a lowercase
letter or digit and use up to 96 ASCII bytes of lowercase letters, digits,
periods, hyphens and underscores. An optional package-local map image is
art, not a baked title. `none|none` is a terminal presentation node, not a
battle. Story references a Challenge encounter without running its
progression or embedding its battle data. Use at most 256 edges
(`from|win-or-loss|to`), unique by source/outcome; require valid endpoints
and reachability from `start`. A missing edge leaves the current node
selected. Cycles across plays are allowed, but an accepted result advances
at most one edge. Encounter-less nodes have no result edges. Browsing the
map or previewing a node cannot change progress.

The parser validates graph structure; binding resolves every encounter by
exact Challenge and encounter ID against a pinned Challenge catalogue.
Story uses the shared encounter request builder and battle session.
Prepare and accept recheck battle admission with the trusted engine
resolver; only a matching accepted campaign receipt can apply a result.
Cancellation invalidates the receipt and never moves the current node.
An accepted result consumes the receipt even when no edge matches.

The read-only map projection exposes authored-order nodes, label keys,
top-left `[0,1]` coordinates and selected/available status for the shared
renderer. The current node is selected; a selected battle node is available
to preflight when no battle is accepted. Availability is not battle
admission. The projection has no screen size or aspect ratio: when map
art is bound, fit it without changing its aspect and place the nodes
inside that fitted rectangle.

Linux desktop Story discovery scans one folder deep under `stories/`, with a
128 KiB manifest limit, 2,048 scanned entries and an 8 MiB total scan
budget. Linked files/directories and duplicate IDs are refused. Discovery
pins only graph data; it does not require a Challenge, a saved Force or
an image for a graph to be inspectable. The standalone read-only map
opens by exact Story ID and starts at the authored start node, not a
saved campaign position. Arrow keys/D-pad or a click inspect other nodes
without activating them; Esc or controller B closes the map. The inspect focus
survives resizing by node ID, independently of campaign selection.
The repository includes a retail-free example in `examples/stories/`.
Run `gf_native --list-stories` to list the current content root, or
`gf_native --story-map example:workshop-story` from `examples/` (using the
absolute executable path). The same map can be exercised without a window
using `--headless --frames 2`.

Optional map art is loaded only for the opened Story: one package-local
TPL image, at most 8 MiB on disk and within the shared decoder's
4,096-pixel dimension bound. Absent or refused art leaves the same graph
on a neutral fitted map field, not another package's art. The layout
draws and hit-tests in drawable pixels while converting logical window
pointer input; narrow windows stack the inspector below the map.
Current labels use the shared compatibility bitmap text and mark
unlocalized labels with `KEY:`; dynamic fonts and localization binding
are not implemented. The standalone `--story-map` remains inspection-only.

The Story host handoff can pin an explicit account/save/Force selection
and admit the current node through the shared portable duel adapter.
The adapter prepares the exact named encounter, revalidates the Force,
and returns the duel report to that same in-memory Story run. A win or
loss may take its corresponding edge; draw, timeout, cancellation,
invalid reports and stale saves never advance the graph. A new node
requires a new admission.

`gf_native --run-story STORY_ID SAVE_ROOT ACCOUNT SAVE FORCE` opens the
playable desktop map with explicit numeric account/save IDs and a
one-based Force number. Inspect with arrows/D-pad or click a node. Press
and release Enter or controller A on the **current** battle node to
launch it; other graph nodes are inspection-only. The shared duel
controls still apply, and Esc or Q/controller Y cancels a duel. On
return the map focuses the resulting current node. Map and duel use
separate, non-overlapping renderer scopes; a stage transition never
reuses a previous stage's GPU assets. `--headless` opens the map without
synthesizing a confirmation press. This mode does not make a save slot,
write campaign progress, or award rewards.

The graph runner is in-memory. Save progress is deferred; a future
engine-owned versioned record would use the Story ID and stable node/flag
IDs, with explicit migration for removed IDs. Rewards and unlocks require
separate engine authorization and are not granted by graph traversal.
Dialogue and map/camera sequences should be bounded typed tracks, not
source event addresses or executable plugins; those layers are deferred.

## Shared Borg library and Collection

No new Borg data file. Build one engine-owned library projection from
`BorgPackageCatalog` and validated `BorgCatalogSnapshot`s, with optional
ownership overlays. Its public row key is **package ID + variant ID**.
It includes display name, exact variant label, tribe, declared cost,
metadata/availability state and independent preview/gameplay-declared
flags. Rows with no parsed catalogue still report the package and reason,
but have no selectable variant; never synthesize one. A gameplay path or
declared capability is not admission. Project owned instances separately
by instance ID; do not group different saved variants or replace their
saved names. Preserve missing/disabled rows in the owned view.

Provide installed and owned scrollable views over the same metadata,
with a visible range and selection preserved by stable key when sorting,
filtering or refreshing. Decode icon/preview files lazily for the visible
or inspected row; absence is not a fatal list error. Reuse the installed
browser, preview and screen/text drawing paths instead of forking a renderer.
The full UI targets dynamic fonts, Unicode strings and numeric formatting,
with current browser text as a documented compatibility presentation.
Neither list opening nor row selection allocates an actor, grants a copy,
acquires a discovery flag, or writes a save. Use the same library rows in
the Force editor's browsing and cheat picker, not another Borg data file.

The desktop slice uses `--browse-borgs` and
`--browse-owned-borgs ROOT ACCOUNT SAVE`. It pins catalogue metadata,
tracks exact installed keys or saved instance IDs, and opens art only
as a read-only presentation. The current compatibility text path uses
uppercase ASCII, replacement marks, clipped labels and plain decimal
numbers; it does not rewrite the UTF-8 data. See
[Borg Collection controls](borg-packages.md#read-only-installed-borg-collection).
The [portable UI font](#portable-ui-font-v1) is the shared upgrade path.
The desktop read-only Collection accepts `--collection-font FONT_ROOT` as
an explicit opt-in. It opens `FONT_ROOT/font.manifest` and its declared atlas
without searching other roots. Omission or failed binding keeps the
compatibility bitmap. This presentation choice is not stored in a save
and does not affect body previews.

## Force editor and cheat picker

The UI view is a mixed projection of existing `PackageBorgRoster` and
retail inventory, not a new roster or fake retail model indices. Editable
positions retain stable Force/slot numbers; packaged placements reference
**owned instance IDs**, not rows or catalogue IDs. Costs come from exact
variants; missing cost or a conflict remains visible and blocks changes
whose validity depends on it. Budget and eligibility are engine checks
and may vary by mode. A known catalogue-only package may be inspectable;
it cannot be declared playable. Existing retail serialization, edited
Borg admission rules and recovery choices are unchanged.

First writable slice: arrange an already owned, eligible packaged copy
using the selected-save transaction path. An empty verified legacy slot
is not permission to overwrite it later. Stage a candidate, show exact
Force/slot/copy, recheck both records (and the exact metadata/cost/identity
used by the proposal) at confirmation, and perform one revision-guarded
write. Cancellation and stale/uncertain commits never substitute or retry.
No cross-record moves until they can be committed atomically under a
single parent save revision. Slots and snapshots outlive a dropdown
index, not the selected-save lease. The recovery actions remain
**Disable in all affected Forces** and **Delete from this save**.

The standalone host exposes this slice through `--place-owned-borg` or
an explicit `--arrange-from-force` handoff from the [saved Force
browser](borg-packages.md#place-an-owned-borg-in-an-empty-force-slot).
The other Collection and Force browsing routes remain read-only. Headless
mode cannot commit. Standalone arrangement does not supply a battle budget
or confer battle eligibility.

The cheat picker selects from the same installed library rows, with an
explicit exact variant. Authorization is a trusted engine/session
capability, not a flag a Borg declares. Browsing and selecting may prepare
an offer, never grant a copy. First build the picker and authorization
gate with no persistent grants. A subsequent confirmed grant needs real
instance identity, persistent provenance (compatible save revision),
engine-owned edited/cheat restrictions, and a capacity check; do not
masquerade it as a normal reward, or make a partially valid save.
The standalone `--cheat-offer-picker` route uses a session-only permission
for metadata offers and never opens a save.

## Presentation, localization and tools

Keep media paths local; bind named text keys, audio cues, art and fonts to
the shared UI and timeline services. Layout should carry anchors, focus
and command IDs, not numeric retail callbacks or rasterized labels. Script
extensions can request bounded engine capabilities; do not add filesystem,
platform commands, arbitrary actor writes, or arbitrary save access. UI
shapes, input and accessibility should be independent of Story and
Challenge state. Per-save features belong to their typed records, not to
an authored presentation file. Original resource conversion remains
optional, read-only on inputs, bounded, and outside shipping rules.

### Resolution, UI and HUD layout

Treat 640×480 as a legacy design canvas, not the desktop output size
or the coordinate system for new screens. Window size, framebuffer
size (including HiDPI), UI scale and the battle viewport are separate
inputs. Derive the 3D camera projection from its actual viewport;
changing aspect must not change simulation, aim or collision.

- New UI and HUD layouts use logical units, safe-area anchors and
  minimum/maximum sizes. Apply one uniform scale to text, icons and
  art; never stretch those assets separately in X and Y. Let panels,
  lists and text reflow, wrap, or scroll when the available area changes.
  Anchor each battle HUD to its own viewport, not the full window.
- Preserve the aspect of map, preview and opening media. For legacy
  4:3 screens, fit the full canvas with a centered uniform transform
  and letterboxing or pillarboxing if needed. Leave room for responsive
  layouts that use the full window; do not force 4:3 on the whole game.
- Draw and hit-test from the same layout. Map pointer input through
  the inverse of the layout transform, including the viewport origin
  and window-to-framebuffer scale; clicks in legacy bars do not select
  content. Resizing must preserve selection by stable key and scroll
  it back into view if necessary.

When a screen adopts the shared layout, test resize and pointer alignment
at 4:3, 16:9, ultrawide, a narrow window and a HiDPI scale. Use text and
long localized labels as part of the check; proportional artwork alone
does not prove the layout works.

### Portable UI font (v1)

Portable UI may supply a prebuilt glyph atlas; it does not need a platform
font, retail font asset, runtime rasterizer, or external library. The
presentation host chooses the font and loads its local files with the
existing path policy. Story and Borg packages do not change the current
UI font by declaring one. Inline colour, markup, font indices, and texture
handles are not content fields.

`font.manifest` is bounded to 256 KiB. It uses the shared versioned text
rules (including valid UTF-8 on comment lines):

```text
GFFONT 1
atlas=art/ui.a8
size=32|16
line-height=12
ascent=9
glyph=U+0020|0|0|0|0|0|0|4
glyph=U+0041|1|1|6|8|0|8|7
glyph=U+FFFD|9|1|6|8|0|8|7
```

The atlas is exactly `width*height` bytes, one byte of coverage per texel
in top-to-bottom row order, with each dimension 1–1024. The shared
renderer uploads white RGBA with coverage as alpha, using nearest
sampling and clamped edges. Rectangles use integer atlas texels and may
share coverage. There are at most 4096 unique glyph rows, keyed by a
canonical uppercase `U+` Unicode scalar (at least four hex digits; no
leading zeros beyond that padding). Reject surrogates, out-of-range
values, NUL/C0/C1 controls, unknown fields and duplicate keys. Runtime LF
is a line break, not a glyph.

Glyph fields are `scalar|x|y|width|height|bearing-x|bearing-y|advance`.
Use integer design pixels: positive X moves right, positive bearing Y
puts the rectangle's top above the baseline. The first baseline is
`top+ascent`; successive baselines are spaced by `line-height` (1–256,
with ascent 0–line-height). Bearings are −256…256 and advance 0–256.
Nonempty rectangles must fit the atlas. An inkless glyph has a zero
rectangle and zero bearings; U+0020 must be inkless with positive
advance. The required U+FFFD replacement must have nonzero advance and
visible coverage. Each unmapped scalar or malformed UTF-8 byte in a runtime
label uses the replacement; persisted strings are not rewritten.

The engine parses and binds the atlas outside any active frame. Use one
uniform scale (`0 < scale ≤ 16`), one shared texture, the existing screen
geometry, and consistent measurement and drawing. At most 16 KiB and
4096 decoded units are considered per atlas text block. Optional width
clipping stops on whole glyphs and resumes on the next explicit line; it is not wrapping or
ellipsizing. Width includes advances and horizontal ink overhang; each
line fits its measured width starting at the requested left edge. Height
counts line boxes; vertical bearings may place ink outside its line box.
The existing bitmap renderer remains the fallback when no valid atlas is
bound. UI callers can pass UTF-8 text, with ASCII compatibility
substitutions, integer scale and a smaller ink budget in that fallback
only. This v1 layout is left-to-right code-point rendering, not
bidirectional shaping, grapheme-aware truncation, kerning, localization
lookup, or locale-aware numeric formatting. Collection and Story can
consume the same text path without changing their content formats or
save behavior.

Engine entry points: `PortableFontMetrics::parse` in
`engine/assets/portable_font.hpp`, then `PortableFont::bind` and
`PortableTextRenderer::measure` / `draw` in
`engine/graphics/portable_text.hpp`. Pass a null font pointer to use the
bitmap presentation deliberately.
