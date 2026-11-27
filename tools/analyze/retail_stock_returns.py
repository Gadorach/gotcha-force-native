"""Index original stock return calls and replay exact owner and Thunder Wire returns."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import reaction_emit
from tools.analyze import retail_stock_lifecycle as lifecycle
from tools.analyze.retail_action_links import ActionLinker, literal_flow
from tools.analyze.retail_projectile_census import ATLAS_SHA256, address, callers
from tools.analyze.retail_type32_roster import word


s = lifecycle.startup
RETURN = 0x8006DE10
WIRE_MODEL = 0x0404
WIRE_HIT_SHA256 = "c0321b8b3c29304a0c26b0f9f5a86d9b269a7fa153727aff152634b75c5b6387"
OWNER = (0x0200, 0x020A, 0x020F, 0x0403)
OWNER_ROW = re.compile(r"stock-owner-return: [0-9A-F]{4}(?: [0-9]){6} 41880000")
WIRE_ROW = re.compile(r"stock-wire-return:(?: [0-9]){5} [0-9A-F]{8}(?: [0-9]){5} 00000000")
WIRE_CASES = ("no-cue", "empty-stock", "carrier-cell-refused", "descriptor-refused", "opening-shape-refused",
              "outside-stage-bounds", "owner-phase-invalid", "owner-model-changed")
TEXT = (
    (0x80073D0C, 112, "3c0a17cae5f10f9d95992499a50affab0abb821ca2873626a28a838ff2a6645f"),
    (0x800DFDA4, 148, "b93bef0c642070f0207ef65ad1ef764e26bbb9603e9564419e6bcb4acc656720"),
    (0x800D8470, 200, "9a14f30f4cbd75024ee7dff70fc4d1568a9df201fa6be5adb21f92332d5b7f62"),
    (0x800D9FBC, 272, "2656a2bba48394a20c447d63f28170d4e1b8b84a8c6874a6caf8fd065147288e"),
    (0x800DA0CC, 144, "d3b3e8dc6c2b543dc4fa93f0725715cffd0c5d773bbf4444f0fea39ce2888e34"),
    (0x800DA15C, 404, "b46428924a4afd0bb424d53595c0634999ca0e685783abd7920fd54e324dafd8"),
    (0x800DA374, 836, "553cb9acb09864ec9e0ad54abe54ddaf0c9bfa223bd44d6a91f622fe5e07f371"),
    (0x80083244, 128, "0d12380007430d1784fc3a5d0183bdd2c9b40b58252868175125d47bbd3e48c4"),
    (0x800E058C, 312, "d1760c991f463afadd23c634578ce59b8a4a3404894b6fc37e83ca143d2cd320"),
    (0x800E075C, 724, "55161c2dab0ed1277642e2f123c8ccb69dd84dc7be712059a2feb2484a32cb7a"),
    (0x800E0DB8, 52, "425fdd6dc72abe7f77b9b981af3c6b976156c65c1f01f2e00a7b9f6a26969d89"),
    (0x800E0E68, 40, "5d0cc6b3acf58489f594ed308c36e6785ba4118e96616314decd7f8097646d65"),
    (0x801304B8, 704, "75d567d05a4ea6ef6537d8d513e3f1f1822187128f2586e81b2cdf4cca442406"),
)


def selector_sites(image, atlas, target):
    linker = ActionLinker(image, {}, ())
    rows = []
    for site, r in sorted(callers(image, atlas, target, 2048).items()):
        start = int(r["function"], 16)
        size = atlas["functions"][r["function"]]["size"]
        words, _, edges, _, _, _ = linker.graph(start, size, 3000)
        selector = literal_flow(start, words, edges, linker.data).get(site, {}).get(4)
        s.link(image, site, target)
        rows.append({"site": r["site"], "function": r["function"], "literal_r4": selector})
    return rows


def source_scope(image, atlas, installed):
    calls = selector_sites(image, atlas, RETURN)
    if Counter(r["literal_r4"] for r in calls) != {0: 17, 2: 36, None: 1}:
        raise ValueError("stock return indexed caller census changed")
    if [r["site"] for r in calls if r["literal_r4"] is None] != ["800E0E7C"]:
        raise ValueError("stock return derived-selector wrapper changed")
    derived = selector_sites(image, atlas, 0x800E058C)
    if [(r["site"], r["literal_r4"]) for r in derived] != [
            ("800BD7BC", 1), ("800E0470", 0), ("80130580", 2), ("801305E8", None)]:
        raise ValueError("type-41 indexed key coverage changed")
    if word(image, 0x801305CC) != 0x38800003:
        raise ValueError("type-41 key-3 paid branch changed")
    raw = image.read(0x80317064, 4*20, "data")
    if sha256(raw).hexdigest() != "b2c99a97c574bae01afbb6a4b251c1b63403220db6ffaf8f4eef689fd5c5f09b":
        raise ValueError("type-41 return selector table changed")
    type41 = []
    for r, key in zip(derived, (1, 0, 2, 3), strict=True):
        channel = struct.unpack_from(">b", raw, key*20+1)[0]
        type41.append({"factory_site": r["site"], "key": key, "row": address(0x80317064+key*20),
                       "signed_return_selector": channel, "return_channel": channel if channel >= 0 else None})
    normal = {r["id"]: r for r in installed["borgs"]}
    for model in OWNER:
        r = normal[f"gg4e08:{model:04x}"]
        if (r["pre_callback"] if model == 0x0403 else r["post_callback"]) != ("800DFDA4" if model == 0x0403 else "80073D0C"):
            raise ValueError("normal owner return callback changed")
    wire = normal["gg4e08:0404"]
    atlas_wire = atlas["profiles"][wire["profile"]]
    x = [r for g in atlas_wire["groups"] if g["group"] == "X" for r in g["rows"]]
    if (not x or {r["code_path"].get("phase_table") for r in x} != {"803109B4"} or
        word(image, 0x803109C0, "data") != 0x800D8470):
        raise ValueError("Thunder Wire is not the exact indexed normal X release")
    raw = image.read(0x80311270, 40, "data")
    if (sha256(raw).hexdigest() != "aa80ecf8fa86f16fbca163a02279b9930beb4653a0a580635183e7205acf08ab" or
        struct.unpack(">4I", image.read(0x80311298, 16, "data")) != (0x800DA15C, 0x800DA2F0, 0x800DA9C4, 0x800DA9D8)):
        raise ValueError("Thunder Wire type-43 row or phases changed")
    return calls, type41


def decode(report):
    lines = report.splitlines()
    if len(report) > 1536 or len(lines) != 21:
        raise ValueError("stock returns report extent")
    owners, wire = [], []
    names = ("at-cap", "depleted-primary-gate", "depleted-open", "depleted-secondary-gate")
    for model in OWNER:
        pre = model == 0x0403
        for i, case in enumerate(names if pre else names[:3]):
            channel, calls = 0 if pre else 2, int(i == 2)
            flags = (2 if pre else 1) if i in (1, 3) else 0
            after = int(i in (0, 2))
            line = lines[len(owners)]
            fields = (f"{model:04X}", *map(str, (i, channel, calls, flags, after, 1)), "41880000")
            if OWNER_ROW.fullmatch(line) is None or tuple(line.split()[1:]) != fields:
                raise ValueError(f"owner stock return changed: {model:04X}/{case}")
            owners.append({"borg": f"gg4e08:{model:04x}", "case": case, "channel": channel,
                           "writer_calls_across_two_visits": calls, "callback_gate_result": flags,
                           "current_after": after, "previous_after": 1, "declared_clock_preserved": 17})
    return_sites = (None, None, "800DA0AC", "800DA0AC", "800DA274", "800DA4CC", None, None)
    for i, case in enumerate(WIRE_CASES):
        made, paid, shape = int(i >= 4), int(i >= 2), int(i >= 5)
        at_release = int(i in (0, 2, 3))
        phase = 3 if i >= 6 else 2 if made else 0
        restored = int(i != 1 and i < 6)
        site = return_sites[i]
        expected = (*map(str, (i, paid, made, at_release, shape)), site or "00000000",
                    *map(str, (phase, made, shape, restored, 1)), "00000000")
        line = lines[len(owners)+i]
        if WIRE_ROW.fullmatch(line) is None or tuple(line.split()[1:]) != expected:
            raise ValueError(f"Thunder Wire stock return changed: {case}")
        wire.append({"case": case, "initial_debit_succeeded": bool(paid), "carrier_allocated": bool(made),
                     "stock_after_release": at_release, "shape_published": bool(shape), "return_site": site,
                     "exit_phase": phase if made else None, "shape_rebuild_visits_to_return": shape,
                     "carrier_and_descriptor_returned": bool(made), "stock_after_case": restored, "clock_after_case": 0})
    return owners, wire


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stock return output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    s.verify_text(image, (*s.TEXT, *s.stock.TEXT, *lifecycle.TEXT, *TEXT))
    for site, target in ((0x800D84B8, 0x8006DBE0), (0x800D84C8, 0x800D9FBC), (0x800DA260, 0x8008AC80),
                         (0x800DA4B8, 0x80083244), (0x800E069C, 0x800E0E68), (0x800E0DD8, 0x800E0E68)):
        s.link(image, site, target)
    s.read_metadata(s.MAPS / "gg4e08-stock-startup.json", lifecycle.STARTUP_SHA256)
    atlas = s.read_metadata(s.MAPS / "gg4e08-code-atlas.json", ATLAS_SHA256)
    installed = s.read_metadata(s.MAPS / "gg4e08-standalone-installers.json", s.INSTALLERS_SHA256)
    calls, type41 = source_scope(image, atlas, installed)
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        plan, _ = s.source_inputs(image, archive, installed)
        hit = reaction_emit.qualified_hit_block(archive, WIRE_MODEL)
        if (sha256(hit).hexdigest() != WIRE_HIT_SHA256 or reaction_emit.read_group(hit, 8) != [(24, 7)] or
            s.archive_stamp(stream) != stamp):
            raise ValueError("Thunder Wire exact hit block or archive input changed")
    owner_rows, wire_rows = decode(s.fixture_report(fixture, dol, module, ("--returns",), hit+plan))
    raw = s.encode({
        "kind": "gf-retail-stock-returns", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "stock_startup_sha256": lifecycle.STARTUP_SHA256, "code_atlas_sha256": ATLAS_SHA256,
        "indexed_return_calls": calls, "type41_derived_selector_scope": type41,
        "actor_callback_cases": owner_rows, "type43_cases": wire_rows,
        "notes": [
            "The call census counts indexed direct sites, not live events. The nonliteral site is the type-41 wrapper; signed minus-one selectors deliberately skip a return.",
            "Type-41 allocation refusal reads its table selector directly; the initializer copies it to carrier +0x158 for the later retirement leaf. Neither path is executed here.",
            "Exact archived normal stocks are loaded by original installer and startup code. Owner callback tests declare a depleted inlet via a separate debit, with a clock sentinel.",
            "Thunder Wire replays the original release with a declared event byte. Stock, object, descriptor, shape, stage-bounds and owner-identity controls are separate.",
            "The one active stage-bounds visit suppresses audible playback via an already-held request. Pose and stage are supplied; the armed tether, contact routing, full scheduling and native gameplay are not qualified.",
        ],
    }, expanded=("indexed_return_calls", "type41_derived_selector_scope", "actor_callback_cases", "type43_cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "indexed_calls": len(calls), "observations": len(owner_rows)+len(wire_rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
