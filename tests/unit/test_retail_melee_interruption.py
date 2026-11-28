from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import native_workshop_interruptions as workshop
from tools.analyze import retail_melee_interruption as tau


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class MeleeInterruptionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace = json.loads((MAPS/"gg4e08-melee-interruption.json").read_bytes())
        cls.workshop = json.loads((MAPS/"native-workshop-interruptions.json").read_bytes())

    def test_exact_source_entry_group_and_pose(self):
        prior = (MAPS/"gg4e08-dual-scene-entry.json").read_bytes()
        self.assertEqual(self.trace["dual_scene_entry_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.trace["actor"], self.trace["subject_seat"], self.trace["original_arm_pose"],
                          self.trace["original_amount_record"], self.trace["original_hp_removed"]),
                         ("gg4e08:0807", 0, 10, "80377AD8", 17))

    def test_uninterrupted_allocation_publication_token_and_return_are_separate(self):
        case = self.trace["original_cases"][0]
        rows = {r["visit"]: r for r in case["checkpoints"]}
        self.assertEqual([case[k] for k in ("subject_input_and_cell_allocation_visit",
                                            "subject_first_publication_visit", "subject_hp_write_visit",
                                            "subject_cell_return_visit")], [8, 27, 27, 32])
        self.assertEqual(rows[8]["melee_allocation_ordinals"], rows[27]["melee_allocation_ordinals"])
        self.assertEqual([rows[v]["actor_outgoing_tokens"][0] for v in (8, 26, 27)], [0, 0, 1])
        self.assertEqual(rows[27]["hp_by_seat"], [60, 43])
        self.assertTrue(rows[32]["exact_cells_on_free_list"][0])

    def test_interrupted_original_strike_returns_without_publishing(self):
        case = self.trace["original_cases"][1]
        rows = {r["visit"]: r for r in case["checkpoints"]}
        self.assertEqual(rows[19]["hp_by_seat"], [43, 60])
        self.assertEqual(rows[19]["subject_lower_selector"], [61, 0])
        self.assertEqual(rows[19]["subject_pose_before_collision_bits"], "40A00000")
        self.assertTrue(rows[20]["exact_cells_on_free_list"][0])
        self.assertTrue(all(not row["published_strikes"][0] for row in rows.values()))
        self.assertFalse(case["subject_token_assigned"])
        self.assertIsNone(case["subject_hp_write_visit"])

    def test_native_cancellation_and_fresh_press_not_projection_of_original_time(self):
        a, b = self.trace["native_authored_cases"]
        self.assertEqual(a["subject_first_hp_write_update"], a["subject_first_arm_update"])
        self.assertEqual((b["incoming_hit_update"], b["subject_interrupt_update"]), (10, 11))
        self.assertEqual((b["subject_fresh_start_after_interrupt"], b["subject_first_arm_update"]), (34, 43))
        self.assertEqual(b["final_hp_by_actor"], [48, 48])

    def test_workshop_current_four_pre_payload_and_one_after_emission(self):
        self.assertEqual(self.workshop["classifications"], {
            "authored-hit-after-payload": 1, "authored-hit-before-melee-window": 4,
        })
        self.assertEqual(self.workshop["subjects"], 206)
        for case in self.workshop["cases"]:
            for hit in case["preceding_accepted_hits"]:
                self.assertEqual(hit["update"]+1, case["interrupt_update"])
                self.assertGreater(hit["hp_removed"], 0)
            if case["borg"] == "gg4e08:0f05":
                self.assertEqual(case["first_payload_update"], 230)
            else:
                self.assertIsNone(case["first_payload_update"])
                self.assertLess(case["frame_at_interrupt"], case["authored_melee_first_frame"])

    def test_unproved_interruptions_do_not_get_a_verdict(self):
        template = ("borg-api: round=0 tick=8 actor=0 move=slash event=start frame=0 object=0 recipient=255 value=0\n"
                    "borg-api: round=0 tick=11 actor=0 move=slash event=interrupt frame=2 object=0 recipient=255 value=0\n")
        missing_hit = workshop.classify(template, 0, 11, {"slash": 9})
        self.assertEqual(missing_hit["classification"], "unresolved")
        missed_recipient = template + ("borg-api: round=0 tick=10 actor=1 move=slash event=hit frame=9 object=4 "
                                        "recipient=2 value=12\n")
        self.assertEqual(workshop.classify(missed_recipient, 0, 11, {"slash": 9})["classification"], "unresolved")
        hit = missed_recipient.replace("recipient=2 value=12", "recipient=0 value=12")
        self.assertEqual(workshop.classify(hit, 0, 11, {"slash": 9})["classification"], "authored-hit-before-melee-window")

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            tau.decode("")
        with TemporaryDirectory() as tmp:
            out = Path(tmp)/"keep.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                tau.export(None, None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
