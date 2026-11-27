"""Join observed standalone GG4E08 installer writes to static action profiles."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze.retail_code_atlas import MAX_SYMBOLS_BYTES, boundaries, object_sizes, read_bounded, roster_paths
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import CATALOG_SHA256, RETAIL_ROWS, read_catalog
from tools.extract.retail_borg_roster import ACTOR_BYTES
from tools.extract.source_image import DolImage

OUTPUT_ROW = re.compile(r"([0-9A-F]{4})" + r" ([0-9A-F]{8})" * 12)


def plan(borgs):
    return "".join(f"{borg['id'].split(':')[1].upper()} {borg['installer']}\n" for borg in borgs)


def join(image, borgs, profiles, function_spans, trace):
    lines = trace.splitlines()
    if len(lines) != len(borgs) or len(trace) > RETAIL_ROWS * 128:
        raise ValueError("incomplete or oversized installer fixture report")
    selected, counts, models = set(), Counter(), []
    for row, borg in zip(lines, borgs, strict=True):
        match = OUTPUT_ROW.fullmatch(row)
        if match is None:
            raise ValueError("malformed installer fixture row")
        model, installer, config, context, selector, main, pre, post, offset, *roots = match.groups()
        if borg["id"] != f"gg4e08:{model.lower()}" or installer != borg["installer"]:
            raise ValueError("installer fixture identity disagreement")
        identity = (config, context, selector, main, roots)
        matches = []
        for pid in borg["profiles"]:
            profile = profiles[pid]
            if identity == (profile["config"], profile["context_table"], profile["selector_table"],
                            profile["main_handler"], profile["program_and_descriptor_roots"]):
                matches.append(pid)
        if len(matches) != 1:
            raise ValueError(f"{borg['id']}: observed configuration is not one static profile")
        if any(int(value, 16) not in function_spans for value in (main, pre, post)):
            raise ValueError(f"{borg['id']}: observed callback is not a function entry")
        spec, rest = divmod(int(offset, 16), ACTOR_BYTES)
        if rest or spec >= 8:
            raise ValueError(f"{borg['id']}: observed resource offset is not a slot-0 record")

        pid = matches[0]
        enabled = {g["group"]: g["mode"] for g in profiles[pid]["groups"][3:] if g["mode"] in (0, 1)}
        selected.add(pid)
        counts["with_profile_alternatives"] += len(borg["profiles"]) > 1
        counts["static_profile_references"] += len(borg["profiles"])
        counts["with_charge"] += bool(enabled)
        counts["with_B-charge"] += "B-charge" in enabled
        counts["with_X-charge"] += "X-charge" in enabled
        counts["with_both"] += "B-charge" in enabled and "X-charge" in enabled
        models.append({"id": borg["id"], "installer": installer, "profile": pid,
                       "pre_callback": pre, "post_callback": post, "spec_record": spec,
                       "config_lane_mask_seed": image.read(int(config, 16) + 2, 1, "data")[0],
                       "enabled_charge_modes": enabled})
    counts["borgs"] = len(models)
    counts["distinct_selected_profiles"] = len(selected)
    return counts, models


def export(dol, catalog, symbols, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("installer execution map output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    metadata = read_bounded(symbols, MAX_SYMBOLS_BYTES)
    catalog_rows = read_catalog(catalog)
    borgs, profiles, _ = roster_paths(image, catalog_rows, object_sizes(metadata))
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, input=plan(borgs), encoding="ascii", capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"installer fixture failed: {result.stderr.strip()}")
    counts, records = join(image, borgs, profiles, boundaries(image, metadata), result.stdout)
    payload = {
        "kind": "gf-retail-standalone-installers", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "catalog_sha256": CATALOG_SHA256, "symbol_metadata_sha256": sha256(metadata).hexdigest(),
        "scope": "one isolated original installer execution per catalogued model; zero standalone, variant and role fields",
        "notes": [
            "Fixture supplies the role-0 resource pointer; record index is computed from the returned offset.",
            "The config lane seed is read at config+2, not an observed constructor write.",
            "Matched profile means all seven action-table pointers and the main handler agreed with exactly one static profile.",
            "Not a full constructor, scene, action scheduler, mode/variant or live asset-load qualification.",
        ],
        "counts": counts, "borgs": records,
    }
    items = []
    for key, value in sorted(payload.items()):
        text = ("[\n" + ",\n".join("    " + json.dumps(row, sort_keys=True) for row in value) + "\n  ]"
                if key == "borgs" else json.dumps(value, sort_keys=True))
        items.append(f"  {json.dumps(key)}: {text}")
    raw = ("{\n" + ",\n".join(items) + "\n}\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **counts}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "catalog", "symbols", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.catalog, args.symbols, args.fixture, args.module, args.output),
                     sort_keys=True))
