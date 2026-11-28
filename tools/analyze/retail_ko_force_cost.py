"""Bind the qualified normal Tau KO cost, not its strike or battle scheduler."""
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256


MODEL, MEMBER, COST = 0x0807, 626, 90
MEMBER_SHA256 = "d4c2344652087957bd1a71205c928083191f78eeefc13b956814e5803f45a7d0"
EVIDENCE = "docs/api-map/gg4e08-world-allied-ko.json"
EVIDENCE_SHA256 = "4a49351e3fee49c2dd4f8057c2715f3e2fc869a0b6001c131cb62805e56b1cbf"
TEXT = (
    (0x80055F90, 444, "e5c63b3dcbe89a313c0816f6589692220629ff56d813604552ac22677d83c1f1"),
    (0x80066168, 48, "7bbfae7ee17d25becbf96c3628bc6fa74ea5340cd55e6c8d90712cad132fafcd"),
)


def qualify(image):
    from tools.analyze.retail_type32_roster import link, verify_text
    from tools.extract.retail_borg_roster import levels, source_table

    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified Tau KO-cost source")
    verify_text(image, TEXT)
    link(image, 0x8005601C, 0x80066168)
    root, = struct.unpack(">I", image.read(0x802DD3B4+4*(MODEL >> 8), 4, "data"))
    normal_cost = root+12*(MODEL & 255)
    if (normal_cost != 0x802DD060 or struct.unpack(">h", image.read(normal_cost, 2, "data")) != (COST,)
            or struct.unpack(">I", image.read(source_table(image, 0x802D0584, MODEL), 4, "data")) != (MEMBER,)
            or levels(image, MODEL)[0][0] != 60):
        raise ValueError("normal Tau KO cost, HP or archive slot changed")
    raw = read_bounded(Path(__file__).resolve().parents[2]/EVIDENCE, 64 << 10)
    if sha256(raw).hexdigest() != EVIDENCE_SHA256:
        raise ValueError("normal Tau KO receipt changed")
    witness = json.loads(raw)
    if (witness["source_dol_sha256"], witness["tau_member_sha256"], witness["cached_force_cost_per_actor"]) != (
            SOURCE_SHA256, MEMBER_SHA256, COST):
        raise ValueError("normal Tau KO receipt scope disagrees")
    return {"cost": COST, "model": "0807", "variant": 0, "member_sha256": MEMBER_SHA256,
            "table_address": f"{normal_cost:08X}", "cache_site": "80056020", "receipt": EVIDENCE,
            "receipt_sha256": EVIDENCE_SHA256, "scope": "KO Force accounting only; not hit admission or round scheduling"}


def bind(model, member_index, member_digest, gameplay, battle_receipt, source):
    if model != MODEL or source is None:
        return gameplay
    rows = gameplay.splitlines()
    if (member_index != MEMBER or member_digest != MEMBER_SHA256 or source.get("cost") != COST
            or source.get("member_sha256") != MEMBER_SHA256 or source.get("variant") != 0
            or source.get("receipt_sha256") != EVIDENCE_SHA256 or not isinstance(battle_receipt, dict)
            or b"variant=normal" not in rows or b"health=60" not in rows
            or len([row for row in rows if row.startswith(b"melee=opening-slash|")]) != 1
            or any(row.startswith(b"ko-force-cost=") for row in rows)
            or "ko_force_cost_test" in battle_receipt or not gameplay.endswith(b"\n")):
        raise ValueError("Tau KO cost needs its exact normal melee package")
    battle_receipt["ko_force_cost_test"] = source
    return gameplay + f"ko-force-cost={COST}\n".encode("ascii")
