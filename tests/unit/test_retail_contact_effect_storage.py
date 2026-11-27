"""Separate a successful visual's scene debit from its object and heap lifetime."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_storage as storage


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-storage.json"


def report():
    lines = []
    for i, (_, _, b, n, total, charge, masks) in enumerate(storage.CASES):
        if charge:
            lines.append(f"effect-storage-frontier: {i} {total} {b+1} {n+charge} 2 {charge}")
        last = len(masks)
        for visit, mask in enumerate(masks, 1):
            phase = (1 if visit < last-2 else 2 if visit == last-2 else 3)
            counters = (b+1, n+charge) if charge and phase == 1 else (b, n)
            heap = total if visit < last else 0
            lines.append(f"effect-storage-visit: {i} {visit} {phase} {mask} {int(phase != 3)} "
                         f"{counters[0]} {counters[1]} {heap}")
    return "\n".join(lines)


class RetailContactEffectStorageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_burst_charges_the_admitted_count_then_returns_the_budget_before_storage(self):
        normal, last = self.data["cases"][:2]
        self.assertEqual((normal["scene_burst_debit"], normal["scene_entry_debit"]), (1, 16))
        self.assertEqual((last["scene_bursts_before"], last["scene_entries_before"], last["scene_entry_debit"]), (31, 511, 1))
        self.assertEqual([row["selector_1_joint_children"] for row in (normal, last)], [16, 1])
        self.assertEqual([row["heap_block_bytes"] for row in (normal, last)], [928, 96])
        self.assertEqual(normal["active_masks_from_visit_1"][:6], [255]*5 + [65535])
        for row, (budget, release) in zip((normal, last), ((18, 20), (17, 19)), strict=True):
            self.assertEqual(row["scene_budget_return_visit"], budget)
            self.assertEqual(row["heap_type3_class2_and_descriptors_return_visit"], release)

    def test_selector_eight_updates_identically_at_empty_and_full_scene_budgets(self):
        empty, full = self.data["cases"][2:]
        self.assertEqual((full["scene_bursts_before"], full["scene_entries_before"]), (32, 512))
        for row in (empty, full):
            self.assertEqual((row["scene_entry_debit"], row["scene_burst_debit"], row["heap_block_bytes"]), (0, 0, 288))
            self.assertIsNone(row["scene_budget_return_visit"])
            self.assertEqual(row["phase_2_visit"], 11)
            self.assertEqual(row["heap_type3_class2_and_descriptors_return_visit"], 13)
            self.assertEqual(row["descriptors_returned"], 8)
        self.assertEqual(empty["active_masks_from_visit_1"], full["active_masks_from_visit_1"])
        self.assertEqual(max(mask.bit_count() for mask in empty["active_masks_from_visit_1"]), 6)

    def test_decoder_rejects_early_heap_return_or_missing_scene_debit(self):
        replay = report()
        self.assertEqual(storage.decode(replay), self.data["cases"])
        for bad in (replay.replace("0 18 2 0 1 0 0 928", "0 18 2 0 1 0 0 0", 1),
                    replay.replace("1 96 32 512 2 1", "1 96 31 512 2 1", 1)):
            with self.assertRaises(ValueError):
                storage.decode(bad)

    def test_refuses_to_replace_output(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "storage.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                storage.export(None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
