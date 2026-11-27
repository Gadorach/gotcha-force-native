"""Verify an ordinary type-0 flight stop with a declared carrier contact bit."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.reaction_emit import qualified_hit_block, read_group
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_origins import origin_tables
from tools.analyze.retail_type0_roster import AFS_SHA256, FACTORY, SOURCE_SHA256, carrier_rows
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage

MODEL, KEY, SITE = 0x0802, 45, 0x8013D77C
HIT_BLOCK_SHA256 = "c72cfa05a63ea7142e55265bd47bcefb9280073d1ae90a9a04c4bf653fa630a1"
EXPECTED = (
    (0, 1, 1, 0, 299, 1, 0, 0, 0, 370),
    (16, 1, 2, 1, 299, 0, 1, 0, 0, 350),
    (16, 2, 3, 1, 299, 0, 1, 0, 0, 350),
    (16, 3, 3, 1, 299, 0, 1, 1, 1, 350),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 512 or len(lines) != len(EXPECTED):
        raise ValueError("type-0 flight report extent")
    samples = []
    for line, expected in zip(lines, EXPECTED, strict=True):
        if re.fullmatch(r"type0-flight:(?: [0-9]{1,3}){9} [0-9]{1,4}\.[0-9]{5}", line) is None:
            raise ValueError("malformed type-0 flight sample")
        *fields, z = line.split()[1:]
        row = (*map(int, fields), float(z))
        if row != expected:
            raise ValueError("type-0 flight or pool return disagreement")
        bit, visit, phase, stopped, timer, attacks, shape, carrier, descriptor, position = row
        samples.append({
            "declared_contact_byte": bit, "visit": visit, "carrier_phase": phase, "carrier_stopped": bool(stopped),
            "carrier_timer": timer, "attack_registry_count": attacks, "shape_free_count": shape,
            "carrier_free_count": carrier, "descriptor_free_count": descriptor, "carrier_z": position,
        })
    return samples


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-0 flight output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    operand, call = struct.unpack(">2I", image.read(SITE-4, 8, "text"))
    if operand != 0x38800000 | KEY or call & 0xFC000003 != 0x48000001 or direct_branch(SITE, call) != FACTORY:
        raise ValueError("Gold Hero type-0 factory call mismatch")
    row = carrier_rows(image, origin_tables(image)["type-0"])[KEY]
    if (row["source_joint_selector"], row["collision_group_selector"], row["launch_speed"]) != (12, 0, 20):
        raise ValueError("Gold Hero type-0 launch operand mismatch")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        block = qualified_hit_block(RetailArchive(stream), MODEL)
        if stamp != archive_stamp(stream):
            raise ValueError("type-0 flight AFS changed during extraction")
    if sha256(block).hexdigest() != HIT_BLOCK_SHA256 or read_group(block, 0) != [(0, 0)]:
        raise ValueError("Gold Hero type-0 hit block or bind group mismatch")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--type0-flight"]
    result = run(argv, input=block, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-0 flight fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    samples = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type0-flight-control", "revision": 1, "borg": "gg4e08:0802",
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256, "hit_block_sha256": HIT_BLOCK_SHA256,
        "static_factory_call": address(SITE), "declared_factory_key": KEY, "carrier_row": row,
        "group": 0, "descriptor": 0, "initial_timer": 300, "initial_carrier_z": 350,
        "initial_attack_registry_count": 1, "stop_before_audio": "800F036C",
        "notes": [
            "The test calls the original ordinary factory directly with the source key; it does not run Gold Hero's action, cue or debit.",
            "The fixture supplies a rigid joint basis, an empty stage, no effect capacity and no visual payload.",
            "The original lists and registry are used. The first type-0 visit consumes a declared contact byte; no receiver or pair loop supplies it.",
            "This does not assign the same outcome to every type-0 key, alternate factory, live collision, or native move.",
        ],
        "samples": samples,
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "samples": len(samples)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
