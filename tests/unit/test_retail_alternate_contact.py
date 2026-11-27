"""Preserve collision reset, alternate publication, and independent role gates."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_alternate_contact as alternate


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-alternate-contact.json"


def report(data):
    lines = []
    for r in data["reset_cases"]:
        values = (r["declared_actor_slot"], r["history_entries_cleared"], r["role_exclusion_mask_after"],
                  r["late_eligibility_byte_after"], r["body_timer_preserved"], r["visual_gate_preserved"], r["hp_preserved"])
        lines.append("alternate-reset: " + " ".join(map(str, values)))
    for r in data["pair_cases"]:
        values = (int(r["borg"].split(":")[1], 16), alternate.CASES.index(r["case"]), r["attacker_role_mask"],
                  r["late_eligibility_byte"], r["receiver_role_mask"], r["ordinary_attack_count"],
                  r["alternate_attack_count"], r["alternate_receiver_count"], r["retained_owner_token"],
                  r["alternate_pair_amount_calls"], r["hp_lost"], r["receiver_hp_after"], r["body_timers_after_pair"][1])
        lines.append("alternate-pair: " + " ".join(map(str, values)))
    return "\n".join(lines)


class RetailAlternateContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_reset_keeps_hp_and_clocks_but_excludes_roles_with_open_late_byte(self):
        for r in self.data["reset_cases"]:
            self.assertTrue(r["rebound_slot_hit_bank"])
            self.assertEqual((r["history_entries_cleared"], r["role_exclusion_mask_after"],
                              r["late_eligibility_byte_after"]), (48, 0xEF, 0))
            self.assertEqual((r["body_timer_preserved"], r["visual_gate_preserved"], r["hp_preserved"]), (9, 3, 123))

    def test_both_outgoing_gates_and_the_receiver_mask_are_independent(self):
        for count, model in ((26, "gg4e08:0600"), (7, "gg4e08:0401")):
            rows = [r for r in self.data["pair_cases"] if r["borg"] == model]
            self.assertEqual([r["alternate_attack_count"] for r in rows], [0, 0, 0, 1, 1])
            self.assertEqual([r["alternate_receiver_count"] for r in rows], [1, 1, 1, 0, 1])
            self.assertEqual([r["hp_lost"] for r in rows], [0, 0, 0, 0, count])
            self.assertTrue(all(r["ordinary_pair_amount_calls"] == 0 and r["retained_owner_token"] == 1 for r in rows))
            self.assertTrue(rows[-1]["retained_pair_refused"])

    def test_roster_census_distinguishes_descriptor_identity_from_live_routes(self):
        rows = self.data["alternate_shape_rows"]
        group = [r for r in rows if r["group"] == 26]
        self.assertEqual((len(rows), len({r["borg"] for r in rows}), len(group)), (60, 55, 46))
        self.assertEqual({(r["step"], r["shape_sha256"]) for r in group}, {(44, alternate.GROUP_26_SHA256)})
        self.assertEqual([r["level_one_hp"] for r in self.data["sources"]], [500, 350])
        self.assertEqual(self.data["census"]["normal_hit_blocks"], 206)

    def test_report_refuses_same_mask_assumptions_and_wrong_hp(self):
        good = report(self.data)
        self.assertEqual(alternate.decode(good), (self.data["reset_cases"], self.data["pair_cases"]))
        for bad in (good.replace("1536 3 111 0 2 0 1 0", "1536 3 111 0 2 0 1 1", 1),
                    good.replace("1 7 173 4", "1 26 154 4", 1)):
            with self.assertRaises(ValueError):
                alternate.decode(bad)

    def test_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "alternate.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                alternate.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
