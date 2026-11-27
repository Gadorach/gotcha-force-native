"""Join Machine Red's paid rifle and original registries to Sasuke contact and HP."""
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
from tools.analyze.retail_rifle_bucket import RELEASE, SLICES as BUCKET_SLICES, source_shape
from tools.analyze.retail_shape_sweep import source_operands
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive, levels
from tools.extract.source_image import DolImage

CREDIT_MODEL, RECEIVER_MODEL = 0x0600, 0x000A
CREDIT_HP, RECEIVER_HP = 500, 180
RECORD = 0x8030A398
SLICES = {
    **BUCKET_SLICES,
    0x8008AB30: (336, "9af045cf898354aadd281a64834e3cdde963848553ea25260df7d4f3de9b439b"),
    0x8002DB58: (944, "4aa097fc70080cd5b6c849d879cd20b39efebb359b80fb70b6a02c486ba38238"),
    0x8002E2A8: (5672, "3f79bf55474f2fdc4d39b4436d481b713312b1477289db13deb902ffa26c0f1e"),
    0x8003CD5C: (1512, "7c752eea0cdf04fbe970796f80ffe1bd98ea2ef6edebea6e33cd1d3462355792"),
    0x8003D344: (164, "f8a7485a7bb1eb37c6a02be76336a2fef43d8a348adbe9670469e6e57d4da06f"),
}
ACTOR_SHA256 = {
    CREDIT_MODEL: "c1dc203649224c2e69c5d80fa872b550462aa7742173fa675632786858007393",
    RECEIVER_MODEL: "bd5c6a7611df76737868e6d34580ff4e8dc464e288d23340c753e3f45cfec6c5",
}
PUBLISHED = "rifle-bucket: 1 1 0 1 300 1 49 1 1 -149.39999 191.00000 365.00000 0.00000 0.00000 79.99999 32.46753"
CONTACT = re.compile(r"rifle-contact:(?: [0-9]{1,5}){13} [0-9A-F]{8}")
CASES = (
    ("miss", 256, 0, 0),
    ("duplicate-token", 0, 1, 0),
    ("ordinary-contact", 0, 0, 0),
    ("reaction-class-masked", 0, 0, 1),
)


def decode_report(report):
    if len(report) > 2048:
        raise ValueError("oversized rifle contact report")
    lines = report.splitlines()
    if len(lines) != 3 * len(CASES):
        raise ValueError("incomplete rifle contact report")
    cases = []
    for i, (name, distance, duplicate, masked) in enumerate(CASES):
        release, publish, line = lines[3*i:3*i+3]
        if release != RELEASE or publish != PUBLISHED or CONTACT.fullmatch(line) is None:
            raise ValueError(f"unqualified rifle contact prefix: {name}")
        fields = line.split()[1:]
        d, dup, mask, writer, damage, hp, prev, dealt, received, flag, token, cursor, reaction = map(int, fields[:-1])
        curve = int(fields[-1], 16)
        accepted = distance == 0 and not duplicate
        amount = 8 if accepted and not masked else 0
        if (d, dup, mask, writer, damage, hp, prev, dealt, received, flag, token, cursor, reaction, curve) != (
            distance, duplicate, masked, int(accepted), amount, RECEIVER_HP - amount,
            RECEIVER_HP if accepted else 65535, amount, amount, 0x10 if accepted else 0,
            int(accepted or duplicate), int(accepted), 2 if accepted else 0, 0x802C3494 if amount else 0,
        ):
            raise ValueError(f"rifle contact, reaction, or HP disagreement: {name}")
        cases.append({
            "case": name, "sphere_center_delta_x": d, "duplicate_token_seeded": bool(dup),
            "receiver_reaction_class_mask": 4 if mask else 0, "contact_accepted": bool(writer),
            "carrier_contact_flags": flag, "receiver_first_token": token, "receiver_token_cursor": cursor,
            "receiver_reaction_class": reaction if writer else None,
            "receiver_reaction_curve": address(curve) if curve else None,
            "hp_writer_reached": bool(writer), "requested_damage": damage, "remaining_hp": hp,
            "previous_hp_field": prev, "credited_damage": dealt, "recorded_receive_damage": received,
        })
    return cases


def read_spec(archive, model):
    member, _ = archive.member(archive.names.index(f"pl{model:04x}.pzz".encode()), model)
    spec, _ = member.read(0, ACTOR_BYTES)
    if len(spec) != ACTOR_BYTES or sha256(spec).hexdigest() != ACTOR_SHA256[model]:
        raise ValueError(f"rifle contact actor spec mismatch at model {model:04x}")
    return spec


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("rifle contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"rifle contact source signature mismatch at {address(entry)}")
    power, = struct.unpack(">H", image.read(RECORD, 2, "data"))
    if power != 10 or (levels(image, CREDIT_MODEL)[0][0], levels(image, RECEIVER_MODEL)[0][0]) != (CREDIT_HP, RECEIVER_HP):
        raise ValueError("rifle power or level-one actor HP disagrees")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        attack = qualified_hit_block(archive, CREDIT_MODEL)
        body = qualified_hit_block(archive, RECEIVER_MODEL)
        credit_spec = read_spec(archive, CREDIT_MODEL)
        receiver_spec = read_spec(archive, RECEIVER_MODEL)
        if stamp != archive_stamp(stream):
            raise ValueError("rifle contact source AFS changed during extraction")
    source_shape(attack)
    receiver = source_operands(body)["receiver"]
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--rifle-contact-hp"]
    result = run(argv, input=attack + body + credit_spec + receiver_spec, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"rifle contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-rifle-contact-hp", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "attack": {"id": "gg4e08:0600", "actor_spec_sha256": ACTOR_SHA256[CREDIT_MODEL],
                   "hit_block_sha256": sha256(attack).hexdigest(), "attack_record": address(RECORD),
                   "raw_attack_power": power, "stock_before_release": 2, "stock_after": 1},
        "receiver": {"id": "gg4e08:000a", "actor_spec_sha256": ACTOR_SHA256[RECEIVER_MODEL],
                     "hit_block_sha256": sha256(body).hexdigest(), **receiver},
        "path": {"receiver_bind": "8008AB30", "sweep": "80089AEC", "pair_loop": "8002DB58",
                 "amount": "8002F108 -> 8003CD5C", "amount_carrier": "rifle object, not credited Borg",
                 "hp_writer": "8002F1D8 -> 8003D344", "stop_after_writer_at": "8002F1DC"},
        "declared": {"actor_slots": [0, 1], "actor_teams": [0, 1], "initial_and_max_hp": [CREDIT_HP, RECEIVER_HP],
                     "team_settings": [0, 0], "credit_and_receive_controller_banks": [0, 0],
                     "actor_ranks": [0, 0], "team_current_and_reference_totals": 100,
                     "previous_hp_sentinel": 65535},
        "cases": cases,
        "notes": [
            "Continues the original paid carrier and narrow bucket replay, with a declared rigid body-9 receiver and no seeded registry pointers.",
            "In positive cases the original loop writes the same receiver to the carrier and credited actor and links the receiver back to the carrier.",
            "The verified amount call receives the carrier pointer; damage is credited to Machine Red. Masked reaction retains contact but requests zero damage.",
            "The original HP writer returns without resuming its caller. No swept motion, full reaction scheduling, KO, retirement, or native move timing is qualified.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "hp_changes": sum(row["remaining_hp"] != RECEIVER_HP for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
