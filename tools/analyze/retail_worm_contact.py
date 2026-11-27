"""Verify paid Worm dispatch, refund ownership and zero-power contact continuation."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_worm_shot as worm
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES as CONTACT_SLICES
from tools.analyze.retail_stock_operands import stock_rows
from tools.analyze.retail_type0_contact import (
    AFS_SHA256, PAIR_TEXT, RECEIVER, RECEIVER_BLOCK_SHA256, SOURCE_SHA256, read_normal,
)
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage

# model, reviewed opening record, raw power, requested damage, bound radius, normal HP,
# PZZ/loose hit block hash, exact normal actor record hash.
PILOTS = (
    (0x0501, 0x8032BA58, 0, 0, 15, 300,
     "550eafbbc514419a6dcc4b186e3159128d3ffcc4bcb611c9d97ac2bac4e9da7f",
     "05221cb8d1c9ef0a8b71e5a0cfad83afe0fef6a89cf0534cb80d82ed2df86864"),
    (0x050B, 0x8038D058, 30, 26, 100, 300,
     "550eafbbc514419a6dcc4b186e3159128d3ffcc4bcb611c9d97ac2bac4e9da7f",
     "051b81b5d6f7ea1070392f481565152662c4489eb461b3563fb0c8b5395d5d27"),
    (0x050D, 0x8032BB18, 0, 0, 7, 150,
     "2efb5d804dbfa168a7f224a6a94113b74f80cc20a73c673697b40d9dc9537e5c",
     "51876ea608253e92e82a0767413627d47ac4145815e4eda99b1de0b9dfa3c595"),
    (0x0517, 0x8038D118, 15, 13, 50, 150,
     "2efb5d804dbfa168a7f224a6a94113b74f80cc20a73c673697b40d9dc9537e5c",
     "46232c0f552f90a0863b600e4a51ea53f2be4d677063a28b3cbe72118f05d31a"),
)
CASES = ("carrier-pool-empty", "shape-pool-empty", "miss", "accepted-contact")
RELEASE = re.compile(r"worm-release:(?: [0-9]{1,4}){8} [0-9A-F]{8}(?: [0-9]{1,3}){4}")
CONTACT = re.compile(
    r"worm-contact: ([0-9]{1,4}) ([23]) previous=([0-9]{1,5}) ledger=([0-9]{1,3})/([0-9]{1,3})"
    r" cursor=([0-9]) radius=([0-9]{1,3}\.[0-9]{5})"
)
REENTRY_SLICES = {
    0x8006DE10: (52, "fdbdc64d6a82e1f539cf9aaedfdf3fb054254649f2cad9c648005b992c0dcaff"),
    0x8013A440: (132, "160580ff12323b2c0c2bb1d6a756020fbe3b66e17a787a92b6cee1a37aba9d76"),
    0x8013A720: (76, "d750cfdb7320300399ac4d0b129816594b4fbf3cbbe4cb7bca12bae064ee319a"),
    0x8013A76C: (172, "a6c17a73728da9a55811e15c6144bfdae1457d08b37e024c30fb6aae47af5d06"),
}


def integer_row(line, label, size):
    if re.fullmatch(re.escape(label) + rf":(?: [0-9]{{1,4}}){{{size}}}", line) is None:
        raise ValueError(f"malformed {label} row")
    return tuple(map(int, line.split()[1:]))


def decode_report(report):
    lines = iter(report.splitlines())
    if len(report) > 4096 or len(report.splitlines()) != 30:
        raise ValueError("Worm fixture report extent")
    pilots = []
    for model, record, _, value, radius, *_ in PILOTS:
        family, key, _ = worm.MODELS[model]
        string = family == 49
        cases = []
        for scenario, case in enumerate(CASES):
            line = next(lines)
            if RELEASE.fullmatch(line) is None:
                raise ValueError("malformed Worm release row")
            fields = line.split()[1:]
            src, typed, operand, index, released, bound, phase, attacks = map(int, fields[:8])
            received_record = int(fields[8], 16)
            flags, token, amount, hp = map(int, fields[9:])
            hit, has_shape = scenario == 3, scenario >= 2
            expected_amount = value if hit else 0
            if (src, typed, operand, index, released, bound, phase, attacks, received_record, flags, token, amount, hp) != (
                model, family, key, scenario, 2 if string and scenario == 0 else 1,
                2 if string and not has_shape else 1, (0, 2, 1, 1)[scenario], int(has_shape),
                record if hit else 0, 16 if hit else 0, int(hit), expected_amount, 180-expected_amount,
            ):
                raise ValueError(f"Worm release/contact disagreement: {model:04X}/{case}")
            result = {
                "case": case, "stock_after_release": released, "stock_after_bind": bound, "carrier_phase_after_bind": phase,
                "registered_attack_count": attacks, "amount_record": address(received_record) if hit else None,
                "contact_flags": flags, "receiver_token": token, "hp_writer_reached": hit,
                "requested_damage": amount, "receiver_hp_after_opening": hp,
            }
            if has_shape:
                contact = CONTACT.fullmatch(next(lines))
                if not contact:
                    raise ValueError("malformed Worm contact row")
                a, b, previous, dealt, taken, cursor = map(int, contact.groups()[:6])
                observed_radius = float(contact[7])
                if (a, b, previous, dealt, taken, cursor, observed_radius) != (
                    model, scenario, 180 if hit else 65535, expected_amount, expected_amount, int(hit), radius,
                ):
                    raise ValueError("Worm contact history, radius or ledger disagreement")
                result.update(previous_hp=previous, credited_damage=dealt, received_damage=taken,
                              receiver_token_cursor=cursor, shape_radius=observed_radius)
            cases.append(result)
        entry = {"borg": f"gg4e08:{model:04x}", "family": family, "key": key, "cases": cases}
        if string:
            visits = []
            for expected in ((model, 1, 1, 1, 1, 10, 1, 0, 1, 1), (model, 2, 1, 2, 0, 10, 1, 1, 2, 1)):
                row = integer_row(next(lines), "worm-rebind", 10)
                if row != expected:
                    raise ValueError("Worm rebind state disagreement")
                _, visit, phase, subphase, stop, timer, recipient, attacks, token, stock = row
                visits.append({"visit": visit, "phase": phase, "subphase": subphase, "stopped": bool(stop),
                               "timer": timer, "retained_recipient": bool(recipient), "attack_count": attacks,
                               "carrier_token": token, "stock": stock})
            follow = integer_row(next(lines), "worm-followup", 10)
            if follow != (model, 1, 1, 2, 2, 1, 179, 1, 1, 1):
                raise ValueError("Worm fresh-token follow-up disagreement")
            entry["rebind"] = visits
            entry["followup"] = {"receiver_tokens": list(follow[2:4]), "receiver_token_cursor": follow[4],
                                 "requested_damage": follow[5], "receiver_hp": follow[6],
                                 "credited_damage": follow[7], "received_damage": follow[8], "stock": follow[9]}
        pilots.append(entry)
    return pilots


def qualified_sources(image, archive):
    body, spec = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    inlet = [body, spec]
    sources = []
    for model, record, power, _, radius, hp, block_hash, spec_hash in PILOTS:
        block, spec = read_normal(archive, model, block_hash, spec_hash)
        inlet += [block, spec]
        family, key, site = worm.MODELS[model]
        base, stride, count, joint_at, factory, *_ = worm.FAMILIES[family]
        literal, call = struct.unpack(">2I", image.read(site-4, 8, "text"))
        row = image.read(base+key*stride, stride, "data")
        string = family == 49
        _, table = reaction_installer_tables.model_table(image, model)
        group = row[1 if string else 2]
        step, index = (9, 6) if string else (0, 0)
        desc = reaction_emit.STEPS + step * reaction_emit.STEP_SIZE
        if (literal != 0x38800000 | key or call & 0xFC000003 != 0x48000001 or direct_branch(site, call) != factory
                or key >= count or row[joint_at] != 12 or group != (6 if string else 0)
                or reaction_emit.read_group(block, group) != [(step, index)] or table+24*index != record
                or struct.unpack(">H", image.read(record, 2, "data")) != (power,)
                or struct.unpack_from(">f", block, desc+24) != (radius,) or levels(image, model)[0][0] != hp):
            raise ValueError(f"Worm launch or opening record mismatch: {model:04X}")
        source = {"release_site": address(site), "owner_hp": hp, "hit_block_sha256": block_hash,
                  "actor_spec_sha256": spec_hash, "stock_channel_0": stock_rows(levels(image, model))[0]["levels"][0],
                  "opening": {"group": group, "descriptor": step, "reaction_index": index,
                              "record": address(record), "raw_power": power}}
        if string:
            follow = table+24*7
            follow_base, = struct.unpack(">H", image.read(follow, 2, "data"))
            if row[2] != 7 or reaction_emit.read_group(block, 7) != [(10, 7)] or follow_base != (2 if key == 0 else 1):
                raise ValueError(f"Worm follow-up source record mismatch: {model:04X}")
            source["followup"] = {"group": 7, "descriptor": 10, "reaction_index": 7,
                                  "record": address(follow), "raw_power": follow_base}
        sources.append(source)
    return b"".join(inlet), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Worm contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    slices = tuple((at, *CONTACT_SLICES[at]) for at in PAIR_TEXT)
    extra = tuple((at, *span) for at, span in REENTRY_SLICES.items())
    for kind, spans in (("text", worm.TEXT+slices+extra), ("data", worm.DATA)):
        for at, size, digest in spans:
            if sha256(image.read(at, size, kind)).hexdigest() != digest:
                raise ValueError(f"Worm source signature mismatch at {address(at)}")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_sources(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("Worm source AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--worm-contact"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"Worm contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    pilots = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-worm-contact", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "paid_dispatch": "80119848", "refund": "8006DE10",
        "receiver": {"borg": "gg4e08:000a", "body_descriptor": 9, "starting_hp": 180},
        "declared_stock_before_release": 2,
        "pilots": [{**result, "source": source} for result, source in zip(pilots, sources, strict=True)],
        "notes": [
            "Original paid dispatch is entered directly, with no B cue or action scheduler.",
            "Positive paths declare rigid poses, pool availability and a singleton receiver on an isolated stage.",
            "Only the original HP writer completes on each accepted contact; its caller is not resumed.",
            "Type-49 follow-up visits keep the same carrier and recipient and observe group 7 under a fresh token.",
            "Class-2 effect pools are empty and no sound is played. Full tether movement, later callbacks and poison are not qualified.",
            "No native move or zero-power damage binding is added.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots),
            "cases": sum(len(p["cases"]) for p in pilots), "second_contacts": sum("followup" in p for p in pilots)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
