"""Inventory of model render features that the portable renderer does not bind.

The point is to turn "some Borgs look untextured" into a list of signatures.
Each packaged `visual/body.hsd` is parsed with the shared HSD analyser and its
fingerprint is aggregated:

* `mobj_render_mode_unknown_bits` — render-mode bits the analyser does not
  recognise. These are the first suspects for a wrong or missing draw.
* `textures` / `tluts` — the texture formats actually present.
* `vtx_attrs` — the vertex attribute combinations actually present.
* `pobj_obj_types` — skin versus envelope geometry.

The output is metadata only: counts and format names, never pixels. Reading is
bounded to the packaged `body.hsd` files and never writes to the roster.
"""
import argparse
import json
from collections import Counter
from pathlib import Path

from tools.analyze import hsd_scene


def aggregate(entries):
    """Fold per-model fingerprints into roster-wide counters."""
    bits = {}
    formats = Counter()
    tluts = Counter()
    attrs = Counter()
    types = Counter()
    affected = []
    for model, fingerprint in entries:
        unknown = fingerprint.get("mobj_render_mode_unknown_bits") or {}
        if unknown:
            affected.append({"model": model, "unknown_bits": dict(sorted(unknown.items()))})
            for bit, count in unknown.items():
                slot = bits.setdefault(bit, {"mobjs": 0, "models": 0})
                slot["mobjs"] += count
                slot["models"] += 1
        formats.update(fingerprint.get("textures") or {})
        tluts.update(fingerprint.get("tluts") or {})
        attrs.update(fingerprint.get("vtx_attrs") or {})
        types.update(fingerprint.get("pobj_obj_types") or {})
    return {
        "kind": "model render-feature inventory; counts and format names only, no pixel data",
        "models": len(entries),
        "models_with_unknown_render_bits": len(affected),
        "unknown_render_bits": dict(sorted(bits.items())),
        "texture_formats": dict(formats.most_common()),
        "tlut_formats": dict(tluts.most_common()),
        "vertex_attributes": dict(attrs.most_common()),
        "pobj_types": dict(types.most_common()),
        "per_model": affected,
    }


def collect(roster):
    roster = Path(roster)
    entries = []
    for folder in sorted(path for path in roster.iterdir() if path.is_dir()):
        model = folder / "visual" / "body.hsd"
        if not model.exists():
            continue
        report = hsd_scene.analyse(model.read_bytes(), folder.name)
        scene = report.get("scene") or {}
        fingerprint = scene.get("fingerprint") or {}
        if fingerprint:
            entries.append((folder.name, fingerprint))
    if not entries:
        raise ValueError("inventory needs packaged body.hsd models")
    return aggregate(entries)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("roster", type=Path, help="exported roster directory")
    parser.add_argument("output", type=Path, help="inventory JSON to create")
    args = parser.parse_args(argv)
    if args.output.exists():
        raise SystemExit("refusing to overwrite the inventory")
    data = collect(args.roster)
    args.output.write_text(json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    print(f"{data['models']} models, {data['models_with_unknown_render_bits']} with unknown render bits")


if __name__ == "__main__":
    main()
