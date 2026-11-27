"""Check a scoped original shape sweep and pair decision, exporting metadata only."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.reaction_emit import HIT_BYTES, STEP_SIZE, STEPS, qualified_hit_block
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_attack_transactions import decode_report, sasuke_source
from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage

CASES = ((81, 0, 0, 0), (82, 0, 0, 0), (83, 0, 0, 0), (81, 1, 0, 0), (81, 0, 1, 0), (81, 0, 0, 1))
SWEEP_ROW = re.compile(r"shape-sweep:(?: [0-9]{1,2}){10}")


def source_operands(block):
    attack = sasuke_source(block)
    # The receiver fixture narrows only the body-list inlet, not the
    # descriptor. Check that the original body list contains step 9.
    if len(block) != HIT_BYTES or 9 not in block[:32].split(b"\xff", 1)[0]:
        raise ValueError("Sasuke body list does not contain receiver step 9")
    attack_step = STEPS + 2 * STEP_SIZE
    body_step = 32 + 9 * STEP_SIZE
    flags, _, first, last = struct.unpack_from(">HHhh", block, attack_step + 2)
    attack_sphere = struct.unpack_from(">4f", block, attack_step + 12)
    body_kind, body_matrix, body_flags = struct.unpack_from(">BBH", block, body_step)
    body_sphere = struct.unpack_from(">4f", block, body_step + 12)
    if (flags, first, last, attack_sphere) != (1, 19, 21, (30., -20., -10., 60.)):
        raise ValueError("Sasuke attack sphere or cursor operands changed")
    if (body_kind, body_matrix, body_flags, body_sphere) != (0, 2, 2, (0., -14., 0., 22.)):
        raise ValueError("Sasuke receiver sphere operands changed")
    return {
        "attack": {**attack, "flags": flags, "cursor_bounds": [first, last],
                   "local_center": attack_sphere[:3], "radius": attack_sphere[3]},
        "receiver": {"body_step": 9, "kind": body_kind, "matrix_selector": body_matrix,
                     "flags": body_flags, "local_center": body_sphere[:3], "radius": body_sphere[3]},
    }


def read_sweep(report):
    decode_report(report)  # Both previously qualified allocation paths must still pass.
    rows = []
    for line in report.splitlines():
        if line.startswith("shape-sweep:"):
            if not SWEEP_ROW.fullmatch(line):
                raise ValueError("malformed shape sweep report")
            rows.append(tuple(map(int, line.split()[1:])))
    if tuple(row[:4] for row in rows) != CASES:
        raise ValueError("missing or reordered shape comparisons")
    out = []
    for distance, masked, relation_gate, pair_gate, published, overlap, allowed, classified, accepted, damage in rows:
        expected = int(not masked and distance <= 82 and not relation_gate and not pair_gate)
        if ((published, overlap, allowed) != (1 - masked, int(distance <= 82), 1 - relation_gate)
                or (classified, accepted, damage) != (expected,) * 3):
            raise ValueError("inconsistent shape comparison")
        out.append({
            "distance": distance, "attack_matrix_mask_cleared": bool(masked),
            "receiver_relation_inhibited_after_publication": bool(relation_gate),
            "receiver_pair_gate_set_after_publication": bool(pair_gate),
            "attack_published": bool(published), "broad_overlap": bool(overlap),
            "direct_relation_helper_allows": bool(allowed), "classifier_reached": bool(classified),
            "classifier_accepted": bool(accepted), "damage_amount_entry_reached": bool(damage),
        })
    return out


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("shape sweep output already exists")
    dol = Path(dol).resolve(strict=True)
    DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        block = qualified_hit_block(RetailArchive(stream), 0x000A)
        if stamp != archive_stamp(stream):
            raise ValueError("shape source AFS changed during extraction")
    operands = source_operands(block)
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)),
            "--sasuke-shape-sweep"]
    result = run(argv, input=block, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"shape fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = read_sweep(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-shape-sweep", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "id": "gg4e08:000a", "source": operands,
        "scope": "Sasuke close-B original attack allocation and a singleton same-model body descriptor",
        "entries": {
            "receiver_bind": "8008AB30", "class4_sweep": "80089AEC", "shape_callback": "8008A65C",
            "publish": "8008A2BC", "pair_loop": "8002DB58", "relation": "8002FD7C",
            "broad_phase": "80039F6C", "sphere_classifier": "80030500", "amount_cut": "8003CD5C",
        },
        "shape_sweep": {
            "declared_cursor_values": [18, 19], "at_cursor_18": "only receiver published",
            "at_cursor_19": "attack and receiver published unless attack is masked",
            "attack_center": [130, 180, 290], "receiver_center": ["130 + distance", 180, 290],
            "actor_radius_scale": 1,
            "attack_mask": "actor+0x76C bit 7", "late_receiver_relation_inhibit": "actor+0x272 bit 0x02",
            "late_receiver_pair_gate": "actor+0x1DA bit 0x02 (set refuses before the relation helper)",
        },
        "amount_frontier": {"caller": "8002F108", "attack_record": "802D4190",
                            "carrier": "attack actor", "receiver": "body actor",
                            "receiver_hp_before_and_at_cut": 180},
        "cases": cases, "notes": [
            "The actual attack and receiver allocators publish their bucket entries. No active list or registry entry is supplied.",
            "Pose matrices, slot validity, clock values and the receiver's singleton body list are fixture inlets.",
            "The classifier is verified separately on the original loop's scratch inputs, then the unmodified pair loop is replayed to the amount cut.",
            "The amount calculation, HP write, real pose timing, projectile sweep and whole match remain outside scope.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "amount_entries": sum(row["damage_amount_entry_reached"] for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
