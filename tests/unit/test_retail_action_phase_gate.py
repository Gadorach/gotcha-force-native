"""Check action-gate caller decoding and completeness against the code atlas."""
import json
from pathlib import Path
import struct
import unittest
from unittest.mock import Mock

from tools.analyze import retail_action_phase_gate as gate


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailActionPhaseGateTest(unittest.TestCase):
    def test_branch_conditions_distinguish_negative_sentinel(self):
        image = Mock()
        for op, expected in ((0x4181, [1]), (0x4081, [-1, 0]), (0x4182, [0]), (0x4082, [-1, 1])):
            with self.subTest(op=op):
                image.read.return_value = struct.pack(">II", 0x2C030000, op << 16 | 0xFFF8)
                branch = gate.caller_branch(image, 0x80004000)
                self.assertEqual(branch["takes_branch_for"], expected)
                self.assertEqual(branch["target"], "80004000")
                self.assertEqual(branch["fallthrough"], "8000400C")
        for cmp, branch in ((0x2C040000, 0x41810018), (0x2C030000, 0x41810019)):
            image.read.return_value = struct.pack(">II", cmp, branch)
            self.assertIsNone(gate.caller_branch(image, 0x80004000))

    def test_every_direct_gate_caller_is_in_the_published_map(self):
        atlas = json.loads((MAPS / "gg4e08-code-atlas.json").read_bytes())
        phase = json.loads((MAPS / "gg4e08-action-phase-gate.json").read_bytes())
        self.assertEqual(atlas["symbol_metadata_sha256"], phase["symbol_metadata_sha256"])
        expected = {(start, gate.address(int(start, 16) + offset))
                    for start, info in atlas["functions"].items()
                    for offset, dest, kind in info.get("direct", ())
                    if (dest, kind) == (phase["entry"], "call")}
        actual = {(item["function"], item["call"]) for item in phase["callers"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), phase["counts"]["calls"])
        self.assertEqual({item["test_vs_zero"] for item in phase["callers"]}, {"gt", "le", "eq", "ne"})
        sites = {item["call"]: item for item in phase["callers"]}
        self.assertEqual(sites["80070D1C"]["takes_branch_for"], [1])
        self.assertEqual(sites["80070F74"]["takes_branch_for"], [-1, 0])
