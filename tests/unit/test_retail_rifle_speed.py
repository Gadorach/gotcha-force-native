"""Pin movement-before-acceleration and the source speed cap."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_rifle_speed as speed


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-rifle-speed.json"
REPORT = speed.RELEASE + "\n" + speed.PUBLISHED + """
rifle-speed: 1 80.00 80.50 80.00000 299
rifle-speed: 2 80.50 81.00 80.50000 298
rifle-speed: 159 159.00 159.50 159.00000 141
rifle-speed: 160 159.50 160.00 159.50000 140
rifle-speed: 161 160.00 160.00 160.00000 139
rifle-speed: 162 160.00 160.00 160.00000 138
"""


class RetailRifleSpeedTest(unittest.TestCase):
    def test_receipt_keeps_used_speed_separate_from_next_speed(self):
        receipt = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(receipt["source_dol_sha256"], speed.SOURCE_SHA256)
        self.assertEqual(receipt["source_afs_sha256"], speed.AFS_SHA256)
        self.assertEqual(receipt["speed"], {"initial": 80, "gain": 0.5, "cap": 160})
        self.assertEqual(receipt["checked_visits"], 162)
        self.assertEqual(receipt["samples"], speed.decode_report(REPORT))
        samples = {row["visit"]: row for row in receipt["samples"]}
        self.assertEqual((samples[160]["position_delta_z"], samples[160]["speed_after"]), (159.5, 160))
        self.assertEqual(samples[161]["position_delta_z"], 160)

    def test_refuses_early_gain_and_exceeding_cap(self):
        for altered in ("", REPORT + REPORT, REPORT.replace("80.00000", "80.50000"),
                        REPORT.replace("161 160.00 160.00", "161 160.00 160.50"),
                        REPORT.replace("162 160.00 160.00", "163 160.00 160.00")):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                speed.decode_report(altered)

    def test_does_not_overwrite_an_existing_artifact(self):
        with TemporaryDirectory() as tmp:
            p = Path(tmp) / "receipt.json"
            p.write_text("preserved")
            with self.assertRaises(FileExistsError):
                speed.export(None, None, None, None, p)
            self.assertEqual(p.read_text(), "preserved")
