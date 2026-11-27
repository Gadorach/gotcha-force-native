"""Census ordinary type-0 carrier operands and static roster release candidates."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from math import isfinite
from pathlib import Path
import struct

from tools.analyze import reaction_emit
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, candidate_census, export_census,
)
from tools.analyze.retail_projectile_origins import TABLES, origin_tables

FACTORY, DISPATCH = 0x80082824, 0x802D9748
PHASES = (0x80082B08, 0x80082FC4, 0x80083180, 0x800831BC)
BASE, STRIDE, COUNT, _, _ = TABLES["type-0"]
TEXT = {
    FACTORY: (328, "3d339c26437064d921fbd0e3652cd1918cd037fe2290fdace81a3b6f9f20bd01"),
    0x80082AB8: (80, "eb2e1fc4a01288aa7a7569a3ecd3406781b920742fb13fec7cfd9ec9b492c2dc"),
    0x80082B08: (1212, "19d466113a0a849d3684a5e415a9f29b166b94b0573f598ef829ac44381bc464"),
    0x80082FC4: (444, "e799cfa7786e84530b731bff730dfd566312aea65ae32c510b6fbcd06884e84c"),
    0x80083180: (60, "11d9d42edba5e1e9db249789bd71cdcd8afa309a97126a770ef99b6787deb25e"),
    0x800831BC: (32, "6a7aae865f493c64c7ec6aa3e03862793ce9d9fe7078c860e6c9a9bd575498ef"),
    0x80083AD4: (208, "15edc0cf80125dea119cd052c77b612f22bbd0f8371d90c86159d023a7da5b23"),
}


def carrier_rows(image, origins):
    if struct.unpack(">4I", image.read(DISPATCH, 16, "data")) != PHASES:
        raise ValueError("ordinary type-0 phase table disagrees")
    for entry, (size, digest) in TEXT.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"type-0 source signature mismatch at {address(entry)}")
    rows = []
    for key, origin in enumerate(origins):
        start = BASE + STRIDE * key
        raw = image.read(start, STRIDE, "data")
        group = reaction_emit.signed_byte(raw[3])
        speed, = struct.unpack_from(">f", raw, 16)
        steer_delay, = struct.unpack_from(">i", raw, 20)
        steering_operand, = struct.unpack_from(">f", raw, 24)
        target_test_operand, = struct.unpack_from(">f", raw, 44)
        axis, flip, _, flatten = raw[48:52]
        if (not 0 <= group < reaction_emit.GROUPS or not 0 <= steer_delay < 128
                or axis not in (0, 1, 2) or flip not in (0, 1) or flatten not in (0, 1, 2, 4)
                or any(not isfinite(v) or not 0 <= v <= 1e5 for v in (speed, steering_operand, target_test_operand))):
            raise ValueError(f"type-0 unsupported operand at key {key}")
        rows.append({
            **origin, "row": address(start), "collision_group_selector": group, "launch_speed": speed,
            "steering_delay_operand": steer_delay, "steering_operand": steering_operand,
            "launch_target_test_operand": target_test_operand, "matrix_axis": axis,
            "reverse_matrix_axis": bool(flip), "fallback_zero_component_mask": flatten,
        })
    if len(rows) != COUNT:
        raise ValueError("ordinary type-0 table extent")
    return rows


def site_selector(ident, release, evidence):
    guard = release.get("release_guard_candidate")
    expected = {
        "actor_byte": "149", "first_value": 0, "zero_store": "80188BC0",
        "one_bit_store": "80188CE0", "requires_nonzero_return_from": "8006DBE0",
    }
    if (ident, release["site"], evidence, guard) != (
        "gg4e08:0d05", "80188CC8", {"pinned-zero-seed-and-one-bit-selector"}, expected,
    ) or release["key"] not in (16, 17):
        return None
    return {"key_selector": {**guard, "evidence": "pinned-zero-seed-and-one-bit-selector"}}


def recover(image, archive, atlas, selections):
    origins = origin_tables(image)
    rows = carrier_rows(image, origins["type-0"])
    counts, sites, candidates = candidate_census(
        image, archive, atlas, selections, origins, "type-0", FACTORY, rows, site_selector)
    counts.update({
        "launch_speeds": dict(Counter(str(int(r["launch_speed"])) for r in rows)),
        "unresolved_basis_keys": [r["key"] for r in rows if r["basis"] == "unresolved"],
        "key_selector_candidates": sum("key_selector" in r for r in candidates),
    })
    return {
        "kind": "gf-retail-type0-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256,
        "standalone_installers_sha256": INSTALLERS_SHA256, "factory": address(FACTORY),
        "phase_table": address(DISPATCH), "phase_entries": list(map(address, PHASES)), "counts": counts,
        "factory_rows": rows, "call_sites": sites, "candidates": candidates,
        "reviewed_model_branch": {
            "site": "801E4748", "source_model": "0F06",
            "equal_call": {"site": "801E4754", "key": 122},
            "not_equal_call": {"site": "801E4764", "key": 123},
        },
        "statically_reviewed_flight": {
            "ordinary_launch_timer": 300, "timer_decrement_site": "80082FE4",
            "bounds_test_site": "80083008", "nonzero_contact_byte_test_site": "80083028",
            "contact_byte": "carrier+0x1D9", "nonzero_contact_enters_phase": 2,
            "retirement": "800831BC -> 80088E50",
        },
        "notes": [
            "Only the ordinary 80082824 constructor is counted; the alternate 8008296C uses a different launch-basis arm.",
            "Selected installer profiles are checked from bounded execution; release edges, operands and queues in this census are static.",
            "The one-bit selector retains both keys at one call site and does not establish a later shot's selector value.",
            "The negative basis selector is unresolved, not clamped to joint zero.",
            "Normal owner descriptors give raw power only. Alternate specs and combined-form ownership remain unqualified.",
            "Static flight control does not establish how a specific contact sets flags or how many live frames a projectile survives.",
        ],
    }


def export(dol, afs, atlas, selections, output):
    return export_census(dol, afs, atlas, selections, output, recover)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.selections, args.output), sort_keys=True))
