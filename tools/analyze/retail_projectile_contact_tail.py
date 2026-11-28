"""Scope the portable slot-hold policy to three reviewed normal type-0 releases."""
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256


@dataclass(frozen=True)
class Route:
    key: int
    release: int
    debit: int
    amount: int
    member: str


ROUTES = {
    0x0800: Route(12, 0x80076C10, 0x80076BFC, 0x802D6270,
                  "a46ff3f12c1ceffc0c8ee51981060b1df35ac1b151a5f3384caa4972051c449b"),
    0x0801: Route(125, 0x800A403C, 0x800A4028, 0x802F7C30,
                  "8f1b062abff2cbee164545b04f0334b9f124801cd624251f36aec1457b66bcd1"),
    0x0802: Route(45, 0x8013D77C, 0x8013D74C, 0x80339E18,
                  "60cdc85a8b4b3f09da6b669cce05a315e0946140bea1023391a96d1f3159f57d"),
}
EVIDENCE = "docs/api-map/gg4e08-type0-contact.json"
EVIDENCE_SHA256 = "062a675dcf556775f610789cade7069fd0a355a9eab7828891c5fec0e0d3d337"
POLICY = b"actor-contact=shot|stop-and-hold-slot|3\n"


def qualify(image, path=None):
    raw = read_bounded(path if path is not None else Path(__file__).resolve().parents[2] / EVIDENCE, 16 << 10)
    if sha256(raw).hexdigest() != EVIDENCE_SHA256 or image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified type-0 contact-tail evidence or source")
    receipt = json.loads(raw)
    if (receipt["kind"], receipt["revision"], receipt["source_dol_sha256"]) != (
            "gf-retail-type0-contact", 1, SOURCE_SHA256):
        raise ValueError("unexpected type-0 contact-tail evidence identity")
    out = {}
    for (model, route), pilot in zip(ROUTES.items(), receipt["pilots"], strict=True):
        missing, repeat, accepted = pilot["cases"]
        if (pilot["borg"], pilot["key"], pilot["source"]["static_release_site"],
                accepted["case"], accepted["original_amount_record"], accepted["carrier_and_descriptor_return_visit"],
                accepted["first_followup"]["attack_count"], accepted["first_followup"]["phase"]) != (
                f"gg4e08:{model:04x}", route.key, f"{route.release:08X}", "accepted-contact",
                f"{route.amount:08X}", 3, 0, 2) or any(
                    row["hp_writer_reached"] or row["first_followup"]["attack_count"] != 1 for row in (missing, repeat)):
            raise ValueError("type-0 contact-tail evidence lost the stop and separate return")
        for site, value, target in ((route.release, 0x38800000 | route.key, 0x80082824),
                                    (route.debit, 0x38C00001, 0x8006DBE0)):
            operand, branch = struct.unpack(">II", image.read(site-4, 8, "text"))
            if operand != value or branch & 0xFC000003 != 0x48000001 or direct_branch(site, branch) != target:
                raise ValueError(f"type-0 contact-tail release or debit changed: {model:04X}")
        out[model] = {
            "scope": "normal authored ordinary B, carrier slot return after accepted contact only",
            "source_factory_family": "type-0", "source_factory_key": route.key, "source_release_site": f"{route.release:08X}",
            "source_debit_site": f"{route.debit:08X}", "source_evidence": EVIDENCE, "source_evidence_sha256": EVIDENCE_SHA256,
            "native_policy": "stop-and-hold-slot", "carrier_return_visits": 3, "retail_behavior_verified": False,
        }
    return out


def bind(model, member_digest, gameplay, receipt, evidence):
    if model not in ROUTES:
        return gameplay
    route = ROUTES[model]
    lines = gameplay.splitlines()
    # Gold Hero's normal/model-gated single-shot emitter records the handler
    # entry rather than the exact factory call. Do not accept his other form.
    origin_matches = (receipt.get("source_release_entry") == "8013D600" and
                      receipt.get("source_model_gated_muzzle") == "only model 0802; form 0803 would request 34") if model == 0x0802 else (
                      receipt.get("source_release_site") == f"{route.release:08X}")
    if (member_digest != route.member or not origin_matches
            or (receipt.get("body_binding"), receipt.get("source_level")) != ("strict record zero", 1)
            or (receipt.get("source_normal_factory_family"), receipt.get("source_normal_factory_key"),
                receipt.get("source_debit_site")) != ("type-0", route.key, f"{route.debit:08X}")
            or receipt.get("reaction_releases") != {"basic-shot": [("type-0", route.key)]}
            or receipt.get("retail_behavior_verified") is not False):
        raise ValueError("type-0 contact-tail test needs the exact normal authored paid release")
    release = f"release=basic-shot|{receipt['native_timing']['release_tick']}|shot-stock|1|shot".encode("ascii")
    if (lines.count(b"variant=normal") != 1 or lines.count(b"body-record=0") != 1
            or [r for r in lines if r.startswith(b"release=")] != [release]
            or sum(r.startswith(b"projectile=shot|") for r in lines) != 1
            or any(r.startswith((b"release-followup=", b"held-fire=", b"actor-contact=shot|")) for r in lines)
            or "projectile_contact_tail_test" in receipt):
        raise ValueError("type-0 contact-tail test cannot replace or widen the ordinary release")
    receipt["projectile_contact_tail_test"] = {**evidence[model], "move": "basic-shot", "projectile": "shot"}
    return gameplay + POLICY
