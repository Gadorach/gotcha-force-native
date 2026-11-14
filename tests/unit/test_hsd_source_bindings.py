import struct
import unittest

from tools.analyze.hsd_source_bindings import model_animation_bindings


def fixture():
    block = bytearray(256)
    pointers = {16: 32, 32: 48, 48: 80, 52: 64, 64: 112, 120: 144, 152: 176, 192: 4}
    for at, value in pointers.items():
        struct.pack_into(">I", block, at, value)
    struct.pack_into(">f", block, 148, 40.)
    struct.pack_into(">I", block, 180, 4)
    block[188] = 5
    block[4:8] = b"data"
    relocations = b"".join(struct.pack(">I", p) for p in pointers)
    public = struct.pack(">II", 16, 0)
    names = b"scene_data\0"
    header = struct.pack(">8I", 32 + len(block) + len(relocations) + len(public) + len(names), len(block), len(pointers), 1, 0, 0, 0, 0)
    return bytearray(header + block + relocations + public + names)


class HsdSourceBindingTests(unittest.TestCase):
    def test_published_record_joint_track_identity(self):
        result = model_animation_bindings(fixture(), [0])
        self.assertEqual(result["published_records"], 1)
        aobj = result["records"][0]["joint_clips"][0]["nodes"][0]["aobj"]
        self.assertEqual(aobj["end"], 40)
        self.assertEqual(aobj["tracks"][0]["bytes"], 4)
        self.assertEqual(result["records"][0]["shape_references"], [])

    def test_relocated_payload_at_zero_is_data(self):
        b = fixture()
        struct.pack_into(">I", b, 32 + 192, 0)
        self.assertEqual(model_animation_bindings(b, [0])["records"][0]["joint_clips"][0]["nodes"][0]["aobj"]["tracks"][0]["payload"], 0)

    def test_unknown_record_and_duplicates(self):
        for ids in [[1], [0, 0], [-1]]:
            with self.subTest(ids=ids), self.assertRaises(ValueError):
                model_animation_bindings(fixture(), ids)

    def test_tree_cycle_refused(self):
        b = fixture()
        struct.pack_into(">I", b, 32 + 112, 112)
        with self.assertRaises(ValueError):
            model_animation_bindings(b, [0])

    def test_nonfinite_end_refused(self):
        b = fixture()
        struct.pack_into(">I", b, 32 + 148, 0x7fc00000)
        with self.assertRaises(ValueError):
            model_animation_bindings(b, [0])

    def test_payload_outside_block_refused(self):
        b = fixture()
        struct.pack_into(">I", b, 32 + 180, 65536)
        with self.assertRaises(ValueError):
            model_animation_bindings(b, [0])


if __name__ == "__main__":
    unittest.main()
