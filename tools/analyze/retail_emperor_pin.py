"""Trace normal Galactic Emperor class placement and the ordinary airborne pin."""
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
MODEL, MEMBER, TABLE = 0x0E04, 684, 0x802DE380
MEMBER_SHA256 = "700d4f89a1d97cd2be5f06ba93dd342d8eeb05bda4b3bad3fa0f8ec9685ef69c"
ROLES = (
    (0, 432, "9ca406b17435763e565eadf3cac9220d985cc6885b3031b5a9ae404f76ddd18d"),
    (2, 7840, "fe5c9ab0b133916092da0a623cfd08b6dade1d043a917966444bf4ee59cb7d29"),
    (3, 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
    (4, 174048, "cc3e18bbbe37e9e14a3e33eda33c1ce148363693cf3ef104f189b607079e9ba2"),
)
DUAL_SHA256 = "7caed283865bedafac11776243ef37f7800aa698e369b78ab87cacaa4a27295b"
TEXT = (
    (0x800541AC, 1456, "b4d201a64a05727912a7435ecce9280e7f9a22075fae0afa2e59b9470f211748"),
    (0x801D626C, 224, "f1b503d51a49631c7000a99f87367fa132b297e83488b762acd5560a9d77a70e"),
    (0x801D634C, 328, "2e77b448a246408f76d43e9a85a72066584c74eb9ddd240bec0cb26595d60539"),
    (0x801D6494, 224, "aebfb968af16bcb0f11a4d2c77fce6e64b084941352137571e0f6a4b46e27f4f"),
    (0x801D6574, 252, "7820ef74ff262cab165032ceb486f9a77607f447b3241ca545fd49cce3dcfc17"),
    (0x80056D28, 2316, "36de41a414aaf8d77ca96619d220732588ccc2207d854bdf64d72b94720ec59c"),
    (0x800579DC, 244, "8c72ba8b33c42ba3dc5b73941cc8497bd236e838e0663d916269eb1679a3b46a"),
    (0x8003F044, 124, "d61dad1cc92099585b159223478f4cb9d531219f1f0c06c95cf64b6ddbd52a35"),
    (0x80037B98, 904, "a64a2e34210881d705ee9a2e8d5ef7ec682befc220dd909d181e38cdbd6e8610"),
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    (0x80056900, 424, "52c4c7e19500682352bbf4503d8973d50f69fed24c5525d5b3ab40bba34bbfa2"),
    (0x8006A6FC, 84, "4efe53e520303e6ad0cf238f8a53d503a8f6ad394e161cc93649b7484b847b0c"),
    (0x8006A750, 100, "0512ba29e89a10d5dc9786ce20496633ed39f7b752b7c0313fa5afa0673e7233"),
)


def verify_source(image):
    dual.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x800544A8, 0x8008AB30), (0x800544D4, 0x80056D28),
                         (0x8005748C, 0x800579DC), (0x80057558, 0x8003F044),
                         (0x800575A8, 0x80037B98), (0x80055670, 0x8005B880),
                         (0x801D63D8, 0x801FB398), (0x801D63EC, 0x801FD114)):
        s.link(image, site, target)
    for base, expected in ((0x802D0584, MEMBER), (0x802D3224, 0x801D626C)):
        if image.read(source_table(image, base, MODEL), 4, "data") != struct.pack(">I", expected):
            raise ValueError("Emperor member or installer mapping changed")
    if (levels(image, MODEL)[0] != (2000, 0, 0, 1, 300, 0, 0, 0, 300)
            or struct.unpack(">3f", image.read(TABLE+8*12, 12, "data")) != (-3648., 1985., -3800.)):
        raise ValueError("Emperor level or stage code 2 placement row changed")


def payload(afs):
    chunks = []
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        sources = s.RetailArchive(stream)
        for model, index, digest, roles in ((MODEL, MEMBER, MEMBER_SHA256, ROLES), dual.MODELS[1]):
            member, seen = sources.member(index, model)
            if seen != digest or member.entries[1]["size"]:
                raise ValueError("Emperor scene source archive identity changed")
            for role, size, digest in roles:
                data = member.read(role, max(size, 1))[0]
                if len(data) != size or sha256(data).hexdigest() != digest:
                    raise ValueError("Emperor scene role identity changed")
                chunks.append(data)
        if stamp != s.archive_stamp(stream):
            raise ValueError("Emperor scene archive changed")
    stage_bytes = sum(row[0] for row in st42.GRIDS)
    return b"".join(chunks)+st42.source_inputs(afs)[-stage_bytes:]


POSE = re.compile(r"emperor-[\w-]+:(?: [0-9]{1,3}){2}(?: -?[0-9]{1,5}\.[0-9]{5}){3}"
                  r"(?: [0-9]{1,5}){6} [0-9A-F]{8}")
PIN = [0., 2000., 0.]
SITES = (("class-inlet", 0), ("placement-inlet", 0), ("before-static", 0), ("before-dynamic", 0),
         ("placement-return", 0), ("constructed", 1), ("first-pre", 22), ("first-pin", 22), ("open", 174),
         ("active-pre", 175), ("active-post-inlet", 175), ("active-post-return", 175))


def decode(text):
    lines = iter(text.splitlines())
    if len(text) > 8192 or len(text.splitlines()) != 64:
        raise ValueError("Emperor report extent")
    cases = []
    for route in (0, 1):
        def pose(site, visit):
            line = next(lines)
            if not POSE.fullmatch(line) or not line.startswith(f"emperor-{site}: {route} {visit} "):
                raise ValueError(f"invalid Emperor pose sample: {route}/{site}")
            _, _, _, x, y, z, heading, phase, role, type_, placed, side, flags = line.split()
            if (phase, role, type_, side) != ("1", "0", "8", "0") or int(placed) not in (0, 1):
                raise ValueError("Emperor pose lost its normal movement/constructor role")
            return {"edge": site, "visit": visit, "xyz": list(map(float, (x, y, z))), "heading": int(heading),
                    "shared_placement_set": bool(int(placed)), "airborne_flag": bool(int(flags, 16)&0x40)}

        track = [pose(site, visit) for site, visit in SITES]
        axis = next(lines)
        if axis != f"emperor-input: {route} 175 2 63 0022 0 52 52 1":
            raise ValueError("Emperor steering control changed")
        track += [pose("active", visit) for visit in (175, 176)]
        if next(lines) != f"emperor-receivers: {route} 176 34 07FF3FFF 00003EF0 0":
            raise ValueError("Emperor receiver publication or saved support state changed")
        for selector in (1, 2, 3, 4, 7, 27, 28, 0):
            for lane in (0, 1):
                if next(lines) != f"emperor-selector: {route} {lane} {selector} {int(selector==0)} 52":
                    raise ValueError("Emperor lane admission changed")
        xyz = [-3648., 1985., -3800.] if route else [0., 1000., 0.]
        heading = 17913 if route else 0
        if (track[0]["xyz"] != [150., 100., 300.] or track[0]["shared_placement_set"]
                or track[1]["xyz"] != PIN or track[1]["heading"] != 8192 or track[1]["shared_placement_set"]
                or any(r["xyz"] != xyz or r["heading"] != heading for r in track[2:7])
                or not all(r["xyz"] == PIN and r["heading"] == 8192 for r in track[7:])
                or any(not r["shared_placement_set"] for r in track[2:])
                or track[0]["airborne_flag"]
                or any(not r["airborne_flag"] for r in track[1:])):
            raise ValueError("Emperor placement no longer differs from the paired ordinary class pin")
        cases.append({
            "case": "st42-team-side-row" if route else "zero-authored-y-empty-grid",
            "source_raw_stage": 2 if route else 0, "table_row": 8 if route else None,
            "initial_dynamic_candidate_count": 0, "poses": track,
            "scheduled_steering": {"visit": 175, "axis_x": 63, "direction_buffer_hex": axis.split()[5],
                                   "upper_state": 52, "lower_state": 52, "cached_airborne": True,
                                   "same_pose_as_neutral_fork": True},
            "receiver_rebuild": {"after_actor_visit": 176, "emperor_count": 25, "peer_count": 9,
                                 "emperor_descriptor_mask": "07FF3FFF", "peer_descriptor_mask": "00003EF0",
                                 "emperor_saved_support_owner": False, "emperor_saved_support_latch": 0},
        })
    return cases


def native_report(path):
    p = run([Path(path).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    expected = "emperor-native: cruise 170 1 1 1 9999\nemperor-native: stationary 120 0 0 0 0\n"
    if p.returncode or p.stderr or p.stdout != expected:
        raise ValueError("authored flying-boss or stationary control changed")
    return [{"profile": "authored-cruise", "floor": 120, "spawn_y": 170, "grounded": False,
             "neutral_moves": True, "steering_changes_heading": True, "boost_accepted": True, "meter_after_boost": 9999},
            {"profile": "stationary-v1", "floor": 120, "spawn_y": 120, "grounded": True,
             "neutral_moves": False, "boost_accepted": False, "allows_moves": False}]


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Emperor output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256))
    s.read_metadata(s.MAPS/"gg4e08-dual-scene-entry.json", DUAL_SHA256)
    text = s.fixture_report(fixture, dol, module, ("--emperor-pin",), payload(afs))
    raw = s.encode({
        "kind": "gf-retail-normal-emperor-pin", "revision": 1, "borg": "gg4e08:0e04",
        "source_dol_sha256": s.SOURCE_SHA256, "source_afs_sha256": s.AFS_SHA256,
        "source_member": MEMBER, "source_member_sha256": MEMBER_SHA256,
        "source_stage_member": st42.MEMBER, "source_stage_member_sha256": st42.MEMBER_SHA256,
        "dual_scene_entry_sha256": DUAL_SHA256, "trace_sha256": sha256(text.encode("ascii")).hexdigest(),
        "original_cases": decode(text), "native_contrasts": native_report(native),
        "lane_setter_control": {"admitted": [0], "refused": [1, 2, 3, 4, 7, 27, 28], "both_lanes": True},
        "notes": [
            "Two archive-loaded normal actors enter the original scene actor pass. Initial placement forks start inside the Emperor's actual constructor after class initialization.",
            "The zero-authored-Y case declares an empty grid; the alternate case declares stage code 2, enabling the world-space 579DC row and using all three unmodified st42 collision grids.",
            "The ordinary actor visits cut before the scene world tail. A separate original list rebuild verifies exact receiver publication, not accepted support or contact. Dynamic candidates are empty at placement; no collision clearance, class child lifetime, hardware input or arbitrary mode is qualified.",
            "The existing native flying-boss controller is an authored contrast, not a matched behavior. No stationary or cruise binding is promoted as the fixed-airborne owner.",
        ],
    }, expanded=("original_cases", "native_contrasts"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": len(text.splitlines()), "native_controls": 2}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for arg in ("dol", "afs", "fixture", "module", "native", "output"):
        cli.add_argument(arg, type=Path)
    opts = cli.parse_args()
    print(json.dumps(export(opts.dol, opts.afs, opts.fixture, opts.module, opts.native, opts.output), sort_keys=True))
