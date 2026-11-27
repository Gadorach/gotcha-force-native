"""Replay normal type-76 opening attack and receiver roles, HP, and hit-latched exit."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_type32_roster as beam
from tools.analyze import retail_type76_roster as roster
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, address, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import PAIR_TEXT, RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


# Model, parent key, reaction table, high-power descriptor, record index, sockets and translated height.
PILOTS = (
    (0x0D01, 0, 0x803628F0, 3, 2, (2, 3), 157.6,
     "d04531e3112b842004c745d3aa77fece58e6989528a57bfc21149ec9610a401b",
     "f3a4ff1a78b3bbc60f92d0ac7d29efff275e1d85c056bb341263700ab23fe843"),
    (0x0D04, 2, 0x8037A830, 1, 1, (8, 10), 169.5,
     "1e6ad0000c5161e6202efa4ac87688ddd15ff737937d7e9db1f93271f8b34888",
     "ae63371a27ff2925e26d85c09f5c9a10c1bc45ffb1ed36dc35673b420b8018a5"),
)
CASES = (
    ("no-shape", 0, 25, None), ("attack-only-slot", 1, 25, None), ("both-at-tip-exit-carrier", 2, 25, 1),
    ("within-receiver-only", 2, -40, None), ("outside-both", 2, -80, None), ("both-at-tip-exit-refused", 2, 25, 0),
)
ROW = re.compile(r"type76-opening:(?: [0-9]{1,4}){13} [0-9A-F]{8}(?: [0-9]{1,4}\.[0-9]{4}){3}")
TEXT = (
    (0x8008AC80, 400, "88d16df3bda187ed3cb011269a356a7d0646ca1c946dcd194664a5abf4a317d0"),
    (0x8008A65C, 996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
    (0x8008A2BC, 788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
    (0x80047AA4, 404, "d1034c09b0f68dabce25aa8dc2fd55eb99b31ee2e017cf379fad8fa7c23eb779"),
    (0x8008A28C, 48, "94cb040d0d0bd380e6ab2d81aec5d82583aaf212f52408a7050212f7e39acc08"),
    (0x80089AEC, 276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    *tuple((at, *SLICES[at]) for at in PAIR_TEXT),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(PILOTS)*2*len(CASES):
        raise ValueError("type-76 opening report extent")
    rows = []
    for model, key, table, first, index, sockets, y, *_ in PILOTS:
        for ordinal, socket in enumerate(sockets):
            for i, (label, slots, offset, exit_cells) in enumerate(CASES):
                line = lines[len(rows)]
                if ROW.fullmatch(line) is None:
                    raise ValueError("malformed type-76 opening row")
                values = line.split()[1:]
                group = 2 if key == 0 else 1
                attacking, receiving = slots > 0, slots == 2
                hit = attacking and offset == 25
                loss = 35 if hit else 0
                requested, allocated = exit_cells is not None, exit_cells == 1
                record = table+24*index if hit else 0
                root = [float(socket*100), y, 300.0]
                if (tuple(map(int, values[:13])) != (model, ordinal, i, socket, group, slots, attacking, 1+receiving,
                                                      loss, 180-loss, hit, requested, allocated)
                        or int(values[13], 16) != record or list(map(float, values[14:])) != root):
                    raise ValueError(f"type-76 opening geometry, role, HP or exit mismatch: {model:04X}/{ordinal}/{label}")
                rows.append({
                    "borg": f"gg4e08:{model:04x}", "parent_key": key, "ordinal": ordinal, "socket": socket,
                    "parent_origin": root, "case": label, "carrier_group": group, "shape_slots": slots,
                    "sasuke_z_offset_from_parent": offset, "attack_descriptor": first if attacking else None,
                    "carrier_receiver_descriptor": first+1 if receiving else None,
                    "accepted_amount_record": address(record) if hit else None, "hp_loss": loss,
                    "receiver_hp_after": 180-loss, "accepted_token": 1 if hit else None, "duplicate_token_refused": hit,
                    "hit_latched_exit": {"type": 8, "key": 0, "spare_carriers": exit_cells,
                                         "allocated": allocated, "returned_parent_shapes": slots} if requested else None,
                })
    return rows


def qualified_inlet(image, archive):
    rows, _ = roster.row_index(image)
    beam.verify_text(image, TEXT)
    for site, target in ((0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344), (0x800FD0F8, 0x800FD288),
                         (0x800FC240, 0x80088AA0)):
        beam.link(image, site, target)
    body, defender = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    if levels(image, RECEIVER)[0][0] != 180:
        raise ValueError("type-76 body HP scope changed")
    inlet, sources = body+defender, []
    for model, key, table, first, index, sockets, _, hit_digest, actor_digest in PILOTS:
        hit, spec = read_normal(archive, model, hit_digest, actor_digest)
        group = rows[key]["carrier_group"]
        if (reaction_installer_tables.model_table(image, model)[1] != table or levels(image, model)[0][0] != 150
                or tuple(v["socket"] for v in rows[key]["socket_variants"]) != sockets
                or reaction_emit.read_group(hit, group) != [(first, index), (first+1, 0)]):
            raise ValueError("type-76 normal actor or group changed")
        shape_sources = []
        for step, amount, header, power, role, radius in (
                (first, index, (0, 0, 1, index, 0, -1, 0), 40, "attack", 30),
                (first+1, 0, (2, 0, 2, 0, 0, -1, 0), 10, "receiver", 55)):
            at = reaction_emit.STEPS+step*reaction_emit.STEP_SIZE
            record = table+24*amount
            if struct.unpack_from(">BBHHhhH", hit, at) != header or image.read(record, 2, "data") != struct.pack(">H", power):
                raise ValueError("type-76 mixed source descriptor or amount changed")
            shape_sources.append({"descriptor": step, "source_kind": header[0], "source_flags": header[2],
                                  "amount_record": address(record), "raw_power": power,
                                  "published_role": role, "published_radius": radius})
        sources.append({"borg": f"gg4e08:{model:04x}", "parent_key": key, "carrier_group": group, "level_one_hp": 150,
                        "hit_block_sha256": hit_digest, "actor_spec_sha256": actor_digest, "shapes": shape_sources})
        inlet += hit+spec
    return inlet, sources, source_operands(body)["receiver"]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-76 opening output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-76 opening AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--opening"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-76 opening fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type76-opening", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases, "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "entries": {"group_bind": "800FD03C", "pair_pass": "8002DB58", "hp_writer": "8003D344",
                    "hit_latch_exit": "800FD100", "child_request": "800FD314"},
        "notes": [
            "The paid leaf runs from declared phase one; ordinal 1 follows a paid refusal at ordinal 0. Socket matrices are distinct declared identity translations.",
            "A slot admits the attack descriptor first. A second slot admits the carrier receiver, not a second attack. Sasuke uses a separate body slot.",
            "The original amount call and HP writer determine damage and set the contact latch. Re-entry with the retained registry checks duplicate refusal.",
            "On the next callback, real contact requests type 8/key 0 from the carrier origin even without a child cell; both carrier shapes return.",
            "No incoming projectile hit the carrier receiver, no type-8 child initialized, and no terrain, live timing, reaction tail or native gameplay was qualified.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
