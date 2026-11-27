"""Verify the four indexed type-60 routes and the separate type-33 contact."""
import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_alternate_vm as vm


FACTORY = 0x8014452C
FACTORY_TABLE = 0x8033D250
CASES = ("opponent", "ally", "miss", "effect-cell-refused", "effect-descriptors-refused",
         "shape-refused", "carrier-cell-refused", "carrier-descriptors-refused", "stock-refused", "wrong-event")
DESCRIPTORS = (7, 7, 7, 7, 6, 7, 7, 3, 7, 7)
ROW = re.compile(r"type60:(?: [0-9]{1,4}){16}")
TEXT = (
    (0x8014452C, 264, "7ac212959f71daf02e64152c6a1d04b42bd6a012b5e3d22e3767d4af036461cf"),
    (0x80144634, 80, "bd8196149d1e9a7db3942bd009d5cbbf9fe62adcc2bc6f847638ccc8dbb7508e"),
    (0x80144684, 432, "d4c40ea6dff8e92269a387f23e7f083490278aa1fbdd33a7997e199830c3227d"),
    (0x80176EC0, 304, "5653723869f2da1fb02e26d8704af5ed03a8480dd393a015f0b24cfa4fc23e8c"),
    (0x8016027C, 256, "afb4c5e5f728a90e7c4fd17d1516a08da49a5e6368e231c2c2fae160bdffdb48"),
    (0x80160640, 244, "e2f8e869d6900b6026fba6d88e3fc011c8a46871e228745581a0845c84c820fe"),
    (0x801B6868, 256, "d3b02e1263e3f099021369e0066b9fc62cfcfdd91ab0c5c32815ccbe12de1065"),
    (0x801B6C2C, 244, "29fb2f1634fa43fb559622d05b13b582b5b4085637ce33fd3821a016a3137ba5"),
)


@dataclass(frozen=True)
class Route:
    model: int
    key: int
    amount_index: int
    hp: int
    release_sites: tuple[int, int]
    profile: str
    table: int
    program: int
    hit_sha256: str
    actor_sha256: str


ROUTES = (
    Route(0x0902, 0, 5, 150, (0x801602FC, 0x801606C0), "p127", 0x80348910, 0x8034823A, *vm.BLOCKS[0x0902]),
    Route(0x0909, 2, 6, 180, (0x8016030C, 0x801606D0), "p127", 0x80348910, 0x8034823A,
          "7d0c526c98b42160c5c74a634b6fee1722ca452fe4277f43e26c8746fc2cae17",
          "e5b5e29ab1620c042523a0ec82aeb0ae2d35fa6871cb5098ecb92f6400f0aa1a"),
    Route(0x0907, 1, 5, 150, (0x801B68E8, 0x801B6CAC), "p132", 0x80381448, 0x8038207A,
          vm.BLOCKS[0x0902][0], "5255ee7f2a6c847bf3aebde16c72a0ef3141696cb3a27bb89572d2998493318d"),
    Route(0x090C, 3, 6, 180, (0x801B68F8, 0x801B6CBC), "p132", 0x80381448, 0x8038207A,
          "e1e06bbec8fbf66cf43ac6e8ff317c8d569f4e6f0ef5dac6ddc8b79895d7bfa6",
          "52269f1f94b4ee43a59e07bf8b86741cf834b7af6f644ac2d0a74e77cc0e39da"),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(ROUTES)*len(CASES):
        raise ValueError("type-60 report extent")
    rows = []
    for r in ROUTES:
        for i, case in enumerate(CASES):
            asked, made, publish = i < 8, i < 6, i < 5
            contact = i in (0, 1, 3, 4)
            effect = i in (0, 1)
            selector = 1+r.key%2
            descriptors = DESCRIPTORS[i]-4*made-3*effect
            values = (r.model, i, asked, made, publish, contact, selector if contact else 0, contact,
                      effect, 180, contact, contact, contact, descriptors, contact, contact)
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != values:
                raise ValueError(f"type-60 release, ownership or contact changed: {r.model:04X}/{case}")
            rows.append({
                "borg": f"gg4e08:{r.model:04x}", "case": case, "stock_spent": int(asked), "carrier_requested": asked,
                "carrier_allocated": made, "ordinary_shape_published": publish, "receiver_linked": contact,
                "receiver_history_token": int(contact), "amount_calls": int(contact), "hp_writer_argument": 0 if contact else None,
                "receiver_hp_after": 180, "type_33_arguments": [selector, 1] if contact else None,
                "type_33_owner": "struck-sasuke" if effect else None, "retained_pair_refused": contact,
                "descriptors_after_first_pair": descriptors,
            })
    return rows


def source_routes(image, archive, atlas):
    table = image.read(FACTORY_TABLE, 4*44, "data")
    if sha256(table).hexdigest() != "82ea41fdf34c93328e44ce9fc025f778d8e5b1b7ccde81adc79b2a829ef5c4f3":
        raise ValueError("type-60 original factory table extent or bytes changed")
    sites = vm.callers(image, atlas, FACTORY, 2048)
    if set(sites) != {p for r in ROUTES for p in r.release_sites}:
        raise ValueError("type-60 indexed release site coverage changed")
    borgs = {r["id"]: r for r in atlas["borgs"]}
    source, payload = [], b""
    for r in ROUTES:
        key = f"gg4e08:{r.model:04x}"
        borg = borgs[key]
        profile = atlas["profiles"][r.profile]
        if r.profile not in borg["profiles"] or int(borg["reaction_table"], 16) != r.table:
            raise ValueError("type-60 installed profile changed")
        x = next(row for row in profile["groups"] if row["group"] == "X")
        phase_tables = {int(row["code_path"]["phase_table"], 16) for row in x["rows"] if row["state"] == 2}
        phases = {vm.beam.word(image, addr+8, "data") for addr in phase_tables}
        if phases != {int(sites[p]["function"], 16) for p in r.release_sites}:
            raise ValueError("type-60 release site is not owned by its X profile")
        for p in r.release_sites:
            vm.beam.link(image, p, FACTORY)
            if vm.beam.word(image, p-4) != (0x38800000 | r.key):
                raise ValueError("type-60 exact key literal changed")
        bank = int(profile["program_and_descriptor_roots"][0], 16)
        group = bank+struct.unpack(">h", image.read(bank+8, 2, "data"))[0]
        entry = group+struct.unpack(">h", image.read(group, 2, "data"))[0]
        if entry != r.program or image.read(entry+44, 8, "data") != bytes.fromhex("0200010100000000"):
            raise ValueError("type-60 program event source changed")
        hit, spec = vm.opening.read_normal(archive, r.model, r.hit_sha256, r.actor_sha256)
        row = r.table+24*r.amount_index
        record = image.read(row, 24, "data")
        selector = 1+r.key%2
        if (vm.reaction_emit.read_group(hit, 6) != [(7, r.amount_index)] or
            struct.unpack_from(">BBHHhh", hit, 0xAA0+7*80) != (0, 0, 1, r.amount_index, 0, -1) or
            struct.unpack_from(">f", hit, 0xAA0+7*80+24)[0] != 50 or
            struct.unpack_from(">Bxhh", table, r.key*44) != (7, 6, selector) or
            struct.unpack_from(">hh", record, 16) != (selector*4, 16 if selector == 1 else 0) or
            record[:4] != bytes(4) or vm.levels(image, r.model)[0][0] != r.hp):
            raise ValueError("type-60 exact queue, effect arm or HP operands changed")
        source.append({
            "borg": key, "profile": r.profile, "key": r.key, "release_site": vm.address(r.release_sites[0]),
            "static_second_release_site": vm.address(r.release_sites[1]), "program_4_0": vm.address(entry),
            "event_1_1": vm.address(entry+44), "queue_group": 6, "shape_step": 7, "socket": 7, "radius": 50,
            "amount_record": vm.address(row), "raw_hp_operand": 0, "effect_arm": selector*4,
        })
        payload += hit+spec
    return source, payload


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-60 output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = vm.DolImage(vm.read_bounded(dol, 8 << 20), vm.SOURCE_SHA256)
    vm.beam.verify_text(image, (*vm.TEXT, *TEXT))
    for site, target in ((0x801446C4, 0x8008AC80), (0x8002F798, 0x80176EC0), (0x8002F848, 0x80176EC0),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        vm.beam.link(image, site, target)
    atlas = vm.read_metadata(vm.MAPS / "gg4e08-code-atlas.json", vm.ATLAS_SHA256)
    with vm.open_regular(afs) as stream:
        stamp = vm.archive_stamp(stream)
        vm.verify_archive(stream)
        archive = vm.RetailArchive(stream)
        hurt, defender = vm.opening.read_normal(archive, vm.opening.RECEIVER, vm.opening.RECEIVER_BLOCK_SHA256,
                                               vm.ACTOR_SHA256[vm.opening.RECEIVER])
        source, payload = source_routes(image, archive, atlas)
        if vm.archive_stamp(stream) != stamp:
            raise ValueError("type-60 archive input changed")
    cases = decode(vm.clock.fixture_report(fixture, dol, module, payload=hurt+defender+payload))
    raw = vm.encode({
        "kind": "gf-retail-type60-contact", "revision": 1,
        "source_dol_sha256": vm.SOURCE_SHA256, "source_afs_sha256": vm.AFS_SHA256, "code_atlas_sha256": vm.ATLAS_SHA256,
        "entries": {"carrier_factory": "8014452C", "carrier_initializer": "80144634", "group_bind": "8008AC80",
                    "ordinary_pair": "8002DB58", "amount": "8003CD5C", "hp_writer": "8003D344", "effect_factory": "80176EC0"},
        "sources": source, "cases": cases,
        "notes": [
            "Eight indexed factory sites belong to two X release leaves per class family. Only the first leaf of each family is executed.",
            "Program 4:0 contains the exact event. The release fixture declares the event and mode-0 stock instead of running the live input and VM schedule.",
            "Carrier setup stops at presentation allocation after the original bind and socket calculation. A supplied socket poses the carrier.",
            "The opponent and ally controls use a struck Sasuke distinct from the stored target. Type-33 requests and allocation are observed separately.",
            "Repeated pair visits keep the initial contact state, with missing effect resources offered late. This is not a fresh token or a live refresh test.",
            "No continued flight, complete team eligibility, recipient scale mutation, effect lifetime, rendering or native behavior is qualified.",
        ],
    }, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "indexed_release_sites": 8}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
