"""Join the original Nurse fallback setup, stock and alternate contact."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_alternate_vm as vm


ALTERNATE_VM_SHA256 = "77e8b3fba6a60ad350871d38ceb3e7bea63d4b25d79fa6aa984f7580a5fcadae"
ROUTE = vm.ROUTES[0]
TEXT = (
    *vm.TEXT,
    (0x80079410, 252, "536a91976099ad2cdf0a0d70d24403b26716936e89a21714871ebfba0db5243e"),
    (0x8007BB6C, 116, "c16bb9fb9143e607d31edf6efe44a9cab115ab9ab3da9d302c9e14c3bae446cd"),
    (0x8007BBE0, 552, "fd6ca542007f43f5858765ed5d00f25daf9da3faddcf9978d2d2c7f066eb80b8"),
    (0x8007BE08, 360, "17c115cf0f41a810271499ffa7056a4579f2e1824e605d4d83aa2c64ba01add7"),
    (0x8004BEB8, 360, "af8c9bb8f3fa6387ee864411f266f0757d936550f2ac0cfa50bfc3b2e35390a2"),
    (0x8006DBE0, 224, "0b2e322f452d317b5f515e0ebd95f821a10cdbee5db9cdbd07678c8d251eed31"),
)
LINKS = (
    (0x8007BD84, 0x8004BEB8),
    (0x8004C004, 0x8004C050),
    (0x8004C6F0, 0x8008AC80),
    (0x8007BDC4, 0x8006DBE0),
    (0x8002E084, 0x8002E2A8),
    (0x8002F1D8, 0x8003D344),
)
ROW = re.compile(r"nurse-setup:(?: [0-9]{1,3}){9} (?:0|-200) [0-9]{1,3}")


def decode(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 8:
        raise ValueError("Nurse setup report extent")
    rows = []
    for i, line in enumerate(lines):
        slot, stock, upper = (not (i & flag) for flag in (1, 2, 4))
        admitted = slot and upper
        prev, clock = (1, 120) if stock else (7, 17)
        cue = 4 if upper else 0
        cue_after = cue if stock else 8
        signed, hp = (-200, 180) if admitted else (0, 90)
        expected = (i, upper, admitted, 0, prev, clock, stock, cue_after, 2, signed, hp)
        if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
            raise ValueError(f"Nurse setup shape, stock or HP changed in case {i}")
        rows.append({
            "lane_mask": 3 if upper else 2, "shape_cell_available": slot,
            "group_3_request_before_debit": upper, "shape_admitted_before_debit": admitted,
            "channel_2_stock_before": int(stock), "channel_2_stock_after": 0,
            "channel_2_previous_stock_after": prev, "channel_2_clock_after": clock,
            "stock_latch_after": int(stock), "cue_before_debit": cue, "cue_after_debit": cue_after,
            "substate_after_one_followup": 2, "shape_retained_after_one_followup": admitted,
            "published_at_supplied_frame_21": False, "published_at_supplied_frame_22": admitted,
            "signed_hp_writer_argument": signed if admitted else None, "ally_hp_after": hp,
        })
    return rows


def verify_source(image):
    vm.beam.verify_text(image, TEXT)
    for site, target in LINKS:
        vm.beam.link(image, site, target)
    bank, dispatch = 0x802C2660, 0x802D6668
    group = bank + struct.unpack(">h", image.read(bank+8, 2, "data"))[0]
    program = group + struct.unpack(">h", image.read(group, 2, "data"))[0]
    table = struct.unpack(">II", image.read(dispatch, 8, "data"))
    wait = image.read(ROUTE.program+36, 4, "data")
    if program != ROUTE.program or table != (0x8007BBE0, 0x8007BE08) or wait != bytes.fromhex("0002000e"):
        raise ValueError("Nurse fallback program or class phase table changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Nurse setup output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = vm.DolImage(vm.read_bounded(dol, 8 << 20), vm.SOURCE_SHA256)
    verify_source(image)
    prior = vm.read_metadata(vm.MAPS / "gg4e08-alternate-vm.json", ALTERNATE_VM_SHA256)
    payload, sources = vm.qualified_inputs(image, afs, prior["static_role_program_choices"], (ROUTE,))
    rows = decode(vm.clock.fixture_report(fixture, dol, module, ("nurse-setup",), payload))
    data = {
        "kind": "gf-retail-nurse-setup", "revision": 1,
        "source_dol_sha256": vm.SOURCE_SHA256, "source_afs_sha256": vm.AFS_SHA256,
        "alternate_vm_sha256": ALTERNATE_VM_SHA256,
        "entries": {"normal_installer": "80079410", "class_substate_dispatch": "8007BB6C",
                    "setup_substate": "8007BBE0", "fallback_program_call": "8007BD84",
                    "stock_debit_call": "8007BDC4", "fallback_followup_substate": "8007BE08",
                    "alternate_pair": "8002DF08", "hp_writer": "8003D344"},
        "program": {"local_bank": "802C2660", "choice": [4, 0], "entry": "802C2B36",
                    "first_wait": "802C2B5A", "first_wait_operand": 14, **sources[0]},
        "cases": rows,
        "notes": [
            "Normal Angel Nurse, fallback substate 0 with no target; the installer supplies the retail program and amount table.",
            "The class dispatch runs twice. A separate entry cut at the debit observes the preceding shape bind and unchanged stock.",
            "Lane masks 3 and 2 and mode-0 stock state are declared. The stock clock starts at 17 with a declared refill preset of 120.",
            "Frames 21 and 22 and joint transforms are supplied separately; the VM wait and next class visit do not establish full pose progression.",
            "The alternate pair observes one declared allied Sasuke at 90 of 180 HP. The original writer completes without resuming its caller.",
            "This does not qualify live X selection, directed targeting, original full-frame order, subsequent substates, or native gameplay.",
        ],
    }
    raw = vm.encode(data, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
