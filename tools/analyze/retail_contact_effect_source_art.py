"""Bind original contact visuals to the exact single and split effect archives."""
import argparse
from hashlib import sha256
from io import BytesIO
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_contact_effect_render_prep as render, retail_dragon_art as art
from tools.analyze import retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import MEMBER_LIMIT, PzzArchive, RangeFile, RetailArchive
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
RENDER_SHA256 = "3df0d8e80a079049d5f946db8a19122f2aab87f5169e00944cf5295d1125c603"
RECORDS = (2, 3, 21, 53, 54)
CASES = (
    ("selector-0",        0, 0, 1, 0, (21,)),
    ("parent-lane-zero",  1, 0, 1, 0, (2,)),
    ("parent-lane-one",   1, 1, 0, 0, (3,)),
    ("forced-lane-zero",  1, 1, 0, 3, (2,)),
    ("selector-8-pair",   8, 1, 0, 0, (53, 54)),
)
ROW = re.compile(r"effect-source:(?: [0-9]{1,6}){9}")
TEXT = (
    (0x80254288, 68, "68434a34c000cfde2763eca6914bdf238278837116e7e7fb69faac3cb2af5606"),
    (0x80040EB4, 40, "809415a08cc1c8c2421222121a472454a4a95582e8dc5c44e99b0f613d658453"),
    (0x8025B00C, 364, "6a3c5877c5d3001cb3cd86fee75e455772c8eb19c62af8e5b90eeecec6403963"),
    (0x8025B178, 56, "1018f42a88ce0eab2af4dee4f4033ce9eefea29be009e77238a575ee1b606579"),
    (0x8025B1B0, 164, "f52cf35de11318d7c7975a330faa7274693ba814f2b84f7039d74034d24a96cd"),
    (0x8024F834, 168, "7097dc1adb66cd754ffdd1fbed2a646eba164a78107a66d948b3470048211569"),
)


def verify_source(image):
    render.verify_source(image)
    art.art_source_matches(image)
    beam.verify_text(image, TEXT)
    if sha256(image.read(0x8036F5D4, 28, "data")).hexdigest() != (
        "509396079406cedb58097847eb00dd742b26cb69457f74e1d0afdea8e12ec884"
    ):
        raise ValueError("common effect archive pool row changed")
    for site, target in (
        (0x80042900, 0x8000B07C), (0x80042970, 0x80197A0C), (0x8004297C, 0x80040EB4),
        (0x80040EC8, 0x80006C88), (0x80006CA0, 0x8025B00C), (0x80006CBC, 0x8025B1B0),
        (0x80006FF8, 0x8024F834), (0x800071F8, 0x8024F834), (0x800072B0, 0x8024F834),
    ):
        beam.link(image, site, target)


def source_blocks(afs):
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        if archive.names[art.EFFECT_MEMBER] != art.EFFECT_NAME or archive.names.count(art.EFFECT_NAME) != 1:
            raise ValueError("common effect archive member changed")
        member = archive.payload(art.EFFECT_MEMBER, MEMBER_LIMIT)
        if sha256(member).hexdigest() != art.EFFECT_SHA or stamp != archive_stamp(stream):
            raise ValueError("common effect PZZ identity or source changed")
    pack = PzzArchive(RangeFile(BytesIO(member), 0, len(member)))
    if len(pack.entries) != len(art.EFFECT_BLOCKS):
        raise ValueError("common effect PZZ block count")
    for slot in (1, 2):
        size, digest, _ = art.EFFECT_BLOCKS[slot]
        raw, *_ = pack.read(slot, size)
        if len(raw) != size or sha256(raw).hexdigest() != digest:
            raise ValueError("common effect HSD block identity")
        yield slot, raw


def decode(report, slot, source_records):
    lines = report.splitlines()
    if slot not in (1, 2) or len(report) > 1024 or len(lines) != 6:
        raise ValueError("common effect source report extent")
    results = []
    i = 0
    for scenario, (name, selector, parent, credit, mode, wanted) in enumerate(CASES):
        models = []
        for record in wanted:
            info = source_records[record]
            expected = (slot, scenario, selector, parent, credit, mode, record, info["descriptor_offset"], info["root_offset"])
            if ROW.fullmatch(lines[i]) is None or tuple(map(int, lines[i].split()[1:])) != expected:
                raise ValueError(f"common effect route or relocated model changed: {slot}/{name}/{record}")
            i += 1
            models.append({"record": record, **info})
        results.append({
            "case": name, "selector": selector, "parent_lane_byte": parent, "credited_actor_lane_byte": credit,
            "scene_mode_byte": mode, "first_update_models": models,
            "first_view_zero_lane_5_descriptor_count": len(models),
        })
    return results


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact visual source art output already exists")
    dol, fixture, module = (Path(p).resolve(strict=True) for p in (dol, fixture, module))
    verify_source(DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256))
    read_metadata(MAPS / "gg4e08-contact-effect-render-prep.json", RENDER_SHA256)
    slots = []
    for slot, raw in source_blocks(afs):
        receipt = art.record_receipts(raw, RECORDS)
        if receipt["record_count"] != 157 or any(r["empty_static"] for r in receipt["records"].values()):
            raise ValueError("common effect source table shape or records changed")
        proc = run([str(fixture), str(dol), str(module), "--source-art"], input=raw, capture_output=True, timeout=45)
        if proc.returncode or proc.stderr:
            raise ValueError(f"contact visual source fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
        slots.append({
            "pzz_slot": slot, "declared_split_selection_byte": int(slot == 2), "archive_bytes": len(raw),
            "archive_sha256": receipt["archive_sha256"], "record_count": receipt["record_count"],
            "verified_relocation_count": struct.unpack_from(">I", raw, 8)[0],
            "cases": decode(proc.stdout.decode("ascii"), slot, receipt["records"]),
        })
    data = {
        "kind": "gf-retail-contact-effect-source-art", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "pzz_member": art.EFFECT_MEMBER, "pzz_member_sha256": art.EFFECT_SHA,
        "contact_effect_render_prep_sha256": RENDER_SHA256,
        "source_entries": {"archive_stage": "80042878", "dvd_entry_cut": "8000B07C",
                           "archive_publish": "80042954", "archive_relocate": "8025B00C",
                           "hsd_pool_init": "80254288", "joint_load": "8024F834"},
        "slots": slots,
        "notes": [
            "The original staging call selects the archive destination; fixture supplies only the qualified decompressed block at that point.",
            "The original archive owner is allocated and the original loader relocates the input in retained RAM. Source files are read-only.",
            "The fixture calls the contact helper directly and runs one original bucket update and one matching-view preparation. Selector 1 has one scene entry left.",
            "Selector 1 takes its lane from the parent, even when the credited actor differs. Scene mode byte 3 forces record 2.",
            "Both selector-8 source records initialize on the same tested update and enter lane 5. It is not a separate direct model-loader test.",
            "No DVD or texture-atlas upload, full effect lifetime with these models, GX submission, live camera scheduling, or native gameplay is qualified.",
        ],
    }
    raw = encode(data, expanded=("slots",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "archive_slots": len(slots),
            "route_cases": sum(len(s["cases"]) for s in slots),
            "model_installs": sum(len(c["first_update_models"]) for s in slots for c in s["cases"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
