"""Test the static projectile -> queue -> reaction binding."""
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.analyze import reaction_emit, reaction_installer_tables, reaction_projectile_map
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/reaction-projectile-map.json"
DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
AFS = Path("/home/gadorach/Projects/GotchaForce/original/extracted/files/afs_data.afs")


class ReactionEmitTest(unittest.TestCase):
    def test_queue_terminates_on_any_negative_byte_and_uses_low_byte(self):
        block = bytearray(reaction_emit.HIT_BYTES)
        start = reaction_emit.QUEUE + 4 * 31
        block[start:start + 4] = bytes((63, 63, 0x80, 127))
        step = reaction_emit.STEPS + reaction_emit.STEP_SIZE * 63
        block[step + 4:step + 6] = b"\xFE\x07"
        self.assertEqual(reaction_emit.read_group(block, 31), [(63, 7), (63, 7)])
        with self.assertRaises(ValueError):
            reaction_emit.read_group(block[:-1], 31)
        with self.assertRaises(ValueError):
            reaction_emit.read_group(block, -1)
        block[start + 1] = 64
        with self.assertRaises(ValueError):
            reaction_emit.read_group(block, 31)

    def test_multiple_releases_and_steps_must_agree(self):
        a = {"mode": 2, "yaw": -1, "pitch": 24}
        b = {"mode": 4, "yaw": -1, "pitch": 24}
        releases = [("type-0", 0), ("type-2", 1)]

        def binding(*steps):
            with patch.object(reaction_emit, "resolve_release", side_effect=steps):
                return reaction_emit.move_binding(None, None, 0x80000000, "x-shot", releases)

        self.assertIsNone(binding({"steps": [a]}, None))
        self.assertIsNone(binding({"steps": [a, b]}, {"steps": [a]}))
        self.assertIsNone(binding({"steps": [a]}, {"steps": [b]}))
        self.assertEqual(binding({"steps": [b]}, {"steps": [b]})["lines"], [
            "# source reaction mode 4 not native; fallback mode 1",
            "reaction-mode=x-shot|1", "reaction-offset=x-shot|-1|24",
        ])
        proved = {"steps": [b], "linked_body_proof": {"pair_link_store": "8008AD1C"}}
        self.assertIn("reaction-mode=x-shot|1", binding(proved, {"steps": [b]})["lines"])
        self.assertEqual(binding(proved, proved)["lines"][-3:],
                         ["reaction-root=x-shot|linked-owner", "reaction-mode=x-shot|4",
                          "reaction-offset=x-shot|-1|24"])

    def test_corrected_pilot_selectors(self):
        pilots = json.loads(ARTIFACT.read_text())["pilots"]
        for model, expected in {"000a": (0, 0, 0), "0615": (0, 15, 0), "061e": (9, 9, 9)}.items():
            binding = pilots[model]
            release, = binding["releases"]
            step, = release["steps"]
            self.assertEqual((release["queue_group"], step["step"], step["index"]), expected)
            self.assertEqual(binding["lines"], ["reaction-mode=basic-shot|2", "reaction-offset=basic-shot|0|24"])
        self.assertEqual(pilots["050b"]["table"], "8038D058")
        self.assertEqual(pilots["0517"]["table"], "8038D118")
        for model, pitch in {"0501":24, "050b":24, "050d":24, "0517":24,
                             "0800":32, "0801":32, "0802":0}.items():
            self.assertEqual(pilots[model]["lines"][-3:],
                             ["reaction-root=basic-shot|linked-owner", "reaction-mode=basic-shot|4",
                              f"reaction-offset=basic-shot|0|{pitch}"])
            release, = pilots[model]["releases"]
            self.assertEqual(release["linked_body_proof"]["linked_root_read"], "80030204")

    @unittest.skipUnless(DOL.is_file() and AFS.is_file(), "original DOL/AFS not available")
    def test_regenerates_byte_identically(self):
        with tempfile.TemporaryDirectory() as name:
            output = Path(name) / ARTIFACT.name
            reaction_projectile_map.export(DOL, AFS, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                reaction_projectile_map.export(DOL, AFS, output)

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_bounded_installer_follows_model_overrides(self):
        with ReadOnlyFile(DOL) as source:
            image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        for model, expected in {0x0500: 0x802D5BA8, 0x050C: 0x802D5CC8,
                                0x0511: 0x80392908, 0x0505: 0x803928D8}.items():
            self.assertEqual(reaction_installer_tables.model_table(image, model)[1], expected)


if __name__ == "__main__":
    unittest.main()
