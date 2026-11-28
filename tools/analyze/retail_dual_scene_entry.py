"""Verify the original scene entry barrier with two loaded normal actors."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_scene_entry as scene
from tools.extract.retail_borg_roster import levels, source_table


s = scene.s
SCENE_SHA256 = "318403fc611f8240dfabe8ee1af167d1a648ac82f94393132e8c7af46997f6fe"
TEXT = (
    (0x800536B0, 248, "99f5f9fd9611f8b4e1ff792fa40de5f9d4e047e024e846ae148937434cb6b645"),
    (0x800537A8, 100, "b83557e7c4652c97b3f27dbc2b8d8821cf5083fc4f19f1a0166bb4797884f53a"),
    (0x800410BC, 460, "02916edb2316fb05006ad643d225c63d9fb26b888ce7d053cecd194e9d0fcafa"),
    (0x80087CF0, 60, "0a36f0211bae8654c9502654856eef93e12a2084ccdbe2ad79665ce4c6c21e72"),
)
SASUKE_ASSETS = (
    (0, 432, "bd5c6a7611df76737868e6d34580ff4e8dc464e288d23340c753e3f45cfec6c5"),
    (2, 7840, "8d90bfdd7a4d598b4c0869fff93364b785d4cc98c876d56c99ce44fc86579414"),
    (3, 368160, "965b57fb4994a6274c3c5e0d7f77fc67eabb9326617db84f5a08f6f4de9ae284"),
    (4, 241128, "ee4ae3bdc9d7babaf3325ed62a9fa373698fe20fbe36aac2594d2250d332a0e4"),
)
MODELS = (
    (0x000A, 495, "d58b8ca4bf80cb58543d32392af48d8f25fd797fa96065df0904ddbe5200d084", SASUKE_ASSETS),
    (0x0106, 504, scene.entry.replacement.MEMBER_SHA256, scene.entry.replacement.ASSETS),
)
STEPS = scene.entry.replacement.RECEIVING_STEPS


def decode(report):
    probe = scene.Report(report, 30)
    rows = []
    for delay in (0, 10):
        opened = 174+delay
        probe.take("dual-cold", (delay, 19, 4, 46, 32))
        visits = sorted({1, 2, 11, 12, 21, 21+delay, 81, 81+delay, 82+delay, 83+delay, opened-1, opened, opened+1})
        samples = []
        pads = []
        for n in visits:
            before = int(n > 81)+2*int(n > 81+delay)
            ready = int(n >= 81)+2*int(n >= 81+delay)
            stage = 0 if n < 82+delay else 1 if n == 82+delay else 2 if n < opened-1 else 3
            status = 3 if n < opened else 4
            timer = 90 if n == 83+delay else 0
            subphases = [0 if n < start else 1 if n < opened else 2 for start in (81, 81+delay)]
            masks = ["00EF" if n < start else "00C3" if n < opened else "0000" for start in (21, 21+delay)]
            shape_bits = "0000" if n < opened else "3EF0"
            if n >= opened:
                accepted = n > opened
                probe.take("dual-pad", (delay, n, 2 if accepted else 0, int(accepted), 9 if accepted else 10,
                                        "80130584" if accepted else "80029490"))
                pads.append({
                    "visit": n, "processed_b_packet_seat": 1, "pad_read_order": [0, 1] if accepted else [],
                    "seat_0_b_held": False, "seat_0_stock_after": 10, "seat_1_stock_after": 9 if accepted else 10,
                    "factory_request": {"owner_seat": 1, "key": 2, "pose": 10,
                                        "allocation_ran": False} if accepted else None,
                })
            probe.take("dual-scene", (delay, n, before, ready, status, stage, timer, *subphases, 0,
                                      *masks, shape_bits, shape_bits))
            samples.append({
                "visit": n, "ready_before_scan_mask": before, "ready_after_actors_mask": ready,
                "scene_status": status, "scene_substep": stage, "timer": timer,
                "actor_pass_order": [0, 1], "actor_subphases": subphases, "role_masks": masks,
                "ordinary_receiving_shape_counts": [0, 0] if n < opened else [9, 9],
            })
        rows.append({
            "seat_1_phase_dispatch_held_visits": delay, "seat_1_hold_visit_range": [2, 11] if delay else None,
            "both_constructors_finished": 1, "ready_writes_by_seat": [81, 81+delay],
            "coordinator_releases_readiness_barrier": 82+delay, "scene_enters_status_four": opened,
            "first_both_receiving_lists": opened, "first_both_pad_reads": opened+1,
            "source_view_roots_by_seat": [19, 4], "total_view_wrappers": 46, "total_bound_shapes": 32,
            "timeline": samples, "processed_b_forks": pads,
        })
    if probe.pos != len(probe.lines):
        raise ValueError("extra dual scene report rows")
    return rows


def verify_source(image):
    scene.verify_source(image)
    s.verify_text(image, TEXT)
    for at, to in ((0x800536EC, 0x800537A8), (0x80041248, 0x8000B07C), (0x80041270, 0x8000A154),
                   (0x8002948C, 0x80087CF0), (0x80054140, 0x8005A580)):
        s.link(image, at, to)
    for model, member, *_ in MODELS:
        if image.read(source_table(image, 0x802D0584, model), 4, "data") != struct.pack(">I", member):
            raise ValueError("dual scene source model archive changed")
    if (levels(image, 0x000A)[0][:9], levels(image, 0x0106)[0][:9]) != (
            (180, 10, 10, 0, 90, 5, 5, 0, 300), (220, 10, 10, 2, 0, 3, 3, 1, 300)):
        raise ValueError("dual scene source level-one stats changed")


def payload(afs):
    chunks = []
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        sources = s.RetailArchive(stream)
        for model, index, digest, roles in MODELS:
            member, checksum = sources.member(index, model)
            if checksum != digest or member.entries[1]["size"]:
                raise ValueError("dual scene source archive identity changed")
            for role, size, checksum in roles:
                raw, _ = member.read(role, size)
                if len(raw) != size or sha256(raw).hexdigest() != checksum:
                    raise ValueError(f"dual scene model {model:04X} role {role} changed")
                if role == 2 and (raw[:32] != bytes((*range(16), *([255]*16))) or
                        tuple(i for i in range(16) if raw[35+80*i] & 2) != STEPS):
                    raise ValueError("dual scene receiving descriptor set changed")
                chunks.append(raw)
        if s.archive_stamp(stream) != stamp:
            raise ValueError("dual scene archive input changed")
    return b"".join(chunks)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("dual scene entry output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-scene-entry.json", SCENE_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, payload=payload(afs)))
    raw = s.encode({
        "kind": "gf-retail-dual-scene-entry", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "scene_entry_sha256": SCENE_SHA256,
        "actors": [{"seat": seat, "borg": f"gg4e08:{model:04x}", "afs_member": index, "member_sha256": digest,
                    "roles": [{"role": role, "bytes": size, "sha256": checksum} for role, size, checksum in roles]}
                   for seat, (model, index, digest, roles) in enumerate(MODELS)],
        "receiving_steps_per_body": list(STEPS), "cases": cases,
        "notes": [
            "The original six-seat initializer reserves the two named seats and leaves four inactive. The host supplies only the exact decompressed role bytes for the original normal cold-load requests.",
            "Each declared delivery invokes the original completion callback and creates a distinct archive owner. The original scene dispatcher then finishes both body constructors, including SDK view allocation.",
            "The actor pass runs to completion on each isolated scene prefix before the common world tail. The ordinary receiving registry rebuild is invoked separately.",
            "The ten-visit control changes only the scene actor mask for seat 1 after both constructors. The original readiness scan blocks until the later actor marks ready.",
            "The processed-pad fork targets seat 1 and cuts before bomb allocation. The preceding seat-0 pad decision remains neutral.",
            "No disc hardware, asynchronous delivery, full world tail, GX drawing, moving-body combat, other roster or native gameplay is qualified.",
        ],
    }, expanded=("actors", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "loaded_actors_per_case": 2}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
