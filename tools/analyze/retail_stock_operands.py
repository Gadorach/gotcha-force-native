"""GG4E08 level stock operands and reviewed same-actor debit links; offline only."""
from collections import deque
from functools import lru_cache
from hashlib import sha256

from tools.analyze.retail_action_links import FACTORIES, direct_branch, immediate_argument
from tools.analyze import retail_dragon_shot as dragon
from tools.analyze.retail_movement_tables import written_register


LEVEL_TABLE = 0x802F2988
NORMAL_LEVEL_START = 0x804339E8
DEBIT = 0x8006DBE0
REFILL = 0x8006DCC0
LEVEL_CHANNELS = (0, 2)
TEXT = (
    (0x800563D8,0x1C0,"74e88eb712ed82b757a8b10426bf0843086fd42d7ed859c78bae5ec4da6635ec"),
    (DEBIT,0xE0,"0b2e322f452d317b5f515e0ebd95f821a10cdbee5db9cdbd07678c8d251eed31"),
    (REFILL,0x150,"2d58ff4852a700a526d906055a7d70dcee182783f6f0a3b0d83d3be2fd20de8f"),
)
POLICIES = {
    0:{"on_debit":"arm-preset-when-empty","shared_refill":"restore-limit-on-crossing-zero"},
    1:{"on_debit":"add-preset-to-clock","shared_refill":"restore-intervening-counts"},
}
# (factory call site -> same actor handler, guarded debit call). These are
# witnesses to the call operands, not a general one-debit-per-child rule.
RELEASE_DEBITS = {
    0x80076C10:(0x80076B48,0x80076BFC),
    0x80076EFC:(0x80076E00,0x80076EE8),
    0x800770D0:(0x80077018,0x800770BC),
    0x800A403C:(0x800A3F74,0x800A4028),
    0x800A4328:(0x800A422C,0x800A4314),
    0x800A44FC:(0x800A4444,0x800A44E8),
    **{pc:(0x8015AA24,0x8015AAAC) for pc in (0x8015AACC,0x8015AAE4,0x8015AAFC,0x8015AB14)},
    0x8010FF24:(0x8010FE78,0x8010FF04),
    0x8010FF30:(0x8010FE78,0x8010FF04),
    0x8018C248:(0x8018C164,0x8018C234),
    0x8018C254:(0x8018C164,0x8018C234),
}


def stock_schema(dol):
    for at,size,digest in TEXT:
        if sha256(dol.read(at,size,"text")).hexdigest()!=digest:
            raise ValueError(f"stock operand source signature: {at:08X}")
    start = dol.read(NORMAL_LEVEL_START,1,"data")[0]
    if start+10>20:
        raise ValueError("normal stock row extent")
    return {"source_level_table":f"{LEVEL_TABLE:08X}","normal_row_base":start,
             "normal_row_base_source":f"{NORMAL_LEVEL_START:08X}","row_bytes":18,"channel_offsets":[2,10],
             "level_row_channels":list(LEVEL_CHANNELS),"untouched_by_level_initializer":[1],
            "halfword_fields":["initial","limit","mode","preset"],
            "initializer_reads":"80056464","initialized_channels":2,
            "debit_helper":f"{DEBIT:08X}","refill_helper":f"{REFILL:08X}","refill_channels_visited":3,
            "reviewed_modes":POLICIES,"clock_units":"source operands, not native ticks",
            "runtime_stock_verified":False}


def stock_rows(levels):
    if len(levels)!=10 or any(len(row)!=9 for row in levels):
        raise ValueError("normal stock level row extent")
    channels = []
    for channel,start in zip(LEVEL_CHANNELS,(1,5),strict=True):
        rows = []
        for level,row in enumerate(levels,1):
            initial,limit,mode,preset = row[start:start+4]
            # Preserve contradictions and disabled rows. These are observed
            # signed fields, not constraints on authored/native capacity.
            item = {"level":level,"initial":initial,"limit":limit,"mode":mode,"preset":preset}
            if initial>limit:
                item["initial_exceeds_limit"]=True
            rows.append(item)
        channels.append({"channel":channel,"levels":rows})
    return channels


def attach_release_stock(releases, linker):
    for release in releases:
        site = int(release["site"],16)
        witness = RELEASE_DEBITS.get(site)
        if witness is None:
            continue
        entry,debit = witness
        entry_key = f"{entry:08X}"
        if not any(entry_key in (r.get("phase_entry"),r.get("relay_handler")) for r in release["routes"]):
            continue
        words,incoming,_,_,_,exhausted = linker.graph(entry,0x900,480)
        if (exhausted or not linker.review_call(entry,debit,DEBIT) or
                words.get(debit+4)!=0x2C030000 or words.get(debit+8,0)&0xFFFF0003!=0x41820000 or
                not debit<site<direct_branch(debit+8,words[debit+8]) or site not in words):
            continue
        channel,count,commit = (immediate_argument(words,incoming,debit,r) for r in (4,5,6))
        if channel not in (0,1,2) or count is None or not 0<count<=255 or commit!=1:
            continue
        release["stock_candidate"] = {"channel":channel,"cost":count,"debit_site":f"{debit:08X}",
                                      "commit":True,"failure_branch":f"{debit+8:08X}",
                                       "normal_level_operand_channel":channel if channel in LEVEL_CHANNELS else None,
                                      "scope":"shared debit call; not per-child accounting"}


def actor_pointer_flow(entry, words, successors):
    """Track only unchanged copies of the inbound actor pointer. Unknown kills proof."""
    before, pending = {entry:1<<3}, deque([entry])
    visits = 0
    while pending:
        pc = pending.popleft()
        if pc not in words:
            continue
        visits += 1
        if visits>8192:
            return {}
        current = before[pc]
        w = words[pc]
        op,rt,ra,rb,xo = w>>26,w>>21&31,w>>16&31,w>>11&31,w>>1&1023
        if op in (16,18,19):
            if w&1:
                # Calls preserve only nonvolatile GPRs in this local proof.
                current &= 0xFFFFC000
        elif op==31 and xo==444 and rt==rb:
            current = (current&~(1<<ra)) | (((current>>rt)&1)<<ra)
        elif op in (24,25,26,27,28,29):
            current &= ~(1<<ra) # immediate integer logic writes RA, not RS
        elif op==7 or (op==31 and xo in (40,104,266)):
            current &= ~(1<<rt) # mulli, subf, neg, add
        elif op==31 and xo==922:
            current &= ~(1<<ra) # extsh
        elif op==37: # stwu writes RA; no other update forms are assumed.
            current &= ~(1<<ra)
        elif op in (38,44,47,48,50,52,54,56,59,60,63):
            pass
        elif op==46:
            current &= (1<<rt)-1
        else:
            try:
                dest = written_register(w)
            except ValueError:
                current = 0
            else:
                if dest is not None:
                    current &= ~(1<<dest)
        for dest in successors.get(pc,()):
            if dest not in words:
                continue
            merged = (before.get(dest,0xFFFFFFFF)&current)
            if dest not in before or before[dest]!=merged:
                before[dest] = merged
                pending.append(dest)
    return before


def reaches(successors, start, target, skip=None):
    seen, pending = set(), [start]
    while pending:
        pc = pending.pop()
        if pc==target:
            return True
        if pc in seen:
            continue
        seen.add(pc)
        pending.extend(dest for dest in successors.get(pc,()) if (pc,dest)!=skip)
    return False


class StockGuardIndex:
    """Strict local CFG links. Neither a missing row nor a proof is a live stock claim."""
    def __init__(self, linker):
        self.linker = linker

    @lru_cache(maxsize=128)
    def handler_guards(self, entry, elemental_key=None):
        factory_targets=FACTORIES
        if elemental_key is None:
            graph=self.linker.graph(entry,0x900,480)
        else:
            if entry!=dragon.CONSUMER or not 0<=elemental_key<len(dragon.CASE_TARGETS) or not dragon.source_matches(self.linker):
                return {}
            graph=self.linker.graph(entry,0x900,480,reviewed_dispatch=(dragon.DISPATCH,dragon.CASE_TARGETS[elemental_key]))
            factory_targets=(dragon.FACTORY,)
        words,incoming,successors,calls,boundaries,exhausted = graph
        if boundaries or exhausted or DEBIT not in calls.values():
            return {}
        aliases = actor_pointer_flow(entry,words,successors)
        sites = [pc for pc,target in sorted(calls.items()) if target in factory_targets and aliases.get(pc,0)&8]
        result = {}
        for debit,target in sorted(calls.items()):
            compare,branch,success = debit+4,debit+8,debit+12
            if (target!=DEBIT or words[debit]>>26!=18 or not words[debit]&1 or not aliases.get(debit,0)&8 or
                    incoming.get(compare)!={debit} or incoming.get(branch)!={compare} or
                    words.get(compare)!=0x2C030000 or words.get(branch,0)&0xFFFF0003!=0x41820000):
                continue
            failure = direct_branch(branch,words[branch])
            if set(successors.get(branch,()))!={success,failure} or success==failure:
                continue
            channel,cost,commit = (immediate_argument(words,incoming,debit,r) for r in (4,5,6))
            if channel not in (0,1,2) or cost is None or not 0<cost<=255 or commit!=1:
                continue
            for site in sites:
                if (not reaches(successors,success,site) or reaches(successors,failure,site) or
                        reaches(successors,entry,site,skip=(branch,success))):
                    continue
                result.setdefault(site,[]).append({"entry":f"{entry:08X}","debit_site":f"{debit:08X}",
                    "success_branch":f"{success:08X}","failure_branch":f"{failure:08X}",
                    "channel":channel,"cost":cost,"commit":True,"same_input_actor":True,
                    "normal_level_operand_channel":channel if channel in LEVEL_CHANNELS else None})
        return result

    def attach(self, releases):
        for release in releases:
            site = int(release["site"],16)
            for route in release["routes"]:
                entry = route.get("release_entry")
                if entry is not None:
                    if release["family"]=="type-47":
                        key=release["key"]
                        if (site!=dragon.RELEASE or route.get("phase_table")!=f"{dragon.PHASE_TABLE:08X}" or
                                dragon.DRAGON_KEYS.get(int(release.get("required_model_at_call","0"),16))!=key):
                            continue
                        guards=self.handler_guards(int(entry,16),key).get(site,())
                    else:
                        guards=self.handler_guards(int(entry,16)).get(site,())
                    if guards:
                        route["local_stock_guards"] = guards
