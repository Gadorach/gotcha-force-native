"""Keep knockout accounting, phase exit, and actual body release distinct."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_phase_exit as exit_replay


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailBodyPhaseExitTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((MAPS / "gg4e08-body-phase-exit.json").read_bytes())
        cls.rows = {(r["area_key"], r["case"]): r for r in cls.data["cases"]}

    def test_fresh_zero_crossing_not_zero_hp_alone_starts_exit(self):
        self.assertEqual(self.data["knockout_sha256"], sha256((MAPS / "gg4e08-body-knockout.json").read_bytes()).hexdigest())
        self.assertEqual(len(self.rows), 10)
        for key in (19, 20):
            started = self.rows[key, "unmasked-lethal"]
            self.assertEqual((started["hp_after_walk"], started["fresh_receive_latch"], started["force_after"]), (0, True, 0))
            self.assertTrue(started["phase2_initializer_reached"])
            self.assertEqual((started["receive_registry_after_rebuild"],
                              started["receiving_shape_still_allocated_after_rebuild"]), (0, True))
            retained = self.rows[key, "retained-zero-without-fresh-hit"]
            self.assertEqual((retained["hp_after_walk"], retained["fresh_receive_latch"], retained["retained_token"]), (0, False, 2))
            self.assertFalse(retained["phase2_initializer_reached"])
            for case, hp in (("scene-holds-hp", 1), ("nonlethal", 163)):
                other = self.rows[key, case]
                self.assertEqual((other["hp_after_walk"], other["dispatch_phase_after_walk"]), (hp, 1))

    def test_refreshed_hp_and_force_mirrors_kept_separate(self):
        for key in (19, 20):
            row = self.rows[key, "unmasked-lethal"]
            self.assertEqual((row["hp_request"], row["force_before"], row["force_capacity"]), (16, 140, 200))
            self.assertEqual(row["calculator_control"], {"hp_mirror": 1, "force_mirror": 140,
                             "hp_mirror_180_request": 17, "force_mirror_200_request": 17, "both_restored_request": 17})

    def test_mask_defers_countdown_removal_flag_and_phase3(self):
        for key in (19, 20):
            for mask in (False, True):
                row = self.rows[key, "masked-lethal" if mask else "unmasked-lethal"]
                self.assertEqual((row["exit_timer_after_walk"], row["masked_actor_visits"]), (29+mask, 2*mask))
                samples = {r["unmasked_visit"]: r for r in row["clock_samples"]}
                self.assertTrue(samples[30+mask]["type13_key0_factory_requested"])
                self.assertFalse(samples[30+mask]["type13_allocated"])
                self.assertTrue(samples[46+mask]["body_remove_flag"])
                self.assertEqual(samples[46+mask]["actor_phase"], 2)
                self.assertEqual(samples[47+mask]["actor_phase"], 3)
                self.assertTrue(samples[47+mask]["actor_cell_active"])

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "changed at row"):
            exit_replay.decode("body-exit: invalid\n"*46)
        with TemporaryDirectory() as name:
            path = Path(name) / "phase-exit.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                exit_replay.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
