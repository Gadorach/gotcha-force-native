"""Verify original post-area body clocks and fresh Time Bomber B input forks."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_scene_bomb_area as area


s = area.s
SCENE_AREA_SHA256 = "3cda65239a0ad53cd13966de0a88715fbb82977b550227a99d80bddda670f75b"
PARENT_HP_SHA256 = "2934423345d6d87e948edd1336fef00869c5c3bfec5a25678381a0c858c28a28"
TEXT = (
    (0x80054CBC, 52, "5031b1cbdf20bc9464eef663b0778abe4c88b6c39931b9b0be6f0feabc0fdb26"),
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    (0x80056900, 424, "52c4c7e19500682352bbf4503d8973d50f69fed24c5525d5b3ab40bba34bbfa2"),
    (0x80059068, 208, "ea22383e1d48094534b9c882b22400b794423bdea5657be04bcfab050da3716c"),
    (0x8005A378, 520, "b6a3dfe9d8bf1b34fb4d470444385f1bbdafe4ba0596786d67699ca9e92ecd2f"),
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8002CDA4, 160, "2dcecd916811217d411f21faca6a1ca97774fa3c65c5f3756bf92d230096a693"),
    (0x8002FC18, 164, "3292958ed7d95859bec49a2d87c4d022d0f977ffe7fd2df767df4c7781631171"),
    (0x801304B8, 704, "75d567d05a4ea6ef6537d8d513e3f1f1822187128f2586e81b2cdf4cca442406"),
    (0x8006DBE0, 224, "0b2e322f452d317b5f515e0ebd95f821a10cdbee5db9cdbd07678c8d251eed31"),
)
PARENT, AREA = "80336418", "802C49D0"
SASUKE, TIME, BOMB = "81020000", "81021E00", "8107B000"
ONE, NEGATIVE = "3F800000", "BD4CCCCD"
BODY_VISITS = (183, 184, 185, 186, 187, 188, 189, 198, 231, 235)
B_VISITS = (184, 185, 186, 189, 198, 235)


def placement(take, delay, shapes, visit):
    has_area = shapes == 2
    same_visit = has_area and visit == 185
    parent_hits = delay == 10 and (not has_area or same_visit)
    token = 3 if has_area else 2
    contacts = []
    if parent_hits:
        contacts.append((PARENT, SASUKE, 26, 180, 0))
    if same_visit:
        contacts.append((AREA, SASUKE, 14 if parent_hits else 17, 154 if parent_hits else 180, int(parent_hits)))
    if has_area:
        contacts.append((AREA, BOMB, 4, 1, 255))
    if same_visit:
        contacts.append((AREA, TIME, 4, 220, 0))
    opponent = (40 if parent_hits else 17) if has_area else (26 if parent_hits else 0)
    self_hit = 4 if has_area else 0
    history = [token, 2] if parent_hits and same_visit else [2 if has_area or parent_hits else 0, 0]
    hp = [180-opponent, 220-self_hit]
    carrier_hp = int(not has_area)
    take("area-reaction-placement", (delay, shapes, visit, len(contacts), token, carrier_hp,
                                     1+int(has_area), *hp, *history, opponent, opponent, self_hit, int(parent_hits)))
    writes = []
    for i, (record, recipient, request, before, tier) in enumerate(contacts):
        take("area-reaction-contact", (delay, shapes, visit, i, record, recipient, request, before, tier))
        writes.append({
            "source": "new-parent" if record == PARENT else "old-area",
            "receiver": {SASUKE: "seat-0", TIME: "seat-1", BOMB: "new-parent"}[recipient],
            "amount_record": record, "source_token": token if record == PARENT else 2,
            "receiver_tier_at_amount": None if tier == 255 else tier,
            "hp_before_write": before, "hp_requested": request, "hp_removed": min(before, request),
        })
    return {
        "visit_from_first_press": visit, "stock_after_second_press": 9, "new_parent_token": token,
        "new_parent_hp_after_world": carrier_hp, "attack_shapes_in_order": ["new-parent", "old-area"] if has_area else ["new-parent"],
        "ordinary_receivers": 19, "hp_after_world_by_seat": hp, "sasuke_history": [t for t in history if t],
        "owner_opponent_credit": opponent, "owner_ally_credit": 0,
        "received_damage_ledgers_by_seat": [opponent, self_hit], "sasuke_tier_after_world": int(parent_hits),
        "hp_writes_in_order": writes,
    }


def decode(report):
    lines = report.splitlines()
    if len(report) > 12288 or len(lines) != 123:
        raise ValueError("scene bomb reaction report extent")
    pos = 0

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"scene bomb reaction changed at row {pos+1}: {tag}")
        pos += 1

    cases = []
    for delay in (0, 10):
        for shapes in (1, 2):
            bodies, presses, placements = [], [], []
            for visit in sorted(set(BODY_VISITS) | set(B_VISITS)):
                if visit in B_VISITS:
                    rate = NEGATIVE if shapes == 2 and visit > 185 else ONE
                    take("area-reaction-b", (delay, shapes, visit, 2, 512, 9, rate))
                    presses.append({"visit_from_first_press": visit, "seat1_pressed_bits": 512,
                                    "stock_at_second_factory": 9, "secondary_rate_at_second_factory_bits": rate})
                    if visit in (185, 186):
                        placements.append(placement(take, delay, shapes, visit))
                if visit not in BODY_VISITS:
                    continue
                hit = shapes == 2 and visit >= 185
                gate = max(0, 189-visit) if hit else 0
                kind = 10 if hit else 0
                secondary = NEGATIVE if hit and visit > 185 else ONE
                multiplier = NEGATIVE if hit else ONE
                for seat in (0, 1):
                    take("area-reaction-body", (delay, shapes, visit, seat, 2, gate, gate, kind, 0, ONE, secondary, multiplier))
                bodies.append({
                    "visit_from_first_press": visit, "both_actors": {
                        "actor_subphase": 2, "reaction_class": kind, "pose_timer": 0,
                        "impact_sound_gate": gate, "impact_visual_gate": gate,
                        "primary_rate_bits": ONE, "secondary_rate_bits": secondary,
                        "secondary_multiplier_1dcc_bits": multiplier,
                    },
                })
            cases.append({
                "entry_hold_visits": delay, "shape_cells_offered": shapes, "retained_coordinator_visits": 235,
                "fresh_b_factory_forks": {"first_visit": 184, "last_visit": 235, "count": 52, "all_paid": True},
                "retained_body_samples": bodies, "fresh_b_sampled_forks": presses, "completed_second_placements": placements,
            })
    if pos != len(lines):
        raise ValueError("extra scene bomb reaction rows")
    return cases


def verify_source(image):
    area.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80054CD0, 0x80056900), (0x80054CD8, 0x80055568),
                         (0x80055598, 0x80059068), (0x80059118, 0x8005A378),
                         (0x8002BBE4, 0x8002CDA4), (0x8002F894, 0x8002FC18),
                         (0x8013055C, 0x8006DBE0), (0x80130580, 0x800E058C)):
        s.link(image, site, target)
    for addr, exact in ((PARENT, "001e00646464000004018001000002000060000000180000"),
                        (AREA, "001400646432000000000002000401000060000000200000")):
        if image.read(int(addr, 16), 24, "data") != bytes.fromhex(exact):
            raise ValueError("scene bomb reaction source amount changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("scene bomb reaction output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-scene-bomb-area.json", SCENE_AREA_SHA256)
    s.read_metadata(s.MAPS / "gg4e08-type41-parent-contact.json", PARENT_HP_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--world-reaction",), area.payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-scene-bomb-reaction", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "scene_bomb_area_sha256": SCENE_AREA_SHA256,
        "type41_parent_contact_sha256": PARENT_HP_SHA256, "actors_by_seat": ["gg4e08:000a", "gg4e08:0106"],
        "parent_amount_record": PARENT, "parent_raw_power": 30, "area_amount_record": AREA,
        "area_raw_power": 20, "area_pose_timer_seed": 0, "second_B_owner_seat": 1,
        "second_B_factory": "800E058C", "second_B_site": "80130580", "second_B_key": 2, "second_B_pose": 10,
        "cases": cases,
        "notes": [
            "Only the initial B is retained in the coordinator stream. Each second B is injected into a separate copy at the processed-pad boundary. The original stage inlet is initialized and empty.",
            "The two accepted area contacts write reaction class 10 and the secondary-rate multiplier on visit 185. The actual secondary rate changes on 186, independently of the zero pose timer, unit primary rate, and four-visit impact gates.",
            "The world can publish a second paid type-41 parent on the area-hit visit or on the following visit. The existing area can hit the newly published parent, which has 1 HP and does not add actor credit.",
            "Only one tested schedule and timing joins both sources against Sasuke on the same visit. Amount-entry samples show the new parent uses tier 0 and the old area uses tier 1; they retain distinct tokens.",
            "No floor construction, stage reach, general recipient order, guard policy, audio or drawing, or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "retained_coordinator_visits": 235*len(cases),
            "fresh_b_factory_forks": 52*len(cases), "completed_second_placements": 2*len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
