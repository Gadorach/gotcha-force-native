"""Join the ordinary Ninja X program request to its generated cue and paid release."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_bomb_release import SOURCE_SHA256, source_operands
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_program_cues import OPCODE_TABLE, SPANS as VM_SPANS, UPPER_CONTROL
from tools.analyze.retail_small_projectile_roster import EXTRA_TEXT, NINJA_BOMBS
from tools.extract.source_image import DolImage


# Exact normal selected profile, installer, configuration, local program bank.
PILOTS = {
    0: ("p000", 0x8006F4F8, 0x802D3DC8, 0x802BDE80),
    3: ("p004", 0x8006F60C, 0x8033AF78, 0x802BDE80),
    10: ("p013", 0x8006F4F8, 0x802D3EE0, 0x802BDE80),
    12: ("p011", 0x8019E9A4, 0x803766F8, 0x80376A60),
}
# Program entry, first wait/cursor, positive upper command, final observed cursor, visits.
NINJA = (
    (0x802BE5EE, 0x802BE606, 0x802BE61E, 0x802BE62E, 21, (2, 11, 14),
     "cf04f726bad0f8f07356840ccec3430ae357caba1b9440a480e39734bde02224"),
    (0x802BE656, 0x802BE672, 0x802BE682, 0x802BE692, 21, (1, 14),
     "d37cae4ed0ee4ebc5b7b9f5984613b2756087cf9d942e9b90e205f46040e1763"),
)
ALPHA = (
    (0x80377188, 0x803771A0, 0x803771AC, 0x803771BC, 22, (11, 15),
     "8635cb1a27298f245c4fcc9221319c07fb6508f7edb22fb6eb4dfa3e9d83dee2"),
    (0x803771E0, 0x803771FC, 0x80377200, 0x80377210, 22, (15,),
     "48125f238db67598cc0258a0a8957420fd0a5ae05120a3551de177f4d4a7ed1f"),
)
SLICES = {
    **{at: (size, digest) for at, size, digest in EXTRA_TEXT
       if at in (0x800715D4, 0x800FF2BC, 0x800FF418, 0x801A0414, 0x801A0434)},
    **{entry: VM_SPANS[entry] for entry in (0x8004BEB8, 0x8004C050, 0x8004CD24, UPPER_CONTROL)},
    0x800FF30C: (268, "72404d4312c0508eb0b293dcfc9a338bbdbad9f719b1a4fedc7cf217c2904f97"),
    0x80071598: (60, "4ac9a8f7c2f0b6fbac5305b3c0db122125ccfe4e454ffe8e0924314cd3a7daf9"),
    0x800677B0: (544, "09ab80886742547a9862056d729a3b23984007549f8cd636ea2d5cf3e63bf246"),
    0x8006D144: (100, "9192553250e34d5b4747df230f7eea747fbdeaa373bdf5436c086e45021c8707"),
    0x8006F4F8: (276, "9bedc00559917b9e5fa76d02977c59be604bed473605fc394e3df7af750ee800"),
    0x8006F60C: (252, "201ead69a9e7e0cb04851db5f7498352ebdf5cbecc88be4486e68e2122e3e962"),
    0x8019E9A4: (276, "eb17cacd3cfb7555fa0a4ebb50f45408b09122925c2ba97d67956d3ccc9c34a7"),
}
SCENARIOS = ("paid", "stock-empty", "carrier-pool-empty", "upper-lane-masked")
ROW = re.compile(r"bomb-x-cue:(?: [0-9]{1,3}){3}(?: [0-9A-F]{8}){3}(?: [0-9]{1,3}){8}")
MASKED_VISITS = 24


def decode_report(report):
    lines = iter(report.splitlines())
    if len(report) > 4096 or len(report.splitlines()) != len(PILOTS)*2*len(SCENARIOS):
        raise ValueError("bomb X cue report extent")
    result = []
    for model in PILOTS:
        rows = []
        for flag, (start, first, _, end, age, _, _) in enumerate(ALPHA if model == 12 else NINJA):
            for scenario, name in enumerate(SCENARIOS):
                line = next(lines)
                if ROW.fullmatch(line) is None:
                    raise ValueError("malformed bomb X cue row")
                args = line.split()[1:]
                values = tuple(map(int, args[:3]))+tuple(int(a, 16) for a in args[3:6])+tuple(map(int, args[6:]))
                cue, paid, made = scenario != 3, scenario in (0, 2), scenario == 0
                expected = (model, flag, scenario, start, first, end if cue else first, age if cue else MASKED_VISITS,
                            age if cue else 0, 2 if cue else 1, int(cue), int(paid), int(made),
                            int(not cue), 300 if paid else 0)
                if values != expected:
                    raise ValueError(f"bomb X cue mismatch: {model:04X}/{flag}/{name}")
                rows.append({
                    "case": name, "declared_actor_flag_5e0_bit_0x40": bool(flag),
                    "program": address(start), "cursor_after_request": address(first),
                    "cursor_at_observation_end": address(values[5]), "phase_one_visits": values[6],
                    "positive_cue_first_visit": age if cue else None, "phase_after": values[8],
                    "phase_one_lane_mask": 3 if cue else 2,
                    "stock_before": int(scenario != 1), "debit_attempts": values[9],
                    "factory_calls": values[10], "carrier_created": made, "stock_after": values[12],
                    "refill_clock": values[13], "constructor_origin": [205, 220, 345] if made else None,
                })
        result.append({"borg": f"gg4e08:{model:04x}", "cases": rows})
    return result


def qualify(image):
    source_operands(image)
    for at, (size, digest) in SLICES.items():
        if sha256(image.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb X source signature mismatch: {address(at)}")
    for at, expected in ((0x8031F000, (0x800FF30C, 0x800FF418)),
                         (0x80433868, (1, 0x800715F8)), (0x80435750, (1, 0x801A0458)),
                         (0x802D3C48, (0x800715D4,)), (OPCODE_TABLE+8, (UPPER_CONTROL,))):
        if struct.unpack(f">{len(expected)}I", image.read(at, 4*len(expected), "data")) != expected:
            raise ValueError(f"bomb X dispatch data mismatch: {address(at)}")

    def read(at, n):
        return image.read(at, n, "data")

    def relative(base, index):
        return base+struct.unpack(">h", read(base+2*index, 2))[0]

    sources = []
    for model, (profile, installer, config, bank) in PILOTS.items():
        choice_table = relative(bank, 4)
        variants = []
        for choice, (start, first, cue, end, _, waits, digest) in enumerate(ALPHA if model == 12 else NINJA):
            if relative(choice_table, choice) != start or sha256(read(start, end-start)).hexdigest() != digest:
                raise ValueError(f"bomb X program prefix mismatch at {address(start)}")
            commands = decode_program(read, start, max_commands=20, max_span=128)["commands"]
            resets = [c for c in commands if c["opcode"] == 2 and c["address"] < first]
            waiting = [c for c in commands if c["opcode"] == 0 and c["address"] < cue]
            positive = next((c for c in commands if c["address"] == cue), None)
            if (len(resets) != 1 or bytes.fromhex(resets[0]["raw"])[2] != 0
                    or not waiting or waiting[0]["address"] != first
                    or tuple(c["threshold"] for c in waiting) != waits or any(c["flags"] != 2 for c in waiting)
                    or positive is None or positive["opcode"] != 2 or bytes.fromhex(positive["raw"])[2] != 1):
                raise ValueError(f"bomb X wait or opcode-2 operand mismatch at {address(start)}")
            variants.append({
                "choice": choice, "program": address(start), "prefix_sha256": digest,
                "opening_clear_site": address(resets[0]["address"]), "positive_cue_site": address(cue),
                "waits": [{"site": address(w["address"]), "flags": w["flags"], "threshold": w["threshold"]} for w in waiting],
            })
        factory_site, key = NINJA_BOMBS[model][0]
        if read(config+2, 1) != b"\x03" or struct.unpack(">3f", read(0x802D6D68+56*key+4, 12)) != (55, 0, 0):
            raise ValueError(f"bomb X lane seed or socket operand mismatch: {model:04X}")
        sources.append({
            "profile": profile, "installer": address(installer), "configuration": address(config),
            "normal_lane_mask_seed": 3, "local_bank": address(bank), "choice_table": address(choice_table),
            "program_group": 4, "requested_lane_mask": 15, "programs": variants,
            "factory_key": key, "factory_call_site": address(factory_site), "constructor_joint": 11,
            "debit_call_site": "801A04B8" if model == 12 else "80071658", "stock_channel": 2, "stock_cost": 1,
        })
    return sources


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb X cue output already exists")
    dol = Path(dol).resolve(strict=True)
    source = qualify(DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256))
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--x-cue"]
    result = run(argv, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb X cue fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    pilots = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-bomb-x-cue", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "pilots": [{**pilot, "source": inputs} for pilot, inputs in zip(pilots, source, strict=True)],
        "entries": {"program_request": "8004BEB8", "phase_zero": "800FF30C", "phase_one": "800FF418",
                    "vm_visit": "8004CD24", "cue_writer": address(UPPER_CONTROL), "consumer_cue_byte": "0x1CEF"},
        "inlets": {"actor_state": 2, "actor_substate": 0, "movement_inhibit_byte_0x7c9": 1,
                   "time_multiplier": 1, "double_ninja_clone_flag": 0},
        "notes": [
            "The original installer selects the checked normal bank. The fixture starts at its ordinary-X handler with state declared.",
            "The original phase-one handler advances the VM and consumes its own generated positive cue. No positive cue is seeded.",
            "The debit observation reruns the one cue visit from the same entry snapshot, without returning a supplied helper result.",
            "The later logical-key-11 pose is supplied; no animation evaluator, live controller, target solve, clock producer, or world visit runs.",
            "Counts are bounded phase-one visits, not hardware frames or native release markers.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots), "cases": len(pilots)*2*len(SCENARIOS)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
