# Gotcha Force — native port

Work-in-progress, independently implemented C++20 native game systems with
platform backends and offline asset readers. Linux is the current reference host.
This repository contains **source code and tools only**. Supply your own original
game image; no game assets, original executable, emulator, generated translation,
reconstructed C decompilation, private evidence or credentials are included.

## Current state

- Shared GL renderer, model/material/animation machinery, input and audio.
- Native menu/editor systems, inventory and save formats.
- Challenge setup and ordered battle initialization in progress.
- Story setup/progression/UI/audio components.
- Standalone native G-Red/Sasuke duel MVP with provisional gameplay rules.
- Opt-in normal-G-Red controller labs using the shared world, animation VM,
  renderer and independently checked native contact receipts.
- **No original-battle, original-hardware or full-game compatibility claim.**

Build and run the desktop prototype:

```sh
python3 -B tools/build_native.py --host --headers --jobs 1
build/direct/gf_native --battle-mvp --no-audio
```

Game data is not included. Prepare local art from your existing qualified native
bundles with `python3 -m tools.extract.native_battle_mvp --help`.
See [native MVP](docs/native-battle-mvp.md), [portable movement kernels](docs/movement-kernels.md)
and [G-Red controller recovery](docs/g-red-animation-controller.md).
The latest opt-in lab is `--source-ground-ledge` (or
`--demo-source-ground-ledge --headless --frames 600`), requiring a local GFMM9 pack.
One controller supports walking/stop and owned-root ordered dash, including both
ledge departures, delayed program13 falling/drift and flat landing. Release then
flick to dash; no selector is forced by the host. Jump/air-dash/flight/combat,
nonflat dash/landing and clipping remain refused. The roof demo walks, stops and
dashes in one world/producer with no teleport/rebind or stage/target edit.
Interactive settings and older narrow policies are unchanged.
Other Borgs do not inherit G-Red's recovered mappings.

Some development demonstrations use private generated metadata profiles and
integration harnesses. Those artifacts are not distributed here. The public
desktop entry point does not yet connect every newer menu/Story/Challenge
component. A successful build or extracted ISO is not a completed game.

## Build

Requires a C++20 compiler. The desktop executable additionally needs `pkg-config`,
SDL2 and desktop OpenGL development packages.

```sh
cmake -S . -B build/native -DGF_BUILD_DESKTOP=ON
cmake --build build/native -j2
ctest --test-dir build/native --output-on-failure
```

Without CMake, the included direct build runner uses the same native sources:

```sh
python3 tools/build_native.py --host
python3 tools/build_native.py --tests backends,hsd_record_animation,force_inventory
build/direct/gf_native --headless --frames 3
```

Tests using original data skip their optional source-data portion when it is
absent; generated test fixtures are separate from original game assets.
Hardware/display-dependent tests are opt-in. Compilation is limited to two jobs.

## Extract your own assets

Python 3 is sufficient for the supplied raw-disc/AFS/PZZ readers and PNG export.
Extraction writes only to a new directory or new file and never modifies
the input image.

```sh
python3 -m tools.extract.disc game.iso original/extracted
python3 tools/extract/member.py original/extracted/files/afs_data.afs \
  pl0615.pzz local/focus/pl0615.pzz
python3 -m tools.extract.pzz_files local/focus/pl0615.pzz local/focus/pl0615
python3 tools/convert/texture.py original/extracted/files/opening.bnr \
  local/assets/banner.png
```

See [asset extraction](docs/asset-extraction.md) for supported formats and limits.
Extraction currently exposes the original data and reusable decoding primitives;
it does **not** yet recreate every private native metadata/profile used by all
development harnesses. Do not download or distribute original assets.

## Layout

- `engine/`: portable native systems; no SDL/OpenGL headers in game logic.
- `platform/`: desktop rendering, input, audio and save backends.
- `tests/unit/`: native and extraction tests.
- `tools/extract/`, `tools/convert/`, `tools/analyze/`: bounded offline readers.
- `tools/content_package.py`: deterministic loose-content packaging.

The first publication consolidates the paused native menu/editor, Challenge,
Story and graphics work, plus relevant reusable Borg asset-analysis utilities.
It deliberately starts a new Git history: the older decompilation/controller
research repository and its private data remain separate.
