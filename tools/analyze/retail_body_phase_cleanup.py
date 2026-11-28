"""Replay the phase-3 wait, next-model request, and body activity boundary."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_body_phase_exit as body_exit
from tools.extract.retail_borg_roster import source_table


s, area = body_exit.s, body_exit.area
EXIT_SHA256 = "218f28c30504831fa51d6bc6afb0c45fb6af65526356a8323f8c3039e43333f6"
TEXT = (
    (0x8005514C, 1052, "d6c97ccc49c45a9a4879c127da9edfc208c9aec62e4cd08d14d6df366d18aca2"),
    (0x800541AC, 1456, "b4d201a64a05727912a7435ecce9280e7f9a22075fae0afa2e59b9470f211748"),
    (0x80068424, 128, "e0858246af60305755b74147e7eef812964b3e37b2fc3362829e0787dc9fb520"),
    (0x800683EC, 56, "b6f0fb762eb732469ba4718d91088962f5a54b3f767da05da80c19e3cfc065f5"),
    (0x80041BF0, 808, "7841132a1b1dea1c9dd15771874175f2547880197713d300f363a12d08b6a000"),
    (0x80041B58, 152, "efd8a652fd31756b0961753e812ce1436414049dc55e725da98b13de052fcc4a"),
    (0x800422D8, 40, "24d70439c67b9c60f1f459cf0d60640e4316882135c08e492e618eed4ab0f696"),
    (0x80197950, 188, "0ec9de608edc53db500d232ccc1673b884437a70824f802eea7941bae3a90b0a"),
    (0x80197A0C, 204, "96222c5bda44dae9f8bb740618f6f8c5d08d455044410156499752c80db51a82"),
    (0x80197AD8, 188, "e3650544ce98694f5c3a5ca208c4dfe1479fbc02f46712a3a8a75d8d210c9082"),
    (0x80006E3C, 80, "60efb8dd56f011372a9dfdce09df21db9920b2aa780bf848746cc8d646ca3da7"),
    (0x80006E8C, 76, "e844265fc7fd2b7a9d32c83f112f764b5b09943637b0d4c59eda206a593acae9"),
)
CASES = (("queue-next", 0x0106, 504), ("roster-exhausted", None, None), ("mode1-terminal-placeholder", 0x0F07, 693))


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 12:
        raise ValueError("normal body phase cleanup report extent")
    cases = []
    pos = 0

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"normal body phase cleanup changed at row {pos+1}: {tag}")
        pos += 1

    for model, _, _, _ in area.bucket.MODELS:
        caster = model & 255
        for i, (name, wanted, afs_member) in enumerate(CASES):
            gone, queued = i == 1, i == 0
            if wanted is not None:
                take("body-cleanup-request", (caster, i, f"{wanted:04X}", afs_member, int(not queued), int(queued),
                                              int(queued), int(queued), int(queued), 1, 1, 1, 60))
            take("body-cleanup-expiry", (caster, i, int(not gone), 3 if gone else 0, 2 if gone else 0,
                                         1 if gone else 3, 1 if gone else 2, 2, 0, int(gone), int(not gone)))
            row = {
                "caster": f"gg4e08:{model:04x}", "area_key": 19+int(model == 0x0107), "case": name,
                "declared_scene_mode": 1 if i == 2 else 0, "declared_roster_length": 2,
                "declared_actor_ordinal": 0 if queued else 2,
                "phase3_wait_armed": 60, "excluded_player_mask_visits": 2,
                "wait_expiry_allowed_visit": 60,
                "next_model_request": None if wanted is None else {
                    "model": f"{wanted:04X}", "color_slot": 0, "afs_member": afs_member,
                    "requested_roster_ordinal": int(queued), "cursor_before": 1, "cursor_at_disc_cut": int(not queued),
                    "selected_row_marked": queued, "old_row_marked": queued,
                    "old_empty_owner_returned": True, "archive_context_heap_returned": True,
                    "actor_still_active_at_disc_cut": True, "disc_load_ran": False,
                },
                "after_wait": {"actor_active": not gone, "actor_phase": 3 if gone else 0,
                               "subphase": 2 if gone else 0, "scene_slot_mask": 1 if gone else 3,
                               "scene_active_count": 1 if gone else 2, "retained_hit_token": 2,
                               "old_empty_owner_still_bound": gone},
            }
            if queued:
                take("body-cleanup-ready", (caster, 1, "0106", 1, 0))
                row["phase_zero_control"] = {
                    "missing_owner_leaves_phase_zero": True, "declared_owner_reused_from_pool": True,
                    "with_owner_model_at_class_setup": "0106", "phase_at_class_setup": 1,
                    "generation_delta_at_class_setup": 1, "hp_at_class_setup": 0, "class_setup_ran": False,
                }
            cases.append(row)
    if pos != len(lines):
        raise ValueError("extra phase cleanup report rows")
    return cases


def verify_source(image):
    body_exit.verify_source(image)
    s.verify_text(image, TEXT)
    for call, target in ((0x800552EC, 0x80068424), (0x800552F4, 0x800683EC), (0x80055340, 0x80041BF0),
                         (0x80041C14, 0x80041B58), (0x80041BBC, 0x80197AD8), (0x80041EE0, 0x8000B07C),
                         (0x800541C8, 0x800422D8), (0x80054324, 0x80056CE0)):
        s.link(image, call, target)
    if image.read(0x8036F5D4+16, 12, "data") != struct.pack(">III", 32 << 16, 0x80006E3C, 0x80006E8C):
        raise ValueError("phase cleanup empty archive owner pool operands changed")
    for _, model, member in CASES:
        if model is not None:
            root = source_table(image, 0x802D0584, model)
            if image.read(root, 4, "data") != struct.pack(">I", member):
                raise ValueError("phase cleanup model member changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body phase cleanup output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-body-phase-exit.json", EXIT_SHA256)
    payload = area.bucket.qualified_payload(image, afs, (area.RECEIVER, area.RECEIVER_BLOCK_SHA256,
                                                         area.ACTOR_SHA256[area.RECEIVER]))
    cases = decode(s.fixture_report(fixture, dol, module, ("--body-cleanup",), payload))
    raw = s.encode({
        "kind": "gf-retail-body-phase-cleanup", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "phase_exit_sha256": EXIT_SHA256, "receiver": "gg4e08:000a",
        "actor_seat": 1, "scene_allow_mask": {"off": 0, "holds_player_1": 1, "allows_player_1": 2},
        "entries": {"phase3": "8005514C", "archive_replacement": "80041BF0", "disc_call_cut": "8000B07C",
                    "phase_zero": "800541AC", "resource_owner_check": "800422D8", "class_setup_cut": "80056CE0"},
        "cases": cases,
        "notes": [
            "Start each fork from the real knockout and unmasked phase-2 exit. Queue contents, scene counts and the later mode-1 branch are declared inlets.",
            "The receiving seat's old resource pointer names a source-allocated empty archive context, not a built rig. The loader returns its original pool cell and heap allocation before the disc-entry cut.",
            "Each post-cut phase-3 actor visit is invoked separately. The unfinished loader is not resumed, and no disc response or replacement content is injected.",
            "The normal queue path cycles the next row and marks both new and old rows; the mode-1 placeholder request leaves the cursor and row flags alone.",
            "Roster exhaustion marks the actor inactive without freeing the declared old archive owner. The other paths remain active at phase zero. The queued path checks a missing-owner refusal and a declared owner-presence control only up to class setup.",
            "These are actor-local visits, not wall-clock frames. Full resource teardown, a completed respawn, transforms, rendering and native gameplay remain unqualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
