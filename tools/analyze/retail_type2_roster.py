"""Census exact type-2 carrier operands and static roster release candidates."""
import argparse
from collections import Counter
from copy import deepcopy
from hashlib import sha256
import json
from math import isfinite
from pathlib import Path
import struct

from tools.analyze import reaction_attack_table, reaction_emit, reaction_installer_tables
from tools.analyze.retail_action_links import ActionLinker, direct_branch
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded, scan
from tools.analyze.retail_projectile_origins import TABLES, origin_tables, release_index
from tools.analyze.retail_seeded_shot_pose import CYBER_MACHINE_SHOTS
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage

ATLAS_SHA256 = "1bfb0ac7d8885156336b7de2e40e0019daca170b25b4afd762bf172662f36eae"
INSTALLERS_SHA256 = "afc01c017966b0646bba7654f052d8d2fe5fef6989bb217524e71c831608906b"
FACTORY, DISPATCH = 0x800C3BE0, 0x803051E4
PHASES = (0x800C3D2C, 0x800C4088, 0x800C426C, 0x800C4448)
EXIT_PHASES = (0x800C42A8, 0x800C42BC, 0x800C4308)
BASE, STRIDE, COUNT, _, _ = TABLES["type-2"]


def read_metadata(path, digest):
    raw = read_bounded(path, 4 << 20)
    if sha256(raw).hexdigest() != digest:
        raise ValueError("atlas or standalone installer receipt identity mismatch")
    return json.loads(raw)


def carrier_rows(image, origins):
    if struct.unpack(">7I", image.read(DISPATCH, 28, "data")) != PHASES + EXIT_PHASES:
        raise ValueError("type-2 phase tables disagree")
    result = []
    for key, origin in enumerate(origins):
        start = BASE + STRIDE * key
        raw = image.read(start, STRIDE, "data")
        baked = reaction_emit.signed_byte(raw[2])
        bearing, flip = raw[28], reaction_emit.signed_byte(raw[29])
        speed, delta, cap = struct.unpack_from(">3f", raw, 44)
        if (baked >= reaction_emit.GROUPS or flip not in (-1, 1)
                or any(not isfinite(v) or not 0 <= v <= 1e5 for v in (speed, delta, cap)) or speed > cap):
            raise ValueError(f"type-2 unsupported operand at key {key}")
        helper = {0x00: "never", 0x80: "when-target-present", 0xC0: "even-without-target"}.get(bearing & 0xC0)
        axis = None if helper == "even-without-target" else (bearing & 0x7F)
        if helper is None or (axis is not None and axis >= 3):
            raise ValueError(f"type-2 unknown direction operands at key {key}")
        result.append({
            **origin, "row": address(start), "collision_group_selector": baked,
            "bearing_control": bearing, "bearing_helper": helper, "matrix_axis_if_no_helper": axis,
            "reverse_matrix_axis": flip < 0 if axis is not None else None,
            "initial_speed": speed, "speed_add_per_active_visit": delta, "speed_cap": cap,
        })
    if len(result) != COUNT:
        raise ValueError("type-2 table extent")
    return result


def callers(image, atlas):
    sites = {}
    for entry, record in atlas["functions"].items():
        start = int(entry, 16)
        for offset, target, kind in record.get("direct", ()):
            if target != address(FACTORY) or kind != "call":
                continue
            code = image.read(start, record["size"], "text")
            transfers, _, _ = scan(start, code)
            site = start + offset
            if (offset, target, kind) not in transfers or site in sites:
                raise ValueError(f"factory edge disagrees at {address(site)}")
            previous = struct.unpack_from(">I", code, offset - 4)[0] if offset >= 4 else 0
            key = previous & 0xFFFF
            sites[site] = {
                "site": address(site), "function": entry,
                "adjacent_li_r4_key": key if previous >> 16 == 0x3880 and key < COUNT else None,
            }
    return sites


def queue_metadata(image, block, table, group):
    if group < 0:
        return {"status": "negative-selector-skips-bind"}
    steps = reaction_emit.read_group(block, group)
    if not steps:
        return {"status": "empty-source-queue"}
    if any(index >= reaction_attack_table.COUNT for _, index in steps):
        return {"status": "unqualified-reaction-index"}
    rows = []
    for step, index in steps:
        record = table + index * reaction_attack_table.SIZE
        power, = struct.unpack(">H", image.read(record, 2, "data"))
        rows.append({"descriptor": step, "reaction_index": index, "raw_power": power})
    powers = {row["raw_power"] for row in rows}
    kind = "zero-power" if powers == {0} else "mixed-power" if len(powers) != 1 else "uniform-positive-power"
    return {"status": kind, "steps": rows}


def recover(image, archive, atlas, selections):
    if (image.sha256 != SOURCE_SHA256 or atlas["source_dol_sha256"] != SOURCE_SHA256
            or selections["source_dol_sha256"] != SOURCE_SHA256):
        raise ValueError("type-2 roster source identity")
    origins = origin_tables(image)
    rows = carrier_rows(image, origins["type-2"])
    sites = callers(image, atlas)
    linker = ActionLinker(image, origins, ())
    models = {row["id"]: row for row in atlas["borgs"]}
    picked = {row["id"]: row for row in selections["borgs"]}
    if models.keys() != picked.keys() or len(models) != 206 or len(picked) != len(selections["borgs"]):
        raise ValueError("type-2 installer coverage")
    candidates = []
    for ident in sorted(models):
        static, selected = models[ident], picked[ident]
        model = int(ident.removeprefix("gg4e08:"), 16)
        installer, table = reaction_installer_tables.model_table(image, model)
        profile = selected["profile"]
        if (profile not in static["profiles"] or address(installer) != static["installer"]
                or selected["installer"] != static["installer"] or not table or address(table) != static["reaction_table"]):
            raise ValueError(f"selected profile or reaction table disagrees for {ident}")
        actions = {"profiles": [deepcopy(atlas["profiles"][profile])]}
        linker.attach(actions, model, (), ())  # No pose/rig supplied; only static factory candidates.
        releases = [r for r in release_index(actions) if r["family"] == "type-2"]
        if not releases:
            continue
        unresolved = ("combined-form-ownership-unqualified" if model in CYBER_MACHINE_SHOTS else
                      "alternate-spec-unqualified" if selected["spec_record"] != 0 else None)
        block = None if unresolved else reaction_emit.qualified_hit_block(archive, model)
        for release in releases:
            pc, key = int(release["site"], 16), release["key"]
            word = linker.text(pc)
            if (pc not in sites or word & 0xFC000003 != 0x48000001 or direct_branch(pc, word) != FACTORY
                    or not 0 <= key < COUNT or sites[pc]["adjacent_li_r4_key"] not in (None, key)
                    or {route["argument_evidence"] for route in release["routes"]} != {"literal-block"}):
                raise ValueError(f"unqualified type-2 release candidate at {address(pc)}")
            group = rows[key]["collision_group_selector"]
            queue = {"status": unresolved} if unresolved else queue_metadata(image, block, table, group)
            contexts = [{
                "group": route["action_group"], "state": route["state"], "substate": route["substate"],
                "selector": route["selector"], "context_rows": route["context_rows"], "scope": route["scope"],
                "release_entry": route.get("release_entry"),
                **({"relay_site": route["relay_site"], "relay_handler": route["relay_handler"]}
                   if route["relay_site"] else {}),
            } for route in release["routes"]]
            candidates.append({
                "borg": ident, "profile": profile, "spec_record": selected["spec_record"],
                "site": address(pc), "key": key, "action_candidates": contexts, "queue": queue,
                "required_model_at_call": release.get("required_model_at_call"),
            })
    joined = {r["site"] for r in candidates}
    counts = {
        "roster_profiles_examined": len(picked), "factory_rows": len(rows), "direct_call_sites": len(sites),
        "call_sites_with_adjacent_li": sum(r["adjacent_li_r4_key"] is not None for r in sites.values()),
        "models_with_candidates": len({r["borg"] for r in candidates}), "joined_call_sites": len(joined),
        "unjoined_call_sites": len(sites) - len(joined), "candidate_rows": len(candidates),
        "joined_keys": len({r["key"] for r in candidates}),
        "action_context_sets": sum(len(r["action_candidates"]) for r in candidates),
        "queue_statuses": dict(Counter(r["queue"]["status"] for r in candidates)),
        "speed_triplets": len({(r["initial_speed"], r["speed_add_per_active_visit"], r["speed_cap"]) for r in rows}),
        "bearing_helpers": dict(Counter(r["bearing_helper"] for r in rows)),
    }
    return {
        "kind": "gf-retail-type2-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "factory": address(FACTORY), "phase_table": address(DISPATCH), "phase_entries": list(map(address, PHASES)),
        "exit_subphase_entries": list(map(address, EXIT_PHASES)), "counts": counts,
        "factory_rows": rows, "call_sites": [sites[s] for s in sorted(sites)], "candidates": candidates,
        "notes": [
            "Selected profiles come from bounded standalone installer execution; the calls associated with those profiles are static candidates.",
            "An adjacent li is a local source observation, not argument proof at every branch predecessor.",
            "Normal owner descriptors describe raw power only. Combined-form and alternate-spec joins are withheld; no route is admitted to gameplay.",
            "Direction classification describes the bearing branch after any target validation. It does not qualify a camera, aim target, or rendered muzzle.",
            "Only Machine Red key 21 has the accompanying joined lifetime execution. Other keys do not inherit its result.",
        ],
    }


def encode(payload):
    def compact(value):
        return json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    expanded = {"factory_rows", "call_sites", "candidates"}
    items = []
    for key, value in sorted(payload.items()):
        text = "[\n" + ",\n".join("    " + compact(row) for row in value) + "\n  ]" if key in expanded else compact(value)
        items.append(f"  {compact(key)}: {text}")
    return ("{\n" + ",\n".join(items) + "\n}\n").encode("ascii")


def export(dol, afs, atlas, selections, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-2 roster output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    atlas = read_metadata(atlas, ATLAS_SHA256)
    selections = read_metadata(selections, INSTALLERS_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        payload = recover(image, RetailArchive(stream), atlas, selections)
        if stamp != archive_stamp(stream):
            raise ValueError("type-2 source AFS changed during census")
    raw = encode(payload)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.selections, args.output), sort_keys=True))
