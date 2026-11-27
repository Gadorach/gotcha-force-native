"""Index GG4E08 function boundaries and all roster installer/action paths.

Output is metadata (addresses, dispatch selectors, graph edges and status),
not a code dump, recompilation or claim of retail behavior parity.
"""
import argparse
from bisect import bisect_right
from collections import Counter
from contextlib import closing
from hashlib import sha256
import json
from pathlib import Path
import re
import sqlite3
import struct

from tools.analyze.reaction_installer_tables import INSTALLER_TABLE, model_table
from tools.analyze.retail_action_links import ActionLinker, direct_branch
from tools.analyze.retail_action_tables import action_candidates
from tools.analyze.retail_battle_engine_map import ROOTS
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import CATALOG_SHA256, open_regular, read_catalog
from tools.extract.source_image import DolImage

MAX_SYMBOLS_BYTES = 4 << 20
MAX_FUNCTIONS = 20000
MAX_DB_BYTES = 512 << 20
FUNCTION = re.compile(rb"=\s*\.(?:init|text):0x([0-9A-Fa-f]+);\s*//\s*type:function\s+size:0x([0-9A-Fa-f]+)(?:\s|$)")
OBJECT = re.compile(rb"=\s*\.[a-z]+:0x([0-9A-Fa-f]+);\s*//\s*type:object\s+size:0x([0-9A-Fa-f]+)(?:\s|$)")
MATCH_NAME = re.compile(r"fn_([0-9A-Fa-f]{8})")
MATCH_STATES = {"matched", "open", "asm", "nosource"}
# Complete bounds and digest of the model-gated +581 wrapper; see the
# action class map. Only these two arms are selected, not its phase paths.
CLOSE_WRAPPER = 0x80070604
CLOSE_SUBTABLES = {True: 0x802D3BE8, False: 0x802D3BD4}
CLOSE_WRAPPER_BYTES = 0x6C
CLOSE_WRAPPER_SHA = "eea8c2484e1a87e9f9ddc1e5c3474d2ed675499f3392a94f7a56c6e64da946cb"
GUARDED_MAIN = 0x80156FFC
GUARDED_MAIN_SHA = "412ce9bc149a5764802350fd38a54f22b1800099c022f6537b533571a8cf70ab"
DIRECT_MAIN = 0x801AE7F4
DIRECT_MAIN_SHA = "6b62a13d5cdfa2eeb13953a8ec3442d2df706d4c2120d1bfa40512eed6fa6950"


def address(value):
    return f"{value:08X}"


def read_bounded(path, ceiling):
    with open_regular(path) as stream:
        data = stream.read(ceiling + 1)
    if not data or len(data) > ceiling:
        raise ValueError("input extent")
    return data


def boundaries(image, raw):
    spans = {}
    for line in raw.splitlines():
        if b"type:function" not in line:
            continue
        match = FUNCTION.search(line)
        if match is None or len(spans) >= MAX_FUNCTIONS:
            raise ValueError("unknown function boundary or function budget")
        start, size = (int(value, 16) for value in match.groups())
        if start in spans or (start | size) & 3 or size < 4:
            raise ValueError("duplicate or unaligned function boundary")
        image.read(start, size, "text")
        spans[start] = size
    spans = dict(sorted(spans.items()))
    end = 0
    for start, size in spans.items():
        if start < end:
            raise ValueError("overlapping function boundaries")
        end = start + size
    if not spans:
        raise ValueError("no function boundaries")
    return spans


def object_sizes(raw):
    sizes = {}
    for match in (OBJECT.search(line) for line in raw.splitlines()):
        if match is not None:
            start, size = (int(value, 16) for value in match.groups())
            sizes[start] = size if sizes.get(start, size) == size else 0
    return sizes


def matching_status(path, spans):
    """Only join exact address/size matches; don't publish names or scores."""
    with open_regular(path) as stream:
        stream.seek(0, 2)
        if not 0 < stream.tell() <= MAX_DB_BYTES:
            raise ValueError("decomp database extent")
    # A local snapshot, never create journals, or consult live sidecars.
    with closing(sqlite3.connect(Path(path).absolute().as_uri() + "?mode=ro&immutable=1",
                                 uri=True)) as db:
        rows = db.execute("SELECT name,size,status FROM functions LIMIT ?", (MAX_FUNCTIONS + 1,))
        found = {}
        for n, (name, size, status) in enumerate(rows):
            if n >= MAX_FUNCTIONS:
                raise ValueError("decomp database row budget")
            match = MATCH_NAME.fullmatch(name)
            if match is None:
                continue
            start = int(match[1], 16)
            if spans.get(start) != size:
                continue
            if start in found or status not in MATCH_STATES:
                raise ValueError("duplicate or unknown function match status")
            found[start] = status
        return found


def scan(start, raw):
    """Walk only entry-reachable words; preserve indirect and off-span edges."""
    words = tuple(value for value, in struct.iter_unpack(">I", raw))
    visited, pending = set(), [0]
    transfers, indirect = [], []
    while pending:
        index = pending.pop()
        while index not in visited:
            visited.add(index)
            word = words[index]
            op, bo, xo, link = word >> 26, word >> 21 & 31, word >> 1 & 1023, word & 1
            pc = start + 4 * index
            # For BO: suppress both the CTR and CR tests.
            conditional = (bo & 20) != 20
            if op in (16, 18):
                target = direct_branch(pc, word)
                if link or not start <= target < start + len(raw):
                    transfers.append((4 * index, address(target), "call" if link else "branch"))
                if start <= target < start + len(raw):
                    pending.append((target - start) // 4)
                if not link and (op == 18 or not conditional):
                    break
            elif op == 19 and xo in (16, 50, 528):
                if xo == 50:  # rfi: neither a function return nor a fallthrough.
                    break
                if xo == 528 or link:  # plain blr/bclr is the ABI return
                    indirect.append((4 * index, ("ctr" if xo == 528 else "lr") +
                                     ("-call" if link else "-branch")))
                if not link and not conditional:
                    break
            index += 1
            if index == len(words):
                transfers.append((4 * (index - 1), address(start + len(raw)), "fallthrough"))
                break
    return sorted(transfers), sorted(indirect), 4 * len(visited)


def containing(starts, spans, dest):
    i = bisect_right(starts, dest) - 1
    return starts[i] if i >= 0 and dest < starts[i] + spans[starts[i]] else None


def make_graph(image, spans, statuses):
    functions, counts = {}, Counter(functions=len(spans))
    starts = list(spans)
    for start, size in spans.items():
        direct, indirect, reached = scan(start, image.read(start, size, "text"))
        record = {"size": size}
        if start in statuses:
            record["decomp"] = statuses[start]
            counts["decomp_" + statuses[start]] += 1
        if reached != size:
            record["reached_bytes"] = reached
        if direct:
            record["direct"] = direct
        if indirect:
            record["indirect"] = indirect
        counts["indirect_sites"] += len(indirect)
        counts["bounded_bytes"] += size
        counts["entry_walk_bytes"] += reached
        for _, target, kind in direct:
            dest = int(target, 16)
            boundary = "entry" if dest in spans else (
                "interior" if containing(starts, spans, dest) is not None else "unmapped")
            counts[kind + "_" + boundary] += 1
        functions[address(start)] = record
    return functions, counts


def table_entry(linker, sizes, table, index):
    if table is None or not 0 <= index < sizes.get(table, 0) // 4:
        return None
    try:
        entry = linker.read_word(table + 4 * index)
    except ValueError:
        return None
    return entry if linker.text(entry) else None


def action_route(linker, sizes, model, main, row):
    """Apply only the reviewed lane-61 chain and known model comparison."""
    if not row["lanes"] & 1 or row["handler_pair"][0] != 61:
        return {"boundary": "no-reviewed-lane"}
    if main == DIRECT_MAIN and (row["state"], row["substate"]) == (0, 0):
        if linker.matches_text(main, 0xB4, DIRECT_MAIN_SHA):
            return {"direct_entry": address(main)}
    if main == GUARDED_MAIN and linker.matches_text(main, 0x6C, GUARDED_MAIN_SHA):
        choices = []
        for condition, table in (("actor+0x582 == 1", 0x803429C4), ("actor+0x582 != 1", 0x803429D4)):
            choices.append({"guard": condition, **family_route(linker, sizes, model, table, row)})
        return {"guarded_families": choices}
    return family_route(linker, sizes, model, linker.dispatch(main, 0x580), row)


def family_route(linker, sizes, model, family_table, row):
    family = table_entry(linker, sizes, family_table, row["state"])
    if family is None:
        return {"boundary": "unresolved-family"}
    result = {"family_table": address(family_table), "family": address(family)}
    if family == CLOSE_WRAPPER:
        if not linker.matches_text(CLOSE_WRAPPER, CLOSE_WRAPPER_BYTES, CLOSE_WRAPPER_SHA):
            return {**result, "boundary": "unverified-model-gated-wrapper"}
        subtable = CLOSE_SUBTABLES[model == 0x000A]
    else:
        subtable = linker.dispatch(family, 0x581)
    sub = table_entry(linker, sizes, subtable, row["substate"])
    if subtable is not None:
        result["substate_table"] = address(subtable)
        if sub is None:
            return {**result, "boundary": "unresolved-substate"}
        result["substate_entry"] = address(sub)
    else:
        result["boundary"] = "no-recognized-substate-table"
    leaf = sub if sub is not None else family
    table = linker.dispatch(leaf, 0x540)
    phases = [(entry, scope) for index, (entry, scope) in enumerate(linker.phase_leaves(leaf))
              if table_entry(linker, sizes, table, index) == entry]
    if phases:
        result["phase_table"] = address(table)
        result["phase_candidates"] = [[address(entry), scope] for entry, scope in phases]
    return result


def roster_paths(image, catalog, sizes):
    linker = ActionLinker(image, {}, ())
    borgs, profiles, profile_keys, installers = [], {}, {}, set()
    counts = Counter()
    for borg in catalog:
        installer, reaction = model_table(image, borg.model)
        installers.add(installer)
        actions = action_candidates(image, installer, borg.model)
        profile_ids = []
        for profile in actions["profiles"]:
            main = int(profile["main_handler"], 16)
            for group in profile["groups"]:
                for row in group["rows"]:
                    classification = row["classification"]
                    counts[classification] += 1
                    if classification == "selector-candidate":
                        route = row["code_path"] = action_route(linker, sizes, borg.model, main, row)
                        arms = route.get("guarded_families", (route,))
                        for key in ("family", "substate_entry", "phase_candidates", "direct_entry"):
                            counts["rows_with_" + key] += any(key in arm for arm in arms)
                        counts["rows_with_runtime_family_guard"] += "guarded_families" in route
                        for boundary in {arm["boundary"] for arm in arms if "boundary" in arm}:
                            counts[boundary] += 1
            key = json.dumps(profile, sort_keys=True, separators=(",", ":"))
            if key not in profile_keys:
                ident = f"p{len(profiles):03d}"
                profiles[ident] = profile
                profile_keys[key] = ident
            profile_ids.append(profile_keys[key])
        gaps = {key: val for key, val in actions.items() if key not in ("profiles", "runtime_context_verified")}
        borgs.append({"id": borg.id, "installer": address(installer),
                      "reaction_table": address(reaction) if reaction else None,
                      "profiles": profile_ids, "installer_path": gaps})
        counts["with_profiles"] += bool(profile_ids)
        counts["with_reaction_table"] += reaction is not None
        counts["profile_references"] += len(profile_ids)
    counts.update(borgs=len(borgs), installers=len(installers), unique_profiles=len(profiles))
    return borgs, profiles, counts


def recover(image, catalog, symbols, db=None):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    spans = boundaries(image, symbols)
    functions, graph_counts = make_graph(image, spans, matching_status(db, spans) if db else {})
    borgs, profiles, roster_counts = roster_paths(image, catalog, object_sizes(symbols))
    roots = {}
    for pc, role in sorted(ROOTS.items()):
        owner = containing(list(spans), spans, pc)
        roots[address(pc)] = {"role": role, "function": address(owner) if owner is not None else None}
    return {
        "kind": "gf-retail-code-atlas", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "catalog_sha256": CATALOG_SHA256, "symbol_metadata_sha256": sha256(symbols).hexdigest(),
        "installer_table": address(INSTALLER_TABLE), "functions": functions,
        "borgs": borgs, "profiles": profiles, "battle_roots": roots,
        "counts": {"graph": graph_counts, "roster": roster_counts},
        "notes": [
            "Function bounds come from external symbol metadata, not an exhaustive source reconstruction.",
            "Transfer offsets are byte offsets from the function entry; indirect targets are not inferred.",
            "An omitted reached_bytes means the entry walk covered the supplied span; it is not execution proof.",
            "Decomp labels are reported snapshot status on exact address and size matches, not newly compiled here.",
            "Profile rows retain all static contexts and refusals; no runtime context or phase timing is proven.",
            "A missing recognized substate table does not imply that the family has no indirect dispatch.",
        ],
    }


def encode(payload):
    def compact(value):
        return json.dumps(value, sort_keys=True, separators=(",", ":"))

    def records(value):
        if isinstance(value, dict):
            return "{\n" + ",\n".join(f"    {compact(k)}: {compact(v)}"
                                       for k, v in sorted(value.items())) + "\n  }"
        return "[\n" + ",\n".join("    " + compact(v) for v in value) + "\n  ]"

    return ("{\n" + ",\n".join(f"  {compact(k)}: " +
                              (records(v) if k in ("functions", "profiles", "borgs") else compact(v))
                              for k, v in sorted(payload.items())) + "\n}\n").encode("ascii")


def export(dol, catalog, symbols, output, db=None):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("code atlas output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    payload = recover(image, read_catalog(catalog), read_bounded(symbols, MAX_SYMBOLS_BYTES), db)
    raw = encode(payload)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "bytes": len(raw), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "catalog", "symbols", "output"):
        parser.add_argument(arg, type=Path)
    parser.add_argument("--db", type=Path, help="optional local matching database snapshot")
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.catalog, args.symbols, args.output, args.db), sort_keys=True))
