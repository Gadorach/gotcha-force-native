"""Pin the reaction content emitted from a move context."""
import unittest
from pathlib import Path

from tools.analyze import reaction_emit
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage, ReadOnlyFile

DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
AFS = Path("/home/gadorach/Projects/GotchaForce/original/extracted/files/afs_data.afs")


@unittest.skipUnless(DOL.is_file() and AFS.is_file(), "original DOL/AFS not available")
class ReactionEmitTest(unittest.TestCase):
    def test_pilot_lines(self):
        with ReadOnlyFile(DOL) as source:
            image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        with AFS.open("rb") as stream:
            archive = RetailArchive(stream)
            expected = ["# source reaction mode 4 not native; fallback mode 1",
                        "reaction-mode=basic-shot|1",
                        "reaction-offset=basic-shot|0|24"]
            self.assertEqual(reaction_emit.content_lines(image, archive, 0x0615, (1, 0)),
                             expected)
            self.assertEqual(reaction_emit.content_lines(image, archive, 0x000A, (2, 0)),
                             expected)
            self.assertEqual(reaction_emit.content_lines(image, archive, 0x061E, (0, 0)),
                             ["# source reaction mode 2 not native; fallback mode 1",
                              "reaction-mode=basic-shot|1",
                              "reaction-offset=basic-shot|0|24"])


if __name__ == "__main__":
    unittest.main()
