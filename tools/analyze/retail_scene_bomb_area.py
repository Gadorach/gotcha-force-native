"""Follow Time Bomber's natural terminal area through the original world scheduler."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_contact_effect_source_art as art
from tools.analyze import retail_scene_bomb_world as world
from tools.analyze import retail_type41_area_hp as area_hp
from tools.analyze import retail_type5_roster as area


s = world.s
WORLD_SHA256 = "e8ca2637daf58e28b2ed6ffb96efa25e88090c8a279264f243f607b77243d0a4"
AREA_HP_SHA256 = "cd8b2b0956d207a6db682152922eb45dd6203c4b9183aca176b4617ca3258e5a"
SOURCE_ART_SHA256 = "025c745717b4c40844049721831a40d402bc82ad31d07fddc451187efb8eb86e"
TEXT = (
    (0x80085E00, 296, "8e4e9e754e10d0a9aa506f32a6ddd8b707fa79eb6eda278760bd6bcf4a03d3e4"),
    (0x80086268, 80, "164154efe400bd3ddf82fb6ac13bf79fe2d85958bd6a2377a27a4e5ca616515b"),
    (0x800865EC, 172, "6ffb959e679f1a5e3960940a2d796565db0d59ec436163965fe3a5be44589da3"),
)
SHARED_VISITS = (1, 2, 180, 181, 182, 183)
ADMITTED_VISITS = (185, 198, 229, 230, 231)
RADIUS_BITS = {182: "42086666", 183: "42823334", 185: "42FE3333"}


def decode(report):
    probe = world.dual.scene.Report(report, 46)
    cases = []
    for delay in (0, 10):
        for shapes in (1, 2):
            admitted = shapes == 2
            probe.take("dual-world-pay", (delay, shapes, 9))
            samples = []
            for visit in SHARED_VISITS + (ADMITTED_VISITS if admitted else ()):
                if visit == 181:
                    probe.take("dual-area-bind", (delay, shapes, visit, shapes-1, 1))
                stock = 9 if visit < 182 else 10
                parent = visit <= 182
                child = 181 <= visit <= (230 if admitted else 182)
                parent_phase = 1 if visit <= 180 else 2 if visit == 181 else 3
                if admitted:
                    child_phase = 0 if visit < 181 else 1 if visit <= 228 else 2 if visit == 229 else 3
                else:
                    child_phase = 0 if visit < 181 else 2 if visit == 181 else 3
                bound = admitted and 181 <= visit <= 197
                published = bound and visit >= 182
                token = 2 if admitted and visit >= 181 else 0
                attacked = visit < 181 or published
                receivers = 19 if visit < 181 else 18
                landed = admitted and visit >= 185
                opponent_loss, self_loss, accepted = (17, 4, 2) if landed else (0, 0, 0)
                hp = [180-opponent_loss, 220-self_loss]
                hit = admitted and visit == 185
                radius = RADIUS_BITS[visit] if published else "00000000"
                probe.take("dual-area", (delay, shapes, visit, stock, int(parent), parent_phase, int(child),
                                         child_phase, int(published), int(attacked), receivers, token, *hp,
                                         2*int(hit), opponent_loss, 0, opponent_loss, self_loss,
                                         accepted, accepted, radius))
                contacts = []
                if hit:
                    for seat, address, loss in ((0, "81020000", 17), (1, "81021E00", 4)):
                        probe.take("dual-area-hp", (delay, shapes, visit, seat, address, loss))
                        contacts.append({"receiver_seat": seat, "relation": ("opponent", "self")[seat],
                                         "requested_hp": loss, "hp_removed": loss, "accepted_token": accepted})
                samples.append({
                    "visit_from_press": visit, "owner_stock": stock, "parent_active": parent,
                    "parent_phase_field": parent_phase, "area_active": child,
                    "area_phase_field": child_phase if visit >= 181 else None, "area_shape_bound": bound,
                    "area_shape_published": published, "area_radius_bits": radius if published else None,
                    "attack_shapes": int(attacked), "ordinary_receivers": receivers,
                    "recorded_area_token": token or None, "hp_by_seat": hp,
                    "owner_opponent_damage_credit": opponent_loss, "owner_ally_damage_credit": 0,
                    "received_damage_ledgers_by_seat": [opponent_loss, self_loss], "hp_writes_in_order": contacts,
                })
            cases.append({
                "entry_hold_visits": delay, "press_in_entry_visit": 175+delay, "kind3_cells_offered": 2,
                "shape_cells_offered": shapes, "stock_at_parent_factory": 9, "parent_token": 1,
                "area_initializer_visit": 181, "parent_shape_active_and_marked_at_area_initializer": True,
                "free_shape_cells_at_area_initializer": shapes-1, "parent_shape_return_visit": 181,
                "parent_stock_return_visit": 182, "parent_carrier_return_visit": 183,
                "area_first_publish_visit": 182 if admitted else None, "area_first_hit_visit": 185 if admitted else None,
                "area_shape_return_visit": 198 if admitted else None, "area_carrier_return_visit": 231 if admitted else 183,
                "completed_coordinator_visits": 231 if admitted else 183, "samples": samples,
            })
    return cases


def verify_source(image):
    world.verify_source(image)
    art.verify_source(image)
    s.verify_text(image, (*area_hp.TEXT, *TEXT))
    for site, target in ((0x800E0B3C, 0x800E0D7C), (0x80085E38, 0x80088AA0),
                         (0x800860E0, 0x8008AE3C), (0x8002DDA8, 0x8002E2A8),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        s.link(image, site, target)


def payload(image, afs):
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        common = area.common_bank(s.RetailArchive(stream))
        if s.archive_stamp(stream) != stamp:
            raise ValueError("scene bomb area common bank input changed")
    rows, groups = area.area_rows(image, common)
    key = rows[19]
    group = next(g for g in groups if g["group"] == key["common_group"])
    if ((key["common_group"], key["visual_record"], key["hold_operand"]) != (179, 22, 4)
            or (group["descriptor"], group["reaction_index"], group["reaction_record"], group["raw_power"])
            != (246, 26, "802C49D0", 20)):
        raise ValueError("scene bomb area key-19 source descriptor changed")
    view = next(raw for slot, raw in art.source_blocks(afs) if slot == 1)
    return world.dual.payload(afs) + common + view


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("scene bomb area output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    for name, digest in (("scene-bomb-world", WORLD_SHA256), ("type41-area-hp", AREA_HP_SHA256),
                         ("contact-effect-source-art", SOURCE_ART_SHA256)):
        s.read_metadata(s.MAPS / f"gg4e08-{name}.json", digest)
    cases = decode(s.fixture_report(fixture, dol, module, ("--world-area",), payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-scene-bomb-area", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "scene_bomb_world_sha256": WORLD_SHA256,
        "type41_area_hp_sha256": AREA_HP_SHA256, "contact_effect_source_art_sha256": SOURCE_ART_SHA256,
        "actors_by_seat": ["gg4e08:000a", "gg4e08:0106"], "credit_owner_seat": 1,
        "parent_type": 41, "parent_key": 2, "area_type": 5, "area_key": 19,
        "common_collision": {"member": area.COMMON_MEMBER, "block": 0, "sha256": area.area.COMMON_COLLISION_SHA,
                             "group": 179, "descriptor": 246, "amount_index": 26,
                             "amount_record": "802C49D0", "raw_power": 20},
        "common_view": {"member": art.art.EFFECT_MEMBER, "block": 1, "sha256": art.art.EFFECT_BLOCKS[1][1],
                        "opening_record": 22}, "cases": cases,
        "notes": [
            "Two separate cold-start actor archives and a normal common effect archive use original delivery callbacks with exact bytes supplied at the disc boundary.",
            "The stage is a declared initialized empty grid with original stage state 1. From the first active press onward, each retained coordinator call completes.",
            "The natural parent timeout takes the original key-19 branch. At the child initializer the parent shape is still live in its cell and marked for close; its later release is too late for the one-shape case.",
            "With two shape cells the child binds at expiry, publishes attack-only on the next visit, then writes both recipients on one visit. Credit remains with the loaded Time Bomber after the parent carrier returns.",
            "The self hit contributes to the owner's received ledger but not his ally- or opponent-credit ledger. The opponent hit contributes 17 to both the owner's opponent ledger and Sasuke's received ledger.",
            "No real arena, other actor ordering, disc I/O, physical pad reads, GX rendering, audible playback, original hardware, or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "coordinator_visits": sum(case["completed_coordinator_visits"] for case in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
