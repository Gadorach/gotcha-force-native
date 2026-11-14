import copy
import unittest

from tools.analyze.hierarchy_maps import audit_hierarchy_maps


LINKS = [(1, -1, -1), (-1, 2, 0), (-1, -1, 0)]


class HierarchyMapTests(unittest.TestCase):
    def test_identity_mapping(self):
        row = audit_hierarchy_maps(LINKS, [0, 1, 2], [0, 1, 2], [0, 1, 2])
        self.assertEqual(row['traversal_position'], [0, 1, 2])
        self.assertTrue(row['active_matches_filtered_traversal'])
        self.assertTrue(row['metadata_only_not_pose_frame_or_owner'])

    def test_nonidentity_logical_traversal(self):
        links = [(-1, 1, 2), (-1, -1, 2), (0, -1, -1)]
        row = audit_hierarchy_maps(links, [2, 0, 1], [1, 2, 0], [1, 0, 2])
        self.assertEqual(row['root'], 2)
        self.assertEqual(row['traversal_position'], [1, 2, 0])
        self.assertEqual(row['active_visits'][0]['mirror_traversal_position'], 1)

    def test_visit_order_can_differ(self):
        row = audit_hierarchy_maps(LINKS, [0, 1, 2], [2, 0, 1], [0, 2, 1])
        self.assertFalse(row['active_matches_filtered_traversal'])

    def test_inactive_duplicates_and_sentinel_retained(self):
        row = audit_hierarchy_maps(LINKS, [0, 1, 2], [0], [0, 255, 255])
        self.assertEqual(row['unvisited_nodes'], [1, 2])
        self.assertEqual(row['inactive_mirror_values'], [{'node': 1, 'value': 255}, {'node': 2, 'value': 255}])

    def test_empty_active_does_not_create_pose(self):
        row = audit_hierarchy_maps(LINKS, [0, 1, 2], [], [-1, 255, 0])
        self.assertEqual(row['active_visits'], [])
        self.assertEqual(row['unvisited_nodes'], [0, 1, 2])

    def test_foreign_active_mirror_refuses(self):
        with self.assertRaisesRegex(ValueError, 'active mirror'):
            audit_hierarchy_maps(LINKS, [0, 1, 2], [1], [0, 2, 1])

    def test_nonbijective_active_mirror_refuses(self):
        with self.assertRaisesRegex(ValueError, 'bijective'):
            audit_hierarchy_maps(LINKS, [0, 1, 2], [1, 2], [0, 1, 1])

    def test_mirror_cycle_is_not_assumed_involution(self):
        row = audit_hierarchy_maps(LINKS, [0, 1, 2], [0, 1, 2], [1, 2, 0])
        self.assertFalse(row['mirror_involution_on_active'])

    def test_duplicate_traversal_refuses(self):
        with self.assertRaisesRegex(ValueError, 'permutation'):
            audit_hierarchy_maps(LINKS, [0, 1, 1], [0], [0, 1, 2])

    def test_cycle_refuses(self):
        with self.assertRaises(ValueError):
            audit_hierarchy_maps([(-1, 0, -1)], [0], [0], [0])

    def test_disconnected_node_refuses(self):
        with self.assertRaisesRegex(ValueError, 'coverage'):
            audit_hierarchy_maps([(-1, -1, -1), (-1, -1, -1)], [0, 1], [], [0, 1])

    def test_child_parent_mismatch_refuses(self):
        with self.assertRaisesRegex(ValueError, 'child parent'):
            audit_hierarchy_maps([(1, -1, -1), (-1, -1, -1)], [0, 1], [], [0, 1])

    def test_sibling_parent_mismatch_refuses(self):
        with self.assertRaisesRegex(ValueError, 'sibling parent'):
            audit_hierarchy_maps([(1, -1, -1), (-1, 2, 0), (-1, -1, -1)], [0, 1, 2], [], [0, 1, 2])

    def test_shapes_domains_and_budgets(self):
        for links, traversal, active, mirror, budget in (
            ([], [], [], [], 4), (LINKS, [0, 1, 2], [], [0, 1, 2], 2),
            ([(True, -1, -1)], [0], [], [0], 4), ([(3, -1, -1)], [0], [], [0], 4),
            (LINKS, [0, 1, 2], [], [False, 1, 2], 4), (LINKS, [0, 1, 2], [], [0, 1], 4),
            (LINKS, [0, 1, 2], [], [0, 1, 2], True),
        ):
            with self.subTest(links=links, budget=budget), self.assertRaises(ValueError):
                audit_hierarchy_maps(links, traversal, active, mirror, max_nodes=budget)

    def test_inputs_and_outputs_do_not_alias(self):
        links = [list(row) for row in LINKS]
        traversal, active, mirror = [0, 1, 2], [2, 1, 0], [0, 2, 1]
        before = copy.deepcopy((links, traversal, active, mirror))
        row = audit_hierarchy_maps(links, traversal, active, mirror)
        self.assertEqual((links, traversal, active, mirror), before)
        row['traversal'][0] = 99
        row['active_visits'][0]['node'] = 99
        self.assertEqual((links, traversal, active, mirror), before)

    def test_active_duplicates_and_domains_refuse(self):
        for active in ([1, 1], [3], [-1], [True], '0'):
            with self.subTest(active=active), self.assertRaises(ValueError):
                audit_hierarchy_maps(LINKS, [0, 1, 2], active, [0, 1, 2])


if __name__ == '__main__':
    unittest.main()
