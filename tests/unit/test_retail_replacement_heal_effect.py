"""Keep admitted heal effects separate from the receiving mask and HP owner."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_replacement_heal_effect as effect


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailReplacementHealEffectTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-replacement-heal-effect.json").read_bytes())
        cls.cases = {(r["previous_ko_source"], r["heal_on_replacement_visit"], r["declared_owner_bit_clear_visit"]): r
                     for r in cls.report["cases"]}

    def test_exact_source_parent_and_resource_scope(self):
        for name in ("replacement-heal-tail", "contact-effect-source-art"):
            path = MAPS / f"gg4e08-{name}.json"
            self.assertEqual(self.report[name.replace("-", "_")+"_sha256"], sha256(path.read_bytes()).hexdigest())
        self.assertEqual((self.report["effect_type"], self.report["selector"], self.report["owner_seat"]), (21, 1, 1))
        self.assertEqual(self.report["receiver_hp_after"], 160)
        self.assertEqual(len(self.cases), 12)
        self.assertEqual(sum(r["actor_and_effect_bucket_visits"] for r in self.cases.values()), 768)
        for row in self.cases.values():
            self.assertEqual((row["active_bucket"], row["collected_common_model_record"], row["heap_payload_bytes"]),
                             (26, 77, 2944))

    def test_active_effect_does_not_change_natural_guard_expiry(self):
        for earlier in ("gg4e08:0106", "gg4e08:0107"):
            for heal, expiry in ((81, 58), (138, 1), (139, None)):
                held = self.cases[earlier, heal, None]
                self.assertEqual(held["body_receivers_reopened_visit"], expiry)
                self.assertIsNone(held["effect_retired_visit"])
                samples = {s["visit_after_heal"]: s for s in held["samples"]}
                observed = samples[expiry or 64]
                self.assertTrue(observed["effect_active"])
                self.assertTrue(observed["heap_header_claimed"])
                self.assertEqual(observed["owner_8c4"], 2)
                self.assertEqual(observed["receiver_counts_ordinary_and_alternate"], [9, 9] if expiry else [0, 0])

    def test_declared_owner_bit_clear_drains_but_never_opens_receivers(self):
        for earlier in ("gg4e08:0106", "gg4e08:0107"):
            for heal in (81, 138, 139):
                held, release = [self.cases[earlier, heal, v] for v in (None, 17)]
                rd = {s["visit_after_heal"]: s for s in release["samples"]}
                self.assertEqual([rd[n]["effect_phase_field"] for n in (0, 1, 17, 31, 32, 33)], [0, 1, 2, 2, 3, 3])
                self.assertEqual((rd[17]["owner_8c4"], release["effect_retired_visit"]), (0, 33))
                self.assertTrue(rd[32]["effect_active"] and rd[32]["heap_header_claimed"])
                self.assertFalse(rd[33]["effect_active"] or rd[33]["heap_header_claimed"])
                self.assertEqual(rd[33]["free_kind2_and_view_cells"], [1, 1])
                self.assertEqual([s["receiver_counts_ordinary_and_alternate"] for s in held["samples"]],
                                 [s["receiver_counts_ordinary_and_alternate"] for s in release["samples"]])

    def test_rejects_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            effect.decode("")
        with self.assertRaisesRegex(ValueError, "changed at 6/81/0/0"):
            effect.decode("heal-effect: invalid\n"*132)
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "effect.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                effect.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
