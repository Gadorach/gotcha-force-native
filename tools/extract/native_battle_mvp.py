"""Prepare loose MVP art from existing native bundles; read-only, no overwrites."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.extract.source_image import ReadOnlyFile


def read(path, budget):
    with ReadOnlyFile(path) as source:
        if not 0 < source.size <= budget:
            raise ValueError(f"input exceeds budget: {path}")
        return source.read(0, source.size)


def word(raw, at):
    if at < 0 or at + 4 > len(raw):
        raise ValueError("truncated word")
    return struct.unpack_from(">I", raw, at)[0]


def blocks(raw):
    count = word(raw, 8)
    if (word(raw, 0) != 0x47464E42 or word(raw, 4) != 1
            or not 1 <= count <= 511 or word(raw, 12) != len(raw) - 16):
        raise ValueError("native bundle header")
    at = 16 + count * 8
    result = []
    for index in range(count):
        if word(raw, 16 + index * 8) != index:
            raise ValueError("native bundle slot order")
        size = word(raw, 20 + index * 8)
        if size > len(raw) - at:
            raise ValueError("native bundle slot extent")
        result.append(raw[at:at + size])
        at += size
    if at != len(raw):
        raise ValueError("native bundle suffix")
    return result


def prepare(stage, g_red, sasuke, g_red_pose, sasuke_pose, output, common=None):
    stage_parts = blocks(read(stage, 32 << 20))
    if len(stage_parts) != 4:
        raise ValueError("four-slot battle stage required")
    stage_size = word(stage_parts[0], 0)
    if not 32 <= stage_size <= len(stage_parts[0]):
        raise ValueError("stage HSD extent")
    if word(stage_parts[1], 0) != 0x53544948:
        raise ValueError("stage floor STIH required")
    files = {"stage.hsd": stage_parts[0][:stage_size], "floor.stih": stage_parts[1]}
    if common is not None:
        shared = blocks(read(common, 32 << 20))
        if len(shared) != 4 or len(shared[2]) < 64:
            raise ValueError("four-slot common resource with motion bank required")
        files["common.motions"] = shared[2]
    for name, path, pose, expected in (
            ("g-red", g_red, g_red_pose, 0xE2B4439A),
            ("sasuke", sasuke, sasuke_pose, 0xEAB3632A)):
        parts = blocks(read(path, 32 << 20))
        if not 10 <= len(parts) <= 32 or not parts[3]:
            raise ValueError("normal character motion/model slots required")
        wrapper = parts[4]  # normal variant only
        at = word(wrapper, 0)
        size = word(wrapper, at)
        if at < 4 or size < 32 or at + size > len(wrapper):
            raise ValueError("normal model wrapper extent")
        model = wrapper[at:at + size]
        value = 2166136261
        for byte in model:
            value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
        if value != expected:
            raise ValueError(f"not the original normal {name} model")
        files.update({name + ".hsd": model, name + ".prefix": wrapper[:at],
                      name + ".motions": parts[3], name + ".pose": read(pose, 1 << 20)})
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    manifest = {}
    for name, raw in files.items():
        with (output / name).open("xb") as stream:
            stream.write(raw)
        manifest[name] = {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}
    with (output / "manifest.json").open("x") as stream:
        json.dump({"kind": "native MVP art, not original gameplay rules", "files": manifest},
                  stream, indent=2)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("stage", "g-red", "sasuke", "g-red-pose", "sasuke-pose", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--common", type=Path, help="optional shared battle-motion native bundle")
    args = parser.parse_args()
    result = prepare(args.stage, args.g_red, args.sasuke, args.g_red_pose,
                     args.sasuke_pose, args.output, args.common)
    print(json.dumps({"output": str(args.output), "files": len(result)}))


if __name__ == "__main__":
    main()
