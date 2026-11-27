"""Bound the opcode-10 operand and lane proof used by close-B emission."""
import json
import os
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.analyze import reaction_emit, reaction_melee, reaction_melee_continuation, reaction_melee_map
from tools.analyze.gred_animation_controller import decode_program
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/reaction-melee-map.json"
ORIGINAL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted"))
DOL, AFS = ORIGINAL / "sys/main.dol", ORIGINAL / "files/afs_data.afs"


class ReactionMeleeTest(unittest.TestCase):
    def events(self, text):
        block = bytes.fromhex(text)
        return reaction_melee.contact_events(decode_program(
            lambda off, size: block[off:off + size], 0))

    def test_reads_all_positive_commands_in_the_opening_program(self):
        self.assertEqual(self.events("0a0000ff 0aff00ff 0a0600ff 00020001 0a0200ff 00010000"), [
            {"program": "00000000", "selector_address": "00000009", "queue_group": 6},
            {"program": "00000000", "selector_address": "00000011", "queue_group": 2},
        ])

    def test_declines_no_contact_secondary_command_and_opaque_setup(self):
        self.assertIsNone(self.events("0a0000ff 0aff00ff 00010000"))
        self.assertIsNone(self.events("0a007fff 0a0400ff 00010000"))
        self.assertEqual(self.events("0a047fff 00010000")[0]["unmapped_secondary_state"], 127)
        self.assertIsNone(self.events("11000300 0a0400ff 00010000"))
        self.assertIsNone(self.events("0a0400ff 60000000"))

    def test_only_pose_only_windups_can_use_a_continuation(self):
        def is_lead_in(text):
            block = bytes.fromhex(text)
            return reaction_melee_continuation.is_lead_in(decode_program(
                lambda off, size: block[off:off + size], 0))
        self.assertTrue(is_lead_in("0103000000000000 0b010100 00030016"))
        self.assertFalse(is_lead_in("0a0100ff 00010000"))
        self.assertFalse(is_lead_in("11000300 00010000"))

    def test_followup_pose_requires_matching_local_lanes_and_a_full_window(self):
        class Linker:
            @staticmethod
            def table_choice(root, group, seed):
                return root + 16

            def program_candidate(self, roots, local, args):
                lanes = [{"lane": lane, "bank": "local", "group": 1, "choice": 1,
                          "archive_offset": local[1, 1]} for lane in (0, 1)]
                if self.disagree and roots[0] == 0x3000:
                    lanes[1]["bank"] = "common"
                return {"program_status": "terminal_wait", "clip_candidates": [
                    {"command_index": 1, "preceding_waits": 0, "descriptor_choice": 1,
                     "start_frame": 0, "lanes": lanes}]}

        linker = Linker()
        linker.disagree = False
        continuation = {"group": 3, "seed": 1, "programs": [
            {"root": "00002000", "first_contact_program": "00002010"},
            {"root": "00003000", "first_contact_program": "00003010"},
        ]}
        actions = [{"program_and_descriptor_roots": [row["root"], "0", "0", "0"]}
                   for row in continuation["programs"]]
        slots = [(1, 0, 0x40), (1, 1, 0x80)]
        metadata = [{"archive_offset": offset, "public_joint_count": 20, "animated_joint_count": 20,
                     "aobj_timing": "indexed", "authored_end_frames": [35.0]} for _, _, offset in slots]

        def qualify():
            return reaction_melee_continuation.qualify_pose(
                linker, continuation, actions, slots, metadata, 25, [1, 0], frame=6, duration=24)

        self.assertEqual(qualify()["clip"], [1, 1])
        linker.disagree = True
        self.assertIsNone(qualify())
        linker.disagree = False
        metadata[1]["authored_end_frames"] = [16.0]
        self.assertIsNone(qualify())
        metadata[1]["authored_end_frames"] = [35.0]
        metadata[1]["strict_metadata_refusals"] = ["aobj-flags"]
        self.assertIsNone(qualify())

    def test_lane_uses_actual_call_and_nearest_mask_writer(self):
        site = 0x8006D000
        branch = 0x48000001 | ((0x8004BEB8 - site) & 0x03FFFFFC)

        class Image:
            def __init__(self, tail):
                words = [0] * (17 - len(tail)) + tail
                self.raw = struct.pack(">17I", *words)

            def read(self, at, size, kind):
                return self.raw

        good = Image([0x3880000F, 0x981F06EA, 0x38A00003, branch])
        self.assertEqual(reaction_melee.request_lane(good, site, 15), {
            "request_site": "8006D000", "mask_site": "8006CFF4", "request_mask": 15, "lane": 0,
        })
        for tail in ([0x3880000F, 0x38800002, branch],
                     [0x3880000F, 0x48000004, branch],
                     [0x3880000F, branch + 4]):
            self.assertIsNone(reaction_melee.request_lane(Image(tail), site, 15))
        self.assertIsNone(reaction_melee.request_lane(Image([0x38800002, branch]), site, 2))

    def test_disagreeing_steps_across_opening_programs_stay_unbound(self):
        class Image:
            def read(self, at, size, kind):
                if size == 1:
                    return b"\x0f"
                # Program at 0 queues group 1; program at 8 queues group 2.
                block = bytes.fromhex("0a0100ff 00010000 0a0200ff 00010000")
                return block[at:at + size]

        source = {"pose": {"request_site": "8006D000", "request_mask": 15},
                  "program_addresses": ["0", "8"]}
        with (patch.object(reaction_melee, "request_lane", return_value={"lane": 0}),
              patch.object(reaction_emit, "resolve_group", side_effect=[
                  [{"mode": 4, "yaw": 0, "pitch": 24}], [{"mode": 4, "yaw": 0, "pitch": 48}]])):
            binding, reason = reaction_melee.recover(
                Image(), bytes(reaction_emit.HIT_BYTES), 0x80300000, source, ["802D0000"])
        self.assertIsNone(binding)
        self.assertIn("disagree", reason)

    def test_public_map_preserves_step_and_program_distinctions(self):
        pilots = json.loads(ARTIFACT.read_text())["pilots"]
        bomber = pilots["0106"]["binding"]
        self.assertEqual(bomber["request_site"], "801616D4")
        self.assertEqual(bomber["continuation"]["seed"], 1)
        wire = pilots["0301"]["binding"]
        self.assertEqual(wire["request_site"], "8010B5E0")
        self.assertEqual(wire["continuation"]["programs"][0]["first_contact_program"], "80322568")
        self.assertEqual([row["first_contact_program"] for row in
                          pilots["0304"]["binding"]["continuation"]["programs"]], ["8033765A", "8038943E"])
        self.assertEqual(pilots["0301"]["continuation_pose"]["clip"], [1, 1])
        self.assertEqual(pilots["0301"]["continuation_pose"]["switch_frame"], 6)
        self.assertEqual(pilots["0106"]["continuation_pose"]["clip_end_frame"], 41.0)
        elemental, = pilots["0202"]["binding"]["contacts"]
        self.assertEqual([(s["step"], s["index"]) for s in elemental["steps"]], [(3, 6), (9, 7)])
        self.assertEqual(len(pilots["0209"]["binding"]["contacts"]), 2)
        self.assertEqual(pilots["0209"]["binding"]["lines"][-2], "reaction-mode=opening-slash|4")
        self.assertEqual(pilots["0807"]["binding"]["lines"][-1], "reaction-offset=opening-slash|0|-64")
        wing, = pilots["0a03"]["binding"]["contacts"]
        self.assertEqual(wing["unmapped_secondary_state"], 127)
        self.assertEqual(pilots["0a03"]["binding"]["lines"][-1], "reaction-offset=opening-slash|0|48")

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_continuation_cannot_inherit_an_unmatched_windup_root(self):
        with ReadOnlyFile(DOL) as file:
            image = DolImage(file.read(0, file.size), SOURCE_SHA256)
        source = {"pose": {"request_site": "8010B478", "program": [3, 0]},
                  "neutral_contact_relay": "80134604",
                  "program_addresses": ["80337636", "8038941A"]}
        self.assertIsNone(reaction_melee_continuation.recover(image, source, ["803371E0"]))
        self.assertIsNone(reaction_melee_continuation.recover(
            image, {**source, "neutral_contact_relay": "80134628"}, ["803371E0", "80389120"]))

    @unittest.skipUnless(DOL.is_file() and AFS.is_file(), "original DOL/AFS not available")
    def test_regenerates_without_overwriting_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / ARTIFACT.name
            reaction_melee_map.export(DOL, AFS, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                reaction_melee_map.export(DOL, AFS, output)


if __name__ == "__main__":
    unittest.main()
