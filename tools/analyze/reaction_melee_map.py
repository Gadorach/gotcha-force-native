"""Publish neutral close-B direct contact operands for reviewed GG4E08 pilots."""
import argparse
import json
import struct
from pathlib import Path

from tools.analyze import reaction_emit, reaction_installer_tables, reaction_melee, reaction_melee_continuation
from tools.analyze.retail_action_links import ActionLinker, direct_branch
from tools.analyze.retail_action_tables import action_candidates
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import BLOCK_LIMIT, RetailArchive, bank_clips, clip_timing, logical_joints, wrapper_model
from tools.extract.source_image import DolImage, ReadOnlyFile

# Exact neutral program addresses and installer config sets established by the
# existing Ninja, Knight, Girl, Samurai, Bomber and Wing pose auditors. Samples
# cover sibling steps, alternate opening programs, and two continuation chains.
PILOTS = {
    0x0001: (0x800FEEA4, (0x8031AAD4,), (0x8031AD70,)),
    0x0106: (0x801615F8, (0x8033525C,), (0x80335E30, 0x80335F48, 0x80336060)),
    0x0202: (0x80123EE8, (0x8032DDE8,), (0x8032E4B0,)),
    0x0209: (0x801B8318, (0x8034342A, 0x803436DA), (0x80382238,)),
    0x0301: (0x8010B478, (0x80322548,), (0x80322F40,)),
    0x0304: (0x8010B478, (0x80337636, 0x8038941A), (0x80336E40, 0x80389D48)),
    0x0700: (0x80148E90, (0x80307C72,), (0x80308150,)),
    0x0807: (0x800FEEA4, (0x80377E40,), (0x80377918,)),
    0x0A03: (0x80147F64, (0x8031DEA2,), (0x8031D4A0,)),
}
CONTINUATIONS = {
    0x0106: ("801315D4", ("80334E20",)),
    0x0301: ("801072FC", ("80322060",)),
    0x0304: ("80134604", ("803371E0", "80389120")),
}


def recover(image, archive):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    dispatch = 0x802D0DA0 + 10 * 4
    site = reaction_melee.ALLOCATOR_CALL
    handler, = struct.unpack(">I", image.read(dispatch, 4, "data"))
    call, = struct.unpack(">I", image.read(site, 4, "text"))
    if (handler != reaction_melee.OPCODE_HANDLER or
            call & 0xFC000003 != 0x48000001 or direct_branch(site, call) != 0x8008AC80):
        raise ValueError("opcode-10 contact path changed")
    pilots = {}
    for model, (request, addresses, cfg) in PILOTS.items():
        programs = [f"{address:08X}" for address in addresses]
        configs = [f"{address:08X}" for address in cfg]
        pose = {"request_site": f"{request:08X}", "request_mask": 15}
        payload = reaction_emit.qualified_hit_block(archive, model)
        installer, table = reaction_installer_tables.model_table(image, model)
        source = {"pose": pose, "program_addresses": programs}
        relay, roots = CONTINUATIONS.get(model, (None, ()))
        if relay:
            source["neutral_contact_relay"] = relay
            pose["program"] = [3, 0]
        binding, reason = reaction_melee.recover(image, payload, table, source, configs, roots)
        pilot = {"programs": programs, "configs": configs, **pose,
                 **({"binding": binding} if binding else {"unbound": reason})}
        if relay and binding:
            member, _ = archive.member(archive.names.index(f"pl{model:04x}.pzz".encode("ascii")), model)
            motion, _ = member.read(3, BLOCK_LIMIT)
            body, _ = member.read(4, BLOCK_LIMIT)
            prefix, hsd = wrapper_model(body)
            clips = bank_clips(motion)
            actions = action_candidates(image, installer, model)
            if (any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores"))
                    or {p["config"] for p in actions["profiles"]} != set(configs)):
                raise ValueError(f"{model:04x}: continuation pose installer scope changed")
            followup = reaction_melee_continuation.qualify_pose(
                ActionLinker(image, {}, ()), binding["continuation"], actions["profiles"], clips,
                clip_timing(motion, clips), len(logical_joints(prefix, hsd)), [1, 0], frame=6, duration=24)
            if followup is None:
                raise ValueError(f"{model:04x}: continuation pose no longer qualifies")
            pilot["continuation_pose"] = followup
        pilots[f"{model:04x}"] = pilot
    return {"kind": "gf-reaction-melee-map", "revision": 4, "source_dol_sha256": SOURCE_SHA256,
            "source": {"request": "8004BEB8", "config_mask_seed": "8004BCD8", "mask_intersect": "8004BEEC",
                       "dispatch_slot": f"{dispatch:08X}", "opcode_handler": f"{handler:08X}",
                       "allocator_call": f"{site:08X}"},
            "scope": "audited normal neutral close-B direct contacts; no live collision, damage, or timing proof",
            "pilots": pilots}


def export(dol, afs, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction melee map output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
    with Path(afs).open("rb") as stream:
        result = recover(image, RetailArchive(stream))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("afs", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.afs, args.output)
