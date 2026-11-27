"""Join a type-41 parent's own contacts, terminal area and recipient tier."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import reaction_emit
from tools.analyze import retail_type41_area_hp as child


s, bucket = child.s, child.bucket
AREA_SHA256 = "cd8b2b0956d207a6db682152922eb45dd6203c4b9183aca176b4617ca3258e5a"
INCOMING_SHA256 = "f4e81c9cea51fcface812b83a446db04c3ad5998ff1e9e3b354139d0cd0e9389"
TEXT = (
    (0x8002CDA4, 160, "2dcecd916811217d411f21faca6a1ca97774fa3c65c5f3756bf92d230096a693"),
    (0x800FC6A0, 288, "4050c866c59419d4115c98a2d59cfb666b9d57191878a4e2a496f47f20eaff87"),
    (0x800FC7C0, 80, "7dd085af31393d70efb8ae3692e9843be03a60cc0839d1fa038fc7d210cb091d"),
)
ROW = re.compile(r"type41-parent:(?: [0-9]{1,3}){12} [0-9A-F]{8}(?: [0-9]{1,3}){7}")
CASES = (
    ("outgoing-near", 30, False, False, False, True),
    ("outgoing-exact-tangent", 42, False, False, False, True),
    ("outgoing-miss", 43, False, False, False, False),
    ("outgoing-ally", 30, False, True, False, True),
    ("incoming-lethal", 0, True, False, False, True),
    ("incoming-miss", 171, True, False, False, False),
    ("outgoing-and-remote-command", 30, False, False, True, True),
    ("incoming-and-remote-command", 0, True, False, True, True),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 14:
        raise ValueError("type-41 parent contact report extent")
    rows = []
    for model, _, cap, _ in bucket.MODELS:
        remote = model == 0x0107
        for i, (name, offset, incoming, ally, command, hit) in enumerate(CASES if remote else CASES[:6]):
            request = (17 if incoming else 6 if ally else 26) if hit else 0
            carrier_hp, receiver_hp = (0, 180) if incoming and hit else (1, 180-request)
            to_body = hit and not incoming
            in_latch, out_latch = int(hit and incoming), 16*to_body
            token = (2 if incoming else 1) if hit else 0
            child_token = token+1 if hit else 0
            stock = cap if hit else cap-1
            initial_ledgers = (request if to_body and not ally else 0, request if to_body and ally else 0,
                               request if to_body else 0)
            area_site = ("800E0AAC" if command else "800E0B18" if incoming else "800E0AD4") if hit else None
            area_hp = (3 if ally else 14) if to_body else 0
            cleared = (4 if ally else 17) if to_body else 0
            final_hp = receiver_hp-area_hp
            expected = (*map(str, (model & 255, i, request, carrier_hp, receiver_hp, out_latch, in_latch,
                                   token, *initial_ledgers, int(hit))), area_site or "00000000",
                        *map(str, (cap-1, stock, child_token, area_hp, final_hp, int(to_body), cleared)))
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None or tuple(line.split()[1:]) != expected:
                raise ValueError(f"type-41 parent, terminal branch or recipient tier changed: {model:04X}/{name}")
            rows.append({
                "borg": f"gg4e08:{model:04x}", "case": name, "parent_shape_role": 3, "parent_radius": 20,
                "axis_separation": offset, "incoming_key_4": incoming, "same_team": ally, "remote_command": command,
                "first_pair_token": token or None,
                "first_amount_row": ("802C4838" if incoming else "80336430" if remote else "80336418") if hit else None,
                "first_hp_request": request if hit else None,
                "parent_hp_after_first_pair": carrier_hp, "sasuke_hp_after_first_pair": receiver_hp,
                "parent_incoming_latch": in_latch, "parent_outgoing_latch": out_latch,
                "first_pair_ledgers": dict(zip(("owner_opponent", "owner_ally", "receiver"), initial_ledgers, strict=True)),
                "terminal_area_site": area_site, "terminal_area_key": 19+remote if hit else None,
                "stock_after_terminal": cap-1, "stock_after_return_visit": stock,
                "child_token": child_token or None, "child_hp_lost": area_hp if to_body else None,
                "sasuke_hp_at_final_cut": final_hp, "receiver_tier_at_child": 1 if to_body else None,
                "amount_with_only_tier_cleared": cleared if to_body else None,
            })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-41 parent contact output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    s.verify_text(image, (*child.TEXT, *TEXT))
    for site, target in ((0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344),
                         (0x800E0AD4, 0x80085E00), (0x800E0B18, 0x80085E00), (0x800E0AAC, 0x80085E00)):
        s.link(image, site, target)
    if (image.read(0x802C7CA0, 16, "data").hex() != "3f8000003f4ccccd3f0000003e800000" or
        any(struct.unpack(">H", image.read(at, 2, "data")) != (power,)
            for at, power in ((0x802C4838, 20), (0x80336418, 30), (0x80336430, 30)))):
        raise ValueError("type-41 parent reaction or common tier source changed")
    prior = s.read_metadata(s.MAPS / "gg4e08-type41-area-hp.json", AREA_SHA256)
    s.read_metadata(s.MAPS / "gg4e08-type76-incoming.json", INCOMING_SHA256)
    payload = bucket.qualified_payload(image, afs, (child.RECEIVER, child.RECEIVER_BLOCK_SHA256,
                                                     child.ACTOR_SHA256[child.RECEIVER]))
    hit = payload[21504:21504+reaction_emit.HIT_BYTES]
    for group, step, amount in ((15, 8, 9), (16, 9, 10)):
        at = reaction_emit.STEPS+step*reaction_emit.STEP_SIZE
        if (reaction_emit.read_group(hit, group) != [(step, amount)] or
            struct.unpack_from(">BBHH", hit, at) != (0, 0, 3, amount) or struct.unpack_from(">f", hit, at+24) != (20.0,)):
            raise ValueError("type-41 parent role or geometry changed")
    rows = decode(s.fixture_report(fixture, dol, module, ("--parent-contact",), payload))
    isolated = {(r["borg"], r["case"]): r["hp_lost"] for r in prior["cases"]}
    for row in rows:
        if row["child_hp_lost"] is not None and isolated[row["borg"], "ally-near" if row["same_team"] else "near"] != row["amount_with_only_tier_cleared"]:
            raise ValueError("type-41 parent tier control did not reproduce standalone area damage")
    raw = s.encode({
        "kind": "gf-retail-type41-parent-contact", "revision": 1,
        "source_dol_sha256": s.SOURCE_SHA256, "source_afs_sha256": s.AFS_SHA256,
        "type41_area_hp_sha256": AREA_SHA256, "type76_incoming_sha256": INCOMING_SHA256,
        "cases": rows,
        "notes": [
            "The normal paid parent publishes role 3 in both lists. Two publication visits hold the declared root before testing the stationary contact.",
            "Incoming controls construct the separate key-4 type-9 projectile from Sasuke. His body stays outside its and the bomb's radii; no fighter HP is removed.",
            "The original terminal parent bucket is isolated from the incoming projectile's bucket. Actual outgoing and zero-HP branches are distinct, and the Remote command takes priority over both.",
            "For outgoing hits the retained parent pair completes before the child's pair. A per-pass reset preserves the first token; the child uses its own token and amount row.",
            "Only a disposable calculator fork clears receiver +0x6CA. The retained sequence observes tier one after the parent hit and uses that tier's actual child amount.",
            "Declared roots, one stationary receiver and an empty stage grid are not live moving-body collision or native gameplay evidence.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
