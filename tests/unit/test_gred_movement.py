from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.analyze.gred_movement import direct_calls, export, movement_data, qualify_setup, recover
from tools.extract.gred_locomotion import encode, export as export_programs, recover_height
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.gred_movement_input import encode as encode_input, export as export_input


class Memory:
    def __init__(self):
        self.raw = bytearray(512)

    def read(self, address, size, kind=None):
        if address < 0 or address + size > len(self.raw):
            raise ValueError("unmapped source")
        return bytes(self.raw[address:address + size])


class GRedMovementTests(unittest.TestCase):
    def test_typed_operands_are_not_health_or_physics_defaults(self):
        raw = bytearray(432)
        struct.pack_into(">h", raw, 4, 10000)
        struct.pack_into(">f", raw, 0x6C, -1)
        struct.pack_into(">4f", raw, 0xF0, 25, -.5, 22, 30)
        rows = movement_data(raw)
        by_role = {row["role"]: row for row in rows}
        self.assertEqual(len(rows), 42)
        self.assertEqual(by_role["resource_limit736"]["value"], 10000)
        self.assertEqual(by_role["fall_vertical_base_scalar"]["value"], -1)
        self.assertEqual(by_role["dash_forward_change"]["value"], -.5)
        self.assertEqual(by_role["dash_forward_speed"]["bits"], "41c80000")
        self.assertNotIn("health", by_role)
        with self.assertRaises(ValueError):
            movement_data(raw[:-1])
        struct.pack_into(">f", raw, 0xF0, float("nan"))
        with self.assertRaises(ValueError):
            movement_data(raw)

    def test_direct_call_sign_absolute_and_link_bit(self):
        image = Memory()
        struct.pack_into(">4I", image.raw, 0x100,
                         0x48000011, 0x4BFFFFFD, 0x48000010, 0x48000083)
        result = direct_calls(image, 0x100, 16)
        self.assertEqual([call["target"] for call in result["direct_calls"]],
                         [0x110, 0x100, 0x80])
        self.assertEqual([call["callsite"] for call in result["direct_calls"]],
                         [0x100, 0x104, 0x10C])
        for address, size in ((1, 4), (0x100, 3), (0x100, 4100)):
            with self.assertRaises(ValueError):
                direct_calls(image, address, size)
        with self.assertRaises(ValueError):
            direct_calls(image, 0x100, 512)

    def test_unqualified_setup_or_revision_cannot_supply_parameters(self):
        with self.assertRaises(ValueError):
            qualify_setup(Memory(), bytes(1064))
        class Wrong:
            sha256 = "wrong"
        with self.assertRaises(ValueError):
            recover(Wrong(), bytes(1064))

    def fixture(self):
        image = Memory()
        image.raw[64:76] = bytes.fromhex("010000020a0000000001ffff")
        image.raw[128:148] = bytes(20)
        commands = [
            dict(address=64, size=8, opcode=1, group=0, choice=0),
            dict(address=72, size=4, opcode=0),
        ]
        tables = [dict(group=group, choices=[64] * 32) for group in (0, 1)]
        descriptors = [dict(group=group, choices=[128]) for group in (0, 1)]
        return image, dict(program_groups=tables, descriptor_groups=descriptors,
                           programs=[dict(address=64, status="terminal_wait",
                                          commands=commands)])

    def test_gfmp3_packaging_keeps_raw_programs_and_descriptor_aliases(self):
        image, bank = self.fixture()
        raw = encode(image.read, bank, bytes([2, 3]))
        self.assertEqual(struct.unpack_from(">IHBBI", raw),
                         (0x47464D50, 3, 3, 32, 0xE2B4439A))
        self.assertEqual(struct.unpack_from(">HBB", raw, 12), (2, 2, 3))
        self.assertEqual(struct.unpack_from(">I", raw, 16)[0], len(raw))
        for index in range(32):
            group, choice, reserved, offset, size = struct.unpack_from(">BBHII", raw, 32 + index * 12)
            self.assertEqual((group, choice, reserved, size), (1, index, 0, 12))
            self.assertEqual(raw[offset:offset + size], image.read(64, 12))
        dtable = struct.unpack_from(">I", raw, 24)[0]
        self.assertEqual(raw[dtable:dtable + 4], bytes([0, 0, 0, 0]))
        self.assertEqual(raw[dtable + 24:dtable + 28], bytes([1, 0, 0, 0]))
        self.assertEqual(image.read(64, 12), bytes.fromhex("010000020a0000000001ffff"))

    def test_gfmp3_export_does_not_grant_event_or_incomplete_flow_execution(self):
        image, bank = self.fixture()
        bank["programs"][0]["commands"][0]["opcode"] = 17
        with self.assertRaises(ValueError):
            encode(image.read, bank, b"\0\0")
        image, bank = self.fixture()
        bank["programs"][0]["status"] = "unsupported_opcode"
        with self.assertRaises(ValueError):
            encode(image.read, bank, b"\0\0")
        image, bank = self.fixture()
        bank["program_groups"][1]["choices"] = [128] * 32
        with self.assertRaises(ValueError):
            encode(image.read, bank, b"\0\0")
        image, bank = self.fixture()
        with self.assertRaises(ValueError):
            encode(image.read, bank, b"\xff\0")

    def test_outputs_refuse_existing_files_and_symlinks_before_input_access(self):
        with tempfile.TemporaryDirectory() as name:
            p = Path(name)
            output = p / "owned"
            output.write_bytes(b"user-work")
            for tool in (lambda: export(p / "absent", p / "absent-profile", output),
                         lambda: export_programs(p / "absent", output)):
                with self.assertRaises(FileExistsError):
                    tool()
            self.assertEqual(output.read_bytes(), b"user-work")
            link = p / "dangling"
            link.symlink_to(p / "absent-target")
            with self.assertRaises(FileExistsError):
                export_programs(p / "absent", link)

    def test_height_profile_reads_exact_source_operands_and_refuses_substitutions(self):
        class Source:
            sha256 = SOURCE_SHA256
            words = {0x804370F4: 0, 0x80437104: 0x3F000000,
                     0x80437130: 0xBD4CCCCD, 0x80437134: 0x3F333333,
                     0x8043714C: 0x40A00000}
            def read(self, address, size):
                self.asserted_size = size
                return struct.pack(">I", self.words[address])
        source = Source()
        self.assertEqual(recover_height(source), struct.pack(
            ">7I", 0x47464843, 1, 0, 0x3F000000, 0xBD4CCCCD, 0x3F333333, 0x40A00000))
        self.assertEqual(source.asserted_size, 4)
        source.words = {**source.words, 0x80437134: 0x3F800000}
        with self.assertRaises(ValueError):
            recover_height(source)
        source.sha256 = "wrong"
        with self.assertRaises(ValueError):
            recover_height(source)

    def test_movement_input_profile_layout_and_instruction_refusals(self):
        class Source:
            sha256 = SOURCE_SHA256
            changed = False
            def read(self, address, size, kind=None):
                instructions = {
                    0x800689AC: 0x38A00014, 0x80068A14: 0x38A00034,
                    0x800689FC: 0x38000002, 0x800689F0: 0x38000003,
                    0x80068A2C: 0x38000014, 0x80068ABC: 0x38000010,
                    0x8008B788: 0x386D8388,
                }
                if address in instructions:
                    return struct.pack(">I", instructions[address] ^ int(self.changed))
                if address == 0x80003334:
                    return bytes.fromhex("3da0804361adb5a0")
                if address == 0x80433928:
                    return bytes(range(1, 9))  #synthetic, not original lookup bytes
                if address == 0x802DA770:
                    return bytes(16)
                return struct.pack(">6H", 1, 0, 0, 1, 0x100, 0)
        source = Source();config = bytearray(192);config[0x11] = 7;data = bytes(range(216))*2
        with patch("tools.extract.gred_movement_input.qualify_setup", return_value=(data, config)):
            raw = encode_input(source, b"synthetic-setup")
            self.assertEqual(len(raw), 192)
            self.assertEqual(struct.unpack_from(">IHHII", raw), (0x47464D49, 1, 0x615, 0xE2B4439A, 192))
            self.assertEqual(raw[20:28], bytes([20, 52, 2, 3, 20, 16, 10, 0]))
            self.assertEqual(raw[176:190], data[4:18])
            source.changed = True
            with self.assertRaises(ValueError):
                encode_input(source, b"synthetic-setup")
        source.sha256 = "wrong"
        with self.assertRaises(ValueError):
            encode_input(source, b"synthetic-setup")

    def test_input_export_refuses_existing_output_before_source_access(self):
        with tempfile.TemporaryDirectory() as name:
            p = Path(name);out = p / "owned";out.write_bytes(b"owned")
            with self.assertRaises(FileExistsError):
                export_input(p / "absent.dol", p / "absent.gfcs", out)
            self.assertEqual(out.read_bytes(), b"owned")
if __name__ == "__main__":
    unittest.main()
