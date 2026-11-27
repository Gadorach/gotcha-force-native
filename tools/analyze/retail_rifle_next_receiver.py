"""Verify distinct recipients and token reuse for the same original rifle carrier."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_rifle_contact_hp import (
    AFS_SHA256, PUBLISHED, RELEASE, SOURCE_SHA256, load_contact_inputs,
)
from tools.analyze.retail_rifle_lifetime import HIT, SLICES
from tools.extract.source_image import DolImage

CASES = (("second-body-miss", 256, 0), ("second-body-with-token", 0, 1), ("fresh-second-body", 0, 0))
ROW = re.compile(r"rifle-next:(?: [0-9]{1,5}){14}(?: [0-9]{1,4}\.[0-9]{5}){2}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 4 * len(CASES):
        raise ValueError("rifle next receiver report extent")
    results = []
    for i, (name, distance, duplicate) in enumerate(CASES):
        release, publish, first, line = lines[4*i:4*i+4]
        if (release, publish, first) != (RELEASE, PUBLISHED, HIT) or ROW.fullmatch(line) is None:
            raise ValueError(f"rifle next-receiver prefix disagrees: {name}")
        fields = line.split()[1:]
        d, dup, token, writer, amount, hp1, hp2, prev, cursor, credit, took1, took2, stock, timer = map(
            int, fields[:14])
        next_z, pair_z = map(float, fields[14:])
        hit = not distance and not duplicate
        value = 8 if hit else 0
        if (d, dup, token, writer, amount, hp1, hp2, prev, cursor, credit, took1, took2, stock, timer,
                next_z, pair_z) != (
            distance, duplicate, 1, int(hit), value, 172, 180-value, 180 if hit else 65535, int(hit),
            8+value, 8, value, 1, 299, 445, 423 if hit else 445,
        ):
            raise ValueError(f"rifle next-receiver outcome disagrees: {name}")
        results.append({
            "case": name, "second_body_extra_z": d, "second_body_seeded_with_token": bool(dup),
            "same_carrier_token": token, "second_hp_writer_reached": bool(writer), "second_amount": amount,
            "first_hp": hp1, "second_hp": hp2, "second_previous_hp_field": prev, "second_token_cursor": cursor,
            "credit_ledger": credit, "first_receive_ledger": took1, "second_receive_ledger": took2,
            "stock": stock, "timer": timer, "carrier_z_after_flight": next_z, "carrier_z_at_amount_cut_or_return": pair_z,
        })
    return results


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("rifle second receiver output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"rifle next receiver source signature mismatch at {address(entry)}")
    inlet = b"".join(load_contact_inputs(afs))
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)),
            "--rifle-next-receiver"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"rifle next receiver fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    results = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-rifle-next-receiver", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "credit": {"borg": "gg4e08:0600", "carrier_key": 21, "amount_record": "8030A398"},
        "declared": {"actor_slots": [0, 1, 2], "teams": [0, 1, 1], "receiver_borg": "gg4e08:000a",
                     "receiver_body_descriptor": 9, "starting_hp_per_receiver": 180, "starting_rifle_stock": 2},
        "first_receiver_control": {"same_geometry_and_token_reaches_amount": False,
                                   "only_history_word_cleared_reaches_amount": True,
                                   "control_writes_hp": False},
        "cases": results,
        "notes": [
            "The first contact is the original paid rifle path and HP writer. Its token and credited damage are retained.",
            "The history counterfactual is discarded. A second original body is then bound on a separate slot, using the same normal body descriptor.",
            "An isolated flight/late/reset/shape visit publishes the same attack and both singleton receivers, before another original pair pass.",
            "Original accepted contact changes the carrier position before the amount cut. The HP writer is run without resuming its caller.",
            "No simultaneous pair iteration, moving-body sweep, later reaction scheduling, native proxy rule, or live timing is qualified.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(results),
            "second_hits": sum(row["second_hp_writer_reached"] for row in results)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
