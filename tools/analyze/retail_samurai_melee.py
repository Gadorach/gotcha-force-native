"""Static joins for the exact neutral close-B pose of the armed Samurai cohort."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


# Configuration IDs, main, substate wrapper, neutral relay, seed address, program.
NORMAL=(("80308150",),"803083A0","800C7FAC",0x800C7FE8,0x800C805C,0x8030800C,0x80307C72)
DEMON=(("8032D2A8","8032D2A8","8032D3C0","8032D3C0"),"8032D610","80122C74",0x80122CB0,0x80122D24,0x8032C7A4,0x8032CC2A)
VAMPIRE=(("8033C4A8",),"8033C6F8","80143F70",0x80143FAC,0x80144020,0x8033C36C,0x8033CCBC)
SHOGUN=(("80351A10",),"80351C60","80175148",0x80175184,0x801751F8,0x803518F4,0x803522AA)
SONIC=(("8034D688",),"8034D8D8","80164628",0x80164664,0x801646D8,0x8034D558,0x8034DEB2)
BEAM=(("8032A278",),"8032A4C8","80116ED0",0x80116F0C,0x80116F48,None,0x80329B42)
MODELS={0x0700:NORMAL,0x0701:DEMON,0x0702:VAMPIRE,0x0704:SHOGUN,0x0707:SHOGUN,
        0x0708:DEMON,0x0709:NORMAL,0x070A:VAMPIRE,0x0703:SONIC,0x0706:BEAM}
BLOCKED_MATERIALS={0x0705,0x070B,0x070C,0x070D}
TEXT=(
    (0x80148D74,0x134,"f5f1607cf03cf6e13928fba773dead0606aacfdfc0b336a8b03e22c9ce02cb53"),
    (0x800C7FE8,0x9C,"e7fb1c64e44c0c6a8aba41fe196c4405ffabbddb0f0cd1a1fc40ea97374bb22a"),
    (0x80122CB0,0x9C,"6afdd905fcb888fa10c7ead6aa8a5041cd9e8d64c771c35067f145ff70489fcd"),
    (0x80143FAC,0x9C,"cb61590cb0746f211e49da77a1aaaf4af1e32fc94a2855d73539afe533567377"),
    (0x80175184,0x9C,"ce9af72adefd6c2d25064dfa318ee287da1ba9df1501edd9391f635b6e6ac0e5"),
    (0x80164664,0x9C,"b56ad27b8c0c1f90642ed7c0667ba395b8808c6ae6505b758e8f862b6569b31e"),
    (0x80116F0C,0x98,"0346bf8e226a3888804e27bd266d207030896d26d651bc1184b4e9b118fd0b60"),
)
DATA=(
    (0x8033ED20,0x0C,"e3004ddc078e878236797e2736bb1e14707a5bceca423bde42e9ede0164fbe33"),
    (0x80307FF8,0x18,"7f86c94b932b83b3e0351273d6c68872901892a384b72cf8b806ea8d36c04550"),
    (0x8032C790,0x18,"c6404bb94b6ccf960ca73f8ed9ce8796e9bf718dde9f2e3f38aa23c4d85d8749"),
    (0x8033C358,0x18,"35414fee85f37a9f6021e718a22de926da0593dcdcf3fa33da92b744054daea2"),
    (0x803518E0,0x18,"d50e25a73a131bb860587260e0a10cf096ff6a056a096dd56f3fbe087d0fc1e1"),
    (0x80307C72,0x38,"b6438b6857987185c4df54442e457483fce6d7a9e0d4337bb63e994e5c6b9ad5"),
    (0x8032CC2A,0x38,"bbdff1216925d420ca1f0986a8074fe7f7bd2acb9f7bd2447b134c84746c8d3f"),
    (0x8033CCBC,0x38,"b6438b6857987185c4df54442e457483fce6d7a9e0d4337bb63e994e5c6b9ad5"),
    (0x803522AA,0x38,"c36f0add94cfdb508a2f5bda0c84f15a00462e254d6452ce452fe562f984e970"),
    (0x8034D544,0x18,"b17553bcb17e90f9c45a67fa867951786bb1e7a5ee3e9277a6c6d1230b55af06"),
    (0x8034DEB2,0x38,"b6438b6857987185c4df54442e457483fce6d7a9e0d4337bb63e994e5c6b9ad5"),
    (0x8032A1B0,0x18,"042ac5c6ae34badf145a6f5889b646470b4eab73739de35a6ceacfe76fe78882"),
    (0x80329B42,0x54,"043515af426497eafc977ccd2531b007be07a4b11ae8410fad863d0eb0af1935"),
)


def verify_source(dol):
    for kind,rows in (("text",TEXT),("data",DATA)):
        for at,size,digest in rows:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Samurai close-B {kind} signature: {at:08X}")


def close_b_pose(linker, model, actions, slots, parts, *, publish_preview):
    configs,table,handler,callback,relay,seed,program=MODELS[model]
    profiles=actions["profiles"]
    if (tuple(sorted(p["config"] for p in profiles))!=configs or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("Samurai close-B installer scope")
    if (linker.leaf(callback,0x581,0)!=relay or (seed is not None and linker.read_word(seed)!=0) or
            linker.leaf(0x80148D74,0x540,0)!=0x80148DC4):
        raise ValueError("Samurai neutral substate, seed or phase owner")
    def must_link(site,dst):
        word=linker.read_word(site,"text")
        delta=word&0x03fffffc
        return word>>26==18 and word&3==1 and site+delta-(0x04000000 if delta&0x02000000 else 0)==dst
    if (not must_link(relay+(0x48 if seed is None else 20),0x80148D74) or not must_link(0x80148E90,0x8004BEB8) or
            (seed is not None and (linker.read_word(relay+16,"text")&0xFFFF)!=(seed&0xFFFF))):
        raise ValueError("Samurai close-B caller or program request")
    decoded=decode_program(linker.data,program,max_commands=24,max_span=96)
    if decoded["status"]!="terminal_wait" or [op["raw"] for op in decoded["commands"][:2]]!=["0a0100ff","0103000000000000"]:
        raise ValueError("Samurai qualified initial pose command")
    local={(g,c):at for g,c,at in slots}
    bindings=[]
    for p in profiles:
        roots=tuple(int(r,16) for r in p["program_and_descriptor_roots"])
        if (p["context_table"],p["main_handler"])!=(table,handler) or linker.table_choice(roots[0],3,0)!=program:
            raise ValueError("Samurai close-B root")
        rows=[r for group in p["groups"] if group["group"]=="B-close" for r in group["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,44,f"{callback:08X}"):
            raise ValueError("Samurai raw neutral context")
        candidates=linker.program_candidate(roots,local,(15,3,0))["clip_candidates"]
        first=[c for c in candidates if c["command_index"]==1 and (c["descriptor_choice"],c["start_frame"],c["preceding_waits"])==(0,0,0)]
        expected=local.get((1,1))
        if len(first)!=1 or expected is None or [(l["bank"],l["group"],l["choice"],l["archive_offset"]) for l in
                first[0]["lanes"] if l["lane"] in (0,1)]!=[("local",1,1,expected)]*2:
            raise ValueError("Samurai exact close-B pose unavailable")
        bindings.append(rows[0])
    defaults=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if defaults!=[(7,1),(11,2)]:
        raise ValueError("Samurai exact hand records unavailable")
    pose={"clip":[1,1],"archive_offset":local[1,1],"program":[3,0],"descriptor":[3,0],"request_mask":15,
          "entry":"80148DC4","request_site":"80148E90","source_start_frame":0,
          "class_callback_chain":[f"{callback:08X}",f"{relay:08X}","80148D74"]}
    if publish_preview:
        for row in bindings:
            row["entry_clip_candidate"]=pose
    seed_receipt={"address":f"{seed:08X}","value":0,"actor_byte":"6EA"} if seed is not None else {
        "site":"80116F50","value":0,"actor_byte":"6EA","guard":"signed actor +6F6 <= 0",
        "excluded_positive_branch":{"site":"80116F64","value":6}}
    return {"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":defaults,
            "program_addresses":[f"{program:08X}"],
            "neutral_program_seed":seed_receipt,
            "program_class_commands":[{"site":f"{op['address']:08X}","raw":op["raw"]} for op in decoded["commands"]
                                      if op["kind"]=="class_event"],"live_contact_verified":False}
