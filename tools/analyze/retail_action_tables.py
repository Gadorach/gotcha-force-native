"""Static GG4E08 action-selector inventory. No control predicates are executed."""
from collections import defaultdict
import struct

from tools.analyze.retail_movement_tables import BETA_INSTALLER, BETA_MODEL_CHECK, BETA_OVERRIDE, beta_override_arm, literal_at_store, signed16


GROUPS = ("B-shot", "B-close", "X", "B-charge", "group-4", "X-charge")
FIELDS = (0x4B0, 0x4EC, 0x4F0, 0x4B4, 0x1D80, 0x1D84, 0x1D88, 0x1D8C)
MODEL_JUMP = (0xA88303E8, 0x3804FAFA, 0x2800000E, 0x4D810020, 0x3C808030,
              0x5400103A, 0x3884F278, 0x7C04002E, 0x7C0903A6, 0x4E800420)


def installer_tables(dol, installer, model):
    """Recognize the shared copy prefix, then enumerate local pointer-store paths."""
    words = tuple(value for value, in struct.iter_unpack(">I", dol.read(installer, 144*4, "text")))
    if (words[0] != 0x38000000 or words[1] >> 16 != 0x3C80 or words[2] != 0x2C000010
            or words[3] >> 16 != 0x3884 or words[4] != 0x40800084):
        raise ValueError("unknown installer table-copy entry")
    for i in range(16):
        if words[5+2*i:7+2*i] != (0x80040000+4*i, 0x900304B4+4*i):
            raise ValueError("unknown installer table-copy rows")
    base = ((signed16(words[1]) << 16) + signed16(words[3])) & 0xFFFFFFFF
    main, = struct.unpack(">I", dol.read(base, 4, "data"))
    initial = (0, *struct.unpack(">2I", dol.read(base+0x38, 8, "data")), main, 0, 0, 0, 0)
    queue, seen, returned = [(37, initial)], set(), set()
    open_paths, missing_stores = set(), set()
    while queue:
        i, fields = queue.pop()
        state = i, fields
        if state in seen:
            continue
        if not 37 <= i < len(words) or len(seen) >= 384:
            open_paths.add(i)
            continue
        seen.add(state)
        override = beta_override_arm(words,installer,i,model)
        if override is not None:
            if override:
                queue.append((i+3,fields))
            else:
                returned.add(fields)
            continue
        if words[i:i+10] == MODEL_JUMP:
            # A pinned 15-entry table indexed by model - 0x0506. The
            # out-of-range arm returns without taking the indirect branch.
            member = model - 0x0506
            if not 0 <= member < 15:
                returned.add(fields)
            else:
                target, = struct.unpack(">I", dol.read(0x802FF278+member*4, 4, "data"))
                displacement = target-installer
                if displacement % 4 or not i+10 <= displacement//4 < len(words):
                    raise ValueError("model-index table target outside installer suffix")
                queue.append((displacement//4, fields))
            continue
        word = words[i]
        offset = word & 0xFFFF
        if offset in FIELDS and word & 0xFC1FFFFF == 0x90030000 + offset:
            try:
                pointer = literal_at_store(words, i)
            except ValueError:
                pointer = 0  # An unqualified override cannot inherit the old pointer.
                missing_stores.add(i)
            binding = list(fields)
            binding[FIELDS.index(offset)] = pointer
            fields = tuple(binding)
        op = word >> 26
        if op in (16, 18):
            bits = 16 if op == 16 else 26
            displacement = word & ((1 << bits) - 4)
            if displacement & (1 << (bits-1)):
                displacement -= 1 << bits
            target = i+displacement//4
            if word & 3 or target <= i:
                open_paths.add(i)
                continue
            queue.append((target, fields))
            if op == 16:
                queue.append((i+1, fields))
        elif op == 19:
            xo, bo = (word >> 1) & 1023, (word >> 21) & 31
            if xo != 16 or word & 1:  # Do not follow an indirect transfer.
                open_paths.add(i)
                continue
            returned.add(fields)
            if bo != 20:
                queue.append((i+1, fields))
        elif word == 0:
            open_paths.add(i)
        else:
            queue.append((i+1, fields))
    return sorted(returned), len(open_paths), len(missing_stores)


def decode_tables(dol, binding):
    cfg, entry, selector = binding[:3]
    modes = struct.unpack(">6b", dol.read(cfg+0xB8, 6, "data"))
    pointers = struct.unpack(">6I", dol.read(entry, 24, "data"))
    pairs = tuple(struct.iter_unpack(">bb", dol.read(selector, 96, "data")))
    if any(n < -1 or n >= 64 for pair in pairs for n in pair):
        raise ValueError("unknown lane selector handler")
    actions = []
    for group, mode in enumerate(modes):
        buckets = defaultdict(list)
        if mode not in (-1,0,1):
            raise ValueError("unknown action table mode")
        sectors = 4 if mode == 1 else 1
        if mode >= 0:
            packed = dol.read(pointers[group], 6*sectors*4, "data")
            for index, (sel, lane, state, substate) in enumerate(struct.iter_unpack(">4b", packed)):
                if sel < -1 or sel >= len(pairs):
                    raise ValueError("unknown action selector")
                if sel == -1:
                    status = "explicit-refusal"
                elif (sel, lane, state, substate) == (0,0,0,0):
                    status = "zero-row"
                else:
                    if lane not in (1,2,3) or not (0 <= state <= 20 and 0 <= substate <= 20):
                        raise ValueError("unknown action row")
                    status = "selector-candidate"
                buckets[(sel,lane,state,substate,status)].append(divmod(index,sectors))
        actions.append({"group": GROUPS[group], "mode": mode, "rows": [
            {"selector": sel, "lanes": lane, "state": state, "substate": substate,
             "classification": status, "contexts": contexts,
             "handler_pair": list(pairs[sel]) if sel >= 0 else None}
            for (sel,lane,state,substate,status),contexts in sorted(buckets.items())]})
    return actions


def action_candidates(dol, installer, model):
    try:
        bindings, open_paths, unknown = installer_tables(dol, installer, model)
    except ValueError as exc:
        return {"status": str(exc), "profiles": [], "runtime_context_verified": False}
    result, refused = [], 0
    for binding in bindings:
        try:
            if not all(binding[:4]):
                raise ValueError("incomplete installer path")
            data = decode_tables(dol, binding)
        except ValueError:
            refused += 1
        else:
            result.append({"config": f"{binding[0]:08X}", "context_table": f"{binding[1]:08X}",
                           "selector_table": f"{binding[2]:08X}", "main_handler": f"{binding[3]:08X}",
                           "program_and_descriptor_roots": [f"{v:08X}" for v in binding[4:]],
                           "groups": data})
    report = {"status": "static paths only", "profiles": result, "refused_bindings": refused,
              "open_path_boundaries": open_paths, "unresolved_stores": unknown,
              "runtime_context_verified": False}
    if installer==BETA_INSTALLER and (len(result)==1 and not (refused or open_paths or unknown)):
        words=struct.unpack(">10I",dol.read(BETA_MODEL_CHECK,40,"text"))
        if words==BETA_OVERRIDE:
            report["model_gated_config"]={"compare_site":"8019858C","override_model":"010A",
                                          "override_selected":model==0x010A}
    return report
