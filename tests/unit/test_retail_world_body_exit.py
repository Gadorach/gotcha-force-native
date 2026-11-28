"""Keep a loaded KO's HP, body, archive and replacement gates separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_world_body_exit as ko


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-world-ko-handoff.json"


class LoadedWorldBodyExitTest(unittest.TestCase):
    def test_ko_writer_does_not_release_the_receiver(self):
        report = json.loads(RECEIPT.read_bytes())
        lives, lethal = report["old_actor_control"]
        w = lethal["writer"]
        self.assertEqual((w["requested_hp"], w["removed_hp"], w["actor_phase_after_writer"]), (14, 1, 1))
        self.assertTrue(w["archive_still_bound_after_writer"])
        self.assertEqual(lives["writer"]["requested_hp"], 17)
        self.assertTrue(all(r["old_shape_cells_returned_mask"] == 0 and r["old_archive_owner_bound"]
                            for r in lives["world_boundaries"]))
        steps = {r["visit_after_scene_open"]: r for r in lethal["world_boundaries"]}
        self.assertEqual(steps[27]["attacker_damage_credit"], 14)
        self.assertEqual([steps[n]["ordinary_receiver_descriptor_mask"] for n in (27, 28)], [6, 0])
        self.assertEqual([steps[n]["old_shape_cells_returned_mask"] for n in (28, 63)], [0, 7])
        self.assertTrue(steps[64]["old_archive_owner_bound"])
        self.assertFalse(steps[65]["old_archive_owner_bound"])
        self.assertEqual(report["handoff"]["returned_view_wrappers"], 4)
        self.assertFalse(report["handoff"]["disk_call_executed"])

    def test_stale_ready_byte_does_not_admit_the_new_generation(self):
        report = json.loads(RECEIPT.read_bytes())
        missing = report["no_owner_wait"][-1]
        self.assertEqual((missing["actor_phase"], missing["seat_generation"]), (0, 1))
        self.assertTrue(missing["old_ready_byte_still_set"])
        self.assertFalse(missing["processed_pad_read"])
        self.assertFalse(missing["archive_owner_bound"])
        new = report["replacement"]
        self.assertEqual((new["seat_generation"], new["history_words_cleared"]), (2, 48))
        self.assertTrue(new["reallocated_same_physical_header"])
        self.assertEqual(new["first_paid_input_fork"]["visit"], 83)
        self.assertFalse(new["first_paid_input_fork"]["bomb_factory_ran"])
        self.assertEqual(next(row["visit_after_declared_delivery"] for row in new["restart_observations"]
                              if row["ordinary_receiver_descriptor_mask"] != "0000"), 140)
        self.assertFalse(report["authored_native_control"][1]["actor_replaced"])

    def test_partial_report_and_existing_output_refuse(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            ko.decode("world-ko-open: 174 2 41 90\n")
        with TemporaryDirectory() as tmp:
            out = Path(tmp)/"existing.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                ko.export(None, None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
