"""Audit normal Borgs in mixed Workshop rosters using the installed native host."""
from argparse import ArgumentParser
import json
from pathlib import Path
import re
from subprocess import TimeoutExpired, run
import sys
from tempfile import TemporaryDirectory


ANCHOR = "example:flight-stick"
STAGE = "example:workshop"
BASE_ID = re.compile(r"[a-z0-9][a-z0-9._-]*:[a-z0-9][a-z0-9._-]*")
PAYLOAD_EVENTS = frozenset(("emit", "extra-emit", "melee-armed", "field-placed", "blast-triggered",
                            "emitter-fired", "walk-pulse-created"))


def fixtures(ids):
    """Place subjects in stable slot order. P1 is an idle authored control."""
    slots = [(0, ANCHOR, "west")]
    marks = ((0, "west-outer"), (0, "west-rear"), (1, "east"),
             (1, "east-outer"), (1, "east-rear"))
    if len(ids) < 3:
        marks = ((1, "east"), (1, "east-outer"))
    slots.extend((team, borg, spawn) for borg, (team, spawn) in zip(ids, marks))
    if len(ids) == 1:
        slots.append((0, ANCHOR, "west-outer"))
    return slots


def first_damaging_enemy_hits(trace, slots):
    """Return the first positive native hit or blast hit tick per attacking slot."""
    hits = {}
    for row in re.finditer(r"borg-api: round=0 tick=(\d+) actor=(\d+) move=\S+ event=(?:hit|blast-hit)"
                           r" frame=\d+ object=\d+ recipient=(\d+) value=(\d+)(?=\s|$)", trace):
        tick, actor, recipient, value = map(int, row.groups())
        if value and actor < len(slots) and recipient < len(slots) and slots[actor][0] != slots[recipient][0]:
            hits[actor] = min(tick, hits.get(actor, tick))
    return hits


def damaging_enemy_slots(trace, slots):
    return set(first_damaging_enemy_hits(trace, slots))


def first_action_effects(trace, slot_count, accepted_ticks):
    """Record observed effects, not inferred misses or hit causes."""
    payloads, impacts, interrupts = {}, {}, {}
    for t, s, kind, object_id in re.findall(r"borg-api: round=0 tick=(\d+) actor=(\d+) move=\S+ event=(\S+)"
                                             r" frame=\d+ object=(\d+) ", trace):
        if kind not in PAYLOAD_EVENTS and kind not in ("stage-impact", "interrupt"):
            continue
        tick, slot = int(t), int(s)
        if not 1 <= tick <= accepted_ticks or slot >= slot_count:
            raise ValueError("reported action effect outside accepted roster ticks or slots")
        if kind in PAYLOAD_EVENTS:
            if int(object_id) and (slot not in payloads or tick < payloads[slot][0]):
                payloads[slot] = tick, kind
        elif kind == "stage-impact":
            impacts[slot] = min(tick, impacts.get(slot, tick))
        else:
            interrupts[slot] = min(tick, interrupts.get(slot, tick))
    return payloads, impacts, interrupts


def round_activity(trace, slots, budget):
    # Audit presets have no timer, pause, or restart. A round that has not
    # ended must account for the whole requested render frame budget.
    summary = re.findall(r"borg-roster-round: round=0 tick=(\d+) result=(fighting|team0|team1|draw)"
                         r" alive=0x([0-9a-f]+)$", trace, re.M)
    if len(summary) != 1:
        raise ValueError("missing or duplicate final roster trace; use a host with round tracing")
    ticks = int(summary[0][0])
    result = summary[0][1]
    alive = int(summary[0][2], 16)
    full = (1 << len(slots)) - 1
    if not 1 <= ticks <= budget or (ticks != budget and result == "fighting") or alive > full:
        raise ValueError("invalid accepted tick count or living-slot mask")
    sides = {team for i, (team, _, _) in enumerate(slots) if alive & (1 << i)}
    expected_result = "fighting" if len(sides) == 2 else f"team{min(sides)}" if sides else "draw"
    if result != expected_result:
        raise ValueError("untimed roster result disagrees with living teams")

    ko = {}
    for t, s in re.findall(r"borg-roster-ko: round=0 tick=(\d+) actor=(\d+)$", trace, re.M):
        tick, slot = int(t), int(s)
        if slot in ko or slot >= len(slots) or not 1 <= tick <= ticks:
            raise ValueError("invalid or duplicate roster KO trace")
        ko[slot] = tick
    if sum(1 << slot for slot in ko) != (full ^ alive):
        raise ValueError("KO trace does not account for all defeated slots")
    if result != "fighting" and max(ko.values(), default=0) != ticks:
        raise ValueError("untimed round did not end on its last KO")

    starts = {}
    for t, s in re.findall(r"borg-api: round=0 tick=(\d+) actor=(\d+) move=\S+ event=start ", trace):
        tick, slot = int(t), int(s)
        if slot >= len(slots) or not 1 <= tick <= ko.get(slot, ticks):
            raise ValueError("invalid roster move-start tick or slot")
        starts[slot] = min(starts.get(slot, tick), tick)
    hits = first_damaging_enemy_hits(trace, slots)
    if any(not 1 <= tick <= ticks for tick in hits.values()):
        raise ValueError("reported hit outside accepted roster ticks")
    return ticks, result, starts, hits, ko


def checkpoint_totals(rows, checkpoint):
    summary = dict(tick=checkpoint, groups_ended=0, subjects_with_round_at_tick=0, cpu_move_started=0, cpu_enemy_hit=0,
                   quiet_defeated=0, quiet_round_ended=0, quiet_fighting=0, cpu_payload_seen=0,
                   started_no_hit_with_payload=0, started_no_hit_no_payload=0,
                   started_no_hit_stage_impact=0, started_no_hit_interrupted=0)

    def observed(tick):
        return tick is not None and tick <= checkpoint

    for row in rows:
        if not row["ran"]:
            continue
        ended = row["result"] != "fighting" and row["accepted_ticks"] <= checkpoint
        summary["groups_ended"] += ended
        if row["accepted_ticks"] >= checkpoint:
            summary["subjects_with_round_at_tick"] += len(row["subjects"])
        for entry in row["activity"]:
            started, hit, ko = (entry[key] for key in ("first_move_tick", "first_enemy_hit_tick", "ko_tick"))
            payload_seen = observed(entry.get("first_payload_tick"))
            damaged = observed(hit)
            summary["cpu_payload_seen"] += payload_seen
            summary["cpu_enemy_hit"] += damaged
            if observed(started):
                summary["cpu_move_started"] += 1
                if not damaged:
                    summary["started_no_hit_with_payload" if payload_seen else "started_no_hit_no_payload"] += 1
                    summary["started_no_hit_stage_impact"] += observed(entry.get("first_stage_impact_tick"))
                    summary["started_no_hit_interrupted"] += observed(entry.get("first_interrupt_tick"))
            elif observed(ko):
                summary["quiet_defeated"] += 1
            elif ended:
                summary["quiet_round_ended"] += 1
            else:
                summary["quiet_fighting"] += 1
    return summary


def audit(binary, content, namespace, expected, ticks=120, checkpoints=()):
    binary = binary.resolve(strict=True)
    content = content.resolve(strict=True)
    if not BASE_ID.fullmatch(namespace + ":a") or not 1 <= expected <= 2048 or not 1 <= ticks <= 600:
        raise ValueError("invalid namespace, expected count (1–2048), or tick budget (1–600)")
    checkpoints = sorted(set(checkpoints) | {ticks})
    if any(not 1 <= checkpoint <= ticks for checkpoint in checkpoints):
        raise ValueError("each checkpoint must be within the frame budget")

    def host(*args):
        return run([binary, *args], cwd=content, capture_output=True, text=True, timeout=45)

    listing = host("--list-borgs")
    if listing.returncode or listing.stderr.strip():
        raise ValueError("cannot audit an incomplete installed Borg catalogue: " + listing.stderr.strip())
    listed = [line.partition("\t")[0] for line in listing.stdout.splitlines()]
    if any(len(borg) > 96 or not BASE_ID.fullmatch(borg) for borg in listed) or len(set(listed)) != len(listed):
        raise ValueError("native listing contains an invalid or duplicate ID")
    subjects = sorted(borg for borg in listed if borg.startswith(namespace + ":"))
    if len(subjects) != expected or ANCHOR not in listed:
        raise ValueError(f"expected {expected} {namespace} Borgs plus the authored Flight Stick; found {len(subjects)} subjects")

    rows = []
    with TemporaryDirectory(prefix="gf-native-roster-matrix-") as scratch:
        for i in range(0, len(subjects), 5):
            ids = subjects[i:i + 5]
            slots = fixtures(ids)
            preset = Path(scratch) / f"group-{i // 5:03}.roster"
            with preset.open("x", encoding="ascii") as out:
                out.write(f"GFROSTER 1\nlocals=1\nstage={STAGE}\n")
                out.writelines(f"fighter={team}|{borg}|normal|{spawn}\n" for team, borg, spawn in slots)
            drivers = [(i, team, "local" if i == 0 else "cpu", borg) for i, (team, borg, _) in enumerate(slots)]
            row = {"subjects": ids, "preflight": False, "ran": False}
            rows.append(row)
            check = host("--check-battle-roster", preset)
            header = f"Roster admission passed: stage={STAGE} slots={len(slots)} locals=1 ticks=0 (headless)"
            checked = re.findall(
                r"^slot=(\d+) team=(\d+) driver=(\w+) id=(\S+) variant=normal spawn=(\S+) "
                r"position=\S+ body-draws=([1-9]\d*)$", check.stdout, re.M)
            if (check.returncode or header not in check.stdout.splitlines()
                    or [(int(n), int(t), d, b, s) for n, t, d, b, s, _ in checked]
                    != [(*driver, slot[2]) for driver, slot in zip(drivers, slots)]):
                row["error"] = "zero-tick preflight did not admit all exact slots"
                if check.stderr:
                    print(check.stderr.strip().splitlines()[-1], file=sys.stderr)
                continue
            row["preflight"] = True
            game = host("--headless", "--no-audio", "--frames", str(ticks),
                        "--trace-package-battle", "--battle-roster", preset)
            actual = re.findall(r"borg-roster-slot: actor=(\d+) team=(\d+) driver=(\w+) id=(\S+)", game.stderr)
            finished = f"package-battle: frames={ticks} actors={len(slots)} restarts=0 "
            if game.returncode or finished not in game.stderr or [(int(n), int(t), d, b) for n, t, d, b in actual] != drivers:
                row["error"] = "bounded run did not retain all exact slots"
                if game.stderr:
                    print(game.stderr.strip().splitlines()[-1], file=sys.stderr)
                continue
            try:
                accepted, result, starts, hits, ko = round_activity(game.stderr, slots, ticks)
                payloads, impacts, interrupts = first_action_effects(game.stderr, len(slots), accepted)
            except ValueError as error:
                row["error"] = str(error)
                print(f"{preset.name}: {error}", file=sys.stderr)
                continue
            row.update(ran=True, accepted_ticks=accepted, result=result)
            row["cpu_move_started"] = [borg for n, borg in enumerate(ids, 1) if n in starts]
            row["cpu_enemy_hit"] = [borg for n, borg in enumerate(ids, 1) if n in hits]
            row["activity"] = [
                dict(id=borg, first_move_tick=starts.get(n), first_enemy_hit_tick=hits.get(n), ko_tick=ko.get(n),
                     first_payload_tick=payloads.get(n, (None, None))[0], first_payload_kind=payloads.get(n, (None, None))[1],
                     first_stage_impact_tick=impacts.get(n), first_interrupt_tick=interrupts.get(n))
                for n, borg in enumerate(ids, 1)]

    return {
        "scope": "native headless Workshop; normal variants, one idle local, subjects as CPUs",
        "namespace": namespace, "frames": ticks, "stage": STAGE, "subjects": expected,
        "preflight": sum(len(row["subjects"]) for row in rows if row["preflight"]),
        "ran": sum(len(row["subjects"]) for row in rows if row["ran"]),
        "cpu_move_started": sum(len(row.get("cpu_move_started", ())) for row in rows),
        "cpu_enemy_hit": sum(len(row.get("cpu_enemy_hit", ())) for row in rows),
        "checkpoints": [checkpoint_totals(rows, checkpoint) for checkpoint in checkpoints],
        "groups": rows,
    }


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("content", type=Path, help="installed borgs/ and Workshop stages/ root, read only")
    parser.add_argument("--namespace", default="gg4e08")
    parser.add_argument("--expect", type=int, default=206)
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--checkpoint", type=int, action="append", default=[],
                        help="also report activity through this tick in the same run (repeatable)")
    args = parser.parse_args()
    try:
        report = audit(args.binary, args.content, args.namespace, args.expect, args.frames, args.checkpoint)
    except (OSError, TimeoutExpired, ValueError) as error:
        parser.exit(2, f"Roster matrix: {error}\n")
    print(json.dumps(report, indent=2))
    sys.exit(0 if report["ran"] == args.expect else 2)
