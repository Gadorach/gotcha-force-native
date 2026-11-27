"""Verify original contact visual transforms, view selection and draw-list preparation."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_contact_effect_storage as storage, retail_type32_roster as beam
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import SOURCE_SHA256, encode, read_metadata
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
STORAGE_SHA256 = "3700a3c3d911cb0148992a6ade7734e8b449941143fa1c79dc4e00bd197fb7bd"
ROW = re.compile(r"effect-render:(?: [0-9]{1,5}){8}")
TEXT = (
    (0x800197E8, 60, "013d2ce232138a5dc1bd81cdfe5c429da9eeda9de7dc4f931361c88b0c4c5a0c"),
    (0x80019824, 60, "00e911e9971d71fe5ac1c9de1ee3e757147ac66c40dddeee7cc771bebe15e889"),
    (0x80019CD0, 412, "b9745fc5282d0b83dcb918a946d9bb3966efd44d371c01cf478e54ff41cd1025"),
    (0x8001A7EC, 224, "47d4070808cc18dff0e9c9fa71a716fb4fdb03e2bde864a759496f553c7b41d9"),
    (0x8001A8CC, 132, "3ec2f3282fd94150a008cc4dd37a6383caf8a0aa877f903552c9340e62c2f452"),
    (0x80089554, 260, "14d54d2183c44456974e44418019670cfd857a373e5baced583924c4c74280df"),
    (0x8008A16C, 288, "3bd3b8fe65544e2c29b8fde77b3fe9dfcc8b9a825f5e68ebed6571212c4ab7b0"),
    (0x80089CD0, 88, "d24082a5d5ec519033a1a9fe62ff7a357dc8675dafd2f72a32b5143e8a4938dc"),
    (0x80089C00, 76, "c5a91958209d29c855298aa97009efedfc791d2cbf0435b7bc8a6ee6029851ff"),
    (0x80089D28, 396, "e54aa8c702fe6c4f5d979f078766e3dc158bfe2009b24b036df7c39fb963e45e"),
    (0x80089EB4, 116, "e2c0602394a30c05a5c2239103f8768fb607d7c7c97081317b2818f81f4cc982"),
    (0x80009790, 36, "5e2c9811953cf250a2ed652272b4e2d4b17a4ad57f7b23109f6b416406578e78"),
    (0x800097B4, 132, "1bd33cf4c2b475f43c730caf391868e91a7c00295966fdf0b07073ba07f10366"),
    (0x80009838, 260, "7c260f7265f09dc341256802d431461e462b08c2887d6c18b70a91f8e9c4c156"),
    (0x80029574, 108, "cd194bc3c8db5091c70b477201b345082bec3223f25ef59f1f6a46c78e302024"),
    (0x8008C440, 492, "9c71cc88023ae058335772b480cb136f0261c265ed4a17a3708eedd6288fa6a2"),
)
CASES = ((1, storage.BURST_MASKS), (8, storage.PIECE_MASKS))


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 2*sum(len(masks) for _, masks in CASES):
        raise ValueError("contact visual render report extent")
    i = 0
    cases = []
    for selector, masks in CASES:
        for visit, mask in enumerate(masks, 1):
            for view in (1, 0):
                prepared = mask if selector == 8 or not view else 0
                collected = visit < len(masks)-1
                queued_mask = (int(bool(mask)) if selector == 1 else mask) if not view else 0
                queued = queued_mask.bit_count()
                if (ROW.fullmatch(lines[i]) is None or tuple(map(int, lines[i].split()[1:])) !=
                        (selector, visit, view, mask, prepared, collected, queued, queued_mask)):
                    raise ValueError(f"contact visual preparation changed: {selector}/{visit}/{view}")
                i += 1
                cases.append({
                    "selector": selector, "visit": visit, "view_context_byte": view, "effect_active_mask": mask,
                    "prepared_joint_mask": prepared, "in_view_zero_effect_list": collected,
                    "lane_5_descriptor_mask": queued_mask, "lane_5_descriptor_count": queued,
                    "reached_render_state_frontier": queued != 0,
                })
    return cases


def verify_source(image):
    storage.verify_source(image)
    beam.verify_text(image, TEXT)
    for a, n, digest in (
        (0x802C8194, 32, "d4723f1a9161f3ef83c8e1e3c83bdab450c86c09cb50d19a59243fd8795e304b"),
        (0x802C37C8, 80, "203c2d831844d80ba0db58f948ab51a5d656e03d65b226faa34c92bcc4a8f2be"),
    ):
        if sha256(image.read(a, n, "data")).hexdigest() != digest:
            raise ValueError(f"contact visual render table changed at {a:08X}")
    for pc, target in (
        (0x80029580, 0x8008A16C), (0x800295B4, 0x80089CD0), (0x80089D10, 0x80089C00),
        (0x80089C28, 0x80089D28), (0x80019E54, 0x800097B4), (0x8001A914, 0x800097B4),
        (0x8008C5F4, 0x80009838), (0x8000987C, 0x8025A210), (0x80009914, 0x8024F07C),
    ):
        beam.link(image, pc, target)


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact visual render output already exists")
    dol, fixture, module = (Path(p).resolve(strict=True) for p in (dol, fixture, module))
    verify_source(DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256))
    read_metadata(MAPS / "gg4e08-contact-effect-storage.json", STORAGE_SHA256)
    proc = run([str(fixture), str(dol), str(module), "--render-prep"], capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"contact visual render fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode(proc.stdout.decode("ascii"))
    data = {
        "kind": "gf-retail-contact-effect-render-prep", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "contact_effect_storage_sha256": STORAGE_SHA256,
        "entries": {
            "early_transform_walk": "80089554", "family_transform_dispatch": "800197E8",
            "effect_list_reset": "8008A16C", "bucket_collect": "80089CD0", "view_zero_family_walk": "80089EB4",
            "family_draw_list_dispatch": "80019824", "selector_1_prepare_and_enqueue": "80019CD0",
            "selector_8_prepare": "8001A7EC", "selector_8_enqueue": "8001A8CC",
            "descriptor_enqueue": "800097B4", "draw_heads_reset": "80009790",
            "lane_consumer": "80009838", "light_fog_frontier": "8025A210",
        },
        "view_visits": cases,
        "notes": [
            "The original bucket update and selected preparation functions run directly, not via the whole scene frame.",
            "Both selectors load a declared single-joint model template without geometry or animation; the original model-record IDs are retained.",
            "For selector 1 the matching view callback prepares live child translations and enqueues the one group descriptor.",
            "Selector 8 prepares active piece joints in the early walk. The matching view callback enqueues one descriptor per active piece, in descending index order in the lane list.",
            "View context values 0 and 1 are fixture selections, not a conclusion about live camera configuration.",
            "The phase-2 effect remains in the view-zero list with no model in the draw list; phase 3 removes it from collection.",
            "The replay stops before light/fog setup and does not call joint submission, emit GX commands or change native gameplay.",
        ],
    }
    raw = encode(data, expanded=("view_visits",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "view_visits": len(cases),
            "render_frontiers": sum(r["reached_render_state_frontier"] for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
