"""Keep Sasuke's direct and area hits, registry gate, and disposal distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_area as area


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-area.json"
REPORT = """\
bomb-area: 0 0 0 0.0000 0.0000 0.0000 0 0 172 0 2 1
bomb-area: 1 1 2 3.1000 34.1000 387.5000 17 2 155 17 50 1
"""


class RetailBombAreaTest(unittest.TestCase):
    def test_receipt_separates_shape_binding_from_attack_and_retirement(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], area.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], area.AFS_SHA256)
        common = data["source"]["common_collision"]
        self.assertEqual((common["member"], common["group"], common["descriptor"], common["raw_radius"]),
                         (696, 3, 4, 310))
        self.assertEqual(data["source"]["reaction"]["record"], "802C4778")
        self.assertEqual(data["cases"], area.decode_report(REPORT))
        empty, bound = data["cases"]
        self.assertIsNone(empty["child_token"])
        self.assertIsNone(empty["child_audio_request"])
        self.assertTrue(empty["one_shape_becomes_free_after_refused_bind"])
        self.assertEqual(empty["carrier_and_both_descriptors_return_visit"], 2)
        self.assertEqual(empty["receiver_hp_after_child"], 172)
        self.assertTrue(bound["bound_shape"])
        self.assertEqual(bound["initial_attack_registry_count"], 0)
        self.assertEqual(bound["initial_bound_radius"], 3.1)
        self.assertEqual(bound["growth_visit_one"], {
            "radius": 34.1, "attack_registry_count": 1, "received_record": "802C4778",
        })
        self.assertEqual(bound["receiver_hit_tokens"], [1, 2])
        self.assertEqual(bound["duplicate_checks_without_another_write"], 2)
        self.assertEqual(bound["credited_and_received_damage"], 25)
        self.assertEqual(bound["receiver_hp_after_child"], 155)
        self.assertEqual(bound["shape_unlink_visit"], 17)
        self.assertEqual(bound["carrier_and_both_descriptors_return_visit"], 50)
        self.assertEqual((empty["stock_after"], bound["stock_after"]), (1, 1))

    def test_refuses_wrong_token_radius_damage_or_disposal(self):
        for report in (
            REPORT*2,
            REPORT.replace("1 1 2 3.1000", "1 1 1 3.1000"),
            REPORT.replace("34.1000", "310.0000"),
            REPORT.replace("17 2 155", "0 0 172"),
            REPORT.replace("17 50 1", "17 17 1"),
            REPORT.replace("172 0 2 1", "172 0 2 2"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                area.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                area.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
