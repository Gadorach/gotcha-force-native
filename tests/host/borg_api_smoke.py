"""Exercise exact local packages in the real host (original art stays ignored)."""
import argparse
import re
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.native_battle_mvp import read
from tools.extract.portable_borg_preview import prepare


def smoke(executable, art, movement, opengl=False):
    executable, art, movement = (p.resolve(strict=True) for p in (executable, art, movement))
    with TemporaryDirectory(prefix="borg-api-smoke-") as temporary:
        root = Path(temporary)
        for model in ("g-red", "sasuke", "arc-scout", "grapple-scout", "charge-scout", "blade-scout",
                      "rotary-scout", "bomb-scout", "remote-scout", "seeker-scout", "guard-scout", "impact-scout",
                      "wingmate-scout", "rewind-scout", "wire-trail-scout", "anchor-arrow-scout", "target"):
            prepare(model, art, root / "borgs" / model,
                    movement if model == "g-red" else None, model in ("g-red", "sasuke"))
        # Verify that model and motion data really come from the packages.
        stage = root / "stage-only"
        stage.mkdir()
        for name in ("stage.hsd", "floor.stih"):
            with (stage / name).open("xb") as dest:
                dest.write(read(art / name, 32 << 20))

        def launch(player, opponent, *extra, expected=0, frames="150", stage_grids=movement):
            result = run([executable, "--battle-borg", player, "--against-borg", opponent,
                          "--battle-mvp-assets", str(stage), "--source-ground-assets", str(stage_grids),
                          "--demo-package-battle", "--trace-package-battle", "--no-audio", "--frames", frames,
                          *([] if opengl else ["--headless"]), *extra],
                         cwd=root, capture_output=True, text=True, timeout=40)
            log = result.stdout + result.stderr
            if result.returncode != expected:
                raise AssertionError(log or f"no log; exit {result.returncode}")
            return log

        gred = launch("gg4e08:0615", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=2 hits=2" in gred, gred
        assert "move=beam-test event=debit frame=16" in gred, gred
        sasuke = launch("gg4e08:000a", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=3 hits=3" in sasuke, sasuke
        assert "move=shuriken-test event=repeat frame=0 object=0 recipient=255 value=2" in sasuke, sasuke
        arc = launch("test:arc-scout", "gg4e08:000a")
        assert "restarts=1 moved=1 airborne=1 shots=6 hits=6" in arc, arc
        assert "move=arc-twin event=emit frame=6 object=2 recipient=255 value=2" in arc, arc
        assert "move=arc-twin event=modifier-applied" in arc, arc
        grapple = launch("test:grapple-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=4 hits=4" in grapple, grapple
        pulls = re.findall(r"round=(\d+) tick=(\d+) actor=0 move=grapple-throw event=pull-(queued|applied|refused)"
                           r" frame=0 object=(\d+) recipient=1 value=(\d+)(?: reason=([\w-]+))?", grapple)
        assert len(pulls) == 4, grapple
        for queued, outcome in zip(pulls[::2], pulls[1::2]):
            assert queued[2] == "queued" and queued[4] == "36", grapple
            assert outcome[0] == queued[0] and outcome[3] == queued[3] and int(outcome[1]) == int(queued[1]) + 1, grapple
        # The first scripted jump overlaps the next update; restart leaves the
        # second caster grounded so the corresponding request can apply.
        assert pulls[1][2] == "refused" and pulls[1][5] == "owner-busy", grapple
        assert pulls[3][2] == "applied" and pulls[3][4] == "36", grapple
        charge = launch("test:charge-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=6 hits=6" in charge, charge
        assert "move=pulse-tap event=button-release frame=0 object=0 recipient=255 value=1" in charge, charge
        assert "move=pulse-charge event=button-release frame=0 object=0 recipient=255 value=12" in charge, charge
        tap = charge.index("move=pulse-tap event=button-release")
        paid = charge.index("move=pulse-tap event=debit frame=3", tap)
        assert "move=pulse-tap event=emit frame=3 object=2 recipient=255 value=1" in charge[paid:], charge
        charged = charge.index("move=pulse-charge event=button-release")
        assert "move=pulse-charge event=emit frame=3 object=2 recipient=255 value=3" in charge[charged:], charge
        assert "move=pulse-charge event=debit" not in charge, charge
        for player, melee in (("test:blade-scout", "close-cut"), ("gg4e08:000a", "close-b-test")):
            close = launch(player, "test:target", "--package-battle-distance", "90")
            assert "restarts=1 moved=1 airborne=1 shots=0 hits=2" in close, close
            for kind in ("melee-armed", "hit", "melee-retired"):
                assert close.count(f"move={melee} event={kind}") == 2, close
            assert not re.search(r"event=(?:debit|emit)\b", close), close
        rotary = launch("test:rotary-scout", "test:target")
        def rotary_events(event):
            return [(int(r), int(t), int(value)) for r,t,value in re.findall(
                rf"round=(\d+) tick=(\d+) actor=0 move=rotary-b event={event} frame=\d+ object=\d+ recipient=255 value=(\d+)",
                rotary)]
        starts, stops = rotary_events("held-fire-start"), rotary_events("held-fire-stop")
        cadence = rotary_events("held-fire-cadence")
        emitted = rotary_events("emit")
        assert len(starts) == len(stops) == 3 and len(emitted) == len(cadence), rotary
        assert starts[0][2] == starts[2][2] == 0 and 0 < starts[1][2] < 1000, rotary
        assert {gap for r, t, gap in cadence if r == 0 and t < stops[0][1]} >= {3, 4, 7}, rotary
        for r,t,_ in emitted:
            assert any(sr == er == r and st < t < et for (sr,st,_),(er,et,_) in zip(starts,stops)), rotary
        assert len(rotary_events("debit")) == len(emitted) and not rotary_events("button-release"), rotary
        bomb = launch("test:bomb-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=4 hits=4" in bomb, bomb
        assert bomb.count("move=bomb-throw event=blast-hit") == 2, bomb
        for rnd in (0, 1):
            # Trigger and damage share a tick but remain distinct results.
            emitted = re.search(rf"round={rnd} tick=(\d+).*move=bomb-throw event=emit frame=3 object=2", bomb)
            triggered = re.search(rf"round={rnd} tick=(\d+).*move=bomb-throw event=blast-triggered frame=0 object=2 recipient=255 value=4", bomb)
            hit = re.search(rf"round={rnd} tick=(\d+).*move=bomb-throw event=blast-hit frame=0 object=2 recipient=1 value=32", bomb)
            assert emitted and triggered and hit and int(triggered[1])-int(emitted[1]) == 18 and triggered[1]==hit[1], bomb
        assert "move=bomb-throw event=hit" not in bomb and "move=bomb-throw event=projectile-contact" not in bomb, bomb
        remote = launch("test:remote-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=2 hits=2" in remote, remote
        for rnd in (0, 1):
            landed = re.search(rf"round={rnd} tick=(\d+).*move=deploy event=projectile-landed frame=0 object=1", remote)
            signal = re.search(rf"round={rnd} tick=(\d+).*move=trigger event=detonation frame=3 object=0 recipient=255 value=1", remote)
            hit = re.search(rf"round={rnd} tick=(\d+).*move=deploy event=blast-hit frame=0 object=1 recipient=1 value=26", remote)
            returned = re.search(rf"round={rnd} tick=(\d+).*move=deploy event=stock-returned frame=0 object=1 recipient=255 value=3", remote)
            assert landed and signal and hit and returned and int(landed[1]) < int(signal[1]) and signal[1] == hit[1] == returned[1], remote
        assert "move=trigger event=debit" not in remote, remote
        assert "event=refill" not in remote and remote.count("event=stock-returned") == 2, remote
        seeker = launch("test:seeker-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=4 hits=4" in seeker, seeker
        for rnd in (0, 1):
            bound = re.search(rf"round={rnd} tick=(\d+).*move=seeker-shot event=guidance-bound frame=0 object=2 recipient=1 value=1", seeker)
            hit = re.search(rf"round={rnd} tick=(\d+).*move=seeker-shot event=hit frame=0 object=2 recipient=1 value=16", seeker)
            assert bound and hit and int(hit[1])>int(bound[1]), seeker
        guard = launch("test:arc-scout", "test:guard-scout")
        assert "restarts=1 moved=1 airborne=1 shots=6 hits=4" in guard, guard
        for rnd in (0, 1):
            block = re.search(rf"round={rnd} tick=(\d+) actor=1 move=brace event=guard-blocked frame=\d+ object=1 recipient=0 value=0", guard)
            retire = re.search(rf"round={rnd} tick=(\d+) actor=0 move=primary event=retire frame=0 object=1 recipient=255 value=7", guard)
            assert block and retire and block[1] == retire[1], guard
        assert "move=primary event=hit" not in guard and guard.count("move=brace event=guard-broken") == 2, guard
        impact = launch("test:impact-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=4 hits=4" in impact, impact
        for rnd in (0, 1):
            assert re.search(rf"round={rnd}.*move=beam event=blast-triggered frame=0 object=1 recipient=255 value=1", impact), impact
            assert re.search(rf"round={rnd}.*move=bomb event=stage-impact frame=0 object=2 recipient=255 value=[123]", impact), impact
            assert re.search(rf"round={rnd}.*move=bomb event=blast-triggered frame=0 object=2 recipient=255 value=8", impact), impact
        unbound = launch("test:impact-scout", "test:target", expected=2, frames="1", stage_grids=stage)
        assert "requires the qualified matching stage and all three st42 grids" in unbound, unbound
        wingmate = launch("test:wingmate-scout", "test:target")
        assert "restarts=1 moved=1 airborne=1 shots=7 hits=7" in wingmate, wingmate
        assert wingmate.count("event=emitter-deployed") == 4 and wingmate.count("event=emitter-retired") == 4, wingmate
        assert wingmate.count("event=debit") == 3 and wingmate.count("event=emitter-fired") == 4, wingmate
        for rnd in (0, 1):
            paid = re.search(rf"round={rnd} tick=(\d+).*move=primary event=debit frame=3", wingmate)
            followed = re.findall(rf"round={rnd} tick=(\d+).*move=primary event=emitter-fired frame=3", wingmate)
            assert paid and followed == [paid[1], paid[1]], wingmate
        rewind = launch("test:rewind-scout", "test:target")
        placed = re.findall(r"round=0.*move=place event=anchor-placed frame=2 object=(\d+)", rewind)
        visited = re.findall(r"round=0.*move=rewind event=rewind-reached frame=0 object=(\d+)", rewind)
        assert len(placed) == 2 and visited == placed[::-1], rewind
        assert rewind.count("event=tap frame=0") == 2 and rewind.count("event=double-tap frame=0") == 2, rewind
        assert "move=rewind event=rewind-completed" in rewind and "move=rewind event=anchor-refused" in rewind, rewind
        assert "restarts=1 moved=1 airborne=1 shots=2 hits=2" in rewind, rewind
        wire = launch("test:wire-trail-scout", "test:arc-scout")
        assert "restarts=1 moved=1 airborne=1 shots=4 hits=4" in wire, wire
        carrier = re.search(r"round=0 tick=(\d+).*move=wire-release event=anchor-path-sealed frame=3 object=(\d+) recipient=255 value=2", wire)
        hit = re.search(r"round=0 tick=(\d+).*move=wire-release event=hit frame=0 object=(\d+) recipient=1 value=9", wire)
        queued = re.search(r"round=0 tick=(\d+).*move=wire-release event=rewind-queued frame=0 object=(\d+) recipient=1 value=2", wire)
        assert carrier and hit and queued and carrier[2]==hit[2]==queued[2] and int(carrier[1])<int(hit[1]) and hit[1]==queued[1], wire
        assert "move=wire-release event=debit" not in wire
        fallback = re.search(r"round=1 tick=(\d+).*move=wire-release event=anchor-fallback frame=3", wire)
        plain = re.search(r"round=1 tick=(\d+).*move=wire-release event=hit frame=0 object=(\d+) recipient=1 value=9", wire)
        assert fallback and plain and int(plain[1])>int(fallback[1]), wire
        assert not re.search(r"round=1.*move=wire-release event=(anchor-path-sealed|rewind-queued)", wire), wire
        arrow = launch("test:anchor-arrow-scout", "test:arc-scout")
        assert "restarts=1 moved=1 airborne=1 shots=4 hits=3" in arrow, arrow
        stage_hit = re.search(r"round=0 tick=(\d+).*move=plant event=stage-impact frame=0 object=(\d+)", arrow)
        planted = re.search(r"round=0 tick=(\d+).*move=plant event=anchor-planted frame=0 object=(\d+)", arrow)
        stored = re.search(r"round=0 tick=(\d+).*move=plant event=anchor-placed frame=0 object=(\d+)", arrow)
        reached = re.search(r"round=0 tick=(\d+).*move=wire-release event=rewind-reached frame=0 object=(\d+) recipient=1 value=0", arrow)
        assert stage_hit and planted and stored and reached and stage_hit.groups()==planted.groups(), arrow
        assert reached[2]==stored[2] and int(reached[1])>int(stored[1]), arrow
        assert "move=plant event=hit" not in arrow and "move=wire-release event=rewind-completed" in arrow, arrow
        wrong_variant = launch("gg4e08:0615", "test:target", "--borg-variant", "bronze", expected=2, frames="1")
        assert "exact base variant" in wrong_variant, wrong_variant
        source_duel = launch("gg4e08:0615", "gg4e08:000a", expected=2, frames="1")
        assert "against an explicit stationary package" in source_duel, source_duel
        roster = launch("gg4e08:000a", "gg4e08:000a", "--demo-roster-battle", expected=0, frames="60")
        assert "actors=4 restarts=0 moved=1 airborne=1 shots=2 hits=2" in roster, roster
        roster_source = launch("gg4e08:0615", "gg4e08:000a", "--demo-roster-battle", expected=0)
        assert "actors=4 restarts=1 moved=1 airborne=1 shots=5 hits=4" in roster_source, roster_source
    print("Package actors, guidance, impacts, synchronized emitters, stock and restart PASS"
          f" ({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--art", type=Path, default=Path("local/content/native-battle-mvp-v1"))
    parser.add_argument("--movement", type=Path, default=Path("local/content/source-gred-ground-v9"))
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.executable, args.art, args.movement, args.opengl)
