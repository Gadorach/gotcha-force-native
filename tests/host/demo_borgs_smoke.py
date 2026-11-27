"""Exercise the generated examples without any proprietary input or loose retail art."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare


def smoke(binary, opengl=False):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-demo-borgs-") as scratch:
        root = Path(scratch) / "examples"
        prepare(root)
        for first, second in (("flight-stick", "jump-stick"), ("jump-stick", "flight-stick")):
            cmd = [binary, "--battle-borg", "example:" + first, "--against-borg", "example:" + second,
                   "--battle-mvp-assets", "arena", "--demo-package-battle", "--trace-package-battle",
                   "--no-audio", "--frames", "215", *([] if opengl else ["--headless"])]
            result = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "event=socket-launch" in log and "event=hit" in log, log
            if first == "flight-stick":
                assert "package-flight: peak=32 cap=32" in log, log
                assert "move=paint-stream event=socket-launch frame=5" in log, log
                assert "move=paint-stream event=hit" in log, log
            else:
                assert "actor=0 cues=10" in log and "actor=0 cues=4" in log, log
                assert "move=lined-shot event=aim-committed frame=3" in log, log
                assert "move=lined-shot event=socket-launch frame=12" in log, log
        wall_root = Path(scratch) / "clipped"
        prepare(wall_root, clipped_beam=True)
        wall_cmd = [binary, "--battle-borg", "example:flight-stick", "--against-borg", "example:jump-stick",
                    "--battle-mvp-assets", "arena-wall", "--demo-package-battle", "--trace-package-battle",
                    "--no-audio", "--frames", "82", *([] if opengl else ["--headless"])]
        for root_dir, should_hit in ((wall_root, False), (root, True)):
            result = run(wall_cmd, cwd=root_dir, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-stream event=socket-launch frame=5" in log, log
            assert ("move=paint-stream event=hit" in log) == should_hit, log
            assert "alpha=0.250" in log, log
        sweep_root = Path(scratch) / "swept"
        prepare(sweep_root, swept_beam=True)
        result = run(wall_cmd, cwd=sweep_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and log.count("move=paint-stream event=stage-impact") >= 2, log
        assert "move=paint-stream event=blast-hit" not in log, log
        smoke_root = Path(scratch) / "smoke"
        prepare(smoke_root, smoke_bomb=True)
        result = run(cmd, cwd=smoke_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "move=drop-charge event=smoke-created" in log, log
        assert "move=drop-charge event=blast-hit" not in log, log
        power_root = Path(scratch) / "power"
        prepare(power_root, power_field=True)
        result = run(["arena" if word == "arena-wall" else word for word in wall_cmd],
                     cwd=power_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "move=paint-scope event=power-field-placed" in log, log
        assert "move=paint-shot event=shot-powered" in log, log
        slow_root = Path(scratch) / "slow"
        prepare(slow_root, slow_field=True)
        result = run(["arena" if word == "arena-wall" else word for word in wall_cmd],
                     cwd=slow_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "move=paint-scope event=slow-field-placed" in log, log
        assert "event=shot-powered" not in log, log
        pulse_root = Path(scratch) / "walk-pulse"
        prepare(pulse_root, walk_pulse=True)
        result = run(["arena" if word == "arena-wall" else word for word in wall_cmd],
                     cwd=pulse_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "event=walk-pulse-created" in log, log
        # Broaden this scratch wave to reach both enemies. Cadence should
        # stay the same, and each emitted wave still resolves only once.
        pulse_profile = pulse_root / "borgs/flight-stick/gameplay.txt"
        rules = pulse_profile.read_text()
        assert rules.count("walk-pulse=40|12|130|25|10|6\n") == 1
        pulse_profile.write_text(rules.replace("walk-pulse=40|12|130|25|10|6\n", "walk-pulse=16|12|240|25|10|6\n"))
        result = run([binary, "--battle-borg", "example:flight-stick", "--against-borg", "example:jump-stick",
                      "--battle-mvp-assets", "arena", "--demo-roster-battle", "--trace-package-battle",
                      "--package-battle-distance", "200", "--no-audio", "--frames", "60",
                      *([] if opengl else ["--headless"])], cwd=pulse_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        waves = re.findall(r"actor=0 move=- event=walk-pulse-created .*object=(\d+)", log)
        contacts = re.findall(r"actor=0 move=- event=hit .*object=(\d+) recipient=(\d+) value=6", log)
        assert result.returncode == 0 and len(waves) == len(contacts) == 12 and "actors=4 " in log, log
        assert {obj for obj, _ in contacts} == set(waves) and all(slot in ("1", "3") for _, slot in contacts), log
        if not opengl:
            cruise = Path(scratch) / "cruise"
            prepare(cruise, cruise_boost=True)
            result = run(["arena" if word == "arena-wall" else word for word in wall_cmd],
                         cwd=cruise, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "actors=2 restarts=0 moved=1 airborne=1" in log, log
            gravity = Path(scratch) / "gravity"
            prepare(gravity, gravity_field=True)
            result = run([*["arena" if word == "arena-wall" else word for word in wall_cmd],
                          "--package-battle-distance", "260"], cwd=gravity, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-scope event=field-placed" in log, log
            assert "event=field-shot-pulled" in log and "move=paint-scope event=hit " in log, log
            magnet = Path(scratch) / "magnet"
            prepare(magnet, magnet_field=True)
            result = run([*["arena" if word == "arena-wall" else word for word in wall_cmd],
                          "--package-battle-distance", "260"], cwd=magnet, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-scope event=field-actor-pulled" in log, log
            assert "event=field-shot-pulled" not in log and "move=paint-scope event=hit " not in log, log
            repel = Path(scratch) / "repel"
            prepare(repel, repel_field=True)
            result = run([*["arena" if word == "arena-wall" else word for word in wall_cmd],
                          "--package-battle-distance", "260"], cwd=repel, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-scope event=field-actor-pulled" in log, log
        fence = Path(scratch) / "fence"
        prepare(fence, electric_fence=True)
        result = run([*["arena" if word == "arena-wall" else word for word in wall_cmd],
                      "--package-battle-distance", "260"], cwd=fence, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and log.count("move=paint-scope event=anchor-placed") == 2, log
        assert "move=- event=fence-recoil" in log and "move=- event=hit frame=0" in log, log
        # The wide authored fence reaches both enemies from the four-actor
        # spawn, including the one outside the owner's current aim line.
        profile = fence / "borgs/flight-stick/gameplay.txt"
        rules = profile.read_text()
        assert rules.count("anchor-fence=fence|26|8|90\n") == 1
        profile.write_text(rules.replace("anchor-fence=fence|26|8|90\n", "anchor-fence=fence|120|8|90\n"))
        roster = run([binary, "--battle-borg", "example:flight-stick", "--against-borg", "example:jump-stick",
                      "--battle-mvp-assets", "arena", "--demo-roster-battle", "--trace-package-battle",
                      "--package-battle-distance", "200", "--no-audio", "--frames", "60",
                      *([] if opengl else ["--headless"])], cwd=fence, capture_output=True, text=True, timeout=35)
        log = roster.stdout + roster.stderr
        recipients = re.findall(r"actor=0 move=- event=hit frame=0 object=\d+ recipient=(\d+) value=8", log)
        assert roster.returncode == 0 and "actors=4 " in log and recipients == ["1", "3"], log
        trail = Path(scratch) / "impact-trail"
        prepare(trail, impact_trail=True)
        result = run(wall_cmd, cwd=trail, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "move=paint-shot event=impact-trail-started" in log, log
        assert not any("move=paint-shot event=" + event in log for event in ("hit", "blast-trigger", "impact-children")), log
        if not opengl:
            hazard = Path(scratch) / "impact-hazard"
            prepare(hazard, impact_hazard=True)
            result = run([*wall_cmd, "--package-battle-distance", "260"], cwd=hazard, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-shot event=impact-hazard-started" in log, log
            assert "move=paint-shot event=impact-trail-started" in log and "move=paint-shot event=impact-hazard-placed" in log, log
            assert "move=paint-shot event=blast-trigger" not in log, log
            hits = [line for line in log.splitlines() if "move=paint-shot event=hit " in line]
            assert len(hits) == 2 and all("recipient=1 value=5" in line for line in hits), log
            for shared_shapes in (False, True):
                area = Path(scratch) / ("impact-area-shared" if shared_shapes else "impact-area")
                prepare(area, impact_area=True)
                if shared_shapes:
                    profile = area / "borgs/flight-stick/gameplay.txt"
                    profile.write_text(profile.read_text() + "impact-area-shapes=paint|shared\n")
                result = run([*wall_cmd, "--package-battle-distance", "260"], cwd=area, capture_output=True, text=True, timeout=35)
                log = result.stdout + result.stderr
                placed = re.findall(r"tick=(\d+) actor=0 move=paint-shot event=impact-hazard-placed frame=0 object=(\d+)", log)
                hits = re.findall(r"tick=(\d+) actor=0 move=paint-shot event=hit frame=0 object=(\d+) recipient=1 value=17", log)
                ticks = {obj: int(tick) for tick, obj in placed}
                assert result.returncode == 0 and placed and hits and len({obj for _, obj in hits}) == len(hits), log
                assert all(obj in ticks and int(tick) > ticks[obj] for tick, obj in hits), log
                assert "move=paint-shot event=blast-trigger" not in log and "event=projectile-shape-refused" not in log, log
            elemental = Path(scratch) / "elemental"
            prepare(elemental, elemental_ball=True)
            result = run(["arena" if word == "arena-wall" else word for word in wall_cmd],
                         cwd=elemental, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-shot event=flight-children" in log, log
            impact = Path(scratch) / "impact"
            prepare(impact, impact_ball=True)
            result = run(wall_cmd, cwd=impact, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-shot event=stage-impact" in log, log
            assert "move=paint-shot event=impact-children" in log and "move=paint-shot event=blast-trigger" not in log, log
        mixed_root = Path(scratch) / "mixed"
        prepare(mixed_root, field_demo=True)
        mixed_cmd = ["arena" if word == "arena-wall" else word for word in wall_cmd]
        result = run(mixed_cmd, cwd=mixed_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "move=paint-scope event=field-placed" in log and "event=shot-powered" in log, log
        if not opengl:
            field_profile = mixed_root / "borgs/flight-stick/gameplay.txt"
            field_profile.write_text(field_profile.read_text() + "field-follow=paint-scope|source\n")
            result = run(mixed_cmd, cwd=mixed_root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "event=field-placed" in log and "event=field-anchor-lost" not in log, log
            (mixed_root / "borgs/flight-stick/scope.rgba").write_bytes(b"\0")
            result = run(mixed_cmd, cwd=mixed_root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode != 0 and "missing or invalid field texture" in log, log
        ink_root = Path(scratch) / "ink"
        prepare(ink_root, ink_bomb=True)
        result = run(cmd, cwd=ink_root, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0 and "move=drop-charge event=ink-applied" in log, log
        if not opengl:
            accuracy_root = Path(scratch) / "ink-accuracy"
            prepare(accuracy_root, accuracy_ink=True)
            result = run(cmd, cwd=accuracy_root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=drop-charge event=accuracy-applied" in log, log
            assert "actor=1 move=paint-shot event=accuracy-affected" in log, log
        if not opengl:
            bomb_root = Path(scratch) / "bombs"
            prepare(bomb_root, proximity_bomb=True)
            result = run(cmd, cwd=bomb_root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=drop-charge event=projectile-landed" in log, log
            assert "move=drop-charge event=bomb-proximity" in log and "move=drop-charge event=stock-returned" in log, log
            attracting = Path(scratch) / "attraction"
            prepare(attracting, attracting_bomb=True)
            attracting_cmd = ["arena-bomb" if word == "arena" else word for word in cmd]
            result = run(attracting_cmd, cwd=attracting, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=drop-charge event=bomb-proximity" in log, log
            fragments = Path(scratch) / "fragments"
            prepare(fragments, fragmenting_bomb=True)
            result = run(cmd, cwd=fragments, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=drop-charge event=blast-children" in log, log
            # Same clip policy, same wall grid, no fade: collision still blocks.
            fade = wall_root / "arena-wall/view-fade.txt"
            saved_fade = fade.read_bytes()
            fade.rename(fade.with_suffix(".off"))
            result = run(wall_cmd, cwd=wall_root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "move=paint-stream event=socket-launch frame=5" in log, log
            assert "move=paint-stream event=hit" not in log and "stage-view:" not in log, log
            fade.write_bytes(saved_fade.replace(b"obstacle=occlusion|4|5|", b"obstacle=occlusion|3|5|"))
            invalid = run(wall_cmd, cwd=wall_root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "stage viewport fade:" in log, log
            fade.write_bytes(saved_fade)
            # The rail grid cannot cover this altered declared rectangle.
            descriptor = wall_root / "arena-wall/projectile-stage.gfsr"
            declared = descriptor.read_bytes()
            descriptor.write_bytes(declared[:20] + bytes.fromhex("46000000") + declared[24:])
            invalid = run(wall_cmd, cwd=wall_root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "authored stage projectile collision" in log, log
            # The following change is confined to the disposable example. It
            # parses, but the untextured rig must not be admitted with a scroll.
            path = root / "borgs/jump-stick/gameplay.txt"
            profile = path.read_text()
            path.write_text(profile.replace("part-material=2|0|0|5|loop", "part-material=2|0|2|2|sample"))
            sampled = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = sampled.stdout + sampled.stderr
            assert sampled.returncode == 0 and "event=hit" in log, log
            path.write_text(profile + "part-scroll=paint-shot|5|u|8|forward\n")
            invalid = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "part-scroll needs explicit repeating texture coordinates" in log, log
            path.write_text(profile.replace("part-material=2|0|0|5|loop\n", "").replace("|3|10|2\n", "|3|10|255\n"))
            invalid = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "unavailable part-swap record or material clip" in log, log
            path.write_text(profile.replace("part-swap-bone=paint-shot|badge|4", "part-swap-bone=paint-shot|badge|31"))
            invalid = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "unbound part-swap bone" in log, log
            path.write_text(profile.replace("part-material=2|0|0|5|loop", "part-material=2|0|0|25|loop"))
            invalid = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "unavailable part-swap record or material clip" in log, log
            path.write_text(profile)
            flight = root / "borgs/flight-stick/gameplay.txt"
            flight.write_text(flight.read_text().replace("|18|28|3|ignore-floor", "|18|28|3|clip-on-stage"))
            invalid = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = invalid.stdout + invalid.stderr
            assert invalid.returncode != 0 and "stage projectile collision requires the qualified matching stage" in log, log
        assert not list(root.rglob("*.pcm")), "examples must not pull source audio"
    print(f"Original Flight/Jump Stick and flat arena PASS ({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
