"""Keep the Saucer carrier hit separate from the floor effect and contact area."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_saucer_contact as saucer


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-saucer-contact.json"
REPORT = """\
saucer-contact: 0 0 180 0 0 0 0 0 160.00000 200.00000 900.00000 0.00000 0.00000 0.00000
saucer-contact: 1 26 154 1 0 0 1 1 160.00000 200.00000 900.00000 160.00000 200.00000 900.00000
saucer-contact: 2 0 180 0 1 1 0 0 160.00000 200.00000 900.00000 0.00000 0.00000 0.00000
saucer-contact: 3 26 154 1 1 1 1 1 160.00000 200.00000 900.00000 160.00000 200.00000 900.00000
saucer-contact: 4 26 154 1 1 1 1 0 160.00000 200.00000 900.00000 160.00000 200.00000 900.00000
saucer-contact: 5 26 154 1 1 1 1 0 160.00000 200.00000 900.00000 160.00000 200.00000 900.00000
"""


class RetailSaucerContactTest(unittest.TestCase):
    def test_contact_and_floor_allocate_independently(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["cases"], saucer.decode_report(REPORT))
        self.assertEqual(data["source"]["amount_record"], "80362C40")
        miss, hit, floor, both, _, _ = data["cases"]
        self.assertEqual(miss["followup_allocation_order"], [])
        self.assertEqual(hit["followup_allocation_order"], ["actor-contact-area"])
        self.assertEqual(floor["followup_allocation_order"], ["terrain-effect"])
        self.assertIsNone(floor["actor_contact_area"])
        self.assertEqual(floor["receiver_hp_after"], 180)
        self.assertEqual(both["followup_allocation_order"], ["terrain-effect", "actor-contact-area"])
        self.assertEqual(both["terrain_effect"], {
            "type": 9, "key": 4, "allocated": True, "origin": [160, 0, 400], "parent": "credited-actor",
        })
        self.assertEqual(both["actor_contact_area"], {
            "key": 0, "allocated": True, "origin": [160, 200, 900], "parent": "type-32-carrier",
        })

    def test_area_refusal_keeps_contact_point_and_completed_carrier_damage(self):
        rows = saucer.decode_report(REPORT)
        for row in (rows[1], *rows[3:]):
            self.assertEqual((row["hp_loss"], row["receiver_hp_after"], row["carrier_token"]), (26, 154, 1))
            self.assertTrue(row["duplicate_token_refused"])
        for row in rows[4:]:
            self.assertEqual(row["followup_allocation_order"], ["terrain-effect"])
            self.assertFalse(row["actor_contact_area"]["allocated"])
            self.assertEqual(row["actor_contact_area"]["origin"], row["saved_actor_contact_point"])

    def test_report_rejects_aliased_points_and_impossible_area_admission(self):
        for old, new in (
            ("160.00000 200.00000 900.00000\n", "160.00000 0.00000 400.00000\n"),
            ("4 26 154 1 1 1 1 0", "4 26 154 1 1 1 1 1"),
            ("2 0 180 0 1 1 0 0", "2 26 154 1 1 1 1 1"),
        ):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                saucer.decode_report(REPORT.replace(old, new, 1))

    def test_export_refuses_to_replace_a_receipt(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "saucer.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                saucer.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
