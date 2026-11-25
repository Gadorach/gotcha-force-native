"""The observer is only useful if the runtime actually discovers and loads it.

A dynamic ModernGekko mod is discovered when its file name ends in `.mgm.so`, and
its descriptor's `version` must parse as a three-part version. Both are easy to
get wrong in a way that looks like "the hook never fired", so they are pinned
here rather than left to a run that reports zero.
"""
import unittest
from pathlib import Path

from tools.analyze import damage_trace

SOURCE = Path(damage_trace.MOD)


class DamageObserverModTest(unittest.TestCase):
    def test_library_is_discoverable(self):
        self.assertTrue(damage_trace.MOD_LIBRARY.endswith(".mgm.so"))

    def test_descriptor_version_is_three_part(self):
        text = SOURCE.read_text()
        self.assertIn('.version = "1.0.0"', text)
        self.assertNotIn('.version = "1",', text)

    def test_hook_is_read_only(self):
        text = SOURCE.read_text()
        self.assertIn("RECOMP_HOOK(", text)
        self.assertNotIn("RECOMP_PATCH(", text)
        self.assertNotIn("moderngekko_mod_write", text)


if __name__ == "__main__":
    unittest.main()
