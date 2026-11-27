"""Do not conflate the type-60 carrier, HP writer or its type-33 child."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type60_contact as probe


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type60-contact.json"


class RetailType60ContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["borg"], r["case"]): r for r in cls.data["cases"]}

    def test_four_models_and_eight_distinct_release_sites(self):
        sources = self.data["sources"]
        self.assertEqual([r["key"] for r in sources], [0, 2, 1, 3])
        self.assertEqual(len({r[field] for r in sources for field in ("release_site", "static_second_release_site")}), 8)
        self.assertTrue(all((r["queue_group"], r["shape_step"], r["raw_hp_operand"]) == (6, 7, 0) for r in sources))

    def test_ally_and_opponent_get_the_model_specific_effect_without_hp_loss(self):
        for source in self.data["sources"]:
            for relation in ("ally", "opponent"):
                r = self.rows[source["borg"], relation]
                self.assertEqual(r["type_33_arguments"], [source["effect_arm"] // 4, 1])
                self.assertEqual(r["type_33_owner"], "struck-sasuke")
                self.assertEqual((r["amount_calls"], r["hp_writer_argument"], r["receiver_hp_after"]), (1, 0, 180))

    def test_resource_refusal_does_not_reassign_or_repeat_contact(self):
        for source in self.data["sources"]:
            key = source["borg"]
            for cause in ("effect-cell-refused", "effect-descriptors-refused"):
                r = self.rows[key, cause]
                self.assertTrue(r["receiver_linked"] and r["retained_pair_refused"])
                self.assertIsNotNone(r["type_33_arguments"])
                self.assertIsNone(r["type_33_owner"])
            for cause in ("shape-refused", "carrier-cell-refused", "carrier-descriptors-refused"):
                r = self.rows[key, cause]
                self.assertEqual(r["stock_spent"], 1)
                self.assertFalse(r["ordinary_shape_published"])
                self.assertIsNone(r["type_33_arguments"])
            for cause in ("wrong-event", "stock-refused"):
                self.assertFalse(self.rows[key, cause]["carrier_requested"])

    def test_rejects_incomplete_or_oversized_receipt(self):
        for value in ("type60: 2306", "x" * 4097):
            with self.assertRaises(ValueError):
                probe.decode(value)

    def test_never_replaces_output(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "type60.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                probe.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
