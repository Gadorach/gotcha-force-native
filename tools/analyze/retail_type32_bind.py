"""Replay carrier versus segment ownership in the borrowed-endpoint type-32 family."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_type32_roster as roster
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


PILOTS = (
    (0x0D03, 8, 1, 0x80362C28, 30, "2eedbece07b7a5ddeecd7b6c8ad3731cc176b8e19666ae42033a2c0054b80cf3"),
    (0x0E02, 9, 6, 0x8038DAE0, 10, "11467f3a1c1491e490093fa99fd3ff69b244b495203fe9dd5de05bd4b00e8e6a"),
)
CASES = ("scratch-empty", "shape-empty", "shape-available", "segment-carrier-empty")
ROW = re.compile(r"type32-branch:(?: [0-9]{1,4}){11}")
TEXT = (
    (0x80006DC8, 116, "a9d42d4b031609acdc4056ee2c21d561240c52207e9ba085f640a656a1129282"),
    (0x80088C40, 344, "9d8ce985c9150d0447eee638dce354da533c2dd9ce8d57b54a2444dff49331db"),
    (0x80139B80, 60, "a1cdcf3995d20673205a7b8a52aa4f7d5161159e4e536c7d64eff6e2e41b9d3e"),
    (0x80139C50, 40, "ffcd4804a903c9f446d1cd9afe57039f3b07397ae8ba702309044a6f99784d3c"),
    (0x801DF72C, 76, "11aa973135e3e4ebe389c7a48ea69074ad121fcc3c57688664fed6db6dd7499d"),
    (0x801DF878, 144, "6a5d8c892832fceb37045762b4c695e98c60713cb5ee20e092d663b4a71ade27"),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 7:
        raise ValueError("type-32 branch report extent")
    rows = []
    for model, key, *_ in PILOTS:
        direct = key == 8
        for n, case in enumerate(CASES[:3] if direct else CASES):
            line = lines[len(rows)]
            if not ROW.fullmatch(line):
                raise ValueError("malformed type-32 branch row")
            supplied = n != 0
            attempted = direct and supplied
            parent_bound = direct and n == 2
            requested = supplied and not direct
            created = requested and n != 3
            bound = created and n == 2
            front = supplied and (not direct or parent_bound)
            observed = tuple(map(int, line.split()[1:]))
            if observed != (model, key, n, supplied, attempted, parent_bound, requested, created, bound, bound, front):
                raise ValueError(f"type-32 branch ownership or endpoint mismatch: {model:04X}/{case}")
            rows.append({
                "borg": f"gg4e08:{model:04x}", "carrier_key": key, "case": case, "scratch_inlet_supplied": supplied,
                "returned_null_from_factory": True if not supplied else None, "carrier_already_published": True,
                "carrier_shape_attempt": attempted, "carrier_shape_bound": parent_bound,
                "reached_pose_setup_cut": front, "active_segment_requested": requested,
                "segment_carrier_allocated": created, "segment_shape_bound": bound,
                "group_bind_attempted": 1 if attempted or created else None, "segment_key_requested": 1 if requested else None,
                "segment_samples": {"first_endpoint": [100, 200, 300], "updated_second_endpoint": [250, 270, 410]}
                if bound else None,
            })
    return rows


def qualified_inlet(image, archive):
    _, rows = roster.row_index(image)
    roster.verify_text(image, TEXT)
    roster.link(image, 0x801DEC64, 0x80006DC8)
    if (struct.unpack(">4I", image.read(0x80339660, 16, "data"))
            != (0x80139BBC, 0x80139C50, 0x80139C78, 0x80139CA0)):
        raise ValueError("type-11 phase dispatch changed")
    dispatch = struct.unpack(">10I", image.read(0x80391658, 40, "data"))
    if dispatch[8:10] != (0x801DF878, 0x801DF72C):
        raise ValueError("type-32 key-local active worker changed")
    payload, sources = [], []
    for model, key, index, table, power, digest in PILOTS:
        row = rows[roster.SCRATCH, key]
        collision = row["collision_path"]
        block = reaction_emit.qualified_hit_block(archive, model)
        if (sha256(block).hexdigest() != digest or reaction_emit.read_group(block, 1) != [(1, index)]
                or reaction_installer_tables.model_table(image, model)[1] != table
                or image.read(table+index*24, 2, "data") != struct.pack(">H", power)
                or collision["selector"] != 1 or row["collision_mode_byte"] != int(key == 9)):
            raise ValueError(f"type-32 normal shape source mismatch: {model:04X}")
        payload.append(block)
        sources.append({"borg": f"gg4e08:{model:04x}", "carrier_key": key, "row": row["row"],
                        "collision_mode_byte": row["collision_mode_byte"], "descriptor": 1,
                        "reaction_index": index, "reaction_table": address(table), "raw_power": power,
                        "hit_block_sha256": digest})
    return b"".join(payload), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-32 branch output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-32 branch AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-32 branch fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type32-bind", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256, "sources": sources, "cases": cases,
        "entries": {"carrier_constructor": "801DEB68", "scratch_request": "801DEC64", "carrier_update": "801DEC90",
                    "carrier_bind": "801DED2C", "carrier_active": "801DEF80", "segment_request": "801DF2B0",
                    "segment_factory": "80139A6C", "segment_bind": "80139BE0", "pose_setup_cut": "80048288"},
        "notes": [
            "The original constructor publishes a carrier before requesting scratch. Scratch refusal returns null and leaves a phase-two carrier.",
            "Successful initialization uses a supplied scratch block and ends before pose or renderer setup.",
            "The active callback uses a declared inner phase, endpoints and clear stage. This is not a continuous timing replay.",
            "The type-11 child is allocated by the active callback and retains the original carrier endpoint addresses.",
            "Shape identity and endpoint sampling are measured; the attack registry, overlap, HP and native gameplay are not.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
