"""Export the authored Battle Test snapshot of split attacker/receiver factors."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

from tools.analyze import damage_amount_model as amount, damage_content_row as rows
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

SETTINGS = 7
RANK = 0
CLASS_COUNT = rows.CLASS_COUNT
MATCHUP_COUNT = 20


def matchup_group(index):
    return f"gg4e08-class-{index}"


def _matchups(image, factor):
    for address, size, digest in (
        (0x80066298, 40, "6da7baea31ab7cd3e4bf99b01697fd7faeb6e3b4e6938f90a3174a353038b084"),
        (0x8003CFB8, 112, "5ad2692bb62d9710c7ae49bb07a567aada6df10c7d5581cb4082d6203a80729f"),
        (0x8003CDBC, 124, "3acdca46359fe09be09077561ec57db3a707e2a9b3db4154f4bce0c6c4489c00"),
    ):
        if hashlib.sha256(image.read(address, size, "text")).hexdigest() != digest:
            raise ValueError("contact matchup mapper or application changed")
    matrix = {}
    for receiver in range(MATCHUP_COUNT):
        incoming = {}
        for attacker in range(MATCHUP_COUNT):
            value = factor(amount.MATCHUP_MATRIX + 4 * (receiver * MATCHUP_COUNT + attacker))
            if value != 1:
                incoming[matchup_group(attacker)] = value
        matrix[matchup_group(receiver)] = incoming
    return {"mapper": "80066298", "model_index_root": f"{amount.MODEL_INDEX_TABLE:08X}",
            "matrix": f"{amount.MATCHUP_MATRIX:08X}", "multiply": "8003D024",
            "dimensions": [MATCHUP_COUNT, MATCHUP_COUNT], "order": ["receiver", "attacker"],
            "neutral": 1.0, "nonneutral_rows": matrix,
            "source_gate": "ordinary live linked attacker and live body receiver (phase 1)"}


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if hashlib.sha256(image.read(0x80006ABC, 0xB8, "text")).hexdigest() != (
            "9e588ad6c753b97f29c29a48491f2ec614c07e2bad627e223c1e67c412dd3ba1"):
        raise ValueError("contact setting-7 writer changed")
    roots = (rows.CREDIT_ROOT, rows.ATTACKER_RANK_ROOT, rows.CLASS_ROOT, rows.RECEIVER_RANK_ROOT)
    pairs = [[rows._row(image, root, side) for side in (0, 1)] for root in roots]
    if [right - left for left, right in pairs] != [128, 32, 32, 32]:
        raise ValueError("contact factor row layout changed")

    def factor(at):
        value = rows._f32(image, at)
        if not math.isfinite(value) or not .01 <= value <= 4:
            raise ValueError("contact role factor outside native domain")
        return round(value, 6)

    sides = []
    for side in (0, 1):
        credit, offense, class_table, defense = (pair[side] for pair in pairs)
        classes = [rows._u32(image, class_table + i * 4) for i in range(CLASS_COUNT)]
        if classes != list(range(classes[0], classes[0] + 128 * CLASS_COUNT, 128)):
            raise ValueError("contact class sheet layout changed")
        sides.append({
            "team": side, "credit_difficulty": factor(credit + 4 * SETTINGS),
            "attacker_rank": factor(offense + 4 * RANK),
            "receiver_rank": factor(defense + 4 * RANK),
            "receive_class": [factor(address + 4 * SETTINGS) for address in classes],
            "class_rows": [f"{address:08X}" for address in classes],
        })
    return {"kind": "gf-damage-contact-roles", "revision": 2,
            "source_dol_sha256": SOURCE_SHA256, "settings": SETTINGS, "rank": RANK,
            "settings_writer": "80006ABC", "settings_state": 2,
            "table_roots": [f"{root:08X}" for root in roots], "sides": sides,
            "matchup": _matchups(image, factor),
            "scope": "authored snapshot for opted-in direct native move hits; no live source battle-mode selection"}


def profile_rows(report, selectors, model, actor, *, image=None):
    """Emit receiver factors only for an exact single-record normal Borg."""
    lines = ["# Authored contact-scale test: settings 7 for both teams, rank 0; not live battle settings."]
    for side in report["sides"]:
        lines.append(f"damage-credit={side['team']}|{rows._number(side['credit_difficulty'])}|"
                     f"{rows._number(side['attacker_rank'])}")
    selector = selectors["models"].get(f"{model:04x}")
    bound = len(actor) == 432 and selector is not None
    matchup = None
    if bound:
        if list(actor[156:159]) != selector or not 0 <= selector[0] < CLASS_COUNT:
            raise ValueError(f"{model:04x}: normal contact class selector mismatch")
        for side in report["sides"]:
            lines.append(f"damage-receive={side['team']}|{rows._number(side['receive_class'][selector[0]])}|"
                         f"{rows._number(side['receiver_rank'])}")
        # Only the verified single normal template is stable enough to
        # resolve its source model identity once, at extraction time.
        if image is not None and 0 <= model < 0x1000:
            index = amount.model_index(image, model)
            if 0 <= index < MATCHUP_COUNT:
                matchup = matchup_group(index)
                incoming = report["matchup"]["nonneutral_rows"][matchup]
                lines.append(f"damage-matchup-group={matchup}")
                lines.extend(f"damage-matchup-from={key}|{rows._number(value)}"
                             for key, value in sorted(incoming.items()))
    return {"settings": SETTINGS, "rank": RANK, "lines": lines,
            "receiver_selector": selector if bound else None,
            "matchup_group": matchup,
            "receiver_status": "exact normal selector" if bound else "no qualified single normal actor record"}


def normal_profile_rows(report, evidence, model, installer, actor_records, *, image):
    """Use the observed standalone record; never resolve a combined component."""
    key = f"gg4e08:{model:04x}"
    proof = evidence.get(key)
    if not proof or proof.get("installer") != f"{installer:08X}":
        raise ValueError(f"{key}: missing exact normal receiver installer")
    record = proof.get("spec_record")
    count, rest = divmod(len(actor_records), 432)
    if (rest or not 1 <= count <= 8 or type(record) is not int or
            not 0 <= record < count):
        raise ValueError(f"{key}: normal receiver record outside actor block")
    offset = 432*record
    data = actor_records[offset:offset+432]
    triple = list(data[156:159])
    digest = hashlib.sha256(data).hexdigest()
    gauge = struct.unpack_from(">h", data)[0]
    if (digest != proof.get("record_sha256") or triple != proof.get("selectors") or
            any(value >= CLASS_COUNT for value in triple) or
            not 1 <= gauge <= 32767 or gauge != proof.get("reset_capacity")):
        raise ValueError(f"{key}: normal receiver record does not match observed data")
    result = profile_rows(report, {"models": {f"{model:04x}": triple}}, model, data, image=image)
    result["receiver_status"] = "exact observed normal actor record"
    result["receiver_record"] = {"spec_record": record, "record_sha256": digest, "reset_capacity": gauge}
    return result


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact roles output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        report = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(report, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.output)
