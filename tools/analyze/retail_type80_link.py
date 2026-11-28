"""Verify the Carrier Helicopter type-80 request, link and ownership boundaries."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_type25_link as tether


s = tether.s
TYPE25_SHA256 = "799f18c5621fb1a3ede60cba8db43a2f3969931b8a8db566554b206f6b65cd40"
MODEL = 0x0D02
RECORD = 0x80338F60
HIT_SHA256 = "8a1cf28147c75aa970db3d08c08aba0a441ce8717d14cb0d0012e2c7c1401e3d"
ACTOR_SHA256 = "58a71dd618a45838a4a691cd6091d15bea8c17edce0e24738808516bbfe29271"
TEXT = (
    (0x80135EE8, 224, "2218f4174919722987e4ff6deecc6980043d4faacc5d729ada50654bb2baa393"),
    (0x801367F0, 84, "062b7470080a99622a7e0dafafa2f26c889aab98847549fa0adc6f06db44346b"),
    (0x800FD3C0, 268, "f8bb74a7dd83eb2826ab17da23aafbf67ad14c2482ccef7684ef7512b8c8ae70"),
    (0x800FD4CC, 152, "a20b6bbfbfd8ee33b8d436f3ec8dc9527f7d8b084dec00f2ff2b27512e335ec5"),
    (0x800FD564, 584, "9af2a0837ef3be979c08e9ecf3125ec5d4d8a60847cf76d49f3436e4f718238f"),
    (0x800FD7AC, 188, "4bfd11dd1e6a28d19f3a6c7626488da0d68b831d41da9a2e2c254d54c6a9e510"),
    (0x800FD868, 936, "1d73869e16d5ccfd5b0cfe41f8a51b9658f416b0176637e3776bdbf685e26831"),
    (0x800FDC10, 1776, "23550afff2d9c89a9a52231246adfc8c1f56cffcc32246ded791bed1e1e20954"),
    (0x800FE4B8, 296, "8ef5c6385dcc5adfd4380c9b825535a8101e24377a16a4938844f44d510a5561"),
    (0x800FE5E0, 12, "e84eeb834936daad4d3bbdaff4f1d2bedaaaf1739dd55c7761e90b0b31092e08"),
    (0x800FE5EC, 32, "790bc373805903ab8c50a20a4d7a8effd27a96d58d9f1f6971fdeeea5bd5c794"),
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
)
DATA = (
    (0x8031EEE8, 36, "a466c235cd8ed9d04d0095a21b57b453448750cd634b4f5cf01cb5f170845284"),
    (0x8031EF0C, 28, "edd5ee6f626d3353f54da2fb470d9dd4749985fbc328940854b7268f403e7158"),
    (RECORD, 24, "1071d30bc57f95ddf22aa695b812c77c931af8c33a7e3918562e640951cf8120"),
)
CASES = tether.CASES + ("carrier-pool-empty", "view-pool-empty", "shape-pool-empty")


def verify_source(image):
    tether.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80136824, 0x800FD3C0), (0x80136830, 0x8006A53C), (0x800FD3E4, 0x80088AA0),
                         (0x800FD6C8, 0x8008AC80), (0x800FD804, 0x800FD868), (0x800FD80C, 0x800FDC10),
                         (0x800FDE08, 0x800FE4B8), (0x800FE5F8, 0x80088E50)):
        s.link(image, site, target)
    for address, size, digest in DATA:
        if sha256(image.read(address, size, "data")).hexdigest() != digest:
            raise ValueError("type80 source data changed")
    if (image.read(0x80338C68, 4, "data") != bytes.fromhex("801367F0")
            or image.read(0x8031EF28, 16, "data") != bytes.fromhex("800FD564800FD7AC800FE5E0800FE5EC")
            or tether.levels(image, MODEL)[0][0] != 200):
        raise ValueError("type80 handler, carrier table or normal HP changed")


def source_inputs(afs):
    contact = tether.contact
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        hit, owner = contact.read_normal(archive, MODEL, HIT_SHA256, ACTOR_SHA256)
        body, receiver = contact.read_normal(archive, contact.RECEIVER, contact.RECEIVER_BLOCK_SHA256,
                                            contact.ACTOR_SHA256[contact.RECEIVER])
        if s.archive_stamp(stream) != stamp:
            raise ValueError("type80 source archive changed")
    step = 0xAA0+2*80
    if (hit[0xA28:0xA2C] != bytes.fromhex("02FFFFFF")
            or hit[step:step+12] != bytes.fromhex("0000000100020000FFFF0000")
            or struct.unpack_from(">4f", hit, step+12) != (0, 0, 0, 15)):
        raise ValueError("type80 original attack descriptor changed")
    return hit + owner + body + receiver


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 21:
        raise ValueError("type80 link report extent")
    pos = 0
    rows = []

    def take(tag, fields, name):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"type80 link changed: {name}/{tag}")
        pos += 1

    for i, name in enumerate(CASES):
        pair = None
        checkpoints = []
        contact = i < 3
        hit = contact and i != 0
        if contact:
            record = f"{RECORD:08X}" if hit else None
            take("type80-pair", (i, 2, record or "00000000", int(hit), 0, 16*hit, 180, int(hit), int(hit), 1), name)
            pair = {"amount_record": record, "hp_writer_reached": hit, "hp_after": 180, "requested_damage": 0,
                    "hp_removed": 0, "carrier_contact_latch": 16*hit, "accepted_token": 1 if hit else None}
        for v in range((2, 9, 2, 1, 1, 3)[i]):
            acquired = i == 1 and v > 0
            masks_e = [2, 1] if acquired and v < 4 else [0, 0]
            masks_f = [2, 1] if i == 1 and v == 3 else [0, 0]
            timer = 601-v if acquired and v < 4 else 0
            outer = 1 if contact else 2 if i == 5 and v == 0 else 3 if i == 5 else 0
            inner = (1 if v < 4 else 2) if acquired else 2 if i == 2 and v == 1 else 0
            busy = int(contact)
            retains = int(hit and v > 0)
            attacks = int(contact and (v == 0 or not hit))
            free_shape = int(contact and not attacks)
            requests = int(i == 1 and v >= 3)
            live = int(contact or (i == 5 and v < 2))
            free_carrier = int(i == 4 or (i == 5 and v == 2))
            free_view = int(i == 3 or (i == 5 and v == 2))
            take("type80-link", (i, v, outer, inner, timer, busy, int(acquired), *masks_e, *masks_f,
                 int(acquired), retains, attacks, free_shape, 180 if contact else 0,
                 int(hit), requests, requests, live, free_carrier, free_view, live), f"{name}/{v}")
            step = (tether.VISITS[v] if v < 5 else "post-expiry-bucket-visit") if contact else (
                ("shape-refusal", "retirement-armed", "resources-returned")[v] if i == 5 else "allocation-refused")
            checkpoints.append({
                "checkpoint": step, "index": v, "carrier_phase": outer, "carrier_link_subphase": inner,
                "timer": timer, "owner_busy_14a": bool(busy), "owner_retains_target_144": acquired,
                "masks_71e_owner_target": masks_e, "masks_71f_owner_target": masks_f,
                "target_status_719": int(acquired), "carrier_retains_target": bool(retains),
                "ordinary_attack_published": bool(attacks), "shape_free_count": free_shape,
                "motion_request_counts_owner_target": [requests, requests], "carrier_live": bool(live),
                "free_carrier_count": free_carrier, "free_view_count": free_view, "carrier_remains_in_bucket": bool(live),
            })
        rows.append({"case": name, "pair": pair, "checkpoints": checkpoints})
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type80 link output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-type25-link.json", TYPE25_SHA256)
    rows = decode(s.fixture_report(fixture, dol, module, ("--type80",), source_inputs(afs)))
    raw = s.encode({
        "kind": "gf-retail-type80-link", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "type25_link_sha256": TYPE25_SHA256,
        "owner": "gg4e08:0d02", "receiver": "gg4e08:000a", "key": 0,
        "launch_socket": 1, "opening_group": 2, "opening_descriptor": 2, "opening_radius": 15,
        "request_handler": "801367F0", "factory": "800FD3C0", "carrier_callback": "800FD4CC",
        "busy_wait_frontier": {"entry": "8006A53C", "call_site": "80136830", "r4": 10},
        "declared_stretch_x": 600, "declared_stretch_both_204": 100, "declared_expiry_timer": 1,
        "cases": rows,
        "notes": [
            "Runs the original Carrier Helicopter request handler directly, not upstream charged-X selection. The fixture declares poses, free lists and empty stage and render inputs.",
            "A busy owner takes the wait frontier both after allocation and after expiry plus four bucket visits. Carrier or view refusal clears busy without consuming the other resource. Shape refusal also clears busy; two bucket visits return the original carrier and view.",
            "Accepted group-2 contact completes the original HP writer without resuming its caller. It records token 1 and a zero request, not damage. The next carrier callback stores the receiver on both owner and carrier, closes the attack, and sets reciprocal +71E.",
            "Holding the pose leaves +71F clear. Declared stretch sets +71F and requests motion on both actors. Expiry clears both masks before the owner releases busy or either target pointer; four bounded follow-ups retain those fields.",
            "An excluded receiver after contact reaches link subphase 2 without establishing the link or publishing the owner pointer. The carrier retaining a target is not proof that the owner got one.",
            "The replay does not advance actors, qualify full accepted-contact retraction, or integrate wall correction during the tether. No live timing, hardware, playback or native behavior is inferred.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "checkpoints": sum(len(r["checkpoints"]) for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
