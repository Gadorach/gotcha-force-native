"""Bounded source movement operands and exact shared read sites, not physics."""
from hashlib import sha256
import math
import struct


ACTOR_BYTES = 432
FIELDS = (
    (0x04,"h"),(0x06,"h"),(0x08,"h"),(0x0A,"h"),(0x0C,"h"),(0x0E,"h"),(0x10,"h"),
    *((at,"f") for at in range(0x18,0x39,4)),
    *((at,"B") for at in range(0x3C,0x42)),
    (0x42,"h"),
    *((at,"f") for at in range(0x44,0x71,4)),
    (0x9F,"B"),
    *((at,"f") for at in range(0xF0,0x11D,4)),
)

# (read site, exact instruction, actor-data offset(s), observed operand use).
# The names state source reads, not engine parameters or a unit conversion.
ROLE_READS = {
    "8005F3A0": (
        (0x8005F3FC,0xC0230018,(0x18,0x1C,0x20,0x24,0x28),"indexed turn-entry planar operand"),
    ),
    "8005F578": (
        (0x8005F5A8,0xC023002C,(0x2C,),"travel-entry planar operand"),
    ),
    "80063A90": (
        (0x80063AE4,0xC023002C,(0x2C,),"alternate type-6 travel planar operand"),
    ),
    "8005FAF0": (
        (0x8005FB74,0xC0030050,(0x50,),"ordinary ascent planar operand"),
        (0x8005FC4C,0xC0030068,(0x68,),"ordinary ascent vertical base"),
        (0x8005FC60,0xC0030048,(0x48,),"ordinary ascent launch formula term"),
    ),
    "8005FECC": (
        (0x8005FF2C,0xC004006C,(0x6C,),"fall-entry vertical base"),
    ),
    "80060C48": (
        # One of three separately indexed planar/timer rows.
        *((pc+i*8,0xC01E0000|(at+i*4),(at+i*4,),use)
          for pc,at in ((0x80060DF0,0xF0),(0x80060E38,0x100),(0x80060E14,0x110))
          for i,use in enumerate(("dash entry speed","dash planar change","dash minimum speed","dash clock preset"))),
    ),
    "80061338": (
        (0x800613DC,0xC0030070,(0x70,),"air-dash wind-up vertical base"),
    ),
    "80061560": (
        (0x800615CC,0xC0030058,(0x58,),"air-dash entry planar speed"),
        (0x800615D8,0xC003005C,(0x5C,),"air-dash planar change"),
        (0x800615E4,0xC0030060,(0x60,),"air-dash entry vertical speed"),
        (0x800615F0,0xC0030070,(0x70,),"air-dash active vertical base"),
        (0x800615FC,0xC0030064,(0x64,),"air-dash active clock preset"),
    ),
    "800633C8": (
        (0x80063420,0xC0030068,(0x68,),"cruise entry vertical base"),
        (0x800634C4,0xC0030044,(0x44,),"cruise pitch-scale term"),
        (0x80063534,0xC023002C,(0x2C,),"cruise planar target"),
    ),
    "80063644": (
        (0x800636D0,0xC0230058,(0x58,),"boost entry planar speed"),
        (0x800636DC,0xC023005C,(0x5C,),"boost planar change"),
        (0x800636E8,0xC0230070,(0x70,),"boost entry vertical base"),
        (0x800636F4,0xC0230064,(0x64,),"boost clock preset"),
    ),
}
METER_READS = (
    (0x800561A8,0xA8840004,(0x04,),"meter initialization limit"),
    (0x80055AD4,0xA8030008,(0x08,),"type-3 nonboosting refill amount"),
    (0x80055AE8,0xA8630004,(0x04,),"type-3 refill cap"),
    (0x8006650C,0xA8040006,(0x06,),"debit reloads refill delay"),
)


def operand_records(actor):
    """Record ordinals are not form, look or runtime binding IDs."""
    if not actor or len(actor)%ACTOR_BYTES or len(actor)>8*ACTOR_BYTES:
        raise ValueError("movement actor record extent")
    rows=[]
    for base in range(0,len(actor),ACTOR_BYTES):
        raw=actor[base:base+ACTOR_BYTES]
        operands={}
        for offset,fmt in FIELDS:
            value,=struct.unpack_from(">"+fmt,raw,offset)
            if fmt=="f" and (not math.isfinite(value) or abs(value)>1e5):
                raise ValueError("movement operand float domain")
            size=struct.calcsize(">"+fmt)
            operands[f"{offset:03X}"]={"bits":raw[offset:offset+size].hex(),"value":value}
        rows.append({"record":base//ACTOR_BYTES,"record_sha256":sha256(raw).hexdigest(),"operands":operands})
    return rows


def verified_reads(dol, entries):
    rows=[]
    for site,word,offsets,use in entries:
        if dol.read(site,4,"text")!=struct.pack(">I",word):
            raise ValueError(f"movement operand read signature: {site:08X}")
        rows.append({"site":f"{site:08X}","offsets":[f"{at:03X}" for at in offsets],"use":use})
    return rows


def movement_operand_schema(dol):
    return {"kind":"static source operand review; not native tuning",
            "record_bytes":ACTOR_BYTES,
            "fields":{f"{at:03X}":{"offset":at,"encoding":{"B":"u8","h":"be-s16","f":"be-f32"}[fmt]}
                      for at,fmt in FIELDS},
            "lower_handler_reads":{handler:verified_reads(dol,reads) for handler,reads in ROLE_READS.items()},
            "meter_read_sites":verified_reads(dol,METER_READS)}


def movement_operand_profile(actor, roles):
    profiles=[]
    for profile in roles["profiles"] if roles else ():
        links=[{"role":r["role_hint"],"selector":r["selector"],"handler":r["handler_addresses"][1]}
               for r in profile["roles"] if r["handler_addresses"][1] in ROLE_READS]
        if links:
            cruise={("cruise",27,"800633C8"),("cruise-boost",28,"80063644")}
            profiles.append({"configuration":profile["config"],"selector_table":profile["selector_table"],
                             "movement_type":profile["movement_type"],"read_links":links,
                             "type_3_meter_read_candidate":profile["movement_type"]==3 and cruise<={
                                 (r["role"],r["selector"],r["handler"]) for r in links}})
    return {"records":operand_records(actor),"profile_candidates":profiles,"runtime_record_binding_verified":False}
