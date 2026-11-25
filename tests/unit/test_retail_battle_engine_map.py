"""Pin the PPC decoders used by the retail battle-engine map."""
import unittest

from tools.analyze import retail_battle_engine_map as engine


class BattleEngineDecodeTest(unittest.TestCase):
    def test_bl_detection_and_target(self):
        self.assertFalse(engine.is_bl(0x48000008))
        self.assertTrue(engine.is_bl(0x48000009))
        self.assertEqual(engine.bl_target(0x48000009, 0x80000000), 0x80000008)

    def test_branch_decoders(self):
        self.assertEqual(engine.branch_target(0x48000008, 0x80000000), 0x80000008)
        self.assertIsNone(engine.branch_target(0x48000009, 0x80000000))
        self.assertEqual(engine.bc_target(0x41820010, 0x80000000), 0x80000010)
        self.assertIsNone(engine.bc_target(0x48000008, 0x80000000))

    def test_blr_detection(self):
        self.assertTrue(engine.is_blr(0x4E800020))
        self.assertFalse(engine.is_blr(0x48000009))

    def test_lis_addi_address(self):
        # lis r4,0x8036 ; addi r4,r4,0x5AC8
        self.assertEqual(engine.lis_addi(0x3C808036, 0x38845AC8), 0x80365AC8)

    def test_lis_addi_rejects_different_register(self):
        self.assertIsNone(engine.lis_addi(0x3C808036, 0x38855AC8))


if __name__ == "__main__":
    unittest.main()
