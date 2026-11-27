"""Verify original Machine Red rifle flight, shape unlink and pool return."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_rifle_contact_hp import (
    AFS_SHA256, PUBLISHED, RELEASE, SLICES as CONTACT_SLICES, SOURCE_SHA256, load_contact_inputs,
)
from tools.extract.source_image import DolImage

SLICES = {
    **CONTACT_SLICES,
    0x800C4088: (484, "1c5b38b48587b163a131c1fb801e92d00c3476f04fdedc10ba7500875b6e9642"),
    0x800C42A8: (20, "9212e9caa3a25e54a968e0a58a25f7946023df4677e5701fd0ee94e4396d8819"),
    0x800C42BC: (76, "913527bd5679824ca3f2390abfa8055f5be3b97c5fe166f6e3d18156c1f6b5c4"),
    0x800C4308: (320, "a709bf70226c9b4cce0a5593fda39b4fd28178c0045271d7d88892ba90f9d9c5"),
    0x800C4448: (32, "c696ee8b212576aa6b92c6c2303db825890accc8aaf58280e96bed9a8fafc9f1"),
    0x80083244: (128, "0d12380007430d1784fc3a5d0183bdd2c9b40b58252868175125d47bbd3e48c4"),
    0x80089554: (260, "14d54d2183c44456974e44418019670cfd857a373e5baced583924c4c74280df"),
    0x80088E50: (328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
    0x80089814: (132, "1bb689f99364d17b27d89cf9d15ceb89ffde7be29dc90d4bba30b495975710dd"),
    0x80197AD8: (188, "e3650544ce98694f5c3a5ca208c4dfe1479fbc02f46712a3a8a75d8d210c9082"),
}
EMPTY = "rifle-bucket: 1 0 0 2 300 0 0 0 0 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000"
HIT = "rifle-contact: 0 0 0 1 8 172 180 8 8 16 1 1 2 802C3494"
ROW = re.compile(r"rifle-life:(?: [0-9]{1,3}){12}(?: -?[0-9]{1,5}\.[0-9]{5}){2}")
CASES = ("timer-expiry", "after-accepted-contact", "declared-stop-bit", "empty-shape-pool", "stage-bounds-exit")
# Visit, phase, subphase, stopped flag, contact flags, timer, registered,
# free shape/carrier/descriptor counts, z, and carrier scale.
LAUNCHED = (0, 1, 0, 0, 0, 300, 1, 0, 0, 0, 365, 1.62338)
FLYING = (1, 1, 0, 0, 0, 299, 1, 0, 0, 0, 445, 1.62338)
EXPECTED = (
    (LAUNCHED, FLYING, (300, 2, 2, 1, 0, 0, 0, 1, 0, 0, 41925, 1.62338),
     (334, 3, 2, 1, 0, 0, 0, 1, 0, 0, 47365, 0.04515), (335, 3, 2, 1, 0, 0, 0, 1, 1, 1, 47365, 0.04515)),
    ((0, 1, 0, 0, 16, 300, 1, 0, 0, 0, 365, 1.62338), (1, 1, 0, 0, 16, 299, 1, 0, 0, 0, 445, 1.62338)),
    ((0, 1, 0, 0, 2, 300, 1, 0, 0, 0, 365, 1.62338), (1, 2, 1, 1, 2, 300, 0, 1, 0, 0, 445, 1.62338),
     (3, 3, 1, 1, 2, 300, 0, 1, 0, 0, 445, 1.62338), (4, 3, 1, 1, 2, 300, 0, 1, 1, 1, 445, 1.62338)),
    ((0, 2, 0, 0, 0, 300, 0, 0, 0, 0, 0, 0), (1, 3, 0, 0, 0, 300, 0, 0, 0, 0, 0, 0),
     (2, 3, 0, 0, 0, 300, 0, 0, 1, 1, 0, 0)),
    (LAUNCHED, FLYING, (237, 2, 2, 1, 0, 64, 0, 1, 0, 0, 31845, 1.62338),
     (271, 3, 2, 1, 0, 64, 0, 1, 0, 0, 37285, 0.04515), (272, 3, 2, 1, 0, 64, 0, 1, 1, 1, 37285, 0.04515)),
)


def decode_report(report):
    if len(report) > 4096:
        raise ValueError("rifle lifetime report extent")
    lines = report.splitlines()
    if len(lines) != 2 * len(CASES) + sum(map(len, EXPECTED)) + 1:
        raise ValueError("incomplete rifle lifetime report")
    cursor = 0
    cases = []
    for key, (case, expected) in enumerate(zip(CASES, EXPECTED, strict=True)):
        if lines[cursor:cursor+2] != [RELEASE, EMPTY if case == "empty-shape-pool" else PUBLISHED]:
            raise ValueError(f"rifle lifetime carrier prefix disagrees: {case}")
        cursor += 2
        if case == "after-accepted-contact":
            if lines[cursor] != HIT:
                raise ValueError("rifle lifetime did not retain the accepted hit")
            cursor += 1
        visits = []
        for snapshot in expected:
            line = lines[cursor]
            cursor += 1
            if ROW.fullmatch(line) is None:
                raise ValueError(f"malformed rifle lifetime visit: {case}")
            fields = line.split()[1:]
            scen, visit, phase, subphase, stopped, flags, timer, registry, shape, carrier, descriptor, stock = map(
                int, fields[:12])
            z, scale = map(float, fields[12:])
            if (scen, stock) != (key, 1) or (visit, phase, subphase, stopped, flags, timer, registry, shape,
                    carrier, descriptor) != snapshot[:10] or z != snapshot[10] or abs(scale - snapshot[11]) > 0.0001:
                raise ValueError(f"rifle lifetime visit disagrees: {case}/{visit}")
            visits.append({
                "visit": visit, "carrier_phase": phase, "carrier_subphase": subphase, "carrier_stopped": bool(stopped),
                "contact_flags": flags, "carrier_timer": timer, "attack_registry_count": registry,
                "shape_free_count": shape, "carrier_free_count": carrier, "descriptor_free_count": descriptor,
                "rifle_stock": stock, "carrier_z": z, "carrier_scale": scale,
            })
        cases.append({"case": case, "stage_max_z": 31000 if case == "stage-bounds-exit" else 62500, "visits": visits})
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("rifle lifetime output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"rifle lifetime source signature mismatch at {address(entry)}")
    inlet = b"".join(load_contact_inputs(afs))
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--rifle-lifetime"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"rifle lifetime fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-rifle-lifetime", "revision": 1, "borg": "gg4e08:0600",
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "per_visit": ["80089A94(8)", "80089554", "8008A28C", "80089AEC"],
        "cleanup": {"unlink": "80088E50", "cell_return": "80089814", "descriptor_return": "80197AD8"},
        "declared_stage": {"grid_side": 128, "layers": 3, "no_terrain_triangles": True, "max_visits": 350},
        "cases": cases,
        "notes": [
            "Visit zero is the previously verified rifle prefix, with an accepted hit in only the after-contact fork.",
            "Each continuation invokes the four original passes separately; this is not a resumed Scene or a live frame count.",
            "Except for the bounded after-hit control, no receiver is present. No further collision or reaction work is invoked.",
            "The explicit stop flag is supplied. The accepted-hit branch preserves the original 0x10 flag and tests only one following visit.",
            "Original stopped-shape unlink precedes original carrier and descriptor return. No pool link or count is repaired by the fixture.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "samples": sum(len(c["visits"]) for c in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
