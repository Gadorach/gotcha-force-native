"""Static neutral-B route for the four Blue/Atlas normal packages."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


MODELS={0x0601,0x0609,0x0617,0x061D}
CONFIGS={"8030C798":0x8030B900,"8030C8B0":0x8030B90C}
HAND_RECORDS=[(7,1),(11,4)]
PROGRAM=0x8030BF5C
TEXT=(
    (0x800CF884,0x70,"a6473e75fb7e44f9776ff41c96a96c147f47be87339cd634e0e0935ba0292d87"),
    (0x80177DD8,0x138,"4106128448bca88d373592727640af6966c50c1d0d0b0b3ab9b40f085eb3298f"),
)
DATA=(
    (PROGRAM,0x58,"aad66f703c6158a8326269b67ede0301fb0a9c355fd4b6d11cf91d42d065ef7d"),
    (0x8030CE94,4,"df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Blue/Atlas close-B {kind} signature: {at:08X}")


def close_b_pose(linker,model,actions,slots,parts,*,publish_preview):
    profiles=actions["profiles"]
    if (model not in MODELS or len(profiles)!=len(CONFIGS) or {p["config"] for p in profiles}!=CONFIGS.keys() or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            linker.leaf(0x800CF884,0x581,0)!=0x800CF8C0 or linker.leaf(0x80177DD8,0x540,0)!=0x80177E28 or
            linker.read_word(0x8030CE94)!=0):
        raise ValueError("Blue/Atlas close-B installer or phase scope")
    program=decode_program(linker.data,PROGRAM,max_commands=24,max_span=112)
    if program["status"]!="terminal_wait" or [c["raw"] for c in program["commands"][:2]]!=["0a0100ff","0103000105000000"]:
        raise ValueError("Blue/Atlas close-B opening program")
    local={(g,c):at for g,c,at in slots};offset=local.get((1,1))
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if offset is None or hands!=HAND_RECORDS:
        raise ValueError("Blue/Atlas needs its exact close clip and hand records")
    bindings=[]
    for profile in profiles:
        roots=(CONFIGS[profile["config"]],0x802BCC64,0x8030AB20,0x802BB698)
        if ((profile["context_table"],profile["main_handler"],tuple(int(x,16) for x in profile["program_and_descriptor_roots"]))!=
                ("8030CBC0","800CEA5C",roots) or linker.table_choice(roots[0],3,0)!=PROGRAM):
            raise ValueError("Blue/Atlas close-B route owner")
        candidates=linker.program_candidate(roots,local,(15,3,0))["clip_candidates"]
        opening=[c for c in candidates if (c["command_index"],c["descriptor_choice"],c["preceding_waits"],c["start_frame"])==(1,0,0,0)]
        if len(opening)!=1 or [(l["lane"],l["bank"],l["group"],l["choice"],l["archive_offset"])
                               for l in opening[0]["lanes"] if l["lane"] in (0,1)]!=[(0,"local",1,1,offset),(1,"local",1,1,offset)]:
            raise ValueError("Blue/Atlas close-B local opening")
        rows=[row for group in profile["groups"] if group["group"]=="B-close" for row in group["rows"] if (0,0) in map(tuple,row["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(1,0,44,"800CF884"):
            raise ValueError("Blue/Atlas close-B neutral context")
        bindings.extend(rows)
    pose={"clip":[1,1],"archive_offset":offset,"program":[3,0],"descriptor":[3,0],"request_mask":15,
          "entry":"80177E28","request_site":"80177EF0","source_start_frame":0}
    if publish_preview:
        for row in bindings:
            row["entry_clip_candidate"]=pose
    return {"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,
            "neutral_program_seed":{"address":"8030CE94","value":0,"actor_byte":"6EA"},
            "class_callback_chain":["800CF884","800CF8C0","80177DD8","80177E28"],
            "live_contact_verified":False,"excluded_moves":["ranged B","X","fusion or form changes"]}
