# Numbered Borg animation classification

The local tool shows looping captures from the existing native
model/pose/skin/GL renderer. It is an inspection tool, not a second game renderer
or an original actor/controller execution.

## Use

Bulk manual classification is paused. Normal G-Red's original controller is now
being [recovered from its state/program/descriptor tables](g-red-animation-controller.md).
Existing labels remain intact and serve as visual cross-checks.

The current classifier is at `http://127.0.0.1:8765/`.

- Choose G-Red or Sasuke, then page through numbered previews.
- Pick a classification under each preview and optionally add notes.
- Categories include low/high-speed movement, ground strafe/dash, **Air Dash**,
  upward/forward jump or flight, landing, damage reactions, melee chain stages,
  normal/charged X and B attacks, fusion combine/separate, transform, Idle,
  Unclassified and Other.
- Pause, change playback speed, search IDs and adjust page size as needed.
- Selections autosave locally. Export classifications for a separate JSON copy.

Current generated data and selections:

```
local/recomp-evidence/animation-classifier-v1/catalog.json
local/recomp-evidence/animation-classifier-v1/annotations.json
```

The catalog retains Borg, raw bank/group/clip, archive index and aliases.
Numbers derive from the first raw group/clip reference, with shared-bank IDs
starting at10000. Null slots are omitted; aliases do not masquerade as distinct
animations. The current data has110 G-Red and111 Sasuke entries, including shared
clips. Unsupported decoding is shown explicitly, never as an idle fallback.
All221 entries have been prepared:207 have playable previews, while14 are
explicitly unsupported (AObj/track or public-bone/extern binding limits).

Classifications are **per Borg**, not a universal locomotion map. Compare other
Borgs later to identify shared patterns/archetypes. The tool does not
automatically bind annotations into gameplay.

The current manually curated G-Red locomotion map uses identified IDs
0–9,23 and24 in `engine/gameplay/native_borg_motion_map.hpp`; uncertain14 and
melee entries stay unused. Sasuke has not inherited those classifications.

Previews play in place and currently show body animation only. Original root
displacement, weapon attachments, effects and full action context are not a
complete gameplay reconstruction; use notes for ambiguity.

## Run / rebuild

```sh
python3 tools/animation_browser.py prepare \
  --binary build/animation-classifier-host-v2/gf_native \
  --assets local/content/native-animation-mosaic-v1 \
  --output local/recomp-evidence/animation-classifier-v1
python3 tools/animation_browser.py serve \
  local/recomp-evidence/animation-classifier-v1 --port 8765
```

Preparation requires a **new** output directory. It reads source art only,
records native frames in bounded batches, creates preview sprite sheets and
removes only its own individually named temporary frames. Annotation updates
modify only the generated annotation JSON, not original assets.

The standalone native preview window remains available:

```sh
build/animation-classifier-host-v2/gf_native --animation-mosaic \
  --battle-mvp-assets local/content/native-animation-mosaic-v1 \
  --size 1152 648 --no-audio
```

Arrow left/right changes page; up/down changes Borg; Space pauses, Q/E changes
speed and Z rotates the previews. `--animation-grid 3 2` makes larger cells.
