from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type80_link as link


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailType80LinkTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-type80-link.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_exact_source_group_and_busy_branch(self):
        prior = (MAPS / "gg4e08-type25-link.json").read_bytes()
        self.assertEqual(self.report["type25_link_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["owner"], self.report["launch_socket"], self.report["opening_group"],
                          self.report["opening_descriptor"]), ("gg4e08:0d02", 1, 2, 2))
        self.assertEqual(self.report["busy_wait_frontier"],
                         {"entry": "8006A53C", "call_site": "80136830", "r4": 10})

    def test_owner_pointer_is_distinct_from_zero_damage_contact_and_masks(self):
        hit = self.rows["accepted-stretch-and-expiry"]
        self.assertTrue(hit["pair"]["hp_writer_reached"])
        self.assertEqual((hit["pair"]["hp_removed"], hit["pair"]["accepted_token"]), (0, 1))
        pair, acquired, unmoved, stretched, expired, *after = hit["checkpoints"]
        self.assertEqual([v["owner_retains_target_144"] for v in (pair, acquired, expired)], [False, True, True])
        self.assertEqual([v["masks_71f_owner_target"] for v in (acquired, unmoved, stretched, expired)],
                         [[0, 0], [0, 0], [2, 1], [0, 0]])
        self.assertTrue(all(v["owner_busy_14a"] for v in hit["checkpoints"]))
        self.assertEqual(acquired["shape_free_count"], 1)
        self.assertEqual(len(after), 4)
        for v in after:
            self.assertEqual((v["carrier_phase"], v["carrier_link_subphase"]), (1, 2))
            self.assertTrue(v["owner_retains_target_144"])
            self.assertFalse(v["free_carrier_count"])

    def test_excluded_receiver_retention_is_not_owner_target_publication(self):
        row = self.rows["target-excluded-after-hit"]
        self.assertTrue(row["pair"]["hp_writer_reached"])
        visit = row["checkpoints"][-1]
        self.assertFalse(visit["owner_retains_target_144"])
        self.assertTrue(visit["carrier_retains_target"])
        self.assertEqual((visit["masks_71e_owner_target"], visit["masks_71f_owner_target"]), ([0, 0], [0, 0]))

    def test_allocation_and_shape_refusal_return_distinct_resources(self):
        for name, expected in (("carrier-pool-empty", (0, 1)), ("view-pool-empty", (1, 0))):
            visit, = self.rows[name]["checkpoints"]
            self.assertFalse(visit["owner_busy_14a"])
            self.assertFalse(visit["carrier_live"])
            self.assertEqual((visit["free_carrier_count"], visit["free_view_count"]), expected)
        visits = self.rows["shape-pool-empty"]["checkpoints"]
        self.assertEqual([v["carrier_phase"] for v in visits], [2, 3, 3])
        self.assertEqual([v["carrier_live"] for v in visits], [True, True, False])
        self.assertFalse(any(v["owner_busy_14a"] for v in visits))
        self.assertEqual((visits[-1]["free_carrier_count"], visits[-1]["free_view_count"]), (1, 1))

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            link.decode("")
        with self.assertRaisesRegex(ValueError, "miss/type80-pair"):
            link.decode("type80-pair: invalid\n"*21)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "type80.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                link.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
