"""Bind direct contacts in audited neutral close-B programs."""
import struct

from tools.analyze import reaction_emit, reaction_installer_tables, reaction_melee_continuation
from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_movement_tables import written_register

MOVE = "opening-slash"
OPCODE_HANDLER = 0x8004C6B4
ALLOCATOR_CALL = 0x8004C6F0
ENTRY_SETUP = {"wait", "select_descriptor", "upper_controls", "lower_controls",
               "lane2_controls", "lane3_controls", "pose_height"}


def request_lane(image, site, expected_mask):
    """Prove the request's lane mask in a small straight-line call window."""
    start = site - 64
    words = tuple(w for w, in struct.iter_unpack(">I", image.read(start, 68, "text")))
    call = words[-1]
    if call & 0xFC000003 != 0x48000001 or direct_branch(site, call) != 0x8004BEB8:
        return None
    for i in range(len(words) - 2, -1, -1):
        word = words[i]
        op = word >> 26
        if op in (38, 44, 48, 52) or (op == 31 and (word >> 1) & 1023 == 535) or (
                op == 59 and (word >> 1) & 31 == 25):
            # Non-updating stores, lfs/lfsx and fmuls do not write an argument GPR.
            continue
        try:
            if written_register(word) != 4:
                continue
        except ValueError:
            return None
        if (type(expected_mask) is int and 1 <= expected_mask <= 15 and expected_mask & 1
                and word == (0x38800000 | expected_mask)):
            return {"request_site": f"{site:08X}", "mask_site": f"{start + i*4:08X}",
                    "request_mask": expected_mask, "lane": 0}
        return None
    return None


def contact_events(program):
    """Enumerate direct opcode-10 arms; do not speculate through setup calls."""
    if program["status"] != "terminal_wait":
        return None
    routes = []
    for command in program["commands"]:
        kind = command["kind"]
        if kind == "actor_event_10":
            raw = bytes.fromhex(command["raw"])
            group = reaction_emit.signed_byte(raw[1])
            secondary = reaction_emit.signed_byte(raw[2])
            if group > 0:
                route = {"program": f"{program['address']:08X}",
                         "selector_address": f"{command['address'] + 1:08X}",
                         "queue_group": group}
                # The BB path runs after 8008AC80 and does not contribute
                # a selector. Record it as outside the direct-contact scope.
                if secondary > 0:
                    route["unmapped_secondary_state"] = secondary
                routes.append(route)
            elif secondary > 0 and not routes:
                return None
        elif not routes and kind not in ENTRY_SETUP:
            return None
    return routes or None


def recover(image, payload, table, source, configs, program_roots=()):
    programs = source.get("program_addresses", ())
    pose = source["pose"]
    if not 1 <= len(programs) <= 8:
        return None, "no bounded opening program set"
    request = request_lane(image, int(pose["request_site"], 16), pose["request_mask"])
    if request is None:
        return None, "request does not prove lane zero"
    # 8004BCD8 seeds actor+0x579 from config+2; 8004BEEC intersects
    # that byte with the request mask. Do not infer this for other states.
    if not 1 <= len(configs) <= 8:
        return None, "no bounded normal config set"
    masks = {config: image.read(int(config, 16) + 2, 1, "data")[0]
             for config in sorted(set(configs))}
    if not all(mask & 1 for mask in masks.values()):
        return None, "normal config does not admit lane zero"
    if table is None or len(payload) != reaction_emit.HIT_BYTES:
        return None, "no qualified owner reaction table or normal hit block"
    def contacts_for(address):
        return contact_events(decode_program(lambda at, size: image.read(at, size, "data"),
                              int(address, 16), max_commands=32, max_span=128))

    groups = [contacts_for(address) for address in sorted(set(programs))]
    continuation = None
    if any(events is None for events in groups):
        # Only a proved pose-only lead-in may use a later program. Never
        # replace a real opening contact just because its rows disagree.
        if not all(events is None for events in groups):
            return None, "opening program has no qualified direct contact"
        continuation = reaction_melee_continuation.recover(image, source, program_roots)
        if continuation is None:
            return None, "opening program has no qualified direct contact"
        request = request_lane(image, int(continuation["request_site"], 16), pose["request_mask"])
        if request is None:
            return None, "continuation request does not prove lane zero"
        groups = []
        for address in sorted({route["first_contact_program"] for route in continuation["programs"]}):
            events = contacts_for(address)
            if not events or events[0]["selector_address"] != f"{int(address, 16) + 1:08X}":
                return None, "continuation lacks a qualified entry contact"
            groups.append(events)
    contacts = []
    for events in groups:
        for event in events:
            steps = reaction_emit.resolve_group(image, payload, table, event["queue_group"])
            if steps is None:
                return None, "opening contact has no reviewed reaction steps"
            contacts.append({**event, "steps": steps})
    lines = reaction_emit.uniform_reaction_lines(MOVE, contacts, root="melee")
    if not lines:
        return None, "opening contacts disagree on mode or offset"
    if continuation:
        lines.insert(0, "# close-B reaction: first proved continuation; native timing is authored")
    return {**request, "config_masks": masks, "table": f"{table:08X}",
            "opcode_handler": f"{OPCODE_HANDLER:08X}",
            "allocator_call": f"{ALLOCATOR_CALL:08X}", "contacts": contacts, "lines": lines,
            **({"continuation": continuation} if continuation else {})}, None


def append_binding(image, payload, model, gameplay, receipt, source, profiles):
    if not any(line.startswith(b"move=opening-slash|") for line in gameplay.splitlines()):
        raise ValueError("close-B reaction binding references undeclared move")
    _, table = reaction_installer_tables.model_table(image, model)
    binding, reason = recover(image, payload, table, source,
        [profile["config"] for profile in profiles],
        [profile["program_and_descriptor_roots"][0] for profile in profiles])
    if binding is None:
        receipt["reaction_unbound_moves"] = [MOVE]
        receipt["reaction_refusals"] = {MOVE: reason}
        return gameplay
    receipt.setdefault("reaction", {})[MOVE] = binding
    return gameplay + ("\n".join(binding["lines"]) + "\n").encode("ascii")
