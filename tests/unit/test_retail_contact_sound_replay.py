"""Separate retail sound selection, byte sharing, and visual gating."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_sound_replay as sound


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-sound-replay.json"
PAIRS = "\n".join(f"impact-sound-pair: {i} {int(not i&1)} {int(not i&2)} 4 4 163 1" for i in range(4))


class RetailContactSoundReplayTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_size_thresholds_are_strict(self):
        rows = self.data["band_cases"]
        self.assertEqual(len(rows), 42)
        for code in sound.CODES:
            cases = [r for r in rows if r["code"] == code]
            self.assertEqual([r["receiver_size"] for r in cases], list(sound.SIZES))
            self.assertEqual([r["size_band_used"] for r in cases], list(sound.BANDS if code else (0,)*7))
            for at, over in zip(cases[1::2], cases[2::2], strict=True):
                self.assertEqual(over["tone_id"]-at["tone_id"], 2 if code else 0)

    def test_counter_partition_and_special_lane_can_alias(self):
        rows = {r["case"]: r for r in self.data["counter_cases"]}
        self.assertEqual((rows["wrap"]["counter_before"], rows["wrap"]["counter_after"]), (255, 0))
        self.assertEqual((rows["shared-partition"]["counter_byte"], rows["shared-partition"]["tone_id"]), (4, 56))
        self.assertEqual((rows["held-gate"]["counter_after"], rows["held-gate"]["tone_id"]), (1, None))
        self.assertEqual(rows["separate-partition"]["counter_byte"], 17)
        self.assertEqual(rows["four-resets"]["tone_id"], 57)
        special, ordinary = rows["special-cross-row"], rows["ordinary-alias"]
        self.assertEqual((special["receiver_partition"], ordinary["receiver_partition"]), (0, 1))
        self.assertEqual((special["counter_byte"], ordinary["counter_byte"]), (22, 22))
        self.assertEqual((special["tone_id"], ordinary["tone_id"], ordinary["counter_after"]), (92, 67, 2))
        self.assertEqual((rows["silent"]["tone_id"], rows["silent"]["sound_gate_after"]), (None, 0))

    def test_paired_sound_and_visual_gates_never_change_the_accepted_hp(self):
        cases = self.data["pair_cases"]
        self.assertEqual(sound.decode_pairs(PAIRS), cases)
        self.assertEqual([(r["sound_counter_delta"], r["visual_requested"]) for r in cases],
                         [(1, True), (0, True), (1, False), (0, False)])
        self.assertTrue(all(r["hp_lost"] == 17 and not r["visual_allocated"] for r in cases))
        with self.assertRaises(ValueError):
            sound.decode_pairs(PAIRS.replace("1 0 1 4 4 163 1", "1 1 1 4 4 163 1", 1))

    def test_static_codes_do_not_expand_ordinary_bindings(self):
        codes = self.data["scoped_source_codes"]
        self.assertEqual(codes["actor_code_counts"], {"0": 1, "128": 34, "136": 33, "192": 2, "200": 3})
        self.assertEqual(codes["common_code_counts"], {"0": 22})
        self.assertEqual(len(codes["actor_records_outside_native_ordinary_codes"]), 5)
        self.assertFalse({0xC0, 0xC8} & set(sound.sound.CODES))

    def test_existing_output_is_preserved(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "sound.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                sound.export(None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
