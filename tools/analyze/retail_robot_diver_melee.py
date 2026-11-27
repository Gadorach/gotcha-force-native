"""Reviewed robot and diver neutral close-B openings, not retail combo rules."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_dragon_art import record_receipts


CLAW={0x0400,0x040A}
BUILD={0x0402,0x0407,0x0408}
MAGNET={0x0405,0x0409}
OMEGA={0x040B,0x040C,0x040D}
DIVING={0x0805,0x080D,0x080E}
DRILL=0x0401
ROBOT_SINGLE={DRILL,0x0403,0x0404,0x0406}
MODELS=CLAW|BUILD|MAGNET|OMEGA|DIVING|ROBOT_SINGLE
TOOLS={
    0x0402:"13dc8292fb9854de9357d66bfa5be3591f0be11c155514d209f54b99fceb2648",
    0x0407:"6040b527715ab1b9962912e4d5d695276e24819c69224f33de6423715051b7a2",
}
# Expected installer owners: config, context table, program/descriptor roots,
# and whether this profile actually contains the neutral close-B row.
ROUTES=(
    (CLAW,0x80074E48,0x802D4EBC,0x80074EA8,"80074CCC",0x802C0D22,(
        ("802D4F60","802D51B0",0x802C0980,0x802C0660,True),)),
    (BUILD,0x80113128,0x80327F70,0x801131C8,"80112E90",0x803278A0,(
        ("80328030","803282E0",0x80327440,0x80327B00,True),
        ("80328030","803282E0",0x8032744C,0x80327B0C,True))),
    (MAGNET,0x800DADA8,0x80311C7C,0x800DAE48,"800DAB34",0x803119DC,(
        ("80311D38","80311F88",0x80311620,0x803112C0,True),)),
    (OMEGA,0x801D4BA8,0x8038BE38,0x801D4C48,"801D4810",0x8038C80A,(
        ("8038BED8","8038C2F8",0x8038C420,0x8038CA20,True),
        ("8038BFF0","8038C2F8",0x8038C420,0x8038CA20,False),
        ("8038C108","8038C2F8",0x8038C420,0x8038CA20,False))),
    (DIVING,0x80115308,0x80329300,0x80115374,"80114AD4",0x80328986,(
        ("803293E8","80329638",0x80328520,0x80328C00,True),)),
    ({DRILL},0x8009B894,0x802F5808,0x8009B960,"8009B74C",0x802F51C2,(
        ("802F58D0","802F5B20",0x802F4E00,0x802F5440,True),)),
    ({0x0403},0x800DFFD0,0x80316AF4,0x800E0070,"800DFE4C",0x8031657C,(
        ("80316BB8","80316E08",0x803161E0,0x80316760,True),)),
    ({0x0404},0x800D808C,0x8031098C,0x800D812C,"800D7F74",0x803103A2,(
        ("80310A40","80310C90",0x80310000,0x803105C0,True),)),
    ({0x0406},0x80185724,0,0x80185784,"801856E8",0x8035CB5A,(
        ("8035D1C8","8035D3B8",0x8035C7C0,0x8035CDC0,True),)),
)
TEXT=(
    (0x80074E48,0x84,"92fa0d1ee61e15189b52cfb35589877710865212009bf7dd7d6c66fd1d0a8248"),
    (0x80113128,0xC4,"76468b0d7006f35bac075528cfe3e6a4d51fab0dcd4fda1a9ac2bf5ce717edae"),
    (0x800DADA8,0xC4,"98c6c81383a742385f6a9c3871661bdd48563bd172ecff8ab2e45180c8b047ab"),
    (0x801D4BA8,0xC4,"ff45ca5686b637b8e22969e7cb6928bfb988cb830a7641791f1c775a68d8a402"),
    (0x800F2374,0x124,"7ff365ba144400688f369feef1cb04f5da7768855f35ab3af8c97082430b6a66"),
    (0x80115308,0x1A4,"298ec56e8a7221ea57f8acbe364e4daea3df006c2be3ea2e7c8133693fe215e3"),
    (0x801131C8,0x24,"238efed797bf9ab3a429dd0b02dacf8780fed64b430642752d1c5eea6c4aaad8"),
    (0x800DAE48,0x24,"ef4f518c7367803eaa4e70c10398113572e36c9639455e51d37761da26235e1d"),
    (0x801D4C48,0x24,"3a4fc1a5ddd0a97c3f1aa974be7b197d8b7ea5a46fca8255c95a49f4777fdc30"),
    (0x8009b894,0xf0,"7be0350add3851b449b53a8bf6f8b611646df3f42985f2edeca25ee955a67ee6"),
    (0x800dffd0,0xc4,"4ac82c7d47f18825c2ac9c4fd84faf0c0f024ff803fbacf130c0621d88a29b50"),
    (0x800d808c,0xc4,"927dc9cac9c376d3e3447d14111167af7cf610e0c8cb7456493a8280248788ad"),
    (0x80185724,0x84,"4acedf805da4677674112ba9d4c4511c4621a431ff3e62354ea8bf9401ab4247"),
    (0x80185784,0x24,"71bd5be69c78972c09f0502de510be3b758cbef8980e842b20c34c1c0f1b03e0"),
    (0x8009B960,0x24,"f046378301e965f718c1f6784595524169a9c971fbfca836c474ebf877818142"),
    (0x800E0070,0x24,"def872ff836c61d2e517b489309826f4d35395ec3bd41c7efbb4e1ab74aeea12"),
    (0x800D812C,0x24,"d46decf3b25910c28a59b28aeab9b7bf0840b1fdb1877b0cbf11d6fb1d8276e6"),
)
DATA=(
    (0x802C0D22,0x54,"0d526d48e2aed7bb997e8dc35d714e4017f1dff04f747faccc15031c5d32ddb6"),
    (0x803278A0,0x44,"919101c99485a23aeac5e01f66c3cf24fee5d796d878c3797db63e24c5efbf9c"),
    (0x803119DC,0x3C,"cac8ebfd79beed16db8e8fb32b15287d99eb07b53cd302af53b86ea7ab61375a"),
    (0x8038C80A,0x3C,"cac8ebfd79beed16db8e8fb32b15287d99eb07b53cd302af53b86ea7ab61375a"),
    (0x80328986,0x54,"611fcc9a681a4b45979c93b3e3c0dc4af892ff84ae37959a26a9cae8dfb72380"),
    (0x802F51C2,0x3C,"0af55da92356a4ece91032270b93a39a96ae69e53a7ad404b49f6ec7d0202193"),
    (0x8031657C,0x3C,"cac8ebfd79beed16db8e8fb32b15287d99eb07b53cd302af53b86ea7ab61375a"),
    (0x803103A2,0x3C,"93346842f625a62017abe28ebc20b3872bcb81b1721c766ad46f1e1a33af63c5"),
    (0x8035CB5A,0x3C,"93346842f625a62017abe28ebc20b3872bcb81b1721c766ad46f1e1a33af63c5"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Robot/Diver close-B {kind} signature: {at:08X}")


def default_hands(model):
    return [(7,1),(11,4)] if model in CLAW else [(11,4)] if model in TOOLS else []


def close_b_pose(linker,model,actions,slots,parts,body,*,publish_preview):
    _,callback,table,relay,main,script,configs=next(route for route in ROUTES if model in route[0])
    diver=model in DIVING
    phase_owner=relay if diver else 0x800F2374
    entry,request,seed=(0x801153C4,0x80115494,1) if diver else (0x800F23C4,0x800F2478,0)
    leaf=linker.read_word(table) if table else linker.leaf(callback,0x581,0)
    if (any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            leaf!=relay or linker.leaf(phase_owner,0x540,0)!=entry):
        raise ValueError("Robot/Diver close-B installer or phase scope")
    program=decode_program(linker.data,script,max_commands=24,max_span=112)
    if program["status"]!="terminal_wait" or [c["raw"] for c in program["commands"][:2]]!=["0a0100ff","0103000104000000"]:
        raise ValueError("Robot/Diver close-B opening program")
    local={(g,c):at for g,c,at in slots};offset=local.get((1,1))
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if offset is None or hands!=default_hands(model):
        raise ValueError("Robot/Diver exact clip or hand records")
    reported=[]
    bindings=[]
    for profile in actions["profiles"]:
        pro,common,desc,common_desc=map(lambda a:int(a,16),profile["program_and_descriptor_roots"])
        if (common,common_desc,profile["main_handler"])!=(0x802BC080,0x802BAE60,main):
            raise ValueError("Robot/Diver close-B owner")
        rows=[row for group in profile["groups"] if group["group"]=="B-close" for row in group["rows"] if (0,0) in map(tuple,row["contexts"])]
        reported.append((profile["config"],profile["context_table"],pro,desc,bool(rows)))
        if not rows:
            continue
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(1,0,44,f"{callback:08X}"):
            raise ValueError("Robot/Diver exact neutral action context")
        roots=(pro,common,desc,common_desc)
        if linker.table_choice(pro,3,seed)!=script:
            raise ValueError("Robot/Diver exact program binding")
        choices=linker.program_candidate(roots,local,(15,3,seed))["clip_candidates"]
        [opening]=[c for c in choices if (c["command_index"],c["descriptor_choice"],c["preceding_waits"],c["start_frame"])==(1,0,0,0)]
        if [(v["lane"],v["bank"],v["group"],v["choice"],v["archive_offset"]) for v in opening["lanes"] if v["lane"] in (0,1)]!=[
                (0,"local",1,1,offset),(1,"local",1,1,offset)]:
            raise ValueError("Robot/Diver close-B exact clip")
        bindings.extend(rows)
    if sorted(reported)!=sorted(configs):
        raise ValueError("Robot/Diver closed set of normal installer configurations")
    pose={"clip":[1,1],"archive_offset":offset,"program":[3,seed],"descriptor":[3,0],"request_mask":15,
          "entry":f"{entry:08X}","request_site":f"{request:08X}","source_start_frame":0}
    info={"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,
          "program_addresses":[f"{script:08X}"],
          "neutral_program_seed":{"value":seed,"actor_byte":"6EA"},"live_contact_verified":False,
          "close_configurations":sorted({c[0] for c in configs if c[4]})}
    if model in TOOLS:
        receipt=record_receipts(body,{4})
        row=receipt["records"][4]
        if receipt["archive_sha256"]!=TOOLS[model] or row["empty_static"] or any(row[t] for t in (
                "joint_table_offset","material_table_offset","shape_table_offset")):
            raise ValueError("Robot requires its exact static left-hand hierarchy")
        info["static_left_hand_record"]=4
    if publish_preview:
        for row in bindings:
            row["entry_clip_candidate"]=pose
    return info
