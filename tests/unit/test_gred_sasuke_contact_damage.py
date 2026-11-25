"""Pin the measured G-Red -> Sasuke contact amount and its arithmetic."""
import json
import unittest
from pathlib import Path

from tools.analyze import gred_sasuke_contact_damage as contact

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gred-sasuke-contact-damage.json"


class ContactDamageTest(unittest.TestCase):
    def test_combine_reproduces_the_live_amount(self):
        amount, product = contact.combine(20, 0.867, 1.13, 1.15, 1.0)
        self.assertEqual(amount, 22)
        self.assertAlmostEqual(product, 22.533329, places=5)

    def test_combine_truncates(self):
        self.assertEqual(contact.combine(20, 0.5, 1.0, 1.0, 1.0)[0], 10)

    def test_artifact_is_measured_not_generalised(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["amount"], 22)
        self.assertEqual(data["credit"], "gg4e08:0615/0")
        self.assertEqual(data["receiver"], "gg4e08:000a/0")
        self.assertTrue(data["retail_behaviour_verified"])
        self.assertFalse(data["generalises_to_other_pairs"])


if __name__ == "__main__":
    unittest.main()
