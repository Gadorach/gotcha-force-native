"""Observe the original Machine Red rifle's movement-before-acceleration order."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.reaction_emit import qualified_hit_block
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_rifle_bucket import HIT_BLOCK_SHA256, source_shape
from tools.analyze.retail_rifle_contact_hp import PUBLISHED, RELEASE, SOURCE_SHA256
from tools.analyze.retail_rifle_lifetime import SLICES
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage

KEY, ROW = 21, 0x803036CC
VISITS = (1, 2, 159, 160, 161, 162)
SAMPLE = re.compile(r"rifle-speed: [0-9]{1,3}(?: [0-9]{1,3}\.[0-9]{2}){2} [0-9]{1,3}\.[0-9]{5} [0-9]{1,3}")
FLIGHT_SLICES = {
    0x800C4088: SLICES[0x800C4088],
    0x800C4540: (452, "1ea83420b1d490197d766ac2e7dcb4a1abbeb3d5800642938c8d3243ceb15e8c"),
    0x80089A94: SLICES[0x80089A94],
    0x800C3D2C: SLICES[0x800C3D2C],
}


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(VISITS)+2 or lines[:2] != [RELEASE, PUBLISHED]:
        raise ValueError("rifle speed report prefix or extent")
    samples = []
    for line, visit in zip(lines[2:], VISITS, strict=True):
        if SAMPLE.fullmatch(line) is None:
            raise ValueError("malformed rifle speed sample")
        n, before, after, distance, timer = line.split()[1:]
        used, next_speed, delta = map(float, (before, after, distance))
        expected = min(160.0, 80+0.5*(visit-1))
        if (int(n), used, next_speed, delta, int(timer)) != (
                visit, expected, min(160.0, expected+0.5), expected, 300-visit):
            raise ValueError(f"rifle speed order disagrees at visit {visit}")
        samples.append({"visit": visit, "speed_before": used, "position_delta_z": delta,
                        "speed_after": next_speed, "timer_after": int(timer)})
    return samples


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("rifle speed output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in FLIGHT_SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"rifle flight source signature mismatch: {address(entry)}")
    initial, gain, cap = struct.unpack(">3f", image.read(ROW+44, 12, "data"))
    if (initial, gain, cap) != (80, 0.5, 160):
        raise ValueError("rifle speed operands disagree")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        block = qualified_hit_block(RetailArchive(stream), 0x0600)
        if stamp != archive_stamp(stream):
            raise ValueError("rifle speed AFS changed during extraction")
    source_shape(block)
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--rifle-speed"]
    result = run(args, input=block, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"rifle speed fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    samples = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-rifle-speed", "revision": 1, "borg": "gg4e08:0600",
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "source_hit_block_sha256": HIT_BLOCK_SHA256, "carrier_key": KEY, "carrier_row": address(ROW),
        "active_update": "800C4088", "movement": "800C4540",
        "speed": {"initial": initial, "gain": gain, "cap": cap},
        "checked_visits": 162, "samples": samples,
        "notes": [
            "Continues the original paid release and first attack-shape registration on a declared empty stage.",
            "Each isolated carrier visit uses the old speed, then writes the increased and clamped scalar.",
            "The pose is supplied and the trajectory is cardinal. No live clock, steering, collision, or native conversion is implied.",
        ],
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
