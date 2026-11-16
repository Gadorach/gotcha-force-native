"""Opt-in host integration tests; use GF_SOURCE_REPLAY_BINARY after building."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


@unittest.skipUnless(os.environ.get("GF_SOURCE_REPLAY_BINARY"), "source replay host not requested")
class SourceProgramReplayTests(unittest.TestCase):
    def setUp(self):
        self.binary = str(Path(os.environ["GF_SOURCE_REPLAY_BINARY"]).resolve(strict=True))
        root = Path("local/recomp-evidence/gred-movement-admission-v1")
        self.programs = root / "locomotion.gfmp"
        self.height = root / "pose-height.gfhc"
        self.base = ["--animation-mosaic", "--battle-mvp-assets",
                     "local/content/native-animation-mosaic-v1", "--animation-grid", "4", "1",
                     "--size", "1152", "648", "--headless", "--no-audio"]
        self.source = ["--animation-programs", str(self.programs),
                       "--animation-height-profile", str(self.height)]

    def run_host(self, args):
        return subprocess.run([self.binary, *args], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, timeout=90)

    def test_all_source_pages_and_replay_wrap(self):
        for page in range(1, 5):
            result = self.run_host([*self.base, *self.source, "--animation-page", str(page),
                                    "--frames", "300"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("14 essentials", result.stdout)
            self.assertIn("96-tick inspection replay", result.stdout)
        invalid = self.run_host([*self.base, *self.source, "--animation-page", "5"])
        self.assertEqual(invalid.returncode, 2, invalid.stdout)

    def test_raw_classifier_route_is_unchanged(self):
        result = self.run_host([*self.base, "--frames", "3"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("G-Red=110 Sasuke=111", result.stdout)
        self.assertNotIn("source-program timelines", result.stdout)

    def test_incompatible_modes_and_missing_pair_refuse(self):
        cases = [
            [*self.base, "--animation-programs", str(self.programs)],
            [*self.base, *self.source, "--animation-borg", "sasuke"],
            [*self.base, *self.source, "--animation-export", "/tmp/opencode/must-not-export"],
            [*self.base, "--animation-motion-profile", "unused.gfmm"],
        ]
        for args in cases:
            result = self.run_host(args)
            self.assertEqual(result.returncode, 2, result.stdout)

    def test_held_input_handler_replay_and_edited_motion_refusal(self):
        motion = Path("local/recomp-evidence/gred-motion-integration-v1/motion-v2.gfmm")
        if not motion.exists():
            self.skipTest("source motion operands not prepared")
        result = self.run_host([*self.base, *self.source, "--animation-motion-profile",
                                str(motion), "--frames", "300"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("VM holds, timer expiry and recovery", result.stdout)
        with tempfile.TemporaryDirectory(dir="/tmp/opencode") as name:
            changed = Path(name)/"edited.gfmm"
            raw = bytearray(motion.read_bytes());raw[-1] ^= 1;changed.write_bytes(raw)
            result = self.run_host([*self.base, *self.source, "--animation-motion-profile", str(changed)])
            self.assertEqual(result.returncode, 2, result.stdout)
            self.assertIn("qualified normal G-Red GFMM", result.stdout)

    def test_gfmm8_remains_numeric_replay_data_not_ledge_actor_authority(self):
        motion = Path("local/content/source-gred-ground-v8/motion.gfmm")
        if not motion.is_file():
            self.skipTest("optional qualified GFMM8 absent")
        result = self.run_host([*self.base, *self.source, "--animation-motion-profile",
                                str(motion), "--frames", "96"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("VM holds, timer expiry and recovery", result.stdout)

    def test_gfmm9_keeps_the_legacy_inspection_replay(self):
        motion = Path("local/content/source-gred-ground-v9/motion.gfmm")
        if not motion.is_file():
            self.skipTest("optional qualified GFMM9 absent")
        args = [*self.base, *self.source, "--animation-motion-profile"]
        old = self.run_host([*args, "local/content/source-gred-ground-v8/motion.gfmm", "--frames", "96"])
        new = self.run_host([*args, str(motion), "--frames", "96"])
        self.assertEqual(old.returncode, 0, old.stdout)
        self.assertEqual(new.returncode, 0, new.stdout)
        self.assertEqual(old.stdout, new.stdout)
        self.assertIn("VM holds, timer expiry and recovery", new.stdout)

    def test_edited_source_profile_and_height_operands_refuse(self):
        original = self.programs.read_bytes()
        with tempfile.TemporaryDirectory(dir="/tmp/opencode") as name:
            output = Path(name)
            edited = bytearray(original)
            edited[14] ^= 1  #valid GFMP default-model byte, but not the recovered profile
            changed = output / "edited.gfmp"
            changed.write_bytes(edited)
            result = self.run_host([*self.base, "--animation-programs", str(changed),
                                    "--animation-height-profile", str(self.height)])
            self.assertEqual(result.returncode, 2, result.stdout)
            self.assertIn("unaltered normal G-Red locomotion profile required", result.stdout)
            edited_height = bytearray(self.height.read_bytes())
            edited_height[-1] ^= 1
            changed_height = output / "edited.gfhc"
            changed_height.write_bytes(edited_height)
            result = self.run_host([*self.base, "--animation-programs", str(self.programs),
                                    "--animation-height-profile", str(changed_height)])
            self.assertEqual(result.returncode, 2, result.stdout)
        self.assertEqual(self.programs.read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
