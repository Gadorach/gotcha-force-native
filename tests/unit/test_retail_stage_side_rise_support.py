from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_side_rise_support as rise


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageSideRiseSupportTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-side-rise-support.json").read_bytes())
        cls.archives = {a["filename"]: a for a in cls.report["archives"]}
        cls.probes = [p for a in cls.archives.values() for p in a["probes"]]

    def test_pins_stationary_support_lifecycle_and_owner_carry(self):
        for stem in ("stage-side-support", "stage-side-recovery", "support-carry"):
            raw = (MAPS / f"gg4e08-{stem}.json").read_bytes()
            self.assertEqual(self.report[stem.replace("-", "_")+"_sha256"], sha256(raw).hexdigest())
        self.assertEqual(self.report["observed_trace_sha256"], rise.TRACE_SHA256)
        self.assertEqual((self.report["declared_break_hp"], self.report["rebind_visit"]), (0, 600))

    def test_timed_boxes_and_planes_exclude_terminal_side_owners(self):
        self.assertEqual(tuple(self.archives), rise.side.NAMES)
        self.assertEqual(self.report["probe_counts"], {"box": 88, "plane": 84, "no_rebind_terminal_owners": 8})
        self.assertEqual(Counter(p["shape_kind"] for p in self.probes), {1: 88, 4: 84})
        self.assertEqual(self.archives["set0904.arc"]["probes"], [])
        for p in self.probes:
            self.assertNotEqual(p["retired_support_cell_ordinal"], p["rebound_support_cell_ordinal"])

    def test_carry_precedes_geometry_catchup_and_survives_next_rise(self):
        self.assertEqual(self.report["unmasked_rise_steps_checked"], 344)
        self.assertEqual(self.report["first_rise"], {
            "allocated_center_held": True, "owner_matrix_moves": True, "carry_matches_height": True,
            "masked_carry_root_held": True, "masked_landing_matches_height": True,
        })
        self.assertEqual(self.report["second_rise"], {"allocated_center_catches_up": True, "carry_matches_height": True})
        self.assertEqual(self.report["actor_update_stop_before"], "80059E58")

    def test_masked_carry_is_separate_from_supported_landing(self):
        self.assertEqual(self.report["masked_first_steps_checked"], 172)
        self.assertTrue(self.report["first_rise"]["masked_carry_root_held"])
        self.assertTrue(self.report["first_rise"]["masked_landing_matches_height"])

    def test_original_float_edges_are_not_flattened(self):
        self.assertEqual(Counter((p["shape_kind"], p["initial_collision_root_above_query_bits"]) for p in self.probes),
                         {(1, "00000000"): 88, (4, "37800000"): 84})
        drift = [p["unmasked_actor_lateral_delta_bits"] for p in self.probes
                 if any(d != "00000000" for step in p["unmasked_actor_lateral_delta_bits"] for d in step)]
        self.assertEqual(drift, [[["B8800000", "00000000"], ["00000000", "00000000"]]])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            rise.decode("", [], [], {}, b"")
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "side-rise.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                rise.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
