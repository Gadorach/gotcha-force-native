"""Static normal close-B joins for Wing and Valkyrie families."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_dragon_art import record_receipts


# IDs, installer config/context, main handler, program/descriptor roots,
# neutral class callback and relay, seed, local program, opening AObj frame.
ROUTES=(
    ((0x0A00,),(("802DB0F8","802DB348"),),"80091A74",0x802DBC00,0x802DB4A0,0x800925A0,0x800925DC,1,0x802DC290,0,1),
    ((0x0A01,),(("80320180","803203D0"),),"80101180",0x8031F920,0x8031F220,0x80101C88,0x80101CD0,1,0x8031FFB0,0,1),
    ((0x0A02,0x0A07),(("8033F9D8","8033FCE0"),("8033FAF0","8033FCE0")),"8014C20C",0x8033EDE0,0x8033F440,
     0x8014C248,0x8014C2B4,3,0x8033F1C4,0,1),
    ((0x0A03,0x0A06),(("8031D4A0","8031D6F0"),),"800F9CE8",0x8031D800,0x8031E100,0x800FA850,0x800FA88C,0,0x8031DEA2,10,0),
    ((0x0A04,0x0A08,0x0A09),(("8031C7B0","8031CA18"),),"800F4D70",0x8031C0E0,0x8031B960,0x800F530C,0x800F5354,1,0x8031C5C2,0,0),
    ((0x0A05,0x0A0A),(("80373E50","803740A0"),),"8019D3B0",0x80374160,0x80374920,0x8019DF20,0x8019DF5C,1,0x803747F0,0,1),
    ((0x0B00,),(("803134C0","80313710"),),"800DE418",0x803124A0,0x80312C80,0x800DE4FC,0x800DE538,0,0x80312B34,0,1),
    ((0x0B01,),(("80346A38","80346C88"),),"8015D4E8",0x80346D60,0x80347560,0x8015D5CC,0x8015D608,0,0x803473FC,0,1),
    ((0x0B02,),(("80348D30","80348F80"),),"80160A58",0x80349800,0x80349FE0,0x80160B3C,0x80160B78,0,0x80349E94,0,1),
    ((0x0B03,),(("80349110","80349360"),),"80160E70",0x8034A780,0x8034AFA0,0x80160F54,0x80160F90,0,0x8034AE16,0,1),
    ((0x0B04,),(("803494D8","80349728"),),"80161248",0x8034B740,0x8034BF20,0x80161354,0x80161390,0,0x8034BDD4,0,1),
    ((0x0B05,0x0B07),(("80375128","80375378"),),"8019E7DC",0x80375480,0x80375C60,0x8019E91C,0x8019E958,0,0x80375B14,0,1),
    ((0x0B06,),(("80379448","80379698"),),"801A5454",0x80379780,0x80379F60,0x801A5538,0x801A5574,0,0x80379E14,0,1),
)
MODELS={model:route for route in ROUTES for model in route[0]}
HIERARCHIES={
    0x0A02:"8551b4d9b845a9147f4845d3a729fa4724dc6d0a4bc2e6aef8c8cab6b5c9e7b0",
    0x0B03:"9c472a85b9702186f3011e0405a37ad21e66161c4425844852ee385705518b1c",
}
TEXT=(
    (0x800925a0,0x68,"5feaec6fd1f46554f451114cf1c2ee431a1d69d6692169791f25bdf5f6c88da2"),
    (0x80101c88,0x6c,"5b10b636a0329fdab4e32b430cb6ae8e16d442d80dd41f5c9b1d807ffe22681e"),
    (0x8014c248,0x9c,"3f9103df25538b6fefe6bccd6340fe480cfd5041c62ef935a057a9265f18fe04"),
    (0x800fa850,0x6c,"703fc794e794af4c5c7107285509a97f47f583cf26d7d141e1b23b11cc256658"),
    (0x800f530c,0x6c,"511491d85de80a10c2fb7f5107f54b8fae1ba321902d58b83016799ae9d25a3a"),
    (0x8019df20,0x6c,"e4048ae81baefaccb59a4bbf7754ab590d63f3af14126ec56bce6a3540c560c0"),
    (0x800de4fc,0x60,"7a904780bfba66a927653b0d74075e4c88b2dcc92bc7cf3d2d22f1705a443250"),
    (0x8015d5cc,0x60,"51b721e2fa6220549de9f1482240ac3637d29f2e8e37718187c68901a67e9570"),
    (0x80160b3c,0x60,"a881922fabb5f9ff983f227155764c0b6eb175d49af4a01d6ea84de6d59d9e12"),
    (0x80160f54,0x60,"084ed47c933f33d86eff45ef488d4466849862d831f038564579a712e0ef292a"),
    (0x80161354,0x60,"5c6155cfc84d8ff28bd9d7650da95a1e9235c754898888872c9d8cbd5779d7a2"),
    (0x8019e91c,0x60,"2130796129800f129a9dd68714783b6c1298b4739e3195422b4b25f56763b1bf"),
    (0x801a5538,0x60,"fceb4ec8b6121f0b3d226edf7730790dbd7112d06ff0ade184e2e78d652c3087"),
    (0x80147e34,0x148,"95ac558126fa47d365c7e015928431cb3a7f46a5756018268f08d811fe247a9e"),
    (0x8014a8c0,0x130,"31f2691862807311bc1cf5bf0a1f327ac3c35af97a6f5c5c68dd4657f6e16bf3"),
)
DATA=(
    (0x802dc290,0x50,"3b6ac1cb80f85ff81f8dd8006eb53a7c00619c6a3c5f3337be64d7071d47806c"),
    (0x8031ffb0,0x38,"4b2ad20fa05a443e944369ed0892bcc738a370e3072bbaada4c8c2ab295b9985"),
    (0x8033f1c4,0x58,"797109992b4d76124f24e65b1c6088b904ad276a1308d83d30afb118dd1c1b69"),
    (0x8031dea2,0x3c,"ab4a23c1369ebef5ec66154e3e058645e0516d3cb214535d8398fa8b53d74a57"),
    (0x8031c5c2,0x44,"ee47990e4446387aec0c55963a49ecf0624ba18d185c192d0a122f16f02189b0"),
    (0x803747f0,0x50,"3b6ac1cb80f85ff81f8dd8006eb53a7c00619c6a3c5f3337be64d7071d47806c"),
    *((a,0x34,"7be2f6b483ad68e8df90695ec6817fd4fc085f7a576d390809cf10a1866dd170") for a in
      (0x80312b34,0x803473fc,0x80349e94,0x8034ae16,0x8034bdd4,0x80375b14,0x80379e14)),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Wing/Valkyrie close-B {kind} signature: {at:08X}")


def default_hands(model):
    return [] if model in (0x0A05,0x0A0A,0x0B05,0x0B07) else [(7,1),(11,4 if model==0x0A01 else 5)]


def close_b_pose(linker,model,actions,slots,parts,body,*,publish_preview):
    _,configs,main,program_root,descriptor_root,callback,relay,seed,script,start,descriptor=MODELS[model]
    roots=(program_root,0x802BC080,descriptor_root,0x802BB714)
    leaf=(linker.read_word(0x8033F920 if model==0x0A02 else 0x8033F934) if callback==0x8014C248
          else linker.leaf(callback,0x581,0))
    phase,entry,request=(0x80147E34,0x80147E90,0x80147F64) if model<0x0B00 else (0x8014A8C0,0x8014A91C,0x8014A9D8)
    if (leaf!=relay or linker.text(relay+8)!=(0x38800000|seed) or
            linker.leaf(phase,0x540,0)!=entry or linker.table_choice(program_root,3,seed)!=script):
        raise ValueError("Wing/Valkyrie close-B exact route or neutral seed")
    profiles=actions["profiles"]
    if (sorted((p["config"],p["context_table"]) for p in profiles)!=sorted(configs) or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("Wing/Valkyrie close-B installer extent")
    program=decode_program(linker.data,script,max_commands=24,max_span=112)
    if program["status"]!="terminal_wait" or program["commands"][0]["raw"] not in ("0a0100ff","0a017fff"):
        raise ValueError("Wing/Valkyrie close-B program opening")
    local={(g,c):at for g,c,at in slots};offset=local.get((1,1))
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if offset is None or hands!=default_hands(model):
        raise ValueError("Wing/Valkyrie close-B clip or hand records")
    selects=linker.program_candidate(roots,local,(15,3,seed))["clip_candidates"]
    [opening]=[c for c in selects if (c["command_index"],c["descriptor_choice"],c["preceding_waits"],c["start_frame"])==(1,descriptor,0,start)]
    if [(l["lane"],l["bank"],l["group"],l["choice"],l["archive_offset"]) for l in opening["lanes"] if l["lane"] in (0,1)]!=[
            (0,"local",1,1,offset),(1,"local",1,1,offset)]:
        raise ValueError("Wing/Valkyrie close-B exact lane binding")
    pose={"clip":[1,1],"archive_offset":offset,"program":[3,seed],"descriptor":[3,descriptor],"request_mask":15,
          "entry":f"{entry:08X}","request_site":f"{request:08X}","source_start_frame":start}
    selector=45 if callback==0x800FA850 else 44
    for p in profiles:
        if (p["main_handler"],tuple(int(a,16) for a in p["program_and_descriptor_roots"]))!=(main,roots):
            raise ValueError("Wing/Valkyrie close-B program owner")
        rows=[r for g in p["groups"] if g["group"]=="B-close" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,selector,f"{callback:08X}"):
            raise ValueError(f"Wing/Valkyrie {model:04X} close-B exact neutral input row")
        if publish_preview:
            rows[0]["entry_clip_candidate"]=pose
    info={"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,"live_contact_verified":False,
          "program_addresses":[f"{script:08X}"],
          "neutral_program_seed":{"relay":f"{relay:08X}","value":seed,"actor_byte":"6EA"},
          "scope":"grounded opening; no retail hover, aerial attacks, or combos"}
    if model in HIERARCHIES:
        receipt=record_receipts(body,{1,5})
        if receipt["archive_sha256"]!=HIERARCHIES[model] or any(r["empty_static"] or any(r[k] for k in (
                "joint_table_offset","material_table_offset","shape_table_offset")) for r in receipt["records"].values()):
            raise ValueError("Wing/Valkyrie exact static hand hierarchies")
        info["static_hand_hierarchies"]=[1,5]
    return info
