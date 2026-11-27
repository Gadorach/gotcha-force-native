"""Qualify the mode-4 body link for ordinary direct factory allocations."""
import struct

# (constructor, saved input actor, allocation call, load +90, store +90).
# The projectile allocation is non-body; its separate +8C parent is not
# what 800300BC reads. These are ordinary normal-fighter release routes.
_FACTORY = {
    "type-0": (0x80082824, 0x8008283C, 0x80082868, 0x800828A8, 0x800828AC, 31, 3),
    "type-49": (0x8013A050, 0x8013A078, 0x8013A07C, 0x8013A0C4, 0x8013A0C8, 30, 3),
    "type-94": (0x801D4D00, 0x801D4D18, 0x801D4D44, 0x801D4D88, 0x801D4D8C, 31, 6),
}


def _check(image, expected):
    for site, value in expected.items():
        word, = struct.unpack(">I", image.read(site, 4, "text"))
        if word != value:
            raise ValueError(f"projectile body-link proof changed at {site:08X}")


def qualify(image, family):
    if family not in _FACTORY:
        return None
    factory, save, allocation, load, store, caster_reg, shot_reg = _FACTORY[family]
    _check(image, {
        0x80053754: 0x939C0090,  # normal actor's self link
        0x800537D0: 0x38600000,
        0x800537D8: 0x987E0083,  # normal body tag
        0x80088B58: 0x981F0083,  # carrier pool tag
        factory + 8: 0x38A00008,  # ordinary carrier pool base
        save: 0x7C601B78 | (caster_reg << 16),
        allocation: 0x48000001 | ((0x80088AA0 - allocation) & 0x03FFFFFC),
        load: 0x80000090 | (caster_reg << 16),
        store: 0x90000090 | (shot_reg << 16),
        0x8008AC9C: 0x83E30090,
        0x8008AD10: 0x80190090,
        0x8008AD1C: 0x90030024,  # pair caches +90, not +8C
        0x800301CC: 0x881C0083,
        0x800301D4: 0x40820020,
        0x800301F4: 0x807D0024,  # mode 4 secondary body link
        0x800301F8: 0x88030083,
        0x80030200: 0x40820020,
        0x80030204: 0xC00308DC,
        0x8003020C: 0xC00308EC,
        0x80030214: 0xC00308FC,
    })
    if shot_reg == 6:
        _check(image, {allocation + 4: 0x7C661B79})  # allocated cell copied r3 -> r6
    if image.read(0x802DA708, 7, "data") != b"\x10" * 7:
        raise ValueError("ordinary carrier pool tags changed")
    return {"constructor": f"{factory:08X}", "allocation": f"{allocation:08X}",
            "copy_from_caster": f"{load:08X}", "copy_to_carrier": f"{store:08X}",
            "normal_self_link": "80053754", "normal_body_tag": "800537D8",
            "carrier_tag": 0x10, "pair_link_store": "8008AD1C", "linked_root_read": "80030204"}
