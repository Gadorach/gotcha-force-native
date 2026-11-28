"""Distinguish pose-adjusted static contact from logical grounding and refill."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_dual_scene_entry as dual, retail_st42_wall as st42
from tools.extract.retail_borg_roster import levels, source_table


s = dual.s
MODELS = (
    (0x0900, 634, "4c6e3c73caf7640ca7b119644c96dd8fcc44304717dcaf1509d367e8f5902f16", (
        (0, 432, "409d8ba4bb1c7d8c173f9700350f432a4110c2b12ec410798e615e54d9883c88"),
        (2, 7840, "7c622003c5307b775dcd853c4fa2d1ee0e6129a630e4335f47f53fd61be59602"),
        (3, 204096, "d8b37e7340f2180bdb0bc8f654a5eca09dec14dc6b804d9b8bf1e4bf2c00bcc8"),
        (4, 214208, "c8cd0b42ef7972efb4424c70c7a0a63dd1665027a1b6a38512475da5fb737c08"),
    )),
    (0x0A00, 647, "45a0b7b081de79ed0a9b66941d2a063726ce42924cb2b1cc34abe58644d9988e", (
        (0, 432, "b4686659ec3fba8f54d0d194ac4bbc40e9edd554095da6cfc648404804481154"),
        (2, 7840, "d6ab3faad24ceacabb225c2a22a623c19009f7e1571190b7a818f7b5e7391aad"),
        (3, 398528, "6d8269eccdd7f7521d685f7271c025b351f73830008ca380c05223b152f0fc71"),
        (4, 214868, "2c56181c0800a4d319927bc4ffabef888d5369152f981c4bea6bc9de5e24c137"),
    )),
)
DUAL_SHA256 = "7caed283865bedafac11776243ef37f7800aa698e369b78ab87cacaa4a27295b"
TEXT = (
    (0x8004D08C, 360, "ba2fb11f03cbfae5126e2f32f33bae5fe2b01840191bd77d8e7348a507ae97e7"),
    (0x80055DCC, 280, "7f60ab140ba8c7261c2ad34d87e1f41962e0c870e435a5c0efc12f216e6fb4a2"),
    (0x8005568C, 1856, "0f964164be0f7581e45a26f8acd1113b414d13ecdbefeb94be72acde53c156ac"),
    (0x80060228, 340, "6aea155fb310e59309a9ded7745296119e2933376552ba2331cdcd0973b289cf"),
    (0x80065E64, 76, "639d986cce7856b16e4cc71c6a88e2b20df50b7561a816f3e90805d96f37281f"),
    (0x800677B0, 544, "09ab80886742547a9862056d729a3b23984007549f8cd636ea2d5cf3e63bf246"),
    (0x8003E0A8, 844, "4310809c29ad48750521ad16c4e2f47192b2c69b1d3dc357d613870af21d1708"),
)
ROW = re.compile(r"hover-[a-z-]+:(?: [0-9]{1,3}){5}(?: [0-9A-F]{8}){4}(?: [0-9]{1,5}){3} [0-9A-F]{8}")


def verify_source(image):
    dual.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80055ED0, 0x8004D08C), (0x800677F0, 0x8003E0A8), (0x80065E90, 0x8006A750)):
        s.link(image, site, target)
    for model, index, *_ in MODELS:
        if image.read(source_table(image, 0x802D0584, model), 4) != struct.pack(">I", index) or levels(image, model)[0][0] != 150:
            raise ValueError("hover-floor control identity changed")


def payload(afs):
    chunks = []
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        sources = s.RetailArchive(stream)
        for model, index, member_hash, roles in MODELS:
            member, digest = sources.member(index, model)
            if digest != member_hash or member.entries[1]["size"]:
                raise ValueError("hover-floor control archive changed")
            for role, size, digest in roles:
                data = member.read(role, size)[0]
                if len(data) != size or sha256(data).hexdigest() != digest:
                    raise ValueError("hover-floor control role changed")
                chunks.append(data)
        if stamp != s.archive_stamp(stream):
            raise ValueError("hover-floor archive changed during extraction")
    return b"".join(chunks)+st42.source_inputs(afs)[-sum(row[0] for row in st42.GRIDS):]


def scalar(bits):
    return struct.unpack(">f", bytes.fromhex(bits))[0]


def decode(text):
    if len(text) > 4096 or len(text.splitlines()) != 28:
        raise ValueError("hover-floor report extent")
    lines = iter(text.splitlines())
    results = []
    for seat, (lift, fall, touch, idle) in enumerate(((13, 24, 77, 95), (17, 47, 71, 116))):
        samples = {}

        def take(edge, visit):
            row = next(lines)
            if not ROW.fullmatch(row) or not row.startswith(f"hover-{edge}: {seat} {visit} "):
                raise ValueError(f"hover-floor edge changed: {seat}/{edge}")
            _, _, _, sel, cache, mode, flags, y, offset, pose, meter, count, phase, cont = row.split()
            if mode != "0" or scalar(pose) != 24.*seat or (seat and scalar(offset) != 0.):
                raise ValueError("pose height was confused with the logical contact offset")
            samples[edge] = {"visit_after_opening": visit, "selector": int(sel), "airborne_cache": int(cache),
                             "flags_hex": flags, "root_y": scalar(y), "contact_offset": scalar(offset),
                             "class_pose_y": scalar(pose), "meter": int(meter), "points_used": int(count),
                             "lower_phase": int(phase), "continuation": cont}
            return samples[edge]

        for edge, visit in (("start", 0), ("takeoff", 1), ("lift", lift), ("seed", lift+1),
                            ("in-air", lift+1), ("fall", fall), ("before-contact", touch-1)):
            take(edge, visit)
        snap = next(lines)
        if snap != (f"hover-snap: {seat} {touch} 41200000 "
                    + ("41200000 00000000" if seat else "BDB64C00 41216C98")):
            raise ValueError("hover-floor callback or static projection changed")
        floor, y, offset = map(scalar, snap.split()[-3:])
        contact = take("contact", touch)
        take("landing-fork", touch+1)
        take("refill-fork", touch+2)
        take("landing", touch+1)
        take("after-landing", touch+2)
        take("neutral", idle)
        def account(edge):
            r = samples[edge]
            return r["selector"], r["airborne_cache"], r["meter"], r["points_used"]

        if (account("takeoff") != (4, 0, 10000, 0) or account("lift") != (5, 1, 10000, 1)
                or account("seed") != (5, 1, 4000, 1) or account("in-air") != (5, 1, 6000 if seat else 4000, 1)
                or samples["in-air"]["root_y"] <= floor or samples["fall"]["continuation"] != "80065E64"
                or account("contact") != (7, 1, 10000 if seat else 4000, 1) or contact["continuation"] != "00000000"
                or (contact["root_y"], contact["contact_offset"]) != (y, offset) or y+offset != floor
                or account("landing-fork") != (7, 0, 6000 if seat else 4000, 1)
                or account("refill-fork") != (7, 0, 8000 if seat else 6000, seat)
                or samples["landing"]["root_y"] != floor or samples["landing"]["contact_offset"] != 0
                or account("neutral") != (0, 0, 10000, seat)):
            raise ValueError("hover-floor contact, logical ground, and resource ordering changed")
        results.append({"borg": f"gg4e08:{MODELS[seat][0]:04x}", "member": MODELS[seat][1],
                        "member_sha256": MODELS[seat][2], "movement_type": seat*2, "resource_policy": seat*2,
                        "floor_callback": {"visit_after_opening": touch, "floor_y": floor, "root_y": y,
                                           "contact_offset": offset, "root_y_plus_contact_offset": y+offset,
                                           "saved_support_owner": False},
                        "edges": samples})
    return results


def native_report(executable):
    p = run([Path(executable).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    expected = "hover-native-resource: 4000 1 6000 0 6000 1 8000 1 4000\nhover-native-authored: 10 10 0\n"
    if p.returncode or p.stderr or p.stdout != expected:
        raise ValueError("native floor or resource contrast changed")
    return {"resource_forks_matched": True, "type2_inhibited_meter": 4000,
            "authored_floor_y_with_or_without_visual_offset": 10,
            "authored_combined_budget_count_at_touchdown": 0}


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("hover-floor output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS/"gg4e08-dual-scene-entry.json", DUAL_SHA256)
    text = s.fixture_report(fixture, dol, module, ("--hover-floor",), payload(afs))
    raw = s.encode({
        "kind": "gf-retail-pose-offset-contact", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "source_stage_member": st42.MEMBER,
        "source_stage_member_sha256": st42.MEMBER_SHA256, "dual_scene_entry_sha256": DUAL_SHA256,
        "source_trace_sha256": sha256(text.encode("ascii")).hexdigest(), "opening_visit": 174,
        "source_cases": decode(text), "native_contrast": native_report(native),
        "notes": [
            "Two loaded normal actors open on relocated st42 grids. A declared processed jump packet is held for 18 actor visits after opening on the subject seat only. Actual takeoff, fall, static contact, landing and neutral run before the scene world tail.",
            "Meter 4000 is a declared observation sentinel after the first ascent entry, not a measured debit. A second fork reseeds it before landing entry to compare the recovery gates with contact and airborne history already produced by the original code.",
            "The callback inlet and height/normal query prove a static snap. The Nurse root is below the selected plane because the produced positive 6D0 offset is subtracted. Wing's positive 1DC0 class pose output does not move its logical contact plane.",
            "No above-floor logical grounding, normal flight-meter expenditure for the Nurse, moving-support reacquisition, hardware input, rendering, or universal hover-floor policy is established. Native resource arithmetic matches the supplied gates; the running authored control is not a matched source actor.",
        ],
    }, expanded=("source_cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as file:
        file.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": len(text.splitlines())}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        cli.add_argument(name, type=Path)
    args = cli.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
