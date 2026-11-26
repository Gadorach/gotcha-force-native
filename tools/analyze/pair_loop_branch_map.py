"""Map the conditional branches of the pair loop `8002DB58`.

The loop has 32 conditional branches: outer prepare, inner validity,
the direct/nearest selection and the publish band. This tool decodes each
`bc` with the nearest compare/mask, plus the two CTR matrix-copy loops.
Addresses and operands only.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze.collision_branch_map import CONDS, _attach, _condition
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002DB58
SIZE = 944
INNER = 0x8002DBF4
SELECT = 0x8002DD90
PUBLISH = 0x8002DE08
CATEGORY_TABLE = 0x802CFF68
CATEGORY_COUNT = 48  # the following word starts the 0x8003A0C8 table
HANDLER_TABLE = 0x802CFF2C


GATES = {
    "8002DBC8": ("direct-category", "[outer+0x88] == 2 selects direct mode"),
    "8002DBD4": ("record-direct-bit", "record+0x10 bit 0 also selects direct"),
    "8002DC04": ("inner-carrier-0x800", "inner carrier node+2 bit 0x800"),
    "8002DC10": ("record-0x8000", "record+0x12 bit 0x8000"),
    "8002DC20": ("outer-carrier-0x1000", "outer carrier node+2 bit 0x1000"),
    "8002DC28": ("inner-carrier-0x2000", "inner carrier node+2 bit 0x2000"),
    "8002DC30": ("outer-carrier-0x4000", "outer carrier node+2 bit 0x4000"),
    "8002DC38": ("inner-carrier-0x2000-pair", "paired 0x2000 test"),
    "8002DC48": ("receiver-474-bit1", "receiver+0x474 bit 1"),
    "8002DC60": ("validity-8002FD7C", "side validity filter result"),
    "8002DC74": ("partner-a-active", "inner linked object +131"),
    "8002DC80": ("partner-b-active", "outer linked object +131"),
    "8002DC98": ("partner-team-equal", "linked-object team equality"),
    "8002DCA4": ("partner-a-flag475", "inner linked +475"),
    "8002DCB0": ("partner-a-model14", "inner linked model 14"),
    "8002DCBC": ("partner-a-link", "inner linked link compare"),
    "8002DCC8": ("partner-b-flag475", "outer linked +475"),
    "8002DCD4": ("partner-b-model14", "outer linked model 14"),
    "8002DCE0": ("partner-b-link", "outer linked link compare"),
    "8002DCF4": ("sphere-80039F6C", "sphere overlap result"),
    "8002DD20": ("matrix-copy", "45-dword matrix copy"),
    "8002DD4C": ("matrix-copy-pair", "second 45-dword matrix copy"),
    "8002DD94": ("handler-80030490", "pair category handler result"),
    "8002DD9C": ("direct-vs-nearest", "r22 chooses direct or nearest"),
    "8002DDC4": ("nearest-distance", "nearest distance <= best"),
    "8002DE04": ("inner-loop", "inner actor count"),
    "8002DE0C": ("direct-mode-kept", "direct pair is kept"),
    "8002DE14": ("nearest-found", "a nearest target was found"),
    "8002DE4C": ("aim-record-0x3C0", "record+0x12 bits 0x3C0"),
    "8002DE6C": ("aim-length", "aim vector length test"),
    "8002DEBC": ("owner-callback", "owner+256 callback presence"),
    "8002DEE8": ("outer-loop", "outer actor count"),
}


def _phase(site):
    if site < INNER:
        return "prepare"
    if site < SELECT:
        return "inner"
    if site < PUBLISH:
        return "select"
    return "publish"


def _category_dispatch(image):
    values = list(image.read(CATEGORY_TABLE, CATEGORY_COUNT, "data"))
    peak = max((value & 0x7F) for value in values if value != 0xFF)
    raw = image.read(HANDLER_TABLE, (peak + 1) * 4, "data")
    handlers = [f"{struct.unpack_from('>I', raw, i * 4)[0]:08X}"
                for i in range(peak + 1)]
    categories = []
    for index, value in enumerate(values):
        if value == 0xFF:
            categories.append({"category": index, "byte": value,
                               "handler": None, "swap": False,
                               "address": None})
            continue
        handler = value & 0x7F
        categories.append({"category": index, "byte": value,
                           "handler": handler, "swap": bool(value & 0x80),
                           "address": handlers[handler]})
    return {"index_table": f"{CATEGORY_TABLE:08X}",
            "handler_table": f"{HANDLER_TABLE:08X}",
            "handlers": handlers, "categories": categories}


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    code = image.read(FUNCTION, SIZE, "text")
    branches = []
    for off in range(0, len(code), 4):
        word = struct.unpack_from(">I", code, off)[0]
        if word >> 26 != 16:
            continue
        site = FUNCTION + off
        bo = (word >> 21) & 31
        bi = (word >> 16) & 31
        displacement = word & 0xFFFC
        if displacement & 0x8000:
            displacement -= 0x10000
        target = site + displacement if not (word & 2) else displacement
        if bo in (16, 18):
            condition, source = {"kind": "bdnz", "text": "decrement CTR"}, None
        else:
            condition, source = _condition(code, site, FUNCTION)
            if source is not None:
                condition = _attach(condition, source, code, FUNCTION)
        entry = {
            "site": f"{site:08X}",
            "target": f"{target & 0xFFFFFFFF:08X}",
            "phase": _phase(site),
            "bo": bo,
            "bi": bi,
            "cr": bi >> 2,
            "cond": CONDS[bi & 3],
            "sense": "true" if bo & 8 else "false",
            "condition": condition,
            **({"source": f"{source:08X}"} if source is not None else {}),
        }
        if f"{site:08X}" in GATES:
            entry["gate"], entry["meaning"] = GATES[f"{site:08X}"]
        branches.append(entry)
    if len(branches) != 32:
        raise ValueError(f"branch count changed: {len(branches)}")
    phases = {}
    for branch in branches:
        phases[branch["phase"]] = phases.get(branch["phase"], 0) + 1
    return {
        "kind": "gf-pair-loop-branch-map",
        "revision": 3,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {"address": f"{FUNCTION:08X}", "size": SIZE,
                     "branches": len(branches)},
        "phases": phases,
        "branches": branches,
        "category_dispatch": _category_dispatch(image),
        "notes": [
            "Each branch records the nearest flag-setting compare/mask in the "
            "eight instructions before it.",
            "The two CTR branches are the 45-dword matrix copies.",
            "`operand`/`operands` name the last load, immediate, mask, indexed "
            "load or call result that fed the tested register; float provenance "
            "is the nearest load, not the whole expression.",
            "`category_dispatch` decodes 80030490: 48 category bytes index a "
            "15-entry handler table; bit 0x80 swaps the two arguments.",
            "Named gates and the pair validity rules are documented separately.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("pair loop branch output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "branches": len(payload["branches"]), "phases": payload["phases"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
