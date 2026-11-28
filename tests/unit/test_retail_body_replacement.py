"""Pin exact replacement delivery, initialization, and receiving shape gates."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_replacement as replacement


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailBodyReplacementTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((MAPS/"gg4e08-body-replacement.json").read_bytes())
        cls.rows = {(r["area_key"], r["shape_cells_offered"]): r for r in cls.data["cases"]}

    def test_exact_role_bytes_and_old_receiving_shape_boundary(self):
        self.assertEqual(self.data["phase_cleanup_sha256"],
                         sha256((MAPS/"gg4e08-body-phase-cleanup.json").read_bytes()).hexdigest())
        self.assertEqual(self.data["replacement_member"], 504)
        self.assertEqual([(r["role"], r["bytes"], r["sha256"]) for r in self.data["delivered_roles"]],
                         list(replacement.ASSETS))
        self.assertEqual(len(self.rows), 4)
        for row in self.rows.values():
            self.assertTrue(row["unallocated_archive_owner_refused"])
            self.assertTrue(row["old_receiving_shape_returned_before_reentry"])

    def test_replacement_restores_actor_stats_but_not_the_force_pool(self):
        for row in self.rows.values():
            self.assertEqual((row["seat"], row["actor_phase"], row["generation_delta"], row["hp"]), (1, 1, 1, 220))
            self.assertEqual(row["stock_channels"], {"0": 10, "2": 3})
            self.assertEqual((row["receiving_force"], row["old_damage_ledger"], row["collision_history_cleared_entries"]),
                             (0, 16, 48))
            self.assertEqual((row["source_view_records"], row["view_wrappers"]), (4, 8))

    def test_allocated_shapes_do_not_imply_unmasked_receivers(self):
        for key in (19, 20):
            for offered in (0, 16):
                row = self.rows[key, offered]
                self.assertEqual((row["body_shapes_bound"], row["initial_role_mask"],
                                  row["ordinary_receiving_shapes_published"]), (offered, "00EF", 0))
                self.assertEqual(row["receiving_steps_with_only_bit_02_cleared"],
                                 list(replacement.RECEIVING_STEPS) if offered else [])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "receive list changed"):
            replacement.decode("body-replacement: invalid\n"*4)
        with TemporaryDirectory() as name:
            path = Path(name) / "replacement.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                replacement.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
