"""Normal battle equipment records; native cosmetics, not source child actors."""
from hashlib import sha256
import struct

from tools.analyze.retail_dragon_art import record_receipts


MODELS={0x0007,0x0208,0x020E}
TEXT=(
    (0x8000332C,0x10,"e9dbf99242c63ba10ba9265e0633f0d8f791d287774eef376a20d7ced0a70ce2"),
    (0x800A7048,0x50,"1b9b4977340cd7f1d4f3df12e580a14dacfb4229efae8fb2597b2886c3354923"),
    (0x800AE630,0x104,"e95b2d58163f2d3a6b50f0dfb535018276c0ae285aa7405958a9c88b71f724a4"),
    (0x800AE788,0x1D4,"b8b4f37c92a67b038290f75d32f1535fc8c7478265c3f71575040143f4ad65d6"),
    (0x800AECE0,0xB8,"997add6814cf348465effc9e3ebec9615f165d9c08e6471af237977e51d6ed3e"),
    (0x800BB4BC,0x68,"880bce8cc4a8eb16f5c9fe169d9ff1be5487bf2651467fb9be06f960a7ffae43"),
    (0x800BE58C,0x110,"f5fe3ad10f9a48b86d7633ad5ba8db2a3567fd263cc2aac884475ed472b1a3d5"),
    (0x800BE6F0,0x170,"62753c8fa096c1d9c8e34e54730426aec887144b68faaee11e630efac15ce63a"),
    (0x800BE8EC,0x5C,"cce39f0de07b3a921dbe764dc33138bad7685122d462ed57b99fa92c89d5e9c7"),
    (0x800F1B28,0x110,"100ffea562bf571179e3bf3b15e3e1ca1a17e31a2d458fdb8ff6e5fd21b4974b"),
    (0x800F1C8C,0x108,"9e41fd0aa0317de0ec6ad23e780b39856f3b2ac5ce1ee88fc69a3b830b8ef10f"),
    (0x800F1D88,0xC,"ee80053a3f0ece331025ccb74950ebf4669ea451437f50033a05a6377759ec6c"),
)
DATA=(
    (0x80433A80,4,"35be6c5ea73a15df3a8b24eaf7fa60c7e43629edd99679a9c8cc16e2b2932a7e"),
    (0x80434440,4,"f51547a1a20e20fdd2a1d0eaee13cd252edc1c047f757bcc38020fc45477e20e"),
    (0x802FA958,24,"37bae302cec754884f74c7f8e71ec1e62b68d44f861e48553a109a03db39b84f"),
    (0x802FA9CC,44,"e6e8b621d3e7a1cd6bedc84da1490f4da463cca3b29c90a84684bf20696cb63c"),
    (0x80301AA8,48,"ed9b211d97245fd92db412fe6e6f0cf5e5c810e10f78c58fdaa540ca79d96d0d"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"equipment {kind} signature: {at:08X}")


def equipment(dol, model, archive, joints, parts):
    if model==0x0007:
        selectors=dol.read(0x80433A80,2,"data")
        mounts=struct.unpack(">6f",dol.read(0x802FA958,24,"data"))
        if selectors!=bytes((10,6)) or [(p["joint"],p["record"]) for p in parts["default_candidates"]]!=[(7,1)]:
            raise ValueError("Cyber exact hand or arm origins")
        rows=[{"slot":f"arm-effect-{i}","joint":joint,"record":8,"point":list(mounts[i*3:i*3+3]),
               "angles":[0,0,0],"spin":"y|32|forward" if i==0 else "y|32|reverse"}
              for i,joint in enumerate(selectors)]
        kind="Cyber normal state-0 record 8 on both arms; authored continuous spin, not source visibility or stock gating"
    elif model in (0x0208,0x020E):
        key=int(model==0x020E)
        record,joint,group,rx,ry,rz,x,y,z=struct.unpack(">6h3f",dol.read(0x80301AA8+24*key,24,"data"))
        if (record,joint,group,rx,ry,rz,x,y,z)!=(10,11,9,0,0,-16384,30.,-5.,0.):
            raise ValueError("lance mount row")
        glow=dol.read(0x80434440,2,"data")[key]
        if glow!=13:
            raise ValueError("lance second child record")
        rows=[{"slot":"lance","joint":joint,"record":record,"point":[x,y,z],"angles":[0,0,-90]},
              {"slot":"lance-core","joint":7,"record":glow,"point":[0,0,0],"angles":[0,0,0]}]
        kind="normal lance and separate record 13 visual candidates; retained as evidence, not bound to gameplay"
    else:
        raise ValueError("not an equipment pilot")
    selected=record_receipts(archive,{p["record"] for p in rows})["records"]
    if any(p["joint"]>=len(joints) or selected[p["record"]]["empty_static"] for p in rows):
        raise ValueError("equipment cannot bind its exact joint or record")
    return {"scope":kind,"parts":rows,"records":selected,"source_child_runtime_verified":False,
            "native_mount_admitted":model==0x0007}


def equipment_rules(evidence):
    if not evidence["native_mount_admitted"]:
        raise ValueError("equipment record evidence is not a qualified native mount")
    lines=["part-record=right-hand|7|1"]
    for p in evidence["parts"]:
        slot=p["slot"];light="|".join(f"{v:.9g}" for v in (*p["point"],*p["angles"]))
        lines.extend((f"part-record={slot}|{p['joint']}|{p['record']}",f"part-pose={slot}|{light}"))
        if p.get("spin"):
            lines.append(f"part-mount-spin={slot}|{p['spin']}")
    lines.extend(f"part-hierarchy={k}" for k in sorted(evidence["records"]))
    return ("\n".join(lines)+"\n").encode("ascii")
