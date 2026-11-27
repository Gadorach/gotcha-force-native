"""Verify same-pass Ninja area allocation before class-4 shape recycling."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_bomb_floor_contact as contact
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_bomb_area import AMOUNT
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


PILOTS = contact.floor.PILOTS
SLICES = {
    0x8008AFF0: (32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
    0x80088E50: (328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
}
ROW = re.compile(r"bomb-actor-area:(?: [0-9]{1,3}){12}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 512 or len(lines) != 2*len(PILOTS):
        raise ValueError("bomb actor area report extent")
    result = []
    for i, model in enumerate(PILOTS):
        cases = []
        for spare, line in enumerate(lines[2*i:2*i+2]):
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed bomb actor area row")
            fields = tuple(map(int, line.split()[1:]))
            if fields != (model, spare, 1, spare, spare, spare, 1, 0, 155 if spare else 172, 1, 2*spare, 1):
                raise ValueError(f"bomb area recycled too early or changed damage: {model:04X}/{spare}")
            _, _, pending, available, cue, bound, returned, retries, hp, opening_token, area_token, stock = fields
            cases.append({
                "opening_hit": {"amount_record": "802D4178", "damage": 8, "receiver_hp": 172, "token": opening_token},
                "same_bucket_area_bind": {
                    "shape_slots_free": available, "opening_unlink_requested": bool(pending),
                    "opening_shape_still_linked": True, "area_bound": bool(bound), "audio_request": 12 if cue else None,
                },
                "first_class4_pass_after_impact": {
                    "opening_shape_returned": True, "shape_slots_free": returned,
                    "registered_attacks": 0, "area_token": area_token or None,
                },
                "growth_visit_one": {
                    "area_bound": bool(bound), "registered_radius": 34.1 if bound else None,
                    "amount_record": address(AMOUNT) if bound else None, "damage": 17 if bound else 0,
                },
                "later_area_bind_attempts": retries, "receiver_hp_after_two_growth_visits": hp,
                "receiver_hit_tokens": [opening_token, area_token] if bound else [opening_token],
                "credited_and_received_damage": 180-hp, "stock_after": stock,
            })
        result.append({"borg": f"gg4e08:{model:04x}", "cases": cases})
    return result


def qualified_inlet(image, archive):
    for at, (size, digest) in SLICES.items():
        if sha256(image.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb actor area source signature mismatch: {address(at)}")
    return contact.qualified_inlet(image, archive)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb actor area output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, common, source = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("bomb actor area AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--actor-area"]
    run_result = run(argv, input=inlet, capture_output=True, timeout=45)
    if run_result.returncode or run_result.stderr:
        raise ValueError(f"bomb actor area fixture failed: {run_result.stderr.decode('utf-8', 'replace').strip()}")
    pilots = decode_report(run_result.stdout.decode("ascii"))
    receipt = {
        "kind": "gf-retail-bomb-actor-area", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "common_collision": common,
        "pilots": [{**pilot, "source": origin} for pilot, origin in zip(pilots, source, strict=True)],
        "entries": {
            "bucket_visit": "80089A94", "contact_child_request": "8007E284", "area_bind": "8008AE3C",
            "request_unlink": "8008AFF0", "class4_pass": "80089AEC", "recycle_cell": "80088E50",
        },
        "receiver": {"borg": "gg4e08:000a", "initial_hp": 180, "body_step": 9, "declared_center_offset_x": 30},
        "notes": [
            "Use the original paid leaves and body overlap. The HP-writer leaf returns without resuming its stopped caller.",
            "The original bucket pass appends and initializes the area before the separate class-4 pass.",
            "The parent unlink byte is set before the area bind, but its shape is not yet free.",
            "The sound frontier stops before playback. Later separate class-4 passes return the parent shape and see the area grow only if its initial bind succeeded.",
            "These results do not establish live timing, projectile movement through other actors, or native shared collision slots.",
        ],
    }
    raw = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots), "cases": 2*len(pilots)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
