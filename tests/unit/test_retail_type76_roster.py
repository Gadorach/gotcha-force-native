"""Protect normal type-76 ordinals and refusal of form damage attribution."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type76_paid as paid, retail_type76_roster as roster


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailType76RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        raw = (MAPS / "gg4e08-type76-roster.json").read_bytes()
        cls.census_digest = sha256(raw).hexdigest()
        cls.census = json.loads(raw)
        cls.paid = json.loads((MAPS / "gg4e08-type76-paid.json").read_bytes())

    def test_ordinals_choose_sockets_but_not_collision_groups_or_child_keys(self):
        rows = self.census["factory_rows"]
        self.assertEqual([r["key"] for r in rows], list(range(6)))
        for key, group, sockets in ((0, 2, [2, 3]), (2, 1, [8, 10])):
            row = rows[key]
            self.assertEqual(row["carrier_group"], group)
            self.assertEqual([v["socket"] for v in row["socket_variants"]], sockets)
            self.assertEqual(row["exit_descendant"], {"type": 8, "key": 0, "site": "800FD314"})
        for key in (1, 3, 4, 5):
            self.assertEqual(rows[key]["exit_descendant"], {"type": 9, "key": 1, "site": "800FD324"})
        self.assertEqual(self.census["active_descendant_select"]["different_helper_for_zero_hp_or_expiry"], "8001BDA8")

    def test_only_normal_routes_have_queues_and_those_queues_are_mixed(self):
        self.assertEqual(self.census_digest, paid.ROSTER_SHA256)
        seen = {(r["borg"], r["action"], r["parent_key"], r["debit"]["channel"]) for r in self.census["candidates"]}
        self.assertEqual(seen, {("gg4e08:0d01", "B-shot", 0, 0), ("gg4e08:0d01", "B-close", 0, 0),
                                ("gg4e08:0d04", "X", 2, 2)})
        for row in self.census["candidates"]:
            queue = row["normal_carrier_queue"]
            self.assertEqual(queue["status"], "mixed-power")
            self.assertEqual([s["raw_power"] for s in queue["steps"]], [40, 10])
            self.assertNotIn("hp_loss", row)
        unresolved = self.census["unqualified_installed_route"]
        self.assertEqual((unresolved["borg"], unresolved["spec_record"], unresolved["site"]), ("gg4e08:0620", 2, "8015C770"))
        self.assertEqual(self.census["counts"]["unqualified_sites"], 4)

    def test_paid_refusal_advances_cursor_and_empty_stock_does_not(self):
        self.assertEqual(len(self.paid["cases"]), 8)
        for one in self.paid["cases"][1::4]:
            first, second, empty = one["visits"]
            self.assertEqual((first["debit_ok"], first["carrier_allocated"], first["cursor_after"]), (True, False, 1))
            self.assertEqual((second["requested_parent_ordinal"], second["carrier_allocated"], second["stock_after"]), (1, True, 0))
            self.assertEqual((empty["debit_ok"], empty["requested_parent_ordinal"], empty["cursor_after"]), (False, None, 0))
        for zero in self.paid["cases"][3::4]:
            self.assertTrue(all(v["cursor_after"] == 0 and not v["carrier_allocated"] for v in zero["visits"]))

    def test_paid_report_refuses_reuse_of_an_unallocated_ordinal(self):
        lines = []
        for row in self.paid["cases"]:
            for visit in row["visits"]:
                values = (int(row["borg"][-4:], 16), paid.CASES.index(row["case"]), visit["visit"],
                          visit["requested_parent_ordinal"] or 0, visit["debit_ok"], visit["carrier_allocated"],
                          visit["cursor_after"], visit["stock_after"])
                lines.append("type76-paid: " + " ".join(str(int(v)) for v in values))
        report = "\n".join(lines)
        self.assertEqual(paid.decode_report(report), self.paid["cases"])
        old = "3329 1 1 0 1 0 1 1"
        self.assertIn(old, report)
        with self.assertRaises(ValueError):
            paid.decode_report(report.replace(old, "3329 1 1 0 1 0 0 1", 1))

    def test_exporters_refuse_existing_receipts(self):
        with TemporaryDirectory() as tmp:
            target = Path(tmp) / "test.json"
            target.write_text("keep")
            with self.assertRaises(FileExistsError):
                roster.export_census(None, None, None, None, target, roster.recover)
            with self.assertRaises(FileExistsError):
                paid.export(None, None, None, None, target)
            self.assertEqual(target.read_text(), "keep")
