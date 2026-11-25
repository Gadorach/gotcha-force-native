import unittest

from tools.analyze.material_stage_inventory import aggregate


class MaterialStageInventoryTest(unittest.TestCase):
    def test_aggregate_counts_bits_per_mobj_and_per_model(self):
        entries = [
            ("m1", {"mobj_render_mode_unknown_bits": {"00000010": 4},
                    "textures": {"128x128/CMPR": 2},
                    "vtx_attrs": {"a": 1},
                    "pobj_obj_types": {"skin": 1}}),
            ("m2", {"mobj_render_mode_unknown_bits": {"00000010": 1, "00000040": 2},
                    "textures": {"64x64/RGB5A3": 3},
                    "vtx_attrs": {"a": 2},
                    "pobj_obj_types": {"envelope": 1}}),
        ]
        out = aggregate(entries)
        self.assertEqual(out["models"], 2)
        self.assertEqual(out["models_with_unknown_render_bits"], 2)
        self.assertEqual(out["unknown_render_bits"]["00000010"], {"mobjs": 5, "models": 2})
        self.assertEqual(out["unknown_render_bits"]["00000040"], {"mobjs": 2, "models": 1})
        self.assertEqual(out["texture_formats"], {"64x64/RGB5A3": 3, "128x128/CMPR": 2})
        self.assertEqual([entry["model"] for entry in out["per_model"]], ["m1", "m2"])

    def test_aggregate_ignores_models_without_unknown_bits_in_the_list(self):
        out = aggregate([("clean", {"textures": {"128x128/CMPR": 1}}),
                         ("dirty", {"mobj_render_mode_unknown_bits": {"00000010": 2}})])
        self.assertEqual(out["models_with_unknown_render_bits"], 1)
        self.assertEqual([entry["model"] for entry in out["per_model"]], ["dirty"])

    def test_aggregate_states_that_it_carries_no_pixels(self):
        out = aggregate([("m1", {"textures": {"128x128/CMPR": 1}})])
        self.assertIn("no pixel data", out["kind"])


if __name__ == "__main__":
    unittest.main()
