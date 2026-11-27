"""Append a grounded authored X-bomb trial to two exact normal Workshop profiles."""
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_action_links import ActionLinker
from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256


# Normal AFS member, digest, existing B key and call, new type-15 key and call.
PILOTS = {
    0x0000: (485, "06134b973ea5e4843a1fa169c12e2e74415d2aa90341d3c23d720a6b6e270399", 0, "800705EC", 0, "80071678"),
    0x000A: (495, "d58b8ca4bf80cb58543d32392af48d8f25fd797fa96065df0904ddbe5200d084", 4, "800705AC", 3, "800716C0"),
}
MOTION_SHA256 = "965b57fb4994a6274c3c5e0d7f77fc67eabb9326617db84f5a08f6f4de9ae284"
EVIDENCE = {
    "x-cue": "347d87226b3dc32b23284b04112a8e707da47f97ea8f67c9e4172e2ea650411b",
    "aim": "be48efd3435f6108d5adc3b058d508db969a746352620e54a7ec473cf8a6fc6d",
    "actor-area": "c38d7c2c04e4be7bd04a3371f47b0bb61224bede91058518dd030f891addbf64",
    "floor-contact": "c04ed8d69b761ebb4b1c5ed9bd993145baff251220c61b30eeff2a32aebc2f3f",
    "area": "cf0cfd5ab6c0b34d4bac8f97ea3ba6f5aa7271c826b5856d9f5cb25785ba56f4",
}
POSE_BANKS = (0x802BDE80, 0x802BC080, 0x802BD780, 0x802BAE60)
X_SLOT = (4, 0, 274144)
NATIVE_TIMING = {"release_tick": 12, "move_ticks": 28, "refill_ticks": 180, "flight_ticks": 120, "area_queries": 16}
ROWS = (
    "# Authored ordinary grounded X; the source phase counter is not a native frame.",
    f"stock=bomb-stock|5|{NATIVE_TIMING['refill_ticks']}",
    f"projectile=ninja-bomb|orb|1|40|0|0.3|0|10|{NATIVE_TIMING['flight_ticks']}|10|stop-on-stage",
    "origin=ninja-bomb|11|55|0|0|fixed",
    "look=ninja-bomb|18|232|146|43",
    "ballistic-aim=ninja-bomb|0.8",
    "ballistic-gravity-refit=ninja-bomb|4|0|7.5|0.15",
    f"impact-area=ninja-bomb|impact|3.1|31|387.5|{NATIVE_TIMING['area_queries']}|20",
    "impact-area-shapes=ninja-bomb|shared",
    f"move=ninja-bomb|x|20|grounded|{NATIVE_TIMING['move_ticks']}|4:0",
    "entry-stock=ninja-bomb|bomb-stock|1",
    f"release=ninja-bomb|{NATIVE_TIMING['release_tick']}|bomb-stock|1|ninja-bomb",
)


def qualify(image, evidence_dir=None):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified Ninja bomb source")
    root = Path(evidence_dir) if evidence_dir is not None else Path(__file__).resolve().parents[2] / "docs/api-map"
    reports, files = {}, {}
    for label, digest in EVIDENCE.items():
        name = f"gg4e08-bomb-{label}.json"
        raw = read_bounded(root / name, 64 << 10)
        if sha256(raw).hexdigest() != digest:
            raise ValueError(f"unqualified Ninja bomb {label} receipt")
        report = json.loads(raw)
        if (report["kind"], report["source_dol_sha256"]) != (f"gf-retail-bomb-{label}", SOURCE_SHA256):
            raise ValueError(f"Ninja bomb {label} receipt disagrees with source")
        reports[label] = report
        files[f"docs/api-map/{name}"] = digest
    ids = {f"gg4e08:{m:04x}" for m in PILOTS}
    for label in ("x-cue", "aim", "actor-area", "floor-contact"):
        if not ids <= {r["borg"] for r in reports[label]["pilots"]}:
            raise ValueError(f"Ninja bomb {label} missing an exact pilot")
    if ActionLinker(image, {}, ()).action_entry_descriptor(POSE_BANKS, 4, 0) != (4, 0, 4, 0):
        raise ValueError("Ninja bomb P4:0 descriptor disagrees")
    for model, (_, _, _, _, key, site) in PILOTS.items():
        row = image.read(0x802D6D68+56*key, 56, "data")
        if (row[2:4] != b"\x0b\x01" or struct.unpack_from(">4f", row, 4) != (55, 0, 0, 40)
                or row[44:46] != b"\x00\x00" or struct.unpack_from(">H", row, 46) != (model,)
                or reports["area"]["source"]["common_collision"]["group"] != 3
                or not any(r["borg"] == f"gg4e08:{model:04x}" and (r["key"], r["release_site"]) == (key, site)
                           for r in reports["aim"]["pilots"])):
            raise ValueError("Ninja bomb origin or factory evidence disagrees")
    return {
        "scope": "exact normal Normal Ninja and Sasuke, authored grounded X appended to Workshop B",
        "borgs": sorted(ids), "source_evidence": files, "retail_behavior_verified": False,
    }


def bind(model, member, digest, gameplay, receipt, evidence, *, motion, clips, timing, joints, level):
    if model not in PILOTS:
        return gameplay
    source_member, source_digest, b_key, b_site, key, site = PILOTS[model]
    if (member, digest) != (source_member, source_digest) or sha256(motion).hexdigest() != MOTION_SHA256:
        raise ValueError("Ninja bomb requires its exact normal member and motion bank")
    if (len(joints) <= 11 or joints[11]["index"] != 11 or X_SLOT not in tuple(map(tuple, clips))
            or tuple(level[5:9]) != (5, 5, 0, 300)):
        raise ValueError("Ninja bomb requires the normal key-11 rig, P4:0 clip and channel-2 stock")
    pose = [r for r in timing if r["archive_offset"] == X_SLOT[2]]
    if (len(pose) != 1 or pose[0].get("aobj_flags") != [0] or pose[0].get("authored_end_frames") != [45.]
            or pose[0].get("strict_metadata_refusals") or pose[0].get("animated_joint_count") != 20):
        raise ValueError("Ninja bomb requires a bounded strict 4:0 pose")
    if (receipt.get("body_binding"), receipt.get("source_normal_factory_family"), receipt.get("source_normal_factory_key"),
            receipt.get("source_release_site"), receipt.get("retail_behavior_verified")) != (
            "strict record zero", "type-14", b_key, b_site, False):
        raise ValueError("Ninja bomb requires the existing ordinary B Workshop profile")
    if "ninja_bomb_test" in receipt:
        raise ValueError("Ninja bomb test cannot replace an existing bind")
    lines = gameplay.splitlines()
    if (not gameplay.endswith(b"\n") or lines.count(b"variant=normal") != 1 or lines.count(b"body-record=0") != 1
            or lines.count(b"release=basic-shot|6|shot-stock|1|shot") != 1):
        raise ValueError("Ninja bomb requires an unchanged normal-only B release")
    if any(line.split(b"|")[1:2] == [b"x"] for line in lines if line.startswith(b"move=")) or any(
        line.startswith((b"stock=bomb-stock|", b"projectile=ninja-bomb|", b"move=ninja-bomb|")) for line in lines
    ):
        raise ValueError("Ninja bomb cannot replace any existing X action or bomb identity")
    receipt["ninja_bomb_test"] = {
        **evidence, "source_factory_family": "type-15", "source_factory_key": key, "source_release_site": site,
        "source_stock_channel": 2, "source_cost": 1, "pose": "4:0", "pose_policy": "single authored full-body lane",
        "native_timing": NATIVE_TIMING, "body_record": 0, "native_base_damage": [10, 20],
        "source_timing_converted": False, "launch_audio_bound": False,
    }
    return gameplay+("\n".join(ROWS)+"\n").encode("ascii")
