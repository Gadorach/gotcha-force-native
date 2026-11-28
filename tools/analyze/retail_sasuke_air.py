"""Verify normal Sasuke's original count edges and compare the native opt-in."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
from subprocess import run

from tools.analyze import retail_dual_scene_entry as dual


s = dual.s
DUAL_SHA256 = "7caed283865bedafac11776243ef37f7800aa698e369b78ab87cacaa4a27295b"
TEXT = (
    (0x8005F978, 280, "5a03af8ca6b2fd72aaa488dcb7f031c19860dd6de3522db7d9d7bacc4e610f6c"),
    (0x8005FAF0, 788, "c0b779ce414959e90194cd49da7613628ad1ccc06db304cbd2d8a181b53a7afa"),
    (0x80060454, 732, "2f2095fbc4692187046c7bf34405d9acf7581cc1e4e6dd4b3c0e98496bc8bfa3"),
    (0x80061338, 440, "b1312388c8c5df27c09b2080217e0340511632a332c27ebb2eb565d35b599966"),
    (0x80069210, 172, "3e71a3acd38a50573c3a0b2ab3958a687210a5971ffa61cc351e677f28560d9e"),
    (0x800692BC, 152, "bb66d3a88be5c04b91c643e45b811f8a265ead84b004f2c19ba3f09fda4ff6ca"),
    (0x800693FC, 196, "92cbdc2d4b6e5efc0927794ce9d7bf6c2aff94e90daa83d0fd05079622688eb9"),
    (0x800694C0, 40, "e980d963242e2be1f9873a510c776a99d7a4848dbf56f6541653328f0c7a1194"),
    (0x80060228, 340, "6aea155fb310e59309a9ded7745296119e2933376552ba2331cdcd0973b289cf"),
    (0x8005568C, 1856, "0f964164be0f7581e45a26f8acd1113b414d13ecdbefeb94be72acde53c156ac"),
    (0x80055DCC, 280, "7f60ab140ba8c7261c2ad34d87e1f41962e0c870e435a5c0efc12f216e6fb4a2"),
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
)


def verify_source(image):
    dual.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80055588, 0x80055DCC), (0x80055590, 0x8005568C), (0x80055598, 0x80059068),
                         (0x800692A8, 0x8006A750), (0x80069340, 0x8006A750), (0x80069438, 0x800694C0),
                         (0x8006947C, 0x8006A750), (0x8005FB60, 0x80066AC0), (0x80060548, 0x8004BEB8),
                         (0x800613A0, 0x80066AC0), (0x80060294, 0x8004BEB8)):
        s.link(image, site, target)
    if (image.read(0x80055DE0, 12, "text") != bytes.fromhex("800305E05400D7FE98030087")
            or image.read(0x80055A9C, 8, "text") != bytes.fromhex("38000000981F0747")):
        raise ValueError("Sasuke airborne-cache or count-reset store changed")


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 68:
        raise ValueError("Sasuke air-point report extent")
    next_row = iter(lines)

    def take(tag, *fields):
        if next(next_row) != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"Sasuke original air-point behavior changed: {tag}/{fields}")

    take("sasuke-air-start", 174, 0, 0, 3, 10000)
    take("sasuke-air-entry", 4, 0, 0, 10000, 0)
    take("sasuke-air-entry", 5, 0, 1, 10000, 1)
    dash = []
    for before in range(4):
        take("sasuke-air-dash-gate", before, 0, 0, before, 0)
        take("sasuke-air-dash-gate", before, 1, 1, before, 1)
        after = max(1, before)
        take("sasuke-air-entry", 15, before, after, 10000, 1)
        dash.append({"used_at_request": before, "accepted_without_flick": False, "accepted_with_flick": True,
                     "used_after_request": before, "used_after_windup_entry": after, "meter_after": 10000})
    gates = []
    for active in (0, 1):
        for used in range(4):
            for ready in (0, 1):
                for inhibited in range(3):
                    accepted = int(bool(active or ready) and used < 3 and not inhibited)
                    take("sasuke-air-gate", active, used, ready, inhibited, accepted, used)
                    entered = bool(accepted and active and not ready and not inhibited)
                    if entered:
                        take("sasuke-air-entry", 8, used, used+1, 10000, 1)
                    gates.append({
                        "gate": "active-dash" if active else "ordinary-fall", "used_before": used,
                        "ready_latch": bool(ready), "control": ("fresh-pulse", "no-pulse", "input-lock")[inhibited],
                        "accepted": bool(accepted), "used_after_request": used, "used_after_entry": used+1 if entered else None,
                    })
    landing = []
    for used in (1, 3):
        take("sasuke-air-land", used, used, used, 0, 0, 10000)
        landing.append({"used_before": used, "at_airborne_resource_visit": used, "at_landing_entry": used,
                        "cached_airborne_after_landing_entry": 0, "at_next_grounded_resource_visit": 0})
    if next(next_row, None) is not None:
        raise ValueError("Sasuke air-point report suffix")
    return {"takeoff_request_used": 0, "ascent_entry_used": 1, "dash_gates_and_entries": dash,
            "relaunch_gates": gates, "landing_and_reset": landing}


def native_report(executable):
    p = run([Path(executable).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    expected = ("native-air-jump: 0 1 2 3 3 0 0 0 1\nnative-air-jump: 1 1 2 3 3 1 3 0 1\n"
                "native-air-ledge: 0 0 0\nnative-air-ledge: 1 0 1\n")
    if p.returncode or p.stderr or p.stdout != expected:
        raise ValueError(f"native air-point edge comparison changed: {p.stdout} {p.stderr}")
    cases = []
    for line in p.stdout.splitlines():
        tag, enabled, *values = line.split()
        enabled = int(enabled)
        values = list(map(int, values))
        if tag == "native-air-jump:":
            cases.append({
                "case": "separate-points-with-policies" if enabled else "combined-authored-budget",
                "used_after_three_jumps": values[:3], "used_after_fourth_attempt": values[3],
                "dash_at_jump_cap_accepted": bool(values[4]), "used_at_touchdown": values[5],
                "used_at_following_grounded_update": values[6], "used_after_fresh_jump": values[7],
            })
        else:
            cases.append({"case": "ledge-with-seed" if enabled else "ledge-without-seed",
                          "used_at_walk_off": values[0], "used_after_air_dash": values[1]})
    return cases


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Sasuke air-point output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-dual-scene-entry.json", DUAL_SHA256)
    actual = s.fixture_report(fixture, dol, module, ("--sasuke-air-points",), dual.payload(afs))
    raw = s.encode({
        "kind": "gf-retail-sasuke-air-points", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "dual_scene_entry_sha256": DUAL_SHA256,
        "source_trace_sha256": sha256(actual.encode("ascii")).hexdigest(), "borg": "gg4e08:000a",
        "source_normal_member_sha256": dual.MODELS[0][2], "air_point_limit": 3,
        "original_opening_visit": 174, "original_edges": decode(actual), "native_cases": native_report(native),
        "notes": [
            "The original count policy comes from the archive-loaded normal Sasuke actor. Branch probes declare the current selector, cached movement state and input pulse rather than qualifying the complete controller or physical stick path.",
            "Ground takeoff entry does not spend; ascent and an admitted relaunch entry add one point. A relaunch request alone does not spend. The fall leaf requires readiness; the active dash leaf does not, but both require a pulse, a clear input lock and a count below three.",
            "The ordinary falling dash leaf admits the buffered flick at every tested count. Wind-up seeds zero to one and leaves all nonzero counts and the meter unchanged at the observed entry cut.",
            "The landing entry clears the airborne state without resetting used points. The next grounded resource prefix clears the count. The floor collision that requests landing is not run in these specific branches.",
            "The portable test matches seed and reset order only. Native takeoff already moves on its accepted jump update; no original pose clock, dash budget, chain, height, collision or complete actor admission is implied.",
        ],
    }, expanded=("native_cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as f:
        f.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "source_observations": len(actual.splitlines())-1, "native_cases": 4}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
