"""Pinned type-89 constructor, selector rows, and area-key write/read chain."""
from hashlib import sha256
import struct

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import address
from tools.extract.battle_initial_target import SOURCE_SHA256


FACTORY, UPDATE, INITIALIZE, EXIT = 0x8016CC24, 0x8016CD1C, 0x8016CDD0, 0x8016D2D0
TABLE, STRIDE, COUNT = 0x8035029C, 28, 17
TABLE_SHA256 = "b14c251ad0c664e38b39424f8789a9ffd7e065eee93b36db98876d44048ec89f"
ROOTS = {0x80350624: (INITIALIZE, 0x8016CFE8, 0x8016D364, 0x8016D378),
         0x80350634: (0x8016D060, EXIT, 0x8016D344)}
AREA_SITES = {0x8016D30C: 0x8016D300, 0x8016D32C: 0x8016D320}
TEXT = (
    (FACTORY, 248, "074f05c5ea8e2dfde9efc8fa6da0b15d6ec72f8fefd11a8a5c16094703e501f5"),
    (UPDATE, 180, "65a5f4dea558377b9dce2ff378c0dbd2c69a9a1303b838525fb5c6c52a06cb2d"),
    (INITIALIZE, 536, "155ed524f83fd8316c94d524fddaaadd4e1475848419d670005af9b5ebc0168e"),
    (0x8016CFE8, 120, "c7f5d13c99613070ef9abb7e6772b6a715c08a393a21fa19ef3a235a44143dfc"),
    (EXIT, 116, "5aaced7096314bbcedb4492f92d89c12bfbeb0d5d79cccc8a3503c8fc9b31768"),
)
ANCESTOR_TEXT = (
    (0x801B372C, 60, "6488ed1ee49a6a4d6061d75cbb2e882cc3aaafdf0a0adf1577564c5a1701467b"),
    (0x801B3824, 364, "5a334ecea99c3908c24afe22745a1f86e8db24e5a6188d254e831cf48828154e"),
    (0x8018ED4C, 52, "c14182d0a842bc4bea25e06ea22adaebe10135949ae2752896a60469c7247451"),
    (0x8018EE08, 40, "6b1be6e9f9893e7d70de0c4a13dde767d2fe3da02b7dea2b072c55767993524c"),
    (0x8018EE30, 84, "e3e2b96992f32da7f6805f2a4237499528292831a96880ad819bbd108faebd92"),
    (0x80179D20, 56, "ebaabd2d612e41fd70e6edee0db1a305fee9178e975879e70ccbdcf09855315a"),
    (0x80179D58, 264, "4f68154533371943319e2342658cf27e287b4ce43c1bc79b8384ceb03aac2372"),
)


def word(image, at, kind="text"):
    return struct.unpack(">I", image.read(at, 4, kind))[0]


def link(image, site, target):
    value = word(image, site)
    if value & 0xFC000003 != 0x48000001 or direct_branch(site, value) != target:
        raise ValueError(f"type-89 call mismatch at {address(site)}")


def verify_text(image, spans):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("type-89 source identity")
    for at, size, digest in spans:
        if sha256(image.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"type-89 source signature mismatch at {address(at)}")


def rows(image):
    verify_text(image, TEXT)
    for at, expected in ROOTS.items():
        if struct.unpack(f">{len(expected)}I", image.read(at, 4*len(expected), "data")) != expected:
            raise ValueError(f"type-89 dispatch mismatch at {address(at)}")
    raw = image.read(TABLE, STRIDE*COUNT, "data")
    if (sha256(raw).hexdigest() != TABLE_SHA256
            or image.read(TABLE+len(raw), 8, "data") != bytes.fromhex("000000003f800000")):
        raise ValueError("type-89 bounded row extent or identity")
    if (word(image, 0x8016CC68) != 0x38000059 or word(image, 0x8016CE44) != 0x881F0003
            or word(image, 0x8016CE48) != 0x981C017D):
        raise ValueError("type-89 definition or cached area writer changed")
    for pc, at in AREA_SITES.items():
        if word(image, at) != 0x88BF017D:
            raise ValueError("type-89 cached area reader changed")
        link(image, pc, 0x80085E00)
    result = []
    for key in range(COUNT):
        at = key*STRIDE
        group, area = raw[at+2:at+4]
        if group >= 32 or area != 16:
            raise ValueError("type-89 group or area domain changed")
        result.append({"key": key, "row": address(TABLE+at), "group": group, "area_key": area})
    return result


def ancestors(image, atlas, selections):
    verify_text(image, ANCESTOR_TEXT)
    models = {r["id"]: r for r in selections["borgs"]}
    nu, black = (models[m] for m in ("gg4e08:0f03", "gg4e08:062a"))
    if (nu["profile"], black["profile"], nu["spec_record"], black["spec_record"]) != ("p166", "p089", 0, 0):
        raise ValueError("type-89 scoped normal installer selections changed")
    for pilot, action, family in ((nu, "B-shot", "801B372C"), (black, "B-charge", "8018ED4C")):
        p = atlas["profiles"][pilot["profile"]]
        if not any(r.get("code_path", {}).get("family") == family
                   for g in p["groups"] if g["group"] == action for r in g["rows"]):
            raise ValueError("type-89 scoped action family changed")
    data = {0x80380EF0: 0x801B3824, 0x80365874: 0x8018EE30, 0x804347A8: 0x80179D58}
    if any(word(image, at, "data") != expected for at, expected in data.items()):
        raise ValueError("type-89 scoped ancestor dispatch changed")
    for site, target in ((0x801B392C, 0x8006DBE0), (0x801B395C, FACTORY), (0x8018EE54, FACTORY),
                         (0x8018ED6C, 0x8018EE08), (0x8018EE1C, 0x80179D20)):
        link(image, site, target)
    if word(image, 0x801B3958) != 0x3880000B or word(image, 0x8018EE50) != 0x38800010:
        raise ValueError("type-89 scoped parent operand changed")
    result = []
    for pilot, site, key, scope, extras in (
        (nu, "801B395C", 11, "ordinary B shared state-two paid branch, exact normal Nu III", {"debit": "801B392C"}),
        (black, "8018EE54", 16, "normal G-Black model-guarded B-charge callback", {}),
    ):
        result.append({
            "borg": pilot["id"], "profile": pilot["profile"], "ancestor_scope": scope,
            "parent": {"family": "type-89", "key": key, "site": site, **extras},
            "descendant": {"key": 16, "sites": [address(pc) for pc in AREA_SITES],
                           "scope": "type-89 exit with a cached selector; contact and alternate gates are separate"},
        })
    return result
