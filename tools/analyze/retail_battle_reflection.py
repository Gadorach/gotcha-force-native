"""Exact normal bodies with authored zero normals on supported reflection meshes."""
from hashlib import sha256


NORMAL_BODIES={
    0x0001:("b8827cc8ea92b9d6b537dedb0b995b02c373fb131c4bfac21b1cb774f82d7b8f",((0x127E8,28),)),
    0x0101:("c9f842af4f5252082a39dfcd5c040ddd1e59350e07e2dba0d0c338fa4a9ebd01",((0xC8D0,18),)),
    0x0208:("8075fd6054fc49ff1bbe5cfd119b9d64f7432aae63eda99cc8624fd67359b685",((0x11428,20),)),
    0x020E:("9973a01c8d91df7c224d1e1079b32e8ed002c3c6728240ab9e1c962ee7cc95bd",((0x11488,20),(0x129E8,44))),
    0x050B:("459b4c2f4e6745ffd722e5763e71faec9d5a9992a3dc96fb3b56bbc36b5d47aa",((0x15C98,7),)),
    0x050D:("d25a8d0883feabfe8302dd915c512735f84cfc920518ec1aea31dc8dec6cf605",((0x15CD8,7),)),
    0x0517:("43cbe82e09022d8e5eb54df8454e866dbb30c900eab718712c918e3f5e87117e",((0x15CD8,7),)),
    0x0801:("0a70a9e2e465a9ea2c914e8699179a6cc23a93a97753368ac88dacef8c43094b",((0xEA60,14),)),
}


def zero_reflection_rules(model,body):
    if model not in NORMAL_BODIES:
        return None
    digest,rows=NORMAL_BODIES[model]
    if sha256(body).hexdigest()!=digest:
        raise ValueError("zero-reflection rules need the exact normal wrapper body")
    return ("".join(f"zero-reflection={pobj:x}|{count}\n" for pobj,count in rows),
            {"meshes":[{"pobj_offset":pobj,"authored_zero_normals":count} for pobj,count in rows],
             "policy":"native neutral reflection UV only; no source hardware equivalence"})
