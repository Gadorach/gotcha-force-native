from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_prop_break_contact as contact


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStagePropBreakContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-prop-break-contact.json").read_bytes())
        cls.cases = {r["case"]: r for r in cls.report["cases"]}

    def test_exact_tau_command_and_prop_damage_inlet(self):
        source = self.report["tau_attack"]
        self.assertEqual([source[k] for k in ("borg", "opcode_address", "queue_group", "steps",
                                             "source_amount_record", "raw_power")],
                         ["gg4e08:0807", "80377E40", 1, [2], "80377AD8", 20])
        self.assertFalse(self.report["prop_hp_supplied"])
        self.assertEqual((self.report["prop_owner_kind"], self.report["hp_writer"]), (22, "8003D344"))

    def test_horizontal_miss_and_same_token_repeat_do_not_write_hp(self):
        miss, capsule, plane = [self.cases[n]["offers"] for n in
                                ("horizontal-miss", "capsule-retained-token", "plane-fresh-tokens")]
        self.assertEqual([x["hp_after"] for x in miss], [100, 100])
        self.assertEqual(self.cases["capsule-retained-token"]["positive_hp_bucket_visit"]["break_cue_requests"], 0)
        for descriptor, rows in ((236, capsule), (237, plane)):
            first, repeat = rows[:2]
            self.assertEqual(first["accepted_prop_descriptor"], descriptor)
            self.assertEqual(first["attack_token"], repeat["attack_token"])
            self.assertEqual(repeat["retained_tokens"], [1])
            self.assertIsNone(repeat["hp_writer_call_site"])
            self.assertEqual(repeat["hp_after"], 83)

    def test_lethal_request_and_real_hp_removal_do_not_imply_credit_or_retirement(self):
        rows = self.cases["plane-fresh-tokens"]["offers"]
        accepted = [r for r in rows if r["hp_writer_call_site"] is not None]
        self.assertEqual([r["attack_token"] for r in accepted], list(range(1, 7)))
        self.assertEqual([r["hp_after"] for r in accepted], [83, 66, 49, 32, 15, 0])
        self.assertEqual([r["requested_damage"] for r in accepted], [17]*6)
        self.assertEqual([r["hp_removed"] for r in accepted], [17]*5+[15])
        self.assertTrue(all(r["caster_damage_credit"] == r["prop_damage_taken_credit"] == 0 for r in accepted))
        self.assertEqual(accepted[-1]["retained_tokens"], list(range(1, 7)))
        self.assertTrue(accepted[-1]["plane_still_published"])
        self.assertTrue(accepted[-1]["actor_support_still_latched"])

    def test_break_replay_matches_prior_and_attack_returns_separately(self):
        raw = (MAPS / "gg4e08-stage-prop-recovery.json").read_bytes()
        self.assertEqual(self.report["stage_prop_recovery_sha256"], sha256(raw).hexdigest())
        prior = json.loads(raw)
        self.assertEqual(self.report["rising_carry_fork"], prior["rising_carry_fork"])
        self.assertTrue(self.report["post_break_observations_match_recovery_receipt"])
        self.assertEqual((self.report["break_collision_cells_returned"], self.report["rebind_visit_after_break"]), (2, 600))
        self.assertEqual(self.report["retained_attack_return"], {
            "last_token": 6, "published_after_close": 0, "cells_returned_on_close": 1, "prop_hp_after_close": 100,
        })

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            contact.decode("")
        with self.assertRaisesRegex(ValueError, "horizontal-miss"):
            contact.decode("stage-prop-hit: invalid\n"*30)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "break.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
