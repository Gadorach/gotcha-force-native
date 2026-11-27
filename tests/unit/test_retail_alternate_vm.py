"""Keep alternate VM shape admission, signed HP, and phase guards separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_alternate_vm as vm


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-alternate-vm.json"


def report(rows):
    lines = []
    for r in rows:
        timers = r["timers_at_hp_writer"] or [0, 0]
        ledgers = r["positive_ledgers_at_hp_writer"] or dict.fromkeys(("attacker_opponent", "attacker_ally", "receiver"), 0)
        values = (int(r["borg"].split(":")[1], 16), r["group"], vm.CASES.index(r["case"]),
                  int(r["shape_admitted"]), *r["actor_secondary_state_bytes"], r["amount_calculator_calls"],
                  int(r["hp_writer_reached"]), r["signed_hp_writer_argument"] or 0, r["receiver_hp_after"],
                  int(r["hp_writer_reached"]), *timers, *(ledgers[k] for k in ("attacker_opponent", "attacker_ally", "receiver")),
                  int(r["waited_at_frame_0_and_8"]))
        lines.append("alternate-vm: " + " ".join(map(str, values)))
    return "\n".join(lines)


class RetailAlternateVMTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = cls.data["cases"]

    def test_nurse_heals_without_amount_calculation_or_revival(self):
        for group in (3, 4):
            rows = {r["case"]: r for r in self.rows if r["borg"] == "gg4e08:0900" and r["group"] == group}
            for name, request, after in (("ally", -200, 180), ("opponent", -50, 140), ("ally-zero-hp", -200, 0)):
                r = rows[name]
                self.assertEqual((r["signed_hp_writer_argument"], r["receiver_hp_after"], r["amount_calculator_calls"]),
                                 (request, after, 0))
                self.assertEqual(r["timers_at_hp_writer"], [15, 15] if group == 3 else [0, 0])

    def test_nurse_group_four_state_survives_refusal_but_shape_has_its_own_delay(self):
        rows = [r for r in self.rows if r["group"] == 4]
        self.assertTrue(all(r["actor_secondary_state_bytes"] == [129, 127, 255] for r in rows))
        self.assertFalse(rows[0]["shape_admitted"])
        self.assertFalse(rows[0]["hp_writer_reached"])
        self.assertTrue(all(r["waited_at_frame_0_and_8"] for r in rows[1:]))

    def test_patra_uses_amount_and_keeps_ally_and_opponent_ledgers_distinct(self):
        _, ally, foe, zero = [r for r in self.rows if r["borg"] == "gg4e08:0902"]
        self.assertEqual([r["signed_hp_writer_argument"] for r in (ally, foe, zero)], [2, 8, 2])
        self.assertEqual([r["hp_change"] for r in (ally, foe, zero)], [-2, -8, 0])
        self.assertEqual(ally["positive_ledgers_at_hp_writer"], {"attacker_opponent": 0, "attacker_ally": 2, "receiver": 2})
        self.assertEqual(foe["positive_ledgers_at_hp_writer"], {"attacker_opponent": 8, "attacker_ally": 0, "receiver": 8})
        self.assertEqual(zero["positive_ledgers_at_hp_writer"], ally["positive_ledgers_at_hp_writer"])

    def test_static_scan_does_not_promote_group_26_or_all_group_3_4_routes(self):
        self.assertEqual(self.data["indexed_program_scan"]["group_26_occurrences"], 0)
        self.assertEqual(self.data["direct_binder_scan"]["unresolved_arguments"], 91)
        self.assertEqual(len(self.data["static_role_program_choices"]), 23)
        self.assertEqual(len(self.data["sources"]), 3)

    def test_report_rejects_revival_and_early_group_four_publication(self):
        good = report(self.rows)
        self.assertEqual(vm.decode(good), self.rows)
        for bad in (good.replace("-200 0 1 15 15", "-200 180 1 15 15", 1),
                    good.replace("-200 180 1 0 0 0 0 0 1", "-200 180 1 0 0 0 0 0 0", 1)):
            with self.assertRaises(ValueError):
                vm.decode(bad)

    def test_does_not_replace_output(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "alternate-vm.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                vm.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
