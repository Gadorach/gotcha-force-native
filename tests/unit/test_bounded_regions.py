import unittest

from tools.analyze.bounded_regions import aligned_layout


class BoundedRegionsTests(unittest.TestCase):
    def test_aligned_segments(self):
        result = aligned_layout(0x1000, 128, [1, 33])
        self.assertEqual([(r.start, r.size) for r in result.segments], [(0x1000, 1), (0x1020, 33)])
        self.assertEqual(result.reserved_bytes, 96)

    def test_front_padding_is_in_budget(self):
        result = aligned_layout(0x1001, 63, [1])
        self.assertEqual((result.front_padding, result.reserved_bytes), (31, 63))
        with self.assertRaisesRegex(ValueError, 'capacity'):
            aligned_layout(0x1001, 32, [1])

    def test_trailing_padding_is_in_budget(self):
        with self.assertRaisesRegex(ValueError, 'capacity'):
            aligned_layout(0x1000, 1, [1])

    def test_zero_segments_have_no_owner(self):
        result = aligned_layout(0x1000, 64, [0, 1, 0, 1, 0])
        self.assertIsNone(result.segments[0])
        self.assertIsNone(result.segments[2])
        self.assertIsNone(result.segments[4])
        self.assertEqual(result.segments[3].start, 0x1020)

    def test_empty_plan_is_metadata_only(self):
        result = aligned_layout(0x1000, 0, [])
        self.assertEqual(result.segments, ())
        self.assertEqual(result.reserved_bytes, 0)

    def test_exclusive_end_may_equal_address_limit(self):
        result = aligned_layout(0xffffffe0, 32, [32])
        self.assertEqual(result.segments[0].end, 1 << 32)

    def test_no_segment_starts_at_wrapped_end(self):
        with self.assertRaisesRegex(ValueError, 'pointer domain'):
            aligned_layout(0xffffffe0, 32, [32, 1])

    def test_alignment_cannot_wrap_start(self):
        with self.assertRaisesRegex(ValueError, 'aligned start'):
            aligned_layout(0xffffffff, 1, [])

    def test_invalid_enclosing_regions(self):
        for start, size in [(-1, 1), (1 << 32, 0), (True, 1), (0, True), (0xffffffff, 2)]:
            with self.subTest(start=start, size=size), self.assertRaises(ValueError):
                aligned_layout(start, size, [])

    def test_invalid_sizes_and_budget(self):
        for sizes in ([True], [-1], [1 << 32], [0]*257, '123'):
            with self.subTest(sizes=sizes), self.assertRaises(ValueError):
                aligned_layout(0, 256, sizes)

    def test_invalid_alignments(self):
        for alignment in (0, 3, -2, True, 1 << 32):
            with self.subTest(alignment=alignment), self.assertRaises(ValueError):
                aligned_layout(0, 256, [], alignment=alignment)

    def test_declared_alignment_one(self):
        result = aligned_layout(3, 5, [2, 3], alignment=1)
        self.assertEqual(result.reserved_bytes, 5)

    def test_inputs_unchanged_on_refusal(self):
        sizes = [1, 33]
        with self.assertRaises(ValueError):
            aligned_layout(0x1001, 1, sizes)
        self.assertEqual(sizes, [1, 33])

    def test_frozen_output(self):
        result = aligned_layout(0, 32, [1])
        with self.assertRaises(AttributeError):
            result.segments[0].start = 1


if __name__ == '__main__':
    unittest.main()
