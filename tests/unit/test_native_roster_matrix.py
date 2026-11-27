import unittest

from tools.analyze.native_roster_matrix import (
    checkpoint_totals, damaging_enemy_slots, first_action_effects, fixtures, round_activity,
)


class NativeRosterAuditTests(unittest.TestCase):
    def test_hits_require_damage_and_an_actual_enemy(self):
        slots = fixtures([f"sample:{index}" for index in range(5)])

        def event(actor, kind, recipient, value, round_no=0):
            return f"[info] borg-api: round={round_no} tick=9 actor={actor} move=move event={kind} " \
                   f"frame=8 object=1 recipient={recipient} value={value}\n"

        log = (
            event(1, "hit", 3, 7)
            + event(1, "hit", 4, 2)  # already counted this slot
            + event(2, "hit", 0, 5)  # ally
            + event(2, "blast-hit", 3, 0)
            + event(2, "field-placed", 3, 9)
            + event(3, "blast-hit", 1, 6)
            + event(4, "hit", 255, 5)
            + event(5, "hit", 1, 4, round_no=1)
            + event(7, "hit", 1, 2)
        )
        self.assertEqual(damaging_enemy_slots(log, slots), {1, 3})

    def test_round_checkpoint_distinguishes_late_ko_and_ended_round(self):
        ids = [f"sample:{index}" for index in range(5)]
        slots = fixtures(ids)
        trace = (
            "[info] borg-api: round=0 tick=3 actor=1 move=b event=start frame=0 object=0 recipient=255 value=0\n"
            "[info] borg-api: round=0 tick=7 actor=1 move=b event=emit frame=4 object=7 recipient=255 value=1\n"
            "[info] borg-api: round=0 tick=8 actor=1 move=b event=hit frame=4 object=7 recipient=3 value=12\n"
            "[info] borg-roster-ko: round=0 tick=8 actor=3\n"
            "[info] borg-api: round=0 tick=35 actor=4 move=x event=start frame=0 object=0 recipient=255 value=0\n"
            "[info] borg-api: round=0 tick=37 actor=4 move=x event=emit frame=2 object=8 recipient=255 value=1\n"
            "[info] borg-api: round=0 tick=38 actor=4 move=x event=stage-impact frame=3 object=8 recipient=255 value=1\n"
            "[info] borg-api: round=0 tick=39 actor=4 move=x event=interrupt frame=4 object=0 recipient=255 value=0\n"
            "[info] borg-roster-ko: round=0 tick=40 actor=4\n"
            "[info] borg-roster-ko: round=0 tick=40 actor=5\n"
            "[info] borg-roster-round: round=0 tick=40 result=team0 alive=0x7\n"
        )
        accepted, result, starts, hits, ko = round_activity(trace, slots, 120)
        self.assertEqual((accepted, result, starts, hits, ko), (40, "team0", {1: 3, 4: 35}, {1: 8}, {3: 8, 4: 40, 5: 40}))
        payloads, impacts, interrupts = first_action_effects(trace, len(slots), accepted)
        row = dict(ran=True, result=result, accepted_ticks=accepted, subjects=ids,
                   activity=[dict(first_move_tick=starts.get(n), first_enemy_hit_tick=hits.get(n), ko_tick=ko.get(n),
                                  first_payload_tick=payloads.get(n, (None, None))[0], first_stage_impact_tick=impacts.get(n),
                                  first_interrupt_tick=interrupts.get(n))
                             for n in range(1, 6)])
        self.assertEqual(checkpoint_totals([row], 20), dict(tick=20, groups_ended=0, subjects_with_round_at_tick=5,
                         cpu_move_started=1, cpu_enemy_hit=1, quiet_defeated=1, quiet_round_ended=0, quiet_fighting=3,
                         cpu_payload_seen=1, started_no_hit_with_payload=0, started_no_hit_no_payload=0,
                         started_no_hit_stage_impact=0, started_no_hit_interrupted=0))
        self.assertEqual(checkpoint_totals([row], 120), dict(tick=120, groups_ended=1, subjects_with_round_at_tick=0,
                         cpu_move_started=2, cpu_enemy_hit=1, quiet_defeated=2, quiet_round_ended=1, quiet_fighting=0,
                         cpu_payload_seen=2, started_no_hit_with_payload=1, started_no_hit_no_payload=0,
                         started_no_hit_stage_impact=1, started_no_hit_interrupted=1))

    def test_only_successful_payload_events_count(self):
        def event(tick, slot, kind, obj, round_no=0):
            return (f"borg-api: round={round_no} tick={tick} actor={slot} move=b event={kind} "
                    f"frame=0 object={obj} recipient=255 value=0\n")

        trace = (event(2, 0, "start", 0) + event(3, 0, "stock-refused", 0) + event(4, 0, "emit-refused", 0)
                 + event(5, 0, "emit", 0) + event(8, 1, "melee-armed", 10) + event(10, 1, "field-placed", 11)
                 + event(9, 2, "walk-pulse-created", 12) + event(1, 0, "emit", 1, round_no=1))
        self.assertEqual(first_action_effects(trace, 3, 20), ({1: (8, "melee-armed"), 2: (9, "walk-pulse-created")}, {}, {}))
        for invalid in (event(21, 1, "emit", 1), event(8, 3, "stage-impact", 1)):
            with self.assertRaises(ValueError):
                first_action_effects(trace + invalid, 3, 20)

    def test_bad_round_trace_refused(self):
        slots = fixtures(["sample:1", "sample:2"])
        end = "[info] borg-roster-round: round=0 tick=12 result=team1 alive=0x6\n"
        ko = "[info] borg-roster-ko: round=0 tick=12 actor=0\n"
        for trace in ("", end, ko + end + end, ko + end.replace("team1", "team0"),
                      ko + end.replace("tick=12", "tick=14"), ko + ko + end,
                      "[info] borg-roster-round: round=0 tick=15 result=fighting alive=0x7\n"):
            with self.subTest(trace=trace):
                with self.assertRaises(ValueError):
                    round_activity(trace, slots, 30)


if __name__ == "__main__":
    unittest.main()
