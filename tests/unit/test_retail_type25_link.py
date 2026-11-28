from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type25_link as link


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailType25LinkTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-type25-link.json").read_bytes())

    def test_source_groups_are_not_socket_indices(self):
        prior = (MAPS / "gg4e08-linked-correction.json").read_bytes()
        self.assertEqual(self.report["linked_correction_sha256"], sha256(prior).hexdigest())
        self.assertEqual([(r["borg"], r["key"], r["opening_group"], r["launch_socket"], r["shape_descriptor"])
                          for r in self.report["pilots"]],
                         [("gg4e08:0201", 0, 0, 11, 25), ("gg4e08:0809", 1, 0, 12, 0)])

    def test_zero_damage_contact_is_distinct_from_acquisition_and_stretch(self):
        for row in self.report["pilots"]:
            hit = row["cases"][1]
            self.assertTrue(hit["hp_writer_reached"])
            self.assertEqual((hit["requested_damage"], hit["target_hp_after"], hit["accepted_token"]), (0, 180, 1))
            pair, acquired, unmoved, stretched, expired = hit["checkpoints"]
            self.assertEqual([r["masks_71e_owner_target"] for r in hit["checkpoints"]],
                             [[0, 0], [2, 1], [2, 1], [2, 1], [0, 0]])
            self.assertEqual([r["masks_71f_owner_target"] for r in hit["checkpoints"]],
                             [[0, 0], [0, 0], [0, 0], [2, 1], [0, 0]])
            self.assertEqual((unmoved["motion_request_counts_owner_target"], stretched["motion_request_counts_owner_target"]),
                             ([0, 0], [1, 1]))
            self.assertTrue(pair["ordinary_attack_published"])
            self.assertFalse(acquired["ordinary_attack_published"])
            self.assertEqual(acquired["shape_free_count"], 1)
            self.assertEqual([r["timer"] for r in (acquired, unmoved, stretched, expired)], [600, 599, 598, 0])
            self.assertEqual(expired["carrier_link_subphase"], 2)
            self.assertTrue(expired["carrier_live"])

    def test_miss_and_excluded_recipient_do_not_publish_link(self):
        for row in self.report["pilots"]:
            miss, _, excluded = row["cases"]
            self.assertFalse(miss["hp_writer_reached"])
            self.assertTrue(excluded["hp_writer_reached"])
            m, e = [r["checkpoints"][-1] for r in (miss, excluded)]
            self.assertEqual([r["masks_71e_owner_target"] for r in (m, e)], [[0, 0]]*2)
            self.assertEqual([r["masks_71f_owner_target"] for r in (m, e)], [[0, 0]]*2)
            self.assertEqual((m["carrier_link_subphase"], e["carrier_link_subphase"]), (0, 2))
            self.assertTrue(e["retains_target_pointer"])
            self.assertFalse(e["ordinary_attack_published"])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            link.decode("")
        with self.assertRaisesRegex(ValueError, "miss/type25-pair"):
            link.decode("type25-pair: invalid\n"*24)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "type25.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                link.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
