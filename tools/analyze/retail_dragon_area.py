"""Static type-5 impact-area operands selected by the six type-47 Dragon carriers."""
import math
import struct
from hashlib import sha256


TABLE = 0x802DA0B8
TABLE_SHA = "9e8a37713e6b95e1ec534f5805983cf0a9a092a54188316b9a51a505a9a8e5c2"
PHASE_TABLE = 0x802DA544
PHASES = (0x80086084,0x80086268,0x800864D0)
RECORD = 22
SOUND = 12
COMMON_COLLISION_SIZE = 21504
COMMON_COLLISION_SHA = "e6a41d20e919e0fcf77898af7800f2beaffc459e2267bf4c227879d38a40c0a1"
TEXT = (
    (0x80085E00,0x128,"8e4e9e754e10d0a9aa506f32a6ddd8b707fa79eb6eda278760bd6bcf4a03d3e4"),
    (0x80086048,0x3C,"921855ec93ee0be62aca4e911e3109f338c427b22fb3d30a730e55d66563dd25"),
    (0x80086084,0x1E4,"336ef667addefcfec2d795c35aeadcde026a043547a27f977f14529362b6dcfd"),
    (0x8008AE10,0x50,"9f137db7ea1175b4451042224c71a9e1927ce871fdca826f2dcdf0a5f0c519b9"),
    (0x8008AE60,0x190,"ed773c3f3f4973120e9f352561f67629417f7a374b39e2215c65901ffba46c62"),
    (0x80086268,0xE8,"c65947796c024991ef2b5e648e4c900339a886c68e07423e72fcaa3140bda18f"),
    (0x80040F1C,0x58,"03ca5f188d47380cd642213aaa4abac531c8031fab18588f103443637270c9b6"),
    (0x80040F74,0x80,"80ca88963c6cf750853b51192948f7e9a037d5046b34ed6b55d51d1b7b2e8978"),
)


def area_index(dol, carriers, *, sound_resolver):
    for at,size,digest in TEXT:
        if sha256(dol.read(at,size,"text")).hexdigest()!=digest:
            raise ValueError(f"elemental impact area text signature: {at:08X}")
    if struct.unpack(">3I",dol.read(PHASE_TABLE,12,"data"))!=PHASES:
        raise ValueError("elemental impact area dispatch signature")
    table=dol.read(TABLE,27*36,"data")
    if sha256(table).hexdigest()!=TABLE_SHA:
        raise ValueError("elemental impact area row table signature")
    tone=sound_resolver(SOUND)
    if tone.get("status")!="static-tone-record":
        raise ValueError("elemental impact area sound tone unavailable")
    rows={}
    for model,carrier in carriers.items():
        key=carrier["tail"]["area_key"]
        if not 0<=key<27 or key in rows:
            raise ValueError("elemental impact area key domain or identity")
        data=table[key*36:(key+1)*36]
        record,group,parameter,material=struct.unpack_from(">4h",data)
        cue,=struct.unpack_from(">i",data,32)
        if (record,group,parameter,cue)!=(RECORD,60,1,SOUND) or material not in (0,1):
            raise ValueError("elemental impact area row scope")
        floats=[]
        for at in range(8,32,4):
            bits=data[at:at+4]
            value,=struct.unpack(">f",bits)
            if not math.isfinite(value) or abs(value)>1e5:
                raise ValueError("elemental impact area numeric domain")
            floats.append({"offset":f"{at:02X}","bits":bits.hex(),"value":value})
        rows[key]={"model":model,"carrier_key":carrier["key"],"area_key":key,
                   "row_address":f"{TABLE+key*36:08X}","raw_row":data.hex(),
                   "visual_record":record,"collision_group":group,"material_selector":material,
                   "initialization_parameter":parameter,"source_float_operands":floats}
    if set(rows)!={3,21,22,23,24,25}:
        raise ValueError("elemental impact area cohort")
    return {"kind":"static elemental impact area operands; not engine content",
            "family":"type-5","factory":"80085E00","phase_table":f"{PHASE_TABLE:08X}",
            "phase_zero":{"collision_bind":"800860E0","collision_bank":"live-scene common collision",
                          "shape_verified":False,"area_origin":"accepted factory point",
                          "credit":"retained carrier credit; carrier remains parent",
                          "record_bind":"800861E4","record_owner":"shared effect archive",
                          "cue":{"site":"8008623C","source_sound_id":SOUND,"owner":"area-child",
                                 "guard":"group bind succeeds and row tone is nonnegative",**tone}},
            "live_damage_verified":False,"rows":[rows[k] for k in sorted(rows)]}


def common_collision(bank):
    """Join scene common block zero to the exact group-60 sphere descriptor."""
    if len(bank)!=COMMON_COLLISION_SIZE or sha256(bank).hexdigest()!=COMMON_COLLISION_SHA:
        raise ValueError("elemental impact common collision block signature")
    group=60
    header=bank[4*group:4*(group+1)]
    if header!=bytes((94,255,255,255)):
        raise ValueError("elemental impact common group-60 header")
    row=header[0]
    at=0x400+80*row
    raw=bank[at:at+80]
    if struct.unpack_from(">BBHHhhH",raw)!=(0,0,1,4,0,-1,0):
        raise ValueError("elemental impact common sphere descriptor")
    x,y,z,radius=struct.unpack_from(">4f",raw,12)
    if (x,y,z,radius)!=(0.,0.,0.,310.):
        raise ValueError("elemental impact common sphere operands")
    return {"member":696,"name":"cmn_data.pzz","pzz_slot":0,"bank_sha256":sha256(bank).hexdigest(),
            "context_assignment":"80040FC0","group":group,"group_header":header.hex(),
            "descriptor_row":row,"descriptor_offset":at,"raw_descriptor":raw.hex(),
            "shape":"sphere","center":[x,y,z],"source_radius":radius,"collision_parameter":4,
            "source_scale_fields":{"object_axes":["058","05C","060"],"uniform_scale":"0B4",
                                   "initial_write":"800861AC","active_write":"80086338",
                                   "initial_source":"area row +08 multiplied by the size copied from the carrier",
                                   "active_source":"area +148"},
            "live_publication_verified":False,"live_radius_verified":False}
