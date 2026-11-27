"""Bind authored projectile reactions to the reviewed factory operands.

Input-context coordinates select controller rows, not collision queues.
The projectile initializer reads a signed queue selector from its factory
record. The normal owner supplies the hit block and the reaction table.
"""
from tools.analyze import reaction_attack_table, reaction_installer_tables, reaction_projectile_owner
from tools.analyze.retail_projectile_origins import TABLES
from tools.analyze.retail_worm_shot import FAMILIES as WORM_FAMILIES

HIT_BYTES = 7840
QUEUE = 0xA20
STEPS = 0xAA0
STEP_SIZE = 0x50
GROUPS = (STEPS - QUEUE) // 4
STEP_COUNT = (HIT_BYTES - STEPS) // STEP_SIZE

# Family -> (offset in factory row, instruction reading it, 8008AC80 call).
# These are the initializers for the ordinary factory allocations only.
SELECTORS = {
    # Operand offset, read site, allocator call, phase-zero dispatch slot.
    "type-0": (3, 0x80082EF8, 0x80082F00, 0x802D9748),
    "type-2": (2, 0x800C3D70, 0x800C3D80, 0x803051E4),
    "type-14": (3, 0x8006F1A4, 0x8006F1AC, 0x802D3AC4),
    "type-15": (3, 0x8007DDFC, 0x8007DE04, 0x802D6F98),
    "type-47": (3, 0x800BED3C, 0x800BED44, 0x80301C08),
    "type-49": (1, 0x8013A378, 0x8013A380, 0x80339708),
    "type-94": (2, 0x801D5198, 0x801D51A0, 0x8038CE08),
}
FACTORY_ROWS = {**{family: row[:3] for family, row in TABLES.items()},
                **{f"type-{family}": row[:3] for family, row in WORM_FAMILIES.items()}}


def signed_byte(value):
    return value - 256 if value >= 128 else value


def read_group(payload, group):
    if len(payload) != HIT_BYTES or not 0 <= group < GROUPS:
        raise ValueError("invalid normal hit block or reaction group")
    rows = []
    for raw in payload[QUEUE + 4 * group:QUEUE + 4 * group + 4]:
        if signed_byte(raw) < 0:
            break
        if raw >= STEP_COUNT:
            raise ValueError("reaction step outside normal hit block")
        # 8008ADB4 loads a halfword; 8008ADB8 stores its low byte at
        # pair+0x11, which the amount/reaction routine reads unsigned.
        rows.append((raw, payload[STEPS + STEP_SIZE * raw + 5]))
    return rows


def qualified_hit_block(archive, model):
    """For public maps, cross-check the installed PZZ slot with the loose member."""
    name = f"pl{model:04x}".encode("ascii")
    pzz, _ = archive.member(archive.names.index(name + b".pzz"), model)
    payload, _ = pzz.read(2, HIT_BYTES)
    loose = name + b"hit.bin"
    if (archive.names.count(loose) != 1 or len(payload) != HIT_BYTES or
            payload != archive.payload(archive.names.index(loose), HIT_BYTES)):
        raise ValueError(f"{model:04x}: normal hit block mismatch")
    return payload


def resolve_group(image, payload, table, group):
    if not 0 <= group < GROUPS:
        return None
    steps = read_group(payload, group)
    if not steps or any(index >= reaction_attack_table.COUNT for _, index in steps):
        return None
    return [{"step": step, **reaction_attack_table.entry(image, table, index)}
            for step, index in steps]


def resolve_release(image, payload, table, family, key):
    if family not in SELECTORS:
        return None
    base, stride, count = FACTORY_ROWS[family]
    if not 0 <= key < count:
        return None
    offset, read_site, call_site, _ = SELECTORS[family]
    address = base + key * stride + offset
    group = signed_byte(image.read(address, 1, "data")[0])
    steps = resolve_group(image, payload, table, group)
    if steps is None:
        return None
    link = reaction_projectile_owner.qualify(image, family) if all(s["mode"] == 4 for s in steps) else None
    return {
        "family": family,
        "key": key,
        "selector_address": f"{address:08X}",
        "read_site": f"{read_site:08X}",
        "allocator_call": f"{call_site:08X}",
        "queue_group": group,
        "steps": steps,
        **({"linked_body_proof": link} if link else {}),
    }


def move_binding(image, payload, table, move, releases):
    """One native move can express only one direction mode and offset."""
    if not releases or len(releases) > 6:
        return None
    resolved = [resolve_release(image, payload, table, family, key) for family, key in releases]
    root = "linked-owner" if all(route and route.get("linked_body_proof") for route in resolved) else None
    lines = uniform_reaction_lines(move, resolved, root=root)
    return {"table": f"{table:08X}", "releases": resolved, "lines": lines} if lines else None


def uniform_reaction_lines(move, routes, *, root=None):
    if any(route is None for route in routes):
        return None
    values = {(step["mode"], step["yaw"], step["pitch"])
              for route in routes for step in route["steps"]}
    if len(values) != 1:
        return None
    mode, yaw, pitch = values.pop()
    lines = []
    if mode == 4 and root == "melee":
        lines.append("# source reaction mode 4 uses the native planar body root")
    elif mode == 4 and root == "linked-owner":
        lines.extend(("# source reaction mode 4 uses the direct projectile owner's native planar body root",
                      f"reaction-root={move}|linked-owner"))
    elif mode not in (0, 1, 2, 3):
        lines.append(f"# source reaction mode {mode} not native; fallback mode 1")
        mode = 1
    lines.extend((f"reaction-mode={move}|{mode}", f"reaction-offset={move}|{yaw}|{pitch}"))
    return lines


def append_bindings(image, payload, model, gameplay, receipt):
    """Emit only from releases already joined to a move by its package builder."""
    moves = receipt.get("reaction_releases")
    if not moves:
        return gameplay
    _, table = reaction_installer_tables.model_table(image, model)
    bindings = {}
    declined = []
    for move, releases in moves.items():
        if not any(line.startswith(f"move={move}|".encode("ascii")) for line in gameplay.splitlines()):
            raise ValueError(f"reaction release binding references undeclared move: {move}")
        binding = (move_binding(image, payload, table, move, releases)
                   if table is not None and len(payload) == HIT_BYTES else None)
        if binding is None:
            declined.append(move)
        else:
            bindings[move] = binding
            gameplay += ("\n".join(binding["lines"]) + "\n").encode("ascii")
    if bindings:
        receipt["reaction"] = bindings
    if declined:
        receipt["reaction_unbound_moves"] = declined
    return gameplay
