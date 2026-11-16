from pathlib import Path
import struct
import tempfile
import unittest

from tools.analyze.gred_animation_controller import (
    decode_program, export, hierarchy, inventory, recover, signed_table,
)


class Memory:
    def __init__(self, size=512):
        self.raw = bytearray(size)

    def read(self, at, size):
        if at < 0 or size < 0 or at + size > len(self.raw):
            raise ValueError("unmapped test source")
        return bytes(self.raw[at:at + size])


class GRedControllerTests(unittest.TestCase):
    def test_signed_relative_offsets_aliases_and_sentinel(self):
        memory = Memory()
        struct.pack_into(">4h", memory.raw, 64, -32, 32, 32, -1)
        self.assertEqual(signed_table(memory.read, 64), [32, 96, 96])
        with self.assertRaises(ValueError):
            signed_table(memory.read, 64, 2)
        for address in (-2, 3, 1 << 32):
            with self.assertRaises(ValueError):
                signed_table(memory.read, address)
        with self.assertRaises(ValueError):
            signed_table(lambda at, size: b"", 64)

    def test_hierarchy_and_descriptor_aliases_are_not_raw_clip_ids(self):
        memory = Memory()
        struct.pack_into(">3h", memory.raw, 0, 16, 16, -1)
        struct.pack_into(">3h", memory.raw, 16, 48, 48, -1)
        struct.pack_into(">2h", memory.raw, 128, 16, -1)
        struct.pack_into(">3h", memory.raw, 144, 32, 32, -1)
        memory.raw[64:76] = bytes.fromhex("010000020a0000000001ffff")
        memory.raw[176] = 255
        memory.raw[180:185] = bytes([0, 6, 1, 0, 255])
        memory.raw[188:193] = bytes([0x81, 9, 5, 0, 255])
        self.assertEqual(len(hierarchy(memory.read, 0)), 2)
        report = inventory(memory.read, 0, 128)
        self.assertEqual(len(report["programs"]), 1)
        self.assertEqual(report["programs"][0]["aliases"],
                         [[0, 0], [0, 1], [1, 0], [1, 1]])
        descriptor = report["descriptors"][0]
        self.assertEqual(descriptor["aliases"], [[0, 0], [0, 1]])
        self.assertEqual(descriptor["root_selector"], -1)
        self.assertEqual(descriptor["lanes"][0]["clip"]["raw_id"], 6)
        self.assertEqual(descriptor["lanes"][1]["clip"]["raw_id"], 10265)
        self.assertTrue(descriptor["lanes"][1]["clip"]["common"])

    def test_wait_continuations_keep_authored_timing(self):
        memory = Memory()
        raw = bytes.fromhex("01000401090000000002000901000e01090000000001ffff")
        memory.raw[64:64 + len(raw)] = raw
        report = decode_program(memory.read, 64)
        self.assertEqual(report["status"], "terminal_wait")
        before, wait, after, terminal = report["commands"]
        self.assertEqual((before["choice"], before["mode"], before["duration"]), (4, 1, 9))
        self.assertEqual((wait["flags"], wait["threshold"]), (2, 9))
        self.assertEqual(before["preceding_waits"], 0)
        self.assertEqual(after["preceding_waits"], 1)
        self.assertEqual(terminal["threshold"], -1)

    def test_unknown_opcodes_remain_explicit_not_reinterpreted(self):
        memory = Memory()
        memory.raw[64:68] = bytes([31, 10, 11, 12])
        report = decode_program(memory.read, 64)
        self.assertEqual((report["status"], report["opcode"]), ("unsupported_opcode", 31))
        self.assertEqual(report["commands"], [])
        with self.assertRaises(ValueError):
            decode_program(lambda at, size: b"", 64)

    def test_actor_event_sizes_are_structural_only(self):
        memory = Memory()
        memory.raw[64:76] = bytes.fromhex("0a0102030c0102030001ffff")
        report = decode_program(memory.read, 64)
        self.assertEqual(report["status"], "terminal_wait")
        self.assertEqual([row["kind"] for row in report["commands"]],
                         ["actor_event_10", "input_control", "wait"])
        memory.raw[64:72] = bytes.fromhex("110002030001ffff")
        event = decode_program(memory.read, 64)["commands"][0]
        self.assertEqual((event["kind"], event["lane"], event["selector"], event["operand"]),
                         ("class_event", 0, 2, 3))

    def test_branch_loops_and_budgets(self):
        memory = Memory()
        memory.raw[64:72] = bytes.fromhex("0000000106000006")
        self.assertEqual(decode_program(memory.read, 64)["status"], "yielding_loop")
        memory.raw[64:68] = bytes.fromhex("06000002")
        self.assertEqual(decode_program(memory.read, 64)["status"], "non_yielding_loop")
        memory.raw[64:68] = bytes.fromhex("06008000")
        self.assertEqual(decode_program(memory.read, 64)["status"], "span_budget")
        memory.raw[64:76] = bytes.fromhex("010000000000000006000008")
        with self.assertRaises(ValueError):
            decode_program(memory.read, 64)  #branch into first command's operands
        memory.raw[64:72] = bytes.fromhex("0700000007000000")
        self.assertEqual(decode_program(memory.read, 64, max_commands=1)["status"], "command_budget")

    def test_revision_and_output_refusals(self):
        class WrongRevision:
            sha256 = "wrong"
        with self.assertRaises(ValueError):
            recover(WrongRevision())
        with tempfile.TemporaryDirectory() as name:
            output = Path(name) / "existing.json"
            output.write_bytes(b"user-owned")
            with self.assertRaises(FileExistsError):
                export(Path(name) / "absent.dol", output)
            self.assertEqual(output.read_bytes(), b"user-owned")
            link = Path(name) / "dangling.json"
            link.symlink_to(Path(name) / "absent.json")
            with self.assertRaises(FileExistsError):
                export(Path(name) / "absent.dol", link)
            invalid = Path(name) / "invalid.dol"
            invalid.write_bytes(b"not qualified")
            target = Path(name) / "new.json"
            with self.assertRaises(ValueError):
                export(invalid, target)
            self.assertFalse(target.exists())
            self.assertEqual(invalid.read_bytes(), b"not qualified")


if __name__ == "__main__":
    unittest.main()
