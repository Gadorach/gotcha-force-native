"""Verify the original damage setup writers and their joined scalar result."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze.damage_contact_indices import recover as recover_indices, scene_setting_writes
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_contact_hp import factor_rows
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive
from tools.extract.source_image import DolImage

SLICES = {
    0x8005438C: (140, "d475c1886c214369ba4ea322131dda6fd43e2cc68132404cd436eab961be7735"),
    0x80056180: (156, "486bd0799d577e9bd455897f5572585962c913b0bae6630d35e369d3bc8c32dc"),
}
CHECKS = {"settings_writes": 13, "controller_bank_writes": 96, "rank_copies": 30}
JOINED = ((2, 0, 3, 0, 0, 35), (2, 0, 1, 0, 0, 40),
          (2, 0, 1, 3, 0, 47), (1, 0, 1, 0, 0, 79))
JOINED_ROW = re.compile(r"setup-amount:(?: [0-9]{1,2}){10}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 1 + len(JOINED):
        raise ValueError("damage setup fixture report extent")
    header = "setup-checks: " + " ".join(map(str, CHECKS.values()))
    if lines[0] != header:
        raise ValueError("damage setup fixture did not complete all checks")
    out = []
    for line, expected in zip(lines[1:], JOINED, strict=True):
        if not JOINED_ROW.fullmatch(line):
            raise ValueError("malformed joined amount row")
        state, bias, mask, setup0, setup1, bank0, bank1, rank0, rank1, amount = map(int, line.split()[1:])
        if (state, bias, mask, rank0, rank1, amount) != expected:
            raise ValueError("unexpected joined settings, ranks or amount")
        if ([setup0, setup1] != scene_setting_writes(state, bias)[:2] or
                (bank0, bank1) != (int(not (mask & 1)), int(not (mask & 2)))):
            raise ValueError("joined writer outputs disagree with the declared scene inlets")
        out.append({
            "mode_state": state, "scene_bias": bias, "input_mask": mask,
            "settings": [setup0, setup1], "banks": [bank0, bank1],
            "ranks": [rank0, rank1], "amount": amount,
            "native_cli_indices": f"{setup0},{setup1}:{rank0},{rank1}:{bank0},{bank1}",
        })
    return out


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("damage setup output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    indices = recover_indices(image)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"damage setup slice changed at {address(entry)}")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        member, _ = archive.member(archive.names.index(b"pl000a.pzz"), 0x000A)
        spec, _ = member.read(0, ACTOR_BYTES)
        if archive_stamp(stream) != stamp:
            raise ValueError("damage setup source AFS changed during extraction")
    base, _ = factor_rows(image, spec)
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, input=spec, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"damage setup fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    joined = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-damage-setup", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "source": {"actor_spec_sha256": sha256(spec).hexdigest(), "model": "000A",
                   "attack_record": "802D4190", "raw_power": base},
        "checks": CHECKS, "joined": joined,
        "writers": {
            "settings": {
                "entry": indices["source"]["setting_writer"], "array": indices["source"]["setting_array"],
                "mode_1": "writes bias & 31 and (31 - bias) & 31; leaves the other two bytes unchanged",
                "mode_2": "writes 7 to all four bytes",
                "other_modes": "no write in the tested modes 0, 3 and 255",
            },
            "controller_bank": {
                "constructor_slice": "8005438C", "stop_before": "80055F90",
                "inlet": "assigned actor+0x3E5 bit and Mode+2 mask",
                "outputs": "actor+0x3E6 and +0x488 become 0 on intersection, 1 otherwise",
                "extra_output": "actor+0x3F3 becomes 1 only on the non-intersecting branch outside mode 2",
            },
            "rank": {
                "initialization_slice": "80056180", "stop_before": "8006826C",
                "source": indices["source"]["rank_source"],
                "output": "actor+0x43A; raw byte copied without clamping",
            },
        },
        "notes": [
            "These fragments receive a declared Scene, mode and actor slots, not a recreated source scheduler.",
            "Joined cases run all three original writer fragments before executing 8003CD5C on the exact normal Sasuke spec.",
            "Join holds teams fixed at 0 and 1 with slot 0 credited and slot 1 receiving; caps/mirrors are declared at 180.",
            "Ranks outside the table are tested only at the write boundary, never as damage inputs.",
            "The authored native bank choices remain explicit. The receipt does not choose live slots, manufacture a hit, or write HP.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **CHECKS, "joined": len(joined)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
