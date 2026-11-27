"""Validate action-input receipts against the atlas and supported scalar domain."""
from collections import Counter
from io import BytesIO
import json
from pathlib import Path
import struct
import unittest

from tools.analyze import retail_action_input as action


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailActionInputTest(unittest.TestCase):
    def test_fast_refusal_for_unqualified_archive(self):
        for raw in (b"", b"not the original AFS"):
            with self.subTest(raw=raw), self.assertRaisesRegex(ValueError, "AFS (extent|revision)"):
                action.verify_archive(BytesIO(raw))

    def test_limits_are_role_zero_spec_operands_not_durations(self):
        spec = bytearray(action.ACTOR_BYTES)
        struct.pack_into(">3f", spec, 0x184, 60, 60, 120)
        self.assertEqual(action.charge_limits(spec), (60, 60, 120))
        with self.assertRaisesRegex(ValueError, "extent"):
            action.charge_limits(spec[:-1])
        for bad in (float("nan"), float("inf"), -1, 10001):
            struct.pack_into(">f", spec, 0x188, bad)
            with self.subTest(bad=bad), self.assertRaisesRegex(ValueError, "domain"):
                action.charge_limits(spec)

    def test_roster_profiles_and_request_priorities(self):
        result = json.loads((MAPS / "gg4e08-action-input.json").read_bytes())
        atlas = json.loads((MAPS / "gg4e08-code-atlas.json").read_bytes())
        self.assertEqual(result["symbol_metadata_sha256"], atlas["symbol_metadata_sha256"])
        self.assertEqual(result["source_dol_sha256"], atlas["source_dol_sha256"])
        records = {row["id"]: row for row in result["borgs"]}
        self.assertEqual(set(records), {b["id"] for b in atlas["borgs"]})
        for b in atlas["borgs"]:
            expected = []
            for ident in b["profiles"]:
                active = {g["group"]: g["mode"] for g in atlas["profiles"][ident]["groups"][3:]
                          if g["mode"] in (0, 1)}
                if active:
                    expected.append({"profile": ident, "enabled_modes": active})
            self.assertEqual(records[b["id"]]["profiles_with_charge"], expected)
        self.assertEqual(sum(r["spec_record"] is not None for r in records.values()),
                         result["counts"]["spec_record_resolved"])
        black = records["gg4e08:062a"]
        self.assertEqual(black["limits"], [60, 60, 120])
        self.assertEqual(black["profiles_with_charge"], [
            {"profile": "p088", "enabled_modes": {"B-charge": 1}},
            {"profile": "p089", "enabled_modes": {"B-charge": 1, "X-charge": 1}},
        ])
        self.assertEqual(records["gg4e08:0c04"]["limits"][2], 600)
        self.assertEqual([p["kind"] for p in result["arbitration"]["probes"]],
                         ["linked-action", "charged-X", "charged-B", "ordinary-X", "close-B", "ordinary-B"])
        pulses = Counter((r["pad_mask"], r["request_pulses"]) for r in result["conditioner"]["button_rows"])
        self.assertEqual(pulses[(0x200, 1)], 2)
        self.assertEqual(pulses[(0x100, 16)], 1)
        self.assertEqual([c["release_request_mask"] for c in result["charge"]["channels"]],
                         [0x400, 0x800, 0x1000])
