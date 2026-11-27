"""Resolve the per-model reaction table from bounded installer paths.

The installer writes a literal address to `actor+0x27C`. Check all
bounded paths, resolving direct model equality gates before accepting
the returned binding. No private decompilation input is needed.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze.retail_basic_shot_test import COHORT
from tools.analyze.retail_action_tables import MODEL_JUMP
from tools.analyze.retail_movement_tables import literal_at_store
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

INSTALLER_TABLE = 0x802D3224
INSTALLER_BYTES = 144 * 4


def _signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def _table_address(image, model):
    base, = struct.unpack(">I", image.read(
        INSTALLER_TABLE + 4 * (model >> 8), 4, "data"))
    address = base + 4 * (model & 255)
    return struct.unpack(">I", image.read(address, 4, "data"))[0]


def resolve_installer(image, installer, model):
    words = tuple(word for word, in struct.iter_unpack(">I", image.read(installer, INSTALLER_BYTES, "text")))
    pending = [(0, None, False, None)]
    seen, returned = set(), set()
    while pending:
        state = pending.pop()
        index, table, model_in_r0, comparison = state
        if state in seen:
            continue
        if not 0 <= index < len(words) or len(seen) >= 384:
            return None
        seen.add(state)
        if words[index:index + len(MODEL_JUMP)] == MODEL_JUMP:
            member = model - 0x0506
            if not 0 <= member < 15:
                returned.add(table)
                continue
            address, = struct.unpack(">I", image.read(0x802FF278 + 4 * member, 4, "data"))
            target, tail = divmod(address - installer, 4)
            if tail or target < index + len(MODEL_JUMP):
                return None
            pending.append((target, table, False, None))
            continue
        word = words[index]
        if word & 0xFC1FFFFF == 0x9003027C:  # stw literal, 0x27C(r3)
            try:
                table = literal_at_store(words, index)
            except ValueError:
                return None
        op = word >> 26
        taken = None
        bo, bi = (word >> 21) & 31, (word >> 16) & 31
        if op in (16, 19) and bo in (4, 12) and bi < 3 and comparison is not None:
            matches = bi == comparison
            taken = matches if bo == 12 else not matches
        if op in (16, 18):
            if word & 3:  # Absolute transfers and calls are not followed.
                return None
            bits = 16 if op == 16 else 26
            target = index + _signed(word & ((1 << bits) - 4), bits) // 4
            if target <= index:
                return None
            if taken is not False:
                pending.append((target, table, model_in_r0, comparison))
            if op == 16 and taken is not True:
                pending.append((index + 1, table, model_in_r0, comparison))
        elif op == 19:
            if (word >> 1) & 1023 != 16 or word & 1:  # Only returns.
                return None
            if taken is not False:
                returned.add(table)
            if bo != 20 and taken is not True:
                pending.append((index + 1, table, model_in_r0, comparison))
        elif word:
            # Track only lha r0, actor+0x3E8 followed by a local chain of
            # cmpwi r0 and branches. Other instructions kill provenance.
            if model_in_r0 and word >> 16 == 0x2C00:
                rhs = _signed(word & 0xFFFF, 16)
                comparison = 0 if model < rhs else 1 if model > rhs else 2
            else:
                model_in_r0, comparison = word == 0xA80303E8, None
            pending.append((index + 1, table, model_in_r0, comparison))
        else:
            return None
    return returned.pop() if len(returned) == 1 else None


def model_table(image, model):
    installer = _table_address(image, model)
    table = resolve_installer(image, installer, model)
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
        "revision": 2,
        "source_dol_sha256": SOURCE_SHA256,
        "installer_table": f"{INSTALLER_TABLE:08X}",
        "models": models,
        "notes": [
            "The table address is the literal returned by the bounded actor+0x27C installer paths for the model.",
            "A null table means the installer did not return one proven literal on all reviewed paths.",
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
