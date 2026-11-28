"""Keep exact body ordinals, registry gates and finite-pool checks distinct."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_registry as registry


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailBodyRegistryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-body-registry.json").read_bytes())
        cls.rows = cls.report["models"]

    def test_exact_roster_and_seat_scope(self):
        prior = MAPS / "gg4e08-standalone-installers.json"
        self.assertEqual(self.report["standalone_installers_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual([r["borg"] for r in self.rows], [r["id"] for r in json.loads(prior.read_bytes())["borgs"]])
        self.assertEqual(self.report["full_capacity_seats"], list(range(6)))
        self.assertEqual(self.report["limited_capacity_seat"], 0)
        self.assertEqual(self.report["queue_entries"], 2421)
        self.assertEqual(sum(len(row["body_descriptor_queue"]) for row in self.rows), 2421)
        self.assertTrue(all(len(row["observed_capacity_cases"]["32"]["secondary_index_queue_ordinals"]) == 1
                            for row in self.rows))

    def test_both_receive_lists_use_exact_cells_and_the_positive_guard_omits_ordinary_only(self):
        for row in self.rows:
            cases = row["observed_capacity_cases"]["32"]["published_queue_ordinals"]
            q = row["body_descriptor_queue"]
            expected = [i for i in reversed(range(len(q))) if int(q[i][3], 16) & 2]
            for mode in ("reset-ef", "declared-c3"):
                self.assertNotIn("ordinary-receiver", cases[mode])
                self.assertNotIn("alternate-receiver", cases[mode])
            self.assertEqual(cases["open"]["ordinary-receiver"], expected)
            self.assertEqual(cases["open"]["alternate-receiver"], expected)
            guard = cases["ordinary-receive-guard"]
            self.assertNotIn("ordinary-receiver", guard)
            self.assertEqual(guard["alternate-receiver"], expected)
            self.assertEqual({k: v for k, v in cases["open"].items() if k != "ordinary-receiver"}, guard)
        totals = self.report["full_capacity_registry_totals_per_one_seat"]
        self.assertEqual(totals["open"]["ordinary-receiver"], 1573)
        self.assertEqual(totals["ordinary-receive-guard"]["alternate-receiver"], 1573)
        self.assertEqual(totals["reset-ef"]["role-400"], 660)

    def test_repeated_ids_publish_twice_and_follow_cell_order(self):
        chosen = self.report["borgs_with_repeated_descriptors"]
        self.assertEqual(chosen, ["gg4e08:0901", "gg4e08:0a04", "gg4e08:0a08", "gg4e08:0a09"])
        for row in self.rows:
            if row["borg"] not in chosen:
                continue
            q = row["body_descriptor_queue"]
            self.assertEqual([j for j, d in enumerate(q) if d[0] == 13], [8, 13])
            visible = row["observed_capacity_cases"]["32"]["published_queue_ordinals"]["open"]["ordinary-receiver"]
            self.assertLess(visible.index(13), visible.index(8))

    def test_joint_switch_and_one_cell_allocation_do_not_pick_a_different_descriptor(self):
        one_cell_receivers = []
        for row in self.rows:
            q = row["body_descriptor_queue"]
            capacities = row["observed_capacity_cases"]
            full = capacities["32"]["published_queue_ordinals"]
            visible = full["joint-0-only"].get("ordinary-receiver", [])
            self.assertEqual(visible, [i for i in full["open"]["ordinary-receiver"] if q[i][2] in (0, 128)])
            self.assertEqual(full["joints-closed"], {})
            self.assertEqual(capacities["1"]["bound_cell_count"], 1)
            self.assertEqual(capacities["0"]["bound_cell_count"], 0)
            self.assertTrue(all(not v for v in capacities["0"]["published_queue_ordinals"].values()))
            first = capacities["1"]["published_queue_ordinals"]["open"].get("ordinary-receiver", [])
            if first:
                self.assertEqual(first, [0])
                one_cell_receivers.append(row["borg"])
        self.assertEqual(one_cell_receivers, ["gg4e08:0e04"])

    def test_refuses_unqualified_inputs_and_output_overwrite(self):
        with self.assertRaisesRegex(ValueError, "bank extent"):
            registry.body_queue(b"")
        with self.assertRaisesRegex(ValueError, "descriptor outside bank"):
            registry.body_queue(bytes([33]) + bytes(7839))
        with self.assertRaisesRegex(ValueError, "report extent"):
            registry.decode("", [(0, "", [registry.Shape(0, 0, 0, 2)])])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "registry.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                registry.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
