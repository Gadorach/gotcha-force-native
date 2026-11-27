"""Replay type-8 distance-based type-9 requests under independent pool pressure."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_type9_lifetime as bucket, retail_type9_roster as effect, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, address, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


KEYS = {0: (40, 250, 7), 5: (50, 200, 4), 6: (50, 200, 4)}
CASES = ("full", "carrier-restored-after-first-spaced-request", "no-descriptors", "one-shape")
REQUEST_ROW = re.compile(r"type8-trail:(?: [0-9]{1,3}){9}")
END_ROW = re.compile(r"type8-end:(?: [0-9]{1,2}){5}")
TEXT = (
    (0x800FCAE0, 248, "a1c772c97680ab8f005b5bd44a6102a5b60c1472ab6627a6dc080f7f01697671"),
    (0x800FCBD8, 352, "8d8f50bf4fac0163ff257896d9e93afcf363f3d260921fb9b3c3065588a078b5"),
    (0x8003F044, 124, "d61dad1cc92099585b159223478f4cb9d531219f1f0c06c95cf64b6ddbd52a35"),
    (0x80088AA0, 416, "800882bf558ba7e7f9e955e1884be71e0c9aad8870807bf7dc0876f56baaeab6"),
)


def fields(line, pattern):
    if pattern.fullmatch(line) is None:
        raise ValueError("malformed type-8 trail row")
    return tuple(map(int, line.split()[1:]))


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(KEYS)*len(CASES)*4:
        raise ValueError("type-8 trail report extent")
    rows = []
    for key, (speed, threshold, interval) in KEYS.items():
        for scenario, label in enumerate(CASES):
            first = len(rows)*4
            attempts = []
            for j in range(3):
                visit, z = 1+interval*j, 400+interval*speed*j
                created = scenario in (0, 3) or (scenario == 1 and j == 2)
                bound = created and (scenario != 3 or j == 0)
                published = (j+1, int(j == 2), 0, 1)[scenario]
                carriers = (2-j, 0, 3, 2-int(j > 0))[scenario]
                descriptors = (2-j, 3-int(j == 2), 0, 2-int(j > 0))[scenario]
                if fields(lines[first+j], REQUEST_ROW) != (key, scenario, visit, z, created, bound, published,
                                                             carriers, descriptors):
                    raise ValueError(f"type-8 trail request, allocation or publication changed: {key}/{label}")
                attempts.append({
                    "visit": visit, "origin": [160, 0, z], "source_request": "800FC428" if j == 0 else "800FC678",
                    "descendant_key": key, "carrier_allocated": created, "shape_published_on_visit": bound,
                    "attack_registry_count_at_end": published, "free_carriers_at_end": carriers,
                    "free_descriptors_at_end": descriptors,
                })
            end = 3+2*interval
            refused_retired = 2 if scenario == 3 else 0
            if fields(lines[first+3], END_ROW) != (key, scenario, end, refused_retired, attempts[-1]["attack_registry_count_at_end"]):
                raise ValueError(f"type-8 trail closing observation changed: {key}/{label}")
            rows.append({
                "key": key, "case": label, "initial_spare_carriers": 0 if scenario == 1 else 3,
                "initial_spare_descriptors": 0 if scenario == 2 else 3, "initial_spare_shapes": 3 if scenario == 0 else 1,
                "one_carrier_added_after_visit": 1+interval if scenario == 1 else None,
                "travel_per_active_visit": speed, "spacing_threshold_operand": threshold,
                "requests": attempts, "shape_refusals_retired_in_window": refused_retired, "last_visit_observed": end,
            })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-8 trail output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT+bucket.TEXT)
    for site, target in ((0x800FC240, 0x80088AA0), (0x800FC428, effect.EFFECT), (0x800FC678, effect.EFFECT),
                         (0x800FC6D8, 0x80088AA0), (0x800FC480, 0x800FCBD8)):
        beam.link(image, site, target)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        bank = effect.common_bank(RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-8 trail AFS changed during extraction")
    effect.source_index(image, bank)
    for key, (speed, spacing, _) in KEYS.items():
        row = effect.TABLE+key*effect.STRIDE
        if tuple(struct.unpack(">f", image.read(row+delta, 4, "data"))[0] for delta in (16, 32)) != (speed, spacing):
            raise ValueError("type-8 speed or spacing operand changed")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--trail"]
    cases = decode_report(effect.fixture_report(args, bank))
    payload = {
        "kind": "gf-retail-type8-trail", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "cases": cases,
        "entries": {"bucket_visit": "80089A94", "parent_factory": address(effect.PARENT),
                    "child_factory": address(effect.EFFECT), "observed_child_allocator": "800FC6D8"},
        "inlet": {"stage": "empty synthetic stage", "origin": [160, 0, 400], "copied_target": None,
                  "declared_owner_model": "0D03", "owner_clock_step": 1},
        "notes": [
            "The three keys come from the reviewed static type-8 callers. The fixture invokes the parent factory directly and does not assign these keys to the declared owner model.",
            "Visit 1 includes the parent initializer. Calls to the child's allocator are observed in the same bucket pass as the parent and child updates.",
            "The active parent updates its spacing point before trying the child factory. Capacity refusal does not change path or move the next request earlier.",
            "A successful child allocation can still refuse its shape on the same visit. No recipient, terrain obstacle, parent expiry, retail aim policy or native gameplay ran.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "visits": sum(r["last_visit_observed"] for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
