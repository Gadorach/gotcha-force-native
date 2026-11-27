"""Index the first known cue operands in the observed standalone program banks."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.gred_animation_controller import decode_program, signed_table
from tools.analyze.retail_code_atlas import MAX_SYMBOLS_BYTES, address, object_sizes, read_bounded, roster_paths
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import CATALOG_SHA256, read_catalog
from tools.extract.source_image import DolImage

INSTALLER_MAP_SHA256 = "afc01c017966b0646bba7654f052d8d2fe5fef6989bb217524e71c831608906b"
OPCODE_TABLE = 0x802D0DA0
UPPER_CONTROL = 0x8004C424
DIRECT_MELEE = 0x8004C6B4
SPANS = {
    0x8004BEB8: (360, "af8c9bb8f3fa6387ee864411f266f0757d936550f2ac0cfa50bfc3b2e35390a2"),
    0x8004C050: (584, "1161d002aab162ea3506cd8a088d941fb362861f51cfae3f35ac6b57547d3ab0"),
    0x8004CD24: (720, "0d8fdecb40f550e2003f0a28a40d4399178851ca4f1901ba73d9039701267c80"),
    UPPER_CONTROL: (100, "be8ac95c75902d94a0afcbb214ac797ca8f501e0676336d015a424da509cc839"),
    DIRECT_MELEE: (168, "1d5f88373d984b7b7afb6a80147ec28f2e39ae3061fce32f4b0a47cd47f5a4a3"),
}


def selected_banks(installed, borgs, profiles):
    if len(installed) != len(borgs):
        raise ValueError("standalone installer cohort disagreement")
    bindings = []
    for observed, borg in zip(installed, borgs, strict=True):
        profile = observed["profile"]
        if (observed["id"] != borg["id"] or observed["installer"] != borg["installer"]
                or profile not in borg["profiles"]):
            raise ValueError("standalone installer profile disagreement")
        local, common, _, _ = profiles[profile]["program_and_descriptor_roots"]
        bindings.append({"id": borg["id"], "profile": profile, "local_bank": local, "common_bank": common,
                         "config_lane_mask_seed": observed["config_lane_mask_seed"]})
    return bindings


def collect_bank(read, root):
    if not root:
        return {"status": "absent-bank"}
    try:
        tables = signed_table(read, root, 16)
    except ValueError:
        return {"status": "refused-bank-table"}
    groups = []
    for table in tables:
        group = {"table": address(table)}
        try:
            group["choices"] = [address(program) for program in signed_table(read, table)]
        except ValueError:
            group["status"] = "refused-choice-table"
        groups.append(group)
    return {"status": "partial" if any("status" in group for group in groups) else "indexed", "groups": groups}


def describe_program(read, entry):
    try:
        decoded = decode_program(read, entry)
    except ValueError:
        return {"status": "refused-program-extent"}
    result = {"status": decoded["status"], "commands": len(decoded["commands"])}
    if decoded["status"] == "unsupported_opcode":
        result["unsupported"] = {"site": address(decoded["next_address"]), "opcode": decoded["opcode"]}
    for command in decoded["commands"]:
        opcode = command["opcode"]
        raw = bytes.fromhex(command["raw"])
        cue = {"site": address(command["address"]), "preceding_waits": command["preceding_waits"]}
        # Handler 2 always stores operand byte 2 on lane zero. Byte 1 is
        # a separate mode operation, even when it's -1 and leaves mode alone.
        if opcode == 2 and 0 < raw[2] < 128:
            result.setdefault("first_positive_upper_cue", {**cue, "value": raw[2]})
        elif opcode == 10 and 0 < raw[1] < 128:
            result.setdefault("first_direct_melee_request", {**cue, "queue": raw[1]})
        elif opcode == 17:
            result.setdefault("first_class_event_candidate", cue)
    return result


def recover(image, catalog, symbols, installer_raw):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if sha256(installer_raw).hexdigest() != INSTALLER_MAP_SHA256:
        raise ValueError("unqualified standalone installer map")
    installed = json.loads(installer_raw)
    if (installed["source_dol_sha256"] != SOURCE_SHA256 or installed["catalog_sha256"] != CATALOG_SHA256
            or installed["symbol_metadata_sha256"] != sha256(symbols).hexdigest()):
        raise ValueError("standalone provenance disagreement")
    for start, (size, digest) in SPANS.items():
        if sha256(image.read(start, size, "text")).hexdigest() != digest:
            raise ValueError(f"VM signature mismatch at {address(start)}")

    def read(at, size):
        return image.read(at, size, "data")

    for opcode, handler in ((2, UPPER_CONTROL), (10, DIRECT_MELEE)):
        if struct.unpack(">I", read(OPCODE_TABLE + 4 * opcode, 4))[0] != handler:
            raise ValueError("VM dispatch slot mismatch")
    borgs, profiles, _ = roster_paths(image, catalog, object_sizes(symbols))
    bindings = selected_banks(installed["borgs"], borgs, profiles)
    unique_roots = {m[k] for m in bindings for k in ("local_bank", "common_bank")}
    bank_map = {root: collect_bank(read, int(root, 16)) for root in sorted(unique_roots)}
    entries = {target for b in bank_map.values() for g in b.get("groups", ()) for target in g.get("choices", ())}
    programs = {entry: describe_program(read, int(entry, 16)) for entry in sorted(entries)}
    counts = Counter(matched_models=len(bindings), observed_profiles=len({b["profile"] for b in bindings}),
                     programs=len(programs), distinct_nonzero_banks=len(bank_map) - ("00000000" in bank_map),
                     refused_choice_tables=len({g["table"] for b in bank_map.values()
                                                for g in b.get("groups", ()) if "status" in g}))
    counts.update(f"bank_{bank['status']}" for bank in bank_map.values())
    counts.update(f"program_{value['status']}" for value in programs.values())
    for program in programs.values():
        for kind in ("first_positive_upper_cue", "first_direct_melee_request", "first_class_event_candidate"):
            if kind in program:
                counts[kind] += 1
                if program[kind]["preceding_waits"] == 0:
                    counts[kind + "_before_any_wait"] += 1
    return {
        "kind": "gf-retail-program-cues", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "catalog_sha256": CATALOG_SHA256, "symbol_metadata_sha256": sha256(symbols).hexdigest(),
        "standalone_installers_sha256": INSTALLER_MAP_SHA256,
        "vm": {"request": "8004BEB8", "interpret": "8004C050", "visit": "8004CD24",
               "opcode_table": address(OPCODE_TABLE), "upper_control": address(UPPER_CONTROL),
               "direct_melee": address(DIRECT_MELEE)},
        "notes": [
            "Indexed bank choices are available data, not evidence that an action selects each program.",
            "As reviewed, opcode 2 writes actor+0x1CEF on lane bit 0x01; a positive byte is a marker, not a shot.",
            "Opcode 10 requests its positive signed queue on lane bit 0x01; no allocation, overlap or damage is implied.",
            "Class-event sites are candidates only; the class callback and its effects are not resolved here.",
            "Preceding waits are counts of static records, not elapsed visits or frames.",
            "Unknown opcodes cut the decoded prefix. A missing cue after that boundary is not proof of absence.",
        ],
        "counts": counts, "models": bindings, "banks": bank_map, "programs": programs,
    }


def export(dol, catalog, symbols, installers, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("program cue map output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    payload = recover(image, read_catalog(catalog), read_bounded(symbols, MAX_SYMBOLS_BYTES),
                      read_bounded(installers, 128 << 10))
    items = []
    for key, value in sorted(payload.items()):
        if key == "models":
            text = "[\n" + ",\n".join("    " + json.dumps(row, sort_keys=True) for row in value) + "\n  ]"
        elif key in ("programs", "banks"):
            text = ("{\n" + ",\n".join(f"    {json.dumps(k)}: {json.dumps(v, sort_keys=True)}"
                                       for k, v in sorted(value.items())) + "\n  }")
        else:
            text = json.dumps(value, sort_keys=True)
        items.append(f"  {json.dumps(key)}: {text}")
    raw = ("{\n" + ",\n".join(items) + "\n}\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "catalog", "symbols", "installers", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.catalog, args.symbols, args.installers, args.output), sort_keys=True))
