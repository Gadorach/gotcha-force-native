"""Replay indexed type-89 requests and carrier admission under pool pressure."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type89 as type89, retail_type89_roster as roster
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import encode
from tools.extract.source_image import DolImage


MODELS = (0x0503, 0x0506, 0x050F, 0x0512)
CASES = (("ample", 8, 8), ("one-carrier", 1, 8), ("one-descriptor", 8, 1),
         ("no-carriers", 0, 8), ("no-descriptors", 8, 0), ("refill-after-first-request", 0, 0))
ROW = re.compile(r"type89-cast:(?: [0-9]{1,4}){8}")
ENTRY, RETURN = 0x800B8D6C, 0x8006A474


def decode_report(report, selector):
    lines = report.splitlines()
    if len(report) > 8192 or not 0 < len(lines) <= len(MODELS)*len(CASES)*8:
        raise ValueError("type-89 cast report extent")
    pos = 0
    pilots = []
    for model in MODELS:
        bank = int(f"{model:04X}" in selector["banks"][1]["model_guard"])
        windows = selector["banks"][bank]["ordered_keys_by_cursor"]
        cases = []
        for scenario, (name, carriers, descriptors) in enumerate(CASES):
            remaining = min(carriers, descriptors)
            cast_visits = []
            for cursor, keys in enumerate([*windows, []]):
                if pos == len(lines) or ROW.fullmatch(lines[pos]) is None:
                    raise ValueError("malformed or missing type-89 cast row")
                observed = tuple(map(int, lines[pos].split()[1:]))
                pos += 1
                if name == "refill-after-first-request" and cursor == 1:
                    remaining = 1
                visit = 4+11*cursor
                admitted = min(remaining, len(keys))
                operands = (keys+[0, 0])[:2]
                if observed != (model, scenario, visit, cursor, len(keys), *operands, admitted):
                    raise ValueError(f"type-89 request order, timer or allocation mismatch: {model:04X}/{name}/{cursor}")
                remaining -= admitted
                if not keys:
                    sentinel = visit
                else:
                    cast_visits.append({"visit": visit, "cursor": cursor, "requested_keys": keys,
                                        "allocated_keys": keys[:admitted]})
            cases.append({
                "case": name, "initial_carrier_cells": carriers, "initial_descriptor_cells": descriptors,
                **({"refill_at_visit": 5, "added_carrier_cells": 1, "added_descriptor_cells": 1}
                   if name == "refill-after-first-request" else {}),
                "casts": cast_visits, "sentinel_visit": sentinel, "cursor_at_sentinel": len(windows),
            })
        pilots.append({"borg": f"gg4e08:{model:04x}", "bank": bank, "cases": cases})
    if pos != len(lines):
        raise ValueError("extra type-89 cast rows")
    return pilots


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-89 cast output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), roster.SOURCE_SHA256)
    type89.verify_text(image, tuple(span for span in roster.TEXT if span[0] == ENTRY))
    type89.rows(image)
    selector = roster.indexed_selectors(image)
    for site in roster.INDEXED:
        type89.link(image, site, type89.FACTORY)
    type89.link(image, 0x800B8EE4, RETURN)
    cooldown, = struct.unpack(">f", image.read(0x8043EA20-25936, 4, "data"))
    if cooldown != 10:
        raise ValueError("type-89 cast cooldown operand changed")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-89 cast fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    pilots = decode_report(result.stdout.decode("ascii"), selector)
    payload = {
        "kind": "gf-retail-type89-cast", "revision": 1, "source_dol_sha256": roster.SOURCE_SHA256,
        "actor_phase_leaf": roster.address(ENTRY), "factory": roster.address(type89.FACTORY),
        "cursor_table": selector["table"], "actor_return_cut": roster.address(RETURN),
        "initial_clock": 2.5, "clock_debit_per_countdown_visit": 1, "timed_step_clock_reset": cooldown,
        "cases": [{"borg": p["borg"], "bank": p["bank"], **case} for p in pilots for case in p["cases"]],
        "notes": [
            "The fixture enters the original phase-two leaf with a supplied actor, cursor zero and positive clock.",
            "Original constructor calls run to return. Independently bounded object and descriptor pools are not recycled.",
            "The negative-pair visit is observed up to the actor-return helper; its action effects are not executed.",
            "Allocated cells are uninitialized carriers. There are no carrier updates, opening shape binds, or hits in this replay.",
            "This is not controller or action admission, a stock debit replay, or native gameplay.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots), "cases": len(pilots)*len(CASES)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
