"""Recover ordinary direct-contact visual descriptors, not retail emitter execution."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import contact_sound, retail_dragon_art
from tools.analyze.retail_movement_operands import ACTOR_BYTES
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

DESCRIPTORS = 0x802C7ED0
FAMILIES = 0x802C8154
REVIEWED = (1, 2, 3, 4, 8)
TEXT = (
    (0x8002F850, 48, "6aec693abade4f28b13baf5987166ffacc2391908cacf9acda76a1d4b8618363"),
    (0x80019550, 76, "ba4e00a2bbd856928326f87a46b029a0fb26319ac6de39f8a776a1199df467f1"),
    (0x8001959C, 308, "8c9a81ff9a8f4589b8b3cc525efd66e18b7f53f6c4829778ccd2cf6b9f43b2cc"),
    (0x80019AEC, 336, "437d2c16fae1fc5cc426cc37e48f0cce52dc1c78db20bfb982df777f32f9bfac"),
    (0x8001A344, 96, "d507653e7b3a98246a20d6e42435b29660b6fc498af6189e6d76146158231749"),
    (0x8001A550, 456, "e6fbcd8e8ccab30b79c7ba2c509095ce1f7b5da97fa40ddad4134fc3c4e7a4eb"),
    (0x8001A730, 100, "44b32f8cee0a60750fa10a38f2321f4d04919c06e628cf58e912c79826d5fe3c"),
    (0x8001A950, 532, "ba38acf823472b62ac109fb890064397b670bcd9abc73cd5d3546c2f7d06b633"),
    (0x80007198, 212, "c5285fd127a1f512f1c162fa638dbea07019a652b42c8a1e275789d237d94c61"),
)


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    for site, size, digest in TEXT:
        if sha256(image.read(site, size, "text")).hexdigest() != digest:
            raise ValueError(f"contact visual binding changed: {site:08X}")
    retail_dragon_art.art_source_matches(image)  # shared effect archive and scene ownership
    families = struct.unpack(">4I", image.read(FAMILIES, 16, "data"))
    if families != (0x802C7F54, 0x802C7F60, 0x802C8098, 0x802C813C):
        raise ValueError("contact visual family pointers changed")
    raw = list(struct.iter_unpack(">4B", image.read(DESCRIPTORS, 9 * 4, "data")))
    descriptors = {str(i): {"family": family, "variant": variant, "allocation_code": code,
                            "reviewed": i in REVIEWED}
                   for i, (_, family, variant, code) in enumerate(raw)}
    templates = {}
    for family, count in ((1, 2), (2, 3), (3, 2)):
        rows = list(struct.iter_unpack(">hhII", image.read(families[family], count * 12, "data")))
        if any(not 0 < rec < 256 or not 0 < span <= 120 for rec, span, _, _ in rows):
            raise ValueError("contact visual template extent")
        templates[str(family)] = [{"record": rec, "halfword_02": ticks, "curves": [f"{a:08X}", f"{b:08X}"]}
                                  for rec, ticks, a, b in rows]
    if ([r["record"] for r in templates["1"]], [r["record"] for r in templates["2"]],
            [r["record"] for r in templates["3"]]) != ([2, 3], [144, 145, 146], [53, 54]):
        raise ValueError("contact visual common records changed")
    if any((descriptors[str(i)]["family"], descriptors[str(i)]["allocation_code"]) != (
            (1, 1) if i == 1 else (3, 8) if i == 8 else (2, 3)) for i in REVIEWED):
        raise ValueError("contact visual selector mapping changed")
    return {"kind": "gf-contact-impact-visuals", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "source": {"selector_offset": 9, "ordinary_call": "8002F87C", "selector_table": f"{DESCRIPTORS:08X}",
                       "family_templates": f"{FAMILIES:08X}", "visual_gate_offset": 687,
                       "descriptor_payload": ["family", "variant", "allocation code"],
                       "source_family_1": "bind source slot record at 80019B8C, clone JObjs, spawn in two randomized groups",
                       "source_family_2": "records 144..146 are sublayers (not alternative records per selector); "
                                          "8001A628 binds each, lifetime at template+2",
                       "source_family_3": "records 53 and 54 bind at 8001AA30; second sublayer can start later",
                       "normal_archive": {"member": retail_dragon_art.EFFECT_MEMBER, "slot": 1,
                                          "sha256": retail_dragon_art.EFFECT_BLOCKS[1][1]}},
            "descriptors": descriptors, "templates": templates,
            "native": {"scope": "ordinary direct contacts with single normal receiver",
                       "scale_percent": 100, "family_1_ticks": 16,
                       "clock": "one representative family-1 particle by attacking team; families 2/3 layered immediately "
                                "for template+2 native ticks; static source geometry, authored placement and scale"}}


def qualify_archive(report, archive):
    if sha256(archive).hexdigest() != report["source"]["normal_archive"]["sha256"]:
        raise ValueError("ordinary contact visual archive identity")
    keys = {row["record"] for family in report["templates"].values() for row in family}
    arts = retail_dragon_art.record_receipts(archive, keys)
    if any(row["empty_static"] for row in arts["records"].values()):
        raise ValueError("empty ordinary contact visual record")
    return arts


def layers(report, selector, team):
    if selector not in REVIEWED or team not in (0, 1):
        raise ValueError("contact visual selector or team outside reviewed scope")
    family = report["descriptors"][str(selector)]["family"]
    rows = report["templates"][str(family)]
    if family == 1:
        return [(rows[team]["record"], report["native"]["family_1_ticks"])]
    return [(row["record"], row["halfword_02"]) for row in rows]


def profile_rows(report, image, bindings, binding, actor_size, *, actor_retirement_art=False):
    receiver = actor_size == ACTOR_BYTES and binding and (
        binding["status"] == "one-static-slot-0-record" and binding["selected_record"] == 0)
    lines = ["contact-impact-receive=normal"] if receiver else []
    moves = {}
    for name, route in sorted(bindings.items()):
        rows = contact_sound.contact_records(image, route)
        selectors = {row["visual"] for row in rows}
        refusal = contact_sound.ordinary_refusal(rows)
        if not refusal and (len(selectors) != 1 or not selectors <= set(REVIEWED)):
            refusal = "source selectors disagree" if len(selectors) != 1 else "unsupported visual selector"
        if not refusal and actor_retirement_art:
            refusal = "package retains authored actor-retirement art"
        row = {"table": route["table"], "entries": rows, "status": refusal or "qualified ordinary contact visual"}
        if not refusal:
            selector, = selectors
            row["selector"] = selector
            row["teams"] = []
            for team in (0, 1):
                timing = layers(report, selector, team)
                row["teams"].append([{"record": rec, "ticks": ticks} for rec, ticks in timing])
                for record, ticks in timing:
                    lines.append(f"contact-impact={name}|{team}|{record}|{report['native']['scale_percent']}|{ticks}")
        moves[name] = row
    return {"receiver": bool(receiver), "moves": moves, "lines": lines,
            "requires_records": any("selector" in move for move in moves.values())}


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact visual output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        result = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.output)
