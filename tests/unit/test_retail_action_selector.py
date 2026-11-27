"""Check original action-cell semantics and the roster-wide selector report."""
import json
from pathlib import Path
import unittest

from tools.analyze import retail_action_selector as selector


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailActionSelectorTest(unittest.TestCase):
    def test_flat_and_sector_tables_do_not_share_row_choices(self):
        self.assertEqual(selector.possible_choices(-1), [])
        self.assertEqual(selector.possible_choices(0), [0, 1, 2, 3, 4])
        self.assertEqual(selector.possible_choices(1), [0, 1, 2, 3, 4, 5])
        choose = selector.normal_choice
        self.assertEqual(choose(0, 0x180, 1, 4, False), 4)
        self.assertEqual(choose(1, 0x180, 1, 4, False), 5)
        self.assertEqual(choose(0, 0x20, -1, 4, False), 1)
        self.assertEqual(choose(1, 0x20, -1, 4, False), 2)
        self.assertEqual(choose(0, 0x200, 0, 4, False), 2)
        self.assertEqual(choose(1, 0x10, 0, 4, False), 3)
        self.assertEqual(choose(0, 0x200, 0, 0, True), 3)
        self.assertIsNone(choose(1, 0x200, 0, 0, True))
        self.assertIsNone(choose(0, 0x80, 0, 0, True))

    def test_sector_signed_comparisons_preserve_edges(self):
        actual = [selector.sector_from_angle(angle, -10, 10, -30, 30)
                  for angle in (None, -31, -30, -11, -10, 0, 10, 11, 30, 31)]
        self.assertEqual(actual, [0, 2, 3, 3, 0, 0, 0, 1, 1, 2])

    def test_published_report_matches_every_atlas_profile(self):
        atlas = json.loads((MAPS / "gg4e08-code-atlas.json").read_bytes())
        result = json.loads((MAPS / "gg4e08-action-selector.json").read_bytes())
        self.assertEqual(atlas["symbol_metadata_sha256"], result["symbol_metadata_sha256"])
        self.assertEqual(atlas["source_dol_sha256"], result["source_dol_sha256"])
        counts, cells = selector.review_cells(atlas["borgs"], atlas["profiles"])
        self.assertEqual(result["counts"], counts)
        self.assertEqual(result["exceptional_cells"], cells)
        self.assertEqual(counts["borgs"], 206)
        self.assertEqual(counts["not_selected_by_normal_lookup"], counts["groups_mode_0"])
        flat_six = [cell for cell in cells if cell["excluded_by_normal_choice"]]
        self.assertTrue(all((cell["choice"], cell["sector"]) == (5, 0) for cell in flat_six))
        zero = [cell for cell in cells if cell["classification"] == "zero-row"]
        self.assertEqual(len(zero), 13)
        self.assertEqual({c["id"] for c in zero}, {"gg4e08:0205", "gg4e08:0207"})
        self.assertEqual([(c["id"], c["choice"]) for c in zero if c["excluded_by_normal_choice"]],
                         [("gg4e08:0207", 5)])
        dispatch = result["tick_dispatch"]
        first = dispatch["tables"]["first"]["targets"]
        second = dispatch["tables"]["second"]["targets"]
        self.assertEqual((len(first), len(second)), (64, 64))
        self.assertTrue(all(addr in atlas["functions"] for addr in first + second))
        self.assertEqual((first[61], first[1], second[0]), ("80065DFC", "8005F23C", "8005F238"))
        self.assertEqual([item["lane_marker"] for item in dispatch["order"]], [2, 1, 0, 2])
