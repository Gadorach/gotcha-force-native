from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import TestCase
from unittest.mock import Mock, patch
import struct

from tools.extract.retail_borg_placeholders import Borg
from tools.extract import retail_borg_review as review


class RetailBorgReview(TestCase):
    def setUp(self):
        tmp = TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.output = Path(tmp.name) / "double"
        self.borg = Borg(3, 17, 0, "DOUBLE NINJA", 210)
        self.actor = bytearray(432)
        self.actor[0x1A0:0x1AD] = bytes((3, 0, 0, 3, 2, 3, 3, 5, 1, 2, 255, 10, 0))
        self.wrapper = bytearray(64)
        struct.pack_into(">I", self.wrapper, 0, 32)
        struct.pack_into(">I", self.wrapper, 32, 32)
        self.member = Mock()
        self.motions = b"motion"
        self.member.read.side_effect = lambda index, _: (
            {0: self.actor, 3: self.motions, 4: self.wrapper}[index], 0)

    def export(self, borg="double-ninja"):
        levels = {
            "double-ninja": ((150, 10), (195, 10)),
            "gatling-gunner": ((250, 100), (295, 100)),
            "revolver-gunman": ((150, 20), (195, 20)),
            "death-borg-beta-ii": ((50, 5), (68, 14)),
            "normal-knight": ((180, 0, 0, 0, 0, 1), (225, 0, 0, 0, 0, 1)),
            "normal-samurai": ((220, 5), (310, 5)),
            "normal-tank": ((300, 20), (390, 20)),
            "battle-girl": ((130, 10), (175, 10)),
            "thunder-robot": ((350, 4), (440, 4)),
            "magnet-robot-plus": ((350, 10, 10, 0, 120, 1), (440, 10, 10, 0, 120, 1)),
            "magnet-robot-minus": ((350, 4, 4, 0, 180, 1), (440, 4, 4, 0, 180, 1)),
            "kung-fu-master": ((180, 5), (270, 5)),
            "angel-nurse": ((150, 10, 10, 0, 300, 1), (195, 10, 10, 0, 300, 1)),
            "patra-witch": ((150, 10, 10, 0, 300, 2), (195, 10, 10, 0, 300, 2)),
            "acceleration-ninja": ((200, 5), (245, 5)),
            "pop-honey": ((150, 5, 5, 0, 300, 1), (195, 5, 5, 0, 300, 1)),
            "roach": ((60, 1), (78, 1)),
            "death-bomb": ((30,), (39,)),
            "blue-striker": ((150, 40, 40, 0, 300, 4), (168, 40, 40, 0, 300, 4)),
            "wing-soldier": ((150, 10), (195, 10)),
            "quick-valkrie": ((120, 10), (165, 10)),
            "beam-satellite": ((1000, 8), (1450, 8)),
            "gold-hero": ((200, 10), (290, 10)),
            "titan-robot": ((450, 20), (540, 20)),
            "copy-man": ((200,), (290,)),
            "flame-dragon": ((1000, 5, 5, 1, 300, 1), (1450, 5, 5, 1, 300, 1)),
            "ice-dragon": ((500, 5, 5, 1, 300, 1), (680, 5, 5, 1, 300, 1)),
            "demon-samurai": ((220, 1), (310, 1)),
            "shuriken-ninja": ((200, 30), (245, 30)),
            "beam-gunner": ((300, 5, 5, 1, 60, 1), (345, 5, 5, 1, 60, 1)),
            "neo-g-red": ((400, 10), (490, 13)),
            "machine-head": ((250, 10, 10, 0, 180, 1), (340, 10, 10, 0, 180, 1)),
            "mechanical-dragon": ((1000, 5, 5, 0, 300, 8), (1450, 5, 5, 0, 300, 8)),
            "machine-dragon": ((1250,), (1790,)),
            "flying-saucer": ((250, 10, 10, 0, 120, 1), (295, 10, 10, 0, 120, 1)),
            "death-head": ((500, 10, 10, 0, 180, 1), (590, 10, 10, 0, 180, 1)),
            "galactic-emperor": ((2000,), (6500,)),
            "jack": ((200, 5, 5, 0, 120, 1), (245, 8, 8, 0, 120, 1)),
            "jet-hero": ((160, 10), (205, 10)),
            "normal-ninja": ((150, 10, 10, 0, 90, 5), (195, 10, 10, 0, 90, 5)),
            "command-girl": ((200, 10, 10, 0, 120, 3), (245, 10, 10, 0, 120, 3)),
            "plasma-dragon": ((1000, 5, 5, 1, 300, 1), (1450, 5, 5, 1, 300, 1)),
            "thunder-dragon": ((500, 5, 5, 1, 300, 1), (680, 5, 5, 1, 300, 1)),
            "g-black": ((350, 5), (440, 8)),
            "ultimate-cannon": ((500, 200, 200, 0, 240, 1), (590, 200, 200, 0, 240, 1)),
            "cyber-girl-super": ((250, 4, 4, 0, 150, 6), (295, 4, 4, 0, 150, 6)),
            "cyber-girl-hyper": ((300, 8, 8, 0, 240, 12), (345, 8, 8, 0, 240, 12)),
            "wire-gunner": ((220, 20, 20, 0, 90, 4), (265, 20, 20, 0, 90, 4)),
            "wire-girl": ((140, 10, 10, 0, 60, 4), (185, 10, 10, 0, 60, 4)),
            "akuma-samurai": ((250, 5), (340, 5)),
            "titan-tank": ((400, 5, 5, 0, 180, 3), (490, 5, 5, 0, 180, 3)),
            "time-bomber": ((220, 10, 10, 2, 0, 3), (265, 10, 10, 2, 0, 3)),
            "remote-bomber": ((220, 8), (265, 8)),
            "cosmic-dragon": ((1300, 5, 5, 1, 300, 1), (1750, 5, 5, 1, 300, 1)),
            "space-dragon": ((650, 5, 5, 1, 300, 1), (740, 5, 5, 1, 300, 1)),
            "machine-red": ((500, 5, 5, 0, 300, 2), (590, 5, 5, 0, 300, 2)),
            "machine-blue": ((550, 4, 4, 0, 120, 5), (640, 4, 4, 0, 120, 5)),
            "cyber-machine-seiryu": ((1050,), (1230,)),
        }[borg]
        with (patch.object(review, "read_catalog", return_value=(self.borg,)),
              patch.object(review, "source_member", return_value=self.member) as source,
              patch.object(review, "normal_level_endpoints", return_value=levels)):
            result = review.prepare(borg, "catalog", "dol", "archive", self.output)
            source.assert_called_once_with("archive", review.REVIEWS[borg])
            return result

    def test_preview_is_not_a_controller_or_variant_fallback(self):
        self.assertEqual(self.export(), "gg4e08:0003")
        manifest = (self.output / "borg.manifest").read_text()
        self.assertIn("preview=preview.txt", manifest)
        self.assertNotIn("gameplay=", manifest)
        self.assertNotIn("rules=", manifest)
        catalogue = (self.output / "catalog.txt").read_text()
        self.assertIn("hp=150|195\nratings=2|3|3|5\n", catalogue)
        self.assertEqual([line for line in catalogue.splitlines() if line.startswith("variant=")],
                         ["variant=normal|Normal|210|placeholder"])
        moves = (self.output / "moves.txt").read_text()
        self.assertIn("move=ninpo-kage-bunshin|base|x|special|Ninpo Kage Bunshin\n", moves)
        self.assertNotIn("outcome=", moves)
        with self.assertRaises(FileExistsError):
            self.export()
        self.assertEqual((self.output / "borg.manifest").read_text(), manifest)

    def test_bad_source_never_publishes_a_package(self):
        self.actor[0x1AC] = 255
        with self.assertRaisesRegex(ValueError, "unexpected Double Ninja display slots"):
            self.export()
        self.assertFalse(self.output.exists())
        self.actor[0x1AC] = 0
        struct.pack_into(">I", self.wrapper, 32, 40)
        with self.assertRaisesRegex(ValueError, "body extent"):
            self.export()
        self.assertFalse(self.output.exists())

    def test_gatling_stock_is_review_metadata_only(self):
        self.borg = Borg(0x0102, 115, 1, "GATLING GUNNER", 420)
        self.actor[0x1A0:0x1AD] = bytes((0, 0, 0, 3, 3, 6, 0, 4, 1, 255, 255, 10, 255))
        self.assertEqual(self.export("gatling-gunner"), "gg4e08:0102")
        manifest = (self.output / "borg.manifest").read_text()
        self.assertIn("name=Gatling Gunner", manifest)
        self.assertNotIn("gameplay=", manifest)
        catalogue = (self.output / "catalog.txt").read_text()
        self.assertIn("hp=250|295\nratings=3|6|0|4\n", catalogue)
        self.assertIn("display-jump=unresolved\n", catalogue)
        moves = (self.output / "moves.txt").read_text()
        b, x = moves.split("move=hand-grenade")
        self.assertIn("ammo=finite:100|100\n", b)
        self.assertIn("activation=hold\n", b)
        self.assertIn("ammo=unresolved\n", x)
        self.assertNotIn("outcome=", moves)

    def test_other_borg_member_data_cannot_fill_gatling(self):
        self.borg = Borg(0x0102, 115, 1, "GATLING GUNNER", 420)
        with self.assertRaisesRegex(ValueError, "unexpected Gatling Gunner display slots"):
            self.export("gatling-gunner")
        self.assertFalse(self.output.exists())

    def test_audited_pairs_export_their_own_sources_without_playable_rules(self):
        parent = self.output
        parent.mkdir()
        for key, source, number, cost, hp, stock, air, start in (
            ("revolver-gunman", 0x0100, 21, 250, "150|195", "20|20", "unresolved", "0:0"),
            ("death-borg-beta-ii", 0x0105, 79, 100, "50|68", "5|14", "unresolved", "0:0"),
            ("normal-knight", 0x020A, 26, 180, "180|225", "1|1", "air-jumps:3", "0:0"),
            ("normal-samurai", 0x0700, 39, 300, "220|310", None, "air-jumps:3", "0:2"),
        ):
            with self.subTest(borg=key):
                self.output = parent / key
                preset = review.REVIEWS[key]
                self.borg = Borg(source, number - 1, 0, preset.name.upper(), cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{source:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                self.assertIn(f"idle={start}\n", (self.output / "preview.txt").read_text())
                catalogue = (self.output / "catalog.txt").read_text()
                self.assertIn(f"hp={hp}\n", catalogue)
                self.assertIn("controllers=controllers.txt\n", catalogue)
                controller = (self.output / "controllers.txt").read_text()
                self.assertIn(f"source=GG4E08:{source:04X}/0\n", controller)
                self.assertIn(f"air-resource={air}\n", controller)
                self.assertIn("contact=unresolved\n", controller)
                moves = (self.output / "moves.txt").read_text()
                actual = [line for line in moves.splitlines() if line.startswith("ammo=finite:")]
                self.assertEqual(actual, [f"ammo=finite:{stock}"] if stock else [])

    def test_unsupported_normal_tank_stays_metadata_only(self):
        preset = review.REVIEWS["normal-tank"]
        self.borg = Borg(0x0C00, 63, 16, "NORMAL TANK", 450)
        self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
        self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
        self.assertEqual(self.export("normal-tank"), "gg4e08:0c00")
        self.assertEqual({file.name for file in self.output.iterdir()},
                         {"borg.manifest", "catalog.txt", "moves.txt", "controllers.txt"})
        manifest = (self.output / "borg.manifest").read_text()
        for key in ("preview=", "model=", "animations=", "gameplay=", "rules="):
            self.assertNotIn(key, manifest)
        controller = (self.output / "controllers.txt").read_text()
        self.assertIn("source-movement-type=1\nselector-family=cohort-b\nmovement=unresolved\n", controller)
        self.assertIn("air-resource=boost-meter\nrecovery=grounded\n", controller)

    def test_battle_girl_can_preview_without_inheriting_tank_controller(self):
        preset = review.REVIEWS["battle-girl"]
        self.borg = Borg(0x0300, 33, 3, "BATTLE GIRL", 230)
        self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
        self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
        self.assertEqual(self.export("battle-girl"), "gg4e08:0300")
        controller = (self.output / "controllers.txt").read_text()
        self.assertIn("source=GG4E08:0300/0\nsource-movement-type=0\n", controller)
        self.assertIn("selector-family=cohort-b\nmovement=grounded\n", controller)
        self.assertTrue((self.output / "preview.txt").is_file())
        self.assertNotIn("gameplay=", (self.output / "borg.manifest").read_text())

    def test_machine_reviews_preserve_identity_but_do_not_declare_x_behavior(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost, b_stock, x_move in (
            ("thunder-robot", 0x0404, 60, 400, 4, "thunder-wire"),
            ("magnet-robot-plus", 0x0405, 61, 400, 10, "magnet-scope"),
            ("magnet-robot-minus", 0x0409, 62, 410, 4, "magnet-scope"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 4, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A4:0x1A8] = bytes((4, 3, 5, 4))
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertIn(f"name={preset.name}\n", manifest)
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                catalogue = (self.output / "catalog.txt").read_text()
                self.assertIn(f"variant=normal|Normal|{cost}|placeholder\n", catalogue)
                self.assertIn("hp=350|440\nratings=4|3|5|4\n", catalogue)
                controller = (self.output / "controllers.txt").read_text()
                self.assertIn(f"source=GG4E08:{model:04X}/0\n", controller)
                self.assertIn("air-resource=unresolved\n", controller)
                moves = (self.output / "moves.txt").read_text()
                b, x = moves.split(f"move={x_move}|")
                self.assertIn(f"ammo=finite:{b_stock}|{b_stock}\n", b)
                self.assertIn("ammo=unresolved\n" if key == "thunder-robot" else "ammo=finite:1|1\n", x)
                self.assertIn("activation=unresolved\n", x)
                self.assertNotIn("outcome=", moves)
                self.assertNotIn("|0:1", (self.output / "preview.txt").read_text())

    def test_support_and_paid_shot_evidence_remain_non_executable(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost, phase, outcome in (
            ("kung-fu-master", 0x0800, 41, 260, "launch-request|program-event-3-1", None),
            ("angel-nurse", 0x0900, 47, 240, "shape-request|lane-gated-program-command",
             "heal|qualified-ally-contact|unresolved|ally-hit"),
            ("patra-witch", 0x0902, 48, 150, "cast-request|program-event-1-1",
             "recipient-effect|qualified-carrier-contact|unresolved|contact-target"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 8, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertIn(f"phase={phase}|", moves)
                self.assertNotIn("release=", moves)
                if outcome:
                    self.assertIn(f"outcome={outcome}|unresolved|unresolved\n", moves)
                else:
                    self.assertNotIn("outcome=", moves)
                if key == "kung-fu-master":
                    self.assertIn("clip=clip-3-11|3:11\n", (self.output / "preview.txt").read_text())

    def test_stance_and_targeting_status_metadata_do_not_admit_battle(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, event, outcome in (
            ("acceleration-ninja", 4, 19, 0, 380, "lane-gated-program-command", "passive-contact"),
            ("pop-honey", 0x0904, 50, 10, 250, "active-carrier-timer", "unresolved"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertIn(f"|{event}|", moves)
                self.assertIn(f"|{outcome}|contact-target|unresolved|unresolved\n", moves)
                self.assertNotIn("release=", moves)

    def test_static_bodies_require_empty_source_motion_and_do_not_admit_battle(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, pose, air in (
            ("roach", 0x0F05, 189, 11, 30, "control-only", "air-jumps:1"),
            ("death-bomb", 0x0D07, 108, 19, 20, "class-owned", "unresolved"),
            ("blue-striker", 0x0D00, 70, 17, 200, "unresolved", "boost-meter"),
            ("beam-satellite", 0x0E01, 148, 18, 800, "unresolved", "unresolved"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.motions = b"not an empty bank"
                with self.assertRaisesRegex(ValueError, "actor, motion or wrapper extent"):
                    self.export(key)
                self.assertFalse(self.output.exists())
                self.motions = b""
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("animations=", manifest)
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                self.assertFalse((self.output / "visual/body.motions").exists())
                preview = (self.output / "preview.txt").read_text()
                self.assertIn("idle=none\n", preview)
                self.assertNotIn("clip=", preview)
                controller = (self.output / "controllers.txt").read_text()
                self.assertIn(f"motion-bank=none\npose={pose}\n", controller)
                self.assertIn(f"air-resource={air}\n", controller)

    def test_glide_review_does_not_bind_fly_or_inherit_bank(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, clip in (
            ("wing-soldier", 0x0A00, 52, 14, 200, "0:12"),
            ("quick-valkrie", 0x0B00, 53, 15, 230, "4:0"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                controller = (self.output / "controllers.txt").read_text()
                self.assertIn(f"source=GG4E08:{model:04X}/0\nsource-movement-type=2\n", controller)
                self.assertIn("air-resource=flight-meter\nrecovery=unresolved\n", controller)
                preview = (self.output / "preview.txt").read_text()
                self.assertIn(f"|{clip}\n", preview)
                bound_fly = (self.output / "moves.txt").read_text().split("move=fly|")[1]
                self.assertIn("activation=unresolved\n", bound_fly)
                self.assertIn("phase=unresolved-phase|", bound_fly)

    def test_fixed_form_review_and_dynamic_copy_are_distinct(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, transition in (
            ("gold-hero", 0x0802, 122, 13, 420, "expire-powered|powered|base|return"),
            ("titan-robot", 0x0604, 166, 4, 1000, "select-robot|tank|base|selected"),
            ("copy-man", 0x0806, 44, 7, 350, None),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor = bytearray(432 * preset.actor_records)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                catalogue = (self.output / "catalog.txt").read_text()
                if transition:
                    self.assertIn("forms=forms.txt\n", catalogue)
                    graph = (self.output / "forms.txt").read_text()
                    self.assertIn(f"transition={transition}\n", graph)
                    self.assertIn("transfer=unresolved\n", graph)
                    self.assertNotIn("model=inherit", graph)
                    self.assertEqual(self.member.read.call_args_list[-3].args, (0, 864))
                    if key == "titan-robot":
                        self.assertNotIn("model=", manifest)
                        self.assertNotIn("preview=", manifest)
                        moves = (self.output / "moves.txt").read_text()
                        self.assertIn("move=transform-return|tank|", moves)
                else:
                    self.assertFalse((self.output / "forms.txt").exists())
                    self.assertIn("forms=unresolved\n", catalogue)
                    self.assertIn("outcome=same-slot-reload|qualified-head-contact|transform|self|",
                                  (self.output / "moves.txt").read_text())

    def test_dragon_child_and_demon_return_reviews_are_not_live_effects(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, declaration in (
            ("flame-dragon", 0x0500, 149, 5, 1500, "contact-child-request|active-visual-object"),
            ("ice-dragon", 0x0516, 113, 5, 1050, "impact-handoff|accepted-carrier-contact"),
            ("demon-samurai", 0x0701, 186, 12, 480, "return-gate|both-owner-flags-clear"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertIn(f"phase={declaration}|", moves)
                self.assertNotIn("outcome=", moves)
                self.assertNotIn("release=", moves)
                if key == "ice-dragon":
                    self.assertFalse((self.output / "controllers.txt").exists())
                if key == "demon-samurai":
                    self.assertIn("idle=0:2\n", (self.output / "preview.txt").read_text())
                    slash = moves.split("move=random-slash|")[1]
                    self.assertIn("ammo=none\n", slash)

    def test_equipment_reviews_keep_pose_samples_and_requests_unbound(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, event, raw in (
            ("shuriken-ninja", 0x0001, 16, 0, 310, "program-part-selection", "4:2"),
            ("beam-gunner", 0x0104, 154, 1, 650, "accepted-late-debit", "4:2"),
            ("neo-g-red", 0x0629, 14, 4, 500, "authored-opening", "4:3"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertIn(f"|{event}|", moves)
                self.assertNotIn("release=", moves)
                self.assertNotIn("outcome=", moves)
                preview = (self.output / "preview.txt").read_text()
                self.assertIn(f"|{raw}\n", preview)
                if key == "beam-gunner":
                    self.assertNotIn("|0:2\n", preview)

    def test_battle_fusion_review_does_not_alias_catalogue_machine_dragon(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost in (
            ("machine-head", 0x0511, 198, 300),
            ("mechanical-dragon", 0x0512, 199, 1200),
            ("machine-dragon", 0x050F, 200, 1500),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 5, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.motions = b"" if preset.static_preview else b"motion"
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                catalogue = (self.output / "catalog.txt").read_text()
                if key == "machine-head":
                    self.assertIn("fusions=fusions.txt\n", catalogue)
                    fusion = (self.output / "fusions.txt").read_text()
                    self.assertIn("first=gg4e08:0511|unresolved|normal\n", fusion)
                    self.assertIn("second=gg4e08:0512|unresolved|normal\n", fusion)
                    self.assertIn("result=unresolved|unresolved|unresolved\n", fusion)
                    self.assertNotIn("result=gg4e08:050f", fusion)
                    self.assertNotIn("animations=", manifest)
                else:
                    self.assertFalse((self.output / "fusions.txt").exists())
                if key == "machine-dragon":
                    self.assertIn("moves=unresolved\n", catalogue)
                    self.assertFalse((self.output / "moves.txt").exists())

    def test_sweep_operands_and_emperor_pose_do_not_bind_gameplay(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, expected_type, event in (
            ("flying-saucer", 0x0D03, 195, 17, 600, 18, "qualified-scratch-endpoints"),
            ("death-head", 0x0505, 202, 5, 500, 2, "model-relative-endpoints"),
            ("galactic-emperor", 0x0E04, 205, 18, 3000, 8, None),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.motions = b""
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("animations=", manifest)
                self.assertIn(f"source-movement-type={expected_type}\n", (self.output / "controllers.txt").read_text())
                if event:
                    moves = (self.output / "moves.txt").read_text()
                    self.assertIn(f"phase=segment-request|{event}|", moves)
                    self.assertNotIn("release=", moves)
                else:
                    self.assertFalse((self.output / "moves.txt").exists())
                    self.assertFalse((self.output / "spawn.txt").exists())

    def test_scope_reviews_preserve_distinct_phases_with_no_field_binding(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, move_id, event, stock in (
            ("jack", 0x080E, 11, 7, 280, "jelly-scope", "accepted-group-bind", "finite:1|1"),
            ("jet-hero", 0x0A01, 46, 13, 350, "power-scope", "accepted-scope-registration", "none"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                scope = (self.output / "moves.txt").read_text().split(f"move={move_id}|")[1].split("move=")[0]
                self.assertIn(f"|{event}|", scope)
                self.assertIn(f"ammo={stock}\n", scope)
                self.assertNotIn("field=", scope)
                self.assertNotIn("outcome=", scope)

    def test_explosion_leads_do_not_generate_hit_outcomes(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, launch, next_phase in (
            ("normal-ninja", 0, 15, 0, 150, "accepted-channel-two-debit", "carrier-contact-or-response-gate"),
            ("command-girl", 0x0306, 36, 3, 330, "accepted-channel-zero-debit", "qualified-active-x-branch"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                self.assertNotIn("gameplay=", (self.output / "borg.manifest").read_text())
                moves = (self.output / "moves.txt").read_text()
                self.assertIn(f"|{launch}|", moves)
                self.assertIn(f"|{next_phase}|", moves)
                self.assertNotIn("outcome=", moves)
                self.assertNotIn("release=", moves)

    def test_dragon_impact_contact_evidence_is_not_a_damage_binding(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost in (
            ("plasma-dragon", 0x0509, 151, 1600),
            ("thunder-dragon", 0x0515, 112, 1100),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 5, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                self.assertNotIn("gameplay=", (self.output / "borg.manifest").read_text())
                moves = (self.output / "moves.txt").read_text()
                self.assertIn("outcome=emitter-hit|qualified-group-seven-contact|damage|contact-target|unresolved|unresolved\n", moves)
                self.assertIn("outcome=descendant-hit|qualified-group-eight-contact|damage|contact-target|unresolved|unresolved\n", moves)
                self.assertNotIn("impact-hazard=", moves)
                self.assertFalse((self.output / "controllers.txt").exists())

    def test_large_display_stock_and_distinct_charges_do_not_weaken_runtime_cap(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost in (
            ("g-black", 0x062A, 206, 4, 600),
            ("ultimate-cannon", 0x0C04, 145, 16, 1500),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                moves = (self.output / "moves.txt").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("release=", moves)
                if key == "g-black":
                    self.assertIn("move=buster-laser|base|b|", moves)
                    self.assertIn("move=black-buster|base|x|", moves)
                    self.assertIn("phase=charged-b-request|qualified-charged-b-cue|", moves)
                    self.assertIn("phase=charged-x-request|qualified-charged-x-cue|", moves)
                else:
                    self.assertIn("ammo=finite:200|200\n", moves)
                    self.assertIn("phase=paid-beam-requests|qualified-ordinary-x-gate|", moves)
                    ultimate = moves.split("move=ultimate-beam|")[1]
                    self.assertIn("phase=unresolved-phase|", ultimate)
                    self.assertNotIn("paid-beam-requests", ultimate)

    def test_cyber_girl_dependents_are_inspection_only(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost, shot, remote in (
            ("cyber-girl-super", 0x0305, 120, 590, 4, 6),
            ("cyber-girl-hyper", 0x030A, 157, 620, 8, 12),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 3, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                b, x = (self.output / "moves.txt").read_text().split("move=remote-beam|")
                self.assertIn(f"ammo=finite:{shot}|{shot}\n", b)
                self.assertIn(f"ammo=finite:{remote}|{remote}\n", x)
                self.assertIn("phase=remote-object-request|qualified-cue-and-owner-mask|", x)
                self.assertNotIn("deploy-emitter=", x)
                self.assertNotIn("outcome=", x)

    def test_wire_reviews_do_not_encode_unverified_path_or_gesture(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost in (
            ("wire-gunner", 0x0103, 23, 1, 270),
            ("wire-girl", 0x0301, 35, 3, 220),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                self.assertNotIn("rules=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertIn("ammo=finite:4|4\n", moves)
                for row in ("place-anchor=", "rewind=", "double-tap", "on-hit="):
                    self.assertNotIn(row, moves)
                if key == "wire-girl":
                    self.assertIn("phase=owner-stock-return|qualified-anchor-return|", moves)
                    self.assertNotIn("|3:0\n", (self.output / "preview.txt").read_text())

    def test_titan_tank_and_akuma_do_not_borrow_other_borg_rules(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, tribe, cost, phase in (
            ("akuma-samurai", 0x0708, 185, 12, 480, "paid-beam-request|accepted-channel-zero-debit"),
            ("titan-tank", 0x0619, 66, 16, 620, "paid-javelin-pair|accepted-channel-two-debit"),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, tribe, preset.name, cost)
                self.actor = bytearray(432 * preset.actor_records)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertIn(f"phase={phase}|", moves)
                self.assertNotIn("release-followup=", moves)
                self.assertNotIn("release-return=", moves)
                self.assertFalse((self.output / "forms.txt").exists())
                if key == "titan-tank":
                    self.assertNotIn("preview=", manifest)
                    self.assertNotIn("model=", manifest)
                    self.assertEqual(self.member.read.call_args_list[-3].args, (0, 864))
                else:
                    self.assertIn("idle=0:2\n", (self.output / "preview.txt").read_text())

    def test_bomber_stockless_command_is_not_broadcast_detonation(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost in (
            ("time-bomber", 0x0106, 24, 220),
            ("remote-bomber", 0x0107, 25, 180),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 1, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertNotIn("gameplay=", manifest)
                moves = (self.output / "moves.txt").read_text()
                self.assertNotIn("detonate=", moves)
                self.assertNotIn("return-stock=", moves)
                if key == "remote-bomber":
                    self.assertNotIn("preview=", manifest)
                    self.assertIn("ammo=finite:8|8\n", moves)
                    x = moves.split("move=detonation|")[1]
                    self.assertIn("ammo=none\n", x)
                    self.assertIn("phase=charge-consume|active-charge-consumes-latch|", x)
                else:
                    self.assertIn("preview=preview.txt\n", manifest)
                    self.assertIn("ammo=finite:10|10\n", moves)
                    self.assertIn("ammo=finite:3|3\n", moves)

    def test_black_hole_reviews_pin_charged_x_without_granting_field_outcomes(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost in (
            ("cosmic-dragon", 0x0504, 201, 1700),
            ("space-dragon", 0x0510, 197, 1200),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 5, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                self.assertNotIn("gameplay=", (self.output / "borg.manifest").read_text())
                moves = (self.output / "moves.txt").read_text()
                self.assertIn("move=black-hole|base|x|special|Black Hole\n", moves)
                phase = moves.split("move=black-hole|")[1]
                self.assertIn("ammo=unresolved\n", phase)
                self.assertIn("phase=owner-slot-publication|qualified-growth-gate|", phase)
                self.assertNotIn("outcome=", phase)
                self.assertNotIn("field-shot-pull=", phase)

    def test_seiryu_review_references_only_the_named_primary(self):
        parent = self.output
        parent.mkdir()
        for key, model, number, cost, stock in (
            ("machine-red", 0x0600, 164, 900, ("5|5", "2|2")),
            ("machine-blue", 0x0601, 165, 850, ("4|4", "5|5")),
            ("cyber-machine-seiryu", 0x0602, 176, 1750, ()),
        ):
            with self.subTest(borg=key):
                preset = review.REVIEWS[key]
                self.output = parent / key
                self.borg = Borg(model, number - 1, 4, preset.name, cost)
                self.actor[0x1A0:0x1A3] = bytes(preset.jump_fields)
                self.actor[0x1A8:0x1AD] = bytes(preset.move_slots)
                self.assertEqual(self.export(key), f"gg4e08:{model:04x}")
                manifest = (self.output / "borg.manifest").read_text()
                self.assertIn("preview=preview.txt\n", manifest)
                self.assertNotIn("gameplay=", manifest)
                catalogue = (self.output / "catalog.txt").read_text()
                self.assertIn(f"variant=normal|Normal|{cost}|placeholder\n", catalogue)
                if stock:
                    moves = (self.output / "moves.txt").read_text()
                    self.assertEqual([line for line in moves.splitlines() if line.startswith("ammo=")],
                                     [f"ammo=finite:{stock[0]}", "ammo=unresolved", f"ammo=finite:{stock[1]}"])
                    self.assertNotIn("outcome=", moves)
                else:
                    self.assertIn("moves=unresolved\n", catalogue)
                    self.assertFalse((self.output / "moves.txt").exists())
                if key == "machine-red":
                    self.assertIn("phase=carrier-request|accepted-channel-zero-debit|", moves)
                    self.assertIn("fusions=fusions.txt\n", catalogue)
                    fusion = (self.output / "fusions.txt").read_text()
                    self.assertIn("first=gg4e08:0600|unresolved|normal\n", fusion)
                    self.assertIn("second=gg4e08:0601|unresolved|normal\n", fusion)
                    self.assertIn("result=gg4e08:0602|unresolved|normal\n", fusion)
                    self.assertIn("result-use=fusion-only\n", fusion)
                    self.assertIn("transfer=unresolved|unresolved\n", fusion)
                    self.assertNotIn("gg4e08:0603", fusion)
                    self.assertNotIn("4:1", (self.output / "preview.txt").read_text())
                else:
                    self.assertIn("fusions=unresolved\n", catalogue)
                    self.assertFalse((self.output / "fusions.txt").exists())
                if key == "machine-blue":
                    self.assertIn("phase=indexed-carrier-request|accepted-channel-zero-debit|", moves)
                    self.assertIn("phase=paired-carrier-requests|accepted-channel-two-debit|", moves)
