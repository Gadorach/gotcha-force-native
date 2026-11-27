"""Join three ordinary type-0 factory keys to original contact, HP and retirement."""
import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_attack_table, reaction_emit, reaction_installer_tables
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_origins import origin_tables
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES as CONTACT_SLICES
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_roster import AFS_SHA256, FACTORY, SOURCE_SHA256, carrier_rows
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive, levels
from tools.extract.source_image import DolImage


@dataclass(frozen=True)
class Pilot:
    model: int
    key: int
    site: int
    record: int
    body_hp: int
    step: int
    reaction_index: int
    clear_delta: tuple[int, int]
    block_digest: str
    spec_digest: str


PILOTS = (
    Pilot(0x0800, 12, 0x80076C10, 0x802D6270, 180, 2, 1, (-40, 0),
          "1738ac3267f93a5d33b0dad227425f7a0b11d5444bf19bcd7f909423debf3e08",
          "9d1492115df0d6df22495cd90e2d92147580088db02e49fe35950cfc6000a90e"),
    Pilot(0x0801, 125, 0x800A403C, 0x802F7C30, 180, 21, 1, (-40, 0),
          "14dff1ec36e66c0d2fa531f402c884844e26c13440944c8f5d5012251d2ed790",
          "0fddcbc7ed6a3f9d3970497e16925a6a51a35bae982221612232674a9b947bdc"),
    Pilot(0x0802, 45, 0x8013D77C, 0x80339E18, 200, 0, 0, (0, 20),
          "c72cfa05a63ea7142e55265bd47bcefb9280073d1ae90a9a04c4bf653fa630a1",
          "7995a8c0fab3e8a3282a82844e2cbae5b296b369d010932cad4feec382616f64"),
)
CASES = (("miss", 256, 0), ("duplicate-token", 0, 1), ("accepted-contact", 0, 0))
RECEIVER = 0x000A
RECEIVER_BLOCK_SHA256 = "8d90bfdd7a4d598b4c0869fff93364b785d4cc98c876d56c99ce44fc86579414"
PAIR_TEXT = (0x8008AB30, 0x8002DB58, 0x8002E2A8, 0x8003CD5C, 0x8003D344)
ROW = re.compile(r"type0-contact:(?: [0-9]{1,4}){7} [0-9A-F]{8}(?: [0-9]{1,3}){8}(?: -?[0-9]{1,3}\.[0-9]{5}){2}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(PILOTS) * len(CASES):
        raise ValueError("type-0 contact report extent")
    results = []
    for i, pilot in enumerate(PILOTS):
        cases = []
        for line, (name, distance, duplicate) in zip(lines[i*len(CASES):(i+1)*len(CASES)], CASES, strict=True):
            if ROW.fullmatch(line) is None:
                raise ValueError(f"malformed type-0 contact row: {pilot.model:04X}/{name}")
            fields = line.split()[1:]
            model, key, d, dup, flag, token, cursor = map(int, fields[:7])
            record = int(fields[7], 16)
            amount, hp, phase, stopped, clock, attacks, shape, returned = map(int, fields[8:16])
            delta = list(map(float, fields[16:]))
            hit = not distance and not duplicate
            if (model, key, d, dup, flag, token, cursor, record, amount, hp, phase, stopped, clock, attacks, shape,
                    returned, tuple(delta)) != (
                pilot.model, pilot.key, distance, duplicate, 16 if hit else 0, int(hit or duplicate), int(hit),
                pilot.record if hit else 0, 17 if hit else 0, 163 if hit else 180, 2 if hit else 1,
                int(hit), 299, int(not hit), int(hit), 3 if hit else 0, (0, 0) if hit else pilot.clear_delta,
            ):
                raise ValueError(f"type-0 contact outcome disagrees: {pilot.model:04X}/{name}")
            cases.append({
                "case": name, "center_delta_x": d, "seeded_duplicate_token": bool(dup),
                "contact_flags_after_pair": flag, "first_receiver_token": token, "receiver_token_cursor": cursor,
                "original_amount_record": address(record) if record else None, "hp_writer_reached": hit,
                "requested_damage": amount, "credited_and_received_damage": amount, "receiver_hp": hp,
                "first_followup": {"phase": phase, "stopped": bool(stopped), "timer": clock,
                                   "attack_count": attacks, "shape_free_count": shape, "motion_xz": delta},
                "carrier_and_descriptor_return_visit": returned or None,
            })
        results.append({"borg": f"gg4e08:{pilot.model:04x}", "key": pilot.key, "cases": cases})
    return results


def read_normal(archive, model, block_digest, spec_digest):
    block = reaction_emit.qualified_hit_block(archive, model)
    member, _ = archive.member(archive.names.index(f"pl{model:04x}.pzz".encode()), model)
    all_specs, _ = member.read(0, 8 * ACTOR_BYTES)
    spec = all_specs[:ACTOR_BYTES]
    if (not all_specs or len(all_specs) % ACTOR_BYTES or len(all_specs) > 8 * ACTOR_BYTES
            or sha256(spec).hexdigest() != spec_digest or sha256(block).hexdigest() != block_digest):
        raise ValueError(f"normal projectile contact input identity: {model:04X}")
    return block, spec


def qualify_pilot(image, pilot, block, rows):
    pc = pilot.site
    operand, branch = struct.unpack(">2I", image.read(pc-4, 8, "text"))
    row = rows[pilot.key]
    group = row["collision_group_selector"]
    _, table = reaction_installer_tables.model_table(image, pilot.model)
    mode, = struct.unpack(">b", image.read(pilot.record+0x0E, 1, "data"))
    power, = struct.unpack(">H", image.read(pilot.record, 2, "data"))
    if (operand != 0x38800000 | pilot.key or branch & 0xFC000003 != 0x48000001 or direct_branch(pc, branch) != FACTORY
            or reaction_emit.read_group(block, group) != [(pilot.step, pilot.reaction_index)]
            or table + pilot.reaction_index * reaction_attack_table.SIZE != pilot.record
            or (power, mode, levels(image, pilot.model)[0][0]) != (20, 4, pilot.body_hp)):
        raise ValueError(f"source type-0 release or damage record mismatch: {pilot.model:04X}")
    return {
        "static_release_site": address(pc), "group": group, "descriptor": pilot.step,
        "reaction_index": pilot.reaction_index, "reaction_mode": mode, "raw_power": power,
        "level_one_max_hp": pilot.body_hp, "hit_block_sha256": pilot.block_digest, "actor_spec_sha256": pilot.spec_digest,
    }


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-0 contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry in PAIR_TEXT:
        size, digest = CONTACT_SLICES[entry]
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"type-0 pair source signature at {address(entry)}")
    rows = carrier_rows(image, origin_tables(image)["type-0"])
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        body, spec = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
        inlet = [body, spec]
        source = []
        for pilot in PILOTS:
            block, spec = read_normal(archive, pilot.model, pilot.block_digest, pilot.spec_digest)
            inlet.extend((block, spec))
            source.append(qualify_pilot(image, pilot, block, rows))
        if stamp != archive_stamp(stream):
            raise ValueError("type-0 contact AFS changed during extraction")
    receiver = source_operands(body)["receiver"]
    if levels(image, RECEIVER)[0][0] != 180:
        raise ValueError("type-0 receiver level-one HP mismatch")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--type0-contact"]
    result = run(argv, input=b"".join(inlet), capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-0 contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    results = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type0-contact", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "factory": address(FACTORY), "pair_loop": "8002DB58",
        "writer": "8003D344", "receiver": {"borg": "gg4e08:000a", "initial_hp": 180, **receiver,
                                          "hit_block_sha256": RECEIVER_BLOCK_SHA256},
        "pilots": [{**r, "source": p} for r, p in zip(results, source, strict=True)],
        "notes": [
            "The source keys enter the ordinary factory directly, without a controller, cue or stock debit.",
            "Both lists and registries are computed by original code. The rigid poses, unit scales, receiver body list, damage input axes and empty stage are declared.",
            "The pair loop sets contact flags and token. Only the original HP writer completes; its caller is not resumed.",
            "Later bucket and shape visits are isolated calls, not battle frames or proof of every key or receiver type.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(results), "cases": sum(len(p["cases"]) for p in results)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
