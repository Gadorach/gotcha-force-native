"""Verify normal roster body-queue allocation and original registry publication."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import reaction_emit
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_contact_pose_clock import fixture_report
from tools.analyze.retail_projectile_census import AFS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.analyze.retail_type32_roster import link, verify_text
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
TEXT = (
    (0x8003CC24, 312, "7fcf39fdb005df817b0de481e6860f17a2a8bb8c0d9fc7b82cb11882b2c9b415"),
    (0x8008AB30, 336, "9af045cf898354aadd281a64834e3cdde963848553ea25260df7d4f3de9b439b"),
    (0x80088AA0, 416, "800882bf558ba7e7f9e955e1884be71e0c9aad8870807bf7dc0876f56baaeab6"),
    (0x80088D98, 184, "a134e40d302eb33d6ebc1c3dd371228809562bdc533d833bebf095b19180ab44"),
    (0x8008A28C, 48, "94cb040d0d0bd380e6ab2d81aec5d82583aaf212f52408a7050212f7e39acc08"),
    (0x80089AEC, 276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    (0x8008A65C, 996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
    (0x8008AA40, 240, "48b0c49de83b28a396e02f233ec8298596003ab69f3746744c69ab3acb14d682"),
    (0x8008A2BC, 788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
    (0x8008AFF0, 32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
    (0x80088E50, 328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
)
VIEWS = (("reset-ef", 0xEF, False, 0xFFFFFFFF), ("declared-c3", 0xC3, False, 0xFFFFFFFF),
         ("open", 0, False, 0xFFFFFFFF), ("ordinary-receive-guard", 0, True, 0xFFFFFFFF),
         ("joint-0-only", 0, False, 1), ("joints-closed", 0, False, 0))
REGISTRIES = (
    ("ordinary-attack", 0x01), ("ordinary-receiver", 0x02), ("role-04", 0x04), ("role-08", 0x08),
    ("role-40", 0x40), ("role-10", 0x10), ("role-20", 0x20), ("role-400", 0x400),
    ("alternate-attack", 0x80), ("alternate-receiver", 0x02),
)
CAPACITIES = (32, 1, 0)


@dataclass(frozen=True)
class Shape:
    descriptor: int
    kind: int
    matrix: int
    roles: int


def body_queue(block):
    if len(block) != reaction_emit.HIT_BYTES:
        raise ValueError("normal body bank extent")
    queue = []
    for step in block[:32]:
        if step >= 0x80:
            break
        if step >= 33:
            raise ValueError("normal body descriptor outside bank")
        start = 32+80*step
        kind, matrix, roles, _, first, last = struct.unpack_from(">BBHHhh", block, start)
        if kind > 5 or (matrix >= 32 and matrix != 0x80) or roles & 0x81 or (first, last) != (0, -1):
            raise ValueError("normal body descriptor outside qualified role, matrix or phase domain")
        queue.append(Shape(step, kind, matrix, roles))
    if not queue:
        raise ValueError("empty normal body queue")
    return queue


def publication_bits(queue, role, mask, receive_guard, joints):
    if role & mask or (role == 2 and receive_guard):
        return 0
    return sum(1 << ordinal for ordinal, row in enumerate(queue)
               if row.roles & role and joints & (1 << (row.matrix & 31)))


def source_inputs(afs, installed):
    models = [int(row["id"][-4:], 16) for row in installed["borgs"]]
    if len(models) != 206 or sorted(set(models)) != models:
        raise ValueError("normal body registry roster identity")
    payload, sources = bytearray(), []
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        for model in models:
            raw = reaction_emit.qualified_hit_block(archive, model)
            sources.append((model, sha256(raw).hexdigest(), body_queue(raw)))
            payload += struct.pack(">H", model) + raw
        if archive_stamp(stream) != stamp:
            raise ValueError("normal body registry source changed")
    return bytes(payload), sources


def decode(report, sources):
    lines = report.splitlines()
    if len(report) > 512_000 or len(lines) != len(sources)*len(CAPACITIES)*len(VIEWS):
        raise ValueError("normal body registry report extent")
    pos = 0
    rows, totals = [], {name: Counter() for name, *_ in VIEWS}
    for model, digest, queue in sources:
        proven = {}
        for capacity in CAPACITIES:
            given = queue[:capacity]
            special = [i for i, shape in enumerate(given) if shape.roles & 0x200][:4]
            views = {}
            for v, (name, mask, hold, joints) in enumerate(VIEWS):
                bits = [publication_bits(given, role, mask, hold and key == "ordinary-receiver", joints)
                        for key, role in REGISTRIES]
                fields = (f"{model:04X}", capacity, v, len(given), len(special), *(f"{b:08X}" for b in bits))
                if lines[pos] != "body-registry: " + " ".join(map(str, fields)):
                    raise ValueError(f"normal body registry changed at {model:04X} capacity {capacity}, {name}")
                pos += 1
                views[name] = {
                    key: [i for i in reversed(range(len(given))) if bitmap & (1 << i)]
                    for (key, _), bitmap in zip(REGISTRIES, bits, strict=True) if bitmap
                }
                if capacity == 32:
                    for (key, _), bitmap in zip(REGISTRIES, bits, strict=True):
                        totals[name][key] += bitmap.bit_count()
            proven[str(capacity)] = {
                "bound_cell_count": len(given), "secondary_index_queue_ordinals": special,
                "published_queue_ordinals": views,
            }
        rows.append({
            "borg": f"gg4e08:{model:04x}", "hit_bank_sha256": digest, "body_descriptor_queue": [
                [r.descriptor, r.kind, r.matrix, f"{r.roles:04X}"] for r in queue
            ], "observed_capacity_cases": proven,
        })
    return rows, {k: dict(v) for k, v in totals.items()}


def verify_source(image):
    verify_text(image, TEXT)
    for site, target in ((0x80054444, 0x8003CC24), (0x800544A8, 0x8008AB30), (0x8008AB84, 0x80088AA0),
                         (0x80088AF0, 0x80088D98), (0x8008A69C, 0x80088E50), (0x8008A9DC, 0x8008AA40),
                         (0x8008A9EC, 0x8008A2BC)):
        link(image, site, target)
    if image.read(0x802DA740, 48, "data") != bytes.fromhex(
        "8003a5e48003acf88003b3048003c0808003c6148003ca488003a4b88003a8fc8003b1708003bd888003c3d88003c8b4"
    ):
        raise ValueError("normal body shape callback table changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body registry output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    verify_source(image)
    payload, sources = source_inputs(afs, read_metadata(MAPS / "gg4e08-standalone-installers.json", INSTALLERS_SHA256))
    rows, totals = decode(fixture_report(fixture, dol, module, payload=payload), sources)
    queue_entries = sum(len(queue) for _, _, queue in sources)
    duplicates = [r["borg"] for r in rows if len({d[0] for d in r["body_descriptor_queue"]}) != len(r["body_descriptor_queue"])]
    raw = encode({
        "kind": "gf-retail-body-registry", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "full_capacity_seats": list(range(6)), "limited_capacity_seat": 0, "shape_capacities": list(CAPACITIES),
        "queue_entry_fields": ["descriptor", "shape_kind", "matrix_selector", "role_word_hex"],
        "masks_and_guard": {name: {"role_mask": f"{mask:04X}", "positive_720": hold, "joint_enable_bits": f"{joints:08X}"}
                            for name, mask, hold, joints in VIEWS},
        "queue_entries": queue_entries, "borgs_with_repeated_descriptors": duplicates,
        "full_capacity_registry_totals_per_one_seat": totals, "models": rows,
        "notes": [
            "Each normal package block 2 matches its loose hit member. These are exact body descriptors, not the separate action collision queues.",
            "The original class-4 binder consumes queue entries in order and prepends the cells. Published lists retain the resulting reverse queue order. Repeated source descriptor numbers bind distinct cells.",
            "The original collision reset supplies mask 00EF. The joint enables, later masks and positive receiver guard are declared inlets; each Borg's actor startup, motion and pad path are not replayed.",
            "Actor and matrix ownership are checked with declared rigid matrices. No pose geometry, collision pairs, damage authoring, combined forms, or native gameplay are qualified.",
            "After six publication checks, the original body close and next rebuild remove the live cells and return exactly the offered free cells.",
        ],
    }, expanded=("models",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "borgs": len(rows), "body_queue_entries": queue_entries,
            "bindings_and_close_probes": 8*len(rows), "publication_visits": 8*len(rows)*len(VIEWS), "registry_totals": totals}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
