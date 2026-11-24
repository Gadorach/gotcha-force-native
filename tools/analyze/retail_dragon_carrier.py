"""Offline type-47 carrier operands. No runtime scheduler or projectile rules."""
from hashlib import sha256
import math
import struct

from tools.analyze import retail_dragon_shot as dragon


ROW_TABLE = 0x80301AE8
PHASE_TABLE = 0x80301C08
TRAIL_TABLE = 0x80301C18
COLLISION_BIND = 0x8008AC80
COLLISION_BIND_TEXT = (COLLISION_BIND,0x190,"88d16df3bda187ed3cb011269a356a7d0646ca1c946dcd194664a5abf4a317d0")
TYPE30_ROWS = 0x8035C288
COLLISION_BYTES = 7840
ROW_TABLE_SHA = "37b867aee89ecdf45af047eb1e377d507280f89c382438594e710c23e32ed839"
TEXT = (
    dragon.TEXT[-1],
    (0x800BEC20,0x98,"e07481c3758c1adb4177f2ac47672d6d6f9f0da49a44e673dd42587e29dd5707"),
    (0x800BECB8,0x164,"780addb11b30cc4777f20c51f94b8cd26a486d6fd6cf1921be177c0af9e408df"),
    (0x800BEE1C,0x328,"db8878b861a3081d1aee82b6645555dd269fdb16ecafe857a4d60e6db83ba11c"),
    (0x800BF144,0xCC,"6a7c3180193ead1f889bc8509267f1ab740bd2116f91ab03613be8c3bd36a4e7"),
    (0x80183730,0xF4,"f6ff5f01585d05ebffcab2c682a51e04dbbc99ebc20e14ec2040b80c946978b1"),
    COLLISION_BIND_TEXT,
    (0x80183A64,0x74,"7e6d852b31a0a76226eddceeae2ce6ebe08ab1b38c23e2678697d0600803ae3b"),
    (0x80183C4C,0x5C,"992c413a92f6f3772f900f838072f420c8d64e9519ee6ece8c8455bf3f3245cd"),
)
TRAIL_KEYS = ((0,0,0,0),(5,1,0.20000000298023224,1),(15,0,1,0),(9999,0,1,0))
PHASES = (0x800BECB8,0x800BEE1C,0x800BF210,0x800BF24C)
TAILS = {
    "retained-contact-latch":{"guard_read":"800BEFF0","area_request":"800BF00C",
                             "secondary_calls":["800BF03C","800BF050"]},
    "terrain-wrapper-hit":{"guard_result":"800BF0B8","area_request":"800BF0D8",
                           "secondary_calls":["800BF108","800BF11C"]},
}
READS = {
    "010":["800BEDB0","800BEDBC"],"014":["800BEDD8","800BF064"],"018":["800BF06C"],
    "01C":["800BF058"],"02E":["800BEFFC","800BF0D0"],"02F":["800BF010","800BF0DC"],
}


def carrier_index(dol):
    for address,size,digest in TEXT:
        if sha256(dol.read(address,size,"text")).hexdigest()!=digest:
            raise ValueError(f"elemental carrier text signature: {address:08X}")
    if struct.unpack(">4I",dol.read(PHASE_TABLE,16,"data"))!=PHASES:
        raise ValueError("elemental carrier dispatch signature")
    raw=dol.read(ROW_TABLE,6*48,"data")
    if sha256(raw).hexdigest()!=ROW_TABLE_SHA:
        raise ValueError("elemental carrier row table signature")
    curve=dol.read(TRAIL_TABLE,4*16,"data")
    entries=tuple(struct.iter_unpack(">4f",curve))
    if entries!=TRAIL_KEYS:
        raise ValueError("elemental carrier trail table signature")
    if any(struct.unpack(">2h",dol.read(TYPE30_ROWS+24*mode,4,"data"))!=(7,8) for mode in (2,3)):
        raise ValueError("elemental type-30 collision group signature")
    rows={}
    for model,key in dragon.DRAGON_KEYS.items():
        row=raw[key*48:(key+1)*48]
        floats={}
        for at in (0x10,0x18,0x1C):
            bits=row[at:at+4]
            value,=struct.unpack(">f",bits)
            if not math.isfinite(value):
                raise ValueError("elemental carrier float domain")
            floats[f"{at:03X}"]={"bits":bits.hex(),"value":value}
        area,tag=struct.unpack_from(">bb",row,0x2E)
        if area<0 or tag not in (0,1,2):
            raise ValueError("elemental carrier impact operand domain")
        rows[f"gg4e08:{model:04x}"]={
            "family":"type-47","key":key,"row_address":f"{ROW_TABLE+key*48:08X}",
            "raw_row":row.hex(),"collision_group":struct.unpack_from(">b",row,3)[0],
            "float_operands":floats,"active_low_byte_at_014":struct.unpack_from(">b",row,0x17)[0],
            "tail":{"area_key":area,"selector_tag":tag,
                    "type_30_mode":{0:None,1:2,2:3}[tag]},
        }
    return {
        "kind":"static elemental carrier operands; not engine content",
        "variant":"normal","live_child_schedule_verified":False,
        "factory":f"{dragon.FACTORY:08X}","dispatch_table":f"{PHASE_TABLE:08X}",
        "init":{"lifetime_seed":600,"lifetime_store":"800BED38","collision_bind":"800BED44","child_cue":"800BEDFC",
                "collision_bind_helper":f"{COLLISION_BIND:08X}"},
        "active":{"entry":"800BEE1C","lifetime_decrement":"800BEE78","row_read_sites":READS,
                  "trail":{"capacity":5,"record_bytes":20,"object_local_offset":"14C",
                           "count_offset":"145","cursor_offset":"144","timer_offset":"148",
                           "curve_helper":"800BF144","curve_table":f"{TRAIL_TABLE:08X}",
                           "curve_rows":[{"time":v[0],"values":list(v[1:]),"raw":curve[i*16:(i+1)*16].hex()}
                                         for i,v in enumerate(entries)],
                           "onboard_records_not_projectile_allocations":True}},
        "tails":TAILS,"type_30_collision":{"mode_table":f"{TYPE30_ROWS:08X}","emitter_bind":"80183AB0",
                                        "descendant_bind":"80183C80","groups_by_mode":{2:[7,8],3:[7,8]}},
        "borgs":rows,
    }


def collision_index(bank, member, carrier_row):
    """Decode only the reviewed shape groups from this exact normal slot 2."""
    if len(bank)!=COLLISION_BYTES:
        raise ValueError("elemental collision bank extent")

    def vector(row,offset):
        vals=struct.unpack_from(">3f",row,offset)
        if any(not math.isfinite(x) or abs(x)>1e7 for x in vals):
            raise ValueError("elemental collision vector domain")
        return vals

    def shape(group,expected_shape):
        if not 0<=group<32:
            raise ValueError("elemental collision group domain")
        start=0xA20+group*4
        header=bank[start:start+4]
        if header[0]>=64 or any(n<128 for n in header[1:]):
            raise ValueError("elemental collision group requires one row")
        ordinal=header[0]
        start=0xAA0+80*ordinal
        raw=bank[start:start+80]
        code,binding,flags,parameter,first,last,extra=struct.unpack_from(">BBHHhhH",raw)
        if (code,binding,flags,parameter,first,last,extra)!=(expected_shape,0,1,group,0,-1,0):
            raise ValueError("elemental collision row shape/header")
        center=vector(raw,0x0C)
        radius,=struct.unpack_from(">f",raw,0x18 if code==0 else 0x24)
        if not math.isfinite(radius) or not 0<radius<=1e7:
            raise ValueError("elemental collision radius domain")
        out={"group":group,"group_header":header.hex(),"row":ordinal,"raw_row":raw.hex(),"flags":flags,
             "window":[first,last],"shape":"sphere" if code==0 else "capsule","center":center,"radius":radius}
        if code==2:
            half=vector(raw,0x18)
            out["half_axis"]=half
            out["endpoints"]=[tuple(c-h for c,h in zip(center,half,strict=True)),tuple(c+h for c,h in zip(center,half,strict=True))]
        return out

    receipt={"member":member,"pzz_slot":2,"bank_sha256":sha256(bank).hexdigest(),
             "carrier":shape(carrier_row["collision_group"],0),"live_publication_verified":False}
    if carrier_row["tail"]["type_30_mode"] in (2,3):
        receipt["type_30_emitter"]=shape(7,0)
        receipt["type_30_descendant"]=shape(8,2)
    return receipt
