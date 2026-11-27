"""Verify the original visual gate reducer and the next fresh contact."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type32_roster as beam, retail_type9_roster as type9
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, ATLAS_SHA256, SOURCE_SHA256, address, encode, read_metadata
from tools.analyze.retail_rifle_contact_hp import SLICES
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
TAIL_SHA256 = "c3da79a83106c076b9ce91d2a36c344455243a90b2a5c64b0dd4370454716250"
GATE = 0x2AF
PRELUDE_ROW = re.compile(r"impact-prelude:(?: [0-5]){12}")
PAIR_ROW = re.compile(r"impact-clock:(?: [0-4]){5} [0-9]{3} [1-2] [1-2]")
TEXT = (
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8002CC78, 300, "2234d9aa7af59047260c7d72939f52c5ef9b31f10e9e89864abf2ff281132ecc"),
    (0x8002CDA4, 160, "2dcecd916811217d411f21faca6a1ca97774fa3c65c5f3756bf92d230096a693"),
    (0x8002E2A8, *SLICES[0x8002E2A8]),
    *type9.CONTACT_TEXT,
)


def displacement_accesses(image, atlas):
    """Find explicit D-form byte accesses, not indirect aliases or clears."""
    found = []
    for entry, span in atlas["functions"].items():
        start = int(entry, 16)
        for i, (word,) in enumerate(struct.iter_unpack(">I", image.read(start, span["size"], "text"))):
            if word & 0xFFFF == GATE and word >> 26 in (34, 35, 38, 39):
                found.append((start+4*i, word))
    found.sort()
    expected = [(0x8002CE18, 0x888302AF), (0x8002CE28, 0x980302AF),
                (0x8002F858, 0x881E02AF), (0x8002F86C, 0x981E02AF)]
    if found != expected:
        raise ValueError("contact gate explicit byte access census changed")
    return [{"site": address(pc), "kind": "load" if word >> 26 == 34 else "store"} for pc, word in found]


def decode_prelude(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 10:
        raise ValueError("contact gate prelude report extent")
    rows = []
    for i, line in enumerate(lines):
        step, visit = (2 if i < 5 else 1), i % 5 + 1
        gate = max(0, 4-visit)
        if (PRELUDE_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) !=
                (step, visit, gate, 0, 4, gate, gate, 4, 4, gate, gate, gate)):
            raise ValueError(f"contact gate traversal scope or cadence changed: {step}/{visit}")
        rows.append({
            "declared_actor_clock_step": step/2, "visit": visit, "active_actor_gate": gate, "zero_actor_gate": 0,
            "inactive_actor_gate": 4, "base_bucket_object_gate": gate, "registered_child_gate": gate,
            "unregistered_child_gate": 4, "inactive_seat_child_gate": 4,
            "end_bucket_child_gate": gate, "first_view_gate": gate, "last_view_gate": gate,
        })
    return rows


def decode_pairs(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 10:
        raise ValueError("contact gate fresh-pair report extent")
    rows = []
    for i, line in enumerate(lines):
        step, visits = (2 if i < 5 else 1), i % 5
        before = 4-visits
        reopened = before == 0
        if (PAIR_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) !=
                (visits, step, before, 4 if reopened else before, reopened, 146, 1, 2)):
            raise ValueError(f"contact gate fresh token, HP or effect changed: {step}/{visits}")
        rows.append({
            "declared_receiver_clock_step": step/2, "reset_visits_between_hits": visits,
            "first_gate_after_hit": 4, "gate_before_second_hit": before, "gate_after_second_hit": 4 if reopened else before,
            "first_token": 1, "second_token": 2, "hp_loss_each": 17, "receiver_hp_after": 146,
            "same_token_refused_after_natural_reopen": True if reopened else None,
            "second_effect_requested": reopened, "second_effect_allocated": reopened,
        })
    return rows


def fixture_report(fixture, dol, module, options=(), payload=b""):
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), *options]
    proc = run(args, input=payload, capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"contact gate fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    return proc.stdout.decode("ascii")


def export(dol, afs, atlas, prelude_fixture, type9_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact gate output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    uses = displacement_accesses(image, read_metadata(atlas, ATLAS_SHA256))
    for site, target in ((0x8002BBE4, 0x8002CDA4), (0x8002BD84, 0x8002CDA4),
                         (0x8002CCBC, 0x8002CDA4), (0x8002CD30, 0x8002CDA4), (0x8002CD80, 0x8002CDA4),
                         (0x8002C4CC, 0x8002DB58), (0x8002C86C, 0x8005A580), (0x8002F87C, 0x80019550)):
        beam.link(image, site, target)
    tail = read_metadata(MAPS / "gg4e08-contact-effect-tail.json", TAIL_SHA256)
    expected = [r for r in tail["cases"] if r["attack_type"] == 9 and r["case"] == "open-gate"]
    if len(expected) != 1 or (expected[0]["source_record"], expected[0]["hp_lost"], expected[0]["gate_after"]) != (
        "802C4838", 17, 4
    ):
        raise ValueError("contact gate type-9 source contact changed")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        bank = type9.common_bank(archive)
        _, _, _, common, _ = type9.source_index(image, bank)
        body_and_specs, _ = type9.contact_inlet(image, archive)
        if stamp != archive_stamp(stream):
            raise ValueError("contact gate AFS changed during extraction")
    prelude = decode_prelude(fixture_report(prelude_fixture, dol, module))
    pairs = decode_pairs(fixture_report(type9_fixture, dol, module, ("--impact-clock",), bank+body_and_specs))
    data = {
        "kind": "gf-retail-contact-gate-clock", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256, "contact_tail_sha256": TAIL_SHA256,
        "explicit_gate_byte_accesses": uses, "prelude_visits": prelude, "fresh_pair_cases": pairs,
        "entries": {"collision_walk": "8002BB14", "bucket_reset_walk": "8002CC78",
                    "gate_reduce": "8002CDA4", "idle_walk_stop": "8002C86C", "pair_call": "8002C4CC"},
        "common_attack": {"type": 9, "key": 4, "amount_record": common["amount_record"], "raw_power": common["raw_power"],
                          "scope": "standalone effect with declared normal Saucer credit against stationary Sasuke"},
        "notes": [
            "The explicit displacement census does not account for indirect writes or whole-object initialization.",
            "An empty-registry collision walk reaches the actor, bucket and view reset calls. It stops at the later 8005A580 call.",
            "The fresh-pair fixture invokes the same reset leaf on the receiver between hits; it does not invoke the complete collision walk with an admitted pair.",
            "The first shape is explicitly unlinked before the second original effect constructor publishes its own token. Neither attack age nor class-2 lifetime is replayed here.",
            "A held visual gate does not suppress a fresh HP write. Four reset calls reopen the tested gate, but not the retained attack token.",
            "Clock inputs do not establish live frame, pause, or hit-stop scheduling. Native gameplay is unchanged.",
        ],
    }
    raw = encode(data, expanded=("prelude_visits", "fresh_pair_cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "prelude_visits": len(prelude), "fresh_pair_cases": len(pairs)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "prelude_fixture", "type9_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.prelude_fixture,
                            args.type9_fixture, args.module, args.output), sort_keys=True))
