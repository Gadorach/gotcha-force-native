"""Bind only the verified rifle continuation to the authored Workshop shot."""
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256

MODEL = 0x0600
MEMBER_SHA256 = "1f89a3b6721ab88c4ab8f07630d6eac3aa7ab2daefbb8e56e789fdaf656e734a"
EVIDENCE = "docs/api-map/gg4e08-rifle-next-receiver.json"
EVIDENCE_SHA256 = "6ebd399d481675d5f367d719dda04a3aea3c401a96f655c6845072c7ef5f35de"
POLICY = b"actor-contact=shot|continue-once-per-actor"
RELEASE_SITE, DEBIT_SITE = 0x800C9F98, 0x800C9F78


def qualify(image, path=None):
    """Require the exact source, proof and two distinct receivers, not a type-2 label."""
    raw = read_bounded(path if path is not None else Path(__file__).resolve().parents[2] / EVIDENCE, 8 << 10)
    if image.sha256 != SOURCE_SHA256 or sha256(raw).hexdigest() != EVIDENCE_SHA256:
        raise ValueError("unqualified rifle continuation source or receipt")
    receipt = json.loads(raw)
    release = struct.unpack(">2I", image.read(RELEASE_SITE-4, 8, "text"))
    debit, = struct.unpack(">I", image.read(DEBIT_SITE, 4, "text"))
    original = receipt["cases"][-1]
    if (
        (receipt["kind"], receipt["revision"], receipt["source_dol_sha256"]) !=
        ("gf-retail-rifle-next-receiver", 1, SOURCE_SHA256)
        or receipt["credit"] != {"borg": "gg4e08:0600", "carrier_key": 21, "amount_record": "8030A398"}
        or (original["case"], original["same_carrier_token"], original["first_hp"], original["second_hp"],
            original["stock"]) != ("fresh-second-body", 1, 172, 172, 1)
        or release[0] != 0x38800015
        or release[1] & 0xFC000003 != 0x48000001 or direct_branch(RELEASE_SITE, release[1]) != 0x800C3BE0
        or debit & 0xFC000003 != 0x48000001 or direct_branch(DEBIT_SITE, debit) != 0x8006DBE0
    ):
        raise ValueError("rifle continuation evidence does not identify the paid release")
    return {
        "scope": "normal Machine Red, authored ordinary B shot only",
        "source_evidence": EVIDENCE, "source_evidence_sha256": EVIDENCE_SHA256,
        "source_release_site": "800C9F98", "source_debit_site": "800C9F78",
        "source_factory_family": "type-2", "source_factory_key": 21,
        "native_policy": "continue-once-per-actor", "retail_behavior_verified": False,
    }


def bind(model, member_digest, gameplay, receipt, evidence):
    """Change one actor-contact row, preserving the authored launch and all defaults."""
    if model != MODEL:
        return gameplay
    if (
        member_digest != MEMBER_SHA256
        or (receipt.get("body_binding"), receipt.get("source_level")) != ("strict record zero", 1)
        or any(receipt.get(f"source_{field}") != evidence[f"source_{field}"]
               for field in ("release_site", "debit_site"))
        or (receipt.get("source_normal_factory_family"), receipt.get("source_normal_factory_key")) !=
        (evidence["source_factory_family"], evidence["source_factory_key"])
        or receipt.get("reaction_releases") != {"basic-shot": [("type-2", 21)]}
        or receipt.get("retail_behavior_verified") is not False
        or "projectile_continuation_test" in receipt
    ):
        raise ValueError("rifle continuation needs the exact normal authored paid B binding")
    lines = gameplay.splitlines()
    release = f"release=basic-shot|{receipt['native_timing']['release_tick']}|shot-stock|1|shot".encode("ascii")
    if (
        lines.count(b"variant=normal") != 1 or lines.count(b"body-record=0") != 1
        or [r for r in lines if r.startswith(b"release=")] != [release]
        or any(r.startswith((b"release-followup=", b"actor-contact=", b"held-fire=")) for r in lines)
        or sum(r.startswith(b"projectile=shot|") for r in lines) != 1
    ):
        raise ValueError("rifle continuation needs one ordinary projectile release and no existing contact policy")
    receipt["projectile_continuation_test"] = {**evidence, "move": "basic-shot", "projectile": "shot"}
    return gameplay + POLICY + b"\n"
