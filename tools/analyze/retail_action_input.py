"""Map GG4E08 processed-pad requests and roster-wide charge operands."""
import argparse
from collections import Counter
from hashlib import sha256
import json
import math
import os
from pathlib import Path
import struct

from tools.analyze.retail_actor_data_binding import actor_record_binding, binding_schema
from tools.analyze.retail_code_atlas import MAX_SYMBOLS_BYTES, address, object_sizes, read_bounded, roster_paths
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import CATALOG_SHA256, open_regular, read_catalog
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive, source_table
from tools.extract.source_image import DolImage

AFS_SHA256 = "a9fc30d91c74ec232ad9602eb7783af54f7a3aa7baa19736471b9e0b15d76ce5"
MAX_AFS_BYTES = 512 << 20
PAD_ROWS = 0x802D38F8
PAD_HANDLERS = 0x802D3988
CHARGE_HELD = 0x802D3970
CHARGE_RELEASE = 0x802D397C
COMMAND_MASKS = (0x20, 0x40, 0x80, 0x200, 1, 2, 4, 8, 0x10, 0x2000)
PROBES = (
    ("linked-action", 0x80069CB0, 5, None, None),
    ("charged-X", 0x80069C50, 5, 0x1000, 0x20),
    ("charged-B", 0x80069BF0, 3, 0x400, 0x08),
    ("ordinary-X", 0x80069B98, 2, 0x80, 0x04),
    ("close-B", 0x80069B10, 1, 0x40, 0x02),
    ("ordinary-B", 0x80069A88, None, 0x20, None),
)
SPANS = {
    0x80056900: (424, "52c4c7e19500682352bbf4503d8973d50f69fed24c5525d5b3ab40bba34bbfa2"),
    0x8008BBC0: (16, "257d48a7d554a62bc9c18c1daf1fa73860c81815999016f64a0a7b01f7cda582"),
    0x800685C0: (304, "ce30ce82c8a3802573cd9741dba970d10c207e00cb2307bc77d7031c1aae4758"),
    0x800686F0: (656, "55523dd4f938a0f11bb9f7a26ae31de5d5eb3647354af3315ba37979340f3817"),
    0x80068B8C: (136, "47cba4509016e06f1bd98132a48e436013e6ff74ff62478a8efc1060eede27a2"),
    0x80068C14: (112, "230e4f4b80234a1ff978cda142ff079bd9c11cfa0b271b7c1b3208a7339ff314"),
    0x80068C84: (128, "178de0acf5085e37b37b6581715136703b885a808261e30c7de13395067c7ab6"),
    0x80068D04: (244, "f077c452065b1619363a0b2190a4c35aa0b018223f9ad9c9fa144cbffe8bb923"),
    0x800699D8: (176, "c809fe8851e308079759e95d7cb982414ac6399dae30d964ee36ee00f0ba07fd"),
    0x80069CB0: (288, "948dca24959a3810bd95deeddc2109e709a342b405a633f060df63e5746299ab"),
    0x80069C50: (96, "64085e89f7385ffc9e08355146fe8c2ce9b61634ccca8bc8ea652174a28b37b2"),
    0x80069BF0: (96, "672733b12993bfdaf4141580b1c35f8914c4fb7f3e6139f877228278dba385fe"),
    0x80069B98: (88, "b1b819b1e9ff72f8e8154ac586bd3b362690f2fd099e3eb2ae73f0cc40e025cc"),
    0x80069B10: (136, "d314cddf30ff9cec52d98556ec16a0051b206c441c685e0e3db0eace3e1e113f"),
    0x80069A88: (136, "c7859e78b66a2dc8fae3d67fa0fe35809f941b7da353bb344b4d60173beb8c0d"),
}


def request_tables(image):
    for start, (size, digest) in SPANS.items():
        if sha256(image.read(start, size, "text")).hexdigest() != digest:
            raise ValueError(f"input signature mismatch at {address(start)}")
    handlers = struct.unpack(">3I", image.read(PAD_HANDLERS, 12, "data"))
    if handlers != (0x80068C14, 0x80068C84, 0x80068D04):
        raise ValueError("button conditioner state table mismatch")
    rows = []
    for i, row in enumerate(struct.iter_unpack(">6H", image.read(PAD_ROWS, 120, "data"))):
        stages, flags, _, pulses, mask, _ = row
        if stages != 1 or pulses not in (1, 16) or flags not in (0, 4, 16) or not mask:
            raise ValueError("button conditioner row domain")
        rows.append({"table_row": address(PAD_ROWS + 12 * i), "pad_mask": mask,
                     "actor_request_mask": COMMAND_MASKS[i], "request_pulses": pulses,
                     "condition": {0: "fresh-all", 4: "fresh-all and actor+0x5E0 bit 0x40",
                                   16: "fresh-any"}[flags]})
    held = struct.unpack(">3I", image.read(CHARGE_HELD, 12, "data"))
    release = struct.unpack(">3I", image.read(CHARGE_RELEASE, 12, "data"))
    if held != (0x200, 0x200, 0x400) or release != (0x400, 0x800, 0x1000):
        raise ValueError("charge conditioner table mismatch")
    channels = [{"group_index": 3 + i, "held_pad_mask": held[i], "release_request_mask": release[i],
                 "mode_field": f"config+0x{0xBB + i:X}", "limit_field": f"spec+0x{0x184 + 4 * i:X}"}
                for i in range(3)]
    return rows, channels


def charge_limits(spec):
    if len(spec) != ACTOR_BYTES:
        raise ValueError("charge spec extent")
    values = struct.unpack_from(">3f", spec, 0x184)
    if any(not math.isfinite(v) or not 0 <= v <= 10000 for v in values):
        raise ValueError("charge operand outside audited domain")
    return values


def charge_records(image, archive, catalog, borgs, profiles):
    binding_schema(image)
    by_id = {borg["id"]: borg for borg in borgs}
    counts, records = Counter(), []
    for row in catalog:
        borg = by_id[row.id]
        place = source_table(image, 0x802D0584, row.model)
        index, = struct.unpack(">I", image.read(place, 4, "data"))
        member, _ = archive.member(index, row.model)
        raw, _ = member.read(0, 8 * ACTOR_BYTES)
        if not raw or len(raw) % ACTOR_BYTES:
            raise ValueError(f"{row.id}: charge actor records")
        chunks = [raw[i:i + ACTOR_BYTES] for i in range(0, len(raw), ACTOR_BYTES)]
        digests = [{"record_sha256": sha256(chunk).hexdigest()} for chunk in chunks]
        binding = actor_record_binding(image, int(borg["installer"], 16), row.model, digests, ACTOR_BYTES)
        selected = binding["selected_record"]
        cases = []
        for ident in borg["profiles"]:
            active = {g["group"]: g["mode"] for g in profiles[ident]["groups"][3:] if g["mode"] in (0, 1)}
            counts["profile_references"] += 1
            if active:
                counts["charge_profile_references"] += 1
                cases.append({"profile": ident, "enabled_modes": active})
                counts.update(f"{name}_profile_references" for name in active)
        counts["borgs"] += 1
        counts["spec_record_resolved"] += selected is not None
        counts["borgs_with_charge_mode"] += bool(cases)
        if cases:
            counts.update(f"borgs_with_{name}" for name in {n for case in cases for n in case["enabled_modes"]})
        if selected is None:
            records.append({"id": row.id, "spec_record": None, "profiles_with_charge": cases,
                            "status": binding["status"]})
            continue
        values = charge_limits(chunks[selected])
        records.append({"id": row.id, "spec_record": selected, "limits": values,
                        "profiles_with_charge": cases})
    return counts, records


def recover(image, archive, catalog, symbols):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    rows, channels = request_tables(image)
    borgs, profiles, _ = roster_paths(image, catalog, object_sizes(symbols))
    counts, records = charge_records(image, archive, catalog, borgs, profiles)
    return {
        "kind": "gf-retail-action-input", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "catalog_sha256": CATALOG_SHA256,
        "symbol_metadata_sha256": sha256(symbols).hexdigest(),
        "packet": {
            "consumer": "80056900", "processed_pad_base": "803C727C", "port_stride": 32,
            "human_button_mask": 0x00FF1F7F,
            "held": "actor+0x5B4", "previous": "actor+0x5B8",
            "pressed": "actor+0x5BC", "released": "actor+0x5C0",
            "source_switch": "actor+0x488 == 3 becomes 0 and zeroes both edges for the visit",
        },
        "conditioner": {"entry": "800685C0", "request_word": "actor+0x5D4", "button_rows": rows},
        "arbitration": {
            "entry": "800699D8", "success": "first nonzero probe return stops lower priorities",
            "probes": [{"kind": kind, "entry": address(pc), "group": group, "request_mask": request,
                        "inhibit_mask_at_0x58E": inhibit} for kind, pc, group, request, inhibit in PROBES],
            "notes": [
                "The linked-action probe may consume priority without committing a row; the live linked path is not qualified.",
                "The close-B probe also checks actor status and close-admit state; ordinary B may choose group 0 or 1.",
                "A nonzero arbiter return is not a program, payload, or contact observation.",
            ],
        },
        "charge": {
            "entry": "800686F0", "channels": channels,
            "notes": [
                "The shared conditioner reads held inputs and publishes a release request if the previous accumulated value meets the spec limit.",
                "Spec limits are floats in the installer-selected slot-0 record, not proven visit counts.",
                "Profile mode -1 disables the corresponding shared accumulator. Group 4 has no enabled normal profile in this atlas.",
                "The profile list is static; live eligibility, clock rate, release timing, and payloads are not assigned.",
            ],
        },
        "counts": counts, "borgs": records,
    }


def verify_archive(stream):
    stream.seek(0, 2)
    if not 0 < stream.tell() <= MAX_AFS_BYTES:
        raise ValueError("charge source AFS extent")
    stream.seek(0)
    digest = sha256()
    size = 0
    for block in iter(lambda: stream.read(1 << 20), b""):
        size += len(block)
        if size > MAX_AFS_BYTES:
            raise ValueError("charge source AFS extent")
        digest.update(block)
    if digest.hexdigest() != AFS_SHA256:
        raise ValueError("charge source AFS revision mismatch")


def archive_stamp(stream):
    stat = os.fstat(stream.fileno())
    return stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns


def export(dol, afs, catalog, symbols, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("action input map output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    statics = read_bounded(symbols, MAX_SYMBOLS_BYTES)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        payload = recover(image, RetailArchive(stream), read_catalog(catalog), statics)
        if archive_stamp(stream) != stamp:
            raise ValueError("charge source AFS changed during extraction")
    items = []
    for key, value in sorted(payload.items()):
        text = ("[\n" + ",\n".join("    " + json.dumps(row, sort_keys=True) for row in value) + "\n  ]"
                if key == "borgs" else json.dumps(value, sort_keys=True))
        items.append(f"  {json.dumps(key)}: {text}")
    raw = ("{\n" + ",\n".join(items) + "\n}\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "catalog", "symbols", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.catalog, args.symbols, args.output), sort_keys=True))
