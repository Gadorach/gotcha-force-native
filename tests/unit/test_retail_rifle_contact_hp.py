"""Verify the published distinction between contact, reaction and damage."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_rifle_contact_hp as rifle


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-rifle-contact-hp.json"
PREFIX = rifle.RELEASE + "\n" + rifle.PUBLISHED + "\n"
REPORT = "".join(PREFIX + line + "\n" for line in (
    "rifle-contact: 256 0 0 0 0 180 65535 0 0 0 0 0 0 00000000",
    "rifle-contact: 0 1 0 0 0 180 65535 0 0 0 1 0 0 00000000",
    "rifle-contact: 0 0 0 1 8 172 180 8 8 16 1 1 2 802C3494",
    "rifle-contact: 0 0 1 1 0 180 180 0 0 16 1 1 2 00000000",
))


class RetailRifleContactHpTest(unittest.TestCase):
    def test_public_receipt_uses_the_rifle_carrier_and_credits_machine_red(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], rifle.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], rifle.AFS_SHA256)
        self.assertEqual(data["attack"]["actor_spec_sha256"], rifle.ACTOR_SHA256[0x0600])
        self.assertEqual(data["receiver"]["actor_spec_sha256"], rifle.ACTOR_SHA256[0x000A])
        self.assertEqual(data["attack"]["raw_attack_power"], 10)
        self.assertEqual(data["cases"], rifle.decode_report(REPORT))
        miss, duplicate, hit, masked = data["cases"]
        self.assertFalse(miss["contact_accepted"])
        self.assertEqual(duplicate["receiver_token_cursor"], 0)
        self.assertEqual((hit["remaining_hp"], hit["credited_damage"]), (172, 8))
        self.assertTrue(masked["contact_accepted"])
        self.assertEqual((masked["remaining_hp"], masked["previous_hp_field"]), (180, 180))
        self.assertIsNone(masked["receiver_reaction_curve"])

    def test_refuses_mismatched_prefix_and_unqualified_outcome(self):
        for altered in (
            "",
            REPORT + REPORT,
            REPORT.replace("49 1 1 -149", "49 1 0 -149", 1),
            REPORT.replace("0 1 0 0 0 180", "0 1 0 1 0 180"),
            REPORT.replace("8 172 180 8 8", "10 170 180 10 10"),
            REPORT.replace("180 180 0 0 16 1 1", "180 65535 0 0 0 0 0"),
            REPORT.replace("802C3494", "00000000"),
        ):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                rifle.decode_report(altered)

    def test_existing_export_output_is_never_overwritten(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "existing.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                rifle.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
