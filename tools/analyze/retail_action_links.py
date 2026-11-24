"""Join static action rows to literal request and origin candidates, without replay."""
from functools import lru_cache
from hashlib import sha256
import struct

from tools.analyze.gred_animation_controller import decode_program, signed_table
from tools.analyze.lane_descriptors import descriptor_lanes
from tools.analyze import retail_dragon_shot as dragon
from tools.analyze.retail_action_sfx import RELAY, RELAY_PARENTS, RELAY_SPAN, review_requests
from tools.analyze.retail_movement_tables import signed16, written_register
from tools.analyze.retail_seeded_shot_pose import (
    ANIMATED_CRUISE_SHOTS, CLOCKED_GROUND_DIRECT_ENTRIES, CLOCKED_GROUND_SHOTS, CYBER_MACHINE_SHOTS, EAGLE_FAMILIES, ORDERED_SHOTS,
    STATIC_CRUISE_SHOTS, beam_b_shot_pose, heavy_b_shot_pose,
    rifle_shot_pose, theta_shot_pose, throw_shot_pose,
)
from tools.analyze.retail_side_shot import neutral_side_shot
from tools.analyze.retail_release_phases import (
    ADVANCE_HELPERS, HELPER_WINDOWS, RELEASE_GUARDS, SEPARATE_CONSUMERS, STATIC_GROUND_PHASE_PAIRS,
    VIOLET_SHOT, WINDOWS as RELEASE_PHASE_WINDOWS,
)


PROGRAM_REQUEST = 0x8004BEB8
CONDITIONAL_ENTRY_TABLE = "804346F8"
CONDITIONAL_ENTRY_SITE = "8015AA0C"
# Reviewed phase-one entries advance +540 from 1 to 2 before requesting
# the shot pose. The enclosing phase window pins this branch and its stores.
PHASE_ONE_SHOT_POSES = {
    "80340E48":(0x8014D934,0x8014D9A8),
    "80348B98":(0x8015DD44,0x8015DDB8),
    "803816D0":(0x801B433C,0x801B43B0),
}
FACTORIES = {0x80082824: "type-0", 0x800C3BE0: "type-2", 0x8006EE14: "type-14",
             0x8007DB5C: "type-15"}

# Previously bounded source witnesses cover these first three phases only.
# Keep the recognized dispatch table and the exact leaf identity paired.
PHASE_WINDOWS = {
    0x80325ACC: (0x8010CEB0, 0x8010CFBC, 0x8010D0D0),
    0x80325A14: (0x8010A5D0, 0x8010A6B0, 0x8010A78C),
    0x8033ED0C: (0x80148874, 0x80148968, 0x80148AB0),
    0x802D486C: (0x8007454C, 0x80074634, 0x800746EC),
}
# The third route is the integer interval (>0302, <0304), not equality
# with the final comparison's exclusive upper bound.
MODEL_GATED_COMPARE = {0x8010D070:(0x8010D050,0), 0x8010D080:(0x8010D044,0),
                       0x8010D0AC:(0x8010D05C,-1)}
MODEL_GATE_SHA = "6c9e2ad9f1bb5c0a2d192cecde3ebad3eedf24ea77bd87dee111f71d3aaa5f25"

# One-hop same-actor release relays. The helper signature validates the
# release block and model comparisons; the caller and call site must also
# match. No other calls are followed and no models inherit a sibling's branch.
RELEASE_RELAYS = {
    0x80070558: (0xAC, "69d0ddf36e6edf532b931695a3cf68c1d9275292a7fb7449872dd787e8b88c84",
                 ((0x8006FC24,0x8006FCA0),(0x8006FE54,0x8006FEE4)),
                 {0x800705AC:(0x000A,),0x800705C4:(0x0003,),0x800705DC:(0x0003,)}),
    0x800C9F40: (0xF4, "a816e2a9b2a081d3340428d28c39fb94c70f323ffba9fb6106d77bdb9cad1a3d",
                 ((0x800C96C0,0x800C9758),(0x800C9A70,0x800C9B30)),
                 {0x800C9F98:(0x0600,),0x800C9FB0:(0x0608,),0x800C9FBC:(0x0608,),
                  0x800C9FD4:(0x0616,),0x800C9FEC:(0x061C,),0x800C9FF8:(0x061C,)}),
    0x8018DCB0: (0xEC, "15769a17c39adc93b92856262d7d59893b474cb04d546226e01852f38328cac0",
                 ((0x8018D438,0x8018D4D0),(0x8018D7E4,0x8018D8A4)),
                 {0x8018DD34:(0x0615,),0x8018DD4C:(0x0629,),0x8018DD64:(0x062A,)}),
    0x800D0C50: (0x60, "b6ccf33c0474957a93e1937b596035af25d3007a7b71a5ebe8ef845c36b6c8cc",
                 ((0x800D0514,0x800D0590),(0x800D0744,0x800D07F8)),{}),
    0x80103428: (0x184,"92aeb411860fca6cb6511e90708bf47453458c466b682f9a504dd94fa1e9ebe1",
                 ((0x80102BF8,0x80102C94),(0x80102F50,0x80103008)),{}),
    0x800A75D0: (0x5C,"678fc7e35c2781dc8e489522bcac0b4ae6b22a4270bedf8573f45babb5c1056d",
                 ((0x800A78B8,0x800A794C),(0x800A7630,0x800A7794)),{}),
    0x800CD1A0: (0x24C,"f803c2b3f35361e5a8248ca1948977e44da1bd62bd8bec95ebd3685a559651d4",
                 ((0x800CC99C,0x800CCA4C),(0x800CCCFC,0x800CCDB0),(0x800CD0A0,0x800CD150)),
                 {site:(model,) for model,start,count in ((0x0602,0x800CD2D8,3),(0x060A,0x800CD308,6),
                                                         (0x060C,0x800CD35C,5),(0x060E,0x800CD3A4,4))
                  for site in range(start,start+12*count,12)}),
    0x8012A308: (0xC4,"654d907fca3257ef61ed89bed817d5f7e10577504a3a882daab8d63738d29125",
                 ((0x80129AC4,0x80129B64),(0x80129E58,0x80129F24),(0x8012A220,0x8012A2C4)),
                 {0x8012A384:(0x0606,),0x8012A390:(0x0606,),0x8012A3A8:(0x061A,),0x8012A3B4:(0x061A,)}),
    0x801313BC: (0x1D4,"2113e67b438b03dca6261948ff2792f323f849642487aebd8a323f99dc285b1a",
                 ((0x801309B8,0x80130A6C),(0x80131184,0x80131244)),{0x80131508:(0x0103,)}),
    0x8013D600: (0x1B8,"55171cbe2412bb42e3de6dee8a7b7f909130f598852bdf06e63820ea56d087c0",
                 ((0x8013CD30,0x8013CDCC),(0x8013D4B4,0x8013D568)),{0x8013D77C:(0x0802,),0x8013D78C:(0x0803,)}),
    0x8019F850: (0x9C,"782ef08d8e999a2ce672f702fa557275a0f9b7e2c051a7cfdb932d506f7f4174",
                 ((0x8019EF14,0x8019EF90),(0x8019F144,0x8019F1D4),(0x8019F3E4,0x8019F45C),(0x8019F68C,0x8019F6EC)),{}),
    0x801A1AF0: (0xC4,"3042cb9212bc9238d7ad58418b03fc10df280e1ef5270696e9d148fbecea7d3f",
                  ((0x801A1A1C,0x801A1ABC),),{0x801A1B48:(0x0906,)}),
    RELAY: (*RELAY_SPAN,tuple((entry,proof[2]) for entry,proof in RELAY_PARENTS.items()),
            {0x8010FF24:(0x080C,),0x8010FF30:(0x080C,)}),
    0x8018BF2C: (0x168,"bfbe7e39357cfa29fb336e0d59a13b41685e6b0a0fb3fe35a2de892aaae0b5fe",
                 ((0x8018BF00,0x8018BF18),),
                 {0x8018C008:(0x0614,),0x8018C014:(0x0614,),0x8018C02C:(0x0628,),0x8018C038:(0x0628,)}),
    0x8018C164: (0x14C,"e90fba51cb6c3fa892ef4f9a973c6e18785dbcfb55eb9ac3129df0cd09ba349d",
                 ((0x8018C138,0x8018C150),),
                 {0x8018C208:(0x0614,),0x8018C214:(0x0614,),0x8018C248:(0x0628,),0x8018C254:(0x0628,)}),
}
RELAY_GROUPS = {0x8018C164:("X","B-charge")}
RELAY_PARENT_SIGNATURES = {
    **{(RELAY,entry):proof[:2] for entry,proof in RELAY_PARENTS.items()},
    (0x8018BF2C,0x8018BF00):(0x2C,"16eec2198cbc3f953071822a1035bcb0d1b28005bca89a3a52416017c3bfc1a0"),
    (0x8018C164,0x8018C138):(0x2C,"16eec2198cbc3f953071822a1035bcb0d1b28005bca89a3a52416017c3bfc1a0"),
}
RELEASE_EXCLUSIONS = {0x800705EC:(0x0003,0x000A)}
# Model-gated factories in direct phase leaves (no relay). A signature
# mismatch suppresses the reviewed sites rather than becoming ungated.
DIRECT_RELEASE_GATES = {
    0x8012B188: (0x2D0,"4818c1005f4ea63cab4eca28a036a068ce0fd38e2ede729f702e8f1372842cbe",
                 {0x8012B3EC:0x0607,0x8012B3F8:0x0607,0x8012B410:0x061B,0x8012B41C:0x061B}),
    0x8015C0C0: (0x370,"6fe9b0005c7582fafe64db31dd42aeb451d1372b6d9aef015c917272e4703767",
                 {0x8015C32C:0x0612,0x8015C338:0x0612,0x8015C350:0x0620,0x8015C35C:0x0620,
                  0x8015C374:0x0624,0x8015C380:0x0624,0x8015C398:0x0626,0x8015C3A4:0x0626}),
    **{entry:(0x1F4,digest,{site:model for model,owner in CLOCKED_GROUND_DIRECT_ENTRIES.items()
                            if owner==entry for site in CLOCKED_GROUND_SHOTS[model][1]})
       for entry,digest in (
           (0x80125DE8,"fcf7063723d9f3a6f825b74ae98cdb6d85e9f0e3553e318536fe3c8aeb53e295"),
           (0x8018C7D8,"5dc04695536286c8eb0724a97997b463b23f7808199dbd9a9eed318e8b7bc4c5"),
       )},
    0x801E7AFC: (0xE4,"4578b38e85a56d9806a0ef9d2a54909db1fbd761dd226f28bf83304a792eb3a3",
                 {site:model for model,(_,sites) in STATIC_CRUISE_SHOTS.items() for site in sites}),
    0x80079E88: (0x12C,"8d0a161dcba6d9b40a1cd01e0ea2dcbb5de146e6ddb731487b0d4980ff0772b0",
                 {0x80079F78:0x0900,0x80079F88:0x0908,0x80079F98:0x090D}),
    0x8007A364: (0x138,"4f3f011abadb6b47fcb6571254dacaca2c0f67d7a7922ff4378242714ef3745f",
                 {0x8007A460:0x0900,0x8007A470:0x0908,0x8007A480:0x090D}),
    0x80152104: (0x104,"597321a6d0ceed53af4c090670e58d8bcb1fd5fa021ceb30e72baf818abb1d07",
                 {0x801521DC:0x0903,0x801521EC:0x090A}),
    0x80152590: (0x10C,"37034858e8eb974f2e6a5a36d24f1df0b77473d0820c3433f44dcac9b3bd5ea4",
                 {0x80152670:0x0903,0x80152680:0x090A}),
    0x8015DF6C: (0x110,"51ad939c73ae7115db6579e09aff9e7ecffa79272be1de95a2036bf8fd29c72a",
                 {0x8015E050:0x0902,0x8015E060:0x0909}),
    0x801B4564: (0x110,"87e13d931e35792aa3a89cb93d76690ac25e0ec3569950592f5e6fd021494405",
                 {0x801B4648:0x0907,0x801B4658:0x090C}),
}
DIRECT_RELEASE_SITES = {site:(entry,size,digest,model)
                        for entry,(size,digest,gates) in DIRECT_RELEASE_GATES.items()
                        for site,model in gates.items()}


def entry_phase_increment(words, incoming, start):
    """Exact actor+540 byte increment on a straight pre-call entry prefix."""
    for pc in range(start,start+0x40,4):
        w=words.get(pc,0)
        op=w>>26
        if not w or (pc!=start and incoming.get(pc)!={pc-4}) or op in (16,18,19):
            break
        if op==34 and w>>16&31==3 and w&0xFFFF==0x540:
            plus,store=words.get(pc+4,0),words.get(pc+8,0)
            src,dst=w>>21&31,plus>>21&31
            if (src!=3 and dst!=3 and plus==(14<<26|dst<<21|src<<16|1) and
                    store==(38<<26|dst<<21|3<<16|0x540) and
                    incoming.get(pc+4)=={pc} and incoming.get(pc+8)=={pc+4}):
                return pc+8
        # Common frame setup, FP loads/stores, and stores via the still
        # unchanged actor pointer cannot overwrite integer r3.
        if w==0x7C0802A6 or (op==37 and (w>>16&31)==1) or op in (38,44,48,50,52,54):
            continue
        try:
            if written_register(w)==3:
                break
        except ValueError:
            break
    return None


def release_model_gate(helper, pc):
    return RELEASE_RELAYS[helper][3].get(pc,()),RELEASE_EXCLUSIONS.get(pc,())


def direct_branch(pc, word):
    bits = 26 if word >> 26 == 18 else 16
    delta = word & ((1 << bits) - 4)
    if delta & (1 << (bits-1)):
        delta -= 1 << bits
    return ((0 if word & 2 else pc) + delta) & 0xFFFFFFFF


def immediate_argument(words, incoming, call, register):
    """Accept only an immediate in the same basic block, with no clobber."""
    pc = call
    for _ in range(16):
        prior = pc-4
        if incoming.get(pc) != {prior} or prior not in words:
            return None
        word = words[prior]
        op = word >> 26
        if op in (16, 18, 19):
            return None
        try:
            writer = written_register(word)
        except ValueError:
            # Quantized/floating arithmetic and ordinary stores cannot
            # produce an integer call argument. Other operations refuse.
            if op not in (38, 44, 48, 50, 52, 54, 59, 63):
                return None
            writer = None
        if writer == register:
            return signed16(word) if op == 14 and not (word >> 16 & 31) else None
        pc = prior
    return None


def literal_flow(start, words, successors, read_data=None):
    """Propagate only proven literal registers; merge disagreements to unknown."""
    before, queue, visits = {start: ({2:0x8043EA20,13:0x8043B5A0} if read_data else {})}, [start], 0
    while queue:
        pc = queue.pop()
        if pc not in words:
            continue
        visits += 1
        if visits > 8192:
            return {}
        known = before[pc].copy()
        w = words[pc]
        op, rt, ra, rb, xo = w>>26, w>>21&31, w>>16&31, w>>11&31, w>>1&1023
        if op in (16,18,19):
            if w & 1:
                for reg in (0,*range(3,13)):
                    known.pop(reg,None)
        elif op in (7,14,15):
            base = known.get(ra) if ra or op==7 else 0
            if base is None:
                known.pop(rt,None)
            else:
                imm = signed16(w)
                known[rt]=((base*imm if op==7 else base+(imm<<(16 if op==15 else 0))))&0xFFFFFFFF
        elif op in (24,26,27) or op==31 and xo==444:
            a, b = known.get(rt), ((w&0xFFFF)<<(16 if op==27 else 0) if op in (24,26,27) else known.get(rb))
            if a is None or b is None:
                known.pop(ra,None)
            else:
                known[ra]=a^b if op in (26,27) else a|b
        elif op==21:
            value=known.get(rt)
            if value is None:
                known.pop(ra,None)
            else:
                start_bit, end_bit = (w>>6)&31,(w>>1)&31
                mask=0
                for bit in range((end_bit-start_bit)%32+1):
                    mask|=1<<(31-((start_bit+bit)%32))
                known[ra]=((value<<rb)|(value>>(32-rb)))&mask
        elif op==31 and xo in (954,922):
            value=known.get(rt)
            if value is None:
                known.pop(ra,None)
            else:
                bits=8 if xo==954 else 16
                value&=(1<<bits)-1
                known[ra]=(value-(1<<bits) if value&(1<<(bits-1)) else value)&0xFFFFFFFF
        elif op==31 and xo in (40,266):
            a,b=known.get(ra),known.get(rb)
            if a is None or b is None:
                known.pop(rt,None)
            else:
                known[rt]=(b-a if xo==40 else a+b)&0xFFFFFFFF
        elif op in (32,34,40,42) or op==31 and xo in (23,87,279,343):
            indexed=op==31
            base=known.get(ra) if ra else 0
            offset=known.get(rb) if indexed else signed16(w)
            width=({23:4,87:1,279:2,343:2}[xo] if indexed else {32:4,34:1,40:2,42:2}[op])
            known.pop(rt,None)
            if read_data and base is not None and offset is not None:
                address=(base+offset)&0xFFFFFFFF
                # Read initialized source operands only, not actor/stack/heap
                # memory. This says nothing about a runtime write to the table.
                try:
                    raw=read_data(address,width)
                except ValueError:
                    pass
                else:
                    if len(raw)==width:
                        known[rt]=int.from_bytes(raw,"big",signed=op==42 or indexed and xo==343)&0xFFFFFFFF
        elif op==31 and xo==824:
            value=known.get(rt)
            if value is None:
                known.pop(ra,None)
            else:
                signed=value-(1<<32) if value>>31 else value
                known[ra]=(signed>>rb)&0xFFFFFFFF
        elif op in (38,44,48,50,52,54,59,63):
            pass  # non-updating stores and floating arithmetic
        elif op == 37 and ra == rt == 1:
            known.pop(1,None)
        else:
            try:
                dest=written_register(w)
            except ValueError:
                known.clear()
            else:
                if dest is not None:
                    known.pop(dest,None)
        for dest in successors[pc]:
            if dest not in words:
                continue
            previous=before.get(dest)
            merged=known.copy() if previous is None else {r:v for r,v in previous.items() if known.get(r)==v}
            if previous!=merged:
                before[dest]=merged
                queue.append(dest)
    return before


class ActionLinker:
    def __init__(self, dol, origins, common_slots):
        self.dol = dol
        self.origins = origins
        self.common_slots = {(g,c): at for g,c,at in common_slots}

    @lru_cache(maxsize=2048)
    def text(self, pc):
        if pc & 3:
            return 0
        try:
            return self.read_word(pc, "text")
        except ValueError:
            return 0

    def read_word(self, pc, kind="data"):
        return struct.unpack(">I", self.dol.read(pc, 4, kind))[0]

    @lru_cache(maxsize=1024)
    def dispatch(self, start, offset):
        """Recognize the standard actor-byte signed-indexed table dispatch."""
        words, incoming, successors, _, _, exhausted = self.graph(start, 0x90, 36)
        if exhausted:
            return None
        before=literal_flow(start,words,successors,self.data)
        candidates=set()
        for pc,word in words.items():
            op, rt, ra = word >> 26, word >> 21 & 31, word >> 16 & 31
            if op == 34 and ra == 3 and word & 0xFFFF == offset:
                for skip in (0,2):
                    # Some wrappers build the table base just after lbz.
                    first=pc+4+skip*4
                    if skip:
                        high,low=words.get(pc+4,0),words.get(pc+8,0)
                        if high>>26!=15 or low>>26!=14 or (high>>21&31)==rt or (low>>21&31)==rt:
                            continue
                    end=first+16
                    if any(incoming.get(p)!={p-4} for p in range(pc+4,end+1,4)):
                        continue
                    ext,shift,indexed,mtctr,branch=(words.get(p,0) for p in range(first,end+1,4))
                    if (ext,shift)!=(0x7C000774 | rt<<21 | rt<<16,0x5400103A | rt<<21 | rt<<16):
                        continue
                    dest,base=indexed>>21&31,indexed>>16&31
                    if indexed!=(0x7C00002E | dest<<21 | base<<16 | rt<<11):
                        continue
                    if mtctr!=(0x7C0903A6 | dest<<21) or branch not in (0x4E800420,0x4E800421):
                        continue
                    value=before.get(first+8,{}).get(base)
                    if value:
                        candidates.add(value)
        return next(iter(candidates)) if len(candidates)==1 else None

    def leaf(self, start, offset, index):
        table = self.dispatch(start, offset)
        if table is None or not 0 <= index <= 20:
            return None
        try:
            dest = self.read_word(table+4*index)
        except ValueError:
            return None
        return dest if self.text(dest) else None

    def phase_leaves(self, handler):
        table = self.dispatch(handler,0x540)
        expected = PHASE_WINDOWS.get(table)
        if expected is not None:
            actual = tuple(self.leaf(handler,0x540,phase) for phase in range(len(expected)))
            if actual == expected:
                return [(dest,f"phase-{phase}-candidate") for phase,dest in enumerate(actual)]
            return []
        first = self.leaf(handler,0x540,0)
        if first is None:
            return []
        result=[(first,"phase-zero-candidate")]
        words,incoming,*_=self.graph(first,0x90,36)
        if entry_phase_increment(words,incoming,first) is not None:
            second=self.leaf(handler,0x540,1)
            if second is not None and second!=first:
                result.append((second,"phase-1-entry-increment-candidate"))
        return result

    @lru_cache(maxsize=128)
    def matches_text(self, entry, size, digest):
        try:
            return sha256(self.dol.read(entry,size,"text")).hexdigest()==digest
        except ValueError:
            return False

    def review_call(self, entry, site, target, literal_r4=None):
        words,incoming,_,calls,_,exhausted = self.graph(entry,0x900,480)
        return (not exhausted and calls.get(site)==target and bool(words[site]&1) and
                (literal_r4 is None or immediate_argument(words,incoming,site,4)==literal_r4))

    @lru_cache(maxsize=32)
    def ordered_shot_sites(self, model):
        phase = STATIC_GROUND_PHASE_PAIRS.get(model)
        if phase:
            table,entry,keys,sites=phase
            wrapper,leaves,end,_,digest,gates=RELEASE_PHASE_WINDOWS[table]
            if (leaves[-1]!=entry or any(gates.get(site)!=0 for site in sites) or
                    self.dispatch(wrapper,0x540)!=table or not self.matches_text(wrapper,end-wrapper,digest)):
                return None
            return sites if (self.ordered_sibling_calls(entry,keys,sites,0x7FE3FB78) and
                             all(self.review_call(entry,site,0x800C3BE0,key) for site,key in zip(sites,keys,strict=True))) else None
        animated = ANIMATED_CRUISE_SHOTS.get(model)
        pair = animated[2:] if animated else ORDERED_SHOTS.get(model) or STATIC_CRUISE_SHOTS.get(model) or CLOCKED_GROUND_SHOTS.get(model)
        if pair is None:
            return None
        keys,sites = pair
        entry = 0x800CD1A0 if model in CYBER_MACHINE_SHOTS else RELAY if model==0x080C else 0x800C9F40
        if model in EAGLE_FAMILIES:
            entry = 0x8012A308
        if model in CLOCKED_GROUND_SHOTS:
            entry = CLOCKED_GROUND_DIRECT_ENTRIES.get(model,0x8018BF2C)
        if animated:
            entry=animated[0]
        static = model in STATIC_CRUISE_SHOTS
        if static:
            entry=0x801E7AFC
            if self.read_word(0x80392550)!=entry:
                return None
        if entry in DIRECT_RELEASE_GATES:
            size,digest,direct_gates=DIRECT_RELEASE_GATES[entry]
        else:
            size,digest,_,_=RELEASE_RELAYS[entry]
            direct_gates=None
        if not self.matches_text(entry,size,digest):
            return None
        restore = 0x7FC3F378 if model in CYBER_MACHINE_SHOTS else 0x7FE3FB78
        if not self.ordered_sibling_calls(entry,keys,sites,restore):
            return None
        for site,key in zip(sites,keys,strict=True):
            required,excluded = ((direct_gates.get(site),),()) if direct_gates is not None else release_model_gate(entry,site)
            family = animated[1] if animated else EAGLE_FAMILIES.get(model,"type-2")
            factory = 0x80082824 if model in CLOCKED_GROUND_SHOTS or family=="type-0" else 0x800C3BE0
            if required!=(model,) or excluded or not self.review_call(entry,site,factory,key):
                return None
        return sites

    def ordered_sibling_calls(self,entry,keys,sites,restore):
        words,incoming,edges,_,_,exhausted = self.graph(entry,0x900,480)
        return not exhausted and all(
            next_site==first+12 and words.get(first+4)==restore and words.get(first+8)==(0x38800000|key) and
            all(edges.get(pc)==(pc+4,) and incoming.get(pc+4)=={pc} for pc in range(first,next_site,4))
            for first,next_site,key in zip(sites,sites[1:],keys[1:]))

    def cued_pair_pose(self, entry, model, roots, local, requests):
        # This is a guarded upper-lane request in the action handler, not the
        # other context's release-only caller and not an entry-path default.
        if entry!=0x8010FD44 or model!=0x080C or roots[0]!=0x80325C80:
            return None
        size,digest,relay_site = RELAY_PARENTS[entry]
        sites = self.ordered_shot_sites(model)
        if (not sites or not self.matches_text(entry,size,digest) or
                not self.review_call(entry,relay_site,RELAY)):
            return None
        expected = {(f"{pc:08X}",key) for pc,key in zip(sites,ORDERED_SHOTS[model][0],strict=True)}
        found = {(r["site"],r["key"]) for r in requests if r["kind"]=="projectile" and r["family"]=="type-2" and
                 r.get("release_relay_site")==f"{relay_site:08X}" and r.get("required_model_at_call")=="080C"}
        if found!=expected:
            return None
        site,args = 0x8010FDBC,(1,2,3)
        rows,_,exhausted = self.calls(entry)
        if (exhausted or not self.review_call(entry,site,PROGRAM_REQUEST) or
                not any(pc==site and kind=="program" and operands==args for pc,kind,operands,_ in rows)):
            return None
        try:
            address = self.table_choice(roots[0],2,3)
            program = decode_program(self.data,address)
        except ValueError:
            return None
        cue = [op for op in program["commands"] if op["kind"]=="upper_controls" and op["raw"][4:8]=="0100"]
        if address!=0x80326026 or program["status"]!="terminal_wait" or len(cue)!=1 or cue[0]["preceding_waits"]!=1:
            return None
        pose = self.entry_clip_binding(entry,site,args,roots,local)
        return ({**pose,"release_helper":f"{RELAY:08X}","release_relay_site":f"{relay_site:08X}",
                 "cue_command":f'{cue[0]["address"]:08X}',"source_condition":"phase-zero-arm; positive cue on later visit"}
                if pose else None)

    @lru_cache(maxsize=128)
    def phase_release_requests(self, handler):
        table = self.dispatch(handler,0x540)
        if table==VIOLET_SHOT["table"]:
            return self.seeded_violet_shots(handler)
        window = RELEASE_PHASE_WINDOWS.get(table)
        if not window:
            return self.helper_phase_release(handler,table)
        wrapper, leaves, end, stores, digest, gates = window
        if (handler!=wrapper or not self.matches_text(wrapper,end-wrapper,digest) or
                tuple(self.leaf(handler,0x540,i) for i in range(len(leaves)))!=leaves):
            return None
        requests, _, exhausted = self.calls(leaves[-1])
        if exhausted:
            return None
        # Do not promote unmapped calls or anything outside the reviewed leaf.
        releases = tuple((pc,args,proof,gates[pc]) for pc,kind,args,proof in requests
                         if kind=="projectile" and leaves[-1]<=pc<end and pc in gates)
        if {r[0] for r in releases}!=gates.keys():
            return None
        evidence = {"phase_table":f"{table:08X}","phase_entry":f"{leaves[-1]:08X}",
                    "phase_transition_stores":tuple(f"{site:08X}" for site in stores)}
        if leaves[-1] in RELEASE_GUARDS:
            evidence["release_guard_candidate"]=RELEASE_GUARDS[leaves[-1]]
        return f"phase-{len(stores)}-reviewed-transition-candidate", evidence, releases

    def seeded_violet_shots(self,handler):
        r=VIOLET_SHOT
        wrapper,leaves,site=r["wrapper"],r["leaves"],r["site"]
        if (handler!=wrapper or not self.matches_text(wrapper,r["end"]-wrapper,r["digest"]) or
                tuple(self.leaf(handler,0x540,i) for i in range(len(leaves)))!=leaves or
                not self.review_call(leaves[-1],site,0x80082824)):
            return None
        evidence={"phase_table":f'{r["table"]:08X}',"phase_entry":f"{leaves[-1]:08X}",
                  "phase_transition_stores":tuple(f"{addr:08X}" for addr in r["stores"]),
                  "reviewed_caller_model":f'{r["model"]:04X}',
                  "release_guard_candidate":{"actor_byte":"149","first_value":0,"zero_store":"80188BC0",
                                             "one_bit_store":"80188CE0","requires_nonzero_return_from":"8006DBE0"}}
        releases=tuple((site,("type-0",key),"pinned-zero-seed-and-one-bit-selector",0) for key in r["keys"])
        return "phase-2-reviewed-selector-candidate",evidence,releases

    def helper_phase_release(self, handler, table):
        window = HELPER_WINDOWS.get(table)
        if window is None:
            return None
        wrapper,entry,consumer,end,digest,advance_site,helper,release_site,model = window
        helper_size,helper_digest,store = ADVANCE_HELPERS[helper]
        if (handler!=wrapper or not self.matches_text(wrapper,end-wrapper,digest) or
                not self.matches_text(helper,helper_size,helper_digest) or
                tuple(self.leaf(handler,0x540,i) for i in range(2))!=(entry,consumer) or
                not self.review_call(entry,advance_site,helper)):
            return None
        if not wrapper<=consumer<end:
            outside = SEPARATE_CONSUMERS.get(consumer)
            if not outside or not self.matches_text(consumer,*outside):
                return None
        requests, _, exhausted = self.calls(consumer)
        releases = tuple((pc,args,proof,0) for pc,kind,args,proof in requests
                         if kind=="projectile" and pc==release_site)
        if exhausted or len(releases)!=1:
            return None
        evidence = {"phase_table":f"{table:08X}","phase_entry":f"{consumer:08X}",
                    "phase_transition_stores":(f"{store:08X}",),
                    "phase_transition_calls":(f"{advance_site:08X}",),
                    "release_guard_candidate":{"actor_byte_equals":{"541":0,"1CEF":3,"1CF0":1},
                                               "requires_nonzero_return_from":"8006DBE0"}}
        # Model is the reviewed caller scope, not a check at the factory.
        return "phase-1-reviewed-helper-transition-candidate", {"reviewed_caller_model":f"{model:04X}",**evidence}, releases

    @lru_cache(maxsize=128)
    def release_routes(self, entry, group="B-shot"):
        _,_,_,calls,_,exhausted=self.graph(entry,0x900,480)
        if exhausted:
            return ()
        result=[]
        for site,helper in sorted(calls.items()):
            if helper not in RELEASE_RELAYS:
                continue
            if group not in RELAY_GROUPS.get(helper,("B-shot",)):
                continue
            size,digest,parents,_=RELEASE_RELAYS[helper]
            if (entry,site) not in parents or not self.text(site)&1:
                continue
            if not self.matches_text(helper,size,digest):
                continue
            parent = RELAY_PARENT_SIGNATURES.get((helper,entry))
            if parent and not self.matches_text(entry,*parent):
                continue
            requests=[q for q in self.calls(helper)[0] if q[1]=="projectile" and helper<=q[0]<helper+size]
            result.extend((site,helper,*q) for q in requests)
        return tuple(result)

    @lru_cache(maxsize=128)
    def graph(self, start, span, budget, *, reviewed_dispatch=None):
        """Bounded local CFG; never enters a branch-and-link target."""
        words, incoming, successors, calls, pending = {}, {start: {None}}, {}, {}, [start]
        boundary = set()
        while pending and len(words)<budget:
            pc = pending.pop()
            if pc in words:
                continue
            word = self.text(pc) if start<=pc<start+span else 0
            if not word:
                boundary.add(pc)
                continue
            words[pc] = word
            op, xo, bo = word>>26, word>>1&1023, word>>21&31
            edge = (pc+4,)
            if op in (16,18):
                dest = direct_branch(pc,word)
                if word & 1:
                    calls[pc] = dest
                else:
                    edge = (pc+4,dest) if op==16 and bo&20!=20 else (dest,)
                    if dest in FACTORIES or dest==PROGRAM_REQUEST:
                        calls[pc]=dest
                        edge=tuple(p for p in edge if p!=dest)
                    elif dest != start and self.text(dest)>>16 == 0x9421:
                        edge = tuple(p for p in edge if p != dest)
                        boundary.add(dest)
            elif op == 19 and xo in (16,528):
                if xo==528 and reviewed_dispatch and pc==reviewed_dispatch[0] and bo&20==20 and not word&1:
                    edge=(reviewed_dispatch[1],)
                elif xo == 528:
                    boundary.add(pc)
                    if bo&20==20 and not word&1:
                        edge=()
                elif bo&20==20 and not word&1:
                    edge = ()
            for dest in edge:
                incoming.setdefault(dest,set()).add(pc)
                pending.append(dest)
            successors[pc]=edge
        return words,incoming,successors,calls,len(boundary),bool(pending)

    @lru_cache(maxsize=512)
    def calls(self, start):
        words,incoming,successors,calls,boundary,exhausted=self.graph(start,0x900,480)
        flow=literal_flow(start,words,successors,self.data) if not exhausted else {}
        def argument(pc,reg):
            value=flow.get(pc,{}).get(reg)
            return value if value is not None else immediate_argument(words,incoming,pc,reg)
        result = []
        for pc,dest in sorted(calls.items()):
            if dest == PROGRAM_REQUEST:
                mask, group, choice = (argument(pc,r) for r in (4,5,6))
                if mask is not None and 1<=mask<16 and group is not None and 0<=group<256 and choice is not None and 0<=choice<128:
                    proof = "literal-block" if all(immediate_argument(words,incoming,pc,r) is not None for r in (4,5,6)) else "static-value-flow"
                    result.append((pc,"program",(mask,group,choice),proof))
            elif dest in FACTORIES:
                key = argument(pc,4)
                family = FACTORIES[dest]
                if key is not None and 0<=key<len(self.origins[family]):
                    proof = "literal-block" if immediate_argument(words,incoming,pc,4) is not None else "static-value-flow"
                    result.append((pc,"projectile",(family,key),proof))
        return tuple(result),boundary,exhausted

    def data(self, pc, size):
        return self.dol.read(pc, size, "data")

    @lru_cache(maxsize=1)
    def model_gate(self):
        # Exact conditional leaf: compare the current source model to
        # 0301/0302/0303 before these three call sites. 0303 also tests
        # stock. This predicate does not say which form is live at runtime.
        first,end=0x8010D040,0x8010D0B4
        try:
            image=self.dol.read(first,end-first,"text")
        except ValueError:
            return {}
        if sha256(image).hexdigest()!=MODEL_GATE_SHA:
            return {}
        return {site:signed16(self.text(cmp))+adjust for site,(cmp,adjust) in MODEL_GATED_COMPARE.items()}

    @lru_cache(maxsize=512)
    def table_choice(self, root, group, choice):
        if not root:
            raise ValueError("missing source program/descriptor root")
        tables = signed_table(self.data,root,16)
        if group>=len(tables):
            raise ValueError("source group outside table")
        rows = signed_table(self.data,tables[group])
        if choice>=len(rows):
            raise ValueError("source choice outside table")
        return rows[choice]

    @lru_cache(maxsize=1024)
    def descriptors(self, roots, mask, group, choice):
        addr = self.table_choice(roots[1 if group&128 else 0],group&127,choice)
        program = decode_program(self.data,addr)
        out = []
        for index,op in enumerate(program["commands"]):
            if op["kind"] == "select_descriptor":
                choices = (op["choice"],)
            elif op["kind"] == "directional_select":
                choices = tuple(dict.fromkeys(op["choices"]))
            else:
                continue
            bank = 3 if op["group"]&128 else 2
            for c in choices:
                try:
                    p = self.table_choice(roots[bank],op["group"]&127,c)
                    lanes = descriptor_lanes(self.data,p,mask)["lanes"]
                except ValueError:
                    continue
                out.append((index,c,op["preceding_waits"],op["start_frame"],
                            tuple((r["lane"],r["group"],r["index"]) for r in lanes)))
        return tuple(out), program["status"]

    def program_candidate(self, roots, local, args):
        mask,g,c = args
        try:
            descriptors,status = self.descriptors(roots,mask,g,c)
        except ValueError:
            descriptors,status = (), "unresolved program or descriptor"
        return {
            "lanes": mask, "program_group": g, "program_choice": c, "program_status": status,
            "clip_candidates": [
                {"command_index": i,"descriptor_choice": choice,"preceding_waits": waits,
                 "start_frame": start,"lanes": [
                    {"lane": lane, "bank": "common" if bg&128 else "local",
                     "group": bg&127, "choice": bc,
                     "archive_offset": (self.common_slots if bg&128 else local).get((bg&127,bc))}
                    for lane,bg,bc in lanes]}
                for i,choice,waits,start,lanes in descriptors]}

    @lru_cache(maxsize=512)
    def action_entry_request(self, entry):
        """A single resolved upper program in the local action-entry graph."""
        _,incoming,successors,calls,boundary,exhausted=self.graph(entry,0x900,480)
        if exhausted or boundary:
            return None
        sites={pc for pc,target in calls.items() if target==PROGRAM_REQUEST}
        requests=[(pc,args) for pc,kind,args,_ in self.calls(entry)[0] if kind=="program"]
        # A dynamic upper request must not disappear behind a literal one.
        if {pc for pc,_ in requests}!=sites:
            return None
        upper=[(pc,args) for pc,args in requests if args[0]&1]
        if len(upper)!=1:
            return None
        # Refuse a conditional or model-gated request that some local path
        # can skip. Opaque helper internals are still outside this analysis.
        call=upper[0][0]
        pending={pc:len(set(edges)) for pc,edges in successors.items() if pc!=call and edges}
        covered,work={call},[call]
        while work:
            for prior in incoming.get(work.pop(),()):
                if prior in pending:
                    pending[prior]-=1
                    if pending[prior]==0:
                        del pending[prior]
                        covered.add(prior)
                        work.append(prior)
        return upper[0] if entry in covered else None

    @lru_cache(maxsize=512)
    def action_entry_descriptor(self, roots, group, choice):
        """One non-directional frame-zero select before any authored wait."""
        try:
            address=self.table_choice(roots[1 if group&128 else 0],group&127,choice)
            program=decode_program(self.data,address)
            if program["status"] not in ("terminal_wait","yielding_loop"):
                return None
            prefix=[]
            for op in program["commands"]:
                if op["kind"]=="wait":
                    break
                prefix.append(op)
            selects=[op for op in prefix if op["kind"] in ("select_descriptor","directional_select")]
            if len(selects)!=1 or selects[0]["kind"]!="select_descriptor" or selects[0]["start_frame"]!=0:
                return None
            select=selects[0]
            dg,dc=select["group"],select["choice"]
            descriptor=self.table_choice(roots[3 if dg&128 else 2],dg&127,dc)
            lane,=descriptor_lanes(self.data,descriptor,1)["lanes"]
            return dg,dc,lane["group"],lane["index"]
        except ValueError:
            return None

    def action_entry_clip(self, leaf, phases, roots, local):
        phase_zero=next((dest for dest,scope in phases if scope in ("phase-0-candidate","phase-zero-candidate")),None)
        if phase_zero is None and self.dispatch(leaf,0x540) is not None:
            return None
        entry=leaf if phase_zero is None else phase_zero
        request=self.action_entry_request(entry)
        if request is None:
            return None
        pc,(mask,program_group,program_choice)=request
        return self.entry_clip_binding(entry,pc,(mask,program_group,program_choice),roots,local)

    def entry_clip_binding(self, entry, pc, args, roots, local):
        mask,program_group,program_choice = args
        descriptor=self.action_entry_descriptor(roots,program_group,program_choice)
        if descriptor is None:
            return None
        dg,dc,g,c=descriptor
        if g>=16 or c>=128 or (g,c) not in local:
            return None
        return {"entry":f"{entry:08X}","request_site":f"{pc:08X}","request_mask":mask,
                "program":[program_group,program_choice],"descriptor":[dg,dc],
                "clip":[g,c],"archive_offset":local[g,c]}

    def conditional_entry_programs(self, review, model):
        if review is None:
            return ()
        _,evidence,releases = review
        if (evidence["phase_table"]!=CONDITIONAL_ENTRY_TABLE or
                not any(gate==model for _,_,_,gate in releases)):
            return ()
        # The pinned entry initializes +6EA to 0 or 1 under the +5E0
        # bit test and loads it into r6 before incrementing the stored byte.
        # Do not use the incremented value or claim this is the next repeat.
        entry,site = 0x8015A8F8,int(CONDITIONAL_ENTRY_SITE,16)
        if not self.review_call(entry,site,PROGRAM_REQUEST):
            return ()
        return tuple((choice,{"actor_word":"5E0","mask":"00000040","set":bool(choice),
                              "choice_byte":"6EA","read_site":"8015A9E4"})
                     for choice in (0,1))

    def phase_one_shot_pose(self, review, model, roots, local):
        if review is None:
            return None
        _,evidence,releases = review
        reviewed = PHASE_ONE_SHOT_POSES.get(evidence["phase_table"])
        if not reviewed or not any(gate in (0,model) for _,_,_,gate in releases):
            return None
        entry,site = reviewed
        args = (15,2,0)
        requests,_,exhausted = self.calls(entry)
        if (exhausted or not self.review_call(entry,site,PROGRAM_REQUEST) or
                not any(pc==site and kind=="program" and operands==args for pc,kind,operands,_ in requests)):
            return None
        pose = self.entry_clip_binding(entry,site,args,roots,local)
        return {**pose,"phase":1,"phase_table":evidence["phase_table"]} if pose else None

    def attach_origin(self, item, args, model, joints):
        family,key = args
        origin = self.origins[family][key]
        selector = origin["source_joint_selector"]
        item.update(family=family,key=key,basis=origin["basis"],
                    source_joint_selector=selector,local_point=origin["local_point"],
                    joint_present_in_normal_rig=0<=selector<len(joints),
                    parent_model_matches=(origin.get("required_parent_model",model)==model))
        for extra in ("required_parent_model","adds_parent_offset","adds_retained_offset"):
            if extra in origin:
                item[extra]=origin[extra]

    def attach(self, actions, model, slots, joints, *, index_action_entries=False, installer=None):
        local = {(g,c): at for g,c,at in slots}
        for profile in actions["profiles"]:
            roots = tuple(int(r,16) for r in profile["program_and_descriptor_roots"])
            main = int(profile["main_handler"],16)
            for group in profile["groups"]:
                for row in group["rows"]:
                    if (row["classification"]!="selector-candidate" or not row["lanes"]&1
                            or row["handler_pair"][0]!=61):
                        continue
                    callback = self.leaf(main,0x580,row["state"])
                    if callback is None:
                        continue
                    leaf = self.leaf(callback,0x581,row["substate"]) or callback
                    phases=self.phase_leaves(leaf)
                    entries = [(leaf,"handler-or-substate")]
                    entries.extend((dest,scope) for dest,scope in phases if dest!=leaf)
                    if index_action_entries and group["group"]!="group-4":
                        entry_clip=self.action_entry_clip(leaf,phases,roots,local)
                        if entry_clip:
                            row["entry_clip_candidate"]=entry_clip
                    found, cues = [], []
                    reviewed = self.phase_release_requests(leaf)
                    if group["group"]=="B-shot":
                        elemental=dragon.dragon_shot(self,model,leaf,row,roots,local)
                        if elemental:
                            key,pose=elemental
                            item={"site":f"{dragon.RELEASE:08X}","scope":"reviewed-elemental-carrier",
                                  "kind":"projectile","argument_evidence":"pinned-model-dispatch",
                                  "control_flow_verified":False,"required_model_at_call":f"{model:04X}",
                                  "release_entry":f"{dragon.CONSUMER:08X}","phase_table":f"{dragon.PHASE_TABLE:08X}",
                                  "phase_entry":f"{dragon.CONSUMER:08X}",
                                  "release_guard_candidate":{"actor_signed_byte_positive":"1CEF","clear_before_request":"80075C28",
                                                             "requires_nonzero_return_from":"8006DBE0"}}
                            self.attach_origin(item,("type-47",key),model,joints)
                            found.append(item)
                            if pose and index_action_entries:
                                row["release_pose_candidate"]={**pose,"phase":1,"phase_table":item["phase_table"],
                                                               "release_site":item["site"]}
                        if index_action_entries and not row.get("entry_clip_candidate"):
                            phase_pose = self.phase_one_shot_pose(reviewed,model,roots,local)
                            if phase_pose:
                                row["entry_clip_candidate"]=phase_pose
                        conditional = self.conditional_entry_programs(reviewed,model)
                        alternatives = []
                        for choice,predicate in conditional:
                            args = (15,2,choice)
                            found.append({"site":CONDITIONAL_ENTRY_SITE,"scope":"phase-zero-conditional-candidate",
                                          "kind":"program","argument_evidence":"pinned-entry-choice",
                                          "control_flow_verified":False,"request_choice_condition":predicate,
                                          **self.program_candidate(roots,local,args)})
                            if index_action_entries:
                                clip = self.entry_clip_binding(0x8015A8F8,int(CONDITIONAL_ENTRY_SITE,16),args,roots,local)
                                if clip:
                                    alternatives.append({**clip,"request_choice_condition":predicate})
                        if len(alternatives)==2:
                            row["entry_clip_alternatives"]=alternatives
                    for entry,scope in entries:
                        requests, _, _ = self.calls(entry)
                        routes=[(None,None,*r) for r in requests]
                        if group["group"] in ("B-shot","X","B-charge"):
                            routes.extend(self.release_routes(entry,group["group"]))
                        if group["group"]!="group-4":
                            cues.extend({**cue,"entry":f"{entry:08X}","scope":scope}
                                        for cue in review_requests(self,entry,model,group["group"]))
                        for relay,helper,pc,kind,args,proof in routes:
                            item = {"site": f"{pc:08X}", "scope": scope,
                                    "kind": kind, "argument_evidence": proof, "control_flow_verified": False}
                            if relay is not None:
                                required,excluded=release_model_gate(helper,pc)
                                if (required and model not in required) or model in excluded:
                                    continue
                                item.update(release_relay_site=f"{relay:08X}",release_relay=f"{helper:08X}")
                                if required: # Each reviewed branch names one model.
                                    item["required_model_at_call"]=f"{required[0]:04X}"
                                if excluded:
                                    item["excluded_models_at_call"]=[f"{m:04X}" for m in excluded]
                            gate = self.model_gate().get(pc)
                            if pc in DIRECT_RELEASE_SITES:
                                start,size,digest,gate=DIRECT_RELEASE_SITES[pc]
                                if not self.matches_text(start,size,digest):
                                    continue
                            if gate is not None:
                                item["required_model_at_call"] = f"{gate:04X}"
                                if model != gate:
                                    continue  # Don't index another model's origin as this model's candidate.
                            if kind=="program":
                                item.update(self.program_candidate(roots,local,args))
                            else:
                                item["release_entry"]=f"{(entry if helper is None else helper):08X}"
                                self.attach_origin(item,args,model,joints)
                            found.append(item)
                    if reviewed is not None:
                        scope,evidence,releases = reviewed
                        caller = evidence.get("reviewed_caller_model")
                        for pc,args,proof,gate in releases:
                            if caller is not None and caller!=f"{model:04X}":
                                continue
                            if (gate>0 and model!=gate) or (gate<0 and model==-gate):
                                continue
                            item = {"site":f"{pc:08X}","scope":scope,"kind":"projectile",
                                    "argument_evidence":proof,"control_flow_verified":False,
                                    "release_entry":evidence["phase_entry"],**evidence}
                            if gate>0:
                                item["required_model_at_call"]=f"{gate:04X}"
                            if gate<0:
                                item["excluded_models_at_call"]=[f"{-gate:04X}"]
                            self.attach_origin(item,args,model,joints)
                            found.append(item)
                    if group["group"]=="B-shot":
                        side = neutral_side_shot(self,model,profile,leaf,row,roots,local,PROGRAM_REQUEST)
                        if side:
                            pose,route = side
                            for side_name,key in zip(("zero","nonzero"),route["keys"],strict=True):
                                item={"site":f'{route["site"]:08X}',"scope":"reviewed-neutral-side-candidate",
                                      "kind":"projectile","argument_evidence":"pinned-side-dispatch","control_flow_verified":False,
                                      "release_entry":pose["release_helper"],"release_relay":pose["release_helper"],
                                      "release_relay_site":pose["release_relay_site"],
                                      "required_model_at_call":f"{model:04X}","source_side_choice":side_name,
                                      "source_side_actor_byte":pose["source_side_actor_byte"]}
                                self.attach_origin(item,("type-0",key),model,joints)
                                found.append(item)
                            if index_action_entries:
                                row["entry_clip_candidate"]=pose
                    if found:
                        row["request_candidates"] = found
                    if index_action_entries and group["group"]=="B-shot" and not row.get("entry_clip_candidate"):
                        pose = rifle_shot_pose(self,installer,leaf,phases,model,roots,local,found,PROGRAM_REQUEST)
                        if not pose:
                            pose = self.cued_pair_pose(leaf,model,roots,local,found)
                        if not pose:
                            pose = throw_shot_pose(self,installer,leaf,phases,model,roots,local,found,PROGRAM_REQUEST)
                        if not pose:
                            pose = theta_shot_pose(self,installer,leaf,phases,model,roots,local,found,PROGRAM_REQUEST)
                        if not pose:
                            pose = beam_b_shot_pose(self,installer,leaf,phases,model,roots,local,found,PROGRAM_REQUEST)
                        if not pose:
                            pose = heavy_b_shot_pose(self,installer,leaf,phases,model,roots,local,found,PROGRAM_REQUEST)
                        if pose:
                            row["entry_clip_candidate"] = pose
                    if cues:
                        row["action_sfx_candidates"] = cues
                    row["class_callback_candidate"] = f"{callback:08X}"
