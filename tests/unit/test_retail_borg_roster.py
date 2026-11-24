import struct
import unittest
import wave
from hashlib import sha256
from io import BytesIO
from pathlib import Path
from unittest.mock import patch

from tools.analyze import retail_action_links
from tools.analyze import retail_dragon_shot as dragon
from tools.analyze import retail_dragon_carrier as carrier
from tools.analyze import retail_dragon_art as dragon_art
from tools.analyze import retail_dragon_area as dragon_area
from tools.analyze import retail_worm_shot as worm
from tools.analyze import retail_battle_reflection as reflection
from tools.analyze import retail_tank_shot as tanks
from tools.analyze import retail_paired_ground_shot as ground_pairs
from tools.analyze import retail_single_ground_shot as single_shots
from tools.analyze import retail_breath_dragon as breath
from tools.analyze import retail_roach_bomb as roach_bomb
from tools.analyze import retail_cannon_beam as cannon_beam
from tools.analyze import retail_knight_melee as knight
from tools.analyze import retail_samurai_melee as samurai
from tools.analyze import retail_gatling_held_fire as gatling
from tools.analyze import retail_equipment_visuals as equipment
from tools.analyze.retail_action_links import ActionLinker, entry_phase_increment, immediate_argument, literal_flow, release_model_gate
from tools.analyze.retail_action_sfx import action_sfx_index, review_requests
from tools.analyze.retail_action_tables import action_candidates, decode_tables, installer_tables
from tools.analyze.retail_actor_data_binding import actor_record_binding, interpret_installer
from tools.analyze.retail_basic_shot_test import (
    basic_shot_test, bind_actor_release_sound, cruise_charge_shot_test, dragon_battle_test, match_shot_context,
    pilot_launch_tone, pose_free_shot_test, whole_body_pose, worm_battle_test,
)
from tools.analyze.retail_release_phases import VIOLET_SHOT
from tools.analyze.retail_ground_pace import ground_pace_candidate
from tools.analyze.retail_action_previews import PREVIEW_GROUPS, action_preview_candidates
from tools.analyze.retail_movement_tables import BETA_INSTALLER, BETA_MODEL_CHECK, BETA_OVERRIDE, literal_at_store, movement_config_candidates
from tools.analyze.retail_movement_operands import ACTOR_BYTES, FIELDS, METER_READS, ROLE_READS, movement_operand_profile, movement_operand_schema, operand_records
from tools.analyze import retail_motion_roles
from tools.analyze.retail_motion_roles import (
    AIR_DASH_PHASES, DASH_DIRECTIONS, HANDLERS, air_dash_subranges, carry_ground_travel,
    dragon_takeoff_clip, link_ground_dash, motion_roles,
)
from tools.analyze.retail_projectile_origins import TABLES, origin_candidates, origin_tables, preview_origin_markers, release_index
from tools.analyze.retail_projectile_sfx import BANK_TABLE, PHASE_ZERO, attach_release_sfx, projectile_sound_index, tone_candidate
from tools.analyze.retail_program_controls import OPCODE_ROW, WRITER, ProgramControlIndex, control_write_schema
from tools.analyze.retail_program_controls import reviewed_joins
from tools.analyze.retail_stock_operands import DEBIT, StockGuardIndex, attach_release_stock, stock_rows, stock_schema
from tools.analyze.retail_seeded_shot_pose import (
    DOUBLE_NINJA_ROUTE, ROUTES as RIFLE_ROUTES, THETA_ROUTE, THROW_ROUTES, rifle_shot_pose, theta_shot_pose, throw_shot_pose,
)
from tools.analyze.retail_side_shot import ROUTES as SIDE_ROUTES, neutral_side_shot
from tools.analyze.retail_shot_readiness import LOCOMOTION, shot_readiness
from tools.extract.retail_shot_pcm import launch_pcm
from tools.extract.retail_borg_roster import (
    bank_clips, clip_timing, cruise_entry_candidate, cruise_lab_candidate, decode_lab_landing_pcm,
    displayed_moves, landing_sound, logical_joints, move_profile, source_parts,
    neutral_preview_mounts, preview_text, role_preview_candidates, validate_preview_mounts,
    variant_preview_files, wrapper_model,
)


def put(data, at, value):
    struct.pack_into(">I", data, at, value)


class RosterFormatTests(unittest.TestCase):
    def test_ground_pair_refuses_reordered_sockets_and_extra_debit(self):
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(32)]
        for model,keys in ((0x0619,(79,80)),(0x061F,(87,88)),(0x0625,(91,92))):
            route=ground_pairs.ROUTES[model]
            source={"model":f"{model:04X}","action_group":"B-shot","debit":ground_pairs.reviewed_debit(route),
                    "ordered_calls":ground_pairs.reviewed_calls(route)}
            roles=[("ground-stop",2,2),("ground-travel",2,route.walk_choice),
                   ("air-continuation",2,14),("ground-dash-forward",2,4)]
            bank,tone=route.tone
            sounds={route.family:{key:{"status":"static-tone-record","bank":bank,"tone":tone} for key in keys}}
            def export():
                return ground_pairs.battle_test(model,joints,roles,{"native_units_per_tick_x100":500},
                                                [400,5,5,0,180],source,sounds,launch_audio=True)
            rules,_,receipt=export()
            self.assertEqual([c["key"] for c in receipt["source_factory_calls"]],list(keys))
            self.assertEqual(rules.count(b"\nrelease="),1)
            self.assertIn(b"release=basic-shot|12|shot-stock|1|shot\nrelease-followup=basic-shot|12|shot-second\n",rules)
            self.assertEqual(rules.count(f"|audio/launch-{bank}-{tone}.pcm\n".encode()),2)
            source["ordered_calls"].reverse()
            with self.assertRaisesRegex(ValueError,"ordered sockets"):
                export()
            source["ordered_calls"].reverse()
            source["debit"]["cost"]=2
            with self.assertRaisesRegex(ValueError,"one same-actor debit"):
                export()
            source["debit"]["cost"]=1
            sounds[route.family][keys[1]]["tone"]=tone+1
            with self.assertRaisesRegex(ValueError,"exact launch tone"):
                export()

    def test_tank_pilot_uses_paid_firing_not_melee_or_a_borrowed_barrel(self):
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(12)]
        roles=[("ground-stop",0,25),("ground-travel",0,26),("air-continuation",0,24),("ground-dash-forward",0,2)]
        for model,key,bone in ((0x0C00,3,7),(0x0C01,7,11),(0x0C05,9,6),(0x0C06,3,7)):
            source={"model":f"{model:04X}","muzzle":{"key":key,"joint":bone,"point":tanks.MUZZLES[key][1]},
                    "debit":{"site":f"{tanks.ROUTES[model][-3]:08X}","channel":0,"cost":1,"commit":True,"same_input_actor":True}}
            rules,_,receipt=tanks.battle_test(model,joints,roles,{"native_units_per_tick_x100":333},
                                              [300,20,20,0,60],source,{"status":"static-tone-record","bank":"snd_com01","tone":6},
                                              launch_audio=True)
            self.assertIn(f"origin=shot|{bone}|".encode(),rules)
            self.assertIn(b"move=basic-shot|b|10|grounded|24|none\n",rules)
            self.assertIn(b"release=basic-shot|8|shot-stock|1|shot\n",rules)
            self.assertIn(b"launch-sound=shot|audio/launch-snd_com01-6.pcm\n",rules)
            self.assertNotIn(b"melee=",rules)
            self.assertEqual(receipt["source_normal_factory_key"],key)
            self.assertEqual(receipt["preview_clip"],"none")
            source["muzzle"]["joint"]=0
            with self.assertRaisesRegex(ValueError,"exact muzzle"):
                tanks.battle_test(model,joints,roles,{"native_units_per_tick_x100":333},[300,20,20,0,60],source,{})

    def test_breath_pilot_needs_its_paid_window_pose_and_beam(self):
        joints=[{"index":i,"descriptor_offset":i*4,"local_translation":[0,0,0],
                 "child_next_parent":[255,255,i-1]} for i in range(32)]
        mapping={"profiles":[{"movement_type":4,"roles":[{"selector":0,"states":[1,1],
            "handler_addresses":["8005F23C","8005F2A4"],
            "program_candidates":[{"site":"8005F2F8","lanes":2,"program_group":0,"program_choice":0,
                "clip_candidates":[{"preceding_waits":0,"start_frame":0,
                    "lanes":[{"lane":1,"bank":"local","archive_offset":0,"group":0,"choice":0}]}]}]}]}]}
        roles=[("ground-travel",0,1),("ground-takeoff",0,2),("landing",0,5),("air-continuation",0,7)]
        source={"model":"0503","action_group":"B-shot","pose":[0,1],
                "debit":{"site":"800B93AC","channel":0,"cost":1,"commit":True,"same_input_actor":True},
                "program_driver":{"site":"800B91D4","helper":"8006D144","id":0x81},
                "origin_evidence":"none"}
        def export(src=source):
            return breath.battle_test(0x0503,joints,roles,mapping,{"native_units_per_tick_x100":375},
                                      [2500,5,5,0,300],src,chain=None)
        rules,_,receipt=export()
        self.assertIn(b"move=breath-test|b|10|grounded|48|0:1\n",rules)
        self.assertIn(b"projectile=breath|beam|1|1|0|0|0|60|30|6|ignore-floor\n",rules)
        self.assertIn(b"sustained-beam=breath|1500|8|0\n",rules)
        self.assertIn(b"look=breath|160|255|150|60\n",rules)
        self.assertIn(b"origin=breath|0|0|40|60|target-height\n",rules)
        self.assertIn(b"release=breath-test|12|breath-stock|1|breath\n",rules)
        self.assertNotIn(b"breath-dive",rules)  # 0503 has no mechanical dive
        # The six non-mechanical Dragons also carry the authored airborne sweep.
        self.assertIn(b"projectile=sweep|beam|1|1|0|0|0|60|40|8|clip-on-stage\n",rules)
        self.assertIn(b"sustained-beam=sweep|2400|6|0\n",rules)
        self.assertIn(b"beam-sweep=sweep|1200|300|180\n",rules)
        self.assertIn(b"origin=sweep|0|0|40|60|fixed\n",rules)
        self.assertIn(b"move=breath-sweep|b|11|airborne|60|none\n",rules)
        self.assertIn(b"release=breath-sweep|8|breath-stock|1|sweep\n",rules)
        self.assertEqual(receipt["native_sweep"]["origin_joint"],0)
        self.assertEqual(receipt["native_sweep"]["floor"],"clip-on-stage")
        self.assertIn(b"activation=breath-test|release-window|24|90\n",rules)
        self.assertIn(b"dash-policy=disabled\n",rules)
        self.assertNotIn(b"field=",rules)
        self.assertEqual(receipt["source_program_driver"]["id"],0x81)
        self.assertEqual(receipt["native_pace"],750)
        self.assertEqual(receipt["native_beam"]["length"],1500)
        # A borrowed or unrequested pose is refused, and so is a wrong debit.
        with self.assertRaisesRegex(ValueError,"paid debit|exact pose"):
            export(dict(source,pose=[0,7]))
        with self.assertRaisesRegex(ValueError,"exact paid debit"):
            export(dict(source,debit=dict(source["debit"],channel=2)))
        # A model with no neutral B clip uses a pose-free action instead.
        pose_free_source=dict(source,model="0504",pose=None,
                              debit=dict(source["debit"],site=f"{breath.COHORTS[0x0504][1]:08X}"))
        pose_free,_,_=breath.battle_test(0x0504,joints,roles,mapping,{"native_units_per_tick_x100":271},
                                         [1300,5,5,1,300],pose_free_source,chain=None)
        self.assertIn(b"move=breath-test|b|10|grounded|48|none\n",pose_free)
        # The two mechanical Dragons add an airborne downward beam from the chest.
        dive_source=dict(source,model="0506",pose=None,
                         debit=dict(source["debit"],site=f"{breath.COHORTS[0x0506][1]:08X}"))
        dive_rules,_,dive_receipt=breath.battle_test(0x0506,joints,roles,mapping,
                                                    {"native_units_per_tick_x100":375},[2000,5,5,0,300],
                                                    dive_source,chain=None)
        self.assertIn(b"projectile=dive|beam|1|30|-100|0|0|60|25|40|stop-on-floor\n",dive_rules)
        self.assertIn(b"origin=dive|12|0|0|0|fixed\n",dive_rules)
        self.assertIn(b"move=breath-dive|b|11|airborne|40|none\n",dive_rules)
        self.assertIn(b"release=breath-dive|8|breath-stock|1|dive\n",dive_rules)
        self.assertNotIn(b"breath-sweep",dive_rules)
        self.assertEqual(dive_receipt["native_dive"]["origin_joint"],12)

    def test_roach_reimplements_the_placed_area_as_a_native_bomb(self):
        joints=[{"index":0,"descriptor_offset":0},{"index":1,"descriptor_offset":4}]
        row={"state":0,"substate":0,"selector":45,"handler_pair":[61,0],
             "class_callback_candidate":"801E7F28"}
        actions={"profiles":[{"config":"803929C8","context_table":"80392BA0","main_handler":"801E7EF0",
                              "program_and_descriptor_roots":["802BD54C","802BD65E","00000000","00000000"],
                              "groups":[{"group":"B-shot","rows":[row]}]}]}
        rules,pose,receipt=roach_bomb.battle_test(joints,actions,[60])
        for line in ("body-motion=static-grounded","stock=mine-stock|3|on-return",
                     "projectile=mine|bullet|1|1|0|1|0|16|70|0|land-on-floor",
                     "blast=mine|expiry|310|12|opponent","bomb-proximity=mine|150|30|blast",
                     "return-stock=mine|expiry-or-proximity","move=mine|b|10|grounded|28|none",
                     "release=mine|12|mine-stock|1|mine"):
            self.assertIn((line+"\n").encode(),rules)
        self.assertEqual(len(pose),44)
        self.assertEqual(receipt["source_area"]["blast_radius"],310)
        self.assertEqual(receipt["source_area"]["stock_cap"],3)
        self.assertEqual(receipt["source_debit"]["site"],"8006DBE0")
        self.assertFalse(receipt["retail_behavior_verified"])
        bad=dict(row,selector=44)
        actions["profiles"][0]["groups"][0]["rows"]=[bad]
        with self.assertRaisesRegex(ValueError,"neutral B row"):
            roach_bomb.battle_test(joints,actions,[60])
        actions["profiles"][0]["groups"][0]["rows"]=[row]
        bad_actions={"profiles":[dict(actions["profiles"][0],config="00000000")]}
        with self.assertRaisesRegex(ValueError,"installer profile"):
            roach_bomb.battle_test(joints,bad_actions,[60])

    def test_cannon_beam_uses_its_charged_x_context_and_appends_to_the_volley(self):
        lines=cannon_beam.lines([500,200,200,0,240])
        self.assertIn("projectile=cannon-beam|beam|1|1|0|0|0|60|40|10|ignore-floor",lines)
        self.assertIn("sustained-beam=cannon-beam|1800|6|0",lines)
        self.assertIn("look=cannon-beam|180|180|220|255",lines)
        self.assertIn("origin=cannon-beam|0|0|45|70|target-height",lines)
        self.assertIn("move=cannon-beam|x|12|grounded|60|4:0",lines)
        self.assertIn("activation=cannon-beam|release-window|20|90",lines)
        self.assertIn("release=cannon-beam|20|shot-stock|2|cannon-beam",lines)
        self.assertFalse(any("aim-lock" in line for line in lines))
        with self.assertRaisesRegex(ValueError,"stock policy"):
            cannon_beam.lines([500,200,200,1,240])
        receipt=cannon_beam.receipt({"callback":"800B0F18","pose":[4,0],"program_requests":["800B1024","800B10B8"],
                                     "origin_evidence":"none","scope":"authored"})
        self.assertEqual(receipt["native_beam"]["length"],1800)
        self.assertEqual(receipt["native_beam"]["stock_cost"],2)

    def test_single_ground_shots_keep_their_poses_and_refuse_borrowed_states(self):
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(32)]
        cases=((0x0104,((0,0),(0,1),(0,24)),[300,5,5,1,60],False),
               (0x0802,((0,0),(0,1),(0,24),(0,2)),[200,10,10,0,300],False),
               (0x0D02,(),[200,40,40,0,300],True),
               (0x0D01,(),[150,2,2,0,300],True),
               (0x0F06,(),[30,5,5,0,300],True),
               (0x0C04,((0,25),(0,1),(0,24)),[500,200,200,0,240],False),
               (0x0303,((0,0),(0,1),(0,24)),[50,6,6,0,60],False),
               (0x0C02,((0,25),(0,26),(0,24),(0,2)),[400,50,50,0,240],False))
        for model,roles,level,static in cases:
            route=single_shots.ROUTES[model]
            source={"model":f"{model:04X}","action_group":route.get("group","B-shot"),
                    "release_entry":f"{route['leaf']:08X}",
                    "muzzle":{"key":route["key"],"joint":route["joint"],"point":list(route["point"])},
                    "debit":{"site":f"{route['debit']:08X}","channel":route.get("channel",0),"cost":1,"commit":True,
                             "same_input_actor":True},"dash":route["dash"]}
            if route["followups"]:
                source["followups"]=[{"key":f["key"],"joint":f["joint"],"point":list(f["point"])}
                                     for f in route["followups"]]
            bank,tone=route["tone"]
            keys={route["key"]}|{f["key"] for f in route["followups"]}
            sounds={route["family"]:{key:{"status":"static-tone-record","bank":bank,"tone":tone} for key in keys}}
            clips=[(name,g,c) for name,(g,c) in zip(
                ("ground-stop","ground-travel","air-continuation","ground-dash-forward"),roles)]
            def export():
                return single_shots.battle_test(model,joints,clips,{"native_units_per_tick_x100":500},level,
                                                source,sounds,launch_audio=True)
            rules,_,receipt=export()
            self.assertIn(f"origin=shot|{route['joint']}|".encode(),rules)
            self.assertIn(f"move={route.get('move','basic-shot')}|{route.get('button','b')}|10|grounded".encode(),rules)
            self.assertIn(f"launch-sound=shot|audio/launch-{bank}-{tone}.pcm\n".encode(),rules)
            self.assertEqual(receipt["source_normal_factory_key"],route["key"])
            if route["followups"]:
                self.assertEqual(rules.count(b"\nrelease="),1)
                name=route.get("move","basic-shot").encode()
                self.assertEqual(rules.count(b"release-followup="+name+b"|"),len(route["followups"]))
                for follow in route["followups"]:
                    self.assertIn(f"origin=shot".encode(),rules)
            else:
                self.assertNotIn(b"release-followup",rules)
            if static:
                self.assertIn(b"body-motion=static-grounded\n",rules)
                self.assertNotIn(b"idle=",rules)
                self.assertIn(b"move=basic-shot|b|10|grounded|28|none\n",rules)
                self.assertIn(b"release=basic-shot|12|shot-stock|1|shot\n",rules)
                if route["barrel"]:
                    self.assertEqual(receipt["source_selector_scope"],
                                     "initial barrel only; repeat alternation not implemented")
                    self.assertEqual(receipt["source_barrel"]["alternate_key"],19)
                else:
                    self.assertNotIn("source_selector_scope",receipt)
            elif route["dash"]=="declined":
                self.assertIn(b"dash-policy=disabled\n",rules)
                self.assertNotIn(b"dash=",rules)
                self.assertNotIn(b"air-dash",rules)
            else:
                self.assertIn(b"dash=0:2\n",rules)
                self.assertNotIn(b"dash-policy",rules)
            source["muzzle"]["joint"]=route["joint"]+1
            with self.assertRaisesRegex(ValueError,"exact muzzle"):
                export()
            source["muzzle"]["joint"]=route["joint"]
            for field,value in (("cost",2),("channel",1),("commit",False),("same_input_actor",False)):
                original=source["debit"][field]
                source["debit"][field]=value
                with self.assertRaisesRegex(ValueError,"dash policy"):
                    export()
                source["debit"][field]=original
            source["dash"]="bound" if route["dash"]=="declined" else "declined"
            with self.assertRaisesRegex(ValueError,"dash policy"):
                export()
            source["dash"]=route["dash"]
            sounds[route["family"]][route["key"]]["tone"]=tone+1
            with self.assertRaisesRegex(ValueError,"exact launch tone"):
                export()
            sounds[route["family"]][route["key"]]["tone"]=tone
            if not static:
                clips[0]=("ground-stop",0,25 if clips[0][2]!=25 else 0)
                with self.assertRaisesRegex(ValueError,"exact indexed local poses"):
                    export()
        hero=single_shots.ROUTES[0x0802]
        self.assertEqual(hero["dash"],"bound")
        self.assertEqual(hero["model_gate"]["form"],0x0803)
        self.assertEqual(hero["model_gate"]["form_key"],34)

    def test_model_id_guard_selects_the_ordinary_actor_record(self):
        prefix=(0x38000000,0x3C80802D,0x2C000010,0x38843AD4,0x40800084,
                *(w for i in range(16) for w in (0x80040000+4*i,0x900304B4+4*i)))
        slot_load=(0x880303E7,0x3C80803C,0x7C000774,0x5400103A,0x38844D94,0x7C04002E)
        start=0x80000000
        def installer(*ops):
            return (prefix+ops+(0,)*144)[:144]
        def resolve(model,words):
            rows,unresolved=interpret_installer(installer(*words),start,model,lambda _:0)
            self.assertEqual(unresolved,0)
            return rows
        # Gold Hero: the form branch sits before the ordinary store.
        gold=(*slot_load[:0],0xA80303E8,0x2C000803,0x41820024,*slot_load,0x900304AC,0x4E800020,
              0x800DACB8,0x900304AC,0x4E800020)
        self.assertEqual(resolve(0x0802,gold),[ (("resource",0),start+46*4) ])
        self.assertEqual(resolve(0x0803,gold),[ (None,start+49*4) ])
        # Metal Hero stores its own record first, then returns for a form.
        metal=(*slot_load,0x900304AC,0xA80303E8,0x2C00080B,0x4C820020,
               0x800DACB8,0x900304AC,0x4E800020)
        self.assertEqual(resolve(0x080A,metal),[ (("resource",0),start+43*4) ])
        self.assertEqual(resolve(0x080B,metal),[ (None,start+48*4) ])
        # The fusion slot is a reviewed standalone constant, so its ==1 branch
        # is unreachable and only the slot-0 record remains.
        fusion=(0x8803048A,0x2C000001,0x40820010,0x800DACB8,0x900304AC,0x4E800020,
                *slot_load,0x900304AC,0x4E800020)
        self.assertEqual(resolve(0x0F06,fusion),[ (("resource",0),start+49*4) ])
        # Any other actor byte is not reviewed and stays unresolved.
        other=(0x8803048B,0x2C000001,0x40820010,0x800DACB8,0x900304AC,0x4E800020,
               *slot_load,0x900304AC,0x4E800020)
        self.assertEqual(resolve(0x0F06,other),[ (None,start+41*4), (("resource",0),start+49*4) ])

    def test_late_neutral_close_uses_matching_attack_lanes_not_intro(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        for model,left in ((0x0101,5),(0x0108,5),(0x0804,4)):
            source={"model":f"{model:04X}","pose":{"clip":[1,1],"source_start_frame":0},
                    "default_part_candidates":[(7,1),(11,left)]}
            rules,_,_=knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[300],source)
            self.assertIn(b"move=opening-slash|b|10|grounded|24|1:1\n",rules)
            self.assertIn(f"part-record=left-hand|11|{left}\n".encode(),rules)
            source["pose"]["clip"]=[1,0]
            with self.assertRaisesRegex(ValueError,"exact pose"):
                knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[300],source)

    def test_wire_action_loop_window_needs_its_uniform_exact_metadata(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        source={"model":"0301","pose":{"clip":[1,0],"source_start_frame":0},"default_part_candidates":[(7,1),(11,6)]}
        with self.assertRaisesRegex(ValueError,"exact bounded authored attack loop window"):
            knight.melee_battle_test(0x0301,joints,clips,{"native_units_per_tick_x100":321},[320],source)
        source["strict_clip_metadata"]={"aobj_flags":[0x20000000],"strict_metadata_refusals":["aobj-flags"],
                                        "authored_end_frames":[38.],"animated_joint_count":20,"authored_loop_joint_count":20}
        rules,_,_=knight.melee_battle_test(0x0301,joints,clips,{"native_units_per_tick_x100":321},[320],source)
        self.assertIn(b"pose-loop-window=opening-slash|authored-bit\n",rules)
        self.assertNotIn(b"walk-loop=",rules)
        source["strict_clip_metadata"]["authored_loop_joint_count"]=19
        with self.assertRaisesRegex(ValueError,"exact bounded authored attack loop window"):
            knight.melee_battle_test(0x0301,joints,clips,{"native_units_per_tick_x100":321},[320],source)

    def test_grounded_close_uses_exact_attack_not_neutral_hold(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        for model,choice,start,hands in (
                (0x0209,1,8,[(7,2),(11,5)]),(0x0302,0,0,[(7,1),(11,5)]),
                (0x0808,30,0,[(7,1),(11,4)]),(0x0904,6,0,[(7,1),(11,5)]),(0x090B,6,0,[])):
            receipt={"model":f"{model:04X}","pose":{"clip":[1,choice],"source_start_frame":start},"default_part_candidates":hands}
            rules,_,_=knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":300},[320],receipt)
            self.assertIn(f"move=opening-slash|b|10|grounded|24|1:{choice}\n".encode(),rules)
            self.assertEqual(b"pose-start=opening-slash|8\n" in rules,start==8)
            self.assertEqual(rules.count(b"part-record="),len(hands))
            receipt["pose"]["clip"]=[1,1-choice if choice<2 else 0]
            with self.assertRaisesRegex(ValueError,"exact pose"):
                knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":300},[320],receipt)

    def test_remaining_robot_loops_and_time_bomber_close_scope(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        loop={"aobj_flags":[0x20000000],"strict_metadata_refusals":["aobj-flags"],"animated_joint_count":20,"authored_loop_joint_count":20}
        for model,choice,parts in ((0x0401,1,[]),(0x0404,1,[]),(0x0106,0,[(7,1),(11,2)]),
                                   (0x0107,0,[(7,1),(11,2)])):
            receipt={"model":f"{model:04X}","pose":{"clip":[1,choice],"source_start_frame":0},"default_part_candidates":parts}
            rules,_,_=knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":300},[320],receipt,
                                           clip_metadata=[loop if model==0x0401 else None,loop])
            self.assertEqual(b"idle-loop=authored-bit\n" in rules,model==0x0401)
            self.assertEqual(b"walk-loop=authored-bit\n" in rules,model not in (0x0106,0x0107))
            self.assertIn(f"move=opening-slash|b|10|grounded|24|1:{choice}\n".encode(),rules)
            self.assertNotIn(b"projectile=",rules)
            self.assertNotIn(b"body-material=",rules)
        self.assertIn(0x0107,knight.PILOTS)
        remote={"model":"0107","pose":{"clip":[1,0],"source_start_frame":0},
                "default_part_candidates":[(7,1),(11,2)]}
        remote_rules,_,_=knight.melee_battle_test(0x0107,joints,clips,{"native_units_per_tick_x100":300},[320],
                                                  remote,body_material="immutable-highlight")
        self.assertIn(b"body-material=immutable-highlight\n",remote_rules)
        self.assertIn(b"move=opening-slash|b|10|grounded|24|1:0\n",remote_rules)

    def test_wing_close_b_preserves_beam_frame_and_exact_hierarchies(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        for model,start in ((0x0A02,0),(0x0A03,10),(0x0A07,0),(0x0B03,0)):
            source={"model":f"{model:04X}","pose":{"clip":[1,1],"source_start_frame":start},
                    "default_part_candidates":[(7,1),(11,5)],"static_hand_hierarchies":[1,5]}
            rules,_,_=knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[300],source)
            self.assertIn(b"move=opening-slash|b|10|grounded|24|1:1\n",rules)
            self.assertEqual(b"pose-start=opening-slash|10\n" in rules,start==10)
            self.assertEqual(b"part-hierarchy=1\npart-hierarchy=5\n" in rules,model in (0x0A02,0x0B03))
            source["pose"]["source_start_frame"]=10-start
            with self.assertRaisesRegex(ValueError,"exact pose"):
                knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[300],source)

    def test_robot_close_b_accepts_only_the_walk_loop_and_its_own_tool(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        proof={"aobj_flags":[0x20000000],"strict_metadata_refusals":["aobj-flags"],"animated_joint_count":20,"authored_loop_joint_count":20}
        source={"model":"0402","pose":{"clip":[1,1],"source_start_frame":0},
                "default_part_candidates":[(11,4)],"static_left_hand_record":4}
        rules,_,_=knight.melee_battle_test(0x0402,joints,clips,{"native_units_per_tick_x100":143},[320],source,
                                           clip_metadata=[None,proof])
        self.assertIn(b"part-record=left-hand|11|4\npart-hierarchy=4\n",rules)
        self.assertIn(b"walk-loop=authored-bit\n",rules)
        self.assertNotIn(b"idle-loop=",rules)
        for bad in ({**proof,"aobj_flags":[0x20000000,0]},{**proof,"authored_loop_joint_count":19}):
            with self.assertRaisesRegex(ValueError,"uniformly authored loop"):
                knight.melee_battle_test(0x0402,joints,clips,{"native_units_per_tick_x100":143},[320],source,
                                         clip_metadata=[None,bad])

    def test_flame_ninja_does_not_substitute_exact_empty_hand_records(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        data={"model":"0006","pose":{"clip":[1,1],"source_start_frame":0},"default_part_candidates":[(7,1),(11,4)]}
        with self.assertRaisesRegex(ValueError,"exact empty defaults"):
            knight.melee_battle_test(6,joints,clips,{"native_units_per_tick_x100":500},[300],data)
        data["empty_default_hand_records"]=[1,4]
        rules,_,_=knight.melee_battle_test(6,joints,clips,{"native_units_per_tick_x100":500},[300],data)
        self.assertNotIn(b"part-record=",rules)
        self.assertIn(b"move=opening-slash|b|10|grounded|24|1:1\n",rules)

    def test_additional_close_openings_keep_exact_hands_or_no_mount(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        for model,choice,start in ((0x0201,4,0),(0x0206,1,8),(0x0207,1,8),(0x030C,0,0)):
            parts=[(7,2)] if model==0x0201 else []
            source={"model":f"{model:04X}","pose":{"clip":[1,choice],"source_start_frame":start},
                    "default_part_candidates":parts,"stockless_test_part":{"record":2,"joint":7}}
            rules,_,_=knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[320],source)
            self.assertIn(f"move=opening-slash|b|10|grounded|24|1:{choice}\n".encode(),rules)
            self.assertEqual(b"part-record=right-hand|7|2\npart-hierarchy=2\n" in rules,bool(parts))
            self.assertEqual(b"pose-start=opening-slash|8\n" in rules,start==8)

    def test_native_worm_zero_reflection_rule_is_asset_bound(self):
        body=b"owned fixture"
        offsets={0x050B:(sha256(body).hexdigest(),((0x15C98,7),))}
        with patch.object(reflection,"NORMAL_BODIES",offsets):
            line,receipt=reflection.zero_reflection_rules(0x050B,body)
            self.assertEqual(line,"zero-reflection=15c98|7\n")
            self.assertEqual(receipt["meshes"],[{"pobj_offset":0x15C98,"authored_zero_normals":7}])
            self.assertIsNone(reflection.zero_reflection_rules(0x0501,body))
            with self.assertRaisesRegex(ValueError,"exact normal wrapper"):
                reflection.zero_reflection_rules(0x050B,b"another body")

    def test_scoped_knight_pairs_use_their_own_opening_and_frame(self):
        clips=[("ground-stop",0,0),("ground-travel",0,1),("air-continuation",0,24),("ground-dash-forward",0,2)]
        joints=[{"index":i,"descriptor_offset":i*4} for i in range(20)]
        for model,choice,start in ((0x0202,4,0),(0x020B,4,0),(0x0203,1,8),(0x020C,1,8)):
            source={"model":f"{model:04X}","pose":{"clip":[1,choice],"source_start_frame":start},
                    "default_part_candidates":[(7,2),(11,5)]}
            mounts=((17,7,-15,-5,0),*((i,i,0,0,0) for i in range(8,20)),(2,7,0,0,0),(5,11,0,0,0)) if model in knight.ELEMENTAL else ()
            rules,_,receipt=knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[320],source,mounts=mounts)
            self.assertIn(f"move=opening-slash|b|10|grounded|24|1:{choice}\n".encode(),rules)
            self.assertEqual(b"pose-start=opening-slash|8\n" in rules,start==8)
            self.assertEqual(rules.count(b"body-mount="),15 if mounts else 0)
            self.assertEqual(receipt["source_start_frame"],start)
            source["pose"]["clip"][1]=5-choice
            with self.assertRaisesRegex(ValueError,"exact pose"):
                knight.melee_battle_test(model,joints,clips,{"native_units_per_tick_x100":321},[320],source,mounts=mounts)

    def test_dragon_trailing_chain_requires_exact_terminal_branch(self):
        joints=[{} for _ in range(32)]
        for i in range(27,32):
            joints[i]={"index":i,"child_next_parent":[i+1 if i<31 else 255,255,20 if i==27 else i-1],
                       "local_translation":[0.,-40.,0.]}
        spec=dragon.trailing_chain(joints)
        self.assertEqual(spec["bones"],(27,28,29,30,31))
        self.assertEqual(spec["bind_local_lengths"],[40.]*5)
        self.assertFalse(spec["gameplay_collision"])
        joints[31]["child_next_parent"][1]=23
        with self.assertRaisesRegex(ValueError,"topology"):
            dragon.trailing_chain(joints)

    def test_equipment_moves_only_cosmetic_record_slots(self):
        arm={"slot":"arm-effect-0","joint":10,"record":8,"point":[25.463,16.664,0],
             "angles":[0,0,0],"spin":"y|32|forward"}
        rules=equipment.equipment_rules({"parts":[arm],"records":{8:{}},"native_mount_admitted":True})
        self.assertIn(b"part-record=arm-effect-0|10|8\n",rules)
        self.assertIn(b"part-mount-spin=arm-effect-0|y|32|forward\n",rules)
        self.assertTrue(rules.endswith(b"part-hierarchy=8\n"))
        self.assertNotIn(b"move=",rules)
        self.assertNotIn(b"projectile=",rules)
        with self.assertRaisesRegex(ValueError,"not a qualified native mount"):
            equipment.equipment_rules({"native_mount_admitted":False})

    def test_ninja_close_b_uses_its_own_hand_records_and_no_x(self):
        joints=[{"index":i,"descriptor_offset":64*(i+1)} for i in range(20)]
        roles=[(r,0,i) for r,i in zip(LOCOMOTION,(0,1,24,2))]
        source={"model":"0009","pose":{"clip":[1,1],"source_start_frame":0},"default_part_candidates":[(7,1),(11,4)]}
        args=(0x0009,joints,roles,{"native_units_per_tick_x100":563},[200],source)
        rules,_,receipt=knight.melee_battle_test(*args)
        self.assertIn(b"part-record=right-hand|7|1\npart-record=left-hand|11|4\n",rules)
        self.assertEqual([row for row in rules.splitlines() if row.startswith(b"move=")],
                         [b"move=opening-slash|b|10|grounded|24|1:1"])
        self.assertFalse(receipt["retail_behavior_verified"])
        source["default_part_candidates"]=[(7,1),(11,2)]
        with self.assertRaisesRegex(ValueError,"exact pose, locomotion or part slots"):
            knight.melee_battle_test(*args)

    def test_gatling_keeps_held_fire_separate_from_one_shot_rules(self):
        joints=[{"index":i,"descriptor_offset":64*(i+1)} for i in range(20)]
        roles=[(r,0,i) for r,i in zip(LOCOMOTION,(0,1,24,2))]
        args=(joints,roles,{"native_units_per_tick_x100":417},[250,100,100,0,480],{"pose":{"clip":[3,11]}})
        rules,_,receipt=gatling.battle_test(*args,launch_audio=True)
        self.assertIn(b"held-fire=gatling-b|gatling-stock|1|gatling|12|8|3|10\n",rules)
        self.assertIn(b"launch-sound=gatling|audio/launch-snd_com01-6.pcm\n",rules)
        self.assertNotIn(b"\nrelease=",rules)
        self.assertFalse(receipt["retail_behavior_verified"])
        self.assertEqual(shot_readiness({"held_fire_battle_test":receipt},[])["status"],"authored-held-fire-test-declared")
        with self.assertRaisesRegex(ValueError,"stock or pace"):
            gatling.battle_test(*args[:3],[250,10,10,0,480],args[-1])

    def test_samurai_uses_its_hand_records_and_neutral_clip_only(self):
        # The four blocked Zeta models have empty default part rows in the
        # source table, so even a qualified material stage could not bind the
        # shared hand records; they stay out of the cohort.
        self.assertNotIn(0x070C,samurai.MODELS)
        self.assertEqual(samurai.BLOCKED_MATERIALS,{0x0705,0x070B,0x070C,0x070D})
        joints=[{"index":i,"descriptor_offset":64*(i+1)} for i in range(20)]
        roles=[(r,0,i) for r,i in zip(LOCOMOTION,(0,1,24,2))]
        source={"model":"0709","pose":{"clip":[1,1],"source_start_frame":0},"default_part_candidates":[(7,1),(11,2)]}
        loop={"aobj_flags":[0x20000000],"strict_metadata_refusals":["aobj-flags"],
              "animated_joint_count":20,"authored_loop_joint_count":20}
        args=(0x0709,joints,roles,{"native_units_per_tick_x100":458},[250],source)
        rules,_,receipt=knight.melee_battle_test(*args,clip_metadata=(loop,loop))
        self.assertIn(b"part-record=right-hand|7|1\npart-record=left-hand|11|2\n",rules)
        self.assertIn(b"move=opening-slash|b|10|grounded|24|1:1\n",rules)
        self.assertNotIn(b"pose-start=",rules)
        self.assertFalse(receipt["retail_behavior_verified"])
        source["pose"]["clip"]=[1,18]
        with self.assertRaisesRegex(ValueError,"exact pose"):
            knight.melee_battle_test(*args,clip_metadata=(loop,loop))

    def test_knight_pilot_uses_exact_parts_and_pose_start_without_stock(self):
        joints = [{"index":i,"descriptor_offset":64*(i+1)} for i in range(20)]
        source = {"model":"0200","pose":{"clip":[1,1],"source_start_frame":8},
                  "default_part_candidates":[(7,2),(11,5)]}
        roles = [(r,0,i) for r,i in zip(LOCOMOTION,(0,1,24,2))]
        pace = {"native_units_per_tick_x100":458}
        rules,_,record = knight.melee_battle_test(0x0200,joints,roles,pace,[180],source)
        for line in (b"part-record=right-hand|7|2\n",b"part-record=left-hand|11|5\n",
                     b"move=opening-slash|b|10|grounded|24|1:1\n",b"pose-start=opening-slash|8\n"):
            self.assertIn(line,rules)
        self.assertNotIn(b"stock=",rules)
        self.assertNotIn(b"projectile=",rules)
        self.assertFalse(record["retail_behavior_verified"])
        source["default_part_candidates"] = [(7,1)]
        for id in (0x0200,0x020F):
            with self.assertRaisesRegex(ValueError,"exact pose, locomotion or part slots"):
                knight.melee_battle_test(id,joints,roles,pace,[180],source)

    def test_gamma_sword_requires_its_distinct_mount_operands(self):
        raw=bytearray(36)
        struct.pack_into(">HBB6f",raw,0,1,10,8,45.18,0,0,45.18,0,0)
        mount=knight.gamma_sword_placement(raw)
        self.assertEqual(mount["roll"]["add_binary_angle"],-16384)
        self.assertFalse(mount["live_verified"])
        joints = [{"index":i,"descriptor_offset":64*(i+1)} for i in range(20)]
        roles = [(r,0,i) for r,i in zip(LOCOMOTION,(0,1,24,2))]
        source={"model":"020F","pose":{"clip":[1,1],"source_start_frame":8},
                "default_part_candidates":[],"sword_mount":mount}
        rules,_,receipt=knight.melee_battle_test(0x020F,joints,roles,{"native_units_per_tick_x100":458},[80],source)
        self.assertIn(b"part-record=sword|10|1\npart-basis=sword|world-zyx\npart-pose=sword|45.1800003|0|0|0|0|-90\n",rules)
        self.assertNotIn(b"part-record=right-hand",rules)
        self.assertFalse(receipt["retail_behavior_verified"])
        raw[2]=11
        with self.assertRaisesRegex(ValueError,"exact sword row"):
            knight.gamma_sword_placement(raw)
        with self.assertRaisesRegex(ValueError,"extent"):
            knight.gamma_sword_placement(raw[:15])

    def test_axe_pilot_preserves_two_loop_roles_and_exact_model_parts(self):
        class Fixture:
            def data(self, at, length):
                assert length==24
                operands={0x80301C88:(8,7,-10,-5,0,0,0,0,0),0x80301CA0:(9,13,0,0,-22,16384,0,16384,0)}
                return struct.pack(">hh3f4h",*operands[at])
        mount=knight.axe_child_parts(Fixture(),0x020D)
        self.assertEqual(mount["key"],1)
        joints=[{"index":i,"descriptor_offset":64*(i+1)} for i in range(20)]
        roles=[(r,0,i) for r,i in zip(LOCOMOTION,(0,1,24,2))]
        source={"model":"020D","pose":{"clip":[1,1],"source_start_frame":0},
                "default_part_candidates":[(7,2),(11,5)],"axe_parts":mount}
        meta={"aobj_flags":[0x20000000],"strict_metadata_refusals":["aobj-flags"],
              "animated_joint_count":20,"authored_loop_joint_count":20}
        args=(0x020D,joints,roles,{"native_units_per_tick_x100":458},[200],source)
        rules,_,receipt=knight.melee_battle_test(*args,clip_metadata=(meta,meta))
        self.assertIn(b"idle-loop=authored-bit\nwalk-loop=authored-bit\n",rules)
        self.assertIn(b"part-record=axe-0|7|8\n",rules)
        self.assertIn(b"part-record=axe-1|13|9\npart-basis=axe-1|bone-zyx\npart-pose=axe-1|0|0|-22|90|0|90\n",rules)
        self.assertNotIn(b"part-swap=",rules)
        self.assertNotIn(b"pose-start=",rules)
        self.assertFalse(receipt["retail_behavior_verified"])
        with self.assertRaisesRegex(ValueError,"uniformly authored loop"):
            knight.melee_battle_test(*args,clip_metadata=(meta,{**meta,"authored_loop_joint_count":19}))

    def test_worm_index_keeps_family_keys_sockets_and_cue_owners_distinct(self):
        blocks = {}
        for table,stride,count,joint_offset,*_ in worm.FAMILIES.values():
            raw = bytearray(stride*count)
            for key in range(count):
                raw[key*stride+joint_offset] = 31 if count==3 and key==1 else 12
                point = (0.,32.25,5.525) if key else (0.,68.5,11.5)
                struct.pack_into(">3f",raw,key*stride+4,*point)
            blocks[table] = bytes(raw)
        data = tuple((at,len(raw),sha256(raw).hexdigest()) for at,raw in blocks.items())
        class Fixture:
            def read(self, at, size, kind):
                return blocks[at][:size]
        def tone(n):
            return {"status":"static-tone-record","bank":("snd_com02" if n==210 else "snd_com03"),"tone":n%128}
        with patch.object(worm,"TEXT",()), patch.object(worm,"DATA",data):
            result = worm.worm_source_index(Fixture(),tone)
            self.assertEqual([(r["family"],r["key"],r["sound"]["owner"],r["sound"]["id"]) for r in result.values()],
                             [("type-49",0,"credited-actor",210),("type-94",0,"allocated-child",260),
                              ("type-49",1,"credited-actor",210),("type-94",2,"allocated-child",260)])
            self.assertTrue(all(r["stock"]["requires_debit_success"] and r["sound"]["requires_collision_bind"]
                                and not r["live_release_verified"] for r in result.values()))
            with self.assertRaisesRegex(ValueError,"cue bank"):
                worm.worm_source_index(Fixture(),lambda _: {})
            first = next(iter(blocks))
            blocks[first] = blocks[first][:-1]
            with self.assertRaisesRegex(ValueError,"data signature"):
                worm.worm_source_index(Fixture(),tone)

    def test_worm_pilot_preserves_exact_pose_and_socket_without_retail_stock_semantics(self):
        class Fixture:
            def table_choice(self, root, group, choice):
                return 0x8032ABDA
            def entry_clip_binding(self, entry, site, args, roots, local):
                return {"clip":[3,0],"archive_offset":local[3,0]}
        model = 0x050D
        source = {"model":"050D","family":"type-49","key":1,"call":"801198B8",
                  "origin":{"source_joint":12,"local_point":[0.,32.25,5.525000095367432]},
                  "stock":{"channel":0,"cost":1,"commit":True,"same_input_actor":True,"site":"80119868"},
                  "sound":{"status":"static-tone-record","bank":"snd_com02","tone":82}}
        joints = [{"index":i,"descriptor_offset":64*(i+1)} for i in range(32)]
        actions = {"profiles":[{"config":config,"main_handler":"80119214","context_table":"8032B9B0",
            "program_and_descriptor_roots":["8032A8E0","8032AD02","8032AF20","8032B204"],
            "groups":[{"group":"B-shot","rows":[{"contexts":[(0,0)],"state":0,"substate":0,"selector":44,
                                                "class_callback_candidate":"80119250"}]}]} for config in sorted(worm.PROFILES)]}
        linked = worm.attach_worm_pose(Fixture(),actions,[(3,0,158016)],joints,source,publish_preview=True)
        self.assertEqual((linked["pose"]["phase"],linked["pose"]["control_write"]),(2,"8032ABF6"))
        roles = [(r,0,i) for r,i in zip(LOCOMOTION,(0,1,7,10))]
        rules,_,receipt = worm_battle_test(model,joints,roles,{"native_units_per_tick_x100":42},
                                          [150,5,5,2,0],linked,launch_audio=True)
        for line in (b"stock=shot-stock|5|180\n",b"origin=shot|12|0|32.25|5.5250001|target-height\n",
                     b"move=basic-shot|b|10|grounded|28|3:0\n",b"launch-sound=shot|audio/launch-snd_com02-82.pcm\n"):
            self.assertIn(line,rules)
        self.assertFalse(receipt["retail_behavior_verified"])
        self.assertEqual(receipt["source_stock_mode"],2)
        summary = {"action_candidates":actions,"role_preview_clips":roles,"lab_ground_pace":{"record":0},
                   "worm_shot_source":linked,"logical_joints":joints,
                   "raw_animation_slots":[(0,c,64*c) for c in (0,1,7,10)]+[(3,0,158016)]}
        summary["local_clip_timing"] = [{"aobj_timing":"indexed","archive_offset":at,"public_joint_count":32}
                                        for _,_,at in summary["raw_animation_slots"]]
        self.assertEqual(shot_readiness(summary,[])["status"],"source-qualified-opt-in-not-installed")
        summary["local_clip_timing"][-1]["strict_metadata_refusals"] = ["animjoint-constraints-or-flags"]
        self.assertEqual(shot_readiness(summary,[])["status"],"strict-clip-metadata-refusal")
        with self.assertRaisesRegex(ValueError,"release or socket evidence"):
            worm_battle_test(0x0501,joints,roles,{"native_units_per_tick_x100":42},[150,5,5,2,0],linked)
        actions["profiles"][0]["groups"][0]["rows"][0]["selector"] = 45
        with self.assertRaisesRegex(ValueError,"phase owner"):
            worm.attach_worm_pose(Fixture(),actions,[(3,0,158016)],joints,source,publish_preview=True)

    def test_impact_area_common_bank_does_not_infer_a_live_scaled_radius(self):
        data=bytearray(dragon_area.COMMON_COLLISION_SIZE)
        group=60*4
        data[group:group+4]=bytes((94,255,255,255))
        row=0x400+80*94
        struct.pack_into(">BBHHhhH4f",data,row,0,0,1,4,0,-1,0,0,0,0,310)
        with patch.object(dragon_area,"COMMON_COLLISION_SHA",sha256(data).hexdigest()):
            result=dragon_area.common_collision(data)
            self.assertEqual((result["member"],result["pzz_slot"],result["group"],result["descriptor_row"]),(696,0,60,94))
            self.assertEqual(result["source_radius"],310)
            self.assertFalse(result["live_radius_verified"])
            self.assertFalse(result["live_publication_verified"])
            data[group]=93
            with self.assertRaisesRegex(ValueError,"block signature"):
                dragon_area.common_collision(data)

    def test_elemental_impact_index_preserves_common_area_scope_and_exact_rows(self):
        table=bytearray(27*36)
        keys=(3,24,21,22,25,23)
        carriers={f"gg4e08:{m:04x}":{"key":i,"tail":{"area_key":key}}
                  for (m,i),key in zip(dragon.DRAGON_KEYS.items(),keys,strict=True)}
        for i,k in enumerate(keys):
            struct.pack_into(">4h6fi",table,36*k,22,60,1,int(i>0),i+1.,2.,3.,4.,5.,6.,12)
        spans={dragon_area.TABLE:table,dragon_area.PHASE_TABLE:struct.pack(">3I",*dragon_area.PHASES)}
        class Fixture:
            def read(self,at,size,kind):
                data=spans[at]
                if len(data)!=size:
                    raise ValueError("unexpected extent")
                return data
        def tone(cue):
            return {"status":"static-tone-record","bank":"snd_com01","bank_slot":0,"tone":cue}
        with patch.object(dragon_area,"TEXT",()),patch.object(dragon_area,"TABLE_SHA",sha256(table).hexdigest()):
            idx=dragon_area.area_index(Fixture(),carriers,sound_resolver=tone)
            self.assertFalse(idx["phase_zero"]["shape_verified"])
            self.assertFalse(idx["live_damage_verified"])
            self.assertEqual(idx["phase_zero"]["cue"]["tone"],12)
            self.assertEqual([r["area_key"] for r in idx["rows"]],sorted(keys))
            self.assertEqual({r["visual_record"] for r in idx["rows"]},{22})
            self.assertEqual(idx["rows"][1]["source_float_operands"][0]["value"],3)
            bad={**carriers,"wrong":{"key":6,"tail":{"area_key":3}}}
            with self.assertRaisesRegex(ValueError,"key domain"):
                dragon_area.area_index(Fixture(),bad,sound_resolver=tone)
            table[36*3+9]^=1
            with self.assertRaisesRegex(ValueError,"row table signature"):
                dragon_area.area_index(Fixture(),carriers,sound_resolver=tone)

    def test_dragon_pilot_uses_later_pose_and_requires_its_model_scoped_release(self):
        identity={"config":"802D5828","context_table":"802D5A00","main_handler":"80075A2C"}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]],"release_pose_candidate":{
            "phase_table":"802D56EC","release_site":"80075CA4","clip":[3,0],"archive_offset":128}}
        actions={"profiles":[{**identity,"groups":[{"group":"B-shot","rows":[row]}]}]}
        mapping={"profiles":[{"movement_type":4,"roles":[{"selector":0,"states":[1,1],
            "handler_addresses":["8005F23C","8005F2A4"],"program_candidates":[{
                "site":"8005F2F8","lanes":2,"program_group":0,"program_choice":0,
                "clip_candidates":[{"preceding_waits":0,"start_frame":0,"lanes":[
                    {"lane":1,"bank":"local","archive_offset":64,"group":0,"choice":0}]}]}]}]}]}
        route={**identity,**row,"action_group":"B-shot","context_rows":[[0,0]],"phase_table":"802D56EC",
               "release_entry":"80075BF8","local_stock_guards":[{
                   "channel":0,"cost":1,"commit":True,"same_input_actor":True,"debit_site":"80075C38"}]}
        shot={"site":"80075CA4","key":0,"family":"type-47","basis":"parent-joint","source_joint_selector":12,
              "local_point":[0,0,0],"joint_present_in_normal_rig":True,"required_model_at_call":"0500","routes":[route]}
        art={"visuals":{"owner":"caster-selected-wrapper","separate_record":{"key":8},"repeated_record":{"key":9,"count":5},
                        "records":{8:{},9:{"empty_static":False}}}}
        joints=[{"index":i,"descriptor_offset":64+i*64} for i in range(13)]
        role_clips=[("ground-travel",0,1),("air-continuation",0,7)]
        def export():
            return dragon_battle_test(0x0500,joints,role_clips,mapping,actions,{"native_units_per_tick_x100":108},
                                      [1000,5,5,1,300],[shot],art)
        rules,_,receipt=export()
        self.assertIn(b"dash-policy=disabled\n",rules)
        self.assertNotIn(b"\ndash=",rules)
        self.assertIn(b"projectile-record=orb|8|10\n",rules)
        self.assertIn(b"projectile-record-trail=orb|9|5|1|6\n",rules)
        self.assertIn(b"impact-record=orb|actor|22|10|18\nimpact-record-growth=orb|25\n",rules)
        self.assertEqual(receipt["impact_record"]["authored_scale_percent"],[10,25])
        self.assertTrue(receipt["impact_record"]["visual_only"])
        self.assertIn(b"move=basic-shot|b|10|grounded|28|3:0\n",rules)
        self.assertFalse(receipt["retail_behavior_verified"])
        art["visuals"]["records"][9]["empty_static"]=True
        self.assertNotIn(b"projectile-record-trail=",export()[0])
        shot["required_model_at_call"]="0509"
        with self.assertRaisesRegex(ValueError,"socket or stock proof"):
            export()

    def test_dragon_visual_receipt_requires_exact_relocated_record_and_never_defaults(self):
        data=bytearray(128)
        # scene export -> table -> record zero -> root, then null terminator.
        for at,value in ((4,8),(8,16),(16,32)):
            struct.pack_into(">I",data,at,value)
        relocs=struct.pack(">3I",4,8,16)
        public=struct.pack(">2I",4,0)
        string=b"scene_data\0"
        size=32+len(data)+len(relocs)+len(public)+len(string)
        header=struct.pack(">8I",size,len(data),3,1,0,0,0,0)
        def archive():
            return header+data+relocs+public+string
        row,=dragon_art.record_receipts(archive(),(0,))["records"].values()
        self.assertEqual(row["root_offset"],32)
        self.assertIsNone(row["material_table_offset"])
        self.assertTrue(row["empty_static"])
        with self.assertRaisesRegex(ValueError,"missing requested record"):
            dragon_art.record_receipts(archive(),(1,))
        struct.pack_into(">I",data,8,127)
        with self.assertRaisesRegex(ValueError,"relocation"):
            dragon_art.record_receipts(archive(),(0,))

    def test_dragon_carrier_index_keeps_trail_and_impact_operands_separate(self):
        raw=bytearray(6*48)
        for index,(area,tag) in enumerate(((3,0),(24,1),(21,0),(22,0),(25,2),(23,0))):
            base=index*48
            struct.pack_into(">b",raw,base+3,1)
            struct.pack_into(">fIff",raw,base+0x10,200 if index<3 else 100,4,4.,100.)
            struct.pack_into(">bb",raw,base+0x2e,area,tag)
        spans={carrier.ROW_TABLE:bytes(raw),carrier.PHASE_TABLE:struct.pack(">4I",*carrier.PHASES),
               carrier.TRAIL_TABLE:b"".join(struct.pack(">4f",*r) for r in carrier.TRAIL_KEYS),
               **{carrier.TYPE30_ROWS+24*i:struct.pack(">2h",7,8) for i in (2,3)}}
        class Fixture:
            def read(self,address,size,kind):
                data=spans[address]
                if len(data)!=size:
                    raise ValueError("unexpected extent")
                return data
        with patch.object(carrier,"TEXT",()),patch.object(carrier,"ROW_TABLE_SHA",sha256(raw).hexdigest()):
            index=carrier.carrier_index(Fixture())
            self.assertFalse(index["live_child_schedule_verified"])
            self.assertEqual(index["init"]["lifetime_seed"],600)
            self.assertTrue(index["active"]["trail"]["onboard_records_not_projectile_allocations"])
            self.assertEqual(index["active"]["trail"]["capacity"],5)
            self.assertEqual(index["borgs"]["gg4e08:0509"]["tail"],{"area_key":24,"selector_tag":1,"type_30_mode":2})
            self.assertEqual(index["borgs"]["gg4e08:0515"]["tail"],{"area_key":25,"selector_tag":2,"type_30_mode":3})
            self.assertIsNone(index["borgs"]["gg4e08:0516"]["tail"]["type_30_mode"])
            self.assertNotIn("gg4e08:050f",index["borgs"])
            spans[carrier.TRAIL_TABLE]=bytes(64)
            with self.assertRaisesRegex(ValueError,"trail table signature"):
                carrier.carrier_index(Fixture())
            spans[carrier.ROW_TABLE]=bytes(len(raw))
            with self.assertRaisesRegex(ValueError,"row table signature"):
                carrier.carrier_index(Fixture())

    def test_dragon_collision_export_keeps_three_distinct_exact_groups(self):
        bank=bytearray(carrier.COLLISION_BYTES)
        for group,row,shape,radius in ((9,13,0,180.),(7,9,0,10.),(8,12,2,110.)):
            bank[0xA20+group*4:0xA24+group*4]=bytes((row,255,255,255))
            base=0xAA0+80*row
            struct.pack_into(">BBHHhhH",bank,base,shape,0,1,group,0,-1,0)
            struct.pack_into(">f",bank,base+(0x18 if shape==0 else 0x24),radius)
            if shape==2:
                struct.pack_into(">f",bank,base+0x10,200.)
                struct.pack_into(">f",bank,base+0x1C,200.)
        row={"collision_group":9,"tail":{"type_30_mode":2}}
        result=carrier.collision_index(bank,561,row)
        self.assertFalse(result["live_publication_verified"])
        self.assertEqual([result[k]["radius"] for k in ("carrier","type_30_emitter","type_30_descendant")],[180,10,110])
        self.assertEqual(result["type_30_descendant"]["endpoints"],[(0,0,0),(0,400,0)])
        self.assertNotIn("type_30_emitter",carrier.collision_index(bank,561,{**row,"tail":{"type_30_mode":None}}))
        bank[0xA20+4*9+1]=14
        with self.assertRaisesRegex(ValueError,"requires one row"):
            carrier.collision_index(bank,561,row)

    def test_dragon_release_table_is_model_scoped_and_every_slot_is_pinned(self):
        fake=ActionLinker(None,{},[])
        row={"state":0,"substate":0,"selector":44}
        words={pc:0x38800000|i for i,pc in enumerate(dragon.CASE_TARGETS)}
        graph=(words,{dragon.RELEASE:{dragon.RELEASE-4}},{},{dragon.RELEASE:dragon.FACTORY},0,False)
        table={dragon.JUMP_TABLE+4*i:dragon.CASE_TARGETS[dragon.DRAGON_KEYS.get(0x0500+i,0)] for i in range(23)}
        pose={"clip":[3,0],"archive_offset":128}
        with patch.object(fake,"matches_text",return_value=True), \
             patch.object(fake,"dispatch",return_value=dragon.PHASE_TABLE), \
             patch.object(fake,"leaf",side_effect=lambda _,__,i:(0x80075AE0,dragon.POSE_ENTRY,dragon.CONSUMER)[i]), \
             patch.object(fake,"read_word",side_effect=table.__getitem__), \
             patch.object(fake,"graph",return_value=graph) as cfg, \
             patch.object(fake,"calls",return_value=(((dragon.POSE_REQUEST,"program",(15,2,0),"literal-block"),),0,False)), \
             patch.object(fake,"entry_clip_binding",return_value=pose):
            for model,key in dragon.DRAGON_KEYS.items():
                self.assertEqual(dragon.dragon_shot(fake,model,dragon.WRAPPER,row,(),{}),(key,pose))
                self.assertEqual(cfg.call_args.kwargs["reviewed_dispatch"],(dragon.DISPATCH,dragon.CASE_TARGETS[key]))
            self.assertIsNone(dragon.dragon_shot(fake,0x050F,dragon.WRAPPER,row,(),{}))
            self.assertIsNone(dragon.dragon_shot(fake,0x0500,dragon.WRAPPER,{**row,"substate":1},(),{}))
            table[dragon.JUMP_TABLE+4*3]=dragon.CASE_TARGETS[1] # even an unrelated unused entry must match
            self.assertIsNone(dragon.dragon_shot(fake,0x0500,dragon.WRAPPER,row,(),{}))

    def test_later_carrier_pose_does_not_replace_the_entry_preview(self):
        original={"clip":[0,1],"archive_offset":64}
        release={"clip":[3,0],"archive_offset":128,"release_site":"80075CA4"}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]],
             "entry_clip_candidate":original,"release_pose_candidate":release}
        p={"config":"802D5828","context_table":"802D5A00","main_handler":"80075A2C",
           "groups":[{"group":name,"mode":0,"rows":[row] if name=="B-shot" else []} for name in PREVIEW_GROUPS]}
        result=action_preview_candidates({"profiles":[p]})
        self.assertEqual([(v["id"],v["clip"]) for v in result],
                         [("b-shot-c0-s0",[0,1]),("b-shot-release-pose",[3,0])])
        second={**p,"groups":[{**g,"rows":[{**row,"release_pose_candidate":{**release,"archive_offset":192}}]}
                             if g["group"]=="B-shot" else g for g in p["groups"]]}
        self.assertEqual([v["id"] for v in action_preview_candidates({"profiles":[p,second]})],["b-shot-c0-s0"])

    def test_cruise_charge_uses_stockless_exact_model_branch_and_no_clip(self):
        identity={"config":"80320000","context_table":"80320100","main_handler":"800F8000"}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]]}
        actions={"profiles":[{**identity,"groups":[{"group":"B-charge","rows":[row]}]}]}
        route={**identity,**row,"action_group":"B-charge","context_rows":[[0,0]],
               "phase_table":"8031D158","release_entry":"800F809C"}
        shot={"site":"800F80D8","key":56,"family":"type-2","basis":"parent-joint","parent_model_matches":True,
              "joint_present_in_normal_rig":True,"source_joint_selector":1,"local_point":[0,0,120],
              "excluded_models_at_call":["0D00"],"routes":[route]}
        args=(0x0D04,[{"index":i,"descriptor_offset":64+i*64} for i in range(2)],actions,[150])
        result,_,receipt=cruise_charge_shot_test(*args,[shot],(240,240,12,12,8,4))
        self.assertIn(b"activation=charge-shot|release-window|6|45\n",result)
        self.assertIn(b"release=charge-shot|8|none|0|shot\n",result)
        self.assertNotIn(b"stock=",result)
        self.assertFalse(receipt["retail_behavior_verified"])
        with self.assertRaisesRegex(ValueError,"expected model branch"):
            cruise_charge_shot_test(*args,[{**shot,"excluded_models_at_call":[]}],(240,240,12,12,8,4))
        route["local_stock_guards"]=[{"channel":0}]
        with self.assertRaisesRegex(ValueError,"one reviewed release"):
            cruise_charge_shot_test(*args,[shot],(240,240,12,12,8,4))

    def test_static_cruise_shots_require_exact_model_keys_without_a_motion_clip(self):
        identity={"config":"80392570","context_table":"80393000","main_handler":"801E7AC0"}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]]}
        actions={"profiles":[{**identity,"groups":[{"group":"B-shot","rows":[row]}]}]}
        route={**identity,**row,"action_group":"B-shot","context_rows":[[0,0]],
               "local_stock_guards":[{"channel":0,"cost":1,"commit":True,"same_input_actor":True,"debit_site":"801E7B38"}]}
        variants=[{"site":site,"key":key,"family":"type-2","basis":"parent-joint","parent_model_matches":True,
                   "joint_present_in_normal_rig":True,"source_joint_selector":1,"local_point":point,
                   "required_model_at_call":"0505","routes":[route]}
                  for site,key,point in (("801E7B58",103,[-175,80,100]),("801E7B64",104,[175,80,100]))]
        args=(0x0505,[{"index":i,"descriptor_offset":64+i*64} for i in range(2)],actions,[500,10,10,0,180])
        sites=(0x801E7B58,0x801E7B64)
        result,_,receipt=pose_free_shot_test(*args,variants,(240,240,12,12,8,4),sites)
        self.assertIn(b"body-motion=static-cruise\n",result)
        self.assertIn(b"move=basic-shot|b|10|airborne|28|none\n",result)
        self.assertNotIn(b"idle=",result)
        self.assertEqual([r["key"] for r in receipt["ordered_factory_calls"]],[103,104])
        variants[1]["required_model_at_call"]="0511"
        with self.assertRaisesRegex(ValueError,"sockets or debit"):
            pose_free_shot_test(*args,variants,(240,240,12,12,8,4),sites)

    def test_clocked_ground_pair_requires_the_reviewed_direct_handler_and_distinct_sockets(self):
        identity={"config":"8032F578","context_table":"8032F8F8","main_handler":"80125DAC"}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]]}
        actions={"profiles":[{**identity,"groups":[{"group":"B-shot","rows":[row]}]}]}
        route={**identity,**row,"action_group":"B-shot","context_rows":[[0,0]],"release_entry":"80125DE8",
               "local_stock_guards":[{"channel":0,"cost":1,"commit":True,"same_input_actor":True,"debit_site":"80125F28"}]}
        shots=[{"site":site,"key":key,"family":"type-0","basis":"credited-actor-joint","parent_model_matches":True,
                "joint_present_in_normal_rig":True,"source_joint_selector":1,"local_point":[x,10,20],
                "required_model_at_call":"0604","routes":[route]}
               for site,key,x in (("80125F50",43,20),("80125F5C",44,-20))]
        args=(0x0604,[{"index":i,"descriptor_offset":64+i*64} for i in range(2)],actions,[450,20,20,0,180])
        roles=[(role,0,i) for i,role in enumerate(LOCOMOTION)]
        def export():
            return pose_free_shot_test(*args,shots,(),(0x80125F50,0x80125F5C),
                                       role_clips=roles,pace={"native_units_per_tick_x100":750})
        data,_,receipt=export()
        self.assertIn(b"move=basic-shot|b|10|grounded|28|none\n",data)
        self.assertIn(b"release=basic-shot|12|shot-stock|1|shot\nrelease-followup=basic-shot|12|shot-second\n",data)
        self.assertEqual([r["key"] for r in receipt["ordered_factory_calls"]],[43,44])
        route["release_entry"]="8018C7D8"
        with self.assertRaisesRegex(ValueError,"paired factory order or clip context"):
            export()
        route["release_entry"]="80125DE8"
        shots[1]["local_point"]=shots[0]["local_point"]
        with self.assertRaisesRegex(ValueError,"sockets or debit"):
            export()

    def test_static_cruise_single_shot_requires_both_the_reviewed_phase_and_mode_one_stock(self):
        identity={"config":"80380AA0","context_table":"80380C78","main_handler":"801B29B8"}
        row={"state":0,"substate":0,"selector":45,"contexts":[[0,0]]}
        actions={"profiles":[{**identity,"groups":[{"group":"B-shot","rows":[row]}]}]}
        guard={"channel":0,"cost":1,"commit":True,"same_input_actor":True,"entry":"801B2AC8","debit_site":"801B2AE8"}
        route={**identity,**row,"action_group":"B-shot","context_rows":[[0,0]],"release_entry":"801B2AC8",
               "phase_table":"80380A80","local_stock_guards":[guard]}
        shot={"site":"801B2AFC","key":60,"family":"type-2","basis":"parent-joint","parent_model_matches":True,
              "joint_present_in_normal_rig":True,"source_joint_selector":1,"local_point":[0,0,40],"routes":[route]}
        args=(0x0F00,[{"index":i,"descriptor_offset":64+i*64} for i in range(2)],actions)
        cruise=(240,240,12,12,8,4)
        text,_,receipt=pose_free_shot_test(*args,[20,5,5,1,280],[shot],cruise,None)
        self.assertIn(b"stock-clock=shot-stock|add-on-debit\n",text)
        self.assertNotIn(b"release-followup=",text)
        self.assertEqual(receipt["source_phase_table"],"80380A80")
        with self.assertRaisesRegex(ValueError,"unsupported pose-free stock"):
            pose_free_shot_test(*args,[20,5,5,0,280],[shot],cruise,None)
        route["phase_table"]="80380EE8"
        with self.assertRaisesRegex(ValueError,"one guarded factory"):
            pose_free_shot_test(*args,[20,5,5,1,280],[shot],cruise,None)

    def test_violet_keeps_both_barrels_indexed_but_exports_only_the_zero_seed(self):
        proof=VIOLET_SHOT
        linker=ActionLinker(None,{},[])
        with patch.object(linker,"matches_text",return_value=True), \
             patch.object(linker,"leaf",side_effect=lambda _,__,i:proof["leaves"][i]), \
             patch.object(linker,"review_call",return_value=True) as calls:
            _,evidence,releases=linker.seeded_violet_shots(proof["wrapper"])
            self.assertEqual([r[1] for r in releases],[("type-0",16),("type-0",17)])
            self.assertEqual(evidence["reviewed_caller_model"],"0D05")
            self.assertIsNone(linker.seeded_violet_shots(0x80188EC4))
            calls.return_value=False
            self.assertIsNone(linker.seeded_violet_shots(proof["wrapper"]))
        identity={"config":"803626A0","context_table":"80362800","main_handler":"80188B24"}
        row={"state":0,"substate":0,"selector":45,"contexts":[[0,0]]}
        actions={"profiles":[{**identity,"groups":[{"group":"B-shot","rows":[row]}]}]}
        route={**identity,**row,"action_group":"B-shot","context_rows":[[0,0]],"release_entry":"80188C54",
               "phase_table":"80362664","reviewed_caller_model":"0D05","local_stock_guards":[{
                   "entry":"80188C54","debit_site":"80188CAC","channel":0,"cost":1,"commit":True,"same_input_actor":True}]}
        shots=[{"site":"80188CC8","key":key,"family":"type-0","basis":"credited-actor-joint","parent_model_matches":True,
                "joint_present_in_normal_rig":True,"source_joint_selector":1,"local_point":[x,0,41],
                "release_guard_candidate":evidence["release_guard_candidate"],"routes":[route]}
               for key,x in ((17,-15),(16,15))]
        joints=[{"index":i,"descriptor_offset":64+i*64} for i in range(2)]
        args=(0x0D05,joints,actions,[150,40,40,0,300],shots,(240,240,12,12,8,4),None)
        data,_,receipt=pose_free_shot_test(*args)
        self.assertIn(b"origin=shot|1|15|0|41|target-height\n",data)
        self.assertNotIn(b"release-followup=",data)
        self.assertEqual(receipt["ordered_factory_calls"],[{"key":16,"site":"80188CC8"}])
        route["reviewed_caller_model"]="0D01"
        with self.assertRaisesRegex(ValueError,"one guarded factory"):
            pose_free_shot_test(*args)

    def test_animated_cruise_pair_uses_the_exact_entry_clip_and_b_shot_route_only(self):
        profile={"config":"80332490","context_table":"80332810","main_handler":"8012B14C"}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]]}
        actions={"profiles":[{**profile,"groups":[{"group":"B-shot","rows":[row]}]}]}
        route={**profile,**row,"action_group":"B-shot","context_rows":[[0,0]],"release_entry":"8012B188",
               "local_stock_guards":[{"channel":0,"cost":1,"commit":True,"same_input_actor":True,"debit_site":"8012B3CC"}]}
        releases=[{"site":site,"key":key,"family":"type-2","basis":"parent-joint","parent_model_matches":True,
                   "joint_present_in_normal_rig":True,"source_joint_selector":joint,"local_point":[x,0,0],
                   "required_model_at_call":"061B","routes":[route,{**route,"action_group":"B-charge"}]}
                  for site,key,joint,x in (("8012B410",83,1,-200),("8012B41C",84,0,200))]
        joints=[{"index":i,"descriptor_offset":64+i*64} for i in range(2)]
        args=(0x061B,joints,actions,[250,5,5,0,120],releases,(240,240,12,12,8,4),(0x8012B410,0x8012B41C))
        result,_,receipt=pose_free_shot_test(*args,cruise_entry=(2,0))
        self.assertIn(b"cruise-entry=2:0\n",result)
        self.assertIn(b"move=basic-shot|b|10|airborne|28|none\n",result)
        self.assertNotIn(b"body-motion=",result)
        self.assertEqual(receipt["source_cruise_entry"],(2,0))
        with self.assertRaisesRegex(ValueError,"exact motion or release evidence"):
            pose_free_shot_test(*args)
        route["action_group"]="B-charge"
        with self.assertRaisesRegex(ValueError,"paired factory order or clip context"):
            pose_free_shot_test(*args,cruise_entry=(2,0))

    def test_static_ground_phase_shot_does_not_borrow_the_sibling_debit(self):
        identity={"config":"80380F08","context_table":"803810E0","main_handler":"801B36F4"}
        row={"state":0,"substate":0,"selector":45,"contexts":[[0,0]]}
        actions={"profiles":[{**identity,"groups":[{"group":"B-shot","rows":[row]}]}]}
        guard={"channel":0,"cost":1,"commit":True,"same_input_actor":True,"entry":"801B3824","debit_site":"801B392C"}
        route={**identity,**row,"action_group":"B-shot","context_rows":[[0,0]],"release_entry":"801B3824",
               "phase_table":"80380EE8","local_stock_guards":[guard]}
        shot={"site":"801B394C","key":61,"family":"type-2","basis":"parent-joint","parent_model_matches":True,
              "joint_present_in_normal_rig":True,"source_joint_selector":1,"local_point":[0,0,40],
              "required_model_at_call":"0F02","routes":[route]}
        args=(0x0F02,[{"index":i,"descriptor_offset":64+i*64} for i in range(2)],actions,[40,5,5,1,280],[shot],(),None)
        def export():
            return pose_free_shot_test(*args,pace={"native_units_per_tick_x100":333})
        data,_,receipt=export()
        self.assertIn(b"body-motion=static-grounded\n",data)
        self.assertIn(b"move=basic-shot|b|10|grounded|28|none\n",data)
        self.assertIn(b"stock-clock=shot-stock|add-on-debit\n",data)
        self.assertNotIn(b"air=",data)
        self.assertEqual(receipt["native_ground_pace"],333)
        guard["debit_site"]="801B389C"
        with self.assertRaisesRegex(ValueError,"sockets or debit"):
            export()

    def test_shot_readiness_does_not_confuse_asset_refusal_with_missing_release(self):
        identity={"config":"80300000","context_table":"80350000","main_handler":"80000000",
                  "state":0,"substate":0,"selector":44}
        entry={"clip":[3,0],"archive_offset":128}
        row={**identity,"contexts":[[0,0]],"entry_clip_candidate":entry}
        profile={**identity,"groups":[{"group":g,"mode":0,"rows":[row] if g=="B-shot" else []} for g in PREVIEW_GROUPS]}
        clips=[{"archive_offset":at,"aobj_timing":"indexed","public_joint_count":2} for at in (64,128)]
        summary={"action_candidates":{"profiles":[profile]},"role_preview_clips":[(g,0,0) for g in LOCOMOTION],
                 "lab_ground_pace":{"native_units_per_tick_x100":500},"logical_joints":[None,None],
                 "raw_animation_slots":[(0,0,64),(3,0,128)],"local_clip_timing":clips,
                 "raw_level_rows":[(100,5,5,0,180,0,0,0,0)]}
        report=shot_readiness(summary,[])
        self.assertEqual(report["contexts"][0]["status"],"missing-correlated-release")
        clips[1]["strict_metadata_refusals"]=["animjoint-constraints-or-flags"]
        report=shot_readiness(summary,[])
        self.assertEqual(report["status"],"strict-clip-metadata-refusal")
        self.assertEqual(report["contexts"][0]["clip_refusals"][0]["clip"],[3,0])
        clips[1].pop("strict_metadata_refusals")
        debit={"normal_level_operand_channel":0,"cost":1,"debit_site":"80000500"}
        route={**identity,"action_group":"B-shot","context_rows":[[0,0]],"local_stock_guards":[debit]}
        hit={"site":"80000514","key":2,"family":"type-0","basis":"credited-actor-joint",
             "joint_present_in_normal_rig":True,"parent_model_matches":True,"routes":[route]}
        self.assertEqual(shot_readiness(summary,[hit])["status"],"needs-strict-native-trial")
        self.assertNotIn("basic_shot_battle_test",summary)

    def test_beta_installer_only_takes_the_override_for_exact_model_010a(self):
        prefix=(0x38000000,0x3C808037,0x2C000010,0x3884FA78,0x40800084,
                *(w for i in range(16) for w in (0x80040000+4*i,0x900304B4+4*i)))
        words=list(prefix)+[0x60000000]*107
        gate=(BETA_MODEL_CHECK-BETA_INSTALLER)//4
        words[gate-5:gate]=[0x3C808037,0x38A4FC50,0x90A304B0,0x38A4FFA0,0x90A304EC]
        words[gate:gate+len(BETA_OVERRIDE)]=BETA_OVERRIDE
        class Dol:
            def read(self,at,size,kind=None):
                if (at,size,kind)==(BETA_INSTALLER,576,"text"):
                    return struct.pack(">144I",*words)
                if kind=="text" and size==4 and at>=BETA_INSTALLER:
                    return struct.pack(">I",words[(at-BETA_INSTALLER)//4])
                if kind=="data":
                    return bytes(size)
                raise ValueError("unexpected read")
        base,_,_=installer_tables(Dol(),BETA_INSTALLER,0x0105)
        other,_,_=installer_tables(Dol(),BETA_INSTALLER,0x010A)
        self.assertEqual([r[:2] for r in base],[(0x8036FC50,0x8036FFA0)])
        self.assertEqual([r[:2] for r in other],[(0x8036FD68,0x8036FFB8)])
        self.assertEqual([r["config"] for r in movement_config_candidates(Dol(),BETA_INSTALLER,0x0105)["candidates"]],
                         ["8036FC50"])
        words[gate+1]=0x2C000109
        unknown,_,_=installer_tables(Dol(),BETA_INSTALLER,0x0105)
        self.assertEqual([r[:2] for r in unknown],[(0x8036FC50,0x8036FFA0),(0x8036FD68,0x8036FFB8)])

    def test_neutral_side_shot_refuses_the_unreviewed_dispatch_config_or_key_bytes(self):
        model=0x0100
        route=SIDE_ROUTES[model]
        profile={"config":route["config"],"main_handler":f'{route["main"]:08X}'}
        roots=(route["root"],0,0,0)
        row={"state":0,"substate":0,"selector":44}
        class Fake:
            keys=bytes((0,1))
            def matches_text(self,*_):
                return True
            def read_word(self,at):
                return dict(route["rows"])[at]
            def review_call(self,*_):
                return True
            def table_choice(self,*_):
                return route["program"]
            def data(self,*_):
                return self.keys
        select={"kind":"directional_select","group":2,"choices":[1,0,2],"preceding_waits":2,"start_frame":11}
        cue={"kind":"upper_controls","address":route["cue"],"raw":"0201010001000000"}
        fake=Fake()
        with patch("tools.analyze.retail_side_shot.decode_program",
                   return_value={"status":"terminal_wait","commands":[select,cue]}), \
             patch("tools.analyze.retail_side_shot.local_upper_poses",return_value=[{"clip":[3,n]} for n in (1,0,2)]):
            def bind(p=profile):
                return neutral_side_shot(fake,model,p,route["code"][0][0],row,roots,{},0x8004BEB8)
            binding,back=bind()
            self.assertEqual((binding["clip"],binding["source_side_choice"],back["keys"]),([3,0],"zero",(0,1)))
            self.assertIsNone(bind({**profile,"config":"8036FD68"}))
            fake.keys=b"\x01\x00"
            self.assertIsNone(bind())

    def test_theta_cue_requires_both_bytes_after_the_reviewed_direction_select(self):
        class Fake:
            data=None
            def matches_text(self,*_):
                return True
            def read_word(self,_):
                return THETA_ROUTE["hook"][0]
            def review_call(self,*_):
                return True
            def table_choice(self,root,group,choice):
                self.asserted=(root,group,choice)
                return THETA_ROUTE["program"]
        linker=Fake()
        root=(THETA_ROUTE["program_root"],0,0,0)
        phases=[(THETA_ROUTE["entry"],"phase-zero-candidate"),
                (THETA_ROUTE["consumer"][0],"phase-1-entry-increment-candidate")]
        requests=[{"kind":"projectile","site":"801A1B48","family":"type-0","key":106,
                   "release_relay":"801A1AF0","required_model_at_call":"0906"}]
        select={"kind":"directional_select","group":2,"choices":[4,3,5],"preceding_waits":1,"start_frame":3}
        cue={"kind":"upper_controls","address":THETA_ROUTE["cue"],"raw":"02ff010100000000"}
        poses=[{"clip":[3,n]} for n in (17,16,18)]
        def bind(model=0x0906):
            return theta_shot_pose(linker,THETA_ROUTE["installer"][0],THETA_ROUTE["dispatch"][0],
                                   phases,model,root,{},requests,0x8004BEB8)
        with patch("tools.analyze.retail_seeded_shot_pose.decode_program",
                   return_value={"status":"terminal_wait","commands":[select,cue]}), \
             patch("tools.analyze.retail_seeded_shot_pose.local_upper_poses",return_value=poses):
            pose=bind()
            self.assertEqual((pose["clip"],pose["program"],pose["cue_bytes"]),([3,16],[2,6],[1,1]))
            self.assertEqual(linker.asserted,(root[0],2,6))
            self.assertIsNone(bind(0x090B))
            cue["raw"]="02ff010000000000"
            self.assertIsNone(bind())

    def test_seeded_throw_keeps_direction_order_and_nonzero_source_start(self):
        handler=0x800D0454
        route=THROW_ROUTES[handler]
        roots=(route["program_root"],0,0x8030CF40,0)
        phases=[(route["entry"],"phase-zero-candidate")]
        release={"kind":"projectile","family":"type-14","key":1,"release_relay":"800D0C50"}
        local={(3,0):64,(3,1):128,(3,2):192}
        class Fake:
            data=None
            def matches_text(self,*_):
                return True
            def read_word(self,_):
                return route["hook"][0]
            def review_call(self,entry,site,target):
                return (entry,site,target)==(route["entry"],route["request"],0x8004BEB8)
            def table_choice(self,root,group,choice):
                if group!=2:
                    raise ValueError("unexpected group")
                return route["program"] if root==roots[0] and choice==0 else choice
        program={"status":"terminal_wait","commands":[
            {"kind":"directional_select","preceding_waits":0,"start_frame":7,"group":2,"choices":[2,0,1]},
            {"kind":"upper_controls","raw":"02ff010000000000","address":route["cue"]}]}
        def bind():
            return throw_shot_pose(Fake(),route["installer"][0],handler,phases,0x0002,roots,local,[release],0x8004BEB8)
        with patch("tools.analyze.retail_seeded_shot_pose.decode_program",return_value=program), \
             patch("tools.analyze.retail_seeded_shot_pose.descriptor_lanes",
                   side_effect=lambda _,at,mask:{"lanes":[{"group":3,"index":at}]}):
            pose=bind()
            self.assertEqual(pose["clip"],[3,0])
            self.assertEqual([b["clip"] for b in pose["source_pose_alternatives"]],[[3,2],[3,0],[3,1]])
            self.assertEqual(pose["source_start_frame"],7)
            release["release_relay"]="80070558"
            self.assertIsNone(bind())
            release["release_relay"]="800D0C50";local.pop((3,1))
            self.assertIsNone(bind())

    def test_double_ninja_no_clone_pose_requires_both_model_gated_calls(self):
        route=DOUBLE_NINJA_ROUTE
        roots=(route["program_root"],0,0,0)
        phases=[(route["entry"],"phase-zero-candidate"),(0x8006FC24,"phase-1-entry-increment-candidate")]
        releases=[{"kind":"projectile","family":"type-14","key":key,"site":site,"required_model_at_call":"0003",
                   "release_relay_site":"8006FCA0","release_relay":"80070558"}
                  for key,site in ((0,"800705C4"),(2,"800705DC"))]
        fake=ActionLinker(None,{},[])
        program={"status":"terminal_wait","commands":[
            {"kind":"select_descriptor","preceding_waits":0,"start_frame":7,"group":2,"choice":0},
            {"kind":"upper_controls","raw":"02ff010000000000","address":route["cue"]}]}
        poses=[{"clip":[3,0],"archive_offset":128,"descriptor":[2,0]}]
        def bind(installer=0x8006F60C):
            return throw_shot_pose(fake,installer,0x8006FB44,phases,3,roots,{},releases,0x8004BEB8)
        with patch.object(fake,"matches_text",return_value=True), \
             patch.object(fake,"read_word",return_value=route["hook"][0]), \
             patch.object(fake,"review_call",return_value=True), \
             patch.object(fake,"table_choice",return_value=route["program"]), \
             patch("tools.analyze.retail_seeded_shot_pose.decode_program",return_value=program), \
             patch("tools.analyze.retail_seeded_shot_pose.local_upper_poses",return_value=poses):
            pose=bind()
            self.assertEqual(pose["source_clone_byte"],{"offset":"7C8","value":0,"branch":"800705D0"})
            self.assertIsNone(bind(0x8006F4F8))
            releases[1]["release_relay_site"]="8006FEE4"
            self.assertIsNone(bind())
        context={"config":"8033AF78","context_table":"8033B180","main_handler":"8006FACC",
                 "selector":44,"state":0,"substate":0}
        guard={"channel":0,"cost":1,"commit":True,"same_input_actor":True,"debit_site":"8007058C"}
        link={**context,"action_group":"B-shot","context_rows":[[0,0]],"local_stock_guards":[guard],
              "relay_site":"8006FCA0","relay_handler":"80070558"}
        exact={"raw_context":[0,0],"routes":[{**context,**pose}]}
        indexed=[{**h,"routes":[link]} for h in releases]
        self.assertEqual(match_shot_context(exact,indexed),([indexed[0]],guard))
        exact["routes"][0].pop("release_site")
        with self.assertRaisesRegex(ValueError,"one guarded factory"):
            match_shot_context(exact,indexed)

    def test_actor_release_sound_matches_the_same_paid_context_not_child_sound(self):
        profile={"config":"80326CF0","context_table":"80327058","main_handler":"8010F86C",
                 "state":0,"substate":4,"selector":45}
        clip={"raw_context":[4,0],"routes":[{**profile,"release_relay_site":"8010FDF4"}]}
        route={**profile,"action_group":"B-shot","context_rows":[[4,0]],
               "relay_site":"8010FDF4","relay_handler":"8010FE78"}
        cue={"site":"8010FF18","source_sound_id":162,"required_model_at_call":"080C",
             "status":"static-tone-record","bank":"snd_com02","tone":34,"routes":[route]}
        debit={"entry":"8010FE78","debit_site":"8010FF04"}
        first={"site":"8010FF24"}
        actual=bind_actor_release_sound(clip,[cue],debit,first,6)
        self.assertEqual((actual["path"],actual["native_release_tick"]),("audio/release-snd_com02-34.pcm",6))
        with self.assertRaisesRegex(ValueError,"context disagrees"):
            bind_actor_release_sound(clip,[{**cue,"routes":[{**route,"relay_site":"8010FCB0"}]}],debit,first,6)
        with self.assertRaisesRegex(ValueError,"paid actor sound"):
            bind_actor_release_sound(clip,[cue],{**debit,"debit_site":"8010FF20"},first,6)

    def test_launch_pcm_resamples_one_bounded_mono_pass(self):
        raw=BytesIO()
        with wave.open(raw,"wb") as out:
            out.setparams((1,2,24000,0,"NONE","not compressed"))
            out.writeframes(struct.pack("<2h",-200,200))
        self.assertEqual(struct.unpack("<8h",launch_pcm(raw.getvalue())),(-200,-200,0,0,200,200,200,200))
        with self.assertRaisesRegex(ValueError,"invalid launch WAV"):
            launch_pcm(b"\x00"*44)
        with self.assertRaisesRegex(ValueError,"short launch WAV"):
            launch_pcm(raw.getvalue()[:-2])
        self.assertIsNone(pilot_launch_tone({"status":"outside-qualified-normal-bank-domain","source_sound_id":-1}))
        with self.assertRaisesRegex(ValueError,"exact launch tone"):
            pilot_launch_tone({"status":"outside-qualified-normal-bank-domain","source_sound_id":384})

    def test_long_impact_sample_does_not_raise_the_launch_budget(self):
        raw=BytesIO()
        with wave.open(raw,"wb") as out:
            out.setparams((1,2,8000,0,"NONE","not compressed"))
            out.writeframes(struct.pack("<h",300)*28000)
        with self.assertRaisesRegex(ValueError,"frame budget"):
            launch_pcm(raw.getvalue())
        extended=launch_pcm(raw.getvalue(),max_seconds=4)
        self.assertEqual(len(extended),168000*4)
        self.assertEqual(extended[-4:],struct.pack("<2h",300,300))

    def test_seeded_rifle_pose_preserves_all_direction_branches_without_fallback(self):
        handler=0x800C9590
        route=RIFLE_ROUTES[handler]
        roots=(route["program_root"],0,0x803095A0,0)
        release={"kind":"projectile","family":"type-2","key":21,
                 "required_model_at_call":"0600","release_relay":"800C9F40"}
        class Fake:
            data=None
            def matches_text(self,*_):
                return True
            def read_word(self,_):
                return self.hook
            def review_call(self,entry,site,target):
                return (entry,site,target)==(route["entry"],route["request"],0x8004BEB8)
            def table_choice(self,root,group,choice):
                if group!=2:
                    raise ValueError("unexpected group")
                return route["program"] if root==roots[0] and choice==0 else choice
        linker=Fake();linker.hook=route["hook"][0]
        program={"status":"terminal_wait","commands":[
            {"kind":"directional_select","preceding_waits":2,"start_frame":0,"group":2,"choices":[7,6,8]},
            {"kind":"upper_controls","raw":"0201010001000000","address":0x8030906A}]}
        local={(3,11):64,(3,12):128,(3,13):192}
        phases=[(route["entry"],"phase-zero-candidate")]
        def bind(model=0x0600):
            return rifle_shot_pose(linker,route["installer"][0],handler,phases,model,roots,local,[release],0x8004BEB8)
        with patch("tools.analyze.retail_seeded_shot_pose.decode_program",return_value=program), \
             patch("tools.analyze.retail_seeded_shot_pose.descriptor_lanes",
                   side_effect=lambda _,at,mask:{"lanes":[{"group":3,"index":at+5}]}):
            pose=bind()
            self.assertEqual(pose["clip"],[3,11])
            self.assertEqual([x["clip"] for x in pose["source_direction_alternatives"]],[[3,12],[3,11],[3,13]])
            self.assertTrue(pose["seed_candidate"]["requires_actor_word_5e0_bit_0_clear"])
            self.assertIsNone(bind(0x0616))
            local.pop((3,13))
            self.assertIsNone(bind())
            local[(3,13)]=192; linker.hook=0
            self.assertIsNone(bind())

    def test_basic_shot_profile_is_scoped_to_exact_model_and_evidence(self):
        joints=[{"index":i,"descriptor_offset":32+64*i} for i in range(4)]
        roles=[(name,0,c) for name,c in (("ground-stop",0),("ground-travel",1),("air-continuation",24),
                                       ("ground-dash-forward",2))]
        route={"config":"80300000","context_table":"80350000","main_handler":"80100000",
               "state":0,"substate":0,"selector":44}
        guard={"channel":0,"cost":1,"commit":True,"same_input_actor":True,"debit_site":"8015AAAC"}
        clips=[{"id":"b-shot-c0-s0-p0","clip":[1,9],"request_choice_condition":{"set":False},
                "raw_context":[0,0],"routes":[route]}]
        level=[400,5,5,0,999,0,0,0,0] # raw preset must not become native timing
        launch={"family":"type-2","key":85,"site":"8015AAE4","basis":"parent-joint",
                "joint_present_in_normal_rig":True,"parent_model_matches":True,"required_model_at_call":"061E",
                "source_joint_selector":3,"local_point":[0,36,59],
                "routes":[{**route,"action_group":"B-shot","context_rows":[[0,0]],"local_stock_guards":[guard]}]}
        pace={"native_units_per_tick_x100":833}
        gameplay,pose,receipt=basic_shot_test(0x061E,joints,roles,clips,pace,level,[launch])
        self.assertIn(b"health=400\n",gameplay)
        self.assertIn(b"origin=shot|3|0|36|59|target-height\n",gameplay)
        self.assertIn(b"stock=shot-stock|5|180\n",gameplay)
        self.assertEqual(pose[:12],b"GFCP\x00\x01\x00\x04\x01\x04\x04\x04")
        self.assertEqual(struct.unpack(">4I",pose[-16:]),(32,96,160,224))
        self.assertFalse(receipt["retail_behavior_verified"])
        self.assertIsNone(basic_shot_test(0x0628,joints,roles,clips,pace,level,[launch]))
        scoped,*_=basic_shot_test(0x0610,joints,roles,clips,pace,level,[{**launch,"key":53,"required_model_at_call":"0610"}])
        self.assertIn(b"body-record=0\n",scoped)
        with self.assertRaisesRegex(ValueError,"socket or stock"):
            basic_shot_test(0x061E,joints,roles,clips,pace,level,[{**launch,"basis":"parent-root-matrix"}])
        with self.assertRaisesRegex(ValueError,"one guarded factory"):
            basic_shot_test(0x061E,joints,roles,clips,pace,level,[launch,{**launch,"key":86}])
        with self.assertRaisesRegex(ValueError,"exact clip context"):
            basic_shot_test(0x061E,joints,roles,[{**clips[0],"raw_context":[1,0]}],pace,level,[launch])
        martial={**launch,"family":"type-0","key":12,"basis":"credited-actor-joint","required_model_at_call":"0800"}
        converted,*_=basic_shot_test(0x0800,joints,roles,[{**clips[0],"id":"b-shot-c0-s0"}],
                                     pace,[400,5,5,1,300,0,0,0,0],[martial])
        self.assertIn(b"stock-clock=shot-stock|add-on-debit\n",converted)
        paired_clip={**clips[0],"id":"b-shot-c0-s0","routes":[{**route,"authored_direction_choice":1,
                                                            "seed_candidate":{"value":0}}]}
        first={**launch,"key":51,"site":"800C9FB0","required_model_at_call":"0608"}
        second={**first,"key":52,"site":"800C9FBC","local_point":[0,36,64]}
        sites=(0x800C9FB0,0x800C9FBC)
        result,_,receipt=basic_shot_test(0x0608,joints,roles,[paired_clip],pace,level,[second,first],paired_sites=sites)
        self.assertIn(b"origin=shot|3|0|36|59|target-height\n",result)
        self.assertIn(b"origin=shot-second|3|0|36|64|target-height\n",result)
        self.assertIn(b"release-followup=basic-shot|3|shot-second\n",result)
        self.assertEqual([r["key"] for r in receipt["ordered_factory_calls"]],[51,52])
        with self.assertRaisesRegex(ValueError,"one shared debit"):
            changed={**second,"routes":[{**second["routes"][0],"local_stock_guards":[{**guard,"debit_site":"800C9F74"}]}]}
            basic_shot_test(0x0608,joints,roles,[paired_clip],pace,level,[first,changed],paired_sites=sites)
        with self.assertRaisesRegex(ValueError,"ordered sibling"):
            basic_shot_test(0x0608,joints,roles,[paired_clip],pace,level,[first,second])
        with self.assertRaisesRegex(ValueError,"rig extent"):
            whole_body_pose(joints[::-1])

    def test_rifle_pair_cannot_branch_or_reenter_between_calls(self):
        first,second=0x800C9FB0,0x800C9FBC
        words={first+4:0x7FE3FB78,first+8:0x38800034}
        outgoing={pc:(pc+4,) for pc in range(first,second,4)}
        incoming={pc+4:{pc} for pc in outgoing}
        with patch.object(ActionLinker,"matches_text",return_value=True), \
             patch.object(ActionLinker,"graph",return_value=(words,incoming,outgoing,{},0,False)), \
             patch.object(ActionLinker,"review_call",return_value=True):
            self.assertEqual(ActionLinker(None,{},[]).ordered_shot_sites(0x0608),(first,second))
            incoming[second].add(first-4)
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0608))
            incoming[second].remove(first-4); words[first+4]=0x40820008
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0608))

    def test_clocked_direct_pair_pins_handler_signature_and_factory_keys(self):
        entry,first,second=0x80125DE8,0x80125F50,0x80125F5C
        words={first+4:0x7FE3FB78,first+8:0x3880002C}
        edges={pc:(pc+4,) for pc in range(first,second,4)}
        incoming={pc+4:{pc} for pc in edges}
        with patch.object(ActionLinker,"matches_text",return_value=True) as signature, \
             patch.object(ActionLinker,"graph",return_value=(words,incoming,edges,{},0,False)), \
             patch.object(ActionLinker,"review_call",return_value=True) as call:
            self.assertEqual(ActionLinker(None,{},[]).ordered_shot_sites(0x0604),(first,second))
            self.assertEqual(signature.call_args.args[:2],(entry,0x1F4))
            self.assertEqual([c.args for c in call.call_args_list],
                             [(entry,first,0x80082824,43),(entry,second,0x80082824,44)])
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0618))
            signature.return_value=False
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0604))

    def test_flying_saucer_pair_requires_the_phase_table_and_straight_second_call(self):
        first,second=0x80189420,0x8018942C
        words={first+4:0x7FE3FB78,first+8:0x38800037}
        edges={pc:(pc+4,) for pc in range(first,second,4)}
        incoming={pc+4:{pc} for pc in edges}
        with patch.object(ActionLinker,"dispatch",return_value=0x8036299C) as dispatch, \
             patch.object(ActionLinker,"matches_text",return_value=True), \
             patch.object(ActionLinker,"graph",return_value=(words,incoming,edges,{},0,False)), \
             patch.object(ActionLinker,"review_call",return_value=True) as call:
            self.assertEqual(ActionLinker(None,{},[]).ordered_shot_sites(0x0D03),(first,second))
            self.assertEqual([c.args for c in call.call_args_list],
                             [(0x801893B4,first,0x800C3BE0,54),(0x801893B4,second,0x800C3BE0,55)])
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0D02))
            incoming[second].add(first-4)
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0D03))
            incoming[second].remove(first-4)
            dispatch.return_value=0x80380A80
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0D03))

    def test_seiryu_requires_straight_order_through_the_third_factory_call(self):
        first,second,third=0x800CD2D8,0x800CD2E4,0x800CD2F0
        words={first+4:0x7FC3F378,first+8:0x38800017,second+4:0x7FC3F378,second+8:0x38800018}
        edges={pc:(pc+4,) for pc in range(first,third,4)}
        incoming={pc+4:{pc} for pc in edges}
        with patch.object(ActionLinker,"matches_text",return_value=True), \
             patch.object(ActionLinker,"graph",return_value=(words,incoming,edges,{},0,False)), \
             patch.object(ActionLinker,"review_call",return_value=True) as call:
            self.assertEqual(ActionLinker(None,{},[]).ordered_shot_sites(0x0602),(first,second,third))
            self.assertEqual([c.args[-1] for c in call.call_args_list],[22,23,24])
            words[second+8]=0x38800019
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0602))
            words[second+8]=0x38800018
            edges[second]=(second+4,third+4)
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x0602))

    def test_six_shot_chain_keeps_every_model_gated_site(self):
        sites=tuple(range(0x800CD308,0x800CD344+1,12))
        words={at+4:0x7FC3F378 for at in sites[:-1]}
        words.update({at+8:0x38800000|key for at,key in zip(sites[:-1],range(106,111),strict=True)})
        edges={pc:(pc+4,) for pc in range(sites[0],sites[-1],4)}
        incoming={pc+4:{pc} for pc in edges}
        with patch.object(ActionLinker,"matches_text",return_value=True), \
             patch.object(ActionLinker,"graph",return_value=(words,incoming,edges,{},0,False)), \
             patch.object(ActionLinker,"review_call",return_value=True) as call:
            self.assertEqual(ActionLinker(None,{},[]).ordered_shot_sites(0x060A),sites)
            self.assertEqual([c.args[-1] for c in call.call_args_list],list(range(105,111)))
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x060C))
            incoming[sites[-1]].add(sites[0])
            self.assertIsNone(ActionLinker(None,{},[]).ordered_shot_sites(0x060A))

    def test_cued_pair_pose_requires_both_followups_on_the_cued_parent(self):
        roots=(0x80325C80,0,0,0)
        rows=[{"kind":"projectile","family":"type-2","site":site,"key":key,
               "release_relay_site":"8010FDF4","required_model_at_call":"080C"}
              for site,key in (("8010FF24",99),("8010FF30",100))]
        command={"kind":"upper_controls","raw":"02ff010000000000","address":0x80326042,"preceding_waits":1}
        with patch.object(ActionLinker,"ordered_shot_sites",return_value=(0x8010FF24,0x8010FF30)), \
             patch.object(ActionLinker,"matches_text",return_value=True), \
             patch.object(ActionLinker,"review_call",return_value=True), \
             patch.object(ActionLinker,"calls",return_value=(((0x8010FDBC,"program",(1,2,3),"literal-block"),),0,False)), \
             patch.object(ActionLinker,"table_choice",return_value=0x80326026), \
             patch.object(ActionLinker,"entry_clip_binding",return_value={"clip":[3,33]}), \
             patch("tools.analyze.retail_action_links.decode_program",return_value={"status":"terminal_wait","commands":[command]}):
            link=ActionLinker(None,{},[])
            self.assertEqual(link.cued_pair_pose(0x8010FD44,0x080C,roots,{},rows)["cue_command"],"80326042")
            self.assertIsNone(link.cued_pair_pose(0x8010FC7C,0x080C,roots,{},rows))
            self.assertIsNone(link.cued_pair_pose(0x8010FD44,0x080C,roots,{},rows[:1]))
            rows[1]["release_relay_site"]="8010FCB0"
            self.assertIsNone(link.cued_pair_pose(0x8010FD44,0x080C,roots,{},rows))
            rows[1]["release_relay_site"]="8010FDF4"; command["raw"]="02ffff0000000000"
            self.assertIsNone(link.cued_pair_pose(0x8010FD44,0x080C,roots,{},rows))

    def test_local_stock_guard_requires_success_edge_and_same_actor_on_both_calls(self):
        entry=0x80003000
        # Preserve actor in r31 across calls. subf can modify another GPR.
        code=(0x7C7F1B78,0x7C001850,0x38800000,0x38A00001,0x38C00001,0x48000001,
              0x2C030000,0x41820010,0x7FE3FB78,0x38800003,0x48000001,0x4E800020)
        words={entry+i*4:word for i,word in enumerate(code)}
        n=lambda i:entry+i*4
        debit,branch,release=n(5),n(7),n(10)
        source_edges={n(i):(n(i+1),) for i in range(11)}
        source_edges[branch]=(n(8),n(11))
        class Linker:
            altered=0; boundary=0; exhausted=False
            def graph(self,start,*_):
                self.start=start
                edges=dict(source_edges)
                current=dict(words)
                if self.altered==1: # entry can bypass debit success entirely
                    edges[n(1)]=(n(2),n(8))
                if self.altered==2: # failure can retry the factory path
                    edges[n(11)]=(n(8),)
                if self.altered==3:
                    current[n(8)]=0x38600000 # r3 is now null, not the actor
                if self.altered==4:
                    current[n(0)]=0x7C7E1B78 # copy r3 to r30 instead
                if self.altered==5:
                    current[n(1)]=0x6C7F8000 # XOR writes r31, so no actor proof
                if self.altered==6:
                    current[debit]=0x41820001 # conditional call is not a proven debit
                if self.altered==7:
                    current[n(1)]=0x7FE01850 # subf writes r31, destroying the saved actor
                incoming={}
                for pc,dests in edges.items():
                    for dest in dests:
                        incoming.setdefault(dest,set()).add(pc)
                return current,incoming,edges,{debit:DEBIT,release:0x80082824},self.boundary,self.exhausted
        fixture=Linker()
        (guard,)=StockGuardIndex(fixture).handler_guards(entry)[release]
        self.assertEqual((guard["channel"],guard["cost"],guard["failure_branch"]),(0,1,f"{n(11):08X}"))
        self.assertTrue(guard["same_input_actor"])
        for fixture.altered in range(1,8):
            self.assertEqual(StockGuardIndex(fixture).handler_guards(entry),{})
        fixture.altered=0
        words[n(1)]=0x6C608000 # xoris writes r0, not r31
        self.assertIn(release,StockGuardIndex(fixture).handler_guards(entry))
        fixture.altered=0; fixture.boundary=1
        self.assertEqual(StockGuardIndex(fixture).handler_guards(entry),{})

    def test_stock_rows_keep_zero_and_over_limit_initial_values(self):
        rows=[(100,0,1,0,600,4,4,1,300)]*9+[(150,23,22,0,60,4,4,3,300)]
        a,b=stock_rows(rows)
        self.assertEqual(a["levels"][0],{"level":1,"initial":0,"limit":1,"mode":0,"preset":600})
        self.assertTrue(a["levels"][9]["initial_exceeds_limit"])
        self.assertEqual((a["levels"][9]["initial"],a["levels"][9]["limit"]),(23,22))
        self.assertEqual(b["levels"][9]["mode"],3)
        with self.assertRaisesRegex(ValueError,"level row extent"):
            stock_rows(rows[1:])
        class Blank:
            def read(self,_,size,kind):
                return bytes(size)
        with self.assertRaisesRegex(ValueError,"stock operand source signature"):
            stock_schema(Blank())

    def test_release_stock_requires_matching_handler_commit_and_failure_branch(self):
        entry,site,debit=0x8010FE78,0x8010FF24,0x8010FF04
        words={debit-12:0x38800000,debit-8:0x38A00001,debit-4:0x38C00001,
               debit+4:0x2C030000,debit+8:0x41820028,site:0x4BF43CBD}
        incoming={pc:{pc-4} for pc in range(debit-8,debit+4,4)}
        class Linker:
            def graph(self,start,*_):
                self.entry=start
                return words,incoming,{},{},0,False
            def review_call(self,start,call,target):
                return (start,call,target)==(entry,debit,0x8006DBE0)
        record={"site":f"{site:08X}","routes":[{"relay_handler":f"{entry:08X}"}]}
        attach_release_stock([record],Linker())
        self.assertEqual((record["stock_candidate"]["channel"],record["stock_candidate"]["cost"]),(0,1))
        self.assertEqual(record.pop("stock_candidate")["normal_level_operand_channel"],0)
        words[debit-12]=0x38800002
        attach_release_stock([record],Linker())
        self.assertEqual(record["stock_candidate"]["channel"],2)
        self.assertIsNone(record["stock_candidate"]["normal_level_operand_channel"])
        record.pop("stock_candidate")
        words[debit-4]=0x38C00000
        attach_release_stock([record],Linker())
        self.assertNotIn("stock_candidate",record)
        words[debit-4]=0x38C00001
        words[debit+8]=0x41820004
        attach_release_stock([record],Linker())
        self.assertNotIn("stock_candidate",record)
        words[debit+8]=0x41820028
        unbound={**record,"routes":[]}
        attach_release_stock([unbound],Linker())
        self.assertNotIn("stock_candidate",unbound)

    def test_program_controls_require_upper_lane_and_exact_release_context(self):
        raw=bytes.fromhex(
            "02ff000000000000"  # initialize both bytes
            "00020009"          # mode/threshold, not a fixed tick delay
            "0201030100040000"  # reviewed release tuple
            "00020004"
            "02ffff0000000000"  # signed recovery
            "0001ffff")         # terminal wait
        start=0x802C1F20
        class Linker:
            def table_choice(self,root,group,choice):
                if (root,group,choice)!=(0x80300000,2,0):
                    raise ValueError("unqualified program")
                return start
            def data(self,address,size):
                return raw[address-start:address-start+size]
        program={"kind":"program","site":"80076B28","scope":"phase-zero-candidate",
                 "lanes":15,"program_group":2,"program_choice":0}
        release={"kind":"projectile","site":"80076C10","phase_table":"804338A0",
                 "phase_transition_calls":["80076B30"],"family":"type-0","key":12,
                 "release_guard_candidate":{"actor_byte_equals":{"541":0,"1CEF":3,"1CF0":1}}}
        row={"state":0,"substate":0,"selector":44,"contexts":[[0,0]],
             "request_candidates":[program,release]}
        actions={"profiles":[{"config":"80300004","context_table":"80300100","main_handler":"80003000",
                              "program_and_descriptor_roots":["80300000","0","0","0"],
                              "groups":[{"group":"B-shot","rows":[row]}]}]}
        index=ProgramControlIndex(Linker())
        result=index.collect(actions)
        (candidate,)=result["programs"]
        self.assertEqual([r["actor_1cef"] for r in candidate["writes"]],[0,3,-1])
        self.assertEqual(candidate["wait_commands"][0],{"address":"802C1F28","flags":2,"threshold":9})
        (join,)=result["reviewed_release_candidates"]
        self.assertEqual((join["control_command"],join["release_site"]),("802C1F2C","80076C10"))
        self.assertFalse(join["runtime_timing_verified"])
        release["phase_table"]="804338A8"
        self.assertEqual(index.collect(actions)["reviewed_release_candidates"],[])
        program["lanes"]=2
        self.assertEqual(index.collect(actions)["programs"],[])

    def test_program_control_source_requires_both_opcode_entry_and_writer_body(self):
        class Dol:
            def read(self,address,size,kind):
                if (address,kind)==(OPCODE_ROW,"data"):
                    return struct.pack(">I",WRITER)
                if (address,kind)==(WRITER,"text"):
                    return bytes(size)
                raise ValueError("unexpected range")
        with self.assertRaisesRegex(ValueError,"writer signature"):
            control_write_schema(Dol())

    def test_action_sound_keeps_model_specific_actor_calls_separate_from_releases(self):
        class Fixture:
            def matches_text(self,entry,size,digest):
                return self.signature
            def review_call(self,entry,site,target,sound_id=None):
                return (entry,site,target,sound_id) in self.calls
        fixture=Fixture()
        fixture.signature=True
        fixture.calls={(0x8010FC7C,0x8010FCB0,0x8010FE78,None),
                       (0x8010FE78,0x8010FEB8,0x800F036C,162),
                       (0x8010FE78,0x8010FF18,0x800F036C,162),
                       (0x800A8F44,0x800A9028,0x800F036C,267)}
        a,=review_requests(fixture,0x8010FC7C,0x0804,"B-shot")
        b,=review_requests(fixture,0x8010FC7C,0x080C,"B-shot")
        self.assertEqual((a["site"],b["site"]),("8010FEB8","8010FF18"))
        self.assertEqual(a["source_sound_id"],b["source_sound_id"])
        self.assertEqual(a["required_model_at_call"],"0804")
        self.assertFalse(review_requests(fixture,0x8010FC7C,0x0804,"X"))
        self.assertFalse(review_requests(fixture,0x8010FC7C,0x0805,"B-shot"))
        self.assertFalse(review_requests(fixture,0x8010FD44,0x0804,"B-shot"))
        direct,=review_requests(fixture,0x800A8F44,7,"B-charge")
        self.assertNotIn("relay_site",direct)
        fixture.signature=False
        self.assertFalse(review_requests(fixture,0x800A8F44,7,"B-charge"))
        row={"state":2,"substate":1,"selector":4,"contexts":[[2,0]],
             "action_sfx_candidates":[{**b,"entry":"8010FC7C","scope":"phase-zero-candidate"}]}
        profile={"config":"80300000","context_table":"80340000","main_handler":"80100000",
                 "groups":[{"group":"B-shot","rows":[row,row]}]}
        actions={"profiles":[profile]}
        banks=[{"bank":name,"tone_bytes":bytes(128*16)} for name in ("a","b","c")]
        cue,=action_sfx_index(actions,banks)
        self.assertEqual((cue["owner"],cue["bank"],cue["tone"]),("action-actor","b",34))
        (route,)=cue["routes"]
        self.assertEqual((route["context_rows"],route["relay_site"]),(((2,0),),"8010FCB0"))
        self.assertEqual(release_index(actions),[])

    def test_projectile_sound_links_keep_source_bank_and_phase_guard(self):
        files,members={},{}
        for slot in range(4):
            name=f"fixture-{slot}"
            tones=bytes([slot])*16*128
            tsb=struct.pack("<4sIII",b"TSBD",2064,256,128)+tones
            files[name]=tsb
            members[name]={kind:{"member":3*slot+i,"path":name,"sha256":sha256(tsb).hexdigest()}
                           for i,kind in enumerate(("tsb","chd","dpk"))}
        table=struct.pack(">9h",*range(9))
        origins={family:[{}]*row[2] for family,row in TABLES.items()}
        spans={at:struct.pack(">I",callback) for at,callback in PHASE_ZERO.items()}
        for family,offset in (("type-0",0x34),("type-2",0x40)):
            base,stride,count,_,_=TABLES[family]
            raw=bytearray(stride*count)
            struct.pack_into(">h",raw,offset,185)
            struct.pack_into(">h",raw,stride+offset,-1)
            if family=="type-2":
                raw[stride+2]=255
            spans[base]=bytes(raw)
        class Fixture:
            def read(self,at,size,kind):
                data=table if at==BANK_TABLE else spans.get(at,bytes(size))
                if len(data)!=size:
                    raise ValueError("unexpected extent")
                return data
        with self.assertRaisesRegex(ValueError,"source signature"):
            projectile_sound_index(Fixture(),origins,files,members)
        with patch("tools.analyze.retail_projectile_sfx.TEXT",()):
            index=projectile_sound_index(Fixture(),origins,files,members)
            self.assertEqual(len(index["normal_banks"]),3)
            self.assertNotIn("type-15",index["tables"])
            self.assertEqual(index["tables"]["type-2"][1]["status"],"phase-zero-skips-request")
            self.assertEqual(index["tables"]["type-2"][1]["operand_bits"],"ffff")
            rows=[{"family":f,"key":k,"routes":[{"scope":"fixture"}]}
                  for f,k in (("type-2",0),("type-0",1),("type-14",0),("type-15",0),("type-47",5))]
            attach_release_sfx(rows,index["tables"])
            self.assertEqual(rows[0]["sfx_candidate"],{"status":"static-tone-record","source_sound_id":185,
                                                        "bank_slot":1,"bank":"fixture-1","tone":57})
            self.assertEqual(rows[1]["sfx_candidate"]["status"],"outside-qualified-normal-bank-domain")
            self.assertEqual(rows[2]["sfx_candidate"]["tone"],8)
            self.assertNotIn("sfx_candidate",rows[3])
            self.assertEqual(rows[4]["sfx_candidate"],{"status":"static-tone-record","source_sound_id":199,
                                                        "bank_slot":1,"bank":"fixture-1","tone":71})
            self.assertEqual(len(index["tables"]["type-47"]),6)
            self.assertTrue(all(cue["source_sound_id"]==199 and "operand_address" not in cue
                                for cue in index["tables"]["type-47"]))
            self.assertEqual(index["routes"]["type-47"]["calls"],[
                {"site":"800BEDFC","helper":"800F061C","owner":"projectile-child","guard":"child-collision-group-binding-succeeds"}])
            self.assertEqual(rows[0]["routes"],[{"scope":"fixture"}])
            self.assertEqual(tone_candidate(384,index["normal_banks"])["status"],"outside-qualified-normal-bank-domain")
            for row in index["normal_banks"]:
                self.assertEqual(row["bank"],f'fixture-{row["slot"]}')
            spans.pop(0x80301C08)
            with self.assertRaisesRegex(ValueError,"dispatch signature"):
                projectile_sound_index(Fixture(),origins,files,members)

    def test_source_pace_uses_shifted_selected_record_and_exact_travel_handler(self):
        roles={"profiles":[{"roles":[{"role_hint":"ground-travel","selector":2,"states":[3,3],
                                       "handler_addresses":["8005F554","8005F578"]}]}]}
        clips=[("ground-travel",0,1)]
        source=bytearray(ACTOR_BYTES*2)
        struct.pack_into(">f",source,0x2c,18)
        struct.pack_into(">f",source,ACTOR_BYTES+0x2c,12)
        operands={"records":operand_records(source),"class_installer_binding":{
            "selected_record":1,"status":"one-static-slot-0-record"}}
        pace=ground_pace_candidate(roles,clips,operands)
        self.assertEqual((pace["record"],pace["source_value"],pace["native_units_per_tick_x100"]),(1,12,500))
        self.assertFalse(pace["retail_speed_verified"])
        self.assertIn(b"lab-ground-pace=500\n",preview_text("normal","body.prefix",b"bank",[(0,0,64)],
                                                          candidates=clips,ground_pace=pace))
        self.assertIsNone(ground_pace_candidate(roles,[],operands))
        self.assertIsNone(ground_pace_candidate({"profiles":[{"roles":[]}]},clips,operands))
        operands["class_installer_binding"]["selected_record"]=None
        self.assertIsNone(ground_pace_candidate(roles,clips,operands))

    def test_static_ground_pace_requires_pinned_carried_requests_and_bound_actor(self):
        requests=[{"site":site,"lanes":mask,"source_selector":1,"request_scope":"carried-from-ground-turn",
                   "program_status":"terminal_wait","clip_candidates":[],"program_group":0,"program_choice":1}
                  for site,mask in (("8005F380",13),("8005F4FC",2))]
        travel={"role_hint":"ground-travel","selector":2,"states":[3,3],
                "handler_addresses":["8005F554","8005F578"],"program_candidates":requests}
        roles={"profiles":[{"movement_type":1,"roles":[travel]}]}
        data=bytearray(ACTOR_BYTES)
        struct.pack_into(">f",data,0x2c,8)
        operands={"records":operand_records(data),"class_installer_binding":{
            "selected_record":0,"status":"one-static-slot-0-record"}}
        def pace():
            return ground_pace_candidate(roles,[],operands,static_ground_body=True)
        self.assertIsNone(ground_pace_candidate(roles,[],operands))
        self.assertEqual(pace()["native_units_per_tick_x100"],333)
        requests[1]["clip_candidates"]=[{"lanes":[]}]
        self.assertIsNone(pace())
        requests[1]["clip_candidates"]=[]
        operands["class_installer_binding"]["status"]="conditional-or-unresolved"
        self.assertIsNone(pace())

    def test_type_six_pace_needs_both_exact_alternate_travel_rows_on_every_path(self):
        rows=[{"role_hint":"alternate-ground-travel","selector":s,"states":[0,36],
               "handler_addresses":["8005F238","80063A90"],"program_candidates":[]} for s in (2,30)]
        roles={"profiles":[{"movement_type":6,"roles":rows}]}
        data=bytearray(ACTOR_BYTES)
        struct.pack_into(">f",data,0x2c,15)
        operands={"records":operand_records(data),"class_installer_binding":{
            "selected_record":0,"status":"one-static-slot-0-record"}}
        def pace():
            return ground_pace_candidate(roles,[],operands,static_ground_body=True)
        self.assertEqual(pace()["native_units_per_tick_x100"],625)
        self.assertIsNone(ground_pace_candidate(roles,[],operands))
        roles["profiles"].append({"movement_type":6,"roles":rows[:1]})
        self.assertIsNone(pace())
        roles["profiles"].pop()
        rows[1]["handler_addresses"][1]="8005F578"
        self.assertIsNone(pace())

    def test_static_actor_binding_keeps_shifted_and_conditional_pointer_returns_separate(self):
        prefix=(0x38000000,0x3C80802D,0x2C000010,0x38843AD4,0x40800084,
                *(w for i in range(16) for w in (0x80040000+4*i,0x900304B4+4*i)))
        slot_load=(0x880303E7,0x3C80803C,0x7C000774,0x5400103A,0x38844D94,0x7C04002E)
        def installer(*ops):
            return (prefix+ops+(0,)*144)[:144]
        start=0x80000000
        rows,n=interpret_installer(installer(*slot_load,0x380001B0,0x900304AC,0x4E800020),start,0x0619,lambda _:0)
        # addi with RA=0 is a literal, not add-to-pointer.
        self.assertEqual(rows,[(("literal",432),start+44*4)])
        self.assertEqual(n,0)
        move=(0x7CA4002E,) # lwzx r5,r4,r0
        words=installer(*slot_load[:-1],*move,0x38A501B0,0x90A304AC,0x4E800020)
        class Fixture:
            def read(self, address, size, kind):
                if (address,size,kind)!=(start,576,"text"):
                    raise ValueError("unexpected read")
                return struct.pack(">144I",*self.words)
        fixture=Fixture(); fixture.words=words
        data=operand_records(bytes(2*ACTOR_BYTES))
        self.assertEqual(actor_record_binding(fixture,start,0x0619,data,ACTOR_BYTES)["selected_record"],1)
        bad=actor_record_binding(fixture,start,0x0619,data[:1],ACTOR_BYTES)
        self.assertIsNone(bad["selected_record"])
        self.assertEqual(bad["return_candidates"][0]["kind"],"unqualified-resource-offset")
        # Branch to a different producer, followed by the same return.
        fixture.words=installer(*slot_load,0x900304AC,0x40820008,0x4E800020,
                                0x800DACB8,0x900304AC,0x4E800020)
        conditional=actor_record_binding(fixture,start,0x0802,data,ACTOR_BYTES)
        self.assertIsNone(conditional["selected_record"])
        self.assertEqual([r["kind"] for r in conditional["return_candidates"]],["slot-0-record","unqualified-pointer"])

    def test_movement_record_operands_keep_bits_and_ordinals(self):
        raw=bytearray(ACTOR_BYTES*2)
        struct.pack_into(">f",raw,0x2c,7.7)
        struct.pack_into(">f",raw,ACTOR_BYTES+0x2c,12)
        struct.pack_into(">h",raw,4,-7)
        rows=operand_records(raw)
        self.assertEqual([r["record"] for r in rows],[0,1])
        self.assertEqual(rows[0]["operands"]["004"],{"bits":"fff9","value":-7})
        self.assertEqual(rows[0]["operands"]["02C"]["bits"],struct.pack(">f",7.7).hex())
        self.assertEqual(rows[1]["operands"]["02C"]["value"],12)
        self.assertNotEqual(rows[0]["record_sha256"],rows[1]["record_sha256"])
        self.assertEqual(len(rows[0]["operands"]),len(FIELDS))
        with self.assertRaisesRegex(ValueError,"extent"):
            operand_records(raw[:-1])
        struct.pack_into(">f",raw,0x2c,float("nan"))
        with self.assertRaisesRegex(ValueError,"float domain"):
            operand_records(raw)

    def test_operand_links_require_pinned_reads_and_exact_lower_handler_identity(self):
        words={site:word for site,word,_,_ in (*METER_READS,*(r for rs in ROLE_READS.values() for r in rs))}
        class Fixture:
            def read(self, address, size, kind):
                if size!=4 or kind!="text":
                    raise ValueError("unexpected read range")
                return struct.pack(">I",words[address])
        schema=movement_operand_schema(Fixture())
        self.assertEqual(schema["lower_handler_reads"]["8005F578"][0]["offsets"],["02C"])
        words[0x8005F5A8]=0
        with self.assertRaisesRegex(ValueError,"8005F5A8"):
            movement_operand_schema(Fixture())
        roles={"profiles":[{"config":"80300000","selector_table":"80300400","movement_type":3,"roles":[
            {"role_hint":role,"selector":sel,"handler_addresses":["8005F238",handler]}
            for role,sel,handler in (("cruise",27,"800633C8"),("cruise-boost",28,"80063644"),
                                     ("ground-travel",2,"80000000"))]}]}
        result=movement_operand_profile(bytes(ACTOR_BYTES),roles)
        (profile,)=result["profile_candidates"]
        self.assertFalse(result["runtime_record_binding_verified"])
        self.assertTrue(profile["type_3_meter_read_candidate"])
        self.assertEqual([r["selector"] for r in profile["read_links"]],[27,28])

    def test_cruise_preview_is_static_and_requires_the_exact_pair_on_every_type_three_path(self):
        roles={"profiles":[{"movement_type":3,"roles":[
            {"role_hint":role,"selector":sel,"states":[0,state],"handler_addresses":["8005F238",handler],
             "program_candidates":[]}
            for role,sel,state,handler in (
                ("cruise",27,33,"800633C8"),("cruise-boost",28,34,"80063644"))]}]}
        tune=cruise_lab_candidate(b"",{"candidate_types":[3]},roles,{})
        self.assertEqual(tune,(240,240,12,12,8,4))
        self.assertIn(b"lab-cruise=240|240|12|12|8|4\n",preview_text("normal","body.prefix",b"",[],cruise=tune))
        self.assertEqual(cruise_lab_candidate(b"clip",{"candidate_types":[3]},roles,{}),())
        self.assertEqual(cruise_lab_candidate(b"",{"candidate_types":[3,10]},roles,{}),())
        self.assertEqual(cruise_lab_candidate(b"",{"candidate_types":[3]},roles,{"unresolved_stores":1}),())
        roles["profiles"].append({"movement_type":3,"roles":roles["profiles"][0]["roles"][:1]})
        self.assertEqual(cruise_lab_candidate(b"",{"candidate_types":[3]},roles,{}),())

    def test_air_dash_subranges_preserve_identical_clips_at_different_source_starts(self):
        from copy import deepcopy
        roles=[]
        for (role,selector,state,handler,site,choice),start in zip(AIR_DASH_PHASES,(0,6,20),strict=True):
            binding={"lane":1,"bank":"local","group":0,"choice":23,"archive_offset":128}
            request={"site":site,"handler_lane":"lower","lanes":2,"program_group":0,"program_choice":choice,
                     "program_status":"terminal_wait","clip_candidates":[{"preceding_waits":0,"start_frame":start,"lanes":[binding]}]}
            roles.append({"role_hint":role,"selector":selector,"states":[state,state],
                          "handler_addresses":["80000000",handler],"program_candidates":[request]})
        maps={"profiles":[{"roles":roles},{"roles":deepcopy(roles)}]}
        timing=[{"archive_offset":128,"aobj_timing":"indexed","authored_end_frames":[36]}]
        got=air_dash_subranges(maps,timing)
        self.assertEqual([v[1:] for v in got],[(0,23,0),(0,23,6),(0,23,20)])
        first=roles[-1]["program_candidates"][0]["clip_candidates"][0]
        first["start_frame"]=19
        self.assertEqual(air_dash_subranges(maps,timing),())
        first["start_frame"]=20
        timing[0]["authored_end_frames"]=[19]
        self.assertEqual(air_dash_subranges(maps,timing),())
        timing[0]["authored_end_frames"]=[36]
        first["lanes"][0]["bank"]="common"
        self.assertEqual(air_dash_subranges(maps,timing),())
        first["lanes"][0]["bank"]="local"
        timing[0].update(strict_metadata_refusals=["aobj-flags"],aobj_flags=[0x20000000],
                         animated_joint_count=2,authored_loop_joint_count=2)
        self.assertEqual(air_dash_subranges(maps,timing),got)
        timing[0]["strict_metadata_refusals"].append("animjoint-constraints")
        self.assertEqual(air_dash_subranges(maps,timing),())

    def test_dragon_takeoff_requires_the_later_phase_requests_and_exact_body_lanes(self):
        from copy import deepcopy
        def local(lane):
            return {"lane":lane,"bank":"local","group":0,"choice":2,"archive_offset":128}
        def request(site,name,mask,lanes):
            return {"site":site,"handler_lane":name,"lanes":mask,"program_group":0,
                    "program_choice":2,"program_status":"terminal_wait",
                    "clip_candidates":[{"preceding_waits":0,"start_frame":0,"lanes":lanes}]}
        aux={"lane":3,"bank":"local","group":2,"choice":0,"archive_offset":None}
        upper=request("80065370","upper",13,[local(0),local(2),aux])
        lower=request("80065588","lower",2,[local(1)])
        earlier=request("800654F8","lower",2,[dict(local(1),choice=5,archive_offset=256)])
        row={"selector":4,"states":[56,56],"handler_addresses":["8006523C","80065394"],
             "role_hint":"dragon-takeoff","program_candidates":[upper,lower,earlier]}
        profile={"movement_type":4,"configured_lanes":7,"roles":[row]}
        cohorts=[profile,deepcopy(profile)]
        self.assertEqual(dragon_takeoff_clip(cohorts),(0,2))
        self.assertIn(("ground-takeoff",0,2),role_preview_candidates({"profiles":cohorts}))
        lower["site"]="800654A0"
        self.assertIsNone(dragon_takeoff_clip(cohorts))
        lower["site"]="80065588"
        aux["archive_offset"]=192
        self.assertIsNone(dragon_takeoff_clip(cohorts))
        aux["archive_offset"]=None
        lower["clip_candidates"][0]["lanes"][0]["archive_offset"]=256
        self.assertIsNone(dragon_takeoff_clip(cohorts))

    def test_animated_cruise_joins_only_the_frame_zero_local_body_lanes(self):
        from copy import deepcopy
        def request(site,mask,lanes):
            first={"preceding_waits":0,"start_frame":0,"lanes":lanes}
            return {"site":site,"lanes":mask,"program_group":0,"program_choice":0,"program_status":"terminal_wait",
                    "clip_candidates":[first,{**first,"preceding_waits":1}]}
        def local(lane):
            return {"lane":lane,"bank":"local","archive_offset":128,"group":4,"choice":6}
        aux={"lane":3,"bank":"common","archive_offset":None,"group":127,"choice":0}
        row={"selector":0,"states":[1,1],"handler_addresses":["8005F23C","8005F2A4"],
             "program_candidates":[request("8005F284",13,[local(0),local(2),aux]),request("8005F2F8",2,[local(1)])]}
        roles={"profiles":[{"roles":[row]},{"roles":[deepcopy(row)]}]}
        timing=[{"archive_offset":128,"aobj_timing":"indexed","public_joint_count":5,"animated_joint_count":4}]
        self.assertEqual(cruise_entry_candidate(roles,timing,5),(4,6))
        self.assertIsNone(cruise_entry_candidate(roles,timing,6))
        changed=deepcopy(roles)
        changed["profiles"][1]["roles"][0]["program_candidates"][1]["clip_candidates"][0]["lanes"][0]["choice"]=7
        self.assertIsNone(cruise_entry_candidate(changed,timing,5))
        row["program_candidates"][0]["clip_candidates"][0]["start_frame"]=1
        self.assertIsNone(cruise_entry_candidate(roles,timing,5))
        text=preview_text("normal","body.prefix",b"motion",[(0,0,64),(4,6,128)],
                          cruise=(240,240,12,12,8,4),cruise_entry=(4,6))
        self.assertTrue(text.startswith(b"GFBORG-PREVIEW 8\n"))
        self.assertIn(b"idle=0:0\n",text)
        self.assertIn(b"candidate=cruise-entry|4:6\n",text)

    def test_action_preview_needs_a_resolved_upper_request_and_frame_zero_descriptor(self):
        entry=0x80000000
        calls={entry+4:retail_action_links.PROGRAM_REQUEST}
        requests=((entry+4,"program",(1,2,3),"literal-block"),)
        linker=ActionLinker(None,{},{})
        edges={entry:(entry+4,)}
        incoming={entry+4:{entry}}
        with patch.object(ActionLinker,"graph",return_value=({}, incoming, edges, calls,0,False)), \
             patch.object(ActionLinker,"calls",return_value=(requests,0,False)):
            self.assertEqual(linker.action_entry_request(entry),(entry+4,(1,2,3)))
            edges[entry+16]=(entry+4,entry+20)
            incoming[entry+4].add(entry+16)
            self.assertIsNone(linker.action_entry_request(entry+16))
            calls[entry+8]=retail_action_links.PROGRAM_REQUEST
            self.assertIsNone(linker.action_entry_request(entry+4))
        op={"kind":"select_descriptor","group":1,"choice":7,"start_frame":0}
        commands=[op,{"kind":"wait"}, {**op,"choice":99}]
        body={"status":"terminal_wait","commands":commands}
        with patch.object(ActionLinker,"table_choice",return_value=0x80300000), \
             patch("tools.analyze.retail_action_links.decode_program",return_value=body), \
             patch("tools.analyze.retail_action_links.descriptor_lanes",return_value={"lanes":[{"group":3,"index":11}]}):
            self.assertEqual(linker.action_entry_descriptor((0,0,0,0),2,3),(1,7,3,11))
            body["commands"]=[{**op,"kind":"directional_select"}]
            self.assertIsNone(linker.action_entry_descriptor((0,0,0,0),2,4))
            body["commands"]=[{**op,"start_frame":1}]
            self.assertIsNone(linker.action_entry_descriptor((0,0,0,0),2,5))
            body["commands"]=[op,op]
            self.assertIsNone(linker.action_entry_descriptor((0,0,0,0),2,6))
            body["commands"]=[{"kind":"wait"},op]
            self.assertIsNone(linker.action_entry_descriptor((0,0,0,0),2,7))

    def test_action_preview_requires_the_same_raw_context_and_exact_clip_across_profiles(self):
        def profile(c0, c1, mode=1):
            rows=[{"contexts":[[index,0]],"state":0,"substate":index,"selector":39,
                   "entry_clip_candidate":{"clip":[2,c],"archive_offset":c*256}}
                  for index,c in enumerate((c0,c1)) if c]
            return {"config":"80300000","context_table":"80350000","main_handler":"80100000",
                    "groups":[{"group":g,"mode":mode if g=="B-shot" else -1,
                               "rows":rows if g=="B-shot" else []} for g in PREVIEW_GROUPS]}
        valid={"profiles":[profile(4,5),profile(6,5)]}
        entry,=action_preview_candidates(valid)
        self.assertEqual((entry["id"],entry["clip"],entry["raw_context"]),("b-shot-c1-s0",[2,5],(1,0)))
        self.assertFalse(entry["runtime_input_verified"])
        self.assertEqual(len(entry["routes"]),2)
        self.assertEqual(action_preview_candidates({**valid,"unresolved_stores":1}),[])
        self.assertEqual(action_preview_candidates({"profiles":[profile(4,5),profile(6,0)]}),[])
        self.assertEqual(action_preview_candidates({"profiles":[profile(4,5),profile(4,5,mode=0)]}),[])
        preferred,=action_preview_candidates({"profiles":[profile(4,5),profile(4,5)]},preferred_b_shot=(1,0))
        self.assertEqual(preferred["clip"],[2,5])
        self.assertEqual(action_preview_candidates(valid,preferred_b_shot=(0,0)),[])
        text=preview_text("normal","body.prefix",b"motion",[(0,0,64)],action_entries=[entry])
        self.assertIn(b"candidate=b-shot-c1-s0|2:5\n",text)

    def test_phase_one_pose_needs_reviewed_table_model_and_literal_request(self):
        entry,site=0x8015DD44,0x8015DDB8
        review=("phase-2",{"phase_table":"80348B98"},[(0x8015DE50,("type-0",102),"literal-block",0x0902)])
        request=(site,"program",(15,2,0),"literal-block")
        with patch.object(ActionLinker,"review_call",return_value=True), \
             patch.object(ActionLinker,"calls",return_value=((request,),0,False)) as calls, \
             patch.object(ActionLinker,"entry_clip_binding",return_value={"clip":[3,14]}) as bind:
            linker=ActionLinker(None,{},[])
            self.assertEqual(linker.phase_one_shot_pose(review,0x0902,(),{}),
                             {"clip":[3,14],"phase":1,"phase_table":"80348B98"})
            bind.assert_called_once_with(entry,site,(15,2,0),(),{})
            self.assertIsNone(linker.phase_one_shot_pose(review,0x0903,(),{}))
            calls.return_value=(((site,"program",(15,2,1),"literal-block"),),0,False)
            self.assertIsNone(linker.phase_one_shot_pose(review,0x0902,(),{}))

    def test_conditional_action_preview_publishes_a_pair_or_neither(self):
        def profile(second=10):
            pairs=[{"program":[2,i],"clip":[1,c],"archive_offset":c*64,
                    "request_choice_condition":{"set":bool(i)}} for i,c in enumerate((9,second))]
            row={"contexts":[[0,0]],"state":0,"substate":0,"selector":44,"entry_clip_alternatives":pairs}
            return {"config":"80300000","context_table":"80350000","main_handler":"80100000",
                    "groups":[{"group":name,"mode":0,"rows":[row] if name=="B-shot" else []} for name in PREVIEW_GROUPS]}
        pair=action_preview_candidates({"profiles":[profile(),profile()]})
        self.assertEqual([(p["id"],p["clip"]) for p in pair],
                         [("b-shot-c0-s0-p0",[1,9]),("b-shot-c0-s0-p1",[1,10])])
        self.assertEqual(action_preview_candidates({"profiles":[profile(),profile(11)]}),[])

    def test_conditional_positive_release_join_needs_exact_program_choice_and_consumer(self):
        condition={"set":True}
        request={"site":"8015AA0C","scope":"phase-zero-conditional-candidate","lanes":15,"program_group":2,
                 "program_choice":1,"request_choice_condition":condition}
        program={"program_address":"803440A0","decode_status":"terminal_wait",
                 "writes":[{"actor_1cef":1,"actor_1cf0":0,"address":"803440C0","preceding_waits":1}]}
        release={"kind":"projectile","phase_table":"804346F8","phase_entry":"8015AA24",
                 "release_guard_candidate":{"actor_signed_byte_positive":"1CEF"},
                 "site":"8015AB14","family":"type-2","key":87}
        context={"action_group":"B-shot"}
        (row,)=reviewed_joins(context,request,program,[release])
        self.assertEqual((row["control_command"],row["key"],row["program_choice"]),("803440C0",87,1))
        self.assertEqual(list(reviewed_joins(context,request,{**program,"program_address":"80344054"},[release])),[])
        self.assertEqual(list(reviewed_joins(context,request,program,[{**release,"phase_entry":"8015AA28"}])),[])

    def test_phase_one_needs_a_straight_same_actor_increment(self):
        start=0x80000000
        code=(0x9421FFF0,0x7C0802A6,0x90010014,0x88C30540,0x38060001,0x98030540,0x4E800020)
        words={start+i*4:w for i,w in enumerate(code)}
        incoming={start+i*4:{start+(i-1)*4} for i in range(1,len(code))}
        self.assertEqual(entry_phase_increment(words,incoming,start),start+20)
        for at,word in ((0,0x38600000),(2,0x48000001),(4,0x38060002),(5,0x98040540)):
            self.assertIsNone(entry_phase_increment({**words,start+at*4:word},incoming,start))
        self.assertIsNone(entry_phase_increment(words,{**incoming,start+20:{start+16,start}},start))

    def test_reviewed_release_phase_needs_exact_table_and_keeps_model_alternatives(self):
        handler,table=0x800F7F98,0x8031D158
        leaves=(0x800F7FF4,0x800F800C,0x800F809C)
        requests=((0x800F80CC,"projectile",("type-2",20),"literal-block"),
                  (0x800F80D8,"projectile",("type-2",56),"literal-block"),
                  (0x800F8004,"projectile",("type-2",1),"literal-block"))
        origins={"type-2":[{"basis":"parent-joint","source_joint_selector":0,"local_point":[0,0,0]}]*57}
        def leaf(_,offset,index):
            if offset==0x540:
                return leaves[index]
            return handler
        def actions():
            return {"profiles":[{"program_and_descriptor_roots":["0"]*4,"main_handler":"80000000",
                                 "config":"80300000","context_table":"80350000","groups":[{"group":"B-charge","rows":[{
                                     "state":0,"substate":0,"selector":42,"contexts":[[0,0]],
                                     "classification":"selector-candidate","lanes":1,"handler_pair":[61,0]}]}]}]}
        with patch.object(ActionLinker,"dispatch",return_value=table), \
             patch.object(ActionLinker,"leaf",side_effect=leaf) as route, \
             patch.object(ActionLinker,"matches_text",return_value=True) as signature, \
             patch.object(ActionLinker,"calls",side_effect=lambda pc:(requests if pc==leaves[-1] else (),0,False)) as calls, \
             patch.object(ActionLinker,"release_routes",return_value=()), \
             patch.object(ActionLinker,"phase_leaves",return_value=[]):
            for model,key,field,expected in ((0x0D00,20,"required_model_at_call","0D00"),
                                             (0x0D04,56,"excluded_models_at_call",["0D00"])):
                data=actions()
                ActionLinker(None,origins,[]).attach(data,model,[],[0])
                result,=release_index(data)
                self.assertEqual(result["key"],key)
                self.assertEqual(result[field],expected)
                (phase,)=result["routes"]
                self.assertEqual(phase["action_group"],"B-charge")
                self.assertEqual(phase["phase_table"],"8031D158")
                self.assertEqual(phase["phase_transition_stores"],("800F8000","800F8084"))
                self.assertEqual(phase["phase_entry"],"800F809C")
            self.assertIsNone(ActionLinker(None,origins,[]).phase_release_requests(handler+4))
            signature.return_value=False
            self.assertIsNone(ActionLinker(None,origins,[]).phase_release_requests(handler))
            signature.return_value=True
            route.side_effect=lambda h,o,i: 0 if i==1 else leaf(h,o,i)
            self.assertIsNone(ActionLinker(None,origins,[]).phase_release_requests(handler))
            route.side_effect=leaf
            calls.side_effect=None
            calls.return_value=(requests[1:],0,False)
            self.assertIsNone(ActionLinker(None,origins,[]).phase_release_requests(handler))

    def test_release_relays_require_a_pinned_same_caller_and_keep_model_gates(self):
        parent,site,helper=0x800C96C0,0x800C9758,0x800C9F40
        request=(0x800C9FB0,"projectile",("type-2",51),"literal-block")
        with patch.object(ActionLinker,"graph",return_value=({}, {}, {}, {site:helper},0,False)), \
             patch.object(ActionLinker,"calls",return_value=((request,),0,False)), \
             patch.object(ActionLinker,"text",return_value=1), \
             patch.object(ActionLinker,"matches_text",return_value=True) as matches:
            linker=ActionLinker(None,{}, {})
            self.assertEqual(linker.release_routes(parent),((site,helper,*request),))
            self.assertEqual(linker.release_routes(parent+4),())
            matches.return_value=False
            self.assertEqual(ActionLinker(None,{},{}).release_routes(parent),())
        self.assertEqual(release_model_gate(helper,0x800C9FB0),((0x0608,),()))
        self.assertEqual(release_model_gate(0x80070558,0x800705EC),((),(3,10)))

    def test_small_data_phase_uses_only_two_entries_and_requires_the_advance_helper(self):
        handler,table,entry,consumer=0x80076CD4,0x804338A8,0x80076D0C,0x80076B48
        site,helper=0x80076D4C,0x80076C80
        request=(0x80076C10,"projectile",("type-0",12),"literal-block")
        with patch.object(ActionLinker,"dispatch",return_value=table), \
             patch.object(ActionLinker,"leaf",side_effect=lambda _,__,i:(entry,consumer)[i]) as leaf, \
             patch.object(ActionLinker,"matches_text",return_value=True) as signature, \
             patch.object(ActionLinker,"review_call",return_value=True) as call, \
             patch.object(ActionLinker,"calls",return_value=((request,),0,False)):
            scope,evidence,releases=ActionLinker(None,{},[]).phase_release_requests(handler)
            self.assertEqual(scope,"phase-1-reviewed-helper-transition-candidate")
            self.assertEqual(evidence["phase_transition_calls"],("80076D4C",))
            self.assertEqual(evidence["phase_transition_stores"],("80076CA0",))
            self.assertEqual(evidence["reviewed_caller_model"],"0800")
            self.assertEqual(evidence["release_guard_candidate"]["actor_byte_equals"],{"541":0,"1CEF":3,"1CF0":1})
            self.assertEqual(releases,((request[0],request[2],request[3],0),))
            self.assertEqual([args for args,_ in leaf.call_args_list],[(handler,0x540,0),(handler,0x540,1)])
            call.assert_called_once_with(entry,site,helper)
            call.return_value=False
            self.assertIsNone(ActionLinker(None,{},[]).phase_release_requests(handler))
            call.return_value=True
            for changed in (handler,helper,consumer):
                signature.side_effect=lambda address,*_,changed=changed:address!=changed
                self.assertIsNone(ActionLinker(None,{},[]).phase_release_requests(handler))

    def test_x_release_relay_needs_pinned_parent_and_does_not_leak_into_b_shot(self):
        entry,site,helper=0x8018C138,0x8018C150,0x8018C164
        request=(0x8018C248,"projectile",("type-2",81),"literal-block")
        with patch.object(ActionLinker,"graph",return_value=({}, {}, {}, {site:helper},0,False)), \
             patch.object(ActionLinker,"calls",return_value=((request,),0,False)), \
             patch.object(ActionLinker,"text",return_value=1), \
             patch.object(ActionLinker,"matches_text",return_value=True) as signature:
            linker=ActionLinker(None,{}, {})
            self.assertEqual(linker.release_routes(entry,"X"),((site,helper,*request),))
            self.assertEqual(linker.release_routes(entry,"B-charge"),((site,helper,*request),))
            self.assertEqual(linker.release_routes(entry,"B-shot"),())
            signature.side_effect=lambda address,*_:address!=entry
            self.assertEqual(ActionLinker(None,{},{}).release_routes(entry,"X"),())
        self.assertEqual(release_model_gate(helper,request[0]),((0x0628,),()))

    def test_release_review_index_preserves_context_and_factory_alternatives(self):
        request={"kind":"projectile","site":"800C9FB0","family":"type-2","key":51,
                 "basis":"parent-joint","source_joint_selector":7,"local_point":[1.,2.,3.],
                 "scope":"phase-1-entry-increment-candidate","argument_evidence":"literal-block",
                 "release_relay_site":"800C9758","release_relay":"800C9F40","required_model_at_call":"0608"}
        row={"contexts":[[1,0],[1,1]],"state":0,"substate":0,"selector":42,
             "request_candidates":[request,{**request,"key":52}]}
        profile={"config":"80300000","context_table":"802AA000","main_handler":"80009988",
                 "groups":[{"group":"B-shot","rows":[row,{**row,"selector":43,"contexts":[[1,2]]}]}]}
        entries=release_index({"profiles":[profile,profile]})
        self.assertEqual([entry["key"] for entry in entries],[51,52])
        self.assertEqual(entries[0]["local_point"],[1.,2.,3.])
        self.assertEqual(entries[0]["required_model_at_call"],"0608")
        self.assertEqual([(r["selector"],r["context_rows"],r["relay_site"]) for r in entries[0]["routes"]],
                         [(42,((1,0),(1,1)),"800C9758"),(43,((1,2),),"800C9758")])
        row["request_candidates"]=[{**request,"family":"type-14","key":2,"adds_retained_offset":True}]
        self.assertTrue(release_index({"profiles":[profile]})[0]["adds_retained_offset"])

    def test_origin_marker_needs_exact_joint_and_no_runtime_offset(self):
        row={"family":"type-2","key":21,"basis":"parent-joint","source_joint_selector":7,
             "local_point":[-52.125,36.3,135.2],"joint_present_in_normal_rig":True,"parent_model_matches":True}
        records=[row,{**row,"site":"81000000"},{**row,"key":22,"joint_present_in_normal_rig":False},
                 {**row,"key":23,"basis":"parent-root-matrix"},{**row,"key":24,"adds_parent_offset":True},
                 {**row,"key":25,"adds_retained_offset":True},{**row,"key":26,"parent_model_matches":False}]
        marks=preview_origin_markers(records)
        self.assertEqual(marks,[("type-2-21",7,-52.125,36.3,135.2)])
        text=preview_text("normal","body.prefix",b"",[],origins=marks)
        self.assertTrue(text.startswith(b"GFBORG-PREVIEW 5\n"))
        self.assertIn(b"origin=type-2-21|7|-52.125|36.3|135.2\n",text)
        with self.assertRaisesRegex(ValueError,"inconsistent"):
            preview_origin_markers([row,{**row,"source_joint_selector":8}])

    def test_motion_bank_preserves_holes_and_aliases(self):
        data = bytearray(0xA0)
        put(data, 0, 0x40)
        put(data, 4, 0x40)
        for i, value in enumerate((0x80, 0, 0x80, 0xFFFFFFFF)):
            put(data, 0x40 + 4*i, value)
        put(data, 0x80, 32)
        self.assertEqual(bank_clips(data), [(0, 0, 0x80), (0, 2, 0x80), (1, 0, 0x80), (1, 2, 0x80)])

    def test_motion_bank_refuses_partial_archive(self):
        data = bytearray(0x80)
        put(data, 0, 0x40)
        put(data, 0x40, 0x60)
        put(data, 0x44, 0xFFFFFFFF)
        put(data, 0x60, 33)
        with self.assertRaisesRegex(ValueError, "archive extent"):
            bank_clips(data)

    def test_landing_pcm_checks_bank_identity_and_bounds_all_decoder_outputs(self):
        files={f"shared-snd_com01.{role}":role.encode() for role in ("tsb","chd","dpk")}
        bank=[(i,k.removeprefix("shared-"),len(data),sha256(data).hexdigest())
              for i,(k,data) in enumerate(files.items())]
        index={"audio_bank_members":{"snd_com01":{name.rsplit(".",1)[1]:{"member":i} for i,name,_,_ in bank}}}
        samples={"landing":b"\x01\x00\x01\x00","landing-18":b"\x02\x00\x02\x00","landing-22":b"\x03\x00\x03\x00"}
        def write(command, *, stdout, check, timeout):
            self.assertEqual(check,True)
            self.assertEqual(timeout,10)
            self.assertEqual([p.read_bytes() for p in command[1:4]],list(files.values()))
            stdout.write(samples[command[-1]])
        module="tools.extract.retail_borg_roster"
        with patch(f"{module}.COMMON_MOVEMENT_BANK",bank),patch(f"{module}.run",side_effect=write) as spawn:
            result=decode_lab_landing_pcm(Path(__file__),files,index)
            self.assertEqual(result,dict(zip((15,18,22),samples.values(),strict=True)))
            self.assertEqual(spawn.call_count,3)
            bad={**files,"shared-snd_com01.tsb":b"bad"}
            with self.assertRaisesRegex(ValueError,"bank identity"):
                decode_lab_landing_pcm(Path(__file__),bad,index)
            self.assertEqual(spawn.call_count,3)
            samples["landing"]=b"\x01"
            with self.assertRaisesRegex(ValueError,"PCM budget"):
                decode_lab_landing_pcm(Path(__file__),files,index)

    def test_clip_index_deduplicates_physical_archive_and_retains_loop(self):
        data = bytearray(64)
        put(data, 8, 32)
        put(data, 12, 52) # a constraint is inventoried, not decoded or cleared
        struct.pack_into(">If", data, 32, 0x20000000, 65)
        table = struct.pack(">3I", 8, 0, 0)
        archive = struct.pack(">8I", 32+len(data)+len(table), len(data), 1, 1, 0, 0, 0, 0) + data + table
        a, = clip_timing(bytes(64) + archive, [(0, 0, 64), (1, 2, 64)])
        self.assertEqual(a["authored_end_frames"], [65])
        self.assertEqual(a["authored_loop_joint_count"], 1)
        self.assertEqual(a["aobj_flags"], [0x20000000])
        self.assertEqual(a["strict_metadata_refusals"],["animjoint-constraints-or-flags","aobj-flags"])

    def test_part_defaults_use_only_supported_groups_and_exact_indices(self):
        class FixtureDol:
            def read(self, address, width):
                return {0x802D2564: struct.pack(">I", 0x2000), 0x2004: struct.pack(">I", 0x3000),
                        0x3000: bytes((2, 7, 0, 0)), 0x3004: bytes((3, 7, 255, 0)),
                        0x3008: bytes((5, 11, 1, 0)), 0x300C: bytes((255, 0, 0, 0))}[address]
        actor = bytearray(432)
        actor[0xBE:0xC0] = bytes((1, 3))  # Outside group 1: leave unresolved.
        value = source_parts(FixtureDol(), 1, actor, [None]*12)
        self.assertEqual(value["raw_default_selectors"], [1, 3])
        self.assertEqual(value["default_candidates"],
                         [{"selector": 0, "default": 1, "joint": 7, "record": 3, "group_records": [2, 3]}])
        self.assertFalse(value["runtime_visibility_resolved"])

    def test_guarded_landing_tone_uses_lower_seven_selector_bits(self):
        class FixtureDol:
            def read(self, address, width):
                return {0x8031B7F8: struct.pack(">I", 0x4000),
                        0x4003: b"\x82", 0x80434414: struct.pack(">h", 22)}[address]
        self.assertEqual(landing_sound(FixtureDol(), 0x0103),
                         {"bank": "snd_com01", "tone": 22, "source_selector": 130,
                          "fixed_gain_dispatch": True, "trigger_qualified": False})

    def test_movement_literal_requires_qualified_straight_line_definition(self):
        literal = [0x3D208034, 0x38098000, 0x900304B0]
        self.assertEqual(literal_at_store(literal, 2), 0x80338000)
        with self.assertRaisesRegex(ValueError, "middle"):
            literal_at_store([0x48000008, *literal], 3)
        with self.assertRaisesRegex(ValueError, "unqualified"):
            literal_at_store([literal[0], 0x41820008, *literal[1:]], 3)
        class FixtureDol:
            def read(self, address, width, kind):
                if kind == "text":
                    return struct.pack(">I", (*literal, 0x4E800020)[(address-0x80004000)//4])
                assert address == 0x80338000 and width == 192
                return bytes((0, 11)) + bytes(190)
        result = movement_config_candidates(FixtureDol(), 0x80004000)
        self.assertEqual(result["candidate_types"], [11])
        self.assertEqual(result["candidates"][0]["store_sites"], ["80004008"])
        self.assertFalse(result["store_reachability_verified"])

    def test_move_display_slots_preserve_gaps_without_runtime_activation(self):
        class FixtureDol:
            def read(self, address, width):
                return {0x80435B40: bytes((0, 0, 0, 1, 1)), 0x80435B48: bytes((1, 2, 3, 0, 3))}[address]
        spec = bytearray(432)
        spec[0x1A8:0x1AD] = bytes((5, 255, 255, 7, 255))
        rows = displayed_moves(FixtureDol(), 1, spec, [None]*5+["One", None, None, "Two", None])
        self.assertEqual([(r["slot"], r["button"], r["text_index"]) for r in rows], [(0, "B", 5), (3, "X", 8)])
        profile = move_profile(rows)
        self.assertIn(b"move=display-slot-3|base|x|unresolved|Two", profile)
        self.assertIn(b"activation=unresolved", profile)

    def test_projectile_origin_tables_keep_matrix_owner_and_model_filter_separate(self):
        class FixtureDol:
            def read(self, address, width, kind):
                self.assert_data(kind)
                for family, (base, stride, count, bone, after) in TABLES.items():
                    if address == base+stride*count:
                        return struct.pack(">I", after)
                    if address != base:
                        continue
                    assert width == stride*count
                    data = bytearray(width)
                    for i in range(count):
                        at = i*stride
                        data[at+bone] = 255 if family == "type-2" and i == 0 else 7
                        struct.pack_into(">3f", data, at+4, 1, 2, 3)
                        if family == "type-15":
                            struct.pack_into(">H", data, at+46, 10 if i == 3 else 0)
                    return bytes(data)
                raise ValueError("unknown table")

            def assert_data(self, kind):
                assert kind == "data"

        tables = origin_tables(FixtureDol())
        self.assertEqual(tables["type-2"][0]["basis"], "parent-root-matrix")
        one = origin_candidates(tables, 10, [None]*8)
        self.assertEqual(one["reviewed_ordinary_b"]["basis"], "retained-actor-joint")
        self.assertEqual(one["parent_matching_type15_rows"][0]["key"], 3)
        self.assertFalse(one["live_launch_verified"])
        self.assertIsNone(origin_candidates(tables, 11, [None]*8)["reviewed_ordinary_b"])

    def test_static_action_group_keeps_explicit_refusal_zero_and_real_selector_distinct(self):
        class FixtureDol:
            def read(self, address, width, kind):
                records = {
                    0x20B8: bytes((1, 255, 255, 255, 255, 255)),
                    0x3000: struct.pack(">6I", 0x5000, 0, 0, 0, 0, 0),
                    0x4000: bytes((0, 255, 61, 3)) + bytes(92),
                    0x5000: bytes((0, 0, 0, 0, 255, 0, 0, 0, 1, 3, 2, 4)) + bytes(84),
                }
                out = records[address]
                assert len(out) == width and kind == "data"
                return out
        groups = decode_tables(FixtureDol(), (0x2000, 0x3000, 0x4000))
        self.assertEqual(groups[0]["group"], "B-shot")
        self.assertEqual(groups[1], {"group": "B-close", "mode": -1, "rows": []})
        rows = {r["classification"]: r for r in groups[0]["rows"]}
        self.assertEqual(rows["selector-candidate"]["contexts"], [(0, 2)])
        self.assertEqual(rows["selector-candidate"]["handler_pair"], [61, 3])
        self.assertIsNone(rows["explicit-refusal"]["handler_pair"])
        self.assertIn((0, 0), rows["zero-row"]["contexts"])

    def test_installer_candidates_preserve_pointer_tuple_across_conditional_paths(self):
        words = [0x38000000, 0x3C808030, 0x2C000010, 0x38840000, 0x40800084]
        for index in range(16):
            words.extend((0x80040000+4*index, 0x900304B4+4*index))
        words.extend((0x3C808034, 0x38041000, 0x900304B0, 0x41820014,
                      0x3C808034, 0x38042000, 0x900304EC, 0x4E800020, 0x4E800020))
        words.extend([0]*(144-len(words)))
        class FixtureDol:
            def read(self, address, width, kind):
                data = {
                    0x80004000: struct.pack(">144I", *words),
                    0x80300000: struct.pack(">I", 0x80004100),
                    0x80300038: struct.pack(">2I", 0x80343000, 0x80344000),
                    0x803410B8: bytes((255,)*6),
                    0x80342000: bytes(24),
                    0x80343000: bytes(24),
                    0x80344000: bytes(96),
                }[address]
                if len(data) != width:
                    raise ValueError("fixture width")
                return data
        result = action_candidates(FixtureDol(), 0x80004000, 1)
        self.assertEqual([(r["config"], r["context_table"], r["selector_table"]) for r in result["profiles"]],
                         [("80341000", "80342000", "80344000"), ("80341000", "80343000", "80344000")])
        self.assertFalse(result["runtime_context_verified"])

    def test_literal_call_arguments_refuse_join_points_and_clobbers(self):
        words = {0x20:0x38800015, 0x24:0x7FA3EB78}
        self.assertEqual(immediate_argument(words, {0x28:{0x24},0x24:{0x20}}, 0x28, 4), 21)
        self.assertIsNone(immediate_argument(words, {0x28:{0x24,0x50},0x24:{0x20}}, 0x28, 4))
        words[0x24] = 0x88830111  # lbz r4,...: must not fall back to the stale literal
        self.assertIsNone(immediate_argument(words, {0x28:{0x24},0x24:{0x20}}, 0x28, 4))

    def test_dispatch_recognizes_only_exact_actor_field_shape(self):
        words = [0x9421FFF0,0x7C0802A6,0x3C808032,0x90010014,0x38848000,
                 0x88030580,0x7C000774,0x5400103A,0x7D84002E,0x7D8903A6,0x4E800421]
        class FixtureDol:
            def read(self, address, width, kind):
                key = (address-0x80002000)//4
                if kind=="text" and 0<=key<len(words):
                    return struct.pack(">I", words[key])
                raise ValueError("unmapped")
        linker = ActionLinker(FixtureDol(), {}, [])
        self.assertEqual(linker.dispatch(0x80002000,0x580), 0x80318000)
        self.assertIsNone(linker.dispatch(0x80002000,0x581))

    def test_literal_dataflow_merges_disagreement_and_clears_call_arguments(self):
        words = {0:0x4182000C, 4:0x38800015, 8:0x48000008, 12:0x38800016,
                 16:0x48000001, 20:0x4E800020}
        edges = {0:(4,12), 4:(8,), 8:(16,), 12:(16,), 16:(20,), 20:()}
        self.assertNotIn(4, literal_flow(0, words, edges)[16])
        words[12] = words[4]
        flow = literal_flow(0, words, edges)
        self.assertEqual(flow[16][4], 21)
        self.assertNotIn(4, flow[20])

    def test_static_initialized_lookup_does_not_fall_back_to_runtime_memory(self):
        words = {0:0x38801234, 4:0x88840003, 8:0x1C840002, 12:0x4E800020}
        edges = {0:(4,),4:(8,),8:(12,),12:()}
        def source(address, size):
            if (address,size)==(0x1237,1):
                return b"\x0b"
            raise ValueError("not source data")
        self.assertEqual(literal_flow(0,words,edges,source)[12][4], 22)
        words[0] = 0x38831234  # addi r4,r3,0x1234; actor r3 is not supplied
        self.assertNotIn(4,literal_flow(0,words,edges,source)[12])

    def test_recognized_model_gate_keeps_exclusive_upper_bound(self):
        base = 0x8010D040
        block = bytearray(0x74)
        for at, limit in ((0x8010D044,0x302),(0x8010D050,0x301),(0x8010D05C,0x304)):
            put(block, at-base, 0x2C000000 | limit)
        class FixtureDol:
            def read(self, address, width, kind):
                return bytes(block[address-base:address-base+width])
        with patch.object(retail_action_links, "MODEL_GATE_SHA", sha256(block).hexdigest()):
            actual = ActionLinker(FixtureDol(), {}, []).model_gate()
        self.assertEqual(actual, {0x8010D070:0x301, 0x8010D080:0x302, 0x8010D0AC:0x303})

    def test_motion_role_uses_handler_identity_not_selector_number(self):
        class FixtureLinker:
            dol = None
            def data(self, address, size):
                if address==0x1000:
                    return bytes((0,3,15))
                # Selector 0: lower shared no-op. Selector 1: exact shared idle.
                return bytes((0,0,255,1)) + bytes([255])*92
            def read_word(self, address):
                return 0x8005F2A4 if address==HANDLERS[1]+4 else 0x8005F238
            def text(self, address):
                return 1
            def calls(self, address):
                return (),0,False
        row={"config":"00001000","selector_table":"00002000","program_and_descriptor_roots":["00000000"]*4}
        result = motion_roles(FixtureLinker(), {"profiles":[row,row]}, [])
        movement,=result["profiles"]
        self.assertEqual(movement["other_present_lower_selectors"], [0])
        self.assertEqual([(r["selector"],r["role_hint"]) for r in movement["roles"]], [(1,"idle")])
        self.assertFalse(result["runtime_reachability_verified"])

    def test_ground_travel_needs_exact_turn_pair_and_request(self):
        request = {"site":"8005F4FC","program_group":0,"program_choice":1}
        turn = {"selector":1,"states":[2,2],"program_candidates":[request]}
        travel = {"selector":2,"states":[3,3],"program_candidates":[]}
        class FixtureDol:
            def read(self, address, size, kind):
                return b"test"
        pin = [(0,4,sha256(b"test").hexdigest())]
        with patch.object(retail_motion_roles, "TURN_TO_TRAVEL_TEXT", pin):
            carry_ground_travel([turn,travel], FixtureDol())
            self.assertEqual(travel["program_candidates"], [
                {**request, "request_scope":"carried-from-ground-turn","source_selector":1}])
            self.assertFalse(travel["entry_request_present"])
            travel["program_candidates"] = []
            turn["states"] = [0,2]
            carry_ground_travel([turn,travel], FixtureDol())
            self.assertEqual(travel["program_candidates"], [])

    def test_role_preview_requires_agreement_and_exact_lower_local_clip(self):
        def profile(choice, bank="local", waits=0):
            return {"roles":[{"role_hint":"landing","program_candidates":[{"handler_lane":"lower","clip_candidates":[
                {"preceding_waits":waits,"lanes":[{"lane":1,"bank":bank,"group":0,"choice":choice,"archive_offset":0x100}]}
            ]}]}]}
        self.assertEqual(role_preview_candidates({"profiles":[profile(9),profile(9)]}), [("landing",0,9)])
        for other in (profile(5),profile(9,"common"),profile(9,waits=1),{"roles":[]}):
            self.assertEqual(role_preview_candidates({"profiles":[profile(9),other]}), [])

    def test_dash_links_require_the_exact_paired_handlers_and_ordinary_lanes(self):
        class Fixture:
            dol=object()
            def program_candidate(self, roots, local, args):
                mask,group,program=args
                return {"lanes":mask,"program_group":group,"program_choice":program}
        def table(upper="80060BE8"):
            return [{"selector":12+i,"states":[13+i,13+i],"handler_addresses":list(pair),
                     "program_candidates":[]} for i,pair in enumerate((
                         (upper,"80060C48"),("80060E94","80060EC4"),("80061048","80061084")))]
        with patch("tools.analyze.retail_motion_roles.ordinary_dash_source_matches", return_value=True):
            bad=table("80060E94")
            link_ground_dash(bad,(0,0,3),(),{},Fixture())
            self.assertEqual(bad[0]["program_candidates"],[])
            rows=table()
            link_ground_dash(rows,(0,3,7),(),{},Fixture())
            link_ground_dash(rows,(0,0,1),(),{},Fixture())
            self.assertEqual(rows[0]["program_candidates"],[])
            link_ground_dash(rows,(0,0,3),(),{},Fixture())
            for mode in (1,2):
                matched=table()
                link_ground_dash(matched,(0,mode,7),(),{},Fixture())
                self.assertEqual(matched,rows)
                mismatch=table()
                mismatch[2]["states"]=[15,14]
                link_ground_dash(mismatch,(0,mode,7),(),{},Fixture())
                self.assertEqual(mismatch[0]["program_candidates"],[])
        lower=[(r["dash_direction"],r["program_choice"],r["lanes"]) for r in rows[0]["program_candidates"]
               if r["handler_lane"]=="lower"]
        self.assertEqual(lower,[(d,27+i,2) for i,d in enumerate(DASH_DIRECTIONS)])
        refused=table()
        with patch("tools.analyze.retail_motion_roles.ordinary_dash_source_matches", return_value=False):
            link_ground_dash(refused,(0,0,3),(),{},Fixture())
        self.assertEqual(refused[0]["program_candidates"],[])

    def test_alternate_ninja_dash_links_need_the_pinned_handler_and_relay(self):
        class Fixture:
            dol=object()
            def program_candidate(self,roots,local,args):
                return {"program_choice":args[2]}
        def table():
            return [{"selector":12+i,"states":[44+i,44+i],"handler_addresses":list(pair),"program_candidates":[]}
                    for i,pair in enumerate((("800646B0","80064710"),("8006475C","8006478C"),("800648BC","800648F8")))]
        with patch("tools.analyze.retail_motion_roles.ordinary_dash_source_matches",return_value=True), \
             patch("tools.analyze.retail_motion_roles.alternate_dash_source_matches",return_value=True) as supported:
            rows=table()
            link_ground_dash(rows,(0,0,3),(),{},Fixture())
            self.assertEqual(rows[0]["source_dash_route"],"alternate-ninja")
            self.assertEqual([(r["site"],r.get("request_relay_site")) for r in rows[0]["program_candidates"][:2]],
                             [("80060D80","80064724"),("800646F0",None)])
            wrong=table();wrong[1]["handler_addresses"][0]="80060E94"
            link_ground_dash(wrong,(0,0,3),(),{},Fixture())
            self.assertEqual(wrong[0]["program_candidates"],[])
            supported.return_value=False
            missing=table()
            link_ground_dash(missing,(0,0,3),(),{},Fixture())
            self.assertEqual(missing[0]["program_candidates"],[])

    def test_dash_previews_preserve_aliases_but_refuse_incomplete_or_ambiguous_maps(self):
        def profile(choices, start=0, bank="local"):
            return {"roles":[{"role_hint":"ground-dash-entry","program_candidates":[
                {"dash_direction":direction,"handler_lane":"lower","clip_candidates":[
                    {"preceding_waits":0,"start_frame":start,"lanes":[
                        {"lane":1,"group":2,"choice":choice,"bank":bank,"archive_offset":0x100}]}]}
                for direction,choice in zip(DASH_DIRECTIONS,choices)]}]}
        choices=(7,8,9,8)
        entry=profile(choices)
        expected=[(f"ground-dash-{d}",2,c) for d,c in zip(DASH_DIRECTIONS,choices,strict=True)]
        self.assertEqual(role_preview_candidates({"profiles":[entry,profile(choices)]}),expected)
        for bad in (profile((7,8,9)),profile((7,8,9,10)),profile(choices,1),profile(choices,bank="common")):
            self.assertEqual(role_preview_candidates({"profiles":[entry,bad]}),[])
        ambiguous=profile(choices)
        req=ambiguous["roles"][0]["program_candidates"][0]
        req["clip_candidates"]*=2
        self.assertEqual(role_preview_candidates({"profiles":[ambiguous]}),[])

    def test_air_dash_preview_requires_all_three_exact_frame_zero_bindings(self):
        def profile(burst=24,windup_start=0):
            return {"roles":[{"role_hint":role,"program_candidates":[{"handler_lane":"lower","clip_candidates":[
                {"preceding_waits":0,"start_frame":start,"lanes":[
                    {"lane":1,"bank":"local","group":0,"choice":choice,"archive_offset":choice*64}]}]}]}
                for role,choice,start in (("air-dash-wind-up",14,windup_start),
                                         ("air-dash-active",burst,0),("air-dash-recovery",26,0))]}
        ok=profile()
        roles=role_preview_candidates({"profiles":[ok,profile()]})
        self.assertEqual(set(roles),{("air-dash-wind-up",0,14),("air-dash-active",0,24),("air-dash-recovery",0,26)})
        for other in (profile(burst=25),profile(windup_start=1)):
            self.assertEqual(role_preview_candidates({"profiles":[ok,other]}),[("air-dash-recovery",0,26)])

    def test_held_glide_requires_the_verified_program_and_exact_post_wait_clip(self):
        def profile(held=2, command=4, mode=2, start=0):
            return {"movement_type":mode,"roles":[{"role_hint":"glide-entry","selector":18,"states":[24,24],
                "handler_addresses":["80062614","8006266C"],"program_candidates":[{
                    "site":"800626D8","program_group":0,"program_choice":34,"handler_lane":"lower","lanes":2,
                    "clip_candidates":[
                        {"command_index":0,"preceding_waits":0,"start_frame":0,
                         "lanes":[{"lane":1,"group":0,"choice":0,"bank":"local","archive_offset":640}]},
                        {"command_index":command,"preceding_waits":1,"start_frame":start,
                         "lanes":[{"lane":1,"group":5,"choice":held,"bank":"local","archive_offset":640+held*64}]}
                    ]}]}]}
        ok=profile()
        self.assertEqual(role_preview_candidates({"profiles":[ok,profile()]}),
                         [("glide-entry",0,0),("glide-held",5,2)])
        for unrelated in (profile(held=8),profile(command=5),profile(mode=0),profile(start=1)):
            self.assertEqual(role_preview_candidates({"profiles":[ok,unrelated]}),[("glide-entry",0,0)])

    def test_wrapper_preserves_case_and_excludes_padding(self):
        prefix = struct.pack(">III", 12, 0, 0)
        model = struct.pack(">I", 32) + bytes(28)
        self.assertEqual(wrapper_model(prefix + model + bytes(8)), (prefix, model))
        with self.assertRaisesRegex(ValueError, "archive extent"):
            wrapper_model(prefix + struct.pack(">I", 33) + bytes(28))

    def test_exact_logical_joint_order_is_not_preorder(self):
        data = bytearray(0xA0)
        for offset, value in ((4, 8), (8, 12), (12, 0x20), (0x28, 0x60)):
            put(data, offset, value)
        struct.pack_into(">3f", data, 0x4C, 1, 2, 3)
        struct.pack_into(">3f", data, 0x8C, 4, 5, 6)
        symbols = struct.pack(">6I", 4, 8, 12, 0x28, 4, 0) + b"scene_data\0"
        model = struct.pack(">8I", 32+len(data)+len(symbols), len(data), 4, 1, 0, 0, 0, 0) + data + symbols
        prefix = struct.pack(">III", 24, 12, 20) + bytes((255, 255, 1, 0, 0, 255, 255, 0, 1, 0, 0, 0))
        rows = logical_joints(prefix, model)
        self.assertEqual([(row["index"], row["descriptor_offset"], row["local_translation"]) for row in rows],
                         [(0, 0x60, (4, 5, 6)), (1, 0x20, (1, 2, 3))])
        with self.assertRaisesRegex(ValueError, "shared identity"):
            loop = bytearray(model)
            put(loop, 32+0x28, 0x20)
            logical_joints(prefix, loop)

    def test_relocated_zero_root_is_not_null(self):
        data = bytearray(0xAC)
        for offset, value in ((0xA0, 0xA4), (0xA4, 0xA8), (8, 0x40)):
            put(data, offset, value)
        symbols = struct.pack(">6I", 8, 0xA0, 0xA4, 0xA8, 0xA0, 0) + b"scene_data\0"
        model = struct.pack(">8I", 32+len(data)+len(symbols), len(data), 4, 1, 0, 0, 0, 0) + data + symbols
        prefix = struct.pack(">III", 24, 12, 20) + bytes((1, 255, 255, 0, 255, 255, 0, 0, 0, 1, 0, 0))
        rows = logical_joints(prefix, model)
        self.assertEqual([r["descriptor_offset"] for r in rows], [0, 0x40])
        self.assertEqual(rows[1]["child_next_parent"], (255, 255, 0))
        invalid = bytearray(model)
        put(invalid, 32+len(data)+12, 4)
        with self.assertRaisesRegex(ValueError, "relocation"):
            logical_joints(prefix, invalid)

    def test_neutral_mounts_use_exact_records_and_preserve_reuse(self):
        class FixtureDol:
            def read(self, address, width):
                opcodes = {0x80123568:0x901D04D4,0x801235A4:0x48007FF9,
                           0x8012B848:0x4BEDB7E9,0x8012B8D8:0x4BEDB759,
                           0x8012BCF4:0x7C7C1A14,0x8012BD18:0x480DFA91}
                operands = {
                    0x803327C8:bytes((17,255,8,13,9,11,10,12,14,14,15,15,16,16,2,5)),
                    0x803327E8:bytes((7,255,3,13,5,9,6,10,14,17,15,18,16,19,7,11)),
                    0x803327F8:struct.pack(">48f",-15,-5,*([0]*46)),
                    0x803328B8:struct.pack(">16f",*([.5]*16)),
                }
                result = struct.pack(">I",opcodes[address]) if address in opcodes else operands[address]
                assert len(result)==width
                return result
        source=FixtureDol()
        rows=neutral_preview_mounts(source, 0x0202, 0x80123438)
        self.assertEqual((len(rows),rows[0],rows[-2:]),(15,(17,7,-15,-5,0),((2,7,0,0,0),(5,11,0,0,0))))
        self.assertEqual([row[1] for row in rows if row[0]==14],[14,17])
        self.assertEqual(neutral_preview_mounts(source, 0x0201, 0x80123438), ())
        text=preview_text("normal","body.prefix",b"",[],mounts=rows)
        self.assertTrue(text.startswith(b"GFBORG-PREVIEW 3\n"))
        self.assertIn(b"mount=17|7|-15|-5|0\n",text)

    def test_mounted_record_requires_exact_static_descriptor(self):
        data=bytearray(0xB0)
        fields={4:0x10,0x10:0x20,0x14:0x30,0x20:0,0x30:0x50}
        for at,target in fields.items():
            put(data,at,target)
        symbols=struct.pack(">7I",*fields,4,0)+b"scene_data\0"
        model=struct.pack(">8I",32+len(data)+len(symbols),len(data),len(fields),1,0,0,0,0)+data+symbols
        validate_preview_mounts(model,[{}],[(1,0,0,0,0)])
        for rows in ([(1,1,0,0,0)],[(2,0,0,0,0)]):
            with self.assertRaises(ValueError):
                validate_preview_mounts(model,[{}],rows)
        bad=bytearray(model)
        put(bad,32+0x38,0x50)
        with self.assertRaisesRegex(ValueError,"nonstatic"):
            validate_preview_mounts(bad,[{}],[(1,0,0,0,0)])

    def test_variant_preview_preserves_only_the_exact_wrapper(self):
        prefix = struct.pack(">III", 12, 0, 0)
        model = struct.pack(">I", 32) + bytes(range(28))
        empty, declared = {}, {}
        with patch("tools.extract.retail_borg_roster.logical_joints", return_value=[]):
            files, rows = variant_preview_files(
                [(5, b"invalid", empty), (7, prefix+model, declared)], b"", [])
        self.assertEqual(rows, [(3, "source-3")])
        self.assertNotIn("variant_id", empty)
        self.assertEqual(files["visual/source-3/body.hsd"], model)
        self.assertIn(b"variant=source-3\n", files["visual/source-3/preview.txt"])
        self.assertIn(b"animations=none", files["variants.txt"])
        self.assertNotIn(b"model=inherit", files["variants.txt"])


if __name__ == "__main__":
    unittest.main()
