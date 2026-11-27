"""Scope upper-marker and direct-queue metadata to the decoded VM prefix."""
from hashlib import sha256
import json
from pathlib import Path
import unittest

from tools.analyze import retail_program_cues as cues


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailProgramCuesTest(unittest.TestCase):
    @staticmethod
    def describe(blob, start=0x8000):
        def read(addr, size):
            if addr < start or addr + size > start + len(blob):
                raise ValueError("outside test byte span")
            return blob[addr - start:addr - start + size]
        return cues.describe_program(read, start)

    def test_positive_marker_and_queue_use_different_operands(self):
        program = self.describe(bytes.fromhex(
            "02ff000000000000"  # mode sentinel does not qualify a zero cue
            "0a0007ff"          # BB does not select the allocation queue
            "00020003"
            "02ff050000000000"
            "0a0200ff"
            "0001ffff"
        ))
        self.assertEqual(program["status"], "terminal_wait")
        self.assertEqual(program["first_positive_upper_cue"],
                         {"site": "00008010", "preceding_waits": 1, "value": 5})
        self.assertEqual(program["first_direct_melee_request"],
                         {"site": "00008018", "preceding_waits": 1, "queue": 2})

    def test_unknown_opcode_does_not_scan_following_data_as_cues(self):
        program = self.describe(bytes.fromhex("0a0002ff120000000a0200ff"))
        self.assertEqual(program, {"status": "unsupported_opcode", "commands": 1,
                                   "unsupported": {"opcode": 18, "site": "00008004"}})
        self.assertEqual(self.describe(bytes.fromhex("02010000")), {"status": "refused-program-extent"})

    def test_installed_roster_bank_aliases_and_exact_observed_programs(self):
        result = json.loads((MAPS / "gg4e08-program-cues.json").read_bytes())
        atlas = json.loads((MAPS / "gg4e08-code-atlas.json").read_bytes())
        installers = (MAPS / "gg4e08-standalone-installers.json").read_bytes()
        self.assertEqual(sha256(installers).hexdigest(), result["standalone_installers_sha256"])
        selected = cues.selected_banks(json.loads(installers)["borgs"], atlas["borgs"], atlas["profiles"])
        self.assertEqual(len(selected), 206)
        self.assertEqual(result["models"], selected)
        self.assertEqual(len(result["programs"]), result["counts"]["programs"])
        roots = {model[key] for model in selected for key in ("local_bank", "common_bank")}
        self.assertEqual(roots, set(result["banks"]))
        self.assertEqual(result["banks"]["00000000"], {"status": "absent-bank"})
        gap = result["banks"]["80310000"]["groups"][4]
        self.assertEqual(gap, {"status": "refused-choice-table", "table": "8031007C"})
        bank = result["banks"]["80310000"]["groups"]
        self.assertEqual(bank[0], bank[1])
        for model, group, choice, program, marker, expected in (
            ("gg4e08:0600", 2, 0, "8030903E", "first_positive_upper_cue",
             {"site": "8030906A", "preceding_waits": 2, "value": 1}),
            ("gg4e08:000a", 3, 1, "802BE3B6", "first_direct_melee_request",
             {"site": "802BE3B6", "preceding_waits": 0, "queue": 2}),
        ):
            root = next(m["local_bank"] for m in selected if m["id"] == model)
            self.assertEqual(result["banks"][root]["groups"][group]["choices"][choice], program)
            self.assertEqual(result["programs"][program][marker], expected)
