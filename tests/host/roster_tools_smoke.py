"""Check metadata-only preset listing and zero-tick headless admission."""
from argparse import ArgumentParser
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory

from tools.analyze.native_roster_matrix import audit
from tools.extract.demo_borgs import mannequin, model, prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary):
    binary = binary.resolve(strict=True)
    presets = Path(__file__).resolve().parents[2] / "examples/battle-rosters"
    with TemporaryDirectory(prefix="gf-roster-tools-") as scratch:
        root = Path(scratch)
        empty = root / "empty"
        empty.mkdir()
        library = root / "library"
        library.mkdir()
        solo = library / "a-solo.roster"
        solo.write_bytes((presets / "workshop-solo.roster").read_bytes())
        timed = library / "z-timed.roster"
        timed.write_text((presets / "workshop-timed.roster").read_text() + "rounds-to-win=2\n")
        (library / "broken.roster").write_text("GFROSTER 99\n")
        (library / "linked.roster").symlink_to(solo)
        (library / "ignore.txt").write_bytes(solo.read_bytes())
        (library / "subdir").mkdir()
        (library / "subdir/hidden.roster").write_bytes(solo.read_bytes())

        def host(*args, cwd=empty):
            return run([binary, *args], cwd=cwd, capture_output=True, text=True, timeout=40)

        listed = host("--list-battle-rosters", library)
        assert listed.returncode == 0, listed.stderr
        assert listed.stdout.splitlines() == [
            "a-solo.roster | stage=example:workshop | slots=6 | locals=1 | metadata only",
            "z-timed.roster | stage=example:workshop | slots=3 | locals=2 | first-to=2 | seconds=45 | metadata only",
        ], listed.stdout
        assert "broken.roster: malformed roster preset" in listed.stderr and "linked.roster:" in listed.stderr, listed.stderr
        top_link = root / "library-link"
        top_link.symlink_to(library, target_is_directory=True)
        assert host("--list-battle-rosters", top_link).returncode == 2
        assert host("--list-battle-rosters", library, "--headless").returncode == 2
        over = root / "oversized-directory"
        over.mkdir()
        (over / "valid.roster").write_bytes(solo.read_bytes())
        for n in range(256):
            (over / f"entry-{n}.txt").touch()
        refused = host("--list-battle-rosters", over)
        assert refused.returncode == 2 and not refused.stdout and "256-entry" in refused.stderr, refused.stderr

        assert host("--check-battle-roster", solo).returncode == 2, "listing must not imply installed assets"
        content = root / "content"
        prepare(content)
        stage = prepare_stage(content)
        check = host("--check-battle-roster", solo, cwd=content)
        assert check.returncode == 0 and "backend=headless" in check.stderr, check.stderr
        assert "Roster admission passed: stage=example:workshop slots=6 locals=1 ticks=0 (headless)" in check.stdout, check.stdout
        prefix = "slot=0 team=0 driver=local id=example:flight-stick variant=normal spawn=west position="
        positions = [line.removeprefix(prefix).split()[0] for line in check.stdout.splitlines() if line.startswith(prefix)]
        assert len(positions) == 1 and tuple(map(float, positions[0].split(","))) == (-240, 0, 300), check.stdout
        bodies = [int(line.rsplit(" body-draws=", 1)[1]) for line in check.stdout.splitlines() if line.startswith("slot=")]
        assert len(bodies) == 6 and all(draws > 0 for draws in bodies), check.stdout
        assert "slot=1 team=0 driver=cpu id=example:jump-stick variant=normal spawn=west-outer " in check.stdout
        assert "borg-api:" not in check.stderr and "package-battle: frames=" not in check.stderr, check.stderr
        assert solo.read_bytes() == (presets / "workshop-solo.roster").read_bytes()
        clock = host("--check-battle-roster", timed, cwd=content)
        assert clock.returncode == 0 and "ticks=0" in clock.stdout, clock.stderr
        assert "borg-round-clock:" not in clock.stderr and "borg-series:" not in clock.stderr
        assert host("--check-battle-roster", solo, "--frames", "1", cwd=content).returncode == 2
        matrix = audit(binary, content, "example", 2, ticks=30)
        assert matrix["ran"] == matrix["preflight"] == matrix["subjects"] == 2, matrix
        assert matrix["groups"][0]["subjects"] == ["example:flight-stick", "example:jump-stick"], matrix
        assert [c["tick"] for c in matrix["checkpoints"]] == [30]

        # Put the CPU shooter beside the idle one-HP anchor on the same
        # side of the wall. The distant CPU has an opening delay of 25.
        rules = content / "borgs/flight-stick/gameplay.txt"
        manifest = stage / "stage.manifest"
        rules_text, manifest_text = rules.read_text(), manifest.read_text()
        rules.write_text(rules_text.replace("health=160", "health=1"))
        manifest.write_text(manifest_text.replace("spawn=west|-240|20|300", "spawn=west|0|0|-200")
                            .replace("spawn=east|240|-10|-300", "spawn=east|0|0|-320")
                            .replace("spawn=east-outer|600|-10|-300", "spawn=east-outer|300|0|-320"))
        ko = audit(binary, content, "example", 2, ticks=30, checkpoints=(1,))
        group = ko["groups"][0]
        assert group["ran"] and group["result"] == "team1" and 1 < group["accepted_ticks"] < 25, group
        early, late = ko["checkpoints"]
        assert early["groups_ended"] == 0 and early["quiet_fighting"] == 1, ko
        assert late["groups_ended"] == 1 and late["quiet_round_ended"] == 1 and late["cpu_enemy_hit"] == 1, ko
        shooter = group["activity"][0]
        assert shooter["first_payload_kind"] == "emit" and shooter["first_move_tick"] < shooter["first_payload_tick"]
        assert shooter["first_payload_tick"] <= shooter["first_enemy_hit_tick"] and late["cpu_payload_seen"] == 1, ko
        rules.write_text(rules_text)
        manifest.write_text(manifest_text)

        bad_spawn = root / "bad-spawn.roster"
        bad_spawn.write_text(solo.read_text().replace("|normal|east-rear", "|normal|missing"))
        failed = host("--check-battle-roster", bad_spawn, cwd=content)
        assert failed.returncode == 2 and "missing stage spawn marker: missing" in failed.stderr, failed.stderr
        assert "Roster admission passed" not in failed.stdout

        # A valid rig whose meshes are all behind the camera must not pass
        # just because the stage, the HUD and the other fighters still draw.
        culled = root / "culled-body.roster"
        culled.write_text("GFROSTER 1\nlocals=1\nstage=example:workshop\n"
                          "fighter=0|example:jump-stick|normal|west\n"
                          "fighter=1|example:flight-stick|normal|east\n"
                          "fighter=0|example:jump-stick|normal|west-rear\n")
        body_file = content / "borgs/flight-stick/body.hsd"
        original_body = body_file.read_bytes()
        parts = mannequin(0xD55848FF, 0xFFD263FF)
        displaced = [(*part[:4], [((x, y + 1_000_000, z), half) for (x, y, z), half in part[4]], part[5])
                     for part in parts]
        body_file.write_bytes(model([displaced], striped_bone=5)[0])
        unseen = host("--check-battle-roster", culled, cwd=content)
        assert unseen.returncode == 2 and "slot 1 (example:flight-stick) did not submit body geometry" in unseen.stderr
        assert "Roster admission passed" not in unseen.stdout
        body_file.write_bytes(original_body)

        (stage / "art/workshop.hsd").write_bytes(b"not an HSD model")
        corrupt = host("--check-battle-roster", solo, cwd=content)
        assert corrupt.returncode == 2 and "portable stage admission refused" in corrupt.stderr, corrupt.stderr
        assert "Roster admission passed" not in corrupt.stdout


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    smoke(parser.parse_args().binary)
    print("Roster tools: bounded metadata listing, exact zero-tick admission, and dependency refusals PASS")
