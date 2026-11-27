"""Shared static joins for pinned projectile carrier census exporters."""
from collections import Counter
from copy import deepcopy
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import reaction_attack_table, reaction_emit, reaction_installer_tables
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_action_links import ActionLinker, direct_branch
from tools.analyze.retail_code_atlas import address, read_bounded, scan
from tools.analyze.retail_projectile_origins import release_index
from tools.analyze.retail_seeded_shot_pose import CYBER_MACHINE_SHOTS
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage

ATLAS_SHA256 = "1bfb0ac7d8885156336b7de2e40e0019daca170b25b4afd762bf172662f36eae"
INSTALLERS_SHA256 = "afc01c017966b0646bba7654f052d8d2fe5fef6989bb217524e71c831608906b"


def read_metadata(path, digest):
    raw = read_bounded(path, 4 << 20)
    if sha256(raw).hexdigest() != digest:
        raise ValueError("atlas or standalone installer receipt identity mismatch")
    return json.loads(raw)


def callers(image, atlas, factory, key_count):
    sites = {}
    for entry, record in atlas["functions"].items():
        start = int(entry, 16)
        for offset, target, kind in record.get("direct", ()):
            if target != address(factory) or kind != "call":
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
                "adjacent_li_r4_key": key if previous >> 16 == 0x3880 and key < key_count else None,
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


def candidate_census(image, archive, atlas, selections, origins, family, factory, rows, selector_proof=None):
    if (image.sha256 != SOURCE_SHA256 or atlas["source_dol_sha256"] != SOURCE_SHA256
            or selections["source_dol_sha256"] != SOURCE_SHA256):
        raise ValueError(f"{family} roster source identity")
    sites = callers(image, atlas, factory, len(rows))
    linker = ActionLinker(image, origins, ())
    models = {row["id"]: row for row in atlas["borgs"]}
    picked = {row["id"]: row for row in selections["borgs"]}
    if models.keys() != picked.keys() or any(len(data["borgs"]) != 206 for data in (atlas, selections)) or len(picked) != 206:
        raise ValueError(f"{family} installer coverage")
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
        linker.attach(actions, model, (), ())  # No local pose/rig; only available static factory candidates.
        releases = [r for r in release_index(actions) if r["family"] == family]
        if not releases:
            continue
        unresolved = ("combined-form-ownership-unqualified" if model in CYBER_MACHINE_SHOTS else
                      "alternate-spec-unqualified" if selected["spec_record"] != 0 else None)
        block = None if unresolved else reaction_emit.qualified_hit_block(archive, model)
        for release in releases:
            pc, key = int(release["site"], 16), release["key"]
            word = linker.text(pc)
            evidence = {route["argument_evidence"] for route in release["routes"]}
            literal = evidence == {"literal-block"}
            extra = {} if literal else selector_proof(ident, release, evidence) if selector_proof else None
            if (pc not in sites or word & 0xFC000003 != 0x48000001 or direct_branch(pc, word) != factory
                    or not 0 <= key < len(rows) or (literal and sites[pc]["adjacent_li_r4_key"] not in (None, key))
                    or extra is None):
                raise ValueError(f"unqualified {family} release candidate at {address(pc)}")
            group = rows[key]["collision_group_selector"]
            scope = unresolved or ("unresolved-origin" if rows[key]["basis"] == "unresolved" else None)
            queue = {"status": scope} if scope else queue_metadata(image, block, table, group)
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
                "required_model_at_call": release.get("required_model_at_call"), **extra,
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
    }
    return counts, [sites[s] for s in sorted(sites)], candidates


def encode(payload, *, expanded=("factory_rows", "call_sites", "candidates")):
    def compact(value):
        return json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    items = []
    for key, value in sorted(payload.items()):
        text = "[\n" + ",\n".join("    " + compact(row) for row in value) + "\n  ]" if key in expanded else compact(value)
        items.append(f"  {compact(key)}: {text}")
    return ("{\n" + ",\n".join(items) + "\n}\n").encode("ascii")


def export_census(dol, afs, atlas, selections, output, recover):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("projectile census output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    atlas = read_metadata(atlas, ATLAS_SHA256)
    selections = read_metadata(selections, INSTALLERS_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        payload = recover(image, RetailArchive(stream), atlas, selections)
        if stamp != archive_stamp(stream):
            raise ValueError("source AFS changed during projectile census")
    raw = encode(payload)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}
