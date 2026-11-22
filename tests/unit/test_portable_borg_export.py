import tempfile
import unittest
from pathlib import Path

from tools.extract import portable_borg_add_ons, portable_borg_preview


class PortableBorgExport(unittest.TestCase):
    def test_add_on_packages_have_stable_ids_and_no_playable_manifest(self):
        with tempfile.TemporaryDirectory() as scratch:
            for model, expected in (
                ("0005", "normal-teleport-ninja-history"),
                ("0009", "normal-switching-ninja-exchange"),
                ("0d06", "normal-death-icbm-impact"),
            ):
                out = Path(scratch) / ("renamed-folder-" + model)
                self.assertEqual(portable_borg_add_ons.prepare(model, out), "gg4e08:" + model)
                self.assertEqual({p.name for p in out.iterdir()}, {"borg.manifest", "catalog.txt", "controllers.txt"})
                manifest = (out / "borg.manifest").read_text()
                self.assertIn(f"id=gg4e08:{model}\n", manifest)
                for key in ("model", "animations", "preview", "rules", "gameplay"):
                    self.assertNotIn("\n" + key + "=", manifest)
                definition = (out / "controllers.txt").read_text()
                self.assertIn(f"source=GG4E08:{model.upper()}/0\n", definition)
                self.assertIn(f"add-on={expected}\n", definition)
                with self.assertRaises(FileExistsError):
                    portable_borg_add_ons.prepare(model, out)
                self.assertEqual((out / "borg.manifest").read_text(), manifest)

    def test_preview_rejects_unqualified_art_before_writing(self):
        with tempfile.TemporaryDirectory() as scratch:
            art = Path(scratch) / "art"
            art.mkdir()
            bad = art / "g-red.hsd"
            bad.write_bytes(b"not the normal model")
            output = Path(scratch) / "package"
            with self.assertRaises(ValueError):
                portable_borg_preview.prepare("g-red", art, output)
            self.assertFalse(output.exists())
            self.assertEqual(bad.read_bytes(), b"not the normal model")
            with self.assertRaises(ValueError):
                portable_borg_preview.prepare("sasuke", art, output, art)
            self.assertFalse(output.exists())

    def test_add_on_refuses_invalid_member_before_writing(self):
        with tempfile.TemporaryDirectory() as scratch:
            member = Path(scratch) / "pl0d06.pzz"
            member.write_bytes(b"not an original member")
            output = Path(scratch) / "package"
            with self.assertRaises(ValueError):
                portable_borg_add_ons.prepare("0d06", output, member)
            self.assertFalse(output.exists())
            self.assertEqual(member.read_bytes(), b"not an original member")

    def test_battle_export_requires_reviewed_inputs_and_is_opt_in_for_retail(self):
        base = Path("local/content/native-battle-mvp-v1")
        if not (base / "sasuke.hsd").is_file():
            self.skipTest("optional original art is absent")
        with tempfile.TemporaryDirectory() as scratch:
            scratch = Path(scratch)
            api = scratch / "combat"
            preview = scratch / "preview"
            self.assertEqual(portable_borg_preview.prepare("sasuke", base, preview), "gg4e08:000a")
            self.assertEqual(portable_borg_preview.prepare("sasuke", base, api, battle_test=True), "gg4e08:000a")
            self.assertIn("gameplay=gameplay.txt\n", (api / "borg.manifest").read_text())
            self.assertNotIn("gameplay=", (preview / "borg.manifest").read_text())
            scout = scratch / "renamed-arcs"
            self.assertEqual(portable_borg_preview.prepare("arc-scout", base, scout), "test:arc-scout")
            self.assertIn("on-hit=arc|walk-speed|60|30\n", (scout / "gameplay.txt").read_text())
            grapple = scratch / "renamed-grapple"
            self.assertEqual(portable_borg_preview.prepare("grapple-scout", base, grapple), "test:grapple-scout")
            self.assertIn("on-hit=grapple|pull-self|36\n", (grapple / "gameplay.txt").read_text())
            charge = scratch / "renamed-charge"
            self.assertEqual(portable_borg_preview.prepare("charge-scout", base, charge), "test:charge-scout")
            self.assertIn("activation=pulse-charge|release-window|8|600\n", (charge / "gameplay.txt").read_text())
            self.assertIn("release=pulse-charge|3|none|0|surge\n", (charge / "gameplay.txt").read_text())
            blade = scratch / "renamed-blade"
            self.assertEqual(portable_borg_preview.prepare("blade-scout", base, blade), "test:blade-scout")
            self.assertIn("melee=close-cut|4|8|115|12|60|30|20\n", (blade / "gameplay.txt").read_text())
            rotary = scratch / "renamed-rotary"
            self.assertEqual(portable_borg_preview.prepare("rotary-scout", base, rotary), "test:rotary-scout")
            self.assertIn("held-fire=rotary-b|rotary-stock|1|rotary-shot|18|9|3|18\n", (rotary / "gameplay.txt").read_text())
            bomb = scratch / "renamed-bomb"
            self.assertEqual(portable_borg_preview.prepare("bomb-scout", base, bomb), "test:bomb-scout")
            self.assertIn("blast=fuse-bomb|expiry|110|32|opponent\n", (bomb / "gameplay.txt").read_text())
            remote = scratch / "renamed-remote"
            self.assertEqual(portable_borg_preview.prepare("remote-scout", base, remote), "test:remote-scout")
            self.assertIn("detonate=trigger|3|charge|landed\n", (remote / "gameplay.txt").read_text())
            self.assertIn("stock=charge-stock|3|on-return\n", (remote / "gameplay.txt").read_text())
            self.assertIn("return-stock=charge|remote\n", (remote / "gameplay.txt").read_text())
            seeker = scratch / "renamed-seeker"
            self.assertEqual(portable_borg_preview.prepare("seeker-scout", base, seeker), "test:seeker-scout")
            self.assertIn("guidance=seeker|tracking|0.12|24\n", (seeker / "gameplay.txt").read_text())
            guard = scratch / "renamed-guard"
            self.assertEqual(portable_borg_preview.prepare("guard-scout", base, guard), "test:guard-scout")
            self.assertIn("guard=brace|2|13|1.0|1|direct\n", (guard / "gameplay.txt").read_text())
            impact = scratch / "renamed-impact"
            self.assertEqual(portable_borg_preview.prepare("impact-scout", base, impact), "test:impact-scout")
            self.assertIn("blast=impact-arc|impact|170|28|opponent\n", (impact / "gameplay.txt").read_text())
            wingmate = scratch / "renamed-wingmate"
            self.assertEqual(portable_borg_preview.prepare("wingmate-scout", base, wingmate), "test:wingmate-scout")
            self.assertIn("deploy-emitter=deploy|2|right\n", (wingmate / "gameplay.txt").read_text())
            rewind = scratch / "renamed-rewind"
            self.assertEqual(portable_borg_preview.prepare("rewind-scout", base, rewind), "test:rewind-scout")
            self.assertIn("rewind=rewind|2|wire|16\n", (rewind / "gameplay.txt").read_text())
            self.assertIn("activation=rewind|double-tap|5|6\n", (rewind / "gameplay.txt").read_text())
            wire = scratch / "renamed-wire"
            self.assertEqual(portable_borg_preview.prepare("wire-trail-scout", base, wire), "test:wire-trail-scout")
            self.assertIn("on-hit=wire-shot|rewind-recipient|wire|16\n", (wire / "gameplay.txt").read_text())
            self.assertIn("empty-anchors=wire-shot|wire-plain\n", (wire / "gameplay.txt").read_text())
            arrow = scratch / "renamed-arrow"
            self.assertEqual(portable_borg_preview.prepare("anchor-arrow-scout", base, arrow), "test:anchor-arrow-scout")
            self.assertIn("plant-anchor=plant-arrow|arrow\n", (arrow / "gameplay.txt").read_text())
            with self.assertRaises(ValueError):
                portable_borg_preview.prepare("g-red", base, scratch / "unqualified", battle_test=True)
            self.assertFalse((scratch / "unqualified").exists())


if __name__ == "__main__":
    unittest.main()
