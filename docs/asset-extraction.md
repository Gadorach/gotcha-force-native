# Offline asset extraction

Run commands from the repository root. Inputs are read-only. Outputs belong in
ignored `original/` or `local/` directories and must not be committed.

## Raw GameCube ISO/GCM

`python3 -m tools.extract.disc IMAGE OUTPUT` reads the disc FST through the shared
bounds-checked `GcmImage` reader, extracts filesystem files into `OUTPUT/files/`,
and the initialized DOL file into `OUTPUT/sys/main.dol`. It records relative file
names, sizes and SHA-256 digests in `OUTPUT/extraction.json`. It never executes the
original executable or exports reconstructed source.

Use `--list` to inspect names without writing output, or repeat `--file PATH` to
select filesystem members. `--no-dol` omits the executable. Paths are checked for
traversal, symlinks, collisions and platform-reserved names. OUTPUT must not exist.
Partial failed output is retained with a failure marker, not treated as success.

Only raw disc images are supported. This is not an RVZ/WBFS/NKit converter and
does not download or repair images. The disc reader has owned synthetic tests;
the public publication checks do not assert an original-ISO end-to-end result.

## AFS

`tools/extract/member.py ARCHIVE NAME OUTPUT` resolves exactly one directory name,
validates its range and refuses overwriting OUTPUT. The stored archive name is
never used as a destination path.

## PZZ

`python3 -m tools.extract.pzz_files ARCHIVE OUTPUT` exports every declared block
using fixed numeric filenames. It uses the native Python word-LZ decoder; no
external decompressor is required. Empty blocks remain empty, decoded padding is
preserved, and compressed consumption/size/digests are recorded.

## Textures and HSD

`tools/convert/texture.py` supports I4, IA4, RGB565, RGB5A3, RGBA8, C4/C8 and BNR1
using Python's standard library for PNG output. HSD metadata/scene/animation inspection tools live in
`tools/analyze/`. Unknown formats and unsupported structures are errors, not
empty substitute models.

## Remaining native-profile integration

`python3 -m tools.extract.native_bundle ARCHIVE.pzz OUTPUT.gfnb` now packages the
decoded blocks into the existing native GFNB format. It preserves empty blocks
and decoded padding, reports block/file hashes and refuses overwrites. This is a
generic container export; it does not guess the game-specific role of a block or
create the separate identity-bound native metadata profiles.

`python3 -m tools.extract.battle_initial_target MAIN.dol OUTPUT.gfnt --model 10`
exports the bounded initial-target profile for normal Sasuke; use `--model 0x615`
for normal G-Red. It requires the qualified DOL SHA-256, binds the class-config
identity, checks the zero-selector policy and exports both reached timer tables.
Inputs are read-only and existing outputs are refused. This profile supplies
constructor operands, not actor or frame readiness.

The engine consumes versioned native profiles for game-specific cameras, UI
bindings, rosters, animation policies and source constants. Earlier research
exports of those profiles are private. The reusable public readers alone do not
produce a complete installed game-content tree; the complete version/revision-
bound profile extraction workflow is still being integrated. No profile binaries,
original strings, textures, audio or captured guest memory are bundled here.
