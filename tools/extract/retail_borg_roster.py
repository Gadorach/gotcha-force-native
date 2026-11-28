"""Extract the GG4E08 normal roster, without running original game code."""
import argparse
from hashlib import sha256
from io import BytesIO
import json
import math
from pathlib import Path
import re
import struct
import unicodedata
from subprocess import CalledProcessError, TimeoutExpired, run
from tempfile import TemporaryDirectory
from tools.analyze.retail_action_tables import action_candidates
from tools.analyze.retail_dragon_carrier import COLLISION_BYTES, carrier_index as dragon_carrier_index, collision_index as dragon_collision_index
from tools.analyze import retail_dragon_art as dragon_art
from tools.analyze.retail_dragon_shot import trailing_chain as dragon_trailing_chain, translation_scale as dragon_translation_scale
from tools.analyze.retail_dragon_area import COMMON_COLLISION_SIZE, area_index as dragon_area_index, common_collision
from tools.analyze.retail_action_sfx import DIRECT, action_sfx_index

RELAY_SOUND_ID = 162
from tools.analyze.retail_actor_data_binding import actor_record_binding, binding_schema
from tools.analyze import (
    contact_impact_visuals, contact_pose_pause, contact_sound, damage_amount_model, damage_attacker_ladders, damage_contact_indices,
    damage_contact_power, damage_contact_roles,
    damage_contact_tiers, damage_content_row,
    damage_receiver_ladders, reaction_emit, reaction_melee, reaction_melee_continuation,
)
from tools.analyze.retail_basic_shot_test import (
    ACTOR_RELEASE_SOUND, ANIMATED_CRUISE_SHOTS, CLOCKED_GROUND_SHOTS, COHORT as BASIC_SHOT_COHORT, CRUISE_CHARGE_SHOTS,
    PREFERRED_B_SHOT, STATIC_CRUISE_PHASE_SHOTS, STATIC_CRUISE_SHOTS, STATIC_GROUND_MODELS, basic_shot_test, cruise_charge_shot_test, pilot_launch_tone,
    VIOLET_SHOT, dragon_battle_test, pose_free_shot_test, shot_factory_keys, worm_battle_test,
)
from tools.analyze.retail_worm_shot import attach_worm_pose, worm_source_index
from tools.analyze.retail_battle_reflection import zero_reflection_rules
from tools.analyze import retail_knight_melee as knight
from tools.analyze import retail_samurai_melee as samurai
from tools.analyze import retail_ninja_melee as ninja
from tools.analyze import retail_girl_melee as girl
from tools.analyze import retail_blue_close_b as blue
from tools.analyze import retail_robot_diver_melee as robots
from tools.analyze import retail_wing_melee as wings
from tools.analyze import retail_bomber_hero_melee as bombers
from tools.analyze import retail_grounded_close_b as grounded
from tools.analyze import retail_tank_shot as tanks
from tools.analyze import retail_paired_ground_shot as ground_pairs
from tools.analyze import retail_single_ground_shot as single_shots
from tools.analyze import retail_breath_dragon as breath
from tools.analyze import retail_roach_bomb as roach
from tools.analyze import retail_zfield_shot as zfield
from tools.analyze import retail_charge_field as charge_field
from tools.analyze import retail_icbm as icbm
from tools.analyze import retail_spawner_shot as spawner_shot
from tools.analyze import retail_spawner_static as spawner_static
from tools.analyze import retail_flying_boss as flying_boss
from tools.analyze import retail_nu_iii as nu_iii
from tools.analyze import retail_staged_field as staged_field
from tools.analyze import retail_cannon_beam as cannon_beam
from tools.analyze import retail_equipment_visuals as equipment
from tools.analyze import retail_gatling_held_fire as gatling
from tools.extract.retail_shot_pcm import decode_shot_audio
from tools.analyze.retail_action_links import ActionLinker
from tools.analyze.retail_action_previews import action_preview_candidates
from tools.analyze.retail_motion_roles import DASH_DIRECTIONS, air_dash_subranges, dragon_takeoff_clip, motion_roles
from tools.analyze.retail_movement_tables import movement_config_candidates
from tools.analyze.retail_projectile_sfx import attach_release_sfx, normal_tone_banks, projectile_sound_index, tone_candidate
from tools.analyze.retail_program_controls import ProgramControlIndex, control_write_schema
from tools.analyze.retail_stock_operands import LEVEL_TABLE, NORMAL_LEVEL_START, StockGuardIndex, attach_release_stock, stock_rows, stock_schema
from tools.analyze.retail_shot_readiness import shot_readiness
from tools.analyze.retail_movement_operands import ACTOR_BYTES, movement_operand_profile, movement_operand_schema
from tools.analyze.retail_ground_pace import cruise_pace_candidate, ground_pace_candidate
from tools.analyze.retail_projectile_origins import origin_candidates, origin_tables, preview_origin_markers, release_index
from tools.analyze import retail_projectile_continuation as projectile_continuation
from tools.analyze import retail_projectile_contact_tail as projectile_contact_tail
from tools.analyze import retail_sasuke_air_point_policy as sasuke_air_point_policy
from tools.analyze import retail_blue_striker_boost as blue_striker_boost
from tools.analyze import retail_ko_force_cost as ko_force_cost
from tools.analyze import retail_projectile_speed_ramp as projectile_speed_ramp
from tools.analyze import retail_ninja_bomb_test as ninja_bomb

from tools.extract.pzz import PzzArchive
from tools.extract.borg_movement_sfx import BANK as COMMON_MOVEMENT_BANK
from tools.extract.retail_borg_placeholders import open_regular, package_text, read_catalog
from tools.extract.source_image import DolImage


DOL_DIGEST = "21454a8880c6254616662710d6ec626e43d853999da2cdfad700030960a94836"
AFS_COUNT = 2857
MEMBER_LIMIT = 2 << 20
BLOCK_LIMIT = 16 << 20
ROSTER_LIMIT = 1 << 30
SPEC_TEXT_DIGEST = "a9ad2b42cf522ced919c132e1741c9b459732530a32321594e69e146f646cc17"
LANDING_PCM_COMMANDS = {15:"landing",18:"landing-18",22:"landing-22"}


def word(data, at):
    if not 0 <= at <= len(data) - 4:
        raise ValueError("word outside block")
    return struct.unpack_from(">I", data, at)[0]


class RangeFile:
    def __init__(self, stream, offset, size):
        self.stream, self.offset, self.size = stream, offset, size

    def read(self, at, size):
        if not 0 <= at <= self.size or not 0 <= size <= self.size - at:
            raise ValueError("read outside input")
        self.stream.seek(self.offset + at)
        data = self.stream.read(size)
        if len(data) != size:
            raise ValueError("short input read")
        return data


class RetailArchive:
    def __init__(self, stream):
        self.source = RangeFile(stream, 0, stream.seek(0, 2))
        if struct.unpack("<4sI", self.source.read(0, 8)) != (b"AFS\0", AFS_COUNT):
            raise ValueError("unexpected GG4E08 AFS header")
        self.pairs = tuple(struct.iter_unpack("<II", self.source.read(8, AFS_COUNT * 8)))
        self.first = min((p for p, n in self.pairs if n), default=0)
        if self.first < 16 + AFS_COUNT * 8:
            raise ValueError("invalid AFS table extent")
        self.directory, size = struct.unpack("<II", self.source.read(self.first - 8, 8))
        if self.directory < self.first or size != AFS_COUNT * 48:
            raise ValueError("invalid AFS name table")
        names = self.source.read(self.directory, size)
        self.names = tuple(names[i:i+32].split(b"\0", 1)[0] for i in range(0, size, 48))

    def payload(self, index, limit):
        offset, size = self.pairs[index]
        if not 0 < size <= limit or not self.first <= offset < offset + size <= self.directory:
            raise ValueError(f"invalid AFS member extent: {index}")
        return self.source.read(offset, size)

    def member(self, index, model):
        name = f"pl{model:04x}.pzz".encode("ascii")
        if not 0 <= index < AFS_COUNT or self.names[index] != name or self.names.count(name) != 1:
            raise ValueError(f"battle member/name mismatch: {index} / {name!r}")
        data = self.payload(index, MEMBER_LIMIT)
        return PzzArchive(RangeFile(BytesIO(data), 0, len(data))), sha256(data).hexdigest()

    def shared_assets(self):
        """Export shared bank containers without inferring cue or controller use."""
        banks, files = {}, {}
        for index, name in enumerate(self.names):
            stem, _, suffix = name.rpartition(b".")
            if suffix not in (b"tsb", b"chd", b"dpk"):
                continue
            if not re.fullmatch(rb"snd_com0[1-4]", stem):
                raise ValueError("unexpected GG4E08 sound bank name")
            members = banks.setdefault(stem.decode("ascii"), {})
            role = suffix.decode("ascii")
            if role in members:
                raise ValueError("duplicate sound bank role")
            data = self.payload(index, 8 << 20)
            path = "shared-" + name.decode("ascii")
            files[path] = data
            members[role] = {"member": index, "path": path, "bytes": len(data),
                             "sha256": sha256(data).hexdigest()}
        if len(banks) != 4 or any(set(members) != {"tsb", "chd", "dpk"} for members in banks.values()):
            raise ValueError("incomplete sound bank")
        common = 696
        if self.names[common] != b"cmn_data.pzz" or self.names.count(self.names[common]) != 1:
            raise ValueError("common motion member mismatch")
        data = self.payload(common, MEMBER_LIMIT)
        if sha256(data).hexdigest() != "21b0994e76f97435e2c1bd7e34c39d7a33ec19a8f1d0d4ebc9207bf697448c1e":
            raise ValueError("common motion member digest mismatch")
        archive=PzzArchive(RangeFile(BytesIO(data),0,len(data)))
        motion,_=archive.read(2,BLOCK_LIMIT)
        collision,_=archive.read(0,COMMON_COLLISION_SIZE)
        path = "shared-common.motions"
        files[path] = motion
        files["shared-common-collision.bin"]=collision
        return files, {"audio_bank_members": banks, "audio_samples_decoded": False,
                       "common_collision":{"member":common,"block":0,"path":"shared-common-collision.bin",
                                           "sha256":sha256(collision).hexdigest()},
                       "common_motion": {"member": common, "block": 2, "path": path,
                                         "sha256": sha256(motion).hexdigest(),
                                         "raw_animation_slots": bank_clips(motion)}}


def bank_clips(data):
    """Return existing raw slots only; never treat a null slot as the default clip."""
    if len(data) < 64:
        raise ValueError("short motion bank")
    result = []
    archive_ranges = {}
    tables = {(0, 64)}
    for group in range(16):
        table = word(data, group * 4)
        if not table:
            continue
        if table < 64 or table % 4:
            raise ValueError("invalid motion group table")
        for choice in range(257):
            offset = word(data, table + choice * 4)
            if offset == 0xFFFFFFFF:
                tables.add((table, table + (choice + 1) * 4))
                break
            if choice == 256:
                raise ValueError("unterminated motion table")
            if not offset:
                continue
            if offset not in archive_ranges:
                size = word(data, offset)
                if offset < 64 or offset % 4 or not 32 <= size <= len(data) - offset:
                    raise ValueError("invalid motion archive extent")
                archive_ranges[offset] = offset + size
            result.append((group, choice, offset))
    intervals = sorted((*tables, *archive_ranges.items()))
    if any(end > start for (_, end), (start, _) in zip(intervals, intervals[1:])):
        raise ValueError("overlapping motion bank ranges")
    return result


def clip_timing(bank, slots):
    """Index authored AObj time and loop flags once per physical archive."""
    results = []
    for at in sorted({offset for _, _, offset in slots}):
        blob = memoryview(bank)[at:at + word(bank, at)]
        data_bytes, count, publics, externs = (word(blob, i) for i in (4, 8, 12, 16))
        relocs = 32 + data_bytes
        symbols = relocs + count * 4
        record = {"archive_offset": at, "public_joint_count": publics}
        if publics > 4096 or externs > 4096 or symbols + (publics + externs) * 8 > len(blob):
            record["aobj_timing"] = "unsupported archive tables"
            results.append(record)
            continue
        fields = {word(blob, relocs+4*i) for i in range(count)}
        data = blob[32:relocs]
        timed, strict = [], set()
        if externs:
            strict.add("extern-symbol-binding")
        for index in range(publics):
            node = word(blob, symbols+index*8)
            if node > len(data) - 20:
                break
            if word(data,node+12) or node+12 in fields or word(data,node+16)>1:
                strict.add("animjoint-constraints-or-flags")
            addr = word(data, node+8)
            if not addr and node + 8 not in fields:
                continue
            if node + 8 not in fields or addr > len(data) - 16 or addr % 4:
                break
            end = struct.unpack_from(">f", data, addr+4)[0]
            if not math.isfinite(end) or not 0 <= end <= 8192:
                break
            if word(data,addr):
                strict.add("aobj-flags")
            timed.append((word(data, addr), end))
        else:
            record["aobj_timing"] = "indexed"
            record["authored_end_frames"] = sorted({end for _, end in timed})
            record["aobj_flags"] = sorted({flags for flags, _ in timed})
            record["authored_loop_joint_count"] = sum(bool(flags & 0x20000000) for flags, _ in timed)
            record["animated_joint_count"] = len(timed)
            if len(record["authored_end_frames"])>1:
                strict.add("mixed-joint-end-times")
            if strict:
                record["strict_metadata_refusals"]=sorted(strict)
            results.append(record)
            continue
        record["aobj_timing"] = "unsupported joint timing descriptor"
        results.append(record)
    return results


def wrapper_model(data):
    at = word(data, 0)
    if not 12 <= at <= min(65536, len(data) - 32) or at % 4:
        raise ValueError("invalid model wrapper prefix")
    size = word(data, at)
    if not 32 <= size <= len(data) - at:
        raise ValueError("invalid model archive extent")
    return data[:at], data[at:at+size]


def logical_joints(prefix, model):
    """Map the authored record-zero rig. A joint is not yet a launch socket."""
    records, order = word(prefix, 4), word(prefix, 8)
    data_size, fields, publics, externs = (word(model, i) for i in (4, 8, 12, 16))
    relocs = 32 + data_size
    symbols = relocs + fields * 4
    strings = symbols + (publics + externs) * 8
    if publics > 4096 or externs > 4096 or strings > len(model):
        raise ValueError("joint archive table bounds")
    reloc = {word(model, relocs + 4 * i) for i in range(fields)}
    if len(reloc) != fields:
        raise ValueError("duplicate joint relocation")
    data = memoryview(model)[32:relocs]

    def ptr(at, *, required=False):
        value = word(data, at)
        if not required and value in (0, 0xFFFFFFFF) and at not in reloc:
            return None
        if (not required and not value) or at not in reloc or value >= len(data) or value % 4:
            raise ValueError("unsupported joint relocation")
        return value

    scenes = []
    for i in range(publics):
        at = symbols + 8 * i
        start = strings + word(model, at + 4)
        end = model.find(b"\0", start, start + 257)
        if end < start:
            raise ValueError("joint symbol extent")
        if model[start:end] == b"scene_data":
            scenes.append(word(model, at))
    if len(scenes) != 1:
        raise ValueError("missing or ambiguous scene_data")
    table = ptr(scenes[0])
    record = ptr(table) if table is not None else None
    root = ptr(record, required=True) if record is not None else None
    if root is None or not 12 <= records < len(prefix) or records % 4 or not 12 <= order < len(prefix):
        raise ValueError("missing record-zero rig")
    todo, seen, nodes = [(root, None)], set(), []
    while todo:
        at, parent = todo.pop()
        if at in seen or len(nodes) >= 32 or order + len(nodes) >= len(prefix) or at + 64 > len(data):
            raise ValueError("joint topology bounds or shared identity")
        if ptr(at) is not None or word(data, at+4) & 0x1000:
            raise ValueError("joint class or instance requires separate binding")
        child, sibling = ptr(at+8), ptr(at+12)
        translation = struct.unpack_from(">3f", data, at+0x2C)
        if not all(map(math.isfinite, translation)):
            raise ValueError("non-finite joint translation")
        nodes.append((prefix[order + len(nodes)], at, parent, child, sibling, translation))
        seen.add(at)
        if sibling is not None:
            todo.append((sibling, parent))
        if child is not None:
            todo.append((child, at))
    count = len(nodes)
    if root == 0 and nodes[0][0] != 0:
        raise ValueError("block-zero root must bind logical slot zero")
    if (set(n[0] for n in nodes) != set(range(count)) or records + 4*count > len(prefix)
            or records < order + count and order < records + 4*count):
        raise ValueError("logical joint order or record extent")
    slots = {n[1]: n[0] for n in nodes}
    result = []
    for slot, at, parent, child, sibling, translation in sorted(nodes):
        links = tuple(slots.get(p, 255) for p in (child, sibling, parent))
        if tuple(prefix[records+4*slot:records+4*slot+3]) != links:
            raise ValueError("wrapper links disagree with the declared joint tree")
        result.append({"index": slot, "descriptor_offset": at, "child_next_parent": links,
                       "local_translation": translation})
    return result


def source_table(dol, table, model, stride=4):
    base, = struct.unpack(">I", dol.read(table + 4 * (model >> 8), 4))
    return base + stride * (model & 255)


def source_parts(dol, model, spec, joints):
    """Read source model-part rows, without guessing visibility or style."""
    pointer, = struct.unpack(">I", dol.read(source_table(dol, 0x802D2564, model), 4))
    rows = []
    for index in range(33):
        values = tuple(dol.read(pointer + index * 4, 4))
        if values == (255, 0, 0, 0):
            break
        record, joint, selector, operand = values
        if index == 32 or record >= 32 or joint >= 32 or selector not in (*range(8), 255) or operand:
            raise ValueError("unsupported model-part table row")
        rows.append(values)
    defaults = list(spec[0xBE:0xC0])
    # The lane-default command reads these two bytes for selector kinds 0/1.
    # Only the proven mapping (0->7, 1->11) is interpreted; retain all raw rows.
    choices = []
    for kind, joint in ((0, 7), (1, 11)):
        groups = [r for r in rows if r[2] == kind and r[1] == joint]
        if len(groups) != 1:
            continue
        group_start = rows.index(groups[0])
        records = [groups[0][0]]
        for rid, bound, selector, _ in rows[group_start+1:]:
            if bound != joint or selector != 255:
                break
            records.append(rid)
        if (records != list(range(records[0], records[0]+len(records))) or
                not joints or joint >= len(joints) or defaults[kind] >= len(records)):
            continue
        choices.append({"selector": kind, "default": defaults[kind], "joint": joint,
                        "record": records[defaults[kind]], "group_records": records})
    return {"raw_rows": rows, "raw_default_selectors": defaults, "default_candidates": choices,
             "runtime_visibility_resolved": False}


def lane_event_index(actions, linker):
    """Descriptor lane events per resolved entry clip.

    Reads only what the row already recorded (request mask and program group /
    choice) and keeps the event and flags bytes that the export otherwise drops.
    """
    out = []
    for profile in actions.get("profiles") or []:
        roots = tuple(int(root, 16) for root in profile.get("program_and_descriptor_roots") or [])
        if len(roots) < 4:
            continue
        for group in profile.get("groups") or []:
            for row in group.get("rows") or []:
                entry = row.get("entry_clip_candidate") or {}
                mask = entry.get("request_mask")
                program = entry.get("program") or ()
                if not isinstance(mask, int) or len(program) != 2:
                    continue
                lanes = linker.lane_events(roots, mask, program[0], program[1])
                if lanes:
                    out.append({"action_group": group.get("group"),
                                "state": row.get("state"), "selector": row.get("selector"),
                                "program": {"mask": mask, "group": program[0], "choice": program[1]},
                                "lanes": lanes})
    return out


def neutral_preview_mounts(dol, model, installer):
    """Source operands for a neutral inspection composite, not actor rules."""
    if installer != 0x80123438 or model not in (0x0202, 0x020B):
        return ()
    # Both initializers allocate eight children via 801235A4. The child
    # setup selects the same model-record pairs for the two family members;
    # 8012BCF4/8012BD18 consume the mount joint and the local translation.
    for address, opcode in ((0x80123568,0x901D04D4),(0x801235A4,0x48007FF9),
                            (0x8012B848,0x4BEDB7E9),(0x8012B8D8,0x4BEDB759),
                            (0x8012BCF4,0x7C7C1A14),(0x8012BD18,0x480DFA91)):
        if word(dol.read(address, 4), 0)!=opcode:
            raise ValueError("neutral part assembly source signature")
    records = dol.read(0x803327C8+(16 if model==0x020B else 0), 16)
    bones = dol.read(0x803327E8, 16)
    offsets = list(struct.iter_unpack(">3f", dol.read(0x803327F8, 16*12)))
    weights = list(struct.iter_unpack(">2f", dol.read(0x803328B8, 8*8)))
    if len(records)!=16 or len(bones)!=16 or len(offsets)!=16 or len(weights)!=8:
        raise ValueError("neutral part table extent")
    mounts = []
    for component in range(8):
        for side in range(2):
            index=2*component+side
            record,bone,offset=records[index],bones[index],offsets[index]
            if record==bone==255 and side==1:
                continue
            if (not 0<record<32 or bone>=32 or any((record,bone)==m[:2] for m in mounts) or
                    not math.isfinite(weights[component][side]) or not 0<=weights[component][side]<1 or
                    any(not math.isfinite(x) or abs(x)>4096 or x!=int(x) for x in offset)):
                raise ValueError("unqualified neutral part mount")
            mounts.append((record,bone,*map(int,offset)))
    return tuple(mounts)


def validate_preview_mounts(model, joints, mounts):
    if not mounts:
        return
    # Use the published table of this exact wrapper. The engine separately
    # validates full geometry, skin, hierarchy and each selected material.
    size, fields, publics, externs = (word(model, i) for i in (4, 8, 12, 16))
    end=32+size
    symbols=end+fields*4
    strings=symbols+(publics+externs)*8
    if strings>len(model) or publics>4096 or externs>4096:
        raise ValueError("mounted model symbol bounds")
    relocations={word(model, end+i*4) for i in range(fields)}
    data=memoryview(model)[32:end]
    def ptr(at):
        target=word(data, at)
        if at not in relocations or target>=size or target%4:
            raise ValueError("mount table relocation")
        return target
    exported=[word(model,symbols+i*8) for i in range(publics)
              if model.startswith(b"scene_data\0",strings+word(model,symbols+i*8+4))]
    if len(exported)!=1:
        raise ValueError("missing mounted scene_data")
    table=ptr(exported[0])
    for index in range(max(m[0] for m in mounts)+1):
        desc=ptr(table+index*4)
        if not desc:
            raise ValueError("mounted record table extent")
        if not any(m[0]==index for m in mounts):
            continue
        if any(m[0]==index and m[1]>=len(joints) for m in mounts) or not ptr(desc) or \
                any(word(data, desc+i) or desc+i in relocations for i in (4,8,12)):
            raise ValueError("missing mount joint or nonstatic mounted record")


def levels(dol, model):
    address = source_table(dol, LEVEL_TABLE, model, 20 * 18)
    start = dol.read(NORMAL_LEVEL_START, 1)[0]
    if start + 10 > 20:
        raise ValueError("unknown normal level-table selector")
    return list(struct.iter_unpack(">9h", dol.read(address + start * 18, 180)))


def landing_sound(dol, model):
    """The shared guarded landing arm; not an assertion that a Borg runs it."""
    route = dol.read(source_table(dol, 0x8031B7F4, model, 1), 1)[0]
    tone, = struct.unpack(">h", dol.read(0x80434410 + (route & 127) * 2, 2))
    if not 0 <= tone < 128:
        raise ValueError("unexpected landing tone outside common bank")
    return {"bank": "snd_com01", "tone": tone, "source_selector": route,
            "fixed_gain_dispatch": bool(route & 128), "trigger_qualified": False}


def decode_lab_landing_pcm(decoder, files, shared_index):
    decoder=Path(decoder).resolve(strict=True)
    if not decoder.is_file():
        raise ValueError("landing PCM decoder must be a local executable")
    with TemporaryDirectory(prefix="gf-roster-landing-") as scratch:
        paths=[]
        for member,name,size,digest in COMMON_MOVEMENT_BANK:
            path=f"shared-{name}"
            role=name.rsplit(".",1)[1]
            binding=shared_index["audio_bank_members"]["snd_com01"][role]
            data=files[path]
            if (binding["member"],len(data),sha256(data).hexdigest())!=(member,size,digest):
                raise ValueError("landing PCM source bank identity")
            dest=Path(scratch)/name
            with dest.open("xb") as stream:
                stream.write(data)
            paths.append(dest)
        result={}
        for tone,command in LANDING_PCM_COMMANDS.items():
            # Read at most the output budget into Python. The offline helper
            # already bounds decoding before it writes the PCM.
            dest=Path(scratch)/f"tone-{tone}.pcm"
            try:
                with dest.open("xb") as stream:
                    run([decoder,*paths,command],stdout=stream,check=True,timeout=10)
            except (CalledProcessError,TimeoutExpired) as exc:
                raise ValueError(f"landing PCM decoder refused tone {tone}") from exc
            with dest.open("rb") as stream:
                pcm=stream.read(48000*4*3+1)
            if not pcm or len(pcm)>48000*4*3 or len(pcm)%4 or not any(pcm):
                raise ValueError(f"landing PCM budget or empty sample: tone {tone}")
            result[tone]=pcm
    return result


def decode_move_labels(data):
    """Read the move group of the existing bounded GFTX display-text export."""
    if not 12 <= len(data) <= 1 << 20 or struct.unpack_from(">4sIHH", data) != (b"GFTX", 1, 2, 0):
        raise ValueError("unexpected specification text header")
    cursor = 12

    def take(size):
        nonlocal cursor
        if size > len(data) - cursor:
            raise ValueError("truncated specification text")
        result = data[cursor:cursor+size]
        cursor += size
        return result

    def half():
        return int.from_bytes(take(2), "big")

    names = []
    for group, expected in ((0, 207), (1, 1030)):
        if half() != expected:
            raise ValueError("unexpected specification text row count")
        for _ in range(expected):
            count = half()
            if not 1 <= count <= 32:
                raise ValueError("specification text line count")
            lines = []
            for __ in range(count):
                size = half()
                if size % 2 or size > 1024:
                    raise ValueError("specification text line extent")
                batch = take(size)
                if group == 1:
                    lines.append(unicodedata.normalize("NFKC", batch.decode("cp932")).strip())
            if group == 1:
                title = " ".join(line for line in lines if line)
                if title and not title.isprintable():
                    raise ValueError("invalid source move display label")
                names.append(title or None)
    if cursor != len(data):
        raise ValueError("trailing specification text")
    return names


def displayed_moves(dol, ordinal, spec, names):
    # 801DD450/54 locate the two five-slot display arrays. The display
    # button and indicator say nothing about engine arbitration or release.
    buttons = dol.read(0x80435B40, 5)
    indicators = dol.read(0x80435B48, 5)
    if any(b not in (0, 1) for b in buttons) or any(i > 3 for i in indicators):
        raise ValueError("unknown move display selector")
    rows = []
    for slot, icon in enumerate(spec[0x1A8:0x1AD]):
        if icon == 255:
            continue
        if icon > 26:
            raise ValueError("unknown move display icon")
        index = 5*ordinal + slot
        rows.append({"slot": slot, "button": ("B", "X")[buttons[slot]],
                     "display_indicator": indicators[slot], "icon": icon, "text_index": index,
                     "name": names[index] if names else None, "gameplay_role": "unresolved"})
    return rows


def move_profile(rows):
    if not rows or any(not row["name"] for row in rows):
        return None
    lines = ["GFBORG-MOVES 1", "# Authored display text and buttons only; no gameplay bindings."]
    for row in rows:
        name = row["name"]
        if len(name.encode("utf-8")) > 96 or "|" in name:
            raise ValueError("source move name does not fit package display field")
        lines.extend((
            f"move=display-slot-{row['slot']}|base|{row['button'].lower()}|unresolved|{name}",
            "contexts=unresolved", "fallback=unresolved", "ammo=unresolved", "refill=unresolved",
            "activation=unresolved", "guard=unresolved", "priority=unresolved",
            "replenishment=unresolved", "interrupt=unresolved",
            "phase=unresolved-phase|unresolved|unresolved|unresolved|unresolved|unresolved|unresolved|unresolved",
        ))
    return ("\n".join(lines) + "\n").encode("utf-8")


def preview_text(variant, prefix_path, motion, clips, extras=False, candidates=(), mounts=(), landing_pcm="", origins=(),
                 action_entries=(), cruise=(), ground_pace=None, cruise_entry=None, air_dash_starts=()):
    if motion and (not clips or clips[0][:2] != (0,0)):
        return None
    version = 9 if air_dash_starts else 8 if cruise_entry else 7 if ground_pace else 6 if cruise else 5 if origins else 4 if landing_pcm else 3 if mounts else 2
    body = (f"GFBORG-PREVIEW {version}\n"
            "model-format=hsd-whole-body-v1\n"
            f"variant={variant}\nrig-prefix={prefix_path}\n")
    if not motion:
        body += "idle=none\n"
    else:
        body += "# Preview starting clip, not a gameplay idle claim.\nidle=0:0\n"
        if extras:
            refs = [(g,c) for g,c,_ in clips[1:] if c<128][:7]
            body += "".join(f"clip=raw-{g}-{c}|{g}:{c}\n" for g,c in refs)
        else:
            body += "".join(f"candidate={role}|{g}:{c}\n" for role,g,c in candidates)
        if action_entries:
            body += "# Exact raw action-entry clip candidates; no playable move rules.\n"
            body += "".join(f"candidate={p['id']}|{p['clip'][0]}:{p['clip'][1]}\n" for p in action_entries)
    if mounts:
        body += "# Fixed neutral assembly; no original part-state updates.\n"
        body += "".join("mount="+"|".join(map(str,mount))+"\n" for mount in mounts)
    if landing_pcm:
        body += f"lab-landing-pcm={landing_pcm}\n"
    if origins:
        body += "# Source joint-local points for inspection only; not launch frames.\n"
        body += "".join(
            f"origin={ident}|{joint}|{x:.9g}|{y:.9g}|{z:.9g}\n" for ident,joint,x,y,z in origins)
    if cruise:
        body += "# Fixed native lab tuning, not retail flight parameters.\nlab-cruise="+"|".join(map(str,cruise))+"\n"
    if cruise_entry:
        body += f"candidate=cruise-entry|{cruise_entry[0]}:{cruise_entry[1]}\n"
    if ground_pace:
        body += "# Source-relative ground pace, not retail speed.\n"
        body += f"lab-ground-pace={ground_pace['native_units_per_tick_x100']}\n"
    if air_dash_starts:
        body += "lab-air-dash-starts="+"|".join(map(str,air_dash_starts))+"\n"
    return body.encode("ascii")


def role_preview_candidates(role_mapping):
    """One exact local lower-lane clip agreed by all reached static profiles."""
    roles = ("ground-travel","ground-takeoff","landing","ground-dash-entry",
             "air-continuation","air-dash-recovery","ground-stop","glide-entry")
    airborne = ("air-dash-wind-up","air-dash-active")
    profiles = role_mapping["profiles"] if role_mapping else []
    def exact(role, *, frame_zero=False):
        hits = []
        for profile in profiles:
            lower = [ (desc,lane)
                for entry in profile["roles"] if entry["role_hint"]==role
                for request in entry["program_candidates"] if request["handler_lane"]=="lower"
                for desc in request["clip_candidates"] if desc["preceding_waits"]==0
                for lane in desc["lanes"] if lane["lane"]==1]
            if frame_zero and any(desc["start_frame"]!=0 or lane["bank"]!="local" or lane["archive_offset"] is None
                                  for desc,lane in lower):
                return None
            hits.append({(lane["group"],lane["choice"]) for _,lane in lower
                         if lane["bank"]=="local" and lane["archive_offset"] is not None})
        if hits and len(hits[0])==1 and all(h==hits[0] for h in hits):
            group, choice = next(iter(hits[0]))
            if group<16 and choice<128:
                return role,group,choice
        return None

    result = [candidate for role in roles if (candidate:=exact(role)) is not None][:7]
    if not any(r[0]=="ground-takeoff" for r in result) and (dragon:=dragon_takeoff_clip(profiles)):
        result.append(("ground-takeoff",*dragon))
    air = [exact(role,frame_zero=True) for role in (*airborne,"air-dash-recovery")]
    if all(air):
        # Retain the existing recovery reference only if it also qualifies
        # for the complete exact frame-zero set.
        if air[-1] in result:
            result.extend(air[:2])
    if any(r[0]=="glide-entry" for r in result):
        hold = []
        for profile in profiles:
            if profile["movement_type"]!=2:
                break
            requests = [q for r in profile["roles"]
                        if r["role_hint"]=="glide-entry" and r["selector"]==18 and r["states"]==[24,24]
                        and r["handler_addresses"]==["80062614","8006266C"]
                        for q in r["program_candidates"] if q["handler_lane"]=="lower"]
            if len(requests)!=1:
                break
            q=requests[0]
            if (q["site"],q["program_group"],q["program_choice"],q["lanes"])!=("800626D8",0,34,2):
                break
            held=[(v,l) for v in q["clip_candidates"] if v["preceding_waits"]==1 for l in v["lanes"] if l["lane"]==1]
            if len(held)!=1:
                break
            v,l=held[0]
            if (v["command_index"]!=4 or v["start_frame"]!=0 or l["bank"]!="local" or l["archive_offset"] is None or
                    l["group"]>=16 or l["choice"]>=128):
                break
            hold.append((l["group"],l["choice"]))
        else:
            if hold and all(x==hold[0] for x in hold):
                result.append(("glide-held",*hold[0]))
    # All four directional choices must independently agree across every
    # profile. An authored directional alias is not a fallback.
    dash=[]
    for direction in DASH_DIRECTIONS:
        hits = [
            [(lane["group"],lane["choice"])
             for entry in profile["roles"] if entry["role_hint"]=="ground-dash-entry"
             for request in entry["program_candidates"]
             if request["handler_lane"]=="lower" and request.get("dash_direction")==direction
             for desc in request["clip_candidates"]
             if desc["preceding_waits"]==0 and desc["start_frame"]==0
             for lane in desc["lanes"]
             if lane["lane"]==1 and lane["bank"]=="local" and lane["archive_offset"] is not None]
            for profile in profiles
        ]
        if not hits or len(hits[0])!=1 or not all(hit==hits[0] for hit in hits):
            return result
        group,choice=hits[0][0]
        if not (group<16 and choice<128):
            return result
        dash.append((f"ground-dash-{direction}",group,choice))
    return result+dash


def variant_preview_files(wrappers, motion, clips, mounts=()):
    """Bind exact variant slot bytes, never the base model as a fallback."""
    files, variants = {}, []
    overrides = ["GFBORG-VARIANTS 1", "# Exact wrapper inspection only; no variant gameplay or acquisition."]
    for slot, blob, rec in wrappers:
        source_variant = slot-4
        ident = f"source-{source_variant}"
        folder = f"visual/{ident}"
        try:
            prefix, model = wrapper_model(blob)
            joints=logical_joints(prefix, model)
            validate_preview_mounts(model, joints, mounts)
        except ValueError as exc:
            rec["preview_status"] = str(exc)
            continue
        raw_preview = preview_text(ident, f"{folder}/body.prefix", motion, clips, mounts=mounts)
        if raw_preview is None:
            rec["preview_status"] = "no exact raw starting clip"
            continue
        files[f"{folder}/body.prefix"] = prefix
        files[f"{folder}/body.hsd"] = model
        files[f"{folder}/preview.txt"] = raw_preview
        rec.update(source_variant=source_variant, preview_status="declared-not-runtime-validated",
                   variant_id=ident, prefix_path=f"{folder}/body.prefix", model_path=f"{folder}/body.hsd")
        variants.append((source_variant, ident))
        overrides.extend((
            f"variant={ident}", f"model={folder}/body.hsd", f"animations={'inherit' if motion else 'none'}",
            f"preview={folder}/preview.txt", "textures=none", "materials=none", "stats=unresolved",
            "movement=unresolved", "moves=unresolved", "collision=unresolved", "effects=unresolved",
            "acquisition=unresolved", "hp-level-offset=unresolved", "ammo-level-offset=unresolved",
        ))
    if variants:
        files["variants.txt"] = ("\n".join(overrides)+"\n").encode("ascii")
    return files, variants


def cruise_lab_tuning(movement, roles, actions):
    """An exact type-3 cruise/boost selector pair on every path."""
    profiles=roles["profiles"] if roles else []
    if not profiles or movement["candidate_types"]!=[3] or any(
            actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")):
        return ()
    expected={
        ("cruise",27,(0,33),("8005F238","800633C8")),
        ("cruise-boost",28,(0,34),("8005F238","80063644")),
    }
    for p in profiles:
        observed={(r["role_hint"],r["selector"],tuple(r["states"]),tuple(r["handler_addresses"]))
                  for r in p["roles"] if not r["program_candidates"]}
        if p["movement_type"]!=3 or not expected<=observed:
            return ()
    # Shared authored cruise-boost schema: height, capacity, debit, refill,
    # delay, minimum. These are deliberate lab numbers, not extracted tune.
    return (240,240,12,12,8,4)


def cruise_lab_candidate(motion, movement, roles, actions):
    """Static body: do not treat an unbound motion bank as a static body."""
    return () if motion else cruise_lab_tuning(movement,roles,actions)


def cruise_entry_candidate(roles, timing, joint_count):
    """Same frame-zero local entry clip on body lanes 0, 1 and 2 in every profile."""
    clips={r["archive_offset"]:r for r in timing}
    bindings=[]
    for p in roles["profiles"]:
        rows=[r for r in p["roles"] if r["selector"]==0 and r["states"]==[1,1] and
              r["handler_addresses"]==["8005F23C","8005F2A4"]]
        if len(rows)!=1:
            return None
        requests=rows[0]["program_candidates"]
        for site,mask,indices in (("8005F284",13,(0,2,3)),("8005F2F8",2,(1,))):
            exact=[r for r in requests if r["site"]==site and (r["lanes"],r["program_group"],r["program_choice"])==(mask,0,0)
                   and r["program_status"]=="terminal_wait"]
            if len(exact)!=1:
                return None
            zero=[c for c in exact[0]["clip_candidates"] if c["preceding_waits"]==0]
            if len(zero)!=1 or zero[0]["start_frame"]!=0 or tuple(l["lane"] for l in zero[0]["lanes"])!=indices:
                return None
            for lane in zero[0]["lanes"]:
                if lane["lane"]==3:
                    if (lane["bank"],lane["group"],lane["choice"],lane["archive_offset"])!=("common",127,0,None):
                        return None
                    continue  # The whole-body lab does not execute the auxiliary common lane.
                offset=lane["archive_offset"]
                if lane["bank"]!="local" or offset not in clips:
                    return None
                bindings.append((lane["group"],lane["choice"],offset))
    if not bindings or len(set(bindings))!=1:
        return None
    g,c,offset=bindings[0]
    record=clips[offset]
    if (record["aobj_timing"]!="indexed" or record.get("strict_metadata_refusals") or
            record["public_joint_count"]!=joint_count or not record["animated_joint_count"]):
        return None
    return g,c


DAMAGE_SELECTORS = Path(__file__).resolve().parents[2] / "docs/api-map/borg-damage-selectors.json"
NORMAL_DAMAGE_RECORDS = DAMAGE_SELECTORS.with_name("gg4e08-normal-receiver-factors.json")
NORMAL_DAMAGE_RECORDS_SHA256 = "0af7ac6153cddbf852f43762755b8772883ec24c6c569e5ceb22d76747eea1b1"
NORMAL_SOUND_RECORDS = DAMAGE_SELECTORS.with_name("gg4e08-normal-receiver-sound.json")
NORMAL_SOUND_RECORDS_SHA256 = "2b65cf6710817596a6d52424dfbef1a1c9d4f4d37e24f4d6676bef3096fd9043"
CONTACT_SIDE_ROWS = Path(__file__).resolve().parents[2] / "docs/api-map/contact-side-rows.json"


def _damage_number(value):
    text = f"{value:.6f}".rstrip("0").rstrip(".")
    return text if "." in text else text + ".0"


def _normal_receiver_proofs(path, digest, kind, rows):
    with open_regular(path) as stream:
        raw = stream.read((128 << 10) + 1)
    if len(raw) > 128 << 10 or sha256(raw).hexdigest() != digest:
        raise ValueError(f"{kind}: normal receiver proof extent or identity")
    normal = json.loads(raw)
    if (normal.get("kind"), normal.get("revision"), normal.get("source_dol_sha256")) != (kind, 1, DOL_DIGEST):
        raise ValueError(f"{kind}: incorrect normal receiver proof")
    observed = {r["borg"]: r for r in normal["records"]}
    if len(normal["records"]) != len(observed) or len(observed) != 206 or set(observed) != {row.id for row in rows}:
        raise ValueError(f"{kind}: normal receiver proof roster mismatch")
    return observed


def contact_side_lines():
    """Recovered side-paired credit rows, emitted for both attacker teams."""
    if not CONTACT_SIDE_ROWS.is_file():
        return []
    variants = json.loads(CONTACT_SIDE_ROWS.read_text()).get("variants", [])
    if not variants:
        return []
    lines = ["# authored: side rows carry the recovered credit and receiver-rank pair;",
             "# the per-receiver class row stays in the damage-vs line."]
    for variant in sorted(variants, key=lambda item: item["team"]):
        credit = variant["credit_difficulty"] * variant["credit_rank"]
        lines.append(f"damage-side={variant['team']}|{_damage_number(credit)}|1.0|"
                     f"{_damage_number(variant['receiver_rank'])}|1.0")
    return lines


def basic_shot_damage_lines(dol, model):
    """Recovered test-battle rows for the authored basic shot (settings 0/16)."""
    if not DAMAGE_SELECTORS.is_file():
        return []
    selectors = json.loads(DAMAGE_SELECTORS.read_text())
    try:
        row = damage_content_row.build(
            dol, selectors,
            {"model": model, "side": 0, "settings": 0, "rank": 0},
            {"model": 0x000A, "side": 1, "settings": 16, "rank": 0}, 8, 1.0)
    except ValueError:
        return []
    return ["# authored test damage rows: attacker settings 0, defender settings 16; not universal battle state",
            *contact_side_lines(),
            row["content"]["line"]]


def package(archive, dol, row, *, preview_extras=False, role_clips=False, source_variants=False,
            move_labels=(), launch_tables=None, linker=None, landing_pcm=None, origin_markers=False, action_clips=False,
             cruise_lab=False, movement_operands=False, source_pace=False, basic_shot=False, stock_guards=None,
             launch_pcm=None, projectile_sounds=None, tone_banks=None, dragon_carrier=None, dragon_impact_art=None,
            dragon_impact_pcm=None, worm_source=None, basic_melee=False, action_pcm=None, contact_roles=None,
            contact_power_test=False,contact_ladders=None,contact_tiers=None,contact_tier_reset_test=False,
             contact_audio=None,contact_visuals=None,contact_indices=None,contact_floor=None,contact_pause=False,
               continuation_source=None,speed_ramp_source=None,ninja_bomb_source=None,contact_tail_source=None,
                 sasuke_air_source=None,blue_striker_boost_source=None,ko_force_source=None):
    cruise_pilot = basic_shot and cruise_lab and (row.model==VIOLET_SHOT["model"] or any(row.model in cohort for cohort in (
        STATIC_CRUISE_SHOTS, STATIC_CRUISE_PHASE_SHOTS, CRUISE_CHARGE_SHOTS, ANIMATED_CRUISE_SHOTS)))
    clocked_pilot = basic_shot and row.model in CLOCKED_GROUND_SHOTS
    static_ground_pilot = basic_shot and row.model in STATIC_GROUND_MODELS
    dragon_pilot = basic_shot and dragon_carrier is not None
    worm_pilot = basic_shot and worm_source is not None
    tank_pilot = basic_shot and row.model in tanks.MODELS
    pair_pilot = basic_shot and row.model in ground_pairs.ROUTES
    single_shot_pilot = basic_shot and row.model in single_shots.MODELS
    breath_pilot = basic_shot and row.model in breath.MODELS
    roach_pilot = basic_shot and row.model==roach.MODEL
    zeta_pilot = basic_shot and row.model in zfield.MODELS
    charge_pilot = basic_shot and row.model in charge_field.ALL_MODELS
    flying_pilot = basic_shot and row.model in flying_boss.MODELS
    nu_pilot = basic_shot and row.model==nu_iii.MODEL
    staged_pilot = basic_shot and row.model==staged_field.MODEL
    icbm_pilot = basic_shot and row.model in icbm.MODELS
    spawner_pilot = basic_shot and row.model==spawner_shot.MODEL
    static_spawner_pilot = basic_shot and row.model==spawner_static.MODEL
    shot_pilot = basic_shot and (row.model in BASIC_SHOT_COHORT or cruise_pilot or clocked_pilot or static_ground_pilot or
                                dragon_pilot or worm_pilot or tank_pilot or pair_pilot or single_shot_pilot or breath_pilot)
    held_pilot = basic_shot and row.model==gatling.MODEL
    melee_pilot = basic_melee and row.model in knight.PILOTS
    ninja_bomb_pilot=ninja_bomb_source is not None and row.model in ninja_bomb.PILOTS
    if ninja_bomb_pilot and (not shot_pilot or melee_pilot):
        raise ValueError(f"{row.id}: Ninja bomb requires the existing B-shot test")
    pilot = (shot_pilot or roach_pilot or zeta_pilot or charge_pilot or icbm_pilot or
             spawner_pilot or static_spawner_pilot or flying_pilot or nu_pilot or staged_pilot or
             melee_pilot or held_pilot)
    # The test package is explicitly normal-only. Keep other models' exact
    # alternate declarations and copy the pilot's wrappers as offline assets.
    source_variants = source_variants and not pilot
    member_index, = struct.unpack(">I", dol.read(source_table(dol, 0x802D0584, row.model), 4))
    member, member_digest = archive.member(member_index, row.model)
    if not 10 <= len(member.entries) <= 32:
        raise ValueError(f"{row.id}: unexpected character block layout")
    actor, _ = member.read(0, BLOCK_LIMIT)
    motion, _ = member.read(3, BLOCK_LIMIT)
    wrapper, _ = member.read(4, BLOCK_LIMIT)
    if not actor or len(actor) % ACTOR_BYTES or len(actor) > 8 * ACTOR_BYTES:
        raise ValueError(f"{row.id}: unrecognized actor records")
    spec = actor[:ACTOR_BYTES]
    installer, = struct.unpack(">I", dol.read(source_table(dol, 0x802D3224, row.model), 4))
    mounts = neutral_preview_mounts(dol, row.model, installer)
    level = levels(dol, row.model)
    ratings = list(spec[0x1A4:0x1A8])
    if any(value > 10 for value in ratings) or any(values[0] <= 0 for values in level):
        raise ValueError(f"{row.id}: unrecognized stats")
    prefix, model = wrapper_model(wrapper)
    dragon_visuals=dragon_art.caster_art(model,dragon_carrier) if dragon_carrier is not None else None
    files = {"visual/body.prefix": prefix, "visual/body.hsd": model}
    wrapper_slots, alternates = [], []
    for i in range(4, 10):
        entry = member.entries[i]
        blob = wrapper if i == 4 else member.read(i, BLOCK_LIMIT)[0]
        rec = dict(slot=i, packed_bytes=entry["size"], compressed=entry["compressed"])
        if blob:
            if i == 4:
                rec.update(prefix_path="visual/body.prefix", model_path="visual/body.hsd")
            elif source_variants:
                alternates.append((i,blob,rec))
            else:
                path = f"visual/source-wrapper-{i}.bin"
                files[path] = blob
                rec["path"] = path
            rec.update(wrapper_bytes=len(blob), wrapper_sha256=sha256(blob).hexdigest())
        wrapper_slots.append(rec)
    try:
        joints = logical_joints(prefix, model)
        validate_preview_mounts(model, joints, mounts)
        joint_status = "indexed"
    except ValueError as exc:
        joints, joint_status = [], str(exc)
    if motion:
        files["visual/body.motions"] = motion
        try:
            clips = bank_clips(motion)
            motion_status = "indexed"
        except ValueError as exc:
            clips = []
            motion_status = str(exc)
    else:
        motion_status, clips = "absent", []
    variant_files, variants = variant_preview_files(alternates, motion, clips, mounts)
    files.update(variant_files)
    for i, blob, rec in alternates:
        if "variant_id" not in rec:
            path = f"visual/source-wrapper-{i}.bin"
            files[path] = blob
            rec["path"] = path
    _, catalog = package_text(row)
    changes = {
        "description": "Extracted normal assets and stats. Preview support varies; no executable gameplay.",
        "hp": f"{level[0][0]}|{level[-1][0]}",
        "ratings": "|".join(map(str, ratings)),
    }
    if pilot:
        test_name=("close-B melee" if melee_pilot else "held-B" if held_pilot else
                   "X-shot (B not implemented)" if (row.model in single_shots.MODELS and
                                                     single_shots.ROUTES[row.model].get("button")=="x") else "B-shot")
        if ninja_bomb_pilot:
            test_name="B-shot + grounded X-bomb"
        changes.update(description=f"Normal-only authored {test_name} Battle Test. Not retail combat.",
                       forms="none",**{"variant-overrides":"none"})
    moves = displayed_moves(dol, row.ordinal, spec, move_labels)
    display = move_profile(moves)
    if display:
        files["moves.txt"] = display
        changes["moves"] = "moves.txt"
    if variants:
        if len(row.variant_costs) != 6:
            raise ValueError("missing original variant cost lanes")
        changes["variant"] = "\n".join((
            f"normal|Normal|{row.normal_cost}|unresolved",
            *(f"variant={ident}|Source Variant {code}|{row.variant_costs[code]}|unresolved" for code,ident in variants),
        ))
        changes["variant-overrides"] = "variants.txt"
    catalog = "\n".join(f"{key}={changes[key]}" if (key := line.partition("=")[0]) in changes else line
                        for line in catalog.splitlines() if not line.startswith("#")) + "\n"
    manifest = (f"GFBORG 2\nid={row.id}\nname={row.name}\ncatalog=catalog.txt\n"
                "model=visual/body.hsd\n")
    if motion:
        manifest += "animations=visual/body.motions\n"
    actions = action_candidates(dol, installer, row.model)
    if linker is not None:
        linker.attach(actions,row.model,clips,joints,index_action_entries=action_clips,installer=installer)
    roach_source = roach.battle_test(joints,actions,level[0]) if roach_pilot else None
    icbm_source = icbm.battle_test(row.model,joints,level[0],actions) if icbm_pilot else None
    static_spawner_source = (spawner_static.battle_test(row.model,joints,level[0],actions)
                             if static_spawner_pilot else None)
    prior_close=tuple(x for x in action_preview_candidates(actions) if x["action_group"]=="B-close") if (
        action_clips and (row.model in knight.LANCES or row.model in samurai.MODELS or row.model in ninja.MODELS or
                          row.model in wings.MODELS or row.model in grounded.MODELS)) else ()
    parts=source_parts(dol,row.model,spec,joints)
    equipment_source=equipment.equipment(dol,row.model,model,joints,parts) if (
        action_clips and row.model in equipment.MODELS) else None
    melee_source=knight.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True) if (
        action_clips and row.model in knight.REVIEWED) else None
    samurai_source=samurai.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True) if (
        action_clips and row.model in samurai.MODELS) else None
    ninja_source=ninja.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True,body=model) if (
        action_clips and row.model in ninja.MODELS) else None
    blue_source=blue.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True) if (
        action_clips and row.model in blue.MODELS) else None
    robot_source=robots.close_b_pose(linker,row.model,actions,clips,parts,model,publish_preview=True) if (
        action_clips and row.model in robots.MODELS) else None
    wing_source=wings.close_b_pose(linker,row.model,actions,clips,parts,model,publish_preview=True) if (
        action_clips and row.model in wings.MODELS) else None
    bomber_source=bombers.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True) if (
        action_clips and (row.model in bombers.MODELS or row.model==bombers.REMOTE)) else None
    grounded_source=grounded.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True) if (
        action_clips and row.model in grounded.MODELS) else None
    tank_source=tanks.close_or_far_b_source(linker,row.model,actions,joints) if (
        action_clips and row.model in tanks.MODELS) else None
    pair_source=ground_pairs.source_pair(linker,row.model,actions,joints) if (
        action_clips and row.model in ground_pairs.ROUTES) else None
    single_shot_source=single_shots.source_shot(linker,row.model,actions,joints) if (
        action_clips and row.model in single_shots.MODELS) else None
    breath_source=breath.source_breath(linker,row.model,actions) if (
        action_clips and row.model in breath.MODELS) else None
    girl_source=girl.close_b_pose(linker,row.model,actions,clips,parts,publish_preview=True) if (
        action_clips and (row.model in girl.MODELS or row.model==girl.MISSING)) else None
    worm = attach_worm_pose(linker,actions,clips,joints,worm_source,publish_preview=action_clips) if worm_source is not None else None
    held_source=gatling.held_fire_source(linker,actions,clips,parts,launch_tables,projectile_sounds,publish_preview=True) if (
        action_clips and row.model==gatling.MODEL) else None
    preferred_b_shot=PREFERRED_B_SHOT.get(row.model) if pilot else None
    action_entries=action_preview_candidates(actions,preferred_b_shot=preferred_b_shot) if action_clips and motion else ()
    if prior_close:
        action_entries.extend(x for x in prior_close if not any(a["id"]==x["id"] for a in action_entries))
    origins=preview_origin_markers(release_index(actions)) if origin_markers else ()
    role_mapping = motion_roles(linker, actions, clips) if linker else None
    movement=movement_config_candidates(dol,installer,row.model)
    cruise=cruise_lab_candidate(motion,movement,role_mapping,actions) if cruise_lab else ()
    candidates = role_preview_candidates(role_mapping) if role_clips else []
    operands=None
    if movement_operands or source_pace:
        operands=movement_operand_profile(actor,role_mapping)
        operands["class_installer_binding"]=actor_record_binding(dol,installer,row.model,operands["records"],ACTOR_BYTES)
    pace=ground_pace_candidate(role_mapping,candidates,operands) if source_pace else None
    cruise_pace=cruise_pace_candidate(operands) if (source_pace and cruise_pilot and operands) else None
    zeta_source = zfield.battle_test(row.model,joints,candidates,pace,level[0],actions) if zeta_pilot else None
    spawner_source = (spawner_shot.battle_test(row.model,joints,candidates,pace,level[0],actions)
                      if spawner_pilot else None)
    charge_source = charge_field.battle_test(row.model,joints,candidates,pace,level[0],actions) if charge_pilot else None
    flying_source = (flying_boss.battle_test(row.model,joints,candidates,operands,level[0],actions)
                     if flying_pilot else None)
    nu_source = (nu_iii.battle_test(row.model,joints,level[0],actions) if nu_pilot else None)
    staged_source = (staged_field.battle_test(row.model,joints,candidates,pace,level[0],actions)
                     if staged_pilot else None)
    timing=clip_timing(motion,clips)
    if girl_source is not None and row.model==girl.AUTHORED_ACTION_LOOP:
        exact=[entry for entry in timing if entry["archive_offset"]==girl_source["pose"]["archive_offset"]]
        if (len(exact)!=1 or exact[0]["aobj_flags"]!=[0x20000000] or
                exact[0].get("strict_metadata_refusals")!=["aobj-flags"] or not exact[0]["animated_joint_count"] or
                exact[0]["authored_loop_joint_count"]!=exact[0]["animated_joint_count"] or
                exact[0]["authored_end_frames"]!=[38.]):
            raise ValueError(f"{row.id}: exact close-B loop-window evidence changed")
        girl_source["strict_clip_metadata"]=exact[0]
    air_dash_starts=()
    if not preview_extras and {"ground-travel","ground-takeoff","air-continuation","landing"}<={r[0] for r in candidates}:
        windows=air_dash_subranges(role_mapping,timing)
        if windows:
            roles={w[0] for w in windows}
            expanded=[r for r in candidates if r[0] not in roles]+[w[:3] for w in windows]
            if len(expanded)+len(action_entries)<=19:
                candidates=expanded
                air_dash_starts=tuple(w[3] for w in windows)
    cruise_entry=None
    if cruise_lab and motion and not mounts and not pace:
        flight_tuning=cruise_lab_tuning(movement,role_mapping,actions)
        if flight_tuning and (cruise_entry:=cruise_entry_candidate(role_mapping,timing,len(joints))):
            cruise=flight_tuning
    battle_receipt = None
    if held_pilot:
        if mounts or not motion or not held_source:
            raise ValueError("Gatling pilot requires exact body and release evidence")
        gameplay,pose,battle_receipt=gatling.battle_test(joints,candidates,pace,level[0],held_source,launch_audio=launch_pcm is not None)
        files.update({"gameplay.txt":gameplay,"visual/body.pose":pose})
        for cue in battle_receipt["authored_launch_sounds"]:
            files[cue["path"]]=launch_pcm[cue["bank"],cue["tone"]]
        manifest += "gameplay=gameplay.txt\n"
    elif melee_pilot:
        source=(melee_source or samurai_source or ninja_source or girl_source or blue_source or robot_source or wing_source or
                bomber_source or grounded_source)
        if not joints or not motion or source is None:
            raise ValueError(f"{row.id}: melee pilot requires its own body, hand slots and motion bank")
        by_clip={(g,c):at for g,c,at in clips}
        by_role={name:(g,c) for name,g,c in candidates}
        by_offset={t["archive_offset"]:t for t in timing}
        loop_metadata=[by_offset.get(by_clip.get(by_role.get(name))) for name in ("ground-stop","ground-travel")]
        gameplay,pose,battle_receipt=knight.melee_battle_test(
            row.model,joints,candidates,pace,level[0],source,clip_metadata=loop_metadata,mounts=mounts,
            body_material=("immutable-highlight" if row.model==bombers.REMOTE else None))
        hit_block,_=member.read(2,reaction_emit.HIT_BYTES)
        gameplay=reaction_melee.append_binding(dol,hit_block,row.model,gameplay,battle_receipt,source,actions["profiles"])
        if continuation:=battle_receipt.get("reaction",{}).get("opening-slash",{}).get("continuation"):
            native=battle_receipt["native_timing"]
            follow=reaction_melee_continuation.qualify_pose(
                linker,continuation,actions["profiles"],clips,timing,len(joints),battle_receipt["source_pose"],
                frame=native["active_frames"][0]-3,duration=native["move_ticks"])
            if follow is None:
                battle_receipt["continuation_pose_status"]="no exact bounded local clip; authored lead-in retained"
            else:
                g,c=follow["clip"]
                gameplay+=(f"# Authored switch before melee arming; not source phase timing.\n"
                           f"pose-followup=opening-slash|{follow['switch_frame']}|{g}:{c}|{follow['source_start_frame']}\n"
                           ).encode("ascii")
                battle_receipt["continuation_pose"]=follow
        files.update({"gameplay.txt":gameplay,"visual/body.pose":pose})
        manifest += "gameplay=gameplay.txt\n"
    elif (roach_source is not None or zeta_source is not None or charge_source is not None or
          icbm_source is not None or spawner_source is not None or static_spawner_source is not None or
          flying_source is not None or nu_source is not None or staged_source is not None):
        gameplay,pose,battle_receipt=(roach_source if roach_source is not None else
                                      zeta_source if zeta_source is not None else
                                      charge_source if charge_source is not None else
                                      icbm_source if icbm_source is not None else
                                      spawner_source if spawner_source is not None else
                                      static_spawner_source if static_spawner_source is not None else
                                      flying_source if flying_source is not None else
                                      nu_source if nu_source is not None else staged_source)
        files.update({"gameplay.txt":gameplay,"visual/body.pose":pose})
        manifest += "gameplay=gameplay.txt\n"
    elif shot_pilot:
        links = release_index(actions)
        if stock_guards is None:
            raise ValueError("basic shot test requires the stock guard index")
        stock_guards.attach(links)
        if launch_pcm is not None:
            attach_release_sfx(links,projectile_sounds)
        stop=next((r[1:] for r in candidates if r[0]=="ground-stop"),None)
        stop_at=next((at for g,c,at in clips if (g,c)==stop),None)
        idle_timing=next((r for r in timing if r["archive_offset"]==stop_at),None)
        if dragon_pilot:
            if not motion or mounts or not dragon_impact_art:
                raise ValueError("elemental pilot requires exact unmounted body and local motion bank")
            gameplay,pose,battle_receipt = dragon_battle_test(
                row.model,joints,candidates,role_mapping,actions,pace,level[0],links,{**dragon_carrier,"visuals":dragon_visuals},
                launch_audio=launch_pcm is not None)
            chain=dragon_trailing_chain(joints)
            gameplay+=("trailing-chain="+"|".join(map(str,chain["bones"]))+"\n").encode("ascii")
            battle_receipt["trailing_chain"]=chain
            percent=dragon_translation_scale(row.model,joints,motion)
            if percent!=100:
                gameplay+=f"animated-translation-percent={percent}\n".encode("ascii")
            battle_receipt["native_animated_translation_percent"]=percent
            files["visual/impact.hsd"]=dragon_impact_art
            if dragon_impact_pcm is not None:
                path="audio/impact-snd_com01-12.pcm"
                files[path]=dragon_impact_pcm
                gameplay+=f"impact-sound=orb|actor|{path}\n".encode("ascii")
                battle_receipt["authored_impact_sound"]={"path":path,"source_sound_id":12,
                                                        "trigger":"accepted terminal actor contact","retail_trigger_verified":False}
        elif worm_pilot:
            if mounts or not motion:
                raise ValueError("Worm pilot requires exact unmounted body and local motion bank")
            gameplay,pose,battle_receipt = worm_battle_test(
                row.model,joints,candidates,pace,level[0],worm,launch_audio=launch_pcm is not None)
        elif tank_pilot:
            if mounts or not motion or tank_source is None:
                raise ValueError("Tank pilot needs its exact body and verified source firing path")
            sfx=projectile_sounds[tanks.FAMILY][tank_source["muzzle"]["key"]]
            gameplay,pose,battle_receipt=tanks.battle_test(row.model,joints,candidates,pace,level[0],tank_source,sfx,
                                                           launch_audio=launch_pcm is not None)
        elif pair_pilot:
            if mounts or not motion or pair_source is None:
                raise ValueError("paired ground shot needs its exact body, local poses and verified release")
            gameplay,pose,battle_receipt=ground_pairs.battle_test(row.model,joints,candidates,pace,level[0],pair_source,
                                                                  projectile_sounds,launch_audio=launch_pcm is not None)
        elif single_shot_pilot:
            static = single_shots.ROUTES[row.model]["body"] is not None
            if mounts or (not motion and not static) or single_shot_source is None:
                raise ValueError("single ground shot needs its exact body, local poses and verified release")
            shot_pace = (ground_pace_candidate(role_mapping,candidates,operands,static_ground_body=True)
                         if static else pace)
            if shot_pace is None and single_shots.ROUTES[row.model].get("pace") != "authored":
                raise ValueError("single ground shot needs its exact source-relative pace")
            gameplay,pose,battle_receipt=single_shots.battle_test(row.model,joints,candidates,shot_pace,level[0],
                                                                  single_shot_source,projectile_sounds,
                                                                  launch_audio=launch_pcm is not None)
            if action_clips and row.model == cannon_beam.MODEL:
                beam_source=cannon_beam.source_beam(dol, actions)
                gameplay += ("\n".join(cannon_beam.lines(level[0]))+"\n").encode("ascii")
                battle_receipt["x_charge_beam"]=cannon_beam.receipt(beam_source)
        elif breath_pilot:
            if mounts or not motion or breath_source is None:
                raise ValueError("breath pilot needs its exact body, poses and paid program window")
            gameplay,pose,battle_receipt=breath.battle_test(row.model,joints,candidates,role_mapping,pace,level[0],
                                                            breath_source,chain=breath.chain_for(joints))
        elif cruise_pilot or clocked_pilot or static_ground_pilot:
            animated_pilot = cruise_pilot and row.model in ANIMATED_CRUISE_SHOTS
            if mounts or bool(motion)!=(clocked_pilot or animated_pilot):
                raise ValueError("pose-free pilot requires the exact unmounted body and motion policy")
            if row.model in CRUISE_CHARGE_SHOTS:
                gameplay,pose,battle_receipt = cruise_charge_shot_test(
                    row.model,joints,actions,level[0],links,cruise,
                    launch_audio=launch_pcm is not None,pace=cruise_pace)
            else:
                gameplay,pose,battle_receipt = pose_free_shot_test(
                    row.model,joints,actions,level[0],links,cruise,linker.ordered_shot_sites(row.model),
                    launch_audio=launch_pcm is not None,role_clips=candidates,
                    pace=(ground_pace_candidate(role_mapping,candidates,operands,static_ground_body=True)
                          if static_ground_pilot else (cruise_pace or pace)),
                    cruise_entry=cruise_entry if animated_pilot else None)
        else:
            gameplay,pose,battle_receipt = basic_shot_test(row.model,joints,candidates,action_entries,pace,level[0],links,
                                                       paired_sites=linker.ordered_shot_sites(row.model),
                                                       launch_audio=launch_pcm is not None,
                                                       actor_sfx=action_sfx_index(actions,tone_banks)
                                                       if launch_pcm is not None and row.model==ACTOR_RELEASE_SOUND[0] else (),
                                                       idle_timing=idle_timing)
            damage_lines=basic_shot_damage_lines(dol,row.model) if contact_roles is None else ()
            if damage_lines:
                gameplay += ("\n".join(damage_lines)+"\n").encode("ascii")
                battle_receipt["damage_rows"]=damage_lines
        if battle_receipt.get("reaction_releases"):
            hit_block,_=member.read(2,reaction_emit.HIT_BYTES)
            gameplay=reaction_emit.append_bindings(dol,hit_block,row.model,gameplay,battle_receipt)
        if continuation_source is not None:
            gameplay=projectile_continuation.bind(row.model,member_digest,gameplay,battle_receipt,continuation_source)
        if speed_ramp_source is not None:
            gameplay=projectile_speed_ramp.bind(row.model,member_digest,gameplay,battle_receipt,speed_ramp_source)
        if contact_tail_source is not None:
            gameplay=projectile_contact_tail.bind(row.model,member_digest,gameplay,battle_receipt,contact_tail_source)
        files.update({"gameplay.txt":gameplay,"visual/body.pose":pose})
        for cue in battle_receipt.get("authored_launch_sounds",()):
            if "path" in cue:
                files[cue["path"]]=launch_pcm[cue["bank"],cue["tone"]]
        if cue:=battle_receipt.get("authored_release_sound"):
            files[cue["path"]]=launch_pcm[cue["bank"],cue["tone"]]
        manifest += "gameplay=gameplay.txt\n"
    if contact_roles is not None and battle_receipt is not None:
        report, normal_records = contact_roles
        roles = damage_contact_roles.normal_profile_rows(report, normal_records, row.model, installer, actor, image=dol)
        if contact_indices is not None:
            selector=roles["receiver_selector"]
            indexed=damage_contact_indices.profile_lines(contact_indices,selector[0] if selector is not None else None)
            roles["lines"].extend(indexed)
            roles["indexed_tables"]=indexed
        if contact_ladders is not None:
            receiver_ladders, attacker_ladders = contact_ladders
            ladder_rows = damage_receiver_ladders.profile_rows(receiver_ladders,roles["receiver_selector"])
            roles["lines"].extend(ladder_rows)
            roles["receiver_ladders"]=bool(ladder_rows)
            outgoing = damage_attacker_ladders.profile_rows(attacker_ladders,dol,battle_receipt.get("reaction",{}))
            roles["lines"].extend(outgoing["lines"])
            roles["attacker_ladders"]=outgoing
        if contact_tiers is not None:
            receiver=roles["receiver_selector"] is not None
            reset=None
            if contact_tier_reset_test:
                normal=roles["receiver_record"]
                reset={"capacity":normal["reset_capacity"],"status":"qualified observed normal receiving record",
                       "spec_record":normal["spec_record"],"record_sha256":normal["record_sha256"]}
            tier_rows = damage_contact_tiers.profile_rows(contact_tiers,dol,battle_receipt.get("reaction",{}),
                                                          receiver=receiver,reset=reset)
            roles["lines"].extend(tier_rows["lines"])
            battle_receipt["contact_tier_test"]=tier_rows
        files["gameplay.txt"]+=("\n".join(roles["lines"])+"\n").encode("ascii")
        battle_receipt["contact_scale_test"]=roles
    if contact_power_test and battle_receipt is not None:
        files["gameplay.txt"]=damage_contact_power.append(files["gameplay.txt"],battle_receipt,dol)
    if contact_floor is not None and battle_receipt is not None:
        files["gameplay.txt"]=damage_contact_power.append_floor(files["gameplay.txt"],battle_receipt,dol,
                                                               minimum=contact_floor["minimum"])
    if contact_pause and battle_receipt is not None:
        held=contact_pose_pause.profile_rows(dol,battle_receipt.get("reaction",{}),
                                           operands["class_installer_binding"],len(actor),
                                           native=b"movement=normal-gred-gfmm9" not in files["gameplay.txt"].splitlines())
        if held["lines"]:
            files["gameplay.txt"]+=("\n".join(held["lines"])+"\n").encode("ascii")
        battle_receipt["contact_pose_pause"]=held
    if contact_audio is not None and battle_receipt is not None:
        report, tones, samples, observed=contact_audio
        receiver=contact_sound.normal_receiver_band(report,dol,actions,observed.get(row.id),installer,actor)
        cue_rows, paths=contact_sound.profile_rows(report,dol,battle_receipt.get("reaction",{}),receiver,tones)
        if cue_rows["lines"]:
            files["gameplay.txt"]+=("\n".join(cue_rows["lines"])+"\n").encode("ascii")
        files.update((path,samples[tone]) for path,tone in paths.items())
        battle_receipt["contact_audio"]=cue_rows
    if contact_visuals is not None and battle_receipt is not None:
        actor_retirement_art=any(line.startswith(b"impact-record=") and line.split(b"|",2)[1:2]==[b"actor"]
                                 for line in files["gameplay.txt"].splitlines())
        cue_rows=contact_impact_visuals.profile_rows(contact_visuals,dol,battle_receipt.get("reaction",{}),
                                                    operands["class_installer_binding"],len(actor),
                                                    actor_retirement_art=actor_retirement_art)
        if cue_rows["requires_records"]:
            path="visual/impact.hsd"
            package_banks=[line for line in files["gameplay.txt"].splitlines() if line.startswith(b"impact-record-bank=")]
            if package_banks and (package_banks!=[f"impact-record-bank={path}".encode("ascii")] or
                                  files.get(path)!=dragon_impact_art):
                raise ValueError("contact visual bank conflicts with package impact records")
            if not package_banks:
                files[path]=dragon_impact_art
                files["gameplay.txt"]+=f"impact-record-bank={path}\n".encode("ascii")
        if cue_rows["lines"]:
            files["gameplay.txt"]+=("\n".join(cue_rows["lines"])+"\n").encode("ascii")
        battle_receipt["contact_visuals"]=cue_rows
    if battle_receipt is not None and pace is not None and operands is not None:
        text=files["gameplay.txt"].decode("ascii")
        if "cruise-boost=" not in text and "air-jump-points=" not in text:
            binding=operands.get("class_installer_binding") or {}
            selected=binding.get("selected_record")
            records=operands.get("records") or []
            budget=None
            if isinstance(selected,int) and 0<=selected<len(records):
                value=(records[selected].get("operands") or {}).get("09F",{}).get("value")
                if isinstance(value,int) and 1<=value<=8:
                    budget=value
            if budget is not None:
                files["gameplay.txt"]+=f"air-action-budget={budget}\n".encode("ascii")
                battle_receipt["air_action_budget"]={"operand":"09F","value":budget,
                                                      "source":"combined jump and air-dash cap",
                                                      "retail_behaviour_verified":False}
    if sasuke_air_source is not None and row.model==sasuke_air_point_policy.MODEL:
        files["gameplay.txt"]=sasuke_air_point_policy.bind(
            row.model,member_digest,files.get("gameplay.txt",b""),battle_receipt,sasuke_air_source)
    if blue_striker_boost_source is not None and row.model==blue_striker_boost.MODEL:
        files["gameplay.txt"]=blue_striker_boost.bind(
            row.model,member_digest,spec,files.get("gameplay.txt",b""),battle_receipt,blue_striker_boost_source)
    # Measured presentation offsets: each rig's posed floor at ticks 1, 10 and
    # 30 sits this far above the actor floor while the actor itself is grounded.
    # These are authored corrections, not recovered source values.
    POSE_GROUND_OFFSETS={
        0x0109:216, 0x0A07:84, 0x050D:83, 0x0517:83, 0x0D01:74, 0x0D02:71,
        0x0401:68, 0x0B00:50, 0x0B06:50, 0x0B03:48, 0x0B05:48, 0x0B07:48,
        0x0406:47, 0x040B:47, 0x040C:47, 0x040D:47, 0x0B02:43, 0x0B01:39,
        0x0B04:38, 0x070C:36, 0x070D:36, 0x0F06:26, 0x0F04:23, 0x0619:22,
        0x061F:22, 0x0625:22,
    }
    if battle_receipt is not None and row.model in POSE_GROUND_OFFSETS:
        if "cruise-boost=" not in files["gameplay.txt"].decode("ascii"):
            offset=POSE_GROUND_OFFSETS[row.model]
            files["gameplay.txt"]+=f"pose-ground-offset={offset}\n".encode("ascii")
            battle_receipt["pose_ground_offset"]={"units":offset,
                                                 "source":"measured posed floor, stable at ticks 1/10/30",
                                                 "retail_behaviour_verified":False}
    if battle_receipt is not None and (reflection:=zero_reflection_rules(row.model,model)):
        rules,policy=reflection
        files["gameplay.txt"]+=rules.encode("ascii")
        battle_receipt["zero_reflection"]=policy
    if battle_receipt is not None and equipment_source is not None and equipment_source["native_mount_admitted"]:
        files["gameplay.txt"]+=equipment.equipment_rules(equipment_source)
        battle_receipt["equipment_visuals"]=equipment_source["scope"]
    landing=landing_sound(dol,row.model)
    landing_path=""
    if landing_pcm is not None and {"ground-travel","ground-takeoff","landing","air-continuation"}<={c[0] for c in candidates}:
        if landing["tone"] not in landing_pcm:
            raise ValueError("unqualified landing PCM selection")
        landing_path="audio/lab-landing.pcm"
    preview = None
    raw_preview = (preview_text("normal", "visual/body.prefix", motion, clips, preview_extras, candidates, mounts, landing_path,
                                 origins, action_entries, cruise, pace, cruise_entry, air_dash_starts)
                   if joints or not mounts else None)
    if raw_preview is not None:
        preview = "static" if not motion else "raw-clips"
        files["preview.txt"] = raw_preview
        manifest += "preview=preview.txt\n"
        if landing_path:
            files.setdefault(landing_path, landing_pcm[landing["tone"]])
            landing["lab_pcm"] = landing_path
    if landing_path and battle_receipt is not None:
        # The landing candidate is bound through the existing movement-sound
        # API; the trigger itself stays authored.
        files.setdefault(landing_path, landing_pcm[landing["tone"]])
        files["gameplay.txt"] += f"movement-sound=landing|{landing_path}\n".encode("ascii")
        battle_receipt["authored_landing_sound"] = {
            "path": landing_path, "trigger": "landing cue", "retail_trigger_verified": False}
    if action_pcm and battle_receipt is not None and "gameplay.txt" in files:
        cues = action_sfx_index(actions, tone_banks)
        if cues:
            text = files["gameplay.txt"].decode("ascii")
            moves = [line[5:].split("|") for line in text.splitlines() if line.startswith("move=")]
            bound = []
            for slot, cue in enumerate(cues):
                tone = (cue.get("bank"), cue.get("tone"))
                pcm = action_pcm.get(tone)
                if not pcm:
                    continue
                groups = {route.get("action_group") for route in cue.get("routes") or []}
                button = "x" if groups & {"X", "X-charge"} else "b"
                target = next((move for move in moves if len(move) > 1 and move[1] == button), None)
                if not target or any(item["move"] == target[0] for item in bound):
                    continue
                path = f"audio/action-{slot}.pcm"
                files[path] = pcm
                files["gameplay.txt"] += f"action-sound={target[0]}|{path}\n".encode("ascii")
                bound.append({"path": path, "move": target[0],
                              "source_sound_id": cue["source_sound_id"],
                              "routes": sorted(group for group in groups if group),
                              "retail_trigger_verified": False})
            if bound:
                battle_receipt["authored_action_sounds"] = bound
    if ninja_bomb_pilot:
        files["gameplay.txt"]=ninja_bomb.bind(
            row.model,member_index,member_digest,files["gameplay.txt"],battle_receipt,ninja_bomb_source,
            motion=motion,clips=clips,timing=timing,joints=joints,level=level[0])
    files["catalog.txt"] = catalog.encode("ascii")
    summary = {
        "id": row.id, "variant": 0, "member": member_index, "member_sha256": member_digest,
        "actor_record_count": len(actor) // ACTOR_BYTES,
        "hp_by_level": [values[0] for values in level],
        "ratings_def_sht_atk_spd": ratings,
        "raw_level_rows": level,
        "raw_jump_display": list(spec[0x1A0:0x1A3]),
        "raw_move_display_slots": list(spec[0x1A8:0x1AD]),
        "move_display_map": moves,
        "motion_index": motion_status, "raw_animation_slots": clips,
        "local_clip_timing": timing,
        "model_wrapper_slots": wrapper_slots, "selectable_variants": ["normal", *(ident for _,ident in variants)],
        "logical_joint_index": joint_status, "logical_joints": joints,
        "model_parts": parts,
        "preview_declaration": preview, "preview_validated": False,
        "projectile_origins": origin_candidates(launch_tables, row.model, joints) if launch_tables else "unresolved",
        "sfx_bindings": {"landing_candidate": landing, "other": "unresolved"},
        "movement_candidates": movement,
        "action_candidates": actions,
        "motion_role_candidates": role_mapping,
        "role_preview_clips": candidates if raw_preview else [],
        "movement_and_move_mapping": "unresolved",
        "asset_sha256": {name: sha256(data).hexdigest() for name, data in files.items()},
    }
    if air_dash_starts and raw_preview:
        summary["lab_air_dash_starts"]=air_dash_starts
    if dragon_carrier is not None:
        bank,_=member.read(2,COLLISION_BYTES)
        summary["dragon_carrier_collision"]=dragon_collision_index(bank,member_index,dragon_carrier)
        summary["dragon_carrier_visuals"]=dragon_visuals
    if worm is not None:
        summary["worm_shot_source"]=worm
    if melee_source is not None:
        summary["knight_close_b_source"]=melee_source
    if samurai_source is not None:
        summary["samurai_close_b_source"]=samurai_source
    if ninja_source is not None:
        summary["ninja_close_b_source"]=ninja_source
    if girl_source is not None:
        summary["girl_close_b_source"]=girl_source
    if blue_source is not None:
        summary["blue_close_b_source"]=blue_source
    if robot_source is not None:
        summary["robot_diver_close_b_source"]=robot_source
    if wing_source is not None:
        summary["wing_close_b_source"]=wing_source
    if bomber_source is not None:
        summary["bomber_hero_close_b_source"]=bomber_source
    if grounded_source is not None:
        summary["grounded_close_b_source"]=grounded_source
    if tank_source is not None:
        summary["tank_shot_source"]=tank_source
    if pair_source is not None:
        summary["paired_ground_shot_source"]=pair_source
    if single_shot_source is not None:
        summary["single_ground_shot_source"]=single_shot_source
    if breath_source is not None:
        summary["breath_source"]=breath_source
    if battle_receipt is not None and "x_charge_beam" in battle_receipt:
        summary["x_charge_beam_source"]=battle_receipt["x_charge_beam"]
    if equipment_source is not None:
        summary["battle_equipment_source"]=equipment_source
    if action_clips and row.model in samurai.BLOCKED_MATERIALS:
        summary["samurai_melee_refusal"]="strict native trial: unsupported material/texture-matrix stages; no substitute"
    if held_source is not None:
        summary["held_fire_source"]=held_source
    if mounts:
        summary["neutral_preview_mounts"] = mounts
    if origins and raw_preview:
        summary["preview_origin_markers"] = origins
    if action_entries and raw_preview:
        summary["action_preview_clips"] = action_entries
    if cruise and raw_preview:
        summary["cruise_lab"]={"source_type":3,"selectors":[27,28],"static_body":not bool(cruise_entry),
                               "native_tuning":cruise,"retail_control_verified":False}
        if cruise_entry:
            summary["cruise_lab"]["entry_clip"]=cruise_entry
    if operands is not None:
        summary["movement_operands"]=operands
    if pace and raw_preview:
        summary["lab_ground_pace"]=pace
    if cruise_pace and raw_preview:
        summary["lab_cruise_pace"]=cruise_pace
    if battle_receipt:
        summary["basic_melee_battle_test" if melee_pilot else "held_fire_battle_test" if held_pilot else "basic_shot_battle_test"]=battle_receipt
    if battle_receipt and (budget_rule:=battle_receipt.get("air_action_budget")):
        summary["air_action_budget"]=budget_rule
    if roach_source is not None:
        summary["roach_bomb_source"]=battle_receipt
    if zeta_source is not None:
        summary["zeta_field_source"]=battle_receipt
    if charge_source is not None:
        summary["charge_field_source"]=battle_receipt
    if icbm_source is not None:
        summary["icbm_source"]=battle_receipt
    if spawner_source is not None:
        summary["spawner_shot_source"]=battle_receipt
    if static_spawner_source is not None:
        summary["spawner_static_source"]=battle_receipt
    if flying_source is not None:
        summary["flying_field_source"]=battle_receipt
    if nu_source is not None:
        summary["nu_field_source"]=battle_receipt
    if staged_source is not None:
        summary["staged_field_source"]=battle_receipt
    if ko_force_source is not None and row.model==ko_force_cost.MODEL:
        files["gameplay.txt"]=ko_force_cost.bind(row.model,member_index,member_digest,files.get("gameplay.txt",b""),
                                                 battle_receipt,ko_force_source)
    files["source-index.json"] = json_line(summary)
    files["borg.manifest"] = manifest.encode("ascii")  # publish last
    return files, summary


def json_line(obj):
    return (json.dumps(obj, sort_keys=True, separators=(",", ":"), allow_nan=False) + "\n").encode("ascii")


def write_new(folder, files):
    for name, data in files.items():
        dest = folder / name
        dest.parent.mkdir(exist_ok=True)
        with dest.open("xb") as target:
            target.write(data)


def convert(catalog, dol_path, afs_path, output, *, preview_extras=False, role_clips=False,
             spec_text=None, source_variants=False, landing_decoder=None, origin_markers=False, action_clips=False,
             cruise_lab=False, movement_operands=False, source_pace=False, basic_shot=False, shot_audio_decoder=None,
             dragon_impact_audio=False,basic_melee=False,contact_scale_test=False,contact_power_test=False,
             contact_ladder_test=False,contact_tier_test=False,contact_tier_reset_test=False,contact_audio_decoder=None,
              contact_art_test=False,contact_index_test=False,contact_floor_test=False,contact_pause_test=False,
               projectile_continuation_test=False,projectile_speed_ramp_test=False,ninja_bomb_test=False,
                 projectile_contact_tail_test=False,sasuke_air_points_test=False,blue_striker_boost_test=False,
                 ko_force_cost_test=False):
    output = Path(output)
    repo = Path(__file__).resolve().parents[2]
    destination = output.resolve()
    if destination.is_relative_to(repo) and not destination.is_relative_to(repo / "local"):
        raise ValueError("in-repository roster output must stay under local/")
    if output.exists() or output.is_symlink():
        raise FileExistsError("refusing to overwrite roster output")
    if landing_decoder is not None and not role_clips:
        raise ValueError("--landing-pcm requires --role-clips")
    if source_pace and not role_clips:
        raise ValueError("--source-pace requires --role-clips")
    if basic_shot and not (source_pace and action_clips):
        raise ValueError("--basic-shot-test requires --source-pace and --action-clips")
    if projectile_continuation_test and not basic_shot:
        raise ValueError("--projectile-continuation-test requires --basic-shot-test")
    if projectile_speed_ramp_test and not basic_shot:
        raise ValueError("--projectile-speed-ramp-test requires --basic-shot-test")
    if projectile_contact_tail_test and not basic_shot:
        raise ValueError("--projectile-contact-tail-test requires --basic-shot-test")
    if sasuke_air_points_test and not basic_shot:
        raise ValueError("--sasuke-air-points-test requires --basic-shot-test")
    if blue_striker_boost_test and not (basic_shot and cruise_lab):
        raise ValueError("--blue-striker-boost-test requires --basic-shot-test and --cruise-lab")
    if ninja_bomb_test and not basic_shot:
        raise ValueError("--ninja-bomb-test requires --basic-shot-test")
    if basic_melee and not (source_pace and action_clips):
        raise ValueError("--basic-melee-test requires --source-pace and --action-clips")
    if ko_force_cost_test and not basic_melee:
        raise ValueError("--ko-force-cost-test requires --basic-melee-test")
    if shot_audio_decoder is not None and not basic_shot:
        raise ValueError("--basic-shot-audio requires --basic-shot-test")
    if dragon_impact_audio and shot_audio_decoder is None:
        raise ValueError("--dragon-impact-audio requires --basic-shot-audio")
    if contact_scale_test and not (basic_shot or basic_melee):
        raise ValueError("--contact-scale-test requires an authored shot or melee Battle Test")
    if contact_power_test and not (basic_shot or basic_melee):
        raise ValueError("--contact-power-test requires an authored shot or melee Battle Test")
    if contact_ladder_test and not contact_scale_test:
        raise ValueError("--contact-ladder-test requires --contact-scale-test")
    if contact_tier_test and not contact_scale_test:
        raise ValueError("--contact-tier-test requires --contact-scale-test")
    if contact_tier_reset_test and not contact_tier_test:
        raise ValueError("--contact-tier-reset-test requires --contact-tier-test")
    if contact_audio_decoder is not None and not (basic_shot or basic_melee):
        raise ValueError("--contact-audio requires an authored shot or melee Battle Test")
    if contact_art_test and not (basic_shot or basic_melee):
        raise ValueError("--contact-art-test requires an authored shot or melee Battle Test")
    if contact_index_test and not contact_scale_test:
        raise ValueError("--contact-index-test requires --contact-scale-test")
    if contact_floor_test and not (basic_shot or basic_melee):
        raise ValueError("--contact-floor-test requires an authored shot or melee Battle Test")
    if contact_pause_test and not (basic_shot or basic_melee):
        raise ValueError("--contact-pause-test requires an authored shot or melee Battle Test")
    rows = read_catalog(catalog)
    labels = ()
    if spec_text is not None:
        with open_regular(spec_text) as stream:
            text = stream.read((1 << 20) + 1)
        if sha256(text).hexdigest() != SPEC_TEXT_DIGEST:
            raise ValueError("not the qualified GG4E08 GFTX specification export")
        labels = decode_move_labels(text)
    with open_regular(dol_path) as stream:
        data = stream.read((8 << 20) + 1)
        if len(data) > 8 << 20:
            raise ValueError("GG4E08 DOL byte budget")
        dol = DolImage(data, DOL_DIGEST)
    if contact_power_test or contact_floor_test:
        damage_contact_power.verify_source(dol)
    contact_ladders = (damage_receiver_ladders.recover(dol), damage_attacker_ladders.recover(dol)
                       ) if contact_ladder_test else None
    contact_tiers = damage_contact_tiers.recover(dol) if contact_tier_test else None
    contact_indices = damage_contact_indices.recover(dol) if contact_index_test else None
    contact_floor = damage_amount_model.recover_finalizer(dol) if contact_floor_test else None
    contact_pause = contact_pose_pause.recover(dol) if contact_pause_test else None
    continuation_source=projectile_continuation.qualify(dol) if projectile_continuation_test else None
    speed_ramp_source=projectile_speed_ramp.qualify(dol) if projectile_speed_ramp_test else None
    contact_tail_source=projectile_contact_tail.qualify(dol) if projectile_contact_tail_test else None
    sasuke_air_source=sasuke_air_point_policy.qualify(dol) if sasuke_air_points_test else None
    blue_striker_boost_source=blue_striker_boost.qualify(dol) if blue_striker_boost_test else None
    ko_force_source=ko_force_cost.qualify(dol) if ko_force_cost_test else None
    ninja_bomb_source=ninja_bomb.qualify(dol) if ninja_bomb_test else None
    contact_roles = None
    if contact_scale_test:
        contact_roles = damage_contact_roles.recover(dol), _normal_receiver_proofs(
            NORMAL_DAMAGE_RECORDS, NORMAL_DAMAGE_RECORDS_SHA256, "gf-retail-normal-receiver-factors", rows)
    with open_regular(afs_path) as stream:
        if any(source is not None for source in (
                continuation_source,speed_ramp_source,contact_tail_source,ninja_bomb_source,
                sasuke_air_source,blue_striker_boost_source,ko_force_source)):
            from tools.analyze.retail_action_input import verify_archive
            verify_archive(stream)
        archive = RetailArchive(stream)
        shared_files, shared_index = archive.shared_assets()
        if contact_power_test:
            shared_index["contact_power_test"] = "uniform positive HP base of qualified direct contact rows"
        if contact_roles is not None:
            report = contact_roles[0]
            shared_index["contact_scale_test"] = {key: report[key] for key in ("settings", "rank", "scope")}
        if contact_ladders is not None:
            shared_index["contact_ladder_test"] = {
                key: report["native_scope"] for key, report in zip(("receiving", "outgoing"), contact_ladders, strict=True)
            }
        if contact_tiers is not None:
            shared_index["contact_tier_test"] = contact_tiers["scope"]
        if contact_tier_reset_test:
            shared_index["contact_tier_reset_test"] = contact_tiers["reset"]["native_scope"]
        if contact_indices is not None:
            shared_index["contact_index_test"] = contact_indices["scope"]
        if contact_floor is not None:
            shared_index["contact_floor_test"] = contact_floor["native_scope"]
        if contact_pause is not None:
            shared_index["contact_pause_test"] = contact_pause["native"]
        if continuation_source is not None:
            shared_index["projectile_continuation_test"]={**continuation_source,"borg":"gg4e08:0600"}
        if speed_ramp_source is not None:
            shared_index["projectile_speed_ramp_test"]={**speed_ramp_source,"borg":"gg4e08:0600"}
        if contact_tail_source is not None:
            shared_index["projectile_contact_tail_test"]={f"gg4e08:{m:04x}": proof
                                                         for m, proof in contact_tail_source.items()}
        if ninja_bomb_source is not None:
            shared_index["ninja_bomb_test"]=ninja_bomb_source
        if sasuke_air_source is not None:
            shared_index["sasuke_air_points_test"]={**sasuke_air_source,"borg":"gg4e08:000a"}
        if blue_striker_boost_source is not None:
            shared_index["blue_striker_boost_test"]={**blue_striker_boost_source,"borg":"gg4e08:0d00"}
        if ko_force_source is not None:
            shared_index["ko_force_cost_test"]={**ko_force_source,"borg":"gg4e08:0807"}
        pcm=None
        if landing_decoder is not None:
            pcm=decode_lab_landing_pcm(landing_decoder,shared_files,shared_index)
            shared_index["lab_landing_pcm"]={
                "format":"stereo 48 kHz s16le",
                "tones":{tone:{"bytes":len(data),"sha256":sha256(data).hexdigest()} for tone,data in pcm.items()},
                "retail_trigger_qualified":False,
            }
        launches = origin_tables(dol)
        projectile_sound = projectile_sound_index(dol,launches,shared_files,shared_index["audio_bank_members"])
        tone_banks = normal_tone_banks(dol,shared_files,shared_index["audio_bank_members"])
        contact_audio=None
        if contact_audio_decoder is not None:
            report=contact_sound.recover(dol)
            observed=_normal_receiver_proofs(
                NORMAL_SOUND_RECORDS, NORMAL_SOUND_RECORDS_SHA256, "gf-retail-normal-receiver-sound", rows)
            tones=contact_sound.tone_files(report,tone_banks)
            pairs=sorted({(tone["bank"],tone["tone"]) for tone in tones.values()})
            samples={}
            for at in range(0,len(pairs),16):
                samples.update(decode_shot_audio(contact_audio_decoder,pairs[at:at+16],shared_files,
                                                 shared_index["audio_bank_members"]))
            contact_audio=report,tones,samples,observed
            shared_index["contact_audio"]={"policy":report["policy"],"tones":tones,
                                            "normal_receiver_proof_sha256":NORMAL_SOUND_RECORDS_SHA256}
        worm_shots=worm_source_index(dol,lambda cue:tone_candidate(cue,tone_banks))
        launch_pcm = None
        if shot_audio_decoder is not None:
            tones = set()
            tank_keys={(tanks.FAMILY,k) for k in tanks.MUZZLES}
            for family,key in shot_factory_keys(cruise=cruise_lab)|tank_keys|ground_pairs.factory_keys()|single_shots.factory_keys():
                tone = pilot_launch_tone(projectile_sound["tables"][family][key])
                if tone is not None:
                    tones.add(tone)
            actor_tone = pilot_launch_tone(tone_candidate(ACTOR_RELEASE_SOUND[1],tone_banks))
            if actor_tone is None:
                raise ValueError("missing paid actor tone")
            tones.add(actor_tone)
            launch_pcm = decode_shot_audio(shot_audio_decoder,tones,shared_files,shared_index["audio_bank_members"])
            worm_tones={pilot_launch_tone(r["sound"]) for r in worm_shots.values()}-tones
            worm_tones.add(pilot_launch_tone(projectile_sound["tables"][gatling.FAMILY][gatling.KEY]))
            worm_tones-=tones
            if worm_tones:
                launch_pcm.update(decode_shot_audio(shot_audio_decoder,worm_tones,shared_files,shared_index["audio_bank_members"]))
        action_pcm = {}
        if shot_audio_decoder is not None and tone_banks is not None:
            # The curated action-SFX tables only ever reference a handful of
            # sound ids, so their tones can be decoded before any package is
            # written; the per-model cue index then just looks them up.
            action_ids = {entry[3] for entry in DIRECT.values()} | {RELAY_SOUND_ID}
            action_tones = set()
            for sound_id in sorted(action_ids):
                tone = pilot_launch_tone(tone_candidate(sound_id, tone_banks))
                if tone is not None:
                    action_tones.add(tone)
            if action_tones:
                action_pcm = decode_shot_audio(shot_audio_decoder, action_tones, shared_files,
                                               shared_index["audio_bank_members"])
        sound_index = "shared-projectile-sfx.json"
        shared_files[sound_index] = json_line(projectile_sound)
        shared_index["projectile_sfx_index"] = sound_index
        linker = ActionLinker(dol, launches, shared_index["common_motion"]["raw_animation_slots"])
        controls = ProgramControlIndex(linker)
        control_schema = control_write_schema(dol)
        stock_operands_schema = stock_schema(dol)
        stock_guards = StockGuardIndex(linker)
        if action_clips:
            knight.verify_source(dol)
            samurai.verify_source(dol)
            ninja.verify_source(dol)
            girl.verify_source(dol)
            blue.verify_source(dol)
            robots.verify_source(dol)
            wings.verify_source(dol)
            bombers.verify_source(dol)
            grounded.verify_source(dol)
            tanks.verify_source(dol)
            ground_pairs.verify_source(dol,linker)
            single_shots.verify_source(dol,linker)
            breath.verify_source(dol,linker)
            equipment.verify_source(dol)
            gatling.verify_source(dol)
        launch_index = "shared-projectile-origins.json"
        shared_files[launch_index] = json_line({"table_scope": "source origin operands, no projectile rules",
                                               "tables": launches})
        shared_index["projectile_origin_index"] = launch_index
        carrier_file = "shared-dragon-carriers.json"
        dragon_carriers = dragon_carrier_index(dol)
        shared_index["dragon_carrier_index"] = carrier_file
        area_file = "shared-dragon-impact-areas.json"
        dragon_areas=dragon_area_index(dol,dragon_carriers["borgs"],sound_resolver=lambda cue:tone_candidate(cue,tone_banks))
        dragon_areas["common_collision"]=common_collision(shared_files["shared-common-collision.bin"])
        impact_pcm=None
        if dragon_impact_audio:
            cue=dragon_areas["phase_zero"]["cue"]
            tone=(cue["bank"],cue["tone"])
            if tone!=("snd_com01",12):
                raise ValueError("elemental pilot impact cue identity")
            impact_pcm=decode_shot_audio(shot_audio_decoder,{tone},shared_files,shared_index["audio_bank_members"],
                                         max_seconds=4)[tone]
        dragon_areas["phase_zero"]["shape_verified"]=True # exact source descriptor, not publication at runtime
        shared_files[area_file] = json_line(dragon_areas)
        shared_index["dragon_impact_area_index"] = area_file
        dragon_art.art_source_matches(dol)
        effect_member=dragon_art.EFFECT_MEMBER
        if archive.names[effect_member]!=dragon_art.EFFECT_NAME or archive.names.count(dragon_art.EFFECT_NAME)!=1:
            raise ValueError("common effect member identity")
        effect=archive.payload(effect_member,MEMBER_LIMIT)
        if sha256(effect).hexdigest()!=dragon_art.EFFECT_SHA:
            raise ValueError("common effect member signature")
        effects=PzzArchive(RangeFile(BytesIO(effect),0,len(effect)))
        if len(effects.entries)!=len(dragon_art.EFFECT_BLOCKS):
            raise ValueError("common effect block count")
        art_files,common_effects=dragon_art.common_art([effects.read(i,size)[0] for i,(size,_,_) in enumerate(dragon_art.EFFECT_BLOCKS)])
        contact_visuals=None
        if contact_art_test:
            contact_visuals=contact_impact_visuals.recover(dol)
            shared_index["contact_visuals"]={
                "archive":contact_impact_visuals.qualify_archive(contact_visuals,art_files["effects/common-single.hsd"]),
                "native":contact_visuals["native"],
            }
        shared_files.update(art_files)
        shared_index["common_effect_archive"]=common_effects
        dragon_carriers["shared_descendant_visual"]=common_effects["descendant_record"]
        if movement_operands or source_pace:
            schema="shared-movement-operand-schema.json"
            shared_files[schema]=json_line({**movement_operand_schema(dol),"class_installer_binding":binding_schema(dol)})
            shared_index["movement_operand_schema"]=schema
        output.mkdir()
        write_new(output, shared_files)
        count = total = variant_count = 0
        release_catalogue = {}
        action_sound_catalogue = {}
        lane_event_catalogue = {}
        program_control_catalogue = {}
        stock_catalogue = {}
        worm_catalogue = {}
        readiness = {} if source_pace and action_clips else None
        basic_shot_ids = []
        basic_melee_ids = []
        held_fire_ids = []
        short_range_ids = []
        knight_catalogue = {}
        samurai_catalogue = {}
        ninja_catalogue = {}
        girl_catalogue = {}
        blue_catalogue = {}
        robot_catalogue = {}
        wing_catalogue = {}
        bomber_catalogue = {}
        grounded_catalogue = {}
        tank_catalogue = {}
        pair_catalogue = {}
        single_shot_catalogue = {}
        breath_catalogue = {}
        shared_bytes = sum(map(len, shared_files.values()))
        for row in rows:
            files, summary = package(archive, dol, row, preview_extras=preview_extras, role_clips=role_clips,
                                     source_variants=source_variants, move_labels=labels,
                                      launch_tables=launches, linker=linker, landing_pcm=pcm, origin_markers=origin_markers,
                                      action_clips=action_clips, cruise_lab=cruise_lab, movement_operands=movement_operands,
                                       source_pace=source_pace,basic_shot=basic_shot,stock_guards=stock_guards,
                                        launch_pcm=launch_pcm,projectile_sounds=projectile_sound["tables"],tone_banks=tone_banks,
                                        dragon_carrier=dragon_carriers["borgs"].get(row.id),
                                         dragon_impact_art=art_files["effects/common-single.hsd"],dragon_impact_pcm=impact_pcm,
                                         worm_source=worm_shots.get(row.model),basic_melee=basic_melee,
                                          action_pcm=action_pcm,contact_roles=contact_roles,
                                          contact_power_test=contact_power_test,contact_ladders=contact_ladders,
                                           contact_tiers=contact_tiers,contact_tier_reset_test=contact_tier_reset_test,
                                            contact_audio=contact_audio,contact_visuals=contact_visuals,
                                            contact_indices=contact_indices,contact_floor=contact_floor,
                                             contact_pause=contact_pause is not None,
                                             continuation_source=continuation_source,speed_ramp_source=speed_ramp_source,
                                              ninja_bomb_source=ninja_bomb_source,contact_tail_source=contact_tail_source,
                                               sasuke_air_source=sasuke_air_source,
                                                blue_striker_boost_source=blue_striker_boost_source,
                                                ko_force_source=ko_force_source)
            if contact_tail_source is not None and row.model in contact_tail_source and (
                    "projectile_contact_tail_test" not in summary.get("basic_shot_battle_test", {})):
                raise ValueError(f"type-0 contact-tail route did not enter its authored shot package: {row.id}")
            if "dragon_carrier_collision" in summary:
                dragon_carriers["borgs"][row.id]["collision"]=summary["dragon_carrier_collision"]
                dragon_carriers["borgs"][row.id]["visuals"]=summary["dragon_carrier_visuals"]
            if "worm_shot_source" in summary:
                worm_catalogue[row.id]=summary["worm_shot_source"]
            if "knight_close_b_source" in summary:
                knight_catalogue[row.id]=summary["knight_close_b_source"]
            if "samurai_close_b_source" in summary:
                samurai_catalogue[row.id]=summary["samurai_close_b_source"]
            if "ninja_close_b_source" in summary:
                ninja_catalogue[row.id]=summary["ninja_close_b_source"]
            if "girl_close_b_source" in summary:
                girl_catalogue[row.id]=summary["girl_close_b_source"]
            if "blue_close_b_source" in summary:
                blue_catalogue[row.id]=summary["blue_close_b_source"]
            if "robot_diver_close_b_source" in summary:
                robot_catalogue[row.id]=summary["robot_diver_close_b_source"]
            if "wing_close_b_source" in summary:
                wing_catalogue[row.id]=summary["wing_close_b_source"]
            if "bomber_hero_close_b_source" in summary:
                bomber_catalogue[row.id]=summary["bomber_hero_close_b_source"]
            if "grounded_close_b_source" in summary:
                grounded_catalogue[row.id]=summary["grounded_close_b_source"]
            if "tank_shot_source" in summary:
                tank_catalogue[row.id]=summary["tank_shot_source"]
            if "paired_ground_shot_source" in summary:
                pair_catalogue[row.id]=summary["paired_ground_shot_source"]
            if "single_ground_shot_source" in summary:
                single_shot_catalogue[row.id]=summary["single_ground_shot_source"]
            if "breath_source" in summary:
                breath_catalogue[row.id]={"source":summary["breath_source"],
                                         "native_field":summary.get("native_field")}
            total += sum(map(len, files.values()))
            if total + shared_bytes > ROSTER_LIMIT:
                raise ValueError("roster output byte budget")
            folder = output / row.folder
            folder.mkdir()
            write_new(folder, files)
            indexed = release_index(summary["action_candidates"])
            if indexed:
                attach_release_sfx(indexed,projectile_sound["tables"])
                attach_release_stock(indexed,linker)
                stock_guards.attach(indexed)
                release_catalogue[summary["id"]] = indexed
            if readiness is not None:
                readiness[summary["id"]] = shot_readiness(summary,indexed)
            stock_catalogue[summary["id"]] = stock_rows(summary["raw_level_rows"])
            if "basic_shot_battle_test" in summary:
                basic_shot_ids.append(summary["id"])
            if "basic_melee_battle_test" in summary:
                basic_melee_ids.append(summary["id"])
            if "held_fire_battle_test" in summary:
                held_fire_ids.append(summary["id"])
            if ("roach_bomb_source" in summary or "zeta_field_source" in summary or
                    "charge_field_source" in summary or "icbm_source" in summary or
                    "spawner_shot_source" in summary or "spawner_static_source" in summary or
                    "flying_field_source" in summary or "nu_field_source" in summary or
                    "staged_field_source" in summary):
                short_range_ids.append(summary["id"])
            cues = action_sfx_index(summary["action_candidates"],tone_banks)
            if cues:
                action_sound_catalogue[summary["id"]] = cues
            program_controls = controls.collect(summary["action_candidates"])
            if program_controls["programs"]:
                program_control_catalogue[summary["id"]] = program_controls
            if linker:
                lane_events = lane_event_index(summary["action_candidates"], linker)
                if lane_events:
                    lane_event_catalogue[summary["id"]] = lane_events
            count += summary["preview_declaration"] is not None
            variant_count += len(summary["selectable_variants"])-1
        release_file="shared-projectile-release-links.json"
        release_bytes=json_line({"kind":"static launch-origin review only; not engine content",
                                 "variant":"normal","live_launch_verified":False,"borgs":release_catalogue})
        action_sound_file="shared-action-sfx.json"
        action_sound_bytes=json_line({"kind":"static direct actor sound requests; not engine content",
                                      "variant":"normal","live_playback_verified":False,"borgs":action_sound_catalogue})
        lane_event_file="shared-lane-events.json"
        lane_event_bytes=json_line({"kind":"static descriptor lane events; not engine content",
                                    "variant":"normal","borgs":lane_event_catalogue})
        control_file="shared-action-program-controls.json"
        control_bytes=json_line({"kind":"static program writes; not engine content","writer":control_schema,
                                 "variant":"normal","borgs":program_control_catalogue})
        stock_file="shared-level-stock-operands.json"
        worm_file="shared-worm-shot-routes.json"
        stock_bytes=json_line({"kind":"normal stock initializer operands; not engine content",
                               "variant":"normal","schema":stock_operands_schema,"borgs":stock_catalogue})
        indexes={release_file:release_bytes,action_sound_file:action_sound_bytes,
                 lane_event_file:lane_event_bytes,control_file:control_bytes,stock_file:stock_bytes,
                 worm_file:json_line({"kind":"Worm ordinary B static review; not engine content","borgs":worm_catalogue}),
                 carrier_file:json_line(dragon_carriers)}
        if action_clips:
            knight_file="shared-knight-close-b.json"
            indexes[knight_file]=json_line({"kind":"Knight close B static review; not engine content","borgs":knight_catalogue})
            shared_index["knight_close_b_index"]=knight_file
            samurai_file="shared-samurai-close-b.json"
            indexes[samurai_file]=json_line({"kind":"Samurai neutral close B static review; not engine content","borgs":samurai_catalogue})
            shared_index["samurai_close_b_index"]=samurai_file
            ninja_file="shared-ninja-close-b.json"
            indexes[ninja_file]=json_line({"kind":"Ninja neutral close B static review; not engine content","borgs":ninja_catalogue})
            shared_index["ninja_close_b_index"]=ninja_file
            girl_file="shared-girl-close-b.json"
            indexes[girl_file]=json_line({"kind":"Girl-family neutral close B static review; not engine content","borgs":girl_catalogue})
            shared_index["girl_close_b_index"]=girl_file
            blue_file="shared-blue-close-b.json"
            indexes[blue_file]=json_line({"kind":"Blue/Atlas neutral close B static review; not engine content","borgs":blue_catalogue})
            shared_index["blue_close_b_index"]=blue_file
            robot_file="shared-robot-diver-close-b.json"
            indexes[robot_file]=json_line({"kind":"Robot/Diver neutral close B static review; not engine content","borgs":robot_catalogue})
            shared_index["robot_diver_close_b_index"]=robot_file
            wing_file="shared-wing-valkyrie-close-b.json"
            indexes[wing_file]=json_line({"kind":"Wing/Valkyrie neutral close B static review; not engine content","borgs":wing_catalogue})
            shared_index["wing_close_b_index"]=wing_file
            bomber_file="shared-bomber-hero-close-b.json"
            indexes[bomber_file]=json_line({"kind":"Bomber/Hero neutral close B static review; not engine content","borgs":bomber_catalogue})
            shared_index["bomber_hero_close_b_index"]=bomber_file
            grounded_file="shared-grounded-close-b.json"
            indexes[grounded_file]=json_line({"kind":"Additional grounded close B static review; not engine content",
                                              "borgs":grounded_catalogue})
            shared_index["grounded_close_b_index"]=grounded_file
            tank_file="shared-tank-b-shot.json"
            indexes[tank_file]=json_line({"kind":"Tank ordinary B firing route review; not engine content","borgs":tank_catalogue})
            shared_index["tank_b_shot_index"]=tank_file
            pair_file="shared-paired-ground-b-shot.json"
            indexes[pair_file]=json_line({"kind":"Ordinary B ordered-pair route review; not engine content","borgs":pair_catalogue})
            shared_index["paired_ground_b_shot_index"]=pair_file
            single_file="shared-single-ground-b-shot.json"
            indexes[single_file]=json_line({"kind":"Ordinary B single-shot route review; not engine content","borgs":single_shot_catalogue})
            shared_index["single_ground_b_shot_index"]=single_file
            breath_file="shared-dragon-breath.json"
            indexes[breath_file]=json_line({"kind":"Authored breath proxy over an exact paid program window","borgs":breath_catalogue})
            shared_index["dragon_breath_index"]=breath_file
        if readiness is not None:
            readiness_name = "shared-basic-shot-readiness.json"
            indexes[readiness_name] = json_line({"kind":"offline authored B-shot review; not retail admission",
                                                "variant":"normal","borgs":readiness})
            shared_index["basic_shot_readiness_index"] = readiness_name
        shared_bytes+=sum(map(len,indexes.values()))
        if total+shared_bytes>ROSTER_LIMIT:
            raise ValueError("roster release catalogue byte budget")
        write_new(output,indexes)
        shared_index["projectile_release_index"]=release_file
        shared_index["action_sfx_index"]=action_sound_file
        shared_index["lane_event_index"]=lane_event_file
        shared_index["action_program_control_index"]=control_file
        shared_index["level_stock_operand_index"]=stock_file
        shared_index["worm_shot_index"]=worm_file
        if basic_shot:
            shared_index["authored_basic_shot_test_ids"]=basic_shot_ids
            shared_index["authored_held_fire_test_ids"]=held_fire_ids
            shared_index["authored_short_range_test_ids"]=short_range_ids
        if basic_melee:
            shared_index["authored_basic_melee_test_ids"]=basic_melee_ids
        with (output / "roster-extraction.json").open("xb") as ledger:
            ledger.write(json_line({"revision": "GG4E08", "normal_packages": len(rows),
                                    "preview_declarations": count, "package_bytes": total,
                                    "source_variant_preview_declarations": variant_count,
                                    "shared_bytes": shared_bytes, "spec_move_text_imported": bool(labels), **shared_index}))
    return len(rows), count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("catalog", type=Path, help="qualified local catalog.gfbc")
    parser.add_argument("dol", type=Path, help="original GG4E08 main.dol")
    parser.add_argument("archive", type=Path, help="original GG4E08 afs_data.afs")
    parser.add_argument("output", type=Path, help="fresh local Borg install root")
    preview = parser.add_mutually_exclusive_group()
    preview.add_argument("--preview-extras", action="store_true",
                         help="declare up to seven raw clips; an unsupported extra may refuse the preview")
    preview.add_argument("--role-clips", action="store_true",
                         help="declare optional static movement-role candidates for the exact normal preview")
    parser.add_argument("--spec-text", type=Path, help="qualified local GG4E08 spec-text.gftx export (optional)")
    parser.add_argument("--source-variants", action="store_true",
                         help="declare raw exact numeric source-wrapper previews, not collectible color or gameplay variants")
    parser.add_argument("--landing-pcm", type=Path, metavar="DECODER",
                        help="use the local borg_movement_pcm helper to add provisional lab landing audio; requires --role-clips")
    parser.add_argument("--origin-markers", action="store_true",
                        help="include exact normal joint-local origin markers for preview inspection only")
    parser.add_argument("--action-clips", action="store_true",
                        help="include bounded named normal action-entry clip candidates for inspection only")
    parser.add_argument("--cruise-lab", action="store_true",
                        help="declare provisional cruise/boost for qualified exact normal type-3 bodies")
    parser.add_argument("--movement-operands", action="store_true",
                        help="index raw actor-record movement scalars and exact shared read-site candidates")
    parser.add_argument("--source-pace", action="store_true",
                        help="declare opt-in source-relative native ground pace; requires --role-clips")
    parser.add_argument("--basic-shot-test", action="store_true",
                         help="add a normal-only authored B-shot Battle Test to the qualified cohort")
    parser.add_argument("--projectile-continuation-test", action="store_true",
                        help="opt Machine Red's authored ordinary B shot into receiver continuation; requires --basic-shot-test")
    parser.add_argument("--projectile-speed-ramp-test", action="store_true",
                        help="opt Machine Red's authored ordinary B shot into a scaled speed ramp; requires --basic-shot-test")
    parser.add_argument("--projectile-contact-tail-test", action="store_true",
                         help="hold the spent carrier slot for three reviewed ordinary B shots; requires --basic-shot-test")
    parser.add_argument("--sasuke-air-points-test", action="store_true",
                         help="use separate jump points with dash seed and delayed reset for normal Sasuke; requires --basic-shot-test")
    parser.add_argument("--blue-striker-boost-test", action="store_true",
                         help="bind normal Blue Striker's unit-rate meter and exit threshold; requires --basic-shot-test and --cruise-lab")
    parser.add_argument("--ninja-bomb-test", action="store_true",
                         help="append an authored grounded X bomb for normal Normal Ninja and Sasuke; requires --basic-shot-test")
    parser.add_argument("--basic-melee-test", action="store_true",
                         help="add a normal-only authored close-B Battle Test to the qualified melee cohort")
    parser.add_argument("--ko-force-cost-test", action="store_true",
                         help="declare normal Tau's verified KO cost; requires --basic-melee-test")
    parser.add_argument("--contact-scale-test", action="store_true",
                         help="use the authored paired settings-7/rank-0 damage snapshot in Battle Test packages")
    parser.add_argument("--contact-index-test", action="store_true",
                         help="bind complete settings/rank curves; requires --contact-scale-test")
    parser.add_argument("--contact-power-test", action="store_true",
                         help="bind the recovered uniform positive HP base on reviewed direct Battle Test moves")
    parser.add_argument("--contact-floor-test", action="store_true",
                         help="keep qualified positive direct hit damage from truncating below one HP")
    parser.add_argument("--contact-pause-test", action="store_true",
                         help="hold native skeletal samples on verified direct-hit slowdown routes")
    parser.add_argument("--contact-ladder-test", action="store_true",
                         help="bind qualified receiving and outgoing HP/force ladders; requires --contact-scale-test")
    parser.add_argument("--contact-tier-test", action="store_true",
                         help="bind the receiver-owned contact meter and damage tiers; requires --contact-scale-test")
    parser.add_argument("--contact-tier-reset-test", action="store_true",
                         help="bind the separate gauge that can reset the tier; requires --contact-tier-test")
    parser.add_argument("--basic-shot-audio", type=Path, metavar="WAV_DECODER",
                         help="bind exact dry launch tones using the local retail_borg_sound helper")
    parser.add_argument("--contact-audio", type=Path, metavar="WAV_DECODER",
                         help="bind ordinary direct-contact tone pairs using the local retail_borg_sound helper")
    parser.add_argument("--contact-art-test", action="store_true",
                         help="show qualified common impact records at accepted normal direct contacts")
    parser.add_argument("--dragon-impact-audio", action="store_true",
                        help="bind an authored accepted-contact cue for elemental Dragon tests")
    args = parser.parse_args()
    try:
        normal, previews = convert(args.catalog, args.dol, args.archive, args.output,
                                   preview_extras=args.preview_extras, role_clips=args.role_clips, spec_text=args.spec_text,
                                    source_variants=args.source_variants, landing_decoder=args.landing_pcm,
                                    origin_markers=args.origin_markers, action_clips=args.action_clips, cruise_lab=args.cruise_lab,
                                     movement_operands=args.movement_operands, source_pace=args.source_pace,basic_shot=args.basic_shot_test,
                                       shot_audio_decoder=args.basic_shot_audio,dragon_impact_audio=args.dragon_impact_audio,
                                        basic_melee=args.basic_melee_test,contact_scale_test=args.contact_scale_test,
                                        contact_power_test=args.contact_power_test,contact_ladder_test=args.contact_ladder_test,
                                        contact_tier_test=args.contact_tier_test,
                                         contact_tier_reset_test=args.contact_tier_reset_test,contact_audio_decoder=args.contact_audio,
                                          contact_art_test=args.contact_art_test,contact_index_test=args.contact_index_test,
                                           contact_floor_test=args.contact_floor_test,contact_pause_test=args.contact_pause_test,
                                           projectile_continuation_test=args.projectile_continuation_test,
                                            projectile_speed_ramp_test=args.projectile_speed_ramp_test,
                                             ninja_bomb_test=args.ninja_bomb_test,
                                              projectile_contact_tail_test=args.projectile_contact_tail_test,
                                               sasuke_air_points_test=args.sasuke_air_points_test,
                                                blue_striker_boost_test=args.blue_striker_boost_test,
                                                ko_force_cost_test=args.ko_force_cost_test)
    except (ValueError, OSError) as exc:
        parser.exit(1, f"retail roster extraction: {exc}\n")
    print(f"GG4E08: {normal} normal asset packages, {previews} preview declarations (not runtime-validated)")


if __name__ == "__main__":
    main()
