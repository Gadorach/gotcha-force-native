import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


@unittest.skipUnless(os.environ.get("GF_SOURCE_WORLD_BINARY"), "source ground host not requested")
class SourceGroundWorldTests(unittest.TestCase):
    def run_host(self, args, *, cwd=None):
        binary = str(Path(os.environ["GF_SOURCE_WORLD_BINARY"]).resolve(strict=True))
        return subprocess.run([binary, *args], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, timeout=90, cwd=cwd)

    def normal_practice_fixture(self, root):
        content = Path(__file__).resolve().parents[2] / "local/content"
        art = content / "native-battle-mvp-v1"
        source = content / "source-gred-ground-v9"
        stage_files = ("stage.hsd", "floor.stih", "sasuke.hsd", "sasuke.prefix", "sasuke.pose", "sasuke.motions")
        stage_grids = ("floor.stih", "secondary.stih", "tertiary.stih")
        inputs = {
            "body.hsd": art / "g-red.hsd",
            "motions.bank": art / "g-red.motions",
            "rig.prefix": art / "g-red.prefix",
            "rig.pose": art / "g-red.pose",
            "locomotion.gfmp": source / "locomotion.gfmp",
            "height.gfhc": source / "pose-height.gfhc",
            "input.gfmi": source / "movement.gfmi",
            "motion.gfmm": source / "motion.gfmm",
        }
        if not all(path.is_file() for path in (*inputs.values(), *(art / x for x in stage_files), *(source / x for x in stage_grids))):
            self.skipTest("qualified local stage and practice inputs not present")
        package = root / "borgs/initial-folder"
        package.mkdir(parents=True)
        for directory, names in ((art, stage_files), (source, stage_grids)):
            target = root / "local/content" / directory.name
            target.mkdir(parents=True)
            for name in names:
                shutil.copyfile(directory / name, target / name)
        for name, path in inputs.items():
            shutil.copyfile(path, package / name)
        manifest = ("GFBORG 1\nid=fixture:gred\nname=Practice G-Red\nmodel=body.hsd\nanimations=motions.bank\n"
                    "rules=practice.rules\ncatalog=catalog.txt\n")
        (package / "borg.manifest").write_text(manifest, encoding="utf-8")
        (package / "catalog.txt").write_text(
            "GFBORG-CATALOG 1\ndescription=Fixture.\ntribe=none\ndefault-variant=normal\n"
            "variant=normal|Normal|0|unresolved\nvariant=shadow|Shadow|0|unresolved\n", encoding="utf-8")
        (package / "practice.rules").write_text(
            "GFBORG-PRACTICE 1\ncontroller=normal-gred-gfmm9\nvariant=normal\n"
            "rig-prefix=rig.prefix\npose-profile=rig.pose\nlocomotion=locomotion.gfmp\n"
            "pose-height=height.gfhc\nmovement-input=input.gfmi\nmovement-motion=motion.gfmm\n", encoding="utf-8")
        return package, manifest, inputs

    def test_packaged_gred_practice_runs_without_any_loose_gred_actor_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package, manifest, inputs = self.normal_practice_fixture(root)
            base = ["--practice-borg", "fixture:gred", "--headless", "--no-audio"]
            exact = [*base, "--borg-variant", "normal"]
            for flags in (
                ["--gred-movement", *base], exact,
                ["--gred-movement", *exact, "--source-ground-assets", str(root / "local/content/source-gred-ground-v9")],
                ["--gred-movement", *exact, "--list-borgs"],
            ):
                result = self.run_host(flags, cwd=root)
                self.assertEqual(result.returncode, 2, result.stdout)
                self.assertNotIn("backend=", result.stdout)
            result = self.run_host(["--demo-gred-walk-ledge-expiry-push", *exact, "--frames", "180"], cwd=root)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("bound fixture:gred variant=normal; qualified normal G-Red movement only", result.stdout)
            self.assertIn("walk-ledge-fall-pushes=2 last-angle-s16=13653 landings=1 resource=10000 selector=0", result.stdout)
            self.assertIn("practice-phase=IDLE", result.stdout)
            package = package.rename(package.with_name("renamed-folder"))
            (package / "borg.manifest").write_text(manifest.replace("Practice G-Red", "Renamed G-Red"), encoding="utf-8")
            result = self.run_host(["--gred-movement", *exact, "--frames", "10"], cwd=root)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("bound fixture:gred variant=normal", result.stdout)
            wrong_variant = self.run_host(["--gred-movement", *base, "--borg-variant", "shadow"], cwd=root)
            self.assertEqual(wrong_variant.returncode, 2, wrong_variant.stdout)
            self.assertIn("explicitly selected normal variant", wrong_variant.stdout)
            self.assertNotIn("backend=", wrong_variant.stdout)
            pose = package / "rig.pose"
            original = pose.read_bytes()
            pose.write_bytes(bytes([original[0] ^ 1]) + original[1:])
            modified = self.run_host(["--gred-movement", *exact], cwd=root)
            self.assertEqual(modified.returncode, 2, modified.stdout)
            self.assertIn("do not match the qualified normal G-Red content", modified.stdout)
            self.assertNotIn("backend=", modified.stdout)
            pose.unlink()
            pose.symlink_to(inputs["rig.pose"])
            linked = self.run_host(["--gred-movement", *exact], cwd=root)
            self.assertEqual(linked.returncode, 2, linked.stdout)
            self.assertIn("unavailable or wrong-sized practice asset: pose-profile", linked.stdout)
            self.assertNotIn("backend=", linked.stdout)

    def test_owned_package_practice_uses_exact_scoped_saved_copy_without_writes(self):
        seed = Path(os.environ["GF_SOURCE_WORLD_BINARY"]).resolve().with_name("gred_practice_replay")
        if not seed.is_file():
            self.skipTest("practice replay fixture seeder not built")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package, manifest, _ = self.normal_practice_fixture(root)
            accounts = root / "new-account-store"
            subprocess.run([seed, "--seed-owned-practice", accounts], check=True, timeout=30, cwd=root)
            before = {path.name: path.read_bytes() for path in accounts.iterdir()}
            package = package.rename(package.with_name("moved-package"))
            (package / "borg.manifest").write_text(manifest.replace("Practice G-Red", "Current Package Name"), encoding="utf-8")

            def start(save, instance, *extra, mode="--gred-movement"):
                return self.run_host([mode, "--practice-owned-borg", str(accounts), "7", str(save), str(instance),
                                      "--headless", "--no-audio", *extra], cwd=root)

            good = start(11, 8, "--frames", "180", mode="--demo-gred-walk-ledge-expiry-push")
            self.assertEqual(good.returncode, 0, good.stdout)
            self.assertIn("bound fixture:gred variant=normal", good.stdout)
            self.assertIn("account=7 save=11 owned instance=8 (Saved G-Red / Saved Standard); save is read-only", good.stdout)
            self.assertIn("walk-ledge-fall-pushes=2 last-angle-s16=13653 landings=1 resource=10000 selector=0", good.stdout)
            for save, instance, message in (
                (11, 9, "owned copy is disabled"), (11, 10, "no exact saved variant"),
                (11, 11, "explicitly selected normal variant"), (11, 12, "uniquely discovered package"),
                (11, 99, "not owned in this save"), (12, 8, "uniquely discovered package"),
                (13, 8, "no package roster"), (999, 8, "requested save unavailable"),
            ):
                with self.subTest(save=save, instance=instance):
                    refused = start(save, instance)
                    self.assertEqual(refused.returncode, 2, refused.stdout)
                    self.assertIn(message, refused.stdout)
                    self.assertNotIn("backend=", refused.stdout)
            for flags in (["--borg-variant", "normal"], ["--practice-borg", "fixture:gred", "--borg-variant", "normal"],
                          ["--browse-owned-borgs", str(accounts), "7", "11"]):
                refused = start(11, 8, *flags)
                self.assertEqual(refused.returncode, 2, refused.stdout)
                self.assertNotIn("backend=", refused.stdout)
            duel = start(11, 8, mode="--demo-battle-mvp")
            self.assertEqual(duel.returncode, 2, duel.stdout)
            self.assertNotIn("backend=", duel.stdout)
            for flags in (["--browse-borgs"], ["--place-owned-borg", str(accounts), "7", "11", "1", "1"]):
                result = self.run_host([*flags, "--practice-from-picker", "--headless"], cwd=root)
                self.assertEqual(result.returncode, 2, result.stdout)
                self.assertNotIn("backend=", result.stdout)
            replay = subprocess.run([seed, "--replay-owned-practice-picker", accounts], capture_output=True,
                                    text=True, timeout=90, cwd=root)
            self.assertEqual(replay.returncode, 0, replay.stdout + replay.stderr)
            self.assertIn("exact launch/return, live routes, disabled refusal, and held-key isolation PASS", replay.stdout)
            self.assertEqual(before, {path.name: path.read_bytes() for path in accounts.iterdir()})

    def test_inclined_dash_ledge_demo_departs_and_lands_without_spend(self):
        result = self.run_host(["--demo-gred-inclined-dash-ledge", "--headless", "--frames", "100", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("one right flick from triangle827 over the cliff toward flat floor2060", result.stdout)
        self.assertIn("ground-departures=1 loss-entries=1 loss-landings=1", result.stdout)
        self.assertRegex(result.stdout, r"G-Red-position=[^/]+/10\.00/")
        self.assertIn("practice-meter=10000/10000 at accepted tick=100", result.stdout)
        self.assertIn("practice-phase=IDLE", result.stdout)

    def test_inclined_walk_ledge_demo_does_not_dispatch_a_dash(self):
        result = self.run_host(["--demo-gred-inclined-walk-ledge", "--headless", "--frames", "100", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("D-pad up to depart triangle827 toward flat floor2060, then neutral", result.stdout)
        self.assertIn("walk-departures=1 dash-departures=0", result.stdout)
        self.assertIn("ground-departures=1 loss-entries=1 loss-landings=1", result.stdout)
        self.assertIn("root-updates=0", result.stdout)
        self.assertRegex(result.stdout, r"G-Red-position=[^/]+/10\.00/")
        self.assertIn("practice-meter=10000/10000 at accepted tick=100", result.stdout)
        self.assertIn("practice-phase=IDLE", result.stdout)

    def test_practice_ledge_cue_depends_on_accepted_frame(self):
        for tick, phase, cue in (
            (7, "WALKING", None),
            (8, "LEDGE DEPARTURE", None),
            (9, "LEDGE FALL ENTRY", "30 cue=DRIFT LOCKED 30 TICKS"),
            (10, "LEDGE FALL", "29 cue=DRIFT LOCKED 29 TICKS"),
            (23, "TOUCHDOWN", "16 cue=LEDGE TIMER 16 TICKS"),
            (24, "LANDING", "15 cue=LEDGE TIMER 15 TICKS"),
            (100, "IDLE", None),
        ):
            with self.subTest(tick=tick):
                result = self.run_host(["--demo-gred-inclined-walk-ledge", "--headless", "--frames", str(tick), "--no-audio"])
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertIn(f"practice-phase={phase}\n", result.stdout)
                self.assertIn(f"practice-meter=10000/10000 at accepted tick={tick}", result.stdout)
                if cue:
                    self.assertIn(f"practice-ledge-timer-ticks={cue}\n", result.stdout)
                else:
                    self.assertNotIn("practice-ledge-timer-ticks=", result.stdout)

    def test_practice_start_uses_grounded_demo_placement_without_scripted_input(self):
        for mode, expected_x, expected_z in (
            ("--gred-walk-ledge-expiry-push", "-1250.97", "-3313.80"),
            ("--gred-inclined-walk-ledge", "2599.01", "-2758.12"),
            ("--gred-movement", "-2760.08", "-3528.05"),
        ):
            with self.subTest(mode=mode):
                result = self.run_host([mode, "--practice-start", "--headless", "--frames", "80", "--no-audio"])
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertIn("practice start: demo placement, live input, no scripted replay", result.stdout)
                self.assertRegex(result.stdout, rf"accepted native tick=80 G-Red-position={expected_x}/[^/]+/{expected_z};")
                self.assertIn("practice-phase=IDLE", result.stdout)
                self.assertIn("practice-meter=10000/10000 at accepted tick=80", result.stdout)
                self.assertNotIn("ground-departures=1", result.stdout)
        ordinary = self.run_host(["--gred-walk-ledge-expiry-push", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(ordinary.returncode, 0, ordinary.stdout)
        self.assertIn("G-Red-position=-240.00/10.00/300.00", ordinary.stdout)
        self.assertNotIn("practice start: demo placement", ordinary.stdout)

    def test_practice_start_refuses_incompatible_and_repeated_options(self):
        for prefix in (
            [], ["--demo-battle-mvp"], ["--demo-gred-movement"], ["--source-walk-ledge"],
            ["--demo-gred-walk-ledge-expiry-push"], ["--gred-movement", "--practice-start"],
        ):
            with self.subTest(prefix=prefix):
                result = self.run_host([*prefix, "--practice-start", "--headless", "--frames", "1"])
                self.assertEqual(result.returncode, 2, result.stdout)
                self.assertNotIn("backend=", result.stdout)

    def test_walk_ledge_fall_push_demo_accepts_projection_then_separate_landing(self):
        for tick, phase, pushes, landings, cue in (
            (24, "LEDGE FALL / PUSH", 1, 0, "15 cue=DRIFT LOCKED 15 TICKS"),
            (77, "TOUCHDOWN", 2, 1, "0 cue=LEDGE TIMER 0 TICKS"),
            (78, "LANDING", 2, 1, "0 cue=LEDGE TIMER 0 TICKS"),
            (120, "IDLE", 2, 1, None),
        ):
            with self.subTest(tick=tick):
                result = self.run_host(["--demo-gred-walk-ledge-fall-push", "--headless", "--frames", str(tick), "--no-audio"])
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertIn(f"practice-phase={phase}\n", result.stdout)
                self.assertIn(f"walk-ledge-fall-pushes={pushes} last-angle-s16=8191 landings={landings} resource=10000", result.stdout)
                self.assertIn(f"practice-meter=10000/10000 at accepted tick={tick}", result.stdout)
                self.assertIn("root-updates=0", result.stdout)
                if cue:
                    self.assertIn(f"practice-ledge-timer-ticks={cue}\n", result.stdout)
                else:
                    self.assertNotIn("practice-ledge-timer-ticks=", result.stdout)

    def test_walk_ledge_high_angle_push_adds_two_pushes_without_changing_timer_rules(self):
        for tick, phase, pushes, landings in (
            (25, "LEDGE FALL / PUSH", 3, 0),
            (26, "LEDGE FALL / PUSH", 4, 0),
            (27, "LEDGE FALL / PUSH", 5, 0),
            (80, "TOUCHDOWN", 5, 1),
            (81, "LANDING", 5, 1),
            (120, "IDLE", 5, 1),
        ):
            with self.subTest(tick=tick):
                result = self.run_host(["--demo-gred-walk-ledge-high-angle-push", "--headless", "--frames", str(tick), "--no-audio"])
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertIn("native cap 13653, same selected-plane and pre/post-travel checks", result.stdout)
                self.assertIn(f"practice-phase={phase}\n", result.stdout)
                angle = 8191 if tick == 25 else 13653
                self.assertIn(f"walk-ledge-fall-pushes={pushes} last-angle-s16={angle} landings={landings} resource=10000", result.stdout)
                self.assertIn(f"practice-meter=10000/10000 at accepted tick={tick}", result.stdout)
                if tick <= 27:
                    self.assertIn(f"practice-ledge-timer-ticks={39 - tick} cue=DRIFT LOCKED {39 - tick} TICKS", result.stdout)

    def test_walk_ledge_expiry_push_only_bridges_the_timer_boundary(self):
        for tick, phase, pushes, angle, landings, cue in (
            (55, "LEDGE FALL / PUSH", 1, 8192, 0, "1 cue=DRIFT LOCKED 1 TICKS"),
            (56, "LEDGE FALL / PUSH", 2, 13653, 0, "0 cue=LEDGE TIMER 0 TICKS"),
            (57, "LEDGE FALL", 2, 13653, 0, "0 cue=DRIFT UNLOCKED"),
            (103, "TOUCHDOWN", 2, 13653, 1, "0 cue=LEDGE TIMER 0 TICKS"),
            (104, "LANDING", 2, 13653, 1, "0 cue=LEDGE TIMER 0 TICKS"),
            (180, "IDLE", 2, 13653, 1, None),
        ):
            with self.subTest(tick=tick):
                result = self.run_host(["--demo-gred-walk-ledge-expiry-push", "--headless", "--frames", str(tick), "--no-audio"])
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertIn("timer698 1 to 0 only after an accepted push, without drift", result.stdout)
                self.assertIn(f"practice-phase={phase}\n", result.stdout)
                self.assertIn(f"walk-ledge-fall-pushes={pushes} last-angle-s16={angle} landings={landings} resource=10000", result.stdout)
                self.assertIn(f"practice-meter=10000/10000 at accepted tick={tick}", result.stdout)
                if cue:
                    self.assertIn(f"practice-ledge-timer-ticks={cue}\n", result.stdout)
                else:
                    self.assertNotIn("practice-ledge-timer-ticks=", result.stdout)

    def test_slope_transition_dash_demo_crosses_and_recovers(self):
        result = self.run_host(["--demo-gred-slope-transition-dash", "--headless", "--frames", "100", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("slope transition demo: one backward flick from flat triangle249 onto incline1052", result.stdout)
        self.assertIn("actor=0 ground-y=482.7", result.stdout)
        self.assertIn("accepted native tick=100", result.stdout)
        self.assertRegex(result.stdout, r"G-Red-position=[^/]+/479\.49/")
        self.assertIn("practice-meter=10000/10000 at accepted tick=100", result.stdout)
        self.assertIn("practice-phase=IDLE", result.stdout)
        self.assertIn("resets=1", result.stdout)

    def test_slope_crease_dash_demo_crosses_and_recovers(self):
        result = self.run_host(["--demo-gred-slope-crease-dash", "--headless", "--frames", "100", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("slope crease demo: one right flick across original triangles295/508", result.stdout)
        self.assertIn("actor=0 ground-y=186.2", result.stdout)
        self.assertIn("accepted native tick=100", result.stdout)
        self.assertRegex(result.stdout, r"G-Red-position=[^/]+/150\.91/")
        self.assertIn("practice-phase=IDLE", result.stdout)
        self.assertIn("resets=1", result.stdout)

    def test_slope_seam_dash_demo_crosses_and_recovers(self):
        result = self.run_host(["--demo-gred-slope-seam-dash", "--headless", "--frames", "100", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("slope seam demo: one backward flick from original triangle254", result.stdout)
        self.assertIn("actor=0 ground-y=480.6", result.stdout)
        self.assertIn("accepted native tick=100", result.stdout)
        self.assertRegex(result.stdout, r"G-Red-position=[^/]+/479\.96/")
        self.assertIn("practice-phase=IDLE", result.stdout)
        self.assertIn("ordered-entries=1 active=6 recovery=19 resets=1", result.stdout)

    def test_slope_dash_demo_descends_and_recovers_on_original_plane(self):
        result = self.run_host(["--demo-gred-slope-dash", "--headless", "--frames", "100", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("slope demo: one backward flick down original triangle1014", result.stdout)
        self.assertIn("actor=0 ground-y=88.1", result.stdout)
        self.assertIn("accepted native tick=100", result.stdout)
        self.assertRegex(result.stdout, r"G-Red-position=[^/]+/50\.98/")
        self.assertIn("practice-phase=IDLE", result.stdout)
        self.assertIn("ordered-entries=1 active=6 recovery=19 resets=1", result.stdout)

    def test_practice_launch_uses_interactive_spawn_and_accepted_meter(self):
        result = self.run_host(["--gred-movement", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("G-Red movement practice: arrows walk/steer, WASD flick dash", result.stdout)
        self.assertIn("practice-meter=10000/10000 at accepted tick=10", result.stdout)
        self.assertIn("practice-phase=IDLE", result.stdout)
        self.assertIn("G-Red-position=-240.00/10.00/300.00", result.stdout)
        self.assertNotIn("demo only:", result.stdout)

    def test_practice_demo_meter_tracks_existing_accepted_windup_route(self):
        phases = ((81, 8450, "DASH WIND-UP / PUSH", None),
                  (86, 8000, "DASH WIND-UP / PUSH", None),
                  (87, 7950, "AIR DASH", 30),
                  (116, 6500, "AIR DASH", 1),
                  (117, 6450, "AIR DASH", 0),
                  (118, 6450, "DASH RECOVERY", None),
                  (140, 6450, "TOUCHDOWN", None),
                  (141, 6450, "LANDING", None),
                  (600, 10000, "IDLE", None))
        for tick, value, phase, release_ticks in phases:
            result = self.run_host(["--demo-gred-movement", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"practice-meter={value}/10000 at accepted tick={tick}", result.stdout)
            self.assertIn(f"practice-phase={phase}\n", result.stdout)
            if release_ticks is None:
                self.assertNotIn("practice-dash-release-ticks=", result.stdout)
            else:
                self.assertIn(f"practice-dash-release-ticks={release_ticks}\n", result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn("controller-windup-pushes=", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)

    def test_combined_windup_entry_push_defers_cost_and_keeps_active_entry_separate(self):
        for tick, pushes, entries, landings, meter, selector, program, cached in (
                (80, 0, 0, 0, 8450, 6, 6, 1),
                (81, 1, 1, 0, 8450, 15, 10, 1),
                (82, 2, 1, 0, 8450, 15, 10, 1),
                (86, 6, 1, 0, 8000, 16, 10, 1),
                (87, 6, 1, 0, 7950, 16, 11, 1),
                (140, 6, 1, 1, 6450, 7, 12, 1),
                (141, 6, 1, 1, 6450, 7, 8, 0),
                (600, 6, 1, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-windup-entry-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-windup-pushes={pushes} entry-pushes={entries} starts={entries} "
                          f"landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no rising entry, active entry, ground-loss falling or positive698 push authority",
                          result.stdout)

    def test_interactive_windup_entry_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-windup-entry-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-windup-pushes=0 entry-pushes=0 starts=0", result.stdout)
        self.assertNotIn("ground-windup-entry-push demo only:", result.stdout)

    def test_combined_relaunch_entry_push_charges_once_and_requires_separate_landing(self):
        for tick, pushes, landings, meter, selector, program, cached in (
                (78, 0, 0, 8550, 6, 6, 1),
                (79, 1, 0, 8400, 8, 14, 1),
                (80, 1, 1, 8400, 7, 21, 1),
                (81, 1, 1, 8400, 7, 8, 0),
                (240, 1, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-relaunch-entry-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-relaunch-pushes={pushes} entry-pushes={pushes} relaunches={pushes} "
                          f"landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no rising entry, dash entry, ground-loss or positive698 push authority",
                          result.stdout)

    def test_interactive_relaunch_entry_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-relaunch-entry-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-relaunch-pushes=0 entry-pushes=0 relaunches=0", result.stdout)
        self.assertNotIn("ground-relaunch-entry-push demo only:", result.stdout)

    def test_combined_relaunch_push_keeps_entry_release_and_landing_separate(self):
        for tick, pushes, landings, meter, selector, program, cached in (
                (82, 0, 0, 8200, 8, 14, 1),
                (83, 1, 0, 8100, 8, 14, 1),
                (84, 1, 1, 8100, 7, 21, 1),
                (85, 1, 1, 8100, 7, 8, 0),
                (240, 1, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-relaunch-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-relaunch-pushes={pushes} entry-pushes=0 relaunches=1 "
                          f"landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no relaunch entry, dash entry, ground-loss or positive698 push authority",
                          result.stdout)

    def test_interactive_relaunch_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-relaunch-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-relaunch-pushes=0 entry-pushes=0 relaunches=0", result.stdout)
        self.assertNotIn("ground-relaunch-push demo only:", result.stdout)

    def test_combined_active_push_keeps_cost_recovery_and_landing_separate(self):
        for tick, active, recovery, landings, meter, selector, program, cached in (
                (156, 0, 0, 0, 1900, 16, 11, 1),
                (157, 1, 0, 0, 1850, 16, 11, 1),
                (158, 1, 1, 0, 1850, 17, 12, 1),
                (159, 1, 1, 1, 1850, 7, 12, 1),
                (160, 1, 1, 1, 1850, 7, 8, 0),
                (360, 1, 1, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-active-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-active-pushes={active} recovery-pushes={recovery} starts=1 "
                          f"landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no active entry, wind-up entry, active relaunch, ground-loss or positive698 push authority",
                          result.stdout)

    def test_interactive_active_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-active-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-active-pushes=0 recovery-pushes=0 starts=0", result.stdout)
        self.assertNotIn("ground-active-push demo only:", result.stdout)

    def test_combined_windup_push_keeps_timer_handoff_separate(self):
        for tick, pushes, landings, meter, selector, program, cached in (
                (85, 0, 0, 8350, 15, 10, 1),
                (86, 1, 0, 8350, 15, 10, 1),
                (87, 2, 0, 7900, 16, 10, 1),
                (88, 2, 0, 7850, 16, 11, 1),
                (360, 2, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-windup-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-windup-pushes={pushes} entry-pushes=0 starts=1 "
                          f"landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no wind-up entry, active dash, active relaunch, ground-loss or positive698 push authority",
                          result.stdout)

    def test_interactive_windup_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-windup-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-windup-pushes=0 entry-pushes=0 starts=0", result.stdout)
        self.assertNotIn("ground-windup-push demo only:", result.stdout)

    def test_combined_recovery_push_stays_in_program_12_until_landing(self):
        for tick, pushes, angle, landings, meter, selector, program, cached in (
                (72, 0, 0, 0, 7650, 17, 12, 1),
                (73, 1, 11582, 0, 7650, 17, 12, 1),
                (74, 1, 11582, 1, 7650, 7, 12, 1),
                (75, 1, 11582, 1, 7650, 7, 8, 0),
                (240, 1, 11582, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-recovery-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-recovery-pushes={pushes} total-pushes={pushes} starts=1 "
                          f"angle-s16={angle} timer698=0.0 landings={landings} resource={meter} "
                          f"selector={selector} programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no active/wind-up dash, active relaunch, ground-loss or positive698 push authority",
                          result.stdout)

    def test_interactive_recovery_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-recovery-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-recovery-pushes=0 total-pushes=0 starts=0", result.stdout)
        self.assertNotIn("ground-recovery-push demo only:", result.stdout)

    def test_combined_relaunch_release_push_keeps_program_and_landing_separate(self):
        for tick, pushes, angle, landings, meter, selector, program, cached in (
                (61, 0, 0, 0, 6800, 6, 21, 1),
                (123, 1, 11582, 0, 6800, 6, 21, 1),
                (124, 1, 11582, 1, 6800, 7, 21, 1),
                (125, 1, 11582, 1, 6800, 7, 8, 0),
                (600, 1, 11582, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-relaunch-fall-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-relaunch-fall-pushes={pushes} relaunches=1 angle-s16={angle} "
                          f"timer698=0.0 landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("no active relaunch, air-dash callback, ground-loss or positive698 push authority",
                          result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)

    def test_interactive_relaunch_release_push_has_no_demo_input(self):
        result = self.run_host(["--source-ground-relaunch-fall-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-relaunch-fall-pushes=0 relaunches=0", result.stdout)
        self.assertNotIn("ground-relaunch-fall-push demo only:", result.stdout)

    def test_combined_post_air_fall_push_waits_for_completed_recovery(self):
        for tick, pushes, angle, landings, meter, selector, program, cached in (
                (111, 0, 0, 0, 5150, 6, 12, 1),
                (112, 1, 11582, 0, 5150, 6, 13, 1),
                (113, 1, 11582, 1, 5150, 7, 13, 1),
                (114, 1, 11582, 1, 5150, 7, 8, 0),
                (136, 1, 11582, 1, 10000, 0, 0, 0),
                (600, 1, 11582, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-air-fall-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-air-fall-pushes={pushes} total-pushes={pushes} starts=1 "
                          f"angle-s16={angle} timer698=0.0 landings={landings} resource={meter} "
                          f"selector={selector} programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("one combined controller with post-air zero698 falling push", result.stdout)
            self.assertIn("completed recovery receipt before zero698/downward falling6/program13",
                          result.stdout)
            self.assertIn("unchanged target/stage", result.stdout)
            self.assertIn("no ground-loss/relaunch/active-callback/positive698 push or combat authority",
                          result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 113:
                self.assertIn("G-Red-position=-603.24/20.67/-1356.33", result.stdout)

    def test_interactive_post_air_push_has_no_demo_input_or_placement(self):
        result = self.run_host(["--source-ground-air-fall-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-air-fall-pushes=0 total-pushes=0 starts=0 angle-s16=0 "
                      "timer698=0.0 landings=0 resource=10000 selector=0 programs=0/0 cached87=0",
                      result.stdout)
        self.assertNotIn("ground-air-fall-push demo only:", result.stdout)

    def test_combined_post_air_push_refuses_older_pack_without_fallback(self):
        result = self.run_host(["--source-ground-air-fall-push", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_combined_ordinary_fall_push_preserves_separate_landing(self):
        for tick, pushes, angle, landings, meter, selector, program, cached in (
                (81, 0, 0, 0, 8350, 6, 6, 1),
                (82, 1, 11582, 0, 8350, 6, 6, 1),
                (83, 1, 11582, 1, 8350, 7, 6, 1),
                (84, 1, 11582, 1, 8350, 7, 8, 0),
                (106, 1, 11582, 1, 10000, 0, 0, 0),
                (600, 1, 11582, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-fall-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-fall-pushes={pushes} angle-s16={angle} timer698=0.0 "
                          f"landings={landings} resource={meter} selector={selector} "
                          f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("ordinary zero698 falling push", result.stdout)
            self.assertIn("ordinary native bounds/target and unchanged stage", result.stdout)
            self.assertIn("exact pre-response numeric/selected-plane projection", result.stdout)
            self.assertIn("no ground-loss/post-air/relaunch/dash/positive698 push or combat authority",
                          result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 83:
                self.assertIn("G-Red-position=-602.99/10.00/-1376.75", result.stdout)

    def test_interactive_ordinary_push_has_no_scripted_placement_or_input(self):
        result = self.run_host(["--source-ground-fall-push", "--headless",
                                "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=10", result.stdout)
        self.assertIn("controller-fall-pushes=0 angle-s16=0 timer698=0.0 "
                      "landings=0 resource=10000 selector=0 programs=0/0 cached87=0", result.stdout)
        self.assertNotIn("ground-fall-push demo only:", result.stdout)

    def test_combined_ordinary_push_refuses_older_operand_pack_without_fallback(self):
        result = self.run_host(["--source-ground-fall-push", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_scripted_ground_world_and_shared_renderer(self):
        result = self.run_host(["--demo-source-ground-dash", "--headless", "--frames", "720", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=720", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        self.assertIn("native body/terrain proxies", result.stdout)

    def test_missing_pack_cannot_fall_back_to_mvp(self):
        result = self.run_host(["--demo-source-ground-dash", "--source-ground-assets",
                                "/tmp/opencode/absent-source-ground-pack", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("bind failed", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_gfmm8_data_remains_usable_without_enabling_new_actor_callbacks(self):
        pack = Path("local/content/source-gred-ground-v8")
        if not (pack/"motion.gfmm").is_file():
            self.skipTest("optional qualified GFMM8 pack absent")
        result = self.run_host(["--demo-source-windup-entry-push", "--source-ground-assets",
                                str(pack), "--headless", "--frames", "81", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=81", result.stdout)
        self.assertIn("windup-entry-surface-pushes=1", result.stdout)
        self.assertIn("landings=0 resource=8450 selector=15", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)

    def test_gfmm9_does_not_silently_replace_legacy_dash_or_walk_ledge_policy(self):
        pack = Path("local/content/source-gred-ground-v9")
        if not (pack/"motion.gfmm").is_file():
            self.skipTest("optional qualified GFMM9 pack absent")
        for mode, frames in (("--demo-source-ground-dash", "720"), ("--demo-source-walk-ledge", "118")):
            args = [mode, "--headless", "--frames", frames, "--no-audio"]
            old = self.run_host(args)
            new = self.run_host([*args, "--source-ground-assets", str(pack)])
            self.assertEqual(old.returncode, 0, old.stdout)
            self.assertEqual(new.returncode, 0, new.stdout)
            self.assertEqual(old.stdout, new.stdout)
            self.assertIn(f"accepted native tick={frames}", new.stdout)

    def test_owned_ordered_ground_tracks_actual_root_speed_and_done_read(self):
        for tick, active, recovery, resets, updates, speed, selector in (
                (6, 0, 0, 0, 1, "0.000000", 13), (12, 6, 0, 0, 7, "12.500000", 13),
                (18, 12, 0, 0, 7, "12.500000", 14), (19, 12, 1, 0, 8, "12.500000", 14),
                (24, 12, 6, 0, 13, "12.500000", 14), (29, 12, 11, 0, 18, "0.000000", 14),
                (37, 12, 19, 1, 25, "0.000000", 0), (600, 12, 19, 1, 25, "0.000000", 0)):
            result = self.run_host(["--demo-source-ordered-ground", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"ordered-entries=1 active={active} recovery={recovery} resets={resets}", result.stdout)
            self.assertIn(f"root-updates={updates} root-speed={speed} selector={selector}", result.stdout)
            self.assertIn("sealed post-VM AObj root-speed reads", result.stdout)
            self.assertIn("no ledge/jump/flight/combat/clipping", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 29:
                self.assertIn("G-Red-position=-568.50/10.00/592.83", result.stdout)
        result = self.run_host(["--source-ordered-ground", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("ordered-entries=0 active=0 recovery=0 resets=0 root-updates=0", result.stdout)
        result = self.run_host(["--source-ordered-ground", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_dash_ledge_keeps_post_motion_momentum_and_later_priority_fall(self):
        for tick, entries, landings, timer, cached, flags in (
                (6, 0, 0, "0.0", 0, "20"), (7, 1, 0, "30.0", 1, "40"),
                (36, 1, 0, "1.0", 1, "40"), (37, 1, 0, "0.0", 1, "40"),
                (38, 1, 1, "0.0", 1, "40"), (39, 1, 1, "0.0", 0, "200"),
                (600, 1, 1, "0.0", 0, "0")):
            result = self.run_host(["--demo-source-dash-ledge", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"ground-departures=1 loss-entries={entries} loss-landings={landings}", result.stdout)
            self.assertIn(f"timer698={timer} cached87={cached} flags={flags}", result.stdout)
            self.assertIn("ordered-entries=1 active=0 recovery=0 resets=0 root-updates=1", result.stdout)
            self.assertIn("original roof/stage and unchanged target", result.stdout)
            self.assertIn("resource=10000", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 36:
                self.assertIn("suppressed-drift=30", result.stdout)
            if tick >= 38:
                self.assertIn("G-Red-position=938.73/10.00/711.72", result.stdout)
        result = self.run_host(["--source-dash-ledge", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("ground-departures=0 loss-entries=0 loss-landings=0", result.stdout)
        self.assertNotIn("dash-ledge demo only:", result.stdout)
        result = self.run_host(["--source-dash-ledge", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_one_ground_ledge_controller_walks_stops_flicks_and_dashes(self):
        for tick, dashes, active, roots, departures, entries, landings, timer, cached, flags in (
                (70, 0, 0, 0, 0, 0, 0, "0.0", 0, "10"),
                (72, 0, 0, 0, 0, 0, 0, "0.0", 0, "0"),
                (73, 1, 0, 1, 0, 0, 0, "0.0", 0, "20"),
                (80, 1, 7, 7, 1, 0, 0, "0.0", 0, "20"),
                (81, 1, 7, 7, 1, 1, 0, "30.0", 1, "40"),
                (111, 1, 7, 7, 1, 1, 0, "0.0", 1, "40"),
                (112, 1, 7, 7, 1, 1, 1, "0.0", 1, "40"),
                (113, 1, 7, 7, 1, 1, 1, "0.0", 0, "200"),
                (600, 1, 7, 7, 1, 1, 1, "0.0", 0, "0")):
            result = self.run_host(["--demo-source-ground-ledge", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"ordered-entries={dashes} active={active} recovery=0 resets=0 root-updates={roots}", result.stdout)
            self.assertIn(f"ground-departures={departures} loss-entries={entries} loss-landings={landings}", result.stdout)
            self.assertIn(f"timer698={timer} cached87={cached} flags={flags}", result.stdout)
            self.assertIn(f"walk-departures=0 dash-departures={departures}", result.stdout)
            self.assertIn("same world/producer, no teleport/rebind", result.stdout)
            self.assertIn("resource=10000", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-ledge", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("walk-departures=0 dash-departures=0", result.stdout)
        self.assertNotIn("ground-ledge demo only:", result.stdout)
        result = self.run_host(["--source-ground-ledge", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_jump_interrupts_owned_dash_before_motion_and_recovers(self):
        for tick, roots, selector, program, cached, meter, landings, air in (
                (28, 1, 13, 27, 0, 10000, 0, 0), (35, 7, 13, 27, 0, 10000, 0, 0),
                (36, 7, 4, 2, 0, 10000, 0, 0), (48, 7, 5, 4, 1, 9850, 0, 1),
                (72, 7, 6, 6, 1, 7550, 0, 25), (124, 7, 7, 6, 1, 7550, 1, 76),
                (125, 7, 7, 8, 0, 7550, 1, 76), (147, 7, 0, 0, 0, 10000, 1, 76),
                (600, 7, 0, 0, 0, 10000, 1, 76)):
            result = self.run_host(["--demo-source-ground-jump", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"root-updates={roots}", result.stdout)
            self.assertIn(f"selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn(f"ground-jump-air-frames={air} landings={landings} resource={meter}", result.stdout)
            self.assertIn(f"dash-jumps={int(tick>=36)} last-interrupted-selector={13 if tick>=36 else 0}", result.stdout)
            self.assertIn("one ground/ledge/jump controller", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 124:
                self.assertIn("G-Red-position=-852.77/35.04/806.94", result.stdout)
        result = self.run_host(["--source-ground-jump", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("ground-jump-air-frames=0 landings=0 resource=10000 dash-jumps=0", result.stdout)
        self.assertNotIn("ground-jump demo only:", result.stdout)
        result = self.run_host(["--source-ground-jump", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_landing_waits_for_accepted_entry_then_jumps_or_dashes(self):
        for tick, jumps, dashes, landings, meter, program, cached, roots in (
                (85, 0, 0, 1, 8150, 6, 1, 0), (86, 0, 0, 1, 8150, 8, 0, 0),
                (87, 1, 0, 1, 10000, 2, 0, 0), (99, 1, 0, 1, 9850, 4, 1, 0),
                (150, 1, 0, 2, 9150, 6, 1, 0), (151, 1, 0, 2, 9150, 8, 0, 0),
                (152, 1, 1, 2, 10000, 29, 0, 1), (600, 1, 1, 2, 10000, 0, 0, 25)):
            result = self.run_host(["--demo-source-ground-landing", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"landing-jumps={jumps} landing-dashes={dashes} landings={landings} resource={meter}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn(f"root-updates={roots}", result.stdout)
            self.assertIn("after accepted landing-entry update, never on touchdown", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-landing", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("landing-jumps=0 landing-dashes=0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-landing demo only:", result.stdout)
        result = self.run_host(["--source-ground-landing", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_loss_relaunch_keeps_actual_timer_floor_tail_and_depletion(self):
        for tick, launches, timer, floors, lower, upper, landings, meter, selector, program, cached in (
                (147, 1, 5, 0, 1, 1, 0, 9850, 8, 14, 1),
                (151, 1, 1, 1, 2, 2, 0, 9450, 8, 14, 1),
                (152, 1, 0, 2, 3, 3, 0, 9350, 8, 15, 1),
                (156, 1, 0, 6, 3, 3, 0, 8950, 8, 15, 1),
                (157, 1, 0, 6, 3, 3, 0, 8850, 8, 15, 1),
                (247, 1, 0, 6, 0, 0, 0, 0, 6, 21, 1),
                (333, 1, 0, 6, 0, 0, 1, 0, 7, 21, 1),
                (334, 1, 0, 6, 0, 0, 1, 0, 7, 8, 0),
                (600, 1, 0, 6, 0, 0, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-loss-relaunch", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-loss-relaunches={launches} timer698={timer:.1f} floors={floors} lower={lower} upper={upper} landings={landings} loss-landings=0 resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("declared original-stage grounded placement/query reference", result.stdout)
            self.assertIn("no midair teleport/rebind", result.stdout)
            self.assertIn("no push/nonflat-dash/combat authority", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-loss-relaunch", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-loss-relaunches=0 timer698=0.0 floors=0 lower=0 upper=0 landings=0 loss-landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-loss-relaunch demo only:", result.stdout)
        result = self.run_host(["--source-ground-loss-relaunch", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_loss_air_uses_actual_timer_without_reset_or_early_cost(self):
        for tick, starts, timer, landings, meter, selector, program, cached in (
                (121, 0, 30, 0, 10000, 6, 13, 1),
                (122, 0, 29, 0, 10000, 6, 13, 1),
                (123, 1, 28, 0, 10000, 15, 10, 1),
                (129, 1, 22, 0, 9550, 16, 10, 1),
                (130, 1, 21, 0, 9500, 16, 11, 1),
                (150, 1, 1, 0, 8500, 16, 11, 1),
                (151, 1, 0, 0, 8450, 16, 11, 1),
                (173, 1, 0, 0, 7400, 17, 12, 1),
                (185, 1, 0, 1, 7400, 7, 12, 1),
                (186, 1, 0, 1, 7400, 7, 8, 0),
                (600, 1, 0, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-loss-air", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-loss-air-starts={starts} timer698={timer:.1f} windup-floor=0 active-floor=0 landings={landings} loss-landings=0 resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("declared original-stage grounded placement/query reference", result.stdout)
            self.assertIn("no midair teleport/rebind", result.stdout)
            self.assertIn("no positive698-relaunch/push/nonflat-dash/combat authority", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-loss-air", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-loss-air-starts=0 timer698=0.0 windup-floor=0 active-floor=0 landings=0 loss-landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-loss-air demo only:", result.stdout)
        result = self.run_host(["--source-ground-loss-air", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_loss_landing_dash_keeps_timer_and_actual_root_receipts(self):
        for tick, dashes, timer, selector, program, cached, roots in (
                (148, 0, 4, 7, 13, 1, 0),
                (149, 0, 3, 7, 8, 0, 0),
                (150, 1, 2, 13, 27, 0, 1),
                (151, 1, 1, 13, 27, 0, 2),
                (152, 1, 0, 13, 27, 0, 3),
                (157, 1, 0, 14, 27, 0, 8),
                (176, 1, 0, 0, 0, 0, 25),
                (600, 1, 0, 0, 0, 0, 25)):
            result = self.run_host(["--demo-source-ground-loss-landing", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-loss-input-jumps=0 dashes={dashes} loss-landings=1 timer698={timer:.1f} landings=1 resource=10000 selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn(f"root-updates={roots}", result.stdout)
            self.assertIn("declared original-stage grounded placement/query reference", result.stdout)
            self.assertIn("native arena-half4500", result.stdout)
            self.assertIn("no midair teleport/rebind", result.stdout)
            self.assertIn("no positive698-flight/push/nonflat-dash/combat authority", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-loss-landing", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-loss-input-jumps=0 dashes=0 loss-landings=0 timer698=0.0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-loss-landing demo only:", result.stdout)
        result = self.run_host(["--source-ground-loss-landing", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_loss_jump_retains_timer_through_accepted_landing_takeoff(self):
        for tick, jumps, surfaces, timer, landings, meter, selector, program, cached in (
                (140, 0, 1, 11, 1, 10000, 7, 8, 0),
                (141, 1, 1, 10, 1, 10000, 4, 2, 0),
                (150, 1, 1, 1, 1, 10000, 4, 2, 0),
                (151, 1, 1, 0, 1, 10000, 4, 2, 0),
                (152, 1, 1, 0, 1, 10000, 5, 2, 0),
                (153, 1, 1, 0, 1, 9850, 5, 4, 1),
                (155, 1, 1, 0, 1, 9650, 5, 4, 1),
                (156, 1, 1, 0, 1, 9650, 6, 6, 1),
                (194, 1, 2, 0, 2, 9650, 7, 6, 1),
                (195, 1, 2, 0, 2, 9650, 7, 8, 0),
                (220, 1, 2, 0, 2, 10000, 0, 0, 0),
                (600, 1, 2, 0, 2, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-loss-jump", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-loss-jumps={jumps} loss-landings=1 surface-callbacks={surfaces} timer698={timer:.1f} landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("declared original-stage grounded placement/query reference and4500", result.stdout)
            self.assertIn("no midair teleport/rebind", result.stdout)
            self.assertIn("no positive698-dash-or-flight/push/combat authority", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-loss-jump", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-loss-jumps=0 loss-landings=0 surface-callbacks=0 timer698=0.0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-loss-jump demo only:", result.stdout)
        result = self.run_host(["--source-ground-loss-jump", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_loss_surface_binds_angle_to_actual_timer_and_separate_landing(self):
        for tick, touchdowns, surfaces, departures, entries, timer, angle, landings, selector, program, cached in (
                (120, 0, 0, 1, 0, 0, 0, 0, 2, 1, 0),
                (121, 0, 0, 1, 1, 30, 0, 0, 6, 13, 1),
                (138, 0, 0, 1, 1, 13, 0, 0, 6, 13, 1),
                (139, 1, 1, 1, 1, 12, 3911, 1, 7, 13, 1),
                (140, 1, 1, 1, 1, 11, 3911, 1, 7, 8, 0),
                (150, 1, 1, 1, 1, 1, 3911, 1, 7, 8, 0),
                (151, 1, 1, 1, 1, 0, 3911, 1, 7, 8, 0),
                (220, 1, 1, 1, 1, 0, 3911, 1, 0, 0, 0),
                (600, 1, 1, 1, 1, 0, 3911, 1, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-loss-surface", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-loss-landings={touchdowns} surface-callbacks={surfaces} departures={departures} entries={entries} timer698={timer:.1f} angle-s16={angle} landings={landings} resource=10000 selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("declared original-stage grounded placement/query reference", result.stdout)
            self.assertIn("native arena-half4500", result.stdout)
            self.assertIn("no midair teleport/rebind", result.stdout)
            self.assertIn("no push/positive698-air-or-landing-input/combat authority", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 139:
                self.assertRegex(result.stdout, r"G-Red-position=-?[\d.]+/16\.71/-?[\d.]+")
        result = self.run_host(["--source-ground-loss-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-loss-landings=0 surface-callbacks=0 departures=0 entries=0 timer698=0.0 angle-s16=0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-loss-surface demo only:", result.stdout)
        result = self.run_host(["--source-ground-loss-surface", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_surface_requires_paired_plane_for_actual_callback_landing(self):
        for tick, surfaces, angle, air, landings, meter, selector, program, cached in (
                (48, 0, 0, 0, 0, 9550, 6, 6, 1),
                (49, 0, 0, 1, 0, 9550, 15, 10, 1),
                (58, 0, 0, 1, 0, 9100, 16, 10, 1),
                (59, 0, 0, 1, 0, 9050, 16, 11, 1),
                (89, 0, 0, 1, 0, 7550, 16, 11, 1),
                (107, 0, 0, 1, 0, 6650, 16, 11, 1),
                (108, 1, 7187, 1, 1, 6600, 7, 11, 1),
                (109, 1, 7187, 1, 1, 6600, 7, 8, 0),
                (131, 1, 7187, 1, 1, 10000, 0, 0, 0),
                (600, 1, 7187, 1, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-surface", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-surface-landings={surfaces} angle-s16={angle} air={air} relaunches=0 pushes=0 landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("no push/positive698/combat authority", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            if tick >= 108:
                self.assertRegex(result.stdout, r"G-Red-position=-?[\d.]+/82\.00/-?[\d.]+")
        result = self.run_host(["--source-ground-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-surface-landings=0 angle-s16=0 air=0 relaunches=0 pushes=0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-surface demo only:", result.stdout)
        result = self.run_host(["--source-ground-surface", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_windup_entry_floor_has_no_motion_gravity_or_early_cost(self):
        for tick, entry, positive, windup, active, landings, meter, selector, program, cached in (
                (86, 0, 0, 0, 0, 0, 7700, 8, 15, 1),
                (87, 1, 1, 1, 0, 0, 7700, 15, 10, 1),
                (94, 1, 1, 8, 0, 0, 7250, 16, 10, 1),
                (95, 1, 1, 8, 1, 0, 7200, 16, 11, 1),
                (124, 1, 1, 8, 30, 0, 5750, 16, 11, 1),
                (125, 1, 1, 8, 30, 1, 5700, 7, 11, 1),
                (126, 1, 1, 8, 30, 1, 5700, 7, 8, 0),
                (600, 1, 1, 8, 30, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-windup-entry-floor", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-windup-entry={entry} positive={positive} windup={windup} active={active} relaunch-floor=8 relaunches=1 pushes=0 landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("no push/positive698/combat authority", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-windup-entry-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-windup-entry=0 positive=0 windup=0 active=0 relaunch-floor=0 relaunches=0 pushes=0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-windup-entry-floor demo only:", result.stdout)
        result = self.run_host(["--source-ground-windup-entry-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_relaunch_floor_preserves_null_callback_and_later_lift(self):
        for tick, snaps, launches, lower, upper, landings, meter, selector, program, cached in (
                (78, 0, 0, 0, 0, 0, 8550, 6, 6, 1),
                (79, 1, 1, 1, 1, 0, 8400, 8, 14, 1),
                (80, 2, 1, 1, 1, 0, 8300, 8, 14, 1),
                (83, 5, 1, 2, 2, 0, 8000, 8, 14, 1),
                (84, 6, 1, 3, 3, 0, 7900, 8, 15, 1),
                (86, 8, 1, 3, 3, 0, 7700, 8, 15, 1),
                (87, 8, 1, 3, 3, 0, 7600, 8, 15, 1),
                (164, 8, 1, 0, 0, 0, 0, 6, 21, 1),
                (242, 8, 1, 0, 0, 1, 0, 7, 21, 1),
                (243, 8, 1, 0, 0, 1, 0, 7, 8, 0),
                (600, 8, 1, 0, 0, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-relaunch-floor", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-relaunch-floor={snaps} relaunches={launches} lower={lower} upper={upper} pushes=0 entry=0 positive-entry=0 landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("no windup-entry-floor/push authority", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-relaunch-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-relaunch-floor=0 relaunches=0 lower=0 upper=0 pushes=0 entry=0 positive-entry=0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-relaunch-floor demo only:", result.stdout)
        result = self.run_host(["--source-ground-relaunch-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_relaunch_preserves_additive_lift_and_independent_latches(self):
        for tick, launches, lower, upper, landings, meter, selector, program, cached in (
                (60, 0, 0, 0, 0, 8550, 6, 6, 1),
                (61, 1, 1, 1, 0, 8400, 8, 14, 1),
                (65, 1, 2, 2, 0, 8000, 8, 14, 1),
                (66, 1, 3, 3, 0, 7900, 8, 15, 1),
                (100, 1, 3, 3, 0, 4500, 8, 15, 1),
                (101, 1, 0, 0, 0, 4500, 6, 21, 1),
                (171, 1, 0, 0, 1, 4500, 7, 21, 1),
                (172, 1, 0, 0, 1, 4500, 7, 8, 0),
                (600, 1, 0, 0, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-relaunch", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-relaunches={launches} lower={lower} upper={upper} air=0 floor=0 pushes=0 landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("no relaunch-floor/entry-floor/push authority", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-relaunch", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-relaunches=0 lower=0 upper=0 air=0 floor=0 pushes=0 landings=0 resource=10000", result.stdout)
        self.assertNotIn("ground-relaunch demo only:", result.stdout)
        result = self.run_host(["--source-ground-relaunch", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_air_floor_keeps_null_callback_and_later_landing_separate(self):
        for tick, starts, windup, active, landings, meter, selector, program, cached in (
                (61, 1, 0, 0, 0, 9550, 15, 10, 1),
                (64, 1, 1, 0, 0, 9550, 15, 10, 1),
                (66, 1, 3, 0, 0, 9100, 16, 10, 1),
                (67, 1, 3, 1, 0, 9050, 16, 11, 1),
                (96, 1, 3, 30, 0, 7600, 16, 11, 1),
                (97, 1, 3, 30, 1, 7550, 7, 11, 1),
                (98, 1, 3, 30, 1, 7550, 7, 8, 0),
                (600, 1, 3, 30, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-air-floor", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-floor-starts={starts} windup={windup} active={active} entry=0 positive-entry=0 landings={landings} relaunches=0 resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("no entry-floor/push/relaunch authority", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-air-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-floor-starts=0 windup=0 active=0 entry=0 positive-entry=0 landings=0 relaunches=0 resource=10000", result.stdout)
        self.assertNotIn("ground-air-floor demo only:", result.stdout)
        result = self.run_host(["--source-ground-air-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_ground_air_keeps_windup_active_recovery_and_resource_contracts(self):
        for tick, starts, active, landings, meter, selector, program, cached in (
                (46, 1, 0, 0, 7150, 15, 10, 1), (51, 1, 0, 0, 6700, 16, 10, 1),
                (52, 1, 1, 0, 6650, 16, 11, 1), (88, 1, 36, 0, 4900, 17, 12, 1),
                (91, 2, 36, 0, 4900, 15, 10, 1), (100, 2, 37, 0, 4400, 16, 11, 1),
                (133, 2, 69, 0, 2800, 17, 12, 1), (159, 2, 69, 1, 2800, 7, 12, 1),
                (160, 2, 69, 1, 2800, 7, 8, 0), (600, 2, 69, 1, 10000, 0, 0, 0)):
            result = self.run_host(["--demo-source-ground-air", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"controller-air-starts={starts} active={active} landings={landings} relaunches=0 resource={meter} selector={selector}", result.stdout)
            self.assertIn(f"programs={program}/{program} cached87={cached}", result.stdout)
            self.assertIn("root-updates=0", result.stdout)
            self.assertIn("no floor/push/relaunch authority", result.stdout)
            self.assertIn("unchanged native spawn/arena/stage/target", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-ground-air", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("controller-air-starts=0 active=0 landings=0 relaunches=0 resource=10000", result.stdout)
        self.assertNotIn("ground-air demo only:", result.stdout)
        result = self.run_host(["--source-ground-air", "--source-ground-assets",
                                "local/content/source-gred-ground-v8", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM9", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_general_ground_walk_stop_dash_and_interactive_idle(self):
        for mode in ("--demo-source-ground", "--source-ground"):
            result = self.run_host([mode, "--headless", "--frames", "360", "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("accepted native tick=360", result.stdout)
            self.assertIn("source walk/turn/stop/dash", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)

    def test_general_ground_requires_version_two_pack(self):
        result = self.run_host(["--source-ground", "--source-ground-assets",
                                "local/content/source-gred-ground-v1", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM2", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_source_vertical_release_and_depletion_land(self):
        result = self.run_host(["--demo-source-vertical-jump", "--headless", "--frames", "600", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=600", result.stdout)
        self.assertIn("vertical-air-frames=257 landings=2 resource=10000 selector=0", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        self.assertIn("native flat-floor/ceiling proxy, effects omitted", result.stdout)
        result = self.run_host(["--source-vertical-jump", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("vertical-air-frames=0 landings=0", result.stdout)

    def test_incompatible_modes_and_unused_pack_refuse(self):
        for flags in (["--demo-source-ground-dash", "--demo-locomotion"],
                      ["--source-ground", "--demo-source-ground-dash"],
                      ["--demo-source-ground", "--demo-battle-mvp"],
                      ["--source-vertical-jump", "--source-ground"],
                      ["--source-jump", "--source-vertical-jump"],
                      ["--source-air-dash", "--source-jump"],
                      ["--source-relaunch", "--source-air-dash"],
                      ["--source-dash-jump", "--source-relaunch"],
                      ["--source-landing", "--source-dash-jump"],
                      ["--source-surface", "--source-landing"],
                      ["--source-steep", "--source-surface"],
                      ["--source-fall-surface", "--source-steep"],
                      ["--source-recovery-surface", "--source-fall-surface"],
                      ["--source-windup-surface", "--source-recovery-surface"],
                      ["--source-active-surface", "--source-windup-surface"],
                      ["--source-relaunch-surface", "--source-active-surface"],
                       ["--source-relaunch-entry-surface", "--source-relaunch-surface"],
                       ["--source-relaunch-floor", "--source-relaunch-entry-surface"],
                       ["--source-air-dash-floor", "--source-relaunch-floor"],
                       ["--source-windup-entry-floor", "--source-air-dash-floor"],
                       ["--source-windup-positive-floor", "--source-windup-entry-floor"],
                       ["--source-windup-entry-push", "--source-windup-positive-floor"],
                       ["--source-walk-ledge", "--source-ground"],
                       ["--source-ordered-ground", "--source-ground"],
                       ["--source-ordered-ground", "--source-walk-ledge"],
                       ["--source-dash-ledge", "--source-ordered-ground"],
                       ["--source-dash-ledge", "--source-walk-ledge"],
                       ["--source-ground-ledge", "--source-dash-ledge"],
                       ["--source-ground-ledge", "--source-walk-ledge"],
                       ["--source-ground-jump", "--source-ground-ledge"],
                       ["--source-ground-jump", "--source-jump"],
                       ["--source-ground-landing", "--source-ground-jump"],
                       ["--source-ground-landing", "--source-landing"],
                       ["--source-ground-air", "--source-ground-landing"],
                       ["--source-ground-air", "--source-air-dash"],
                       ["--source-ground-air-floor", "--source-ground-air"],
                       ["--source-ground-air-floor", "--source-air-dash-floor"],
                       ["--source-ground-relaunch", "--source-ground-air-floor"],
                       ["--source-ground-relaunch", "--source-relaunch"],
                       ["--source-ground-relaunch-floor", "--source-ground-relaunch"],
                       ["--source-ground-relaunch-floor", "--source-relaunch-floor"],
                       ["--source-ground-windup-entry-floor", "--source-ground-relaunch-floor"],
                       ["--source-ground-windup-entry-floor", "--source-windup-positive-floor"],
                       ["--source-ground-surface", "--source-ground-windup-entry-floor"],
                       ["--source-ground-surface", "--source-surface"],
                       ["--source-ground-loss-surface", "--source-ground-surface"],
                       ["--source-ground-loss-surface", "--source-walk-ledge"],
                       ["--source-ground-loss-jump", "--source-ground-loss-surface"],
                       ["--source-ground-loss-jump", "--source-ground-landing"],
                       ["--source-ground-loss-landing", "--source-ground-loss-jump"],
                       ["--source-ground-loss-landing", "--source-ground-landing"],
                       ["--source-ground-loss-air", "--source-ground-loss-landing"],
                       ["--source-ground-loss-air", "--source-air-dash"],
                       ["--source-ground-loss-relaunch", "--source-ground-loss-air"],
                       ["--source-ground-loss-relaunch", "--source-relaunch-floor"],
                      ["--source-ground-assets", "unused"]):
            result = self.run_host([*flags, "--headless", "--no-audio"])
            self.assertEqual(result.returncode, 2, result.stdout)

    def test_source_moving_jump_steering_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-jump", "--headless", "--frames", "480", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=480", result.stdout)
        self.assertIn("landings=2 resource=10000 selector=0", result.stdout)
        self.assertIn("ascent body turn, falling camera-relative drift", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-jump", "--source-ground-assets", "local/content/source-gred-ground-v2", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM3", result.stdout)

    def test_null_callback_relaunch_floor_stays_airborne_until_separate_touchdown(self):
        for tick, contacts, landings, meter, selector in (
                (79, 1, 0, 8400, 8), (80, 2, 0, 8300, 8),
                (81, 2, 1, 8300, 7), (240, 2, 1, 10000, 0),
                (600, 2, 1, 10000, 0)):
            result = self.run_host(["--demo-source-relaunch-floor", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"null-callback-relaunch-floor-contacts={contacts} last-floor-programs=14/14", result.stdout)
            self.assertIn(f"landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            self.assertIn("ordinary native spawn/bounds/target/references", result.stdout)
        result = self.run_host(["--source-relaunch-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("null-callback-relaunch-floor-contacts=0", result.stdout)
        self.assertNotIn("relaunch-floor demo:", result.stdout)
        result = self.run_host(["--source-relaunch-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v6", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM7", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_air_dash_floor_caller_responses_and_later_real_callback(self):
        for tick, windup, active, landings, meter, selector in (
                (80, 1, 0, 0, 8550, 15), (86, 7, 0, 0, 8100, 16),
                (87, 7, 1, 0, 8050, 16), (116, 7, 30, 0, 6600, 16),
                (117, 7, 30, 1, 6550, 7), (240, 7, 30, 1, 10000, 0),
                (600, 7, 30, 1, 10000, 0)):
            result = self.run_host(["--demo-source-air-dash-floor", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"air-dash-floor-windup={windup} active={active}", result.stdout)
            self.assertIn(f"landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            self.assertIn("ordinary native spawn/bounds/target/references", result.stdout)
        result = self.run_host(["--source-air-dash-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("air-dash-floor-windup=0 active=0", result.stdout)
        self.assertNotIn("air-dash-floor demo:", result.stdout)
        result = self.run_host(["--source-air-dash-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v6", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM7", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_windup_entry_floor_after_relaunch_preserves_no_motion_and_later_costs(self):
        for tick, entry, windup, active, landings, meter, selector in (
                (80, 0, 0, 0, 0, 8300, 8), (81, 1, 1, 0, 0, 8300, 15),
                (88, 1, 8, 0, 0, 7850, 16), (89, 1, 8, 1, 0, 7800, 16),
                (119, 1, 8, 30, 1, 6300, 7), (240, 1, 8, 30, 1, 10000, 0),
                (600, 1, 8, 30, 1, 10000, 0)):
            result = self.run_host(["--demo-source-windup-entry-floor", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"windup-entry-floor-contacts={entry}", result.stdout)
            self.assertIn(f"air-dash-floor-windup={windup} active={active}", result.stdout)
            self.assertIn(f"landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            self.assertIn("ordinary native spawn/bounds/target/references", result.stdout)
        result = self.run_host(["--source-windup-entry-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("windup-entry-floor-contacts=0", result.stdout)
        self.assertNotIn("windup-entry-floor demo:", result.stdout)
        result = self.run_host(["--source-windup-entry-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v6", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM7", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_positive_windup_floor_entry_is_not_upward_motion(self):
        for tick, positive, entry, windup, active, landings, meter, selector in (
                (86, 0, 0, 0, 0, 0, 7700, 8), (87, 1, 1, 1, 0, 0, 7700, 15),
                (94, 1, 1, 8, 0, 0, 7250, 16), (95, 1, 1, 8, 1, 0, 7200, 16),
                (125, 1, 1, 8, 30, 1, 5700, 7), (240, 1, 1, 8, 30, 1, 10000, 0),
                (600, 1, 1, 8, 30, 1, 10000, 0)):
            result = self.run_host(["--demo-source-windup-positive-floor", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"windup-positive-floor-contacts={positive}", result.stdout)
            self.assertIn(f"windup-entry-floor-contacts={entry}", result.stdout)
            self.assertIn(f"air-dash-floor-windup={windup} active={active}", result.stdout)
            self.assertIn(f"landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            self.assertIn("ordinary native spawn/bounds/target/references", result.stdout)
        result = self.run_host(["--source-windup-positive-floor", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("windup-positive-floor-contacts=0", result.stdout)
        self.assertNotIn("windup-positive-floor demo:", result.stdout)
        result = self.run_host(["--source-windup-positive-floor", "--source-ground-assets",
                                "local/content/source-gred-ground-v6", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM7", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_windup_entry_push_keeps_ret0_damping_and_deferred_cost(self):
        for tick, entry, windup, pushes, landings, meter, selector in (
                (80, 0, 0, 1, 0, 8450, 6), (81, 1, 1, 2, 0, 8450, 15),
                (86, 1, 6, 7, 0, 8000, 16), (87, 1, 6, 7, 0, 7950, 16),
                (140, 1, 6, 7, 1, 6450, 7), (240, 1, 6, 7, 1, 10000, 0),
                (600, 1, 6, 7, 1, 10000, 0)):
            result = self.run_host(["--demo-source-windup-entry-push", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"windup-entry-surface-pushes={entry}", result.stdout)
            self.assertIn(f"windup-surface-pushes={windup}", result.stdout)
            self.assertIn(f"static-surface-pushes={pushes}", result.stdout)
            self.assertIn(f"landings={landings} resource={meter} selector={selector}", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            self.assertIn("native arena-half4500", result.stdout)
            self.assertIn("interactive arena/spawn unchanged", result.stdout)
        result = self.run_host(["--source-windup-entry-push", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("windup-entry-surface-pushes=0", result.stdout)
        self.assertNotIn("windup-entry-push demo only:", result.stdout)
        result = self.run_host(["--source-windup-entry-push", "--source-ground-assets",
                                "local/content/source-gred-ground-v6", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM7", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_walk_ledge_delays_falling_vm_and_drift_without_fake_jump(self):
        for tick, departures, entries, landings, timer, cached, flags in (
                (85, 1, 0, 0, "0.0", 0, "10"), (86, 1, 1, 0, "30.0", 1, "40"),
                (116, 1, 1, 0, "0.0", 1, "40"), (117, 1, 1, 1, "0.0", 1, "40"),
                (118, 1, 1, 1, "0.0", 0, "200"), (240, 1, 1, 1, "0.0", 0, "0"),
                (600, 1, 1, 1, "0.0", 0, "0")):
            result = self.run_host(["--demo-source-walk-ledge", "--headless",
                                    "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"accepted native tick={tick}", result.stdout)
            self.assertIn(f"ground-departures={departures} loss-entries={entries} loss-landings={landings}", result.stdout)
            self.assertIn(f"timer698={timer} cached87={cached} flags={flags}", result.stdout)
            self.assertIn("resource=10000", result.stdout)
            self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
            self.assertIn("interactive arena/spawn unchanged", result.stdout)
            if tick>=116:
                self.assertIn("suppressed-drift=30", result.stdout)
        result = self.run_host(["--source-walk-ledge", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("ground-departures=0 loss-entries=0 loss-landings=0", result.stdout)
        self.assertNotIn("walk-ledge demo only:", result.stdout)
        result = self.run_host(["--source-walk-ledge", "--source-ground-assets",
                                "local/content/source-gred-ground-v7", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM8", result.stdout)
        self.assertNotIn("accepted native tick=", result.stdout)

    def test_source_air_dash_policy_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-air-dash", "--headless", "--frames", "480", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=480", result.stdout)
        self.assertIn("air-dash-starts=2", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        self.assertIn("no per-flight count substitution", result.stdout)
        result = self.run_host(["--source-air-dash", "--source-ground-assets", "local/content/source-gred-ground-v3", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM4", result.stdout)

    def test_source_selected_surface_landing_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-surface", "--headless", "--frames", "240", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=240", result.stdout)
        self.assertIn("paired-surface-landings=1 last-angle-s16=4448", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("takeoff samples re-press without locomotion cancel", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("paired-surface-landings=0", result.stdout)
        result = self.run_host(["--source-surface", "--source-ground-assets", "local/content/source-gred-ground-v4", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM5", result.stdout)

    def test_source_steep_push_then_lower_floor_landing_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-steep", "--headless", "--frames", "240", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=240", result.stdout)
        self.assertIn("static-surface-pushes=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("no landing/refill", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--demo-source-steep", "--headless", "--frames", "82", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("static-surface-pushes=1", result.stdout)
        self.assertIn("landings=0", result.stdout)
        self.assertIn("selector=6", result.stdout)
        result = self.run_host(["--source-steep", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("static-surface-pushes=0", result.stdout)
        result = self.run_host(["--source-steep", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)

    def test_source_history_falling_surface_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-fall-surface", "--headless", "--frames", "240", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=240", result.stdout)
        self.assertIn("post-dash-fall-pushes=0 post-relaunch-fall-pushes=1 last-push-programs=21/21", result.stdout)
        self.assertIn("static-surface-pushes=1", result.stdout)
        self.assertIn("relaunches=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--demo-source-fall-surface", "--headless", "--frames", "123", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("post-relaunch-fall-pushes=1 last-push-programs=21/21", result.stdout)
        self.assertIn("landings=0", result.stdout)
        self.assertIn("selector=6", result.stdout)
        result = self.run_host(["--source-fall-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("post-relaunch-fall-pushes=0", result.stdout)
        result = self.run_host(["--source-fall-surface", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)

    def test_source_recovery_surface_push_retains_program12_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-recovery-surface", "--headless", "--frames", "240", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=240", result.stdout)
        self.assertIn("recovery-surface-pushes=1", result.stdout)
        self.assertIn("post-dash-fall-pushes=0 post-relaunch-fall-pushes=0 last-push-programs=12/12", result.stdout)
        self.assertIn("air-dash-starts=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--demo-source-recovery-surface", "--headless", "--frames", "73", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("recovery-surface-pushes=1", result.stdout)
        self.assertIn("last-push-programs=12/12", result.stdout)
        self.assertIn("landings=0", result.stdout)
        self.assertIn("selector=17", result.stdout)
        result = self.run_host(["--source-recovery-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("recovery-surface-pushes=0", result.stdout)
        result = self.run_host(["--source-recovery-surface", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)

    def test_source_continued_windup_surface_and_native_demo_assumptions(self):
        result = self.run_host(["--demo-source-windup-surface", "--headless", "--frames", "360", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=360", result.stdout)
        self.assertIn("windup-surface-pushes=2", result.stdout)
        self.assertIn("last-push-programs=10/10", result.stdout)
        self.assertIn("recovery-surface-pushes=0", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("air-dash-starts=1", result.stdout)
        self.assertIn("arena-half=4500 ground-query-reference=591.9127", result.stdout)
        self.assertIn("interactive arena/spawn unchanged", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        for tick, pushes, selector, meter in ((86, 1, 15, 8350), (87, 2, 16, 7900)):
            result = self.run_host(["--demo-source-windup-surface", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"windup-surface-pushes={pushes}", result.stdout)
            self.assertIn(f"landings=0 resource={meter} selector={selector}", result.stdout)
            self.assertIn("last-push-programs=10/10", result.stdout)
        result = self.run_host(["--demo-source-windup-surface", "--headless", "--frames", "600", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=600", result.stdout)
        self.assertIn("windup-surface-pushes=2", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        result = self.run_host(["--source-windup-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("windup-surface-pushes=0", result.stdout)
        self.assertNotIn("windup demo only:", result.stdout)
        result = self.run_host(["--source-windup-surface", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)

    def test_source_continued_active_surface_idle_and_legacy_pack_refusal(self):
        result = self.run_host(["--source-active-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("active-surface-pushes=0", result.stdout)
        self.assertIn("selector16/program11 spending before steering/motion/contact", result.stdout)
        result = self.run_host(["--source-active-surface", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)
        result = self.run_host(["--demo-source-active-surface", "--headless", "--frames", "360", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=360", result.stdout)
        self.assertIn("active-surface-pushes=1 last-active-push-programs=11/11", result.stdout)
        self.assertIn("recovery-surface-pushes=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("active demo only: native arena-half=4500", result.stdout)
        self.assertIn("releases direction after first accepted active push", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        for tick, selector, landings in ((157, 16, 0), (158, 17, 0), (159, 7, 1)):
            result = self.run_host(["--demo-source-active-surface", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("active-surface-pushes=1 last-active-push-programs=11/11", result.stdout)
            self.assertIn(f"landings={landings} resource=1850 selector={selector}", result.stdout)
        result = self.run_host(["--demo-source-active-surface", "--headless", "--frames", "600", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=600", result.stdout)
        self.assertIn("active-surface-pushes=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)

    def test_source_continued_relaunch_surface_and_separate_entry(self):
        result = self.run_host(["--demo-source-relaunch-surface", "--headless", "--frames", "240", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=240", result.stdout)
        self.assertIn("relaunch-surface-pushes=1 last-relaunch-push-programs=14/14", result.stdout)
        self.assertIn("relaunches=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("ordinary arena-half1800, target and floor references retained", result.stdout)
        self.assertNotIn("arena-half=4500", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        for tick, pushes, selector, landings, meter in ((82, 0, 8, 0, 8200), (83, 1, 8, 0, 8100), (84, 1, 7, 1, 8100)):
            result = self.run_host(["--demo-source-relaunch-surface", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn(f"relaunch-surface-pushes={pushes}", result.stdout)
            self.assertIn(f"landings={landings} resource={meter} selector={selector}", result.stdout)
        result = self.run_host(["--demo-source-relaunch-surface", "--headless", "--frames", "600", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=600", result.stdout)
        self.assertIn("relaunch-surface-pushes=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        result = self.run_host(["--source-relaunch-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("relaunch-surface-pushes=0", result.stdout)
        result = self.run_host(["--source-relaunch-surface", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)

    def test_source_descending_relaunch_entry_surface(self):
        result = self.run_host(["--demo-source-relaunch-entry-surface", "--headless", "--frames", "240", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=240", result.stdout)
        self.assertIn("relaunch-entry-surface-pushes=1", result.stdout)
        self.assertIn("relaunch-surface-pushes=1 last-relaunch-push-programs=14/14", result.stdout)
        self.assertIn("relaunches=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("not a motion override or original input/spawn script", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        for tick, selector, landings in ((79, 8, 0), (80, 7, 1)):
            result = self.run_host(["--demo-source-relaunch-entry-surface", "--headless", "--frames", str(tick), "--no-audio"])
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("relaunch-entry-surface-pushes=1", result.stdout)
            self.assertIn(f"landings={landings} resource=8400 selector={selector}", result.stdout)
        result = self.run_host(["--demo-source-relaunch-entry-surface", "--headless", "--frames", "600", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=600", result.stdout)
        self.assertIn("relaunch-entry-surface-pushes=1", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        result = self.run_host(["--source-relaunch-entry-surface", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("relaunch-entry-surface-pushes=0", result.stdout)
        result = self.run_host(["--source-relaunch-entry-surface", "--source-ground-assets", "local/content/source-gred-ground-v5", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM6", result.stdout)

    def test_source_ground_dash_interruption_reuses_takeoff(self):
        result = self.run_host(["--demo-source-dash-jump", "--headless", "--frames", "480", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=480", result.stdout)
        self.assertIn("dash-jumps=2 last-interrupted-selector=13", result.stdout)
        self.assertIn("landings=2 resource=10000 selector=0", result.stdout)
        self.assertIn("no MVP dash-momentum carry", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-dash-jump", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("dash-jumps=0", result.stdout)
        result = self.run_host(["--source-dash-jump", "--source-ground-assets", "local/content/source-gred-ground-v3", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM4", result.stdout)

    def test_source_landing_jump_dash_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-landing", "--headless", "--frames", "480", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=480", result.stdout)
        self.assertIn("landing-jumps=1 landing-dashes=1", result.stdout)
        self.assertIn("landings=2 resource=10000 selector=0", result.stdout)
        self.assertIn("accepted landing entry required", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-landing", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("landing-jumps=0 landing-dashes=0", result.stdout)
        result = self.run_host(["--source-landing", "--source-ground-assets", "local/content/source-gred-ground-v3", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM4", result.stdout)

    def test_source_relaunch_handoff_repeats_and_old_pack_refusal(self):
        result = self.run_host(["--demo-source-relaunch", "--headless", "--frames", "480", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("accepted native tick=480", result.stdout)
        self.assertIn("relaunches=2 lower-stage=0 upper-stage=0", result.stdout)
        self.assertIn("landings=1 resource=10000 selector=0", result.stdout)
        self.assertIn("additive launch velocity", result.stdout)
        self.assertIn("projectiles-spawned=0 hits=0 health=100/100", result.stdout)
        result = self.run_host(["--source-relaunch", "--headless", "--frames", "10", "--no-audio"])
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("relaunches=0", result.stdout)
        result = self.run_host(["--source-relaunch", "--source-ground-assets", "local/content/source-gred-ground-v3", "--headless", "--no-audio"])
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("requires GFMM4", result.stdout)


if __name__ == "__main__":
    unittest.main()
