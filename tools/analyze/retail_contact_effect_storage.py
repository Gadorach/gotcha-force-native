"""Record original dynamic contact visual allocation, updates, and resource return."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_contact_effect_init as init, retail_type32_roster as beam
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import SOURCE_SHA256, encode, read_metadata
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
INIT_SHA256 = "d4fd0a1f81360c56621734776550cae256f6feb158f8568973c35261354a5c9b"
FRONTIER = re.compile(r"effect-storage-frontier:(?: [0-9]{1,5}){6}")
ROW = re.compile(r"effect-storage-visit:(?: [0-9]{1,5}){8}")
BURST_MASKS = (255,)*5 + (65535,)*11 + (65280, 0, 0, 0)
PIECE_MASKS = (3, 7, 15, 31, 63, 63, 57, 49, 33, 1, 0, 0, 0)
CASES = (
    ("selector-1-two-batches", 1, 0, 0, 928, 16, BURST_MASKS),
    ("selector-1-last-entry", 1, 31, 511, 96, 1, (1,)*16 + (0, 0, 0)),
    ("selector-8-empty-scene-budgets", 8, 0, 0, 288, 0, PIECE_MASKS),
    ("selector-8-full-scene-budgets", 8, 32, 512, 288, 0, PIECE_MASKS),
)
TEXT = (
    (0x80019C3C, 148, "7282367907686ee2961058378638dfdb1f8aa55237c1a33640e3b030c48416bb"),
    (0x80019E6C, 700, "aa01d375f97cd7e45753c7a145fa24a4930ebb519f4e308be9c94c3f8f43688e"),
    (0x8001A128, 352, "5c1d4d9997f1ed4f96c97d7b4d7e55bbc8df952e22f1167b72a2627618eb61c6"),
    (0x8001A7B0, 60, "fd1e74c42bf670326721d7d4426e34a27021aa6d5ce356eaa1d0aca817ac541c"),
    (0x8001AA64, 264, "6155dbea27792789d71cc7b92f7d956b8b0d665372d3110a1de2b43f33af6031"),
    (0x8001A950, 276, "74ec5d8c160365b5e7cf5a1f61111cea961b4d8fee198ea44cadd234cc05cf05"),
    (0x80007198, 212, "c5285fd127a1f512f1c162fa638dbea07019a652b42c8a1e275789d237d94c61"),
    (0x8000726C, 204, "2cd2179ba4d407b0f5e11d39542df2aad38ebee11cbf6f381a459c3c208f3b9f"),
    (0x80197950, 188, "0ec9de608edc53db500d232ccc1673b884437a70824f802eea7941bae3a90b0a"),
    (0x80197AD8, 188, "e3650544ce98694f5c3a5ca208c4dfe1479fbc02f46712a3a8a75d8d210c9082"),
    (0x80006F1C, 68, "640d4a850b849c4ea81a250ae396e2a3467341c1a3f98a894bdf8ee99fd09e56"),
    (0x801FFEC0, 112, "993ea39b1d011af37207ea3565e301047386725515b2ad62f29bc9b933b4b4b5"),
    (0x80253DF0, 420, "bb7a59cc2d0460b416ef512e62e409d6c21bd7d67922554d61ae9ea98828f6b5"),
    (0x8025AE44, 116, "081a21a76a36c591777d241aebd9db01299a1d0d40909a3363812007865b6de4"),
    (0x80200320, 8, "8f22d97bd11da6985b228ec3a70d7a2c4e876764de4a39a105413cb7fd42b810"),
    (0x80200328, 8, "040028ccb49fe14b191cabe131f007ee370a8a5498b6d756bdbc0aa8c003da8e"),
    (0x801FFF30, 108, "723e91351541ac66a577fdf5fbed613dd4a92a113a74448a8110e59fa782fef4"),
    (0x80253F94, 96, "a1cffe0e8a418ca78816a91754beaa663a888d38f15eaf0dce7fd4ff283d336b"),
    (0x801FFD38, 252, "cd33aa3d44eb1ef7b4315d90ea58a969f665eb335d7243e514a1b16e3f17b441"),
    (0x80253FF4, 52, "f3c6d9b4bc779e7bd574aa7fda8b5069ee1f3bed56d226ea3222c98d7b496ca6"),
    (0x801FFE34, 124, "5cd55438a8950b00a9769ebc5a347a75011af2d7523f338ff94c6a30872b2a38"),
    (0x80259644, 48, "0eedd2b4eb1faec10effe5d4e23d406b2cdd5f27407b288242fffa5e9005bcd9"),
)


def verify_source(image):
    init.verify_source(image)
    beam.verify_text(image, TEXT)
    for pc, target in (
        (0x80019B8C, 0x80007198), (0x80019C80, 0x8001A128), (0x8001A7C4, 0x8001AA64),
        (0x8001AA30, 0x8000726C), (0x800197C0, 0x80197AD8), (0x80006F3C, 0x8025AF74),
        (0x80253FBC, 0x801FFD38), (0x80254010, 0x801FFE34),
        (0x80253E98, 0x8025AE44), (0x80253F4C, 0x801FFF30),
    ):
        beam.link(image, pc, target)
    for address, n, digest in (
        (0x8036F60C, 28, "e67a20d1738a0d33b5b9eb90d006f65678e206ebed8c159761b0f9dd348b18b1"),
        (0x802C813C, 24, "3a9564628503bcebfdec55cba48b5f08790d189916deadbc6c98f4e546ee1ac0"),
    ):
        if sha256(image.read(address, n, "data")).hexdigest() != digest:
            raise ValueError(f"contact visual source record changed at {address:08X}")


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 2+sum(len(row[-1]) for row in CASES):
        raise ValueError("contact visual storage report extent")
    i = 0
    results = []
    for scenario, (name, selector, bursts, entries, size, charge, masks) in enumerate(CASES):
        retire = len(masks)
        if charge:
            if (FRONTIER.fullmatch(lines[i]) is None or tuple(map(int, lines[i].split()[1:])) !=
                    (scenario, size, bursts+1, entries+charge, 2, charge)):
                raise ValueError(f"contact visual model frontier changed: {name}")
            i += 1
        for visit, mask in enumerate(masks, 1):
            phase = 1 if visit < retire-2 else 2 if visit == retire-2 else 3
            debit = bool(charge) and phase == 1
            expected = (scenario, visit, phase, mask, phase != 3, bursts+debit, entries+charge*debit,
                        size if visit < retire else 0)
            if ROW.fullmatch(lines[i]) is None or tuple(map(int, lines[i].split()[1:])) != expected:
                raise ValueError(f"contact visual lifetime changed: {name}/{visit}")
            i += 1
        results.append({
            "case": name, "selector": selector, "scene_bursts_before": bursts, "scene_entries_before": entries,
            "heap_payload_bytes": size-32, "heap_block_bytes": size, "scene_burst_debit": int(bool(charge)),
            "scene_entry_debit": charge, "scene_budget_return_visit": retire-2 if charge else None,
            "selector_1_model_record": 2 if charge else None, "selector_1_joint_children": charge if charge else None,
            "active_masks_from_visit_1": list(masks),
            "phase_2_visit": retire-2, "visual_enable_cleared_visit": retire-1,
            "heap_type3_class2_and_descriptors_return_visit": retire,
            "descriptors_returned": 1 if selector == 1 else 8,
        })
    return results


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact visual storage output already exists")
    dol, fixture, module = (Path(p).resolve(strict=True) for p in (dol, fixture, module))
    verify_source(DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256))
    read_metadata(MAPS / "gg4e08-contact-effect-init.json", INIT_SHA256)
    proc = run([str(fixture), str(dol), str(module)], capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"contact visual storage fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode(proc.stdout.decode("ascii"))
    data = {
        "kind": "gf-retail-contact-effect-storage", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "contact_effect_init_sha256": INIT_SHA256,
        "entries": {"os_heap_table_init": "801FFEC0", "heap_dispatch_init": "80253DF0", "os_alloc": "801FFD38",
                    "os_free": "801FFE34", "type3_source_pool_row": "8036F60C", "type3_destructor": "80006F1C",
                    "burst_group_prepare": "80007198", "burst_update": "80019C3C", "pieces_update": "8001A7B0"},
        "cases": cases,
        "notes": [
            "The guest RAM arena and a one-cell type-3 row are declared; the original allocator writes the heap table, dispatch and return value.",
            "The helper is invoked directly; the replay does not run a new accepted pair or retail boot.",
            "Selector 1 uses a declared single-joint template, no geometry or animation, and the original ID-pool initializer.",
            "Selector 8 uses empty model records 53 and 54. Its mask and lifetime do not establish geometry, drawing, or retail lifetime in a loaded match.",
            "Only the fixed-bucket update is repeated. Neither route publishes a collider in these visits.",
            "The returned type-3 heap block is separate from reservations made by HSD object pools; the selector-1 arena need not return to its original free-list extent.",
            "Native gameplay is unchanged.",
        ],
    }
    raw = encode(data, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "visits": sum(len(row["active_masks_from_visit_1"]) for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
