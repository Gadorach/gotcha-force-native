"""Authored Roach B: the placed type-5 area child, reimplemented as a native bomb.

The source release pays one channel-0 unit, creates a second child at
80085E00, arms it, senses through the six-slot proximity service and
requests the area/release exits. The native bomb controller already has
the same shape: 3D proximity, an arming clock, an expiry/proximity blast,
a capped stock return and retirement. This module binds the exact source
row/debit and authors the placement, blast and timing explicitly.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODEL=0x0F05
CALLBACK=0x801E7F28
CONFIG="803929C8"
CONTEXT_TABLE="80392BA0"
MAIN_HANDLER="801E7EF0"
ROOTS=["802BD54C","802BD65E","00000000","00000000"]
DEBIT=0x8006DBE0
BLAST_RADIUS=310   # common collision group 3 sphere radius
PROXIMITY_RADIUS=150  # scoped six-slot proximity distance from the Roach receipt
SOURCE_ARMING=300  # source timer before the area child is active
ARMING=30          # authored playable arm; the source 300 is too long for the demo window
LIFETIME=70        # authored playable lifetime
STOCK_CAP=3
AUTHORED_DAMAGE=12
AUTHORED_PACE=333  # 3.33 native units/tick, the 0F01/0F02 family pace


def battle_test(joints,actions,level,*,pace=AUTHORED_PACE):
    """Return (gameplay, pose, receipt). Exact row/debit; authored placement/damage."""
    if level[0]<1 or level[0]>10000 or pace!=AUTHORED_PACE:
        raise ValueError("Roach bomb needs its source health and family pace")
    profiles=actions.get("profiles") or []
    if (len(profiles)!=1 or profiles[0].get("config")!=CONFIG or
            profiles[0].get("context_table")!=CONTEXT_TABLE or
            profiles[0].get("main_handler")!=MAIN_HANDLER or
            profiles[0].get("program_and_descriptor_roots")!=ROOTS or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("Roach bomb needs the exact installer profile")
    rows=[r for g in profiles[0]["groups"] if g.get("group")=="B-shot" for r in g["rows"]]
    if (len(rows)!=1 or (rows[0].get("state"),rows[0].get("substate"),rows[0].get("selector"),
                        rows[0].get("handler_pair"),rows[0].get("class_callback_candidate"))!=
            (0,0,45,[61,0],f"{CALLBACK:08X}")):
        raise ValueError("Roach bomb needs its exact neutral B row")
    if len(joints)!=2 or [j["index"] for j in joints]!=[0,1]:
        raise ValueError("Roach bomb needs its two-joint record-zero body")
    lines=[
        "GFBORG-GAMEPLAY 1",
        "# Authored placed-area B; native bomb proxy, not the retail area-child handoff.",
        "variant=normal",f"health={level[0]}",
        f"movement=grounded-v1|{pace//100}.{pace%100:02d}|13|15",
        "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0",
        "body-motion=static-grounded",
        f"stock=mine-stock|{STOCK_CAP}|on-return",
        f"projectile=mine|bullet|1|1|0|1|0|16|{LIFETIME}|0|land-on-floor",
        "actor-contact=mine|ignore",
        f"blast=mine|expiry|{BLAST_RADIUS}|{AUTHORED_DAMAGE}|opponent",
        f"bomb-proximity=mine|{PROXIMITY_RADIUS}|{ARMING}|blast",
        "return-stock=mine|expiry-or-proximity",
        "look=mine|33|255|178|84",
        "origin=mine|0|0|0|30|fixed",
        "move=mine|b|10|grounded|28|none",
        "entry-stock=mine|mine-stock|1",
        "release=mine|12|mine-stock|1|mine",
    ]
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"authored placed-area bomb; source row/debit exact, placement/blast/timing authored",
        "retail_behavior_verified":False,"preview_clip":"none",
        "source_profile":{"config":CONFIG,"context_table":CONTEXT_TABLE,"main_handler":MAIN_HANDLER,
                          "roots":ROOTS,"callback":f"{CALLBACK:08X}",
                          "row":{"state":0,"substate":0,"selector":45,"handler_pair":[61,0]}},
        "source_debit":{"site":f"{DEBIT:08X}","channel":0,"cost":1,"commit":True},
        "source_area":{"factory":"80085E00","phase_table":"802DA544",
                       "row":{"record":22,"group":3,"parameter":4,"material":0,"cue":12},
                       "blast_radius":BLAST_RADIUS,"proximity_radius":PROXIMITY_RADIUS,
                       "source_arming":SOURCE_ARMING,"stock_cap":STOCK_CAP},
        "authored":{"damage":AUTHORED_DAMAGE,"arming":ARMING,"lifetime":LIFETIME,"pace":pace,
                    "origin":"joint 0 plus (0,0,30)","visual":"authored orange bomb",
                    "note":"the source area row stores the impact record/cue, not damage"},
    }
