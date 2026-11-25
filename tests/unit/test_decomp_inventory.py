"""Pin the decompilation inventory grouping."""
import unittest

from tools.analyze import decomp_inventory as inventory


def row(address, size, status="matched", score=0):
    return {"address": address, "name": f"fn_{address:08X}", "size": size,
            "status": status, "score": score}


class DecompInventoryTest(unittest.TestCase):
    def test_function_address(self):
        self.assertEqual(inventory.function_address("fn_8002C4D0"), 0x8002C4D0)
        self.assertIsNone(inventory.function_address("not-a-function"))

    def test_groups_split_by_range_and_status(self):
        rows = [
            row(0x80028264, 92),
            row(0x8002BB14, 3712, "open"),
            row(0x8003D344, 164),
            row(0x8006FB08, 60),
            row(0x12345678, 4),
        ]
        groups = {entry["range"]: entry for entry in inventory.group(rows)}
        self.assertEqual(groups["scene-actor"]["matched"], 1)
        self.assertEqual(groups["collision-pair-reaction"]["open"], 1)
        self.assertEqual(groups["collision-pair-reaction"]["largest_open"][0]["name"],
                         "fn_8002BB14")
        self.assertEqual(groups["damage"]["matched"], 1)
        self.assertEqual(groups["action"]["matched"], 1)


if __name__ == "__main__":
    unittest.main()
