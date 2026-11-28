"""Bind only the proven point-accounting edges to normal authored Sasuke."""
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256


MODEL = 0x000A
MEMBER_SHA256 = "d58b8ca4bf80cb58543d32392af48d8f25fd797fa96065df0904ddbe5200d084"
EVIDENCE = "docs/api-map/gg4e08-sasuke-air-points.json"
EVIDENCE_SHA256 = "e347cab0866c4cf5ad41fcfd0a71d731ef79d040e14d5160e31920d4614b6e1d"
OLD = b"air-action-budget=3"
POINT_ROWS = (b"air-jump-points=3", b"air-dash-point=seed-if-zero", b"air-point-reset=next-grounded-update")


def qualify(image, path=None):
    raw = read_bounded(path if path is not None else Path(__file__).resolve().parents[2] / EVIDENCE, 32 << 10)
    if sha256(raw).hexdigest() != EVIDENCE_SHA256 or image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified Sasuke air-point evidence or source")
    proof = json.loads(raw)
    if (proof["kind"], proof["revision"], proof["source_dol_sha256"], proof["borg"],
            proof["source_normal_member_sha256"], proof["air_point_limit"]) != (
            "gf-retail-sasuke-air-points", 1, SOURCE_SHA256, "gg4e08:000a", MEMBER_SHA256, 3):
        raise ValueError("Sasuke air-point evidence changed its identity")
    for at, op in ((0x8006137C, "887F07477C6007754082000C38030001981F0747"),
                   (0x80055DE0, "800305E05400D7FE98030087"), (0x80055A9C, "38000000981F0747")):
        expected = bytes.fromhex(op)
        if image.read(at, len(expected), "text") != expected:
            raise ValueError("Sasuke exact seed or reset store changed")
    edges = proof["original_edges"]
    if ([(r["used_at_request"], r["accepted_with_flick"], r["used_after_windup_entry"])
          for r in edges["dash_gates_and_entries"]] != [(0, True, 1), (1, True, 1), (2, True, 2), (3, True, 3)]
            or [(r["at_landing_entry"], r["at_next_grounded_resource_visit"])
                for r in edges["landing_and_reset"]] != [(1, 0), (3, 0)]):
        raise ValueError("Sasuke source point edges changed")
    return {
        "scope": "normal Sasuke authored movement, separate point count only",
        "source_evidence": EVIDENCE, "source_evidence_sha256": EVIDENCE_SHA256,
        "point_limit": 3, "dash_policy": "seed-if-zero", "reset_policy": "next-grounded-update",
        "retail_behavior_verified": False,
    }


def bind(model, digest, gameplay, receipt, proof):
    if model != MODEL:
        return gameplay
    if (digest != MEMBER_SHA256 or not receipt or receipt.get("source_level") != 1
            or receipt.get("retail_behavior_verified") is not False
            or receipt.get("air_action_budget") != {"operand": "09F", "value": 3,
                                                     "source": "combined jump and air-dash cap",
                                                     "retail_behaviour_verified": False}):
        raise ValueError("Sasuke air-point opt-in needs the exact normal authored count and member")
    lines = gameplay.splitlines()
    if (lines.count(b"variant=normal") != 1 or lines.count(b"body-record=0") != 1 or lines.count(OLD) != 1
            or OLD+b"\n" not in gameplay
            or not any(line.startswith(b"movement=grounded-v1|") for line in lines)
            or any(line.startswith((b"air-jump-points=", b"air-dash-point=", b"air-point-reset=", b"air-dash=none",
                                    b"dash-policy=",
                                    b"cruise-boost=", b"powered-flight=")) for line in lines)
            or "sasuke_air_points_test" in receipt):
        raise ValueError("Sasuke air-point opt-in cannot replace other movement policies")
    receipt.pop("air_action_budget")
    receipt["sasuke_air_points_test"] = dict(proof)
    return gameplay.replace(OLD+b"\n", b"\n".join(POINT_ROWS)+b"\n", 1)
