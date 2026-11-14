import unittest

from tools.analyze.lane_descriptors import descriptor_lanes, lane_partition


class LaneDescriptorTests(unittest.TestCase):
    def reader(self, data):
        calls = []

        def read(at, size):
            calls.append((at, size))
            offset = at - 0x1000
            if offset < 0 or offset + size > len(data):
                raise ValueError('outside initialized fixture')
            return data[offset:offset + size]

        return read, calls

    def test_three_lanes_stop_at_last_used_byte(self):
        data = bytearray(25)
        data[0] = 255
        for lane in range(3):
            data[4+lane*8:9+lane*8] = bytes([4, lane, 5, 60, 255])
        read, calls = self.reader(bytes(data))
        value = descriptor_lanes(read, 0x1000, 7)
        self.assertEqual(value['root_selector'], -1)
        self.assertEqual([r['lane'] for r in value['lanes']], [0, 1, 2])
        self.assertEqual(value['lanes'][2]['event'], -1)
        self.assertEqual(value['required_end'], 25)
        self.assertEqual(max(at+n for at, n in calls), 0x1019)

    def test_sparse_fourth_lane_is_not_third_lane(self):
        read, calls = self.reader(bytes(33))
        value = descriptor_lanes(read, 0x1000, 11)
        self.assertEqual([r['offset'] for r in value['lanes']], [4, 12, 28])
        self.assertNotIn((0x1014, 5), calls)
        self.assertEqual(value['required_end'], 33)

    def test_two_lane_reader_refuses_not_normalizes(self):
        read, calls = self.reader(bytes(33))
        with self.assertRaisesRegex(ValueError, 'unsupported bits'):
            descriptor_lanes(read, 0x1000, 7, supported_lanes=2)
        self.assertEqual(calls, [])

    def test_two_lane_models_and_controls(self):
        data = bytes(range(20))
        read, _ = self.reader(data)
        result = descriptor_lanes(read, 0x1000, 3)
        self.assertEqual(result['lanes'][0]['model_bytes'], [9, 10])
        self.assertEqual(result['lanes'][1]['control_bytes'], [17, 18, 19])
        self.assertEqual(result['required_end'], 20)

    def test_truncated_third_lane_refuses(self):
        read, _ = self.reader(bytes(24))
        with self.assertRaises(ValueError):
            descriptor_lanes(read, 0x1000, 7)

    def test_short_read_refuses(self):
        with self.assertRaisesRegex(ValueError, 'short descriptor'):
            descriptor_lanes(lambda at, n: b'', 0x1000, 3)

    def test_wrap_refuses(self):
        with self.assertRaisesRegex(ValueError, 'wrap'):
            descriptor_lanes(lambda at, n: bytes(n), 0xfffffffc, 3)

    def test_mask_and_address_domains(self):
        for mask in (0, -1, 16, True):
            with self.assertRaises(ValueError):
                descriptor_lanes(lambda at, n: bytes(n), 0x1000, mask)
        for address in (0, -1, 1 << 32, True):
            with self.assertRaises(ValueError):
                descriptor_lanes(lambda at, n: bytes(n), address, 3)

    def test_active_empty_lane_retained(self):
        result = lane_partition(11, 6, [2, 4, 6], list(range(6)), list(range(6)))
        self.assertEqual(result['lanes'][-1]['slots'], [])
        self.assertEqual(result['retained_slots'], [4, 5])

    def test_mirror_must_stay_in_lane(self):
        with self.assertRaisesRegex(ValueError, 'mirror'):
            lane_partition(3, 4, [2, 4, 4], [0, 1, 2, 3], [2, 1, 0, 3])

    def test_duplicate_active_bones_refuse(self):
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            lane_partition(3, 4, [2, 4, 4], [0, 1, 1, 3], [0, 1, 2, 3])

    def test_partition_extents_and_boundaries(self):
        for cuts in ([3, 2, 4], [2, 4, 5], [2, 4]):
            with self.assertRaises(ValueError):
                lane_partition(3, 4, cuts, list(range(4)), list(range(4)))
        with self.assertRaises(ValueError):
            lane_partition(3, 4, [2, 4, 4], [0, 1, 2], list(range(4)))


if __name__ == '__main__':
    unittest.main()
