"""Explain interrupted, no-hit subjects from a recorded native Workshop audit."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.analyze import native_roster_matrix as matrix


EVENT = re.compile(r"borg-api: round=0 tick=(\d+) actor=(\d+) move=(\S+) event=(\S+) "
                   r"frame=(\d+) object=(\d+) recipient=(\d+) value=(\d+)")


def rows(trace):
    return [
        (int(tick), int(actor), move, event, int(frame), int(obj), int(recipient), int(value))
        for tick, actor, move, event, frame, obj, recipient, value in EVENT.findall(trace)
    ]


def classify(trace, slot, first_interrupt, melee_windows):
    events = rows(trace)
    interrupt, = (e for e in events if (e[0], e[1], e[3]) == (first_interrupt, slot, "interrupt"))
    tick, _, move, _, frame, *_ = interrupt
    starts = [e[0] for e in events if e[1:4] == (slot, move, "start") and e[0] <= tick]
    if not starts:
        raise ValueError("interrupt has no corresponding native move start")
    start = max(starts)
    hits = [
        {"update": e[0], "actor_slot": e[1], "move": e[2], "kind": e[3], "hp_removed": e[7]}
        for e in events if e[3] in ("hit", "blast-hit") and e[6] == slot and e[7] and start <= e[0] == tick-1
    ]
    payloads = [{"update": e[0], "kind": e[3]} for e in events
                if e[1:3] == (slot, move) and e[3] in matrix.PAYLOAD_EVENTS and start <= e[0] <= tick]
    first = melee_windows.get(move)
    if not hits:
        verdict = "unresolved"
    elif payloads:
        verdict = "authored-hit-after-payload"
    elif first is not None and frame < first:
        verdict = "authored-hit-before-melee-window"
    else:
        verdict = "unresolved"
    return {
        "move": move, "start_update": start, "interrupt_update": tick, "frame_at_interrupt": frame,
        "authored_melee_first_frame": first, "preceding_accepted_hits": hits, "payloads_before_interrupt": payloads,
        "classification": verdict,
    }


def read_matrix(path):
    with Path(path).open("rb") as stream:
        data = stream.read((1 << 20)+1)
    if len(data) > 1 << 20:
        raise ValueError("Workshop audit byte budget")
    report = json.loads(data)
    if (report["stage"], report["namespace"], report["subjects"], report["ran"], report["frames"]) != (
            matrix.STAGE, "gg4e08", 206, 206, 240) or len(report["groups"]) != 42:
        raise ValueError("expected the 206-subject, 240-update native Workshop audit")
    return report


def audit(binary, content, report):
    binary = Path(binary).resolve(strict=True)
    content = Path(content).resolve(strict=True)
    cases = []
    with TemporaryDirectory(prefix="gf-workshop-interruption-") as t:
        for index, group in enumerate(report["groups"]):
            ids = group["subjects"]
            if (not group["ran"] or not 1 <= len(ids) <= 5
                    or any(not re.fullmatch(r"gg4e08:[0-9a-f]{4}", g) for g in ids)):
                raise ValueError("invalid exact subject group")
            candidates = [(slot, e) for slot, e in enumerate(group["activity"], 1)
                          if e["first_move_tick"] and e["first_interrupt_tick"] and not e["first_enemy_hit_tick"]]
            if not candidates:
                continue
            slots = matrix.fixtures(ids)
            preset = Path(t)/f"group-{index}.roster"
            with preset.open("x", encoding="ascii") as f:
                f.write(f"GFROSTER 1\nlocals=1\nstage={matrix.STAGE}\n")
                f.writelines(f"fighter={team}|{borg}|normal|{spawn}\n" for team, borg, spawn in slots)
            result = run([binary, "--headless", "--no-audio", "--frames", "240",
                          "--trace-package-battle", "--battle-roster", preset],
                         cwd=content, capture_output=True, text=True, timeout=45)
            if result.returncode:
                raise ValueError(f"Workshop replay failed: {result.stderr[-4000:]}")
            accepted, _, starts, hits, _ = matrix.round_activity(result.stderr, slots, 240)
            observed, _, interrupts = matrix.first_action_effects(result.stderr, len(slots), accepted)
            for slot, e in candidates:
                if (slot > len(ids) or e["id"] != slots[slot][1] or starts.get(slot) != e["first_move_tick"]
                        or interrupts.get(slot) != e["first_interrupt_tick"] or slot in hits
                        or observed.get(slot, (None,))[0] != e["first_payload_tick"]):
                    raise ValueError("Workshop subject replay no longer matches the audited matrix")
                package = content/"borgs"/e["id"].replace(":","-")
                with (package/"gameplay.txt").open("r", encoding="ascii") as f:
                    profile = f.read((64 << 10)+1)
                if len(profile) > 64 << 10:
                    raise ValueError("Workshop profile byte budget")
                windows = {m: int(fr) for m, fr in re.findall(r"^melee=([^|]+)\|(\d+)\|", profile, re.M)}
                fields = classify(result.stderr, slot, e["first_interrupt_tick"], windows)
                cases.append({"borg": e["id"], "seat": slot, "group_subjects": ids,
                              "first_payload_update": e["first_payload_tick"], **fields})
    return {
        "kind": "gf-native-workshop-interruptions", "revision": 1, "scope": "authored native Workshop, not original behavior",
        "subjects": report["subjects"], "updates": report["frames"], "checkpoints": report["checkpoints"],
        "classifications": dict(Counter(r["classification"] for r in cases)), "cases": cases,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("binary", "content", "matrix", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    if args.output.exists() or args.output.is_symlink():
        raise FileExistsError("Workshop interruption output already exists")
    report = audit(args.binary, args.content, read_matrix(args.matrix))
    with args.output.open("x", encoding="ascii") as f:
        f.write(json.dumps(report, sort_keys=True, indent=2)+"\n")
