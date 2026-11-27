"""Verify original type-15 requests, partial allocation, and debit ownership."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_small_projectile_roster import CHARGED_BOMBS, NINJA_BOMBS
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import levels
from tools.extract.source_image import DolImage

SLICES = {
    0x80166FA8: (600, "30f6972ada4411da7d2c0bb74570a9f89eb2b0508ed2aa007f334d18f00ca6f1"),
    0x800715F8: (224, "7735b73fbd6b3149290d56bad8eed6be6202d6afbcb315708646b0717bbda4a8"),
    0x801A0458: (152, "2ee1e0ea72c2777df944ed787a8754bd49c0d6ce9ecb70c762baf21901c82a7c"),
    0x8007DB5C: (400, "d759aaf2c9b38c2924ac451e7b29c4d67ef37c2d01492fc5654f04821eeaa0dc"),
    0x8006DBE0: (224, "0b2e322f452d317b5f515e0ebd95f821a10cdbee5db9cdbd07678c8d251eed31"),
    0x80088D98: (184, "a134e40d302eb33d6ebc1c3dd371228809562bdc533d833bebf095b19180ab44"),
    0x80197A0C: (204, "96222c5bda44dae9f8bb740618f6f8c5d08d455044410156499752c80db51a82"),
}
BOUNDARIES = ("leaf-return", "before-charged-sound", "before-ordinary-X-request", "before-movement-tail")
# Label, owner model, state, extra 0x7C8 byte, stock, carrier cells, descriptor cells.
CASES = (
    ("charged-full", 0x0101, 3, 0, 1, 4, 4), ("charged-no-current-stock", 0x0101, 3, 0, 0, 4, 4),
    ("charged-carrier-short", 0x0101, 3, 0, 1, 2, 4), ("charged-descriptor-short", 0x0101, 3, 0, 1, 4, 2),
    ("charged-no-carriers", 0x0101, 3, 0, 1, 0, 4), ("charged-no-descriptors", 0x0101, 3, 0, 1, 4, 0),
    ("ordinary-other-request", 0x0101, 2, 0, 1, 4, 4), ("ordinary-debit-refused", 0x0101, 2, 0, 0, 4, 4),
    ("ninja-paid", 0x0000, 2, 0, 1, 1, 1), ("ninja-debit-refused", 0x0000, 2, 0, 0, 1, 1),
    ("double-extra-clear", 0x0003, 2, 0, 1, 2, 2), ("double-extra-short", 0x0003, 2, 1, 1, 2, 1),
    ("double-extra-full", 0x0003, 2, 1, 1, 2, 2), ("sasuke-no-carrier-last-round", 0x000A, 2, 0, 1, 0, 1),
    ("sasuke-paid", 0x000A, 2, 0, 2, 1, 1), ("alpha-II-paid", 0x000C, 2, 0, 2, 1, 1),
)
ROW = re.compile(r"bomb:(?: [0-9]{1,4}){13}")
WRONG_PARENT = "bomb-wrong-parent: 10 8 0 1 1"


def encoded(keys):
    return sum(key*10**i for i, key in enumerate(reversed(keys)))


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(CASES)+1 or lines[-1] != WRONG_PARENT:
        raise ValueError("bomb release report extent or parent control mismatch")
    result = []
    for line, (label, model, state, extra, stock, carriers, descriptors) in zip(lines[:-1], CASES, strict=True):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed bomb release row")
        fields = tuple(map(int, line.split()[1:]))
        gunman = model == 0x0101
        charged = gunman and state == 3
        paid = bool(stock and not charged)
        if charged:
            keys = [key for _, key in CHARGED_BOMBS]
        elif paid and not gunman:
            keys = [key for _, key in NINJA_BOMBS[model]][:1+extra]
        else:
            keys = []
        accepted = keys[:min(carriers, descriptors)]
        remaining = stock-int(paid)
        clock = (120 if gunman else 300) if paid and not remaining else 0
        boundary = 1 if charged else 2 if gunman and paid else 3 if gunman else 0
        if fields != (model, state, extra, stock, carriers, descriptors, len(keys), encoded(keys), len(accepted),
                      encoded(accepted), remaining, clock, boundary):
            raise ValueError(f"incorrect original bomb transaction: {label}")
        result.append({
            "case": label, "borg": f"gg4e08:{model:04x}", "declared_actor_state": state, "declared_extra_byte": extra,
            "stock_before": stock, "stock_after": remaining, "refill_clock": clock,
            "declared_free_carriers": carriers, "declared_free_descriptors": descriptors,
            "factory_requests": keys, "published_in_request_order": accepted, "list_head_to_tail": accepted[::-1],
            "remaining_free_carriers": carriers-len(accepted), "remaining_free_descriptors": descriptors-len(accepted),
            "boundary": BOUNDARIES[boundary],
        })
    return result


def source_operands(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb release source signature mismatch at {address(entry)}")
    records = []
    for model, calls in (*NINJA_BOMBS.items(), (0x0101, CHARGED_BOMBS)):
        channel = levels(image, model)[0][5:]
        expected = (1, 1, 0, 120) if model == 0x0101 else (5, 5, 0, 300)
        if tuple(channel) != expected:
            raise ValueError(f"unexpected channel-2 stock for {model:04X}")
        for _, key in calls:
            row = 0x802D6D68+56*key
            raw = image.read(row, 56, "data")
            required, = struct.unpack_from(">H", raw, 46)
            if required != model or raw[2] != (7 if key in (6, 8) else 11) or bool(raw[44]) != (key == 2):
                raise ValueError(f"unexpected bomb parent or muzzle operand for key {key}")
        records.append({"borg": f"gg4e08:{model:04x}", "channel": 2, "cap": channel[1],
                        "mode": channel[2], "preset": channel[3]})
    return records


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb release output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    stock = source_operands(image)
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb release fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-bomb-release", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "factory": "8007DB5C", "class": 3, "bucket": 9,
        "entries": {"powered_gunman_state_split": "8016711C", "ordinary_ninja_paid_leaf": "800715F8",
                    "alpha_II_paid_leaf": "801A0458", "stock_debit": "8006DBE0"},
        "stock_operands": stock,
        "wrong_parent": {"declared_parent": "gg4e08:000a", "requested_key": 8, "published": False,
                         "remaining_free_carriers": 1, "remaining_free_descriptors": 1},
        "cases": cases,
        "notes": [
            "The Powered Gunman slice begins after the cue and movement producers with state supplied, not a played charged-X action.",
            "Allocation attempts, prepended list links and exact socket samples come from the original factory on declared free lists and matrices.",
            "Each successful bomb pops one carrier and one descriptor. Refusal does not retry or undo an earlier debit.",
            "No active-list callbacks, shape bind, audio playback, bomb impact, descendant effect or battle timing is qualified.",
            "The ordinary branch debit in this slice does not establish when the separate charged path pays.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "parent_controls": 1}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
