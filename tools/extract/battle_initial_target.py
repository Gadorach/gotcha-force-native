"""Export the qualified normal-class initial-target tables from a verified DOL."""
import argparse
import json
from pathlib import Path
import struct

from tools.extract.native_bundle import fnv32
from tools.extract.source_image import DolImage, ReadOnlyFile


SOURCE_SHA256 = "21454a8880c6254616662710d6ec626e43d853999da2cdfad700030960a94836"
CLASSES = {10: (0x802d3ee0, 3937624874), 0x615: (0x80365ac8, 3803464602)}


def encode(image, model):
    if image.sha256 != SOURCE_SHA256 or model not in CLASSES:
        raise ValueError("unqualified source revision/normal class")
    config, model_hash = CLASSES[model]
    selectors = image.read(config + 0x108, 6)
    if selectors != bytes(6):
        raise ValueError("nonzero initial target policy unsupported")
    maximum = image.read(0x80436e88, 4)
    if maximum != struct.pack(">I", 0x4dbebc20):
        raise ValueError("source target distance operand")
    raw = bytearray(4688)
    struct.pack_into(">6I2H", raw, 0, 0x47464e54, 1, len(raw), model_hash,
                     fnv32(image.read(config, 192)), config, model, 6)
    raw[28:34] = selectors
    raw[48:52] = maximum
    struct.pack_into(">I", raw, 52, 6)
    raw[64:80] = image.read(0x802cfaf4, 16)
    for selector in range(18):
        root = int.from_bytes(image.read(0x802cb2f0 + 4 * selector, 4), "big")
        raw[80 + selector * 256:80 + (selector + 1) * 256] = image.read(root, 256)
    return bytes(raw)


def export(source, output, model):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("initial target output exists")
    image = DolImage(source.read(0, source.size), SOURCE_SHA256)
    raw = encode(image, model)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "fnv32": fnv32(raw), "model": model,
            "source_sha256": image.sha256}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--model", type=lambda value: int(value, 0), required=True,
                        choices=tuple(CLASSES))
    args = parser.parse_args()
    with ReadOnlyFile(args.dol) as source:
        print(json.dumps(export(source, args.output, args.model)))


if __name__ == "__main__":
    main()
