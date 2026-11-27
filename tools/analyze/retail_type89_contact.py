"""Resolve the registered type-89 sweep through contact, HP, and its area request."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_type89 as type89, retail_type89_cast as cast, retail_type89_opening as opening
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import PAIR_TEXT, RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


ACTORS = {
    0x0503: (2500, "1d9fd3cad8cfba0d07f17008791ebe6e068cdb357f09ae0ac22136f3547d366a"),
    0x0506: (2000, "93b431fb5e0a713e1add0aec3b8b96e8ffa519f9e4cbcc9774c8f3ef6f5a4e42"),
    0x050F: (1250, "cfb0a9daab30a3e1ef508bfb9929410cd1ca4ec487d18d12fd6787e13666ebc1"),
    0x0512: (1000, "33b54611af651852d2c31224d1796500ef49592faf9c5ad93e2fccccb5a64c46"),
}
DISTANCES = (30, 51, 52, 53)
ROW = re.compile(r"type89-contact:(?: [0-9]{1,4}){8}(?: -?[0-9]{1,4}\.[0-9]{4}){3}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(cast.MODELS)*len(DISTANCES):
        raise ValueError("type-89 contact report extent")
    cases = []
    for pos, line in enumerate(lines):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed type-89 contact row")
        model = cast.MODELS[pos//len(DISTANCES)]
        distance = DISTANCES[pos % len(DISTANCES)]
        pair = model in (0x0503, 0x050F)
        contact = distance < 52
        key = 10 if pair else 3
        expected_hp_loss = (17 if model < 0x050F else 8) if contact else 0
        fields = line.split()[1:]
        if tuple(map(int, fields[:8])) != (model, distance, key, expected_hp_loss, 180-expected_hp_loss,
                                           int(contact), int(contact), int(contact)):
            raise ValueError(f"type-89 contact HP, token or area mismatch: {model:04X}/{distance}")
        delta = list(map(float, fields[8:]))
        if contact:
            along, across, other = (delta[1], delta[0], delta[2]) if pair else (delta[0], delta[2], delta[1])
            if abs(along-(-5000 if pair else 5000)) > 0.01 or across != distance-22 or other != 0:
                raise ValueError("type-89 contact point did not follow the qualified sweep")
        elif any(delta):
            raise ValueError("type-89 miss reported a contact point")
        table, raw, _ = opening.SOURCES[model]
        cases.append({
            "borg": f"gg4e08:{model:04x}", "registered_key": key, "contact_bucket_visit": 32,
            "sweep_axis": "+Y" if pair else "-X", "receiver_offset_axis": "+X" if pair else "+Z",
            "receiver_offset_from_midpoint": distance, "raw_power": raw,
            "amount_record_reached": address(table+3*24) if contact else None,
            "received_and_credited_hp_loss": expected_hp_loss, "receiver_hp": 180-expected_hp_loss,
            "same_registry_duplicate_refused": contact, "receiver_token": int(contact), "receiver_history_count": int(contact),
            "area_allocation": {"bucket_visit": 33, "key": 16, "site": "8016D30C",
                                "saved_point_minus_carrier_root": delta} if contact else None,
        })
    return cases


def qualified_inlet(image, archive):
    type89.verify_text(image, tuple((at, *SLICES[at]) for at in PAIR_TEXT))
    block_input, sources = opening.qualified_inlet(image, archive)
    receiver_block, receiver_spec = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    body = source_operands(receiver_block)["receiver"]
    if levels(image, RECEIVER)[0][0] != 180:
        raise ValueError("type-89 receiver level-one HP changed")
    specs = []
    for model, source in zip(cast.MODELS, sources, strict=True):
        hp, digest = ACTORS[model]
        _, spec = read_normal(archive, model, source["hit_block_sha256"], digest)
        if levels(image, model)[0][0] != hp:
            raise ValueError(f"type-89 attacker level-one HP changed: {model:04X}")
        source.update(level_one_hp=hp, actor_spec_sha256=digest)
        specs.append(spec)
    return block_input+receiver_block+b"".join(specs)+receiver_spec, sources, body


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-89 contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-89 contact AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--contact"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-89 contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type89-contact", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases, "receiver": {
            **body, "borg": "gg4e08:000a", "level_one_hp": 180, "hit_block_sha256": RECEIVER_BLOCK_SHA256,
            "actor_spec_sha256": ACTOR_SHA256[RECEIVER],
        },
        "notes": [
            "The supplied actor phase requests the initial cast; one attacker shape slot isolates key 10 or key 3.",
            "The clear-stage replay builds the original sweep between successive publication points. The receiver is stationary at a declared midpoint offset.",
            "The exact normal receiver block is narrowed to body descriptor 9; its sphere and reaction record are not edited.",
            "Contact runs through the original amount and HP routines at separate cuts. The same registry is retried without seeding a receiver token.",
            "The next bucket visit allocates the area from the original saved contact point and stops before the area's initializer.",
            "No area damage, simultaneous hits, live action admission, or native gameplay is qualified.",
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
