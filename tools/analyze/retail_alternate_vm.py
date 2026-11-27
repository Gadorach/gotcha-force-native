"""Bind indexed alternate-role programs to original VM and signed HP observations."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import reaction_emit, reaction_installer_tables, retail_alternate_contact as alternate
from tools.analyze import retail_contact_pose_clock as clock, retail_type32_roster as beam, retail_type76_opening as opening
from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_action_links import ActionLinker, literal_flow
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, ATLAS_SHA256, SOURCE_SHA256, address, callers, encode, read_metadata
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
ALTERNATE_SHA256 = "c004b8cc94cb16cf9ccc7a3c776ed3abd2d89a519e307d5b39c6ea270d7d22a5"
PROGRAM_SHA256 = "fa2a0b4b706f64148184f67b3bf8e5e56974a949844f1a39e47786d7124993f8"
CASES = ("shape-refused", "ally", "opponent", "ally-zero-hp")
ROW = re.compile(r"alternate-vm:(?: [0-9]{1,4}){9} -?[0-9]{1,3}(?: [0-9]{1,4}){8}")
TEXT = (
    *alternate.TEXT,
    (0x8002FD7C, 832, "88d85748f4401cc2a4be18a7c321b2051d29f8e8d284525a611aa91cefa71247"),
    (0x8004C6B4, 168, "1d5f88373d984b7b7afb6a80147ec28f2e39ae3061fce32f4b0a47cd47f5a4a3"),
    (0x8004C050, 584, "1161d002aab162ea3506cd8a088d941fb362861f51cfae3f35ac6b57547d3ab0"),
    (0x8004C7A4, 88, "207bc58eeec6e6a1acab6d44437f4eb9c3a7114d34b4b2d13e8b57eb830eb8ca"),
)


@dataclass(frozen=True)
class Route:
    model: int
    program: int
    opcode: int
    group: int
    step: int
    joint: int
    index: int
    first_frame: int
    seed: int
    raw_power: int
    table: int

    @property
    def record(self):
        return self.table+24*self.index


ROUTES = (
    Route(0x0900, 0x802C2B36, 0x802C2B3A, 3, 5, 1, 3, 22, 15, 200, 0x802D6930),
    Route(0x0900, 0x802C2BEA, 0x802C2BEA, 4, 6, 7, 4,  9,  0, 200, 0x802D6930),
    Route(0x0902, 0x803481B2, 0x803481B2, 3, 5, 1, 3, 16,  4,  10, 0x80348910),
)
BLOCKS = {
    0x0900: ("7c622003c5307b775dcd853c4fa2d1ee0e6129a630e4335f47f53fd61be59602",
             "409d8ba4bb1c7d8c173f9700350f432a4110c2b12ec410798e615e54d9883c88"),
    0x0902: ("7e21667611789bf6790aa60d76b5b2ce518b0a17ce6ad0caee049256c44d492c",
             "e58fda9561ca6bd0f920211fe97dcbd6e8333a4b43e053767abbe9dbf418878a"),
}


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 12:
        raise ValueError("alternate VM report extent")
    rows = []
    for route in ROUTES:
        nurse = route.model == 0x0900
        for i, case in enumerate(CASES):
            slot = i != 0
            same = i in (1, 3)
            before = 0 if i == 3 else 90
            request = (0, -200, -50, -200)[i] if nurse else (0, 2, 8, 2)[i]
            hp = min(180, max(0, before-request)) if before else 0
            secondary = (129, 127, 255) if route.group == 4 else (0, 0, 0)
            timers = route.seed if slot else 0
            ledgers = (8 if i == 2 and not nurse else 0, 2 if same and not nurse else 0, 0 if nurse else request)
            expected = (route.model, route.group, i, slot, *secondary, slot and not nurse, slot, request, hp,
                        slot, timers, timers, *ledgers, slot and route.group == 4)
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"alternate VM relation, signed amount, or HP changed: {route.model:04X}/{route.group}/{case}")
            rows.append({
                "borg": f"gg4e08:{route.model:04x}", "group": route.group, "case": case, "shape_admitted": slot,
                "actor_secondary_state_bytes": list(secondary),
                "waited_at_frame_0_and_8": bool(slot and route.group == 4),
                "same_team": same, "amount_calculator_calls": int(slot and not nurse),
                "hp_writer_reached": slot, "signed_hp_writer_argument": request if slot else None,
                "receiver_hp_before": before, "receiver_hp_after": hp, "hp_change": hp-before,
                "timers_at_hp_writer": [timers, timers] if slot else None,
                "positive_ledgers_at_hp_writer": {"attacker_opponent": ledgers[0], "attacker_ally": ledgers[1],
                                                  "receiver": ledgers[2]} if slot else None,
            })
    return rows


def program_scope(image, cue, prior):
    decoded, states, commands = {}, Counter(), 0
    for key in cue["programs"]:
        result = decode_program(lambda pc, size: image.read(pc, size, "data"), int(key, 16), max_commands=64, max_span=512)
        states[result["status"]] += 1
        decoded[key] = [op for op in result["commands"] if op["opcode"] == 10]
        commands += len(decoded[key])
    if states != {"terminal_wait": 3633, "yielding_loop": 133, "command_budget": 21,
                  "unsupported_opcode": 8, "span_budget": 3} or commands != 837:
        raise ValueError("indexed alternate VM program scan changed")
    if any(bytes.fromhex(op["raw"])[1] == 26 for ops in decoded.values() for op in ops):
        raise ValueError("indexed group-26 opcode scan changed")
    roles = {(r["borg"], r["group"]): r["step"] for r in prior["alternate_shape_rows"]}
    joined = []
    for model in cue["models"]:
        key = model["id"]
        for source in ("local_bank", "common_bank"):
            bank = model[source]
            for i, row in enumerate(cue["banks"][bank].get("groups", ())):
                for j, program in enumerate(row.get("choices", ())):
                    for op in decoded[program]:
                        selector = bytes.fromhex(op["raw"])[1]
                        if (key, selector) in roles:
                            joined.append({"borg": key, "profile": model["profile"], "bank": bank, "program_slot": [i, j],
                                           "program": program, "opcode": address(op["address"]), "queue_group": selector,
                                           "shape_step": roles[key, selector]})
    if len(joined) != 23 or len({r["borg"] for r in joined}) != 9 or any(r["queue_group"] not in (3, 4) for r in joined):
        raise ValueError("indexed alternate VM role join changed")
    return joined, {"programs": len(decoded), "decode_status": dict(states), "opcode_10_occurrences": commands,
                    "group_26_occurrences": 0, "joined_role_0x80_program_choices": 23}


def direct_scope(image, atlas):
    links = callers(image, atlas, 0x8008AC80, 2048)
    linker = ActionLinker(image, {}, ())
    values = Counter()
    for site, row in links.items():
        start = int(row["function"], 16)
        size = atlas["functions"][row["function"]]["size"]
        words, _, graph, _, _, _ = linker.graph(start, size, 3000)
        values[literal_flow(start, words, graph, linker.data).get(site, {}).get(4)] += 1
    if values != {None: 91, 0: 5, 1: 1, 5: 1, 8: 1, 9: 1, 12: 1, 24: 1}:
        raise ValueError("direct binder argument scan changed")
    return {"direct_sites": len(links), "constant_arguments": len(links)-values[None],
            "unresolved_arguments": values[None], "proved_group_26_constants": 0}


def qualified_inputs(image, afs, joined):
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        body, data = opening.read_normal(archive, opening.RECEIVER, opening.RECEIVER_BLOCK_SHA256, ACTOR_SHA256[opening.RECEIVER])
        payload = body+data
        sources = []
        for r in ROUTES:
            key = f"gg4e08:{r.model:04x}"
            if not any((p["borg"], p["program"], p["opcode"], p["queue_group"], p["shape_step"]) ==
                       (key, address(r.program), address(r.opcode), r.group, r.step) for p in joined):
                raise ValueError("alternate VM program is no longer joined to its own installed role")
            if image.read(r.opcode, 4, "data") != bytes((10, r.group, 127 if r.group == 4 else 0, 255)):
                raise ValueError("alternate VM original command changed")
            hit, spec = opening.read_normal(archive, r.model, *BLOCKS[r.model])
            raw = image.read(r.record, 24, "data")
            at = reaction_emit.STEPS+r.step*80
            shape = struct.unpack_from(">BBHHh", hit, at)
            if (shape != (0, r.joint, 0x80, r.index, r.first_frame) or
                reaction_emit.read_group(hit, r.group) != [(r.step, r.index)] or
                reaction_installer_tables.model_table(image, r.model)[1] != r.table or levels(image, r.model)[0][0] != 150 or
                int.from_bytes(raw[:2], "big") != r.raw_power or raw[8] != r.seed):
                raise ValueError("alternate VM exact body or amount data changed")
            payload += hit+spec
            sources.append({"borg": key, "opcode": address(r.opcode), "queue_group": r.group, "step": r.step,
                            "amount_record": address(r.record), "raw_hp_operand": r.raw_power,
                            "first_shape_frame": r.first_frame, "timer_seed": r.seed})
        if archive_stamp(stream) != stamp:
            raise ValueError("alternate VM AFS input changed")
    return payload, sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("alternate VM output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    for site, target in ((0x8004C6F0, 0x8008AC80), (0x8002E084, 0x8002E2A8),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        beam.link(image, site, target)
    prior = read_metadata(MAPS / "gg4e08-alternate-contact.json", ALTERNATE_SHA256)
    joined, scanned = program_scope(image, read_metadata(MAPS / "gg4e08-program-cues.json", PROGRAM_SHA256), prior)
    direct = direct_scope(image, read_metadata(MAPS / "gg4e08-code-atlas.json", ATLAS_SHA256))
    payload, sources = qualified_inputs(image, afs, joined)
    rows = decode(clock.fixture_report(fixture, dol, module, payload=payload))
    data = {
        "kind": "gf-retail-alternate-vm", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "code_atlas_sha256": ATLAS_SHA256, "alternate_contact_sha256": ALTERNATE_SHA256, "program_cues_sha256": PROGRAM_SHA256,
        "entries": {"opcode_10": "8004C6B4", "queue_bind": "8008AC80", "interpreter": "8004C050",
                    "alternate_pair": "8002DF08", "amount": "8003CD5C", "hp_writer": "8003D344"},
        "indexed_program_scan": scanned, "direct_binder_scan": direct, "static_role_program_choices": joined,
        "sources": sources, "cases": rows,
        "notes": [
            "Group 26 had no indexed opcode-10 or proved direct binder constant. This does not exhaust dynamic selector values or live triggers.",
            "Three exact normal indexed programs run in a declared lane and pose, with no inferred controller input, stock debit or VM continuation.",
            "Group-3 visits stop at the next clip loader. Group 4 returns after installing the wait and its secondary actor bytes, even if no shape allocates.",
            "The group-4 shape remains active but unpublished at frames 0 and 8. The fixture supplies frame 9 rather than replaying a live phase schedule.",
            "The original signed HP writer completes, but its pair caller does not resume after that observation cut.",
            "A zero-HP receiver shape is deliberately published as a writer control; this is not a scene revival or live dead-body claim.",
            "No emitted visuals, stock/accounting after HP, whole-match timing or native gameplay are qualified.",
        ],
    }
    raw = encode(data, expanded=("static_role_program_choices", "sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "static_program_choices": len(joined)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
