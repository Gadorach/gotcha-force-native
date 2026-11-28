from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_st42_wall as wall


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSt42WallTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-st42-wall.json").read_bytes())
        cls.sites = cls.report["sites"]

    def test_exact_archive_and_distinct_grid_roles(self):
        prior = (MAPS / "gg4e08-stage-collision-walk.json").read_bytes()
        self.assertEqual(self.report["stage_collision_walk_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["afs_member"], self.report["afs_member_name"]), (473, "st42.pzz"))
        self.assertEqual([g["triangle_count"] for g in self.report["stih_members"]], [2799, 1816, 348])
        self.assertEqual([(s["axis"], s["outward_sign"]) for s in self.sites], [("x", 1), ("x", -1), ("z", 1)])

    def test_vertex_tangent_differs_from_stored_plane_and_has_one_step_edge(self):
        for site in self.sites:
            rows = {r["case"]: r for r in site["cases"]}
            exact, outside, inside = (rows[f"vertex-{tag}"] for tag in ("tangent", "one-step-out", "one-step-in"))
            axis = "xyz".index(site["axis"])
            tangent = int(site["vertex_tangent_bits"], 16)
            sign = site["outward_sign"]
            self.assertEqual([int(v["pre_walk_root_bits"][axis], 16) for v in (exact, outside, inside)],
                             [tangent, tangent+sign, tangent-sign])
            self.assertEqual([v["stage_contact_flag"] for v in (exact, outside, inside)], [True, False, True])
            self.assertEqual(exact["post_walk_displacement_bits"], ["00000000"]*3)
            self.assertEqual(inside["post_walk_root_bits"], exact["post_walk_root_bits"])
            self.assertNotEqual(inside["post_walk_displacement_bits"][axis], "00000000")
            self.assertNotEqual(rows["stored-plane-tangent"]["pre_walk_root_bits"], exact["pre_walk_root_bits"])
        self.assertEqual([s["cases"][2]["stage_contact_flag"] for s in self.sites], [False, True, False])

    def test_empty_secondary_and_stationary_behind_are_separate_controls(self):
        for site in self.sites:
            empty, _, _, hit, cross, behind, *_ = site["cases"]
            self.assertEqual(empty["pre_walk_root_bits"], hit["pre_walk_root_bits"])
            self.assertEqual(cross["pre_walk_root_bits"], behind["pre_walk_root_bits"])
            self.assertEqual([v["stage_contact_flag"] for v in (empty, hit, cross, behind)], [False, True, True, False])
            self.assertEqual(hit["post_walk_root_bits"], cross["post_walk_root_bits"])
            self.assertTrue(all(v["hp_after"] == 180 for v in site["cases"]))

    def test_stationary_projection_observes_selected_source_record(self):
        for site in self.sites:
            hit = site["cases"][3]
            self.assertIn(site["reference_triangle_ordinal"], hit["first_eight_projection_operands_from_face_path"])
            self.assertGreaterEqual(hit["projection_function_calls"], len(hit["first_eight_projection_operands_from_face_path"]))
            cross = site["cases"][4]
            self.assertTrue(cross["projection_function_calls"])
            self.assertTrue(all(x is None for x in cross["first_eight_projection_operands_from_face_path"]))

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            wall.decode("")
        with self.assertRaisesRegex(ValueError, "1549/empty-secondary"):
            wall.decode("st42-wall: invalid\n"*54)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "st42.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                wall.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
