"""Export the per-Borg damage selectors from `plXXXXdata.bin`.

The amount pipeline selects the defender class row by `model+156`, the two
post-hit HP rows by `model+157` and `model+158`. Those three bytes live in the
432-byte per-Borg data block that the AFS name table publishes as
`pl<model>data.bin`:

    pl000adata.bin +156..158 = 1, 1, 0   (Sasuke)
    pl0615data.bin +156..158 = 2, 2, 2   (G-Red)

Combined forms use `data2.bin` (864 bytes) and `data3.bin` (1296 bytes): one
432-byte component block each, with its own selector triple. This tool reads
the AFS directory and those blocks only. It never writes to the archive and
refuses to overwrite its output. Three selector bytes per component are
numeric metadata, not model, texture or audio bytes.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

from tools.extract.source_image import ReadOnlyFile

AFS_COUNT = 2857
BLOCK_SIZE = 432
NAME_SIZE = 48
SELECTOR_OFFSETS = {"class": 156, "hp_157": 157, "hp_158": 158}
PATTERN = re.compile(r"pl([0-9a-f]{4})data(\d*)\.bin")
ASSET = re.compile(r"pl([0-9a-f]{4}).+")
KNOWN = {
    0x000A: (1, 1, 0),
    0x030A: (1, 2, 0),
    0x0615: (2, 2, 2),
}


def _u32(source, at):
    return struct.unpack("<I", source.read(at, 4))[0]


def _directory(source):
    if source.read(0, 4) != b"AFS\0":
        raise ValueError("not a GG4E08 AFS")
    count = _u32(source, 4)
    if count != AFS_COUNT:
        raise ValueError("unexpected AFS member count")
    pairs = list(struct.iter_unpack("<II", source.read(8, count * 8)))
    first = min((offset for offset, size in pairs if size), default=source.size)
    if first < 8 + count * 8 or first - 8 < 8 + count * 8:
        raise ValueError("invalid AFS payload start")
    names_offset, names_size = struct.unpack("<II", source.read(first - 8, 8))
    if names_size != count * NAME_SIZE:
        raise ValueError("unexpected AFS name table")
    if names_offset < 8 + count * 8 or names_offset + names_size > source.size:
        raise ValueError("AFS name table range")
    blob = source.read(names_offset, names_size)
    names = []
    for index in range(count):
        raw = blob[index * NAME_SIZE:(index + 1) * NAME_SIZE]
        end = raw.find(b"\0")
        if end < 0:
            raise ValueError("unterminated AFS member name")
        names.append(raw[:end].decode("ascii"))
    return pairs, blob, names


def _selectors(blocks):
    return [tuple(block[SELECTOR_OFFSETS[key]]
                  for key in ("class", "hp_157", "hp_158"))
            for block in blocks]


def recover(source):
    pairs, name_blob, names = _directory(source)
    models = {}
    combined = {}
    blocks = []
    asset_models = set()
    for index, name in enumerate(names):
        asset = ASSET.fullmatch(name)
        if asset is not None:
            asset_models.add(int(asset.group(1), 16))
        match = PATTERN.fullmatch(name)
        if match is None:
            continue
        model = int(match.group(1), 16)
        if model in models or model in combined:
            raise ValueError(f"duplicate Borg data block: {model:04x}")
        parts = int(match.group(2) or 1)
        offset, size = pairs[index]
        if size != BLOCK_SIZE * parts:
            raise ValueError(f"{name} extent changed: {size}")
        raw = source.read(offset, size)
        rows = [raw[BLOCK_SIZE * part:BLOCK_SIZE * (part + 1)]
                for part in range(parts)]
        selectors = _selectors(rows)
        for triple in selectors:
            if not all(0 <= value < 16 for value in triple):
                raise ValueError(f"{name} selector range: {triple}")
        if parts == 1:
            models[model] = selectors[0]
        else:
            combined[model] = selectors
        blocks.append(struct.pack(">H", model) + raw)
    blocks.sort()
    if len(models) != 198 or len(combined) != 13:
        raise ValueError(f"data block counts changed: "
                         f"{len(models)} single, {len(combined)} combined")
    for model, expected in KNOWN.items():
        if models.get(model) != expected:
            raise ValueError(f"selector mismatch for {model:04x}: "
                             f"{models.get(model)}")
    missing = sorted(asset_models - set(models) - set(combined))
    return {
        "kind": "gf-borg-damage-selectors",
        "revision": 1,
        "archive": {
            "member_count": AFS_COUNT,
            "name_table_sha256": hashlib.sha256(name_blob).hexdigest(),
            "blocks_sha256": hashlib.sha256(b"".join(blocks)).hexdigest(),
            "block_size": BLOCK_SIZE,
        },
        "offsets": SELECTOR_OFFSETS,
        "count": len(models),
        "combined_count": len(combined),
        "models": {f"{model:04x}": list(selectors)
                   for model, selectors in sorted(models.items())},
        "combined_models": {f"{model:04x}": [list(part) for part in parts]
                            for model, parts in sorted(combined.items())},
        "models_without_block": [f"{model:04x}" for model in missing],
        "notes": [
            "Selectors feed the defender rows of the amount pipeline: "
            "class at D100, HP 157 at D1EC, HP 158 at D260.",
            "The class row is 80433600[side][class][settings[team]]; the HP "
            "rows are 80433608/80433610[side][hp_n][post-hit HP index].",
            "Single-block models are in `models`; combined forms store one "
            "triple per component in `combined_models`.",
            "Which combined-form component feeds model+156 is not yet "
            "verified; the components are listed without choosing one.",
            "Eight asset-only ids have no data block (0604 cousins such as "
            "0619, 061b, 061f, 0620, 0628, and 0803, 080b, 0f07); they are "
            "listed and their selectors are not recovered.",
        ],
    }


def export(afs, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("selector output already exists")
    with ReadOnlyFile(afs) as source:
        if source.size <= 8 + AFS_COUNT * 8:
            raise ValueError("AFS extent")
        payload = recover(source)
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "models": payload["count"],
            "combined": payload["combined_count"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("afs", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.afs, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
