"""Verify original type-25 contact, reciprocal mask publication and expiry."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_linked_correction as link
from tools.analyze import retail_worm_contact as contact
from tools.extract.retail_borg_roster import levels


s = link.s
LINK_SHA256 = "03927e9871db437615e0c6988d8e07e259bc08bd298767d193b5a3807e0e3e58"
PARAM_SHA256 = "8ed56f8f4d2bdc351b974ed4e2d08c4ed68221411ff187c6c5df86a58af5f386"
TEXT = (
    (0x800C9000, 356, "273d77e540d410ef90e43c415953df2670ea7c3c432393f68ac8d34585183690"),
    (0x801565CC, 292, "0ab14996dd74f914cc4a6ec73a600e79b2611d2533aedca038103a0b64dd01a1"),
    (0x800C4704, 308, "3b2a4aa7926c20be10124bf2020dd5ebfae113586d7139f21fbfff3f0236308e"),
    (0x800C4838, 152, "81e51ab93f431a14546b1d0f3887705c7c3bab2409934b526d578af40dcb5ede"),
    (0x800C48D0, 632, "3c7f23b809f7472ba44c50f86be15aae6922fc743bb141a61a8810b09c895bd0"),
    (0x800C4B48, 92, "8adad62c2c023bbace5dfd622f7d6de7c8b8e46af702ef8b2cb8339645bb6c62"),
    (0x800C4BA4, 1132, "b3fac39a4c04c27ce536ae253baf29bfd5893c8a1077aedf08be527bbdf8dd7e"),
    (0x800C5010, 1824, "d8c0058718f8ee1c3cdaabfb6a58f59e34a881413e9669fbe69a0b666e9696cd"),
    (0x800C590C, 296, "a856f9852bcbc841e6fca19389992d096b07276fafb34b6863a006df6b625a3d"),
    (0x80084498, 244, "f615da96a5c0bdd156ae57e7d5d27aa95a0f7ba9a45fe000f8fe3cc4d1c9fcdd"),
)
# Model, key, socket, descriptor, local center/core, base HP, amount record,
# static model caller and exact factory call site.
ROUTES = (
    (0x0201, 0, 11, 25, (30, 0, 0, 15), 250, 0x80309EB0, 0x800C9000, 0x800C90B8,
     "9e9e50bbaf590a7ff35e1e04718fffe4bbac36879ede38be1cd1589b82c63e89",
     "3c3db2babc5247c3ea2ecd78a6ce5cc37b7148a720674493cdd92636b53d5fb9"),
    (0x0809, 1, 12, 0, (0, 25, 0, 25), 50, 0x80342910, 0x801565CC, 0x801566BC,
     "8b3f9ab407ef99c2e8dd2d9473d7b1a97fc2f76d849d1bd004190648ebb5a709",
     "45c368f43235291739ba96d34791fd6c3318fa17828e727b89946f6a8d7ccbb8"),
)
CASES = ("miss", "accepted-stretch-and-expiry", "target-excluded-after-hit")
VISITS = ("after-pair", "acquisition", "unmoved", "declared-stretch", "declared-expiry")


def verify_source(image):
    link.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x800C90B8, 0x800C4704), (0x801566BC, 0x800C4704), (0x800C4738, 0x80088AA0),
                         (0x800C4A64, 0x8008AC80), (0x800C4B80, 0x800C4BA4), (0x800C4B88, 0x800C5010),
                         (0x800C521C, 0x800C590C), (0x800C5594, 0x80084498), (0x800C55A0, 0x80084498)):
        s.link(image, site, target)
    for address, value in ((0x80305240, 0x800C48D0), (0x80305244, 0x800C4B48), (0x80309C44, 0x800C9000),
                           (0x80342680, 0x801565CC), (0x80438784, 0x43E10000)):
        if image.read(address, 4, "data") != value.to_bytes(4, "big"):
            raise ValueError("type25 pointer or length constant changed")
    params = image.read(0x80305200, 64, "data")
    if sha256(params).hexdigest() != PARAM_SHA256:
        raise ValueError("type25 source parameter rows changed")
    for model, key, socket, _, _, hp, amount, *_ in ROUTES:
        if params[32*key:32*key+2] != bytes((0, socket)) or levels(image, model)[0][0] != hp:
            raise ValueError("type25 group, socket or normal HP changed")
        if image.read(amount, 24, "data")[:4] != b"\x00"*4:
            raise ValueError("type25 raw amount changed")


def source_inputs(afs):
    payload = []
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        for model, _, _, descriptor, geometry, *_, hit_hash, spec_hash in ROUTES:
            hit, spec = contact.read_normal(archive, model, hit_hash, spec_hash)
            step = 0xAA0+80*descriptor
            if (hit[0xA20:0xA24] != bytes((descriptor, 255, 255, 255))
                    or hit[step:step+12] != bytes.fromhex("0000000100000000FFFF0000")
                    or struct.unpack_from(">4f", hit, step+12) != geometry):
                raise ValueError("type25 source attack descriptor changed")
            payload.extend((hit, spec))
        payload.extend(contact.read_normal(archive, contact.RECEIVER, contact.RECEIVER_BLOCK_SHA256,
                                           contact.ACTOR_SHA256[contact.RECEIVER]))
        if s.archive_stamp(stream) != stamp:
            raise ValueError("type25 source archive changed")
    return b"".join(payload)


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 24:
        raise ValueError("type25 link report extent")
    pos = 0
    pilots = []

    def take(tag, fields, name):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"type25 link changed: {name}/{tag}")
        pos += 1

    for model, key, socket, descriptor, geometry, hp, record, caller, call_site, *_ in ROUTES:
        cases = []
        for j, name in enumerate(CASES):
            i = 3*key+j
            hits = int(j != 0)
            amount = f"{record:08X}" if hits else "00000000"
            take("type25-pair", (i, descriptor, amount, hits, 0, hits*16, 180, hits, hits, 1), name)
            history = []
            for v in range(5 if j == 1 else 2):
                acquired = j == 1 and v > 0
                retracting = v == 4 or (j == 2 and v == 1)
                linked = acquired and not retracting
                pulled = v == 3
                mask_e = [2, 1] if linked else [0, 0]
                mask_f = [2, 1] if pulled else [0, 0]
                timer = 601-v if linked else 0
                owner_state, phase = (3, 2) if retracting else (2, 1) if linked else (1, 0)
                attacks = bool(not v or not hits)
                request_count = int(v >= 3)
                retained_target = bool(v and hits)
                take("type25-link", (i, v, 1, phase, timer, owner_state, *mask_e, *mask_f, int(acquired),
                     int(retained_target), int(attacks), int(not attacks), 180, hits, request_count, request_count, 1),
                     f"{name}/{VISITS[v]}")
                history.append({
                    "checkpoint": VISITS[v], "carrier_phase": 1, "carrier_link_subphase": phase, "timer": timer,
                    "owner_status_144": owner_state, "masks_71e_owner_target": mask_e, "masks_71f_owner_target": mask_f,
                    "target_status_719": int(acquired), "retains_target_pointer": retained_target,
                    "ordinary_attack_published": attacks, "shape_free_count": int(not attacks),
                    "motion_request_counts_owner_target": [request_count, request_count], "carrier_live": True,
                })
            cases.append({
                "case": name, "amount_record": amount if hits else None, "hp_writer_reached": bool(hits),
                "requested_damage": 0, "hp_removed": 0, "target_hp_after": 180,
                "carrier_contact_latch": 16*hits, "accepted_token": hits or None, "checkpoints": history,
            })
        pilots.append({
            "borg": f"gg4e08:{model:04x}", "key": key, "base_hp": hp, "launch_socket": socket,
            "opening_group": 0, "shape_descriptor": descriptor, "shape_local_center": list(geometry[:3]),
            "shape_radius": geometry[3], "static_request_handler": f"{caller:08X}",
            "static_factory_call_site": f"{call_site:08X}", "cases": cases,
        })
    return pilots


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type25 link output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-linked-correction.json", LINK_SHA256)
    pilots = decode(s.fixture_report(fixture, dol, module, payload=source_inputs(afs)))
    raw = s.encode({
        "kind": "gf-retail-type25-link", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "linked_correction_sha256": LINK_SHA256, "receiver": "gg4e08:000a",
        "factory": "800C4704", "carrier_callback": "800C4838", "link_callback": "800C4BA4",
        "active_tether_callback": "800C5010", "retract_helper": "800C590C",
        "declared_stretch_x": 600, "declared_stretch_both_204": 100, "declared_expiry_timer": 1,
        "pilots": pilots,
        "notes": [
            "Static model callers are source-pinned. Original execution starts at the exact carrier factory with a declared owner pending byte, not the action handler, trigger or debit.",
            "The factory allocates the carrier; its original callback binds an unedited group-0 attack. A normal Sasuke sphere is bound and published by original code. The HP writer completes on the accepted zero-damage contact; its caller is not resumed.",
            "A hit alone does not publish either mask. The next carrier callback retains the receiver, closes the attack, and writes reciprocal +71E. The first unchanged pose keeps +71F clear.",
            "With the declared stretched pose, the same callback requests motion on both actors and sets both +71F bits while restoring +71E. This establishes the mask producer, not a replay of wall transfer during a live tether.",
            "An excluded target after the pair and a miss both leave the masks clear, with different carrier subphases and shape ownership. Setting the captured timer to one demonstrates the expiry edge: both mask pairs clear when the carrier starts retracting.",
            "Motion requests are not integrated. Full retraction, stock return, carrier retirement, type 80, live stage scheduling, graphics, audio playback and native gameplay are outside this receipt.",
        ],
    }, expanded=("pilots",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": sum(len(p["cases"]) for p in pilots),
            "carrier_checkpoints": sum(len(c["checkpoints"]) for p in pilots for c in p["cases"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
