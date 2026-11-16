import os
from pathlib import Path
import subprocess
import unittest


@unittest.skipUnless(os.environ.get("GF_SOURCE_WORLD_BINARY"), "source ground host not requested")
class SourceGroundWorldTests(unittest.TestCase):
    def run_host(self, args):
        binary = str(Path(os.environ["GF_SOURCE_WORLD_BINARY"]).resolve(strict=True))
        return subprocess.run([binary, *args], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, timeout=90)

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
