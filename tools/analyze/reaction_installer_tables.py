"""Resolve each pilot model's reaction table from its installer code.

The installer function writes a literal table address to `actor+0x27C`.
The `lis`+`addi` pair immediately feeding the `stw ..., 0x27C` is
decoded directly from the DOL, so no private decompilation input is
needed. Covers the authored basic-shot cohort.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze.retail_basic_shot_test import COHORT
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

INSTALLER_TABLE = 0x802D3224
INSTALLER_BYTES = 288


def _signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def _table_address(image, model):
    base, = struct.unpack(">I", image.read(
        INSTALLER_TABLE + 4 * (model >> 8), 4, "data"))
    address = base + 4 * (model & 255)
    return struct.unpack(">I", image.read(address, 4, "data"))[0]


def resolve_installer(image, installer):
    code = image.read(installer, INSTALLER_BYTES, "text")
    registers = {}
    for offset in range(0, len(code), 4):
        word = struct.unpack_from(">I", code, offset)[0]
        op = word >> 26
        destination = (word >> 21) & 31
        source = (word >> 16) & 31
        immediate = word & 0xFFFF
        if op == 15:
            registers[destination] = _signed(immediate << 16, 32)
        elif op == 14:
            if source == 0:
                registers[destination] = _signed(immediate, 16)
            elif source in registers:
                registers[destination] = registers[source] + _signed(immediate, 16)
            else:
                registers.pop(destination, None)
        elif op == 36:  # stw RS, off(RA)
            if immediate == 0x27C and destination in registers:
                return registers[destination] & 0xFFFFFFFF
    return None


def model_table(image, model):
    installer = _table_address(image, model)
    table = resolve_installer(image, installer)
    if table is None:
        return installer, None
    try:
        image.read(table, 24, "data")
    except ValueError:
        return installer, None
    return installer, table


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    models = []
    for model in sorted(COHORT):
        installer, table = model_table(image, model)
        models.append({"model": f"{model:04X}",
                       "installer": f"{installer:08X}",
                       "table": f"{table:08X}" if table else None})
    return {
        "kind": "gf-reaction-installer-tables",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "installer_table": f"{INSTALLER_TABLE:08X}",
        "models": models,
        "notes": [
            "The table address is the lis/addi pair feeding stw ..., 0x27C "
            "in the model's installer.",
            "A null table means the installer does not use the direct literal "
            "pattern.",
            "Models cover the authored basic-shot cohort.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction installer table output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "models": len(payload["models"]),
            "resolved": sum(bool(model["table"]) for model in payload["models"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
