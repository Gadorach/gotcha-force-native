"""Static GG4E08 projectile sound call operands, not playback bindings."""
from hashlib import sha256
import struct

from tools.analyze import retail_dragon_shot as dragon
from tools.analyze.retail_dragon_carrier import COLLISION_BIND_TEXT
from tools.analyze.retail_projectile_origins import TABLES


SOUND_REQUEST = 0x800F036C
BANK_TABLE = 0x802D0BEC
# Whole factory / dispatch / first phase, and the request's bank-ID split.
TEXT = (
    (0x800434F8,0xE4,"2d32e75dc60a78d5228593cd067d11b811d3cb68cce28ebd1dae09eb9718f628"),
    (0x800435DC,0xC0,"5a87ecc83acb4394fa265edb26cdf3fbc0caad7ef859c4c88c21ea08ca71c557"),
    (0x800EFB3C,0xC8,"166caf117dce26eea64c8dcca8c88f971b0926375bcf721e41062a840cf426ba"),
    (0x800F036C,0x54,"ef81b5c3d2e79825e2191621107327329cabc6651c9828613801b9f25230712f"),
    (0x800F04B4,0x78,"839c4c6e4469de152b62eef05230add8b4fcb5e3eab918ac7c61dca5632cca3e"),
    (0x80082824,0x148,"3d339c26437064d921fbd0e3652cd1918cd037fe2290fdace81a3b6f9f20bd01"),
    (0x80082AB8,0x50,"eb2e1fc4a01288aa7a7569a3ecd3406781b920742fb13fec7cfd9ec9b492c2dc"),
    (0x80082B08,0x4BC,"19d466113a0a849d3684a5e415a9f29b166b94b0573f598ef829ac44381bc464"),
    (0x800C3BE0,0xFC,"45c8eb1aded088fcec8e2fef373ffa6d22c8f91dddcad4814e96d8c15f2e80c8"),
    (0x800C3CDC,0x50,"8e10b3ca6fe1b434a8fd38f0d68cba22066ed681c1b1e3b7be71782025117be7"),
    (0x800C3D2C,0x35C,"e53cd894e0eb38d5ae2e60d624b3bb74a8372b3bb561d6ef5287bdecf69e9c6a"),
    (0x8006EE14,0x2B8,"440ee31c552d2bc28d40c9cb5ce817a0aa1759b1678dd95f2e874c27c789560d"),
    dragon.TEXT[-1], # elemental carrier constructor
    (0x800BEC20,0x98,"e07481c3758c1adb4177f2ac47672d6d6f9f0da49a44e673dd42587e29dd5707"),
    (0x800BECB8,0x164,"780addb11b30cc4777f20c51f94b8cd26a486d6fd6cf1921be177c0af9e408df"),
    (0x800F061C,0xD4,"2fe767cfa30b31735d2836466da0a707d7e32800d4d44324263ed5190518b754"),
    COLLISION_BIND_TEXT,
)
PHASE_ZERO = {0x802D9748:0x80082B08, 0x803051E4:0x800C3D2C, 0x80301C08:0x800BECB8}
ROUTES = {
    "type-0": {"factory":"80082824","consumer":"80082B08","scope":"child-phase-zero",
               "operand_offset":0x34,"encoding":"be-s16","calls":[
                   {"site":"80082F9C","owner":"credited-actor","guard":"child-byte-12-is-zero"},
                   {"site":"80082FAC","owner":"parent","guard":"child-byte-12-is-nonzero"}]},
    "type-2": {"factory":"800C3BE0","consumer":"800C3D2C","scope":"child-phase-zero",
               "operand_offset":0x40,"encoding":"be-s16","calls":[
                   {"site":"800C4064","owner":"projectile-child","guard":"row-byte-02-nonnegative-and-resource-allocation-succeeds"}]},
    "type-14": {"factory":"8006EE14","consumer":"8006EE14","scope":"factory-success",
                "literal":8,"calls":[
                     {"site":"8006F0B4","owner":"factory-caller","guard":"retained-actor-gate-and-child-allocation-succeed"}]},
    "type-47": {"factory":"800BE948","dispatcher":"800BEC20","consumer":"800BECB8","scope":"child-phase-zero",
                "literal":199,"calls":[
                    {"site":"800BEDFC","helper":"800F061C","owner":"projectile-child",
                     "guard":"child-collision-group-binding-succeeds"}]},
}


def normal_tone_banks(dol, files, members):
    """Resolve regular slots by the loader's member triples, not a name guess."""
    table = struct.unpack(">9h",dol.read(BANK_TABLE,18,"data"))
    indexed = {(row["tsb"]["member"],row["chd"]["member"],row["dpk"]["member"]): (name,row)
               for name,row in members.items()}
    if len(indexed)!=len(members):
        raise ValueError("ambiguous projectile sound bank member identity")
    banks = []
    for at in range(0,len(table),3):
        triple = table[at:at+3]
        if triple not in indexed or any(binding["members"]==triple for binding in banks):
            raise ValueError("projectile sound bank member identity")
        name,row = indexed[triple]
        blob = files[row["tsb"]["path"]]
        if (len(blob)!=16+128*16 or struct.unpack_from("<4sIII",blob)!=(b"TSBD",len(blob),256,128) or
                sha256(blob).hexdigest()!=row["tsb"]["sha256"]):
            raise ValueError("projectile sound tone table extent or identity")
        banks.append({"bank":name,"members":triple,"tone_bytes":blob[16:]})
    return banks


def tone_candidate(sound_id, banks):
    # Refuse signed negative IDs and slots outside the qualified normal loader.
    # Never mask them down to an apparently valid common-bank tone.
    if not 0<=sound_id<len(banks)*128:
        return {"status":"outside-qualified-normal-bank-domain"}
    slot,tone = divmod(sound_id,128)
    bank = banks[slot]
    record = bank["tone_bytes"][16*tone:16*(tone+1)]
    return {"status":"static-tone-record","bank_slot":slot,"bank":bank["bank"],"tone":tone,
            "tone_record_sha256":sha256(record).hexdigest()}


def projectile_sound_index(dol, origins, files, members):
    for at,size,digest in TEXT:
        if sha256(dol.read(at,size,"text")).hexdigest()!=digest:
            raise ValueError(f"projectile sound source signature: {at:08X}")
    for at,callback in PHASE_ZERO.items():
        if dol.read(at,4,"data")!=struct.pack(">I",callback):
            raise ValueError(f"projectile sound dispatch signature: {at:08X}")
    banks = normal_tone_banks(dol,files,members)
    tables = {}
    for family,route in ROUTES.items():
        base,stride,count,_,_ = TABLES[family]
        if len(origins[family])!=count:
            raise ValueError("projectile sound origin table extent")
        block = dol.read(base,stride*count,"data") if "operand_offset" in route else b""
        cues = []
        for key in range(count):
            row = key*stride
            offset = route.get("operand_offset")
            at = row+offset if offset is not None else None
            sound_id = struct.unpack_from(">h",block,at)[0] if at is not None else route["literal"]
            entry = {"key":key,"source_sound_id":sound_id}
            if at is not None:
                entry.update(operand_address=f"{base+at:08X}",operand_bits=block[at:at+2].hex())
            if family=="type-2":
                gate = struct.unpack_from(">b",block,row+2)[0]
                entry["row_byte_02"] = gate
                if gate<0:
                    cues.append({**entry,"status":"phase-zero-skips-request"})
                    continue
            cues.append({**entry,**tone_candidate(sound_id,banks)})
        tables[family] = cues
    return {"kind":"static projectile sound operands; not engine content",
            "sound_request":f"{SOUND_REQUEST:08X}","normal_bank_table":f"{BANK_TABLE:08X}",
            "normal_banks":[{"slot":i,"bank":b["bank"],"members":b["members"]} for i,b in enumerate(banks)],
            "routes":ROUTES,"tables":tables,"live_playback_verified":False}


def attach_release_sfx(releases, tables):
    """Join only existing exact factory keys, without rewriting action routes."""
    for release in releases:
        rows = tables.get(release["family"])
        if rows is None:
            continue
        key = release["key"]
        if not 0<=key<len(rows) or rows[key]["key"]!=key:
            raise ValueError("projectile sound release key identity")
        fields = ("status","source_sound_id","bank_slot","bank","tone")
        release["sfx_candidate"] = {k:rows[key][k] for k in fields if k in rows[key]}
