"""Recover the first direct close-B program after an audited neutral lead-in."""
import hashlib
import struct

from tools.analyze.gred_animation_controller import decode_program, signed_table
from tools.analyze.retail_action_links import direct_branch

# Opening request -> dispatcher, phase table, first three phases,
# continuation request, neutral relay call offset, exact dispatcher window.
_CHAINS = {
    0x8010B478: (0x8010B2F4, 0x80325A68, (0x8010B344, 0x8010B400, 0x8010B51C),
                 0x8010B5E0, 16, 0x8010B610,
                 "cc32dd902e91499c06284a88fee5e2e0170a4b089d9499c031b34a4f5973ceac"),
    0x801615F8: (0x801613FC, 0x8034C6A8, (0x8016144C, 0x80161508, 0x80161610),
                 0x801616D4, 28, 0x80161704,
                 "f94bdcf0fa85a0b636aa35d1b90ad9cd071bafecc295ce67054027b77a603883"),
}


def _word(image, at):
    return struct.unpack(">I", image.read(at, 4, "text"))[0]


def is_lead_in(program):
    # The auxiliary handler only changes a lane flag at actor+0x709.
    # No contact, class, branch or input opcode is allowed in this proof.
    return (program["status"] == "terminal_wait" and bool(program["commands"]) and
            all(command["opcode"] in (0, 1, 2, 3, 11) for command in program["commands"]))


def recover(image, source, program_roots):
    """Qualify the first continuation, not the timer or any later combo arm."""
    pose = source["pose"]
    opening = int(pose["request_site"], 16)
    relay = source.get("neutral_contact_relay")
    if (opening not in _CHAINS or not relay or pose.get("program") != [3, 0]
            or not 1 <= len(program_roots) <= 8):
        return None
    relay = int(relay, 16)
    dispatcher, table, phases, request, call_offset, end, digest = _CHAINS[opening]
    # These pinned paths copy the relay's zero to actor+0x6EA, request the old
    # signed byte from group 3, then increment it and phase +0x540. The first
    # continuation therefore requests seed 1 when its source guard is taken.
    if (hashlib.sha256(image.read(dispatcher, end - dispatcher, "text")).hexdigest() != digest
            or struct.unpack(">3I", image.read(table, 12, "data")) != phases
            or _word(image, relay + 8) != 0x38800000):
        return None
    site = relay + call_offset
    # The Bomber relay also halves its signed +0x18DA operand, without
    # changing r4. The other reviewed relay goes directly to dispatch.
    before_call = "90010014" if call_offset == 16 else "90010014a80318da7c000e70b00318da"
    if image.read(relay + 12, call_offset - 12, "text") != bytes.fromhex(before_call):
        return None
    call = _word(image, site)
    if call & 0xFC000003 != 0x48000001 or direct_branch(site, call) != dispatcher:
        return None
    if (struct.unpack(">I", image.read(0x802D0DCC, 4, "data")) != (0x8004C75C,)
            or hashlib.sha256(image.read(0x8004C75C, 0x48, "text")).hexdigest()
            != "8c40becf06bbe24019e2929e905fb07c9acec39387d626f4f60112c9de6f5c14"):
        return None
    read = lambda address, size: image.read(address, size, "data")
    programs = []
    for root in sorted(set(program_roots)):
        groups = signed_table(read, int(root, 16), 16)
        if len(groups) <= 3:
            return None
        choices = signed_table(read, groups[3])
        if len(choices) < 2:
            return None
        before, after = choices[:2]
        if not is_lead_in(decode_program(read, before, max_commands=16, max_span=96)):
            return None
        programs.append({"root": root, "lead_in": f"{before:08X}", "first_contact_program": f"{after:08X}"})
    if {row["lead_in"] for row in programs} != set(source.get("program_addresses", ())):
        return None
    return {"relay": f"{relay:08X}", "dispatcher": f"{dispatcher:08X}",
            "phase_table": f"{table:08X}", "opening_request": f"{opening:08X}",
            "phase": 2, "group": 3, "seed": 1, "request_site": f"{request:08X}", "programs": programs}


def qualify_pose(linker, continuation, profiles, slots, timing, joint_count, lead_clip, *, frame, duration):
    """Bind the first continuation clip; its switch frame is native authored."""
    if not 1 <= frame < duration <= 600 or not 1 <= len(profiles) <= 8:
        return None
    group, seed = continuation["group"], continuation["seed"]
    programs = {row["root"]: int(row["first_contact_program"], 16) for row in continuation["programs"]}
    if set(programs) != {p["program_and_descriptor_roots"][0] for p in profiles}:
        return None
    local = {(g, c): offset for g, c, offset in slots}
    candidates = set()
    for profile in profiles:
        raw = profile["program_and_descriptor_roots"]
        roots = tuple(int(addr, 16) for addr in raw)
        if linker.table_choice(roots[0], group, seed) != programs[raw[0]]:
            return None
        choice = linker.program_candidate(roots, local, (15, group, seed))
        if choice["program_status"] != "terminal_wait" or len(choice["clip_candidates"]) != 1:
            return None
        (selected,) = choice["clip_candidates"]
        if (selected["command_index"], selected["preceding_waits"]) != (1, 0):
            return None
        lanes = [lane for lane in selected["lanes"] if lane["lane"] in (0, 1)]
        if [lane["lane"] for lane in lanes] != [0, 1] or any(lane["bank"] != "local" for lane in lanes):
            return None
        for lane in lanes:
            candidates.add((lane["group"], lane["choice"], lane["archive_offset"],
                            selected["descriptor_choice"], selected["start_frame"]))
    if len(candidates) != 1:
        return None
    (g, c, offset, descriptor, start), = candidates
    if offset is None or (g, c) == tuple(lead_clip) or not 0 <= start <= 600:
        return None
    metadata = {row["archive_offset"]: row for row in timing}
    clip = metadata.get(offset)
    lead = metadata.get(local.get(tuple(lead_clip)))
    if not clip or not lead or clip["aobj_timing"] != "indexed" or clip.get("strict_metadata_refusals"):
        return None
    # Compare to this model's proven lead-in public binding, not a
    # cross-model joint-count guess. Some rigs have unanimated appendages.
    public = clip["public_joint_count"]
    ends = clip["authored_end_frames"]
    if (not 0 < public == lead["public_joint_count"] <= joint_count or not clip["animated_joint_count"] or
            len(ends) != 1 or start + duration - 1 - frame > ends[0]):
        return None
    return {"switch_frame": frame, "clip": [g, c], "source_start_frame": start,
            "source_program": [group, seed], "descriptor_choice": descriptor, "archive_offset": offset,
            "clip_end_frame": ends[0], "public_joint_count": public,
            "policy": "authored switch three ticks before native melee arming; not source phase timing"}
