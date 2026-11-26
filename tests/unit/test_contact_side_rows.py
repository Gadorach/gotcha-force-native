"""Pin the side-paired contact rows."""
import json
import unittest
from pathlib import Path

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/contact-side-rows.json"


class ContactSideRowsTest(unittest.TestCase):
    def test_variants(self):
        data = json.loads(ARTIFACT.read_text())
        variants = {row["team"]: row for row in data["variants"]}
        self.assertEqual(variants[0]["credit_difficulty"], 1.02)
        self.assertEqual(variants[0]["credit_rank"], 0.85)
        self.assertEqual(variants[0]["receiver_rank"], 1.15)
        self.assertEqual(variants[1]["credit_difficulty"], 0.92)
        self.assertEqual(variants[1]["credit_rank"], 0.85)
        self.assertEqual(variants[1]["receiver_rank"], 1.15)


if __name__ == "__main__":
    unittest.main()
