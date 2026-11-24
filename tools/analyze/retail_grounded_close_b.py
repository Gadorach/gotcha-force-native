"""Scoped grounded opening poses, independent of the source attack effects."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


IMPERIAL=0x0209
GUARDS={0x0302,0x0306}
CYBER_HERO=0x0808
HONEY={0x0904,0x090B}
POWERED,BILLY,STAR=0x0101,0x0108,0x0804
LATER={POWERED,BILLY,STAR}
MODELS={IMPERIAL,CYBER_HERO}|GUARDS|HONEY|LATER

# main callback, neutral wrapper, its first animation-request phase,
# request site, config/context/roots, and exact program-to-clip joins.
ROUTES={
    IMPERIAL:("801B8074",0x801B8170,0x801B822C,0x801B8318,0x80342F80,0x803438E0,
              {("80382238","80382428"):(1,"801B8170")},
              ((0,0x8034342A,0,1),(11,0x803436DA,0,1)),1,8),
    0x0302:("801080E4",0x80108120,0x8010B400,0x8010B478,0x80323320,0x80323C00,
            {("803241C8","80324478"):(1,"80108120"),("80382498","80382670"):(0,"8010A508")},
            ((0,0x80323814,0,0),),0,0),
    CYBER_HERO:("801D94A4",0x801D94E0,0x801C9054,0x801C911C,0x8038F640,0x8038FFA0,
                {("8038F208","8038F470"):(1,"801D94E0")},
                ((1,0x8038F9A2,23,1),),30,0),
    0x0904:("8012C6A8",0x8012CE08,0x8012CF50,0x8012D050,0x80332980,0x80333000,
            {("80333710","80333960"):(1,"8012CE08")},
            ((2,0x80332DEC,2,1),),6,0),
    0x090B:("801A1290",0x801A1C14,0x801A1D5C,0x801A1E58,0x80378B80,0x803787C0,
            {("803782B0","803785D0"):(1,"801A1C14"),("803783C8","803785D0"):None},
            ((2,0x80378F72,2,1),),6,0),
}
LATER_ROUTES={
    POWERED:("80166110",0x80166D40,0x80166D7C,0x8034F382,
             {("8034E830","8034EAE0"):(0x8034EBE0,0x8034F640)}),
    BILLY:("800722F8",0x80073558,0x80073594,0x802BF822,
           {("802D4478","802D46C8"):(0x802BF040,0x802BE960)}),
    STAR:("8010F86C",0x8010FF48,0x8010FFB4,0x8032606E,
          {("80326CF0","80327058"):(0x80325C80,0x80326640),
           ("80326E08","80327058"):(0x80325C80,0x8032664C)}),
}
TEXT=(
    (0x801b8170,0x1ac,"35cf335db3e47facb579d976ba78803c164cd34fa236edd1c40d75fee74cc774"),
    (0x80108120,0x6c,"c1d13e03360820d7970cf93aa9b9785e962b074d9e96037ff7f095da61d1a60d"),
    (0x801d94e0,0x78,"90cc4505ed02a4c5f3ef9833d52760f339bb718b063a8c5273b856b87112d5a9"),
    (0x8012ce08,0x24c,"8cd3144bb42162b17a7808da1ad0129ea09e9af110f478cbfe97c79864746b98"),
    (0x801a1c14,0x25c,"ebd9a8e88f1298be7d8bf325e59bc74d2ff61318c167d62024127afca4f320a0"),
    (0x80073558,0x6c,"939ab1797ab6fb7c64021112f98268b4adb81e25849efc3f8ff5653d98e8b20e"),
    (0x80166d40,0x6c,"e2f0ad9121870bb37effec10d3c6c34b990414350f601d892d088bdfea3ff294"),
    (0x8010ff48,0x420,"d298ba5b4ab80f134bc1c594561fc201333a2321324136f6a5e2260878343c46"),
)
DATA=(
    (0x8034342a,0x64,"48b60f4cdf56ab2fae2de0055e10540577cdff4234d70ad319de96ae51ae1106"),
    (0x803436da,0x64,"3bb79f443ac017c467f91e98ef8981b984a4741497159497ad36492c3e730bcc"),
    (0x80323814,0x20,"cbcc3fc2cc9d76861d4603281774a12bfdc11da37e44e44ed6c8bb01d2351363"),
    (0x8038f9a2,0x34,"0d55a1785932b83030ab36509879a95791b6cfb21fc134464c33cb55fe935252"),
    (0x80332dec,0x44,"19f241ff93c3fdaed88516ca7ced16fa7f061b3e5cfe8568f93ee2307cccf397"),
    (0x80378f72,0x44,"f2901a81039b8a3aabb3c00a96b08ecad4d90c69f549fb4f4b745e36ed3bcee2"),
    (0x8034f382,0x78,"bf5b26e8e493bfaa8d1c1b80d11892d4349e2edfae7a7c5dc81864ddf0bbe533"),
    (0x802bf822,0x78,"9d17e540a22106d74b0011e11b28263c0afe1f237d6f5aefdc8ccf185f0b7d87"),
    (0x8032606e,0x2c,"7df479059dd0accce6e95f33480a2bfc737c75be4a068c50f7e6204dca034b89"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"grounded close-B {kind} signature: {at:08X}")


def route(model):
    return ROUTES[0x0302 if model in GUARDS else model]


def clip(model):
    return (1,1,0) if model in LATER else (1,*route(model)[-2:])


def default_hands(model):
    if model==IMPERIAL:
        return [(7,2),(11,5)]
    if model==0x090B:
        return []
    return [(7,1),(11,4 if model in (CYBER_HERO,STAR) else 5)]


def phase_matches(linker,model,callback,entry):
    phase=lambda at,index=0:linker.leaf(at,0x540,index)
    relay=lambda at:linker.leaf(at,0x581,0)
    if model==IMPERIAL:
        return linker.read_word(0x80382204)==0x801B81D0 and phase(0x801B81D0)==entry
    if model in GUARDS:
        return relay(callback)==0x80108168 and phase(0x8010B2F4,1)==entry
    if model==CYBER_HERO:
        return relay(callback)==0x801D9528 and phase(0x801C9004)==entry and linker.read_word(0x8038F03C)==1
    leaf=0x8012CE44 if model==0x0904 else 0x801A1C50
    return relay(callback)==leaf and phase(leaf,1)==entry


def close_b_pose(linker,model,actions,slots,parts,*,publish_preview):
    if model in LATER:
        return later_close_b_pose(linker,model,actions,slots,parts,publish_preview=publish_preview)
    main,callback,entry,request,program,desc,configs,choices,choice,start=route(model)
    roots=(program,0x802BC080,desc,0x802BAE60)
    profiles=actions["profiles"]
    if (len(profiles)!=len(configs) or {(p["config"],p["context_table"]) for p in profiles}!=configs.keys() or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            not phase_matches(linker,model,callback,entry)):
        raise ValueError("grounded close-B exact installer or phase scope")
    local={(g,c):at for g,c,at in slots};offset=local.get((1,choice))
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if offset is None or hands!=default_hands(model):
        raise ValueError("grounded close-B exact local clip or hand records")
    checked=[]
    for seed,script,descriptor,command in choices:
        if linker.table_choice(program,3,seed)!=script or decode_program(linker.data,script,max_commands=24,max_span=112)["status"]!="terminal_wait":
            raise ValueError("grounded close-B exact program")
        options=linker.program_candidate(roots,local,(15,3,seed))["clip_candidates"]
        rows=[x for x in options if (x["command_index"],x["descriptor_choice"],x["preceding_waits"],x["start_frame"])==(command,descriptor,0,start)]
        if len(rows)!=1 or [(v["lane"],v["bank"],v["group"],v["choice"],v["archive_offset"])
                            for v in rows[0]["lanes"] if v["lane"] in (0,1)]!=[(0,"local",1,choice,offset),(1,"local",1,choice,offset)]:
            raise ValueError("grounded close-B exact local lane")
        checked.append({"program":[3,seed],"descriptor":[3,descriptor],"program_address":f"{script:08X}"})
    pose={**checked[0],"clip":[1,choice],"archive_offset":offset,"request_mask":15,
          "entry":f"{entry:08X}","request_site":f"{request:08X}","source_start_frame":start}
    for p in profiles:
        expected=configs[p["config"],p["context_table"]]
        if (p["main_handler"],tuple(int(v,16) for v in p["program_and_descriptor_roots"]))!=(main,roots):
            raise ValueError("grounded close-B exact program owner")
        rows=[q for g in p["groups"] if g["group"]=="B-close" for q in g["rows"] if (0,0) in map(tuple,q["contexts"])]
        if expected is None:
            if rows:
                raise ValueError("unexpected alternate close-B row")
            continue
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                expected[0],0,44,expected[1]):
            raise ValueError("grounded close-B exact neutral context")
        if publish_preview and expected[0]==1:
            rows[0]["entry_clip_candidate"]=pose
    return {"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,
            "same_opening_programs":checked,"live_contact_verified":False,
            "scope":"exact close opening request, no source combos, guard/barrier state or weapon scripts"}


def later_close_b_pose(linker,model,actions,slots,parts,*,publish_preview):
    main,callback,relay,script,configs=LATER_ROUTES[model]
    star=model==STAR
    entry,request,seed=(0x801101F0,0x80110360,1) if star else (0x80161508,0x801615F8,0)
    route_ok=(linker.read_word(0x80326B20)==relay and linker.leaf(relay,0x540,0)==0x80110010 and
              linker.leaf(relay,0x540,2)==entry) if star else (
                  linker.leaf(callback,0x581,0)==relay and linker.text(relay+8)==0x38800000 and
                  linker.leaf(0x801613FC,0x540,1)==entry)
    profiles=actions["profiles"]
    if (not route_ok or len(profiles)!=len(configs) or {(p["config"],p["context_table"]) for p in profiles}!=configs.keys() or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("later close-B exact installer or phase")
    local={(g,c):at for g,c,at in slots};offset=local.get((1,1))
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if offset is None or hands!=default_hands(model) or decode_program(linker.data,script,max_commands=32,max_span=128)["status"]!="terminal_wait":
        raise ValueError("later close-B exact clip, program or hands")
    pose={"clip":[1,1],"archive_offset":offset,"program":[3,seed],"descriptor":[3,1],"request_mask":15,
          "entry":f"{entry:08X}","request_site":f"{request:08X}","source_start_frame":0}
    for p in profiles:
        program,desc=configs[p["config"],p["context_table"]]
        roots=(program,0x802BC080,desc,0x802BAE60)
        if ((p["main_handler"],tuple(int(s,16) for s in p["program_and_descriptor_roots"]))!=(main,roots) or
                linker.table_choice(program,3,seed)!=script):
            raise ValueError("later close-B exact descriptor owner")
        options=linker.program_candidate(roots,local,(15,3,seed))["clip_candidates"]
        join=(1,1,0,0) if star else (5,1,1,0)
        chosen=[c for c in options if (c["command_index"],c["descriptor_choice"],c["preceding_waits"],c["start_frame"])==join]
        if len(chosen)!=1 or [(v["lane"],v["bank"],v["group"],v["choice"],v["archive_offset"])
                              for v in chosen[0]["lanes"] if v["lane"] in (0,1)]!=[(0,"local",1,1,offset),(1,"local",1,1,offset)]:
            raise ValueError("later close-B exact lane join")
        rows=[q for g in p["groups"] if g["group"]=="B-close" for q in g["rows"] if (0,0) in map(tuple,q["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,44,f"{callback:08X}"):
            raise ValueError("later close-B exact neutral context")
        if publish_preview:
            rows[0]["entry_clip_candidate"]=pose
    return {"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,
            "scope":"later matching-lane attack only; no source hold, lead-in, movement, or hit timing","live_contact_verified":False}
