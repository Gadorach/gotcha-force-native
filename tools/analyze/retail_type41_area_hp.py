"""Verify HP attribution and range for the actual Time and Remote Bomber children."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_type41_bucket as bucket
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.analyze.retail_type76_opening import RECEIVER, RECEIVER_BLOCK_SHA256


s = bucket.s
BUCKET_SHA256 = "a72911d4e47c272804df9dc76d5e2c17d38dcc2717da4542a5bdaac5db469d05"
SIBLING_SHA256 = "7de0c87cd11c06365aa80bb5d9bf38131875fcec3d5a9dd8053614258432fd4e"
TEXT = (
    (0x80086084, 484, "336ef667addefcfec2d795c35aeadcde026a043547a27f977f14529362b6dcfd"),
    (0x8008AE3C, 36, "4a5238df2bbfcfc97ddc6e932511a41407c18a16b307bcfa3eb7827b46ecd02f"),
    (0x80089554, 260, "14d54d2183c44456974e44418019670cfd857a373e5baced583924c4c74280df"),
    (0x8002DB58, 944, "4aa097fc70080cd5b6c849d879cd20b39efebb359b80fb70b6a02c486ba38238"),
    (0x8002E2A8, 5672, "3f79bf55474f2fdc4d39b4436d481b713312b1477289db13deb902ffa26c0f1e"),
    (0x8003CD5C, 1512, "7c752eea0cdf04fbe970796f80ffe1bd98ea2ef6edebea6e33cd1d3462355792"),
    (0x8003D344, 164, "f8a7485a7bb1eb37c6a02be76336a2fef43d8a348adbe9670469e6e57d4da06f"),
)
ROW = re.compile(r"type41-area-hp:(?: [0-9]{1,3}){15}(?: [0-9A-F]{8}){3}")
# case, lateral separation, first accepted pair, HP loss, bit-exact impact radius
CASES = (
    ("shape-refused", 22, 0, 0, "00000000"),
    ("near", 22, 2, 17, "42086666"),
    ("middle", 70, 3, 17, "42823334"),
    ("last-integer-hit", 409, 14, 17, "43C1C000"),
    ("exact-tangent", 409.5, 14, 17, "43C1C000"),
    ("first-integer-miss", 410, 0, 0, "00000000"),
    ("ally-near", 22, 2, 4, "42086666"),
    ("receiver-mask-refused", 22, 0, 0, "00000000"),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(CASES)*len(bucket.MODELS):
        raise ValueError("type-41 area HP report extent")
    rows = []
    for model, _, capacity, _ in bucket.MODELS:
        remote = model == 0x0107
        for i, (name, distance, hit, damage, impact_radius) in enumerate(CASES):
            shape, ally = i != 0, i == 6
            first, token, unlink, retire = (2, 2, 18, 51) if shape else (0, 0, 0, 3)
            at_hit = capacity if hit else 0
            ledgers = (0 if ally else damage, damage if ally else 0, damage)
            floats = ("40466666" if shape else "00000000", impact_radius,
                      "43C1C000" if shape else "00000000")
            expected = (model & 0xFF, i, capacity, first, hit, token, damage, damage, at_hit,
                        *ledgers, 3, unlink, retire)
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None:
                raise ValueError("invalid type-41 area HP row")
            fields = line.split()[1:]
            if tuple(map(int, fields[:15])) != expected or tuple(fields[15:]) != floats:
                raise ValueError(f"type-41 HP attribution or area lifecycle changed: {model:04X}/{name}")
            rows.append({
                "borg": f"gg4e08:{model:04x}", "case": name, "parent_type": 41, "area_key": 19+remote,
                "common_group": 179+remote, "common_amount_record": "802C49E8" if remote else "802C49D0",
                "target_axis_separation": distance, "same_team": ally, "receiver_role_masked": i == 7,
                "first_published_visit": first or None, "first_hit_visit": hit or None,
                "bound_token": token or None, "accepted_token": token if hit else None,
                "signed_hp_request": damage if hit else None, "hp_lost": damage, "receiver_hp_after": 180-damage,
                "owner_stock_at_first_hit": at_hit if hit else None, "owner_stock_return_visit": 2,
                "ledgers": dict(zip(("owner_opponent", "owner_ally", "receiver"), ledgers, strict=True)),
                "parent_carrier_return_visit": 3, "area_shape_return_visit": unlink or None,
                "area_carrier_return_visit": retire,
                "radius_bits": dict(zip(("opening", "at_accepted_hit", "peak"), floats, strict=True)),
            })
    return rows


def compare_time_area(rows, prior):
    current = {r["case"]: r for r in rows if r["borg"] == "gg4e08:0106"}
    old = {r["case"]: r for r in prior["cases"] if r["borg"] == "gg4e08:0106"}
    tags = ("shape-refused", "near", "middle")
    if any(current[t][key] != old[t][key] for t in tags
           for key in ("area_key", "common_group", "common_amount_record", "first_published_visit",
                       "first_hit_visit", "receiver_hp_after", "area_shape_return_visit", "area_carrier_return_visit")):
        raise ValueError("Time Bomber area disagrees with the scoped type-33 controls")
    return list(tags)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-41 area HP output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    s.verify_text(image, TEXT)
    for site, target in ((0x800860E0, 0x8008AE3C), (0x8002DDA8, 0x8002E2A8),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        s.link(image, site, target)
    s.read_metadata(s.MAPS / "gg4e08-type41-bucket.json", BUCKET_SHA256)
    prior = s.read_metadata(s.MAPS / "gg4e08-type5-sibling-followup.json", SIBLING_SHA256)
    if any(struct.unpack(">H", image.read(record, 2, "data")) != (20,) for record in (0x802C49D0, 0x802C49E8)):
        raise ValueError("type-41 scoped common power words changed")
    payload = bucket.qualified_payload(image, afs, (RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER]))
    rows = decode(s.fixture_report(fixture, dol, module, ("--area-hp",), payload))
    raw = s.encode({
        "kind": "gf-retail-type41-area-hp", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "type41_bucket_sha256": BUCKET_SHA256, "type5_sibling_followup_sha256": SIBLING_SHA256,
        "time_key19_type33_comparisons": compare_time_area(rows, prior),
        "origin": [200, 100, 300], "receiver": "gg4e08:000a", "cases": rows,
        "notes": [
            "One paid parent per case uses its original initializer and terminal bucket. Terminal root, Time's near-expiry input and the stationary Sasuke pose are declared.",
            "Visit 1 is the terminal bucket, including child initialization. Stock returns on visit 2 before its pair pass; the parent carrier returns on visit 3.",
            "The original pair and HP writer run against the published child only. The HP writer completes, but the accepted pair caller is not resumed.",
            "A retained token does not repeat the HP write. Geometry at the verified axis separation is not a moving-body sweep or an arbitrary stage reach.",
            "Area material inlets are empty. No effects rendering, full actor scheduling, input chronology or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
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
