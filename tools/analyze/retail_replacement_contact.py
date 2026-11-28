"""Join the original replacement guard to ordinary and alternate HP paths."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_alternate_vm as vm
from tools.analyze import retail_body_registry as registry
from tools.analyze import retail_melee_pose_clock as melee
from tools.analyze import retail_replacement_entry as entry


s = entry.s
RECEIPTS = {
    "replacement-entry": "a1e8c383eb3717c56969fdeef95d2894a87f686249861ccfacefa933464194f9",
    "body-registry": "63682c1f85334bcdb5565a69191676fba916c7071f95230d340f77e220efac92",
    "alternate-vm": "77e8b3fba6a60ad350871d38ceb3e7bea63d4b25d79fa6aa984f7580a5fcadae",
    "melee-pose-clock": "7bb38ad95e772e1d0e3e3527a647d88319e757ef2695ce430694c1c7576af682",
}
VISITS = ((80, 0xC3, 59), (81, 0, 58), (82, 0, 57), (138, 0, 1), (139, 0, 0))
MODELS = (0x0807, 0x0900, 0x0902)


def verify_source(image):
    entry.verify_source(image)
    registry.verify_source(image)
    s.verify_text(image, (*vm.TEXT, *melee.TEXT))
    for site, target in ((0x8004C6F0, 0x8008AC80), (0x8002E084, 0x8002E2A8),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344),
                         (0x8002F6DC, 0x8010E158)):
        s.link(image, site, target)
    if (image.read(0x8002F6E0, 16, "text") != bytes.fromhex("a87e02723800000160630043b07e0272")
            or [image.read(pc+16, 2, "data") for pc in (melee.ROUTES[0].amount, vm.ROUTES[0].record, vm.ROUTES[2].record)]
            != [b"\x00\x01", b"\x00\x03", b"\x00\x01"]):
        raise ValueError("replacement contact post-HP role-mask operands changed")


def source_inputs(image, afs):
    receipts = {name: s.read_metadata(s.MAPS / f"gg4e08-{name}.json", digest)
                for name, digest in RECEIPTS.items()}
    installed = receipts["body-registry"]
    time = next(row for row in installed["models"] if row["borg"] == "gg4e08:0106")
    candidates = time["observed_capacity_cases"]["32"]["published_queue_ordinals"]
    if (sorted(candidates["open"]["ordinary-receiver"]) != list(entry.replacement.RECEIVING_STEPS)
            or candidates["open"]["ordinary-receiver"] != candidates["ordinary-receive-guard"]["alternate-receiver"]
            or "ordinary-receiver" in candidates["ordinary-receive-guard"]):
        raise ValueError("replacement contact body registry scope changed")
    reviewed = s.read_metadata(s.MAPS / "reaction-melee-map.json", melee.MELEE_SHA256)
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        tau, tau_source = melee.verify_route(image, s.RetailArchive(stream), reviewed, melee.ROUTES[0])
        if s.archive_stamp(stream) != stamp:
            raise ValueError("replacement contact Tau source changed")
    alt, alt_sources = vm.qualified_inputs(image, afs, receipts["alternate-vm"]["static_role_program_choices"],
                                            (vm.ROUTES[0], vm.ROUTES[2]))
    # The first two pieces of the alternate fixture were the stationary Sasuke inlet.
    # Here the receiver is the original Time Bomber replacement instead.
    attacks = tau + alt[7840+432:]
    return entry.replacement.payload(image, afs) + attacks, [tau_source, *alt_sources]


def decode(report):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != 60:
        raise ValueError("normal replacement contact report extent")
    rows = []
    for earlier in (6, 7):
        for visit, roles, guard in VISITS:
            ordinary, alternate = (0x3EF0 if guard == 0 else 0), (0x3EF0 if visit >= 81 else 0)
            bits = struct.pack(">f", guard).hex().upper()
            for model in MODELS:
                is_ordinary = model == MODELS[0]
                nurse = model == MODELS[1]
                reaches = bool(ordinary if is_ordinary else alternate)
                closes = nurse and reaches
                for opponent in (False, True):
                    request = ({0x0807: (4, 17), 0x0900: (-200, -50), 0x0902: (2, 9)}[model][opponent]) if reaches else 0
                    hp_after = min(220, 110-request)
                    damage = max(request, 0)
                    credit = [damage, 0] if opponent else [0, damage]
                    after_mask = roles | (0x43 if closes else 0)
                    after_ordinary, after_alternate = (0, 0) if closes else (ordinary, alternate)
                    fields = (earlier, visit, f"{model:04X}", int(opponent), f"{roles:04X}", bits,
                              f"{ordinary:04X}", f"{alternate:04X}", int(reaches and not nurse), int(reaches),
                              request, hp_after, 3 if reaches else 0, *credit, 16+damage,
                              f"{after_mask:04X}", f"{after_ordinary:04X}", f"{after_alternate:04X}")
                    if lines[len(rows)] != "entry-contact: " + " ".join(map(str, fields)):
                        raise ValueError(f"normal replacement contact changed at {earlier}/{visit}/{model:04X}/{int(opponent)}")
                    rows.append({
                        "previous_ko_source": f"gg4e08:010{earlier}", "replacement_visit": visit,
                        "probe_borg": f"gg4e08:{model:04x}", "relation": "opponent" if opponent else "ally",
                        "pair_list": "ordinary" if is_ordinary else "alternate",
                        "role_mask_before": f"{roles:04X}", "ordinary_guard_before_and_after": guard,
                        "receiving_cells_before": [ordinary.bit_count(), alternate.bit_count()],
                        "hp_before": 110, "amount_calculator_calls": int(reaches and not nurse),
                        "signed_hp_request": request if reaches else None, "hp_after": hp_after,
                        "hp_removed": damage, "hp_restored": max(0, hp_after-110),
                        "accepted_token": 3 if reaches else None, "repeat_hp_writes": 0,
                        "probe_opponent_and_ally_damage_credit": credit,
                        "replacement_received_damage_ledger": 16+damage,
                        "role_mask_after": f"{after_mask:04X}",
                        "receiving_cells_on_next_rebuild": [after_ordinary.bit_count(), after_alternate.bit_count()],
                    })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal replacement contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    payload, sources = source_inputs(image, afs)
    rows = decode(s.fixture_report(fixture, dol, module, ("--replacement-contact",), payload))
    raw = s.encode({
        "kind": "gf-retail-replacement-contact", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "prior_receipts": RECEIPTS, "replacement": "gg4e08:0106", "seat": 1,
        "receive_count_order": ["ordinary", "alternate"], "first_alternate_receiving_visit": 81,
        "first_ordinary_receiving_visit": 139, "sources": sources, "cases": rows,
        "notes": [
            "The qualified Time Bomber replacement retains its original body startup visits under declared scene state 4. The two receiving gates run without any host edits to the mask or guard.",
            "Three exact opcode-10 requests run in separate disposable attack forks. Their caster, frame, alignment and team relation are declared; the replacement HP is set to 110 only in these forks. The old knockout area is closed first.",
            "The original HP caller returns. Angel Nurse's flagged amount row applies the post-HP role-mask OR 0043 at 8002F6EC; it does not spend the ordinary guard or free the bound body cells.",
            "Receiver counts are observed on both registries. A same-list repeat of the accepted token cannot write HP again. The main replacement path remains neutral and the forks do not advance another actor visit.",
            "This does not qualify live player attacks, further reaction/guard timing, audible playback, or native gameplay.",
        ],
    }, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "attack_forks": len(rows),
            "hp_writer_forks": sum(row["accepted_token"] is not None for row in rows),
            "replacement_actor_prefix_visits": 278}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
