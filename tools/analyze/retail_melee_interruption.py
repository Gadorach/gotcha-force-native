"""Compare Tau's original pre-publication melee interruption and the authored native test."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct
from subprocess import run

from tools.analyze.reaction_emit import read_group
from tools.analyze import retail_bomber_hero_melee as tau
from tools.analyze import retail_dual_scene_entry as dual
from tools.analyze import retail_stage_side_scheduled_support as collision
from tools.extract.retail_borg_roster import levels, source_table


s = dual.s
MODEL, MEMBER = 0x0807, 626
MEMBER_SHA256 = "d4c2344652087957bd1a71205c928083191f78eeefc13b956814e5803f45a7d0"
ROLES = (
    (0, 432, "6c9b35fad296fc38940fb4f9b163219327556289526d8864406b2f7fd77fc051"),
    (2, 7840, "8835c40673a51544702ca7770649c92744d7d12105cdc1ab9851e23b212cf12a"),
    (3, 192512, "8d122314ee35eed8839ec723b002a4b06b14bca4b0c4fb3cfd58f7485b1e5144"),
    (4, 60072, "c27d0854dab9208d8fa7c4b4c9c6774bcf6ca9120ca1fa7b8da3ebf2ce647a06"),
)
DUAL_SHA256 = "7caed283865bedafac11776243ef37f7800aa698e369b78ab87cacaa4a27295b"


def verify_source(image):
    dual.verify_source(image)
    tau.verify_source(image)
    s.verify_text(image, tuple(row for row in collision.TEXT if row[0] == 0x8002BB14))
    for site, target in ((0x8002C4CC, 0x8002DB58), (0x8002C5A0, 0x8005C290),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        s.link(image, site, target)
    if (image.read(source_table(image, 0x802D0584, MODEL), 4, "data") != struct.pack(">I", MEMBER)
            or levels(image, MODEL)[0][0] != 60
            or image.read(0x80377E40, 4, "data") != bytes.fromhex("0A0100FF")
            or image.read(0x80377AD8, 2, "data") != bytes.fromhex("0014")):
        raise ValueError("Tau melee route identity changed")


def payload(afs):
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        source, digest = archive.member(MEMBER, MODEL)
        if digest != MEMBER_SHA256 or source.entries[1]["size"]:
            raise ValueError("Tau normal member changed")
        chunks = []
        for role, size, digest in ROLES:
            raw, _ = source.read(role, size)
            if len(raw) != size or sha256(raw).hexdigest() != digest:
                raise ValueError(f"Tau normal role {role} changed")
            if role == 2 and (read_group(raw, 1) != [(2, 1)] or
                              raw[0xAA0+2*80:0xAA0+2*80+8] != bytes.fromhex("000100010001000A")):
                raise ValueError("Tau exact group-1 descriptor changed")
            chunks.append(raw)
        if s.archive_stamp(stream) != stamp:
            raise ValueError("Tau source archive changed while reading")
    return b"".join(chunks)*2


def decode(report):
    lines = report.splitlines()
    if len(report) > 16384 or len(lines) != 81 or lines[0] != "melee-start: 174 4 42":
        raise ValueError("Tau melee interruption report extent or opening")
    bits = collision.rise.support.bits
    f32 = collision.rise.carry.stage.f32
    cases = []
    for scenario in (0, 1):
        early = bool(scenario)
        marks = []
        for visit in range(1, 41):
            a = 255 if visit < 8 or visit >= (20 if early else 32) else 7 if early else 6
            b = 6 if early and visit <= 23 else 255
            pub_a, pub_b = int(not early and 27 <= visit <= 31), int(early and 19 <= visit <= 23)
            token_a, token_b = int(not early and visit >= 27), int(early and visit >= 19)
            write = int(visit == (19 if early else 27))
            lower = 1 if visit < 8 else 0 if early and visit >= 20 else 61
            lower_after = 0 if early and visit == 19 else lower
            if visit < 8:
                pose = 137+visit
            elif early:
                pose = max(0, visit-14) if visit <= 19 else 50
            elif visit <= 27:
                pose = max(0, visit-17)
            else:
                pose = f32(pose+(.03 if visit <= 30 else 1))
            after_pose = 0 if early and visit == 19 else pose
            a_hp, b_hp = 60-17*(early and visit >= 19), 60-17*(not early and visit >= 27)
            returned_a, returned_b = int(visit >= (20 if early else 32)), int(early and visit >= 24)
            fields = (scenario, visit, a, b, pub_a, pub_b, token_a, token_b, write, bits(pose), bits(after_pose),
                      a_hp, b_hp, lower, lower_after, returned_a, returned_b)
            if lines[scenario*40+visit] != "melee-visit: " + " ".join(map(str, fields)):
                raise ValueError(f"Tau original melee timeline changed: {scenario}/{visit}")
            if visit in (1, 8, 18, 19, 20, 24, 26, 27, 31, 32, 40):
                marks.append({
                    "visit": visit, "melee_allocation_ordinals": [None if j == 255 else j for j in (a, b)],
                    "published_strikes": [pub_a, pub_b], "actor_outgoing_tokens": [token_a, token_b],
                    "hp_writes": write, "hp_by_seat": [a_hp, b_hp], "subject_pose_before_collision_bits": bits(pose),
                    "subject_pose_after_collision_bits": bits(after_pose), "subject_lower_selector": [lower, lower_after],
                    "exact_cells_on_free_list": [bool(returned_a), bool(returned_b)],
                })
        cases.append({
            "case": "early-opponent-hit" if early else "uninterrupted",
            "subject_input_and_cell_allocation_visit": 8, "subject_first_publication_visit": None if early else 27,
            "subject_hp_write_visit": None if early else 27, "opponent_hp_write_visit": 19 if early else None,
            "subject_cell_return_visit": 20 if early else 32,
            "subject_token_assigned": not early, "checkpoints": marks,
        })
    return cases


def native_report(executable):
    result = run([Path(executable).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    expected = ("native-melee: 0 8 0 0 0 17 17 60 48\n"
                "native-melee: 1 8 10 11 34 43 43 48 48\n")
    if result.returncode or result.stderr or result.stdout != expected:
        raise ValueError(f"authored native melee trace changed: {result.stdout} {result.stderr}")
    rows = []
    for line in result.stdout.splitlines():
        early, start, hit, cancel, restart, arm, dealt, a, b = map(int, line.split()[1:])
        rows.append({
            "case": "early-opponent-hit" if early else "uninterrupted", "subject_start_update": start,
            "incoming_hit_update": hit or None, "subject_interrupt_update": cancel or None,
            "subject_fresh_start_after_interrupt": restart or None, "subject_first_arm_update": arm,
            "subject_first_hp_write_update": dealt, "final_hp_by_actor": [a, b],
        })
    return rows


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Tau melee interruption output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-dual-scene-entry.json", DUAL_SHA256)
    report = s.fixture_report(fixture, dol, module, ("--melee-interrupt",), payload(afs))
    rows = decode(report)
    raw = s.encode({
        "kind": "gf-retail-melee-interruption", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "dual_scene_entry_sha256": DUAL_SHA256,
        "original_trace_sha256": sha256(report.encode("ascii")).hexdigest(),
        "actor": "gg4e08:0807", "subject_seat": 0, "opponent_seat": 1,
        "original_arm_pose": 10, "original_amount_record": "80377AD8", "original_hp_removed": 17,
        "original_opening_visit": 174, "collision_stop_before": "8005A580", "original_cases": rows,
        "native_authored_cases": native_report(native),
        "notes": [
            "Two independent normal Tau archives load through the original cold delivery and readiness paths. The initial roots, empty stage and processed B packets are declared.",
            "The original actor visit stops before the stage dispatcher. A separate original list rebuild and collision pass stop before the first actor presentation call. Visit counts refer to this bounded sequence, not completed normal battle frames.",
            "B allocates group-1 step 2 at entry. Publication and token assignment occur later at pose 10. An earlier opposing Tau reaches the exact amount and HP writer while the subject is at pose 5. The subject's lower selector clears before presentation, then the withheld cell is found on the original free list next visit without publishing.",
            "The native comparison uses authored Tau opening data; no slots are reserved before its melee window. An early hit cancels on the next native update. Holding that press does not replay the canceled move, while a fresh press later arms and hits.",
            "This verifies one interruption and its cell return. It does not qualify original recovery or subsequent re-entry, other actions, exact native geometry or damage, hardware, or original-game equivalence.",
        ],
    }, expanded=("original_cases", "native_authored_cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as f:
        f.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_cases": len(rows), "original_visits": 80}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
