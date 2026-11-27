"""Scale the bounded key-21 speed proportions into the authored Workshop shot."""
from decimal import Decimal, InvalidOperation
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_continuation import MODEL, authored_rifle_rows, verify_paid_rifle
from tools.extract.battle_initial_target import SOURCE_SHA256

EVIDENCE = "docs/api-map/gg4e08-rifle-speed.json"
EVIDENCE_SHA256 = "23c182e20ce2dfb8e18b2438252c682ef6541c9813478b671f2f6db4268cf2c3"
ROW = 0x803036CC
KEY = 21


def qualify(image, path=None):
    raw = read_bounded(path if path is not None else Path(__file__).resolve().parents[2] / EVIDENCE, 8 << 10)
    if sha256(raw).hexdigest() != EVIDENCE_SHA256:
        raise ValueError("unqualified rifle speed receipt")
    verify_paid_rifle(image)
    initial, gain, cap = struct.unpack(">3f", image.read(ROW+44, 12, "data"))
    source_speed = {"initial": initial, "gain": gain, "cap": cap}
    receipt = json.loads(raw)
    capped = {sample["visit"]: sample for sample in receipt["samples"]}
    if (
        (receipt["kind"], receipt["revision"], receipt["source_dol_sha256"], receipt["borg"],
         receipt["carrier_key"], receipt["carrier_row"], receipt["checked_visits"]) !=
        ("gf-retail-rifle-speed", 1, SOURCE_SHA256, "gg4e08:0600", KEY, f"{ROW:08X}", 162)
        or (initial, gain, cap) != (80, 0.5, 160) or receipt["speed"] != source_speed
        or (capped[160]["speed_before"], capped[160]["speed_after"], capped[161]["position_delta_z"]) !=
        (159.5, 160, 160)
    ):
        raise ValueError("rifle speed witness does not qualify the active key-21 ramp")
    return {
        "scope": "normal Machine Red, authored ordinary B shot only",
        "source_evidence": EVIDENCE, "source_evidence_sha256": EVIDENCE_SHA256,
        "source_factory_family": "type-2", "source_factory_key": KEY,
        "source_speed": source_speed, "retail_behavior_verified": False,
    }


def bind(model, member_digest, gameplay, receipt, evidence):
    if model != MODEL:
        return gameplay
    lines = authored_rifle_rows(member_digest, gameplay, receipt)
    if any(r.startswith(b"projectile-speed-ramp=") for r in lines) or "projectile_speed_ramp_test" in receipt:
        raise ValueError("rifle speed ramp cannot replace an existing ramp")
    shot, = (line.split(b"|") for line in lines if line.startswith(b"projectile=shot|"))
    if (len(shot) != 11 or shot[1:3] != [b"bullet", b"1"] or shot[4:7] != [b"0"]*3
            or len(shot[3]) > 16 or shot[-1] not in (b"ignore-floor", b"stop-on-floor", b"stop-on-stage")):
        raise ValueError("rifle speed ramp needs the ordinary authored free-flight shot")
    try:
        launch = Decimal(shot[3].decode("ascii"))
    except (InvalidOperation, UnicodeError) as exc:
        raise ValueError("rifle speed ramp needs a finite authored launch speed") from exc
    if not launch.is_finite() or not 1 <= launch <= 140:
        raise ValueError("rifle speed ramp needs a bounded authored launch speed")
    source = {field: Decimal(str(value)) for field, value in evidence["source_speed"].items()}
    scale = launch / source["initial"]
    gain, cap = source["gain"]*scale, source["cap"]*scale
    if not Decimal(".01") <= gain <= 20 or not launch < cap <= 140:
        raise ValueError("authored rifle speed ramp exceeds the native domain")
    encoded_gain, encoded_cap = (f"{value.normalize():f}" for value in (gain, cap))
    if len(encoded_gain) > 16 or len(encoded_cap) > 16:
        raise ValueError("authored rifle speed ramp requires unsupported decimal precision")
    receipt["projectile_speed_ramp_test"] = {
        **evidence, "move": "basic-shot", "projectile": "shot",
        "native_speed": {"initial": float(launch), "gain": float(gain), "cap": float(cap)},
        "authored_ratio_scale": float(scale), "live_clock_converted": False,
    }
    return gameplay + f"projectile-speed-ramp=shot|{encoded_gain}|{encoded_cap}\n".encode("ascii")
