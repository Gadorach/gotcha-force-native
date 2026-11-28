"""Protect the distinct body sphere radius, motion envelope and basis gate."""
from hashlib import sha256
import json
import math
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_geometry as geometry


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailBodyGeometryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-body-geometry.json").read_bytes())
        cls.rows = cls.report["models"]

    def test_source_roster_and_roles(self):
        prior = (MAPS / "gg4e08-body-registry.json").read_bytes()
        self.assertEqual(self.report["body_registry_sha256"], sha256(prior).hexdigest())
        old = json.loads(prior)["models"]
        self.assertEqual(len(self.rows), 206)
        self.assertEqual(sum(len(r["sphere_queue_ordinals"]) for r in self.rows), 770)
        self.assertEqual(sum(bool(r["sphere_queue_ordinals"]) for r in self.rows), 184)
        for row, bank in zip(self.rows, old, strict=True):
            self.assertEqual(row["borg"], bank["borg"])
            self.assertEqual(row["sphere_queue_ordinals"], [i for i, d in enumerate(bank["body_descriptor_queue"])
                                                          if d[1] == 0 and int(d[3], 16) & 2])

    def test_original_basis_gate_threshold_and_independent_matrix_sources(self):
        totals = self.report["both_receiver_list_totals_individually"]
        self.assertEqual(self.report["basis_column_threshold_bits"], "3C23D70A")
        self.assertEqual(self.report["declared_views"]["threshold-successor-z"]["matrix_diagonal_bits"][-1], "3C23D70B")
        self.assertEqual([totals[n] for n in ("zero-x", "below-y", "threshold-z", "threshold-successor-z")],
                         [0, 0, 0, 1573])
        self.assertEqual((totals["zero-root-z"], totals["zero-joint-z"]), (1536, 37))
        self.assertEqual(totals["reflect-x"], totals["rigid"])
        for row in self.rows:
            counts = row["receiver_counts_both_lists_individually"]
            self.assertEqual(counts["zero-root-z"] + counts["zero-joint-z"], counts["rigid"])

    def test_transitional_envelope_is_not_a_new_core_radius(self):
        self.assertEqual(self.report["sphere_broad_radius_expansion_counts"], {
            "nonuniform/pass-0": 615, "reflect-x/pass-0": 130,
        })
        self.assertEqual(self.report["sphere_reference_radius_ulp_error_counts"], {"0": 6143, "1": 17})
        local = (4, -8, 3, 22)
        old, new = (260, 120, -61), (264, 124, -52)
        moving = [geometry.bits(v) for v in (*new, *old, 22, 22 + math.dist(old, new))]
        self.assertEqual(geometry.read_sphere(moving, local, 2, 0), (0, True))
        with self.assertRaisesRegex(ValueError, "radius disagrees"):
            geometry.read_sphere(moving[:-1] + [geometry.bits(22)], local, 2, 0)
        settled = [geometry.bits(v) for v in (*new, *new, 22, 22)]
        self.assertEqual(geometry.read_sphere(settled, local, 2, 1), (0, False))

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            geometry.decode("", b"", [])
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "body.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                geometry.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
