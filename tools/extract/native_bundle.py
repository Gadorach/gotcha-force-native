"""Export PZZ decoded blocks as the native GFNB container; no role inference."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from tools.extract.pzz import PzzArchive
from tools.extract.source_image import ReadOnlyFile


def encode(blocks):
    if not 1 <= len(blocks) <= 511:
        raise ValueError("native bundle block count")
    payload_size = len(blocks) * 8 + sum(map(len, blocks))
    if any(len(b) > 32 << 20 for b in blocks) or payload_size + 16 > 32 << 20:
        raise ValueError("native bundle byte budget")
    return (struct.pack(">4I", 0x47464e42, 1, len(blocks), payload_size)
            + b"".join(struct.pack(">2I", i, len(b)) for i, b in enumerate(blocks))
            + b"".join(blocks))


def fnv32(data):
    result = 2166136261
    for byte in data:
        result = ((result ^ byte) * 16777619) & 0xffffffff
    return result


def export(source, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("native bundle output exists")
    archive = PzzArchive(source)
    remaining = (32 << 20) - 16 - 8 * len(archive.entries)
    blocks = []
    for i in range(len(archive.entries)):
        block = archive.read(i, max_output=min(16 << 20, remaining))[0]
        remaining -= len(block)
        blocks.append(block)
    payload = encode(blocks)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(payload)
    return {"bytes": len(payload), "fnv32": fnv32(payload),
            "sha256": hashlib.sha256(payload).hexdigest(),
            "blocks": [{"index": i, "bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}
                       for i, b in enumerate(blocks)]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    with ReadOnlyFile(args.archive) as source:
        result = export(source, args.output)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
