"""Exact offline archive/record bindings for the elemental carrier visuals."""
from hashlib import sha256
import struct
from tools.analyze.retail_dragon_area import RECORD as IMPACT_RECORD


EFFECT_MEMBER = 697
EFFECT_NAME = b"efct.pzz"
EFFECT_SHA = "89a0de2ccf3652b5c1b5c12f3bd9a4c923908904c6da01a22e82eef7d1b565bf"
EFFECT_BLOCKS = (
    (32832,"6c2a477c2bda2e2ecea9625a243dc9e19f4f17b68a752093fabb46cfc644bfeb","effects/common-atlas.tpl"),
    (417956,"18761bf6cf2d39b9ddf0a9f08f5ba7926f220911e5a30c5485277ad7a0000ff0","effects/common-single.hsd"),
    (408296,"da036fe0f374b8f1c1ca4b73b7c63c7e8295f0a3c1840e7a33d6b4d1580772c9","effects/common-split.hsd"),
)
TEXT = (
    (0x800BECB8,0x164,"780addb11b30cc4777f20c51f94b8cd26a486d6fd6cf1921be177c0af9e408df"),
    (0x80006F78,0x20,"4fcc7bc89a7483631e0852a9fd189017180b431d5a0b09297898f50a05e5d9a5"),
    (0x80006FB4,0x7C,"e307fd696e8a8bd309200cf2acd71ced60a4bee61433b45373034f58fc96096e"),
    (0x800412C0,0xA4,"d837771fce4397bf8c5877c051977763c07ab56e4e465b69a612d5d3be48afca"),
    (0x80041678,0x58,"97d3e895dff642dc6394f14b722393d8a6089458044e9a59cf586c7933c437f4"),
    (0x80006C88,0xC4,"afac18ada3931593eb2f6090d92d6574481630aa9b34345f8b72de0623dfd030"),
    (0x80042878,0xDC,"f470515dbb6dd7350deb2b637a0ed732e12068ea1fa12274d4455452973b410c"),
    (0x80042954,0x40,"3d24a406a9cc9a88374918cec6fdd1cb5d73922fd8b34c00b3636b555dbd7dd5"),
    (0x80183CA8,0x144,"235371b256f77b51d08fc677d32be6e44d5e2efe33d18d7739532c10ee39ad46"),
    (0x8000726C,0xBC,"d691ab9dc4bbc32c681a70d45918f2077ac11296d36c8e2f9e949453d87e4581"),
    (0x800BF26C,0x1A0,"384fa12c135c1d97ea33e9dc4c13ecff4fbb8db16ae031dae581ba79d6c180de"),
)


def art_source_matches(dol):
    for address,size,digest in TEXT:
        if sha256(dol.read(address,size,"text")).hexdigest()!=digest:
            raise ValueError(f"elemental visual source signature: {address:08X}")


def record_receipts(archive, keys):
    """Validate the scene table and preserve only requested descriptor identities."""
    if not 32<=len(archive)<=16<<20:
        raise ValueError("elemental visual HSD extent")
    size,extent,count,public,external=struct.unpack_from(">5I",archive)
    reloc_start=32+extent
    symbols=reloc_start+4*count
    names=symbols+8*(public+external)
    if (not 32<=size<=len(archive)<=size+31 or any(archive[size:]) or not 0<public<=4096 or
            external>4096 or names>=size or count>extent//4):
        raise ValueError("elemental visual HSD tables")
    relocs={n for (n,) in struct.iter_unpack(">I",archive[reloc_start:symbols])}
    if len(relocs)!=count:
        raise ValueError("elemental visual duplicate relocations")
    data=memoryview(archive)[32:reloc_start]

    def ptr(at,span,required=False):
        if not 0<=at<=len(data)-4 or at%4:
            raise ValueError("elemental visual pointer extent")
        value,=struct.unpack_from(">I",data,at)
        if not required and not value and at not in relocs:
            return None
        if at not in relocs or value%4 or value>len(data)-span:
            raise ValueError("elemental visual relocation")
        return value

    scenes=[]
    for i in range(public):
        offset,name=struct.unpack_from(">2I",archive,symbols+8*i)
        start=names+name
        end=archive.find(b"\0",start,min(size,start+257))
        if end<0:
            raise ValueError("elemental visual public name extent")
        if archive[start:end]==b"scene_data":
            scenes.append(offset)
    if len(scenes)!=1:
        raise ValueError("elemental visual scene_data identity")
    table=ptr(scenes[0],4,True)
    requested=set(keys)
    if not requested or any(not 0<=k<4096 for k in requested):
        raise ValueError("elemental visual requested record domain")
    records={}
    for index in range(4096):
        desc=ptr(table+index*4,16)
        if desc is None:
            if records.keys()!=requested:
                raise ValueError("elemental visual missing requested record")
            return {"record_count":index,"archive_sha256":sha256(archive).hexdigest(),"records":records}
        if index in requested:
            root=ptr(desc,64,True)
            joint,material,shape=(ptr(desc+i,4) for i in (4,8,12))
            empty=(joint is None and material is None and shape is None and struct.unpack_from(">I",data,root+4)==(0,) and
                   all(ptr(root+i,4) is None for i in (0,8,12,16,56,60)))
            records[index]={"descriptor_offset":desc,"root_offset":root,"joint_table_offset":joint,
                            "material_table_offset":material,"shape_table_offset":shape,"empty_static":empty}
    raise ValueError("elemental visual unterminated record table")


def caster_art(model_archive, carrier):
    raw=bytes.fromhex(carrier["raw_row"])
    primary,secondary=raw[:2]
    return {**record_receipts(model_archive,(primary,secondary)),"owner":"caster-selected-wrapper",
            "exact_normal_candidate":"visual/body.hsd","repeated_record":{"key":primary,"count":5,"request":"800BED7C"},
            "separate_record":{"key":secondary,"request":"800BED9C"},
            "source_draw":{"callback":"800BF26C","trail_count_offset":"145","trail_cursor_offset":"144",
                           "trail_matrix_write":"800BF348","trail_submit":"800BF354",
                           "carrier_matrix_write":"800BF3D8","carrier_submit":"800BF3E4","submit_flags":68},
            "live_draw_verified":False}


def common_art(blocks):
    files={}
    rows=[]
    for slot,(size,digest,path) in enumerate(EFFECT_BLOCKS):
        data=blocks[slot]
        if len(data)!=size or sha256(data).hexdigest()!=digest:
            raise ValueError(f"common effect block signature: {slot}")
        files[path]=data
        rows.append({"pzz_slot":slot,"path":path,"bytes":size,"sha256":digest,
                     **(record_receipts(data,(IMPACT_RECORD,83)) if slot else {})})
    return files,{"member":EFFECT_MEMBER,"name":EFFECT_NAME.decode("ascii"),"member_sha256":EFFECT_SHA,
                  "owner":"common-effect-archive","source_mode_slots":{"single":1,"split":2},"texture_slot":0,
                  "descendant_record":{"key":83,"request":"80183DE0","type_30_modes":[2,3]},
                  "impact_area_record":{"key":IMPACT_RECORD,"request":"800861E4","family":"type-5"},
                  "live_draw_verified":False,"blocks":rows}
