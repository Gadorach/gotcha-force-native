"""Protect the distinction between an original wall response and supplied correction."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_correction as stage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageCorrectionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-correction.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_declared_grid_scope_and_exact_source_descriptors(self):
        prior = MAPS / "gg4e08-receiver-correction.json"
        self.assertEqual(self.report["receiver_correction_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual((self.report["grid_role"], self.report["declared_wall_plane"],
                          self.report["stage_contact_sphere_descriptor"], self.report["damage_capsule_descriptor"]),
                         (1, [1, 0, 0, -100], 1, 11))
        self.assertEqual(len(self.rows), 12)
        self.assertTrue(all(r["hp_after_stage_only"] == 180 for r in self.rows.values()))

    def test_tangent_can_set_contact_flags_without_a_nonzero_correction(self):
        outside, tangent, overlap = [self.rows[tag] for tag in ("just-outside", "tangent", "front-overlap")]
        self.assertEqual([r["owner_correction_flag"] for r in (outside, tangent, overlap)], [False, True, True])
        self.assertEqual([r["generated_correction_x"] for r in (outside, tangent, overlap)], [0, 0, 20])
        self.assertEqual(overlap["sphere_and_capsule_broad_center_x"], [150, 150])
        self.assertEqual(overlap["allocated_capsule_endpoints_x"], [115, 145])

    def test_stage_correction_depends_on_the_declared_previous_pose(self):
        crossing, behind = [self.rows[tag] for tag in ("cross-plane", "stationary-behind")]
        self.assertEqual((crossing["declared_pose_x_samples"], behind["declared_pose_x_samples"]),
                         ([170, 90], [90, 90]))
        self.assertEqual((crossing["generated_correction_x"], behind["generated_correction_x"]), (60, 0))
        self.assertEqual((crossing["post_solver_root_x"], behind["post_solver_root_x"]), (150, 90))

    def test_stage_response_admits_one_hp_write_and_removes_another(self):
        for side, x, expected in (("left", 40, [17, 0]), ("right", 240, [0, 17])):
            rows = [self.rows[f"attack-{side}-{stage}"] for stage in ("empty", "wall")]
            self.assertEqual([r["generated_correction_x"] for r in rows], [0, 20])
            pairs = [r["ordinary_pair"] for r in rows]
            self.assertEqual([r["tau_center_x"] for r in pairs], [x, x])
            self.assertEqual([r["hp_removed"] for r in pairs], expected)
            self.assertEqual([r["sphere_capsule_kernel_reached"] for r in pairs], [bool(n) for n in expected])
        moved = self.rows["attack-right-wall"]["ordinary_pair"]
        self.assertEqual((moved["scratch_endpoints_x"], moved["hp_after"], moved["accepted_token"]),
                         ([135, 165], 163, 1))

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            stage.decode("")
        with self.assertRaisesRegex(ValueError, "changed at row 1"):
            stage.decode("stage-correction: invalid\n"*28)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "stage.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                stage.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
