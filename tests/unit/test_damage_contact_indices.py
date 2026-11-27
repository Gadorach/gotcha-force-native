"""Static table extent, selector provenance and snapshot compatibility."""
import json
import os
from pathlib import Path
import tempfile
import unittest

from tools.analyze import damage_contact_indices as indices

ROOT = Path(__file__).resolve().parents[2] / "docs/api-map"
DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = ROOT / "damage-contact-indices.json"


class ContactDamageIndicesTest(unittest.TestCase):
    def test_source_setting_writes_and_rank_identity(self):
        write = indices.scene_setting_writes
        self.assertEqual(write(1, 15), [15, 16, None, None])
        self.assertEqual(write(1, -1), [31, 0, None, None])
        self.assertEqual(write(2), [7, 7, 7, 7])
        self.assertEqual(write(3), [None] * 4)
        source = json.loads(ARTIFACT.read_bytes())["source"]
        self.assertEqual(source["rank_source"], "Scene[62 + s8(actor+996)]")
        self.assertEqual(source["rank3_writer_value"], 3)

    def test_indexed_tables_preserve_existing_snapshot(self):
        data = json.loads(ARTIFACT.read_bytes())
        snapshot = json.loads((ROOT / "damage-contact-roles.json").read_bytes())
        for side, saved in zip(data["sides"], snapshot["sides"], strict=True):
            self.assertEqual(len(side["credit_setting"]["cells"]), 32)
            self.assertEqual(len(side["credit_rank"]["cells"]), 8)
            self.assertEqual(len(side["receive_rank"]["cells"]), 8)
            self.assertEqual(side["credit_setting"]["cells"][7], saved["credit_difficulty"])
            self.assertEqual(side["credit_rank"]["cells"][0], saved["attacker_rank"])
            self.assertEqual(side["receive_rank"]["cells"][0], saved["receiver_rank"])
            self.assertEqual([r["cells"][7] for r in side["receive_setting"]], saved["receive_class"])
        self.assertEqual(data["sides"][1]["receive_setting"][1]["cells"][31], 2.0)
        self.assertEqual(len(indices.profile_lines(data, 1)), 8)
        self.assertEqual(len(indices.profile_lines(data, None)), 4)
        with self.assertRaises(ValueError):
            indices.profile_lines(data, 8)

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_overwrite(self):
        with tempfile.TemporaryDirectory() as name:
            output = Path(name) / ARTIFACT.name
            indices.export(DOL, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                indices.export(DOL, output)


if __name__ == "__main__":
    unittest.main()
