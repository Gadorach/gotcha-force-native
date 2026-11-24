"""Export a reviewed normal retail body preview and non-executable metadata."""
import argparse
from dataclasses import dataclass
from hashlib import sha256
from io import BytesIO
from pathlib import Path
import struct

from tools.extract.pzz import PzzArchive
from tools.extract.retail_borg_placeholders import open_regular, package_text, read_catalog
from tools.extract.source_image import DolImage


DOL_SHA256 = "21454a8880c6254616662710d6ec626e43d853999da2cdfad700030960a94836"
DOL_BYTES = 8 << 20
ACTOR_BYTES = 432


@dataclass(frozen=True)
class MoveEvidence:
    move: str
    phases: tuple
    outcomes: tuple = ()
    stockless: bool = False
    form: str = "base"


@dataclass(frozen=True)
class FormTransitionReview:
    id: str
    source: str
    destination: str
    kind: str
    trigger: str
    event: str
    guard: str = "unresolved"
    timer: str = "unresolved"


DRAGON_BREATH_PHASES = (
    ("visual-request", "stockless-program-cue", "unresolved", "unresolved", "unresolved",
     "not-applicable", "not-applicable", "not-applicable"),
    ("contact-child-request", "active-visual-object", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
)
DRAGON_BOMB_PHASES = (
    ("impact-handoff", "accepted-carrier-contact", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
)
DRAGON_STORM_PHASES = DRAGON_BOMB_PHASES + (
    ("emitter-bind", "qualified-group-seven-bind", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
    ("descendant-bind", "accepted-descendant-allocation", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
)
DRAGON_STORM_OUTCOMES = (
    ("emitter-hit", "qualified-group-seven-contact", "damage", "contact-target", "unresolved", "unresolved"),
    ("descendant-hit", "qualified-group-eight-contact", "damage", "contact-target", "unresolved", "unresolved"),
)

# The checker models one result package; this source reconfigures two slots.
# Do not use the separate catalogue entry 050f as an alias for either result.
MACHINE_DRAGON_FUSION_REVIEW = (
    "GFBORG-FUSIONS 1\n"
    "# GG4E08 battle stages internal primary 0513/0 and companion 0514/0, not catalogue 050F/0.\n"
    "fusion=gg4e08:machine-dragon-battle\n"
    "first=gg4e08:0511|unresolved|normal\n"
    "second=gg4e08:0512|unresolved|normal\n"
    "result=unresolved|unresolved|unresolved\n"
    "result-use=fusion-only\n"
    "trigger=conditioned-fusion-request\nguard=eligible-two-slot-match\n"
    "synchronize=linked-readiness\ncommit=both-slots-stage-seven\n"
    "interrupt=unresolved\nseparation=guarded-per-owner-restore\n"
    "duration=eligible-team-clock\nentry=unresolved|unresolved|unresolved\n"
    "exit=unresolved|unresolved|unresolved\ntransfer=unresolved|unresolved\n"
)

# Only the named primary fits this catalogue relationship. The companion
# rebind is required in battle but is not a separate retail collection ID.
SEIRYU_FUSION_REVIEW = (
    "GFBORG-FUSIONS 1\n"
    "# GG4E08 battle also reconfigures the other owner as internal companion 0603/0.\n"
    "# This row names only the catalogue primary and does not implement the paired rebind.\n"
    "fusion=gg4e08:seiryu-battle\n"
    "first=gg4e08:0600|unresolved|normal\n"
    "second=gg4e08:0601|unresolved|normal\n"
    "result=gg4e08:0602|unresolved|normal\n"
    "result-use=fusion-only\n"
    "trigger=conditioned-fusion-request\nguard=eligible-two-slot-match\n"
    "synchronize=linked-readiness\ncommit=both-slots-stage-seven\n"
    "interrupt=pre-commit-stage-clear\nseparation=guarded-per-owner-restore\n"
    "duration=eligible-team-clock\nentry=unresolved|unresolved|unresolved\n"
    "exit=unresolved|unresolved|unresolved\ntransfer=unresolved|unresolved\n"
)

CYBER_GIRL_REMOTE_PHASES = (
    ("indexed-dependent-init", "class-initialization", "unresolved", "unresolved", "unresolved",
     "not-applicable", "not-applicable", "not-applicable"),
    ("remote-object-request", "qualified-cue-and-owner-mask", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
)
DRAGON_BLACK_HOLE_REVIEW = MoveEvidence("black-hole", (
    ("carrier-request", "qualified-charged-x-cue", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
    ("startup-travel", "accepted-carrier-bind", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
    ("owner-slot-publication", "qualified-growth-gate", "unresolved", "unresolved", "unresolved",
     "unresolved", "unresolved", "unresolved"),
))


@dataclass(frozen=True)
class Review:
    model: int
    name: str
    member: int
    digest: str
    jump_fields: tuple
    move_slots: tuple
    moves: tuple
    move_note: str
    combat_role: str = "unresolved"
    display_jump: str = "unresolved"
    catalog_note: str = ""
    stock_columns: tuple = ()  # (move ID, original level-row index), display only
    clips: tuple = (("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"))
    controller_air_resource: str = ""  # Empty means do not generate a review row.
    controller_source_type: int = 0
    controller_selector_family: str = "cohort-a"
    controller_movement: str = "grounded"
    controller_recovery: str = "unresolved"
    move_evidence: tuple = ()
    static_preview: bool = False
    controller_pose: str = "clip-based"
    forms: tuple = ()
    transitions: tuple = ()
    actor_records: int = 1
    fusion_review: str = ""


REVIEWS = {
    "double-ninja": Review(
        0x0003, "Double Ninja", 488, "3df336fbec44f9edd3dfa4f9bf650f222f4f14820e94935241775987b952491e",
        (3, 0, 0), (1, 2, 255, 10, 0),
        (
            ("shinobi-gatana", "b", "melee", "Shinobi Gatana", "unresolved"),
            ("shuriken", "b", "projectile", "Shuriken", "unresolved"),
            ("shinobi-bomb", "x", "projectile", "Shinobi Bomb", "unresolved"),
            ("ninpo-kage-bunshin", "x", "special", "Ninpo Kage Bunshin", "charge"),
        ),
        "Display names: https://gotchaforce.fandom.com/wiki/Double_Ninja?oldid=6397",
        display_jump="air:3",
        catalog_note="Air-jump label: https://gotchaforce.fandom.com/wiki/Double_Ninja?oldid=6397",
    ),
    "gatling-gunner": Review(
        0x0102, "Gatling Gunner", 500, "e46ecb42599c8fafe923f6de1dbcd0a86fe5df1c03ec92b12890334e31466808",
        (0, 0, 0), (1, 255, 255, 10, 255),
        (
            ("special-gatling", "b", "projectile", "Special Gatling", "hold"),
            ("hand-grenade", "x", "projectile", "Hand Grenade", "unresolved"),
        ),
        "Display names: GG4E08 specification text; B stock endpoints: original normal level rows.",
        combat_role="long-range",
        catalog_note="No jump count glyph in the checked source specification row; do not infer a movement controller.",
        stock_columns=(("special-gatling", 1),),
    ),
    "revolver-gunman": Review(
        0x0100, "Revolver Gunman", 498, "4f38e4f0aec343a2b735e2f1d2c06648fca4f0c99b5e471773496b7dd24e1896",
        (0, 0, 0), (1, 4, 255, 1, 255),
        (
            ("fast-shooting", "b", "projectile", "Fast Shooting", "unresolved"),
            ("punch-kick", "b", "melee", "Punch Kick", "unresolved"),
            ("double-recoil", "x", "projectile", "Double Recoil", "unresolved"),
        ),
        "Display names: GG4E08 specification text; Fast Shooting stock: original normal level rows.",
        combat_role="long-range",
        stock_columns=(("fast-shooting", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-3-0", "3:0")),
        controller_air_resource="unresolved",
    ),
    "death-borg-beta-ii": Review(
        0x0105, "Death Borg Beta II", 503, "8637a483c078ac33c3799b1dd5f85f40f3593d659ae36d707fef67a56b91328e",
        (0, 0, 0), (1, 255, 255, 1, 255),
        (
            ("shot", "b", "projectile", "Shot", "unresolved"),
            ("double-shot", "x", "projectile", "Double Shot", "unresolved"),
        ),
        "Display names: GG4E08 specification text; Shot stock: original normal level rows.",
        combat_role="long-range",
        stock_columns=(("shot", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-3-0", "3:0")),
        controller_air_resource="unresolved",
    ),
    "normal-knight": Review(
        0x020A, "Normal Knight", 519, "21400dd6158a2a64af1ea7b4db04752f1ee0268f9144c4a834846c4e3b95892b",
        (1, 6, 1), (255, 2, 2, 1, 255),
        (
            ("broad-sword", "b", "melee", "Broad Sword", "unresolved"),
            ("cross-shape-slash", "b", "unresolved", "Cross-Shape Slash", "charge"),
            ("shield-slash", "x", "special", "Shield Slash", "unresolved"),
        ),
        "Display names: GG4E08 specification text; checked Shield Slash channel 2 capacity: original normal level rows.",
        combat_role="short-range",
        stock_columns=(("shield-slash", 5),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        controller_air_resource="air-jumps:3",
    ),
    "normal-samurai": Review(
        0x0700, "Normal Samurai", 606, "f07968fb48c3c5a557ecfa70f48ae7706d3a97bb88369440ed160c5b968cc1a2",
        (1, 0, 0), (1, 3, 255, 3, 255),
        (
            ("shinku-slash", "b", "projectile", "Shinku Slash", "unresolved"),
            ("nitoryu", "b", "melee", "Nitoryu", "unresolved"),
            ("rolling-slash", "x", "melee", "Rolling Slash", "unresolved"),
        ),
        "Display names: GG4E08 specification text; actor-owned shape paths are review only.",
        combat_role="short-range",
        # 0:0 and 0:1 use unsupported AObj bindings. State 0:2 is the
        # explicitly selected *inspection* start, not a gameplay idle role.
        clips=(("idle", "0:2"), ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="air-jumps:3",
    ),
    "normal-tank": Review(
        0x0C00, "Normal Tank", 666, "928af7ae6f10239e40f0c95903d8cbfcc27127e4c6b5b4fe169252cdc5d069f3",
        (0, 4, 0), (1, 255, 255, 1, 255),
        (
            ("machine-gun", "b", "projectile", "Machine Gun", "unresolved"),
            ("cannonball", "x", "projectile", "Cannonball", "unresolved"),
        ),
        "Display names: GG4E08 specification text; no release or hit behavior is bound.",
        combat_role="long-range",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Normal_Tank?oldid=6435",
        display_jump="boost",
        # The host refuses this exact wrapper on its published record 1 POBJ.
        # Never turn an unsupported render/attachment into a missing mesh.
        clips=(),
        controller_air_resource="boost-meter",
        controller_source_type=1,
        controller_selector_family="cohort-b",
        controller_movement="unresolved",
        controller_recovery="grounded",
    ),
    "battle-girl": Review(
        0x0300, "Battle Girl", 525, "dba3c291d9cb5ac70494a959b0cf7aa43e375ee8b3a96ef01f721c649adab309",
        (2, 0, 0), (1, 2, 255, 6, 255),
        (
            ("sub-machine-gun", "b", "projectile", "Sub Machine Gun", "unresolved"),
            ("beam-knife", "b", "melee", "Beam Knife", "unresolved"),
            ("missile-launcher", "x", "projectile", "Missile Launcher", "unresolved"),
        ),
        "Display names: GG4E08 specification text; no release or hit behavior is bound.",
        combat_role="speed",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Battle_Girl?oldid=6750",
        display_jump="boost",
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        controller_air_resource="boost-meter",
        controller_selector_family="cohort-b",
        controller_recovery="grounded",
    ),
    "thunder-robot": Review(
        0x0404, "Thunder Robot", 543, "ab20c337299193f1a226c00f724c7eec1fce9aa910b36dbb1d42b3486da44fcf",
        (1, 0, 0), (24, 4, 255, 11, 255),
        (
            ("thunder-beam", "b", "projectile", "Thunder Beam", "unresolved"),
            ("thunder-claw", "b", "melee", "Thunder Claw", "unresolved"),
            ("thunder-wire", "x", "special", "Thunder Wire", "unresolved"),
        ),
        "Display names: GG4E08 specification text; Thunder Beam stock: original normal level rows.",
        combat_role="short-range",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Thunder_Robot?oldid=6730",
        stock_columns=(("thunder-beam", 1),),
        clips=(("idle", "0:0"), ("clip-0-2", "0:2"), ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="unresolved",
        controller_selector_family="machine-04",
        controller_movement="unresolved",
    ),
    "magnet-robot-plus": Review(
        0x0405, "Magnet Robot (+)", 544, "8bcc17e248814735035a9ab7722aeece2705c2bb854d4826b3ce798aebcb0fa9",
        (4, 0, 0), (1, 4, 255, 12, 255),
        (
            ("machine-gun", "b", "projectile", "Machine Gun", "unresolved"),
            ("magnet-arm", "b", "melee", "Magnet Arm", "unresolved"),
            ("magnet-scope", "x", "special", "Magnet Scope", "unresolved"),
        ),
        "Display names: GG4E08 specification text; Machine Gun/Magnet Scope stock: original normal level rows.",
        combat_role="support",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Magnet_Robot_%28%2B%29?oldid=6672",
        stock_columns=(("machine-gun", 1), ("magnet-scope", 5)),
        clips=(("idle", "0:0"), ("clip-0-2", "0:2"), ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="unresolved",
        controller_selector_family="machine-04",
        controller_movement="unresolved",
    ),
    "magnet-robot-minus": Review(
        0x0409, "Magnet Robot (-)", 548, "671042981fcb93c80ab9e5327ce3cd3a43dfcb754cea1373dd903ffd28d5b0f2",
        (4, 0, 0), (24, 4, 255, 12, 255),
        (
            ("beam", "b", "projectile", "Beam", "unresolved"),
            ("magnet-arm", "b", "melee", "Magnet Arm", "unresolved"),
            ("magnet-scope", "x", "special", "Magnet Scope", "unresolved"),
        ),
        "Display names: GG4E08 specification text; Beam/Magnet Scope stock: original normal level rows.",
        combat_role="support",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Magnet_Robot_%28-%29?oldid=6433",
        stock_columns=(("beam", 1), ("magnet-scope", 5)),
        clips=(("idle", "0:0"), ("clip-0-2", "0:2"), ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="unresolved",
        controller_selector_family="machine-04",
        controller_movement="unresolved",
    ),
    "kung-fu-master": Review(
        0x0800, "Kung-Fu Master", 620, "a46ff3f12c1ceffc0c8ee51981060b1df35ac1b151a5f3384caa4972051c449b",
        (1, 0, 0), (1, 4, 255, 4, 255),
        (
            ("spirit-shot", "b", "projectile", "Spirit Shot", "press"),
            ("cosmic-kenpo", "b", "melee", "Cosmic Kenpo", "press"),
            ("senpuken", "x", "unresolved", "Senpuken", "unresolved"),
        ),
        "GG4E08 display names/level rows and reviewed ordinary B program/debit phases.",
        combat_role="short-range",
        stock_columns=(("spirit-shot", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-3-11", "3:11")),
        controller_air_resource="air-jumps:3",
        move_evidence=(
            MoveEvidence("spirit-shot", (
                ("selection", "conditioned-b", "unresolved", "unresolved", "unresolved", "not-applicable",
                 "not-applicable", "not-applicable"),
                ("launch-request", "program-event-3-1", "unresolved", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved"),
                ("recovery", "signed-recovery-event", "unresolved", "unresolved", "unresolved", "not-applicable",
                 "not-applicable", "not-applicable"),
            )),
            MoveEvidence("cosmic-kenpo", (
                ("selection", "conditioned-close-b", "unresolved", "unresolved", "unresolved", "not-applicable",
                 "not-applicable", "not-applicable"),
            )),
        ),
    ),
    "angel-nurse": Review(
        0x0900, "Angel Nurse", 634, "4c6e3c73caf7640ca7b119644c96dd8fcc44304717dcaf1509d367e8f5902f16",
        (4, 0, 0), (1, 19, 255, 25, 255),
        (
            ("nurse-shot", "b", "projectile", "Nurse Shot", "unresolved"),
            ("nurse-attack", "b", "melee", "Nurse Attack", "unresolved"),
            ("healing", "x", "special", "Healing", "unresolved"),
        ),
        "GG4E08 display names/level rows and reviewed X setup, shape request and recipient gate.",
        combat_role="support",
        stock_columns=(("nurse-shot", 1), ("healing", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0"),
               ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        move_evidence=(MoveEvidence("healing", (
            ("setup", "fallback-program-request", "unresolved", "unresolved", "unresolved", "not-applicable",
             "not-applicable", "not-applicable"),
            ("shape-request", "lane-gated-program-command", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
        ), (("heal", "qualified-ally-contact", "unresolved", "ally-hit", "unresolved", "unresolved"),)),),
    ),
    "patra-witch": Review(
        0x0902, "Patra Witch", 636, "d04b4df7ea1be145b31d2c2fa5ea84dd49742646293c4c7ee49a5ac4283780b3",
        (4, 0, 0), (1, 19, 255, 8, 255),
        (
            ("magic-shot", "b", "projectile", "Magic Shot", "unresolved"),
            ("stick-of-enlarge", "b", "melee", "Stick of Enlarge", "unresolved"),
            ("big-spell", "x", "projectile", "Big Spell", "unresolved"),
        ),
        "GG4E08 display names/level rows and reviewed X launch/recipient-effect requests.",
        combat_role="support",
        stock_columns=(("magic-shot", 1), ("big-spell", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0"),
               ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        move_evidence=(MoveEvidence("big-spell", (
            ("cast-request", "program-event-1-1", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
        ), (("recipient-effect", "qualified-carrier-contact", "unresolved", "contact-target",
             "unresolved", "unresolved"),)),),
    ),
    "acceleration-ninja": Review(
        0x0004, "Acceleration Ninja", 489, "05c603f66e2ed66d10d84c47ad18fa0f73e8b4b903f47e48484caedd7b6af0fc",
        (2, 0, 0), (1, 255, 0, 0, 255),
        (
            ("kamaitachi", "b", "projectile", "Kamaitachi", "unresolved"),
            ("midare-kamaitachi", "b", "unresolved", "Midare Kamaitachi", "charge"),
            ("ninpo-full-throttle", "x", "special", "Ninpo Full Throttle", "press"),
        ),
        "GG4E08 display names and reviewed fresh-X stance/shape proposals.",
        combat_role="speed",
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        move_evidence=(MoveEvidence("ninpo-full-throttle", (
            ("stance-entry", "conditioned-x", "unresolved", "unresolved", "unresolved", "not-applicable",
             "not-applicable", "not-applicable"),
            ("shape-request", "lane-gated-program-command", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        ), (("stance-contact", "qualified-shape-contact", "passive-contact", "contact-target", "unresolved",
             "unresolved"),)),),
    ),
    "pop-honey": Review(
        0x0904, "Pop Honey", 638, "2d633f1980c2dba9f3bfcb7e54aaaac907ac830a18315087734ff6c3db7c599c",
        (4, 0, 0), (1, 19, 255, 8, 255),
        (
            ("honey-shot", "b", "projectile", "Honey Shot", "unresolved"),
            ("honey-mike", "b", "melee", "Honey Mike", "unresolved"),
            ("lovely-voice", "x", "special", "Lovely Voice", "unresolved"),
        ),
        "GG4E08 display names, normal level rows and scoped Lovely Voice recipient behavior.",
        combat_role="support",
        stock_columns=(("lovely-voice", 5),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0"),
               ("clip-4-1", "4:1")),
        controller_air_resource="air-jumps:3",
        move_evidence=(MoveEvidence("lovely-voice", (
            ("paid-setup", "conditioned-x", "unresolved", "unresolved", "unresolved", "not-applicable",
             "not-applicable", "not-applicable"),
            ("carrier-wake", "paid-program-cue", "unresolved", "unresolved", "unresolved", "not-applicable",
             "not-applicable", "not-applicable"),
            ("note-request", "active-carrier-timer", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
        ), (("targeting-status", "qualified-note-contact", "unresolved", "contact-target", "unresolved",
             "unresolved"),)),),
    ),
    "roach": Review(
        0x0F05, "Roach", 691, "02cac08ed86890113399789e033b2d249240d8d40bc609fda4e08e4e2332a564",
        (1, 0, 0), (10, 255, 255, 255, 255),
        (("bomb", "b", "projectile", "Bomb", "unresolved"),),
        "GG4E08 display names/level rows and distinct reviewed bomb exit requests.",
        combat_role="short-range",
        display_jump="air:1",
        catalog_note="Air-jump display label: https://gotchaforce.fandom.com/wiki/Roach?oldid=6558",
        stock_columns=(("bomb", 1),),
        clips=(),
        static_preview=True,
        controller_air_resource="air-jumps:1",
        controller_source_type=1,
        controller_selector_family="cohort-e",
        controller_recovery="grounded",
        controller_pose="control-only",
        move_evidence=(MoveEvidence("bomb", (
            ("carrier-request", "paid-class-phase", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
            ("timeout-handoff", "carrier-timeout-or-contact-latch", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
            ("proximity-exit", "qualified-roster-proximity", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
    ),
    "death-bomb": Review(
        0x0D07, "Death Bomb", 679, "c2bd72476d6d2e97fc175bc57bb1338d4720bd80eeed5745148eddfcb0844aac",
        (1, 2, 0), (255, 255, 255, 18, 255),
        (
            ("drop", "x", "special", "Fall", "press"),
            ("impact", "none", "special", "Impact (passive review)", "automatic"),
        ),
        "Fall is the GG4E08 display name; the Drop lead and passive impact describe separate paths.",
        combat_role="short-range",
        clips=(),
        static_preview=True,
        controller_air_resource="unresolved",
        controller_source_type=10,
        controller_selector_family="death-bomb-fall",
        controller_movement="other",
        controller_pose="class-owned",
        move_evidence=(
            MoveEvidence("drop", (
                ("fast-descent-request", "conditioned-x", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
            ), stockless=True),
            MoveEvidence("impact", (
                ("impact-request", "late-contact-gate", "unresolved", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved"),
            ), (("queued-self-defeat", "late-contact-gate", "unresolved", "self", "unresolved", "unresolved"),)),
        ),
    ),
    "blue-striker": Review(
        0x0D00, "Blue Striker", 672, "eba3a267ada2a759640e0c47a94687ed721703c18bdb9d78b669014865e2b4f4",
        (2, 2, 0), (1, 255, 24, 6, 255),
        (
            ("vulcan", "b", "projectile", "Vulcan", "unresolved"),
            ("power-beam", "b", "projectile", "Power Beam", "charge"),
            ("missile", "x", "projectile", "Missile", "unresolved"),
        ),
        "Display names and Vulcan/Missile normal stock endpoints from GG4E08; no weapon behavior bound.",
        combat_role="speed",
        stock_columns=(("vulcan", 1), ("missile", 5)),
        clips=(),
        static_preview=True,
        controller_air_resource="boost-meter",
        controller_source_type=3,
        controller_selector_family="cohort-g",
        controller_movement="continuous-flight",
        controller_recovery="not-boosting",
        controller_pose="unresolved",
    ),
    "wing-soldier": Review(
        0x0A00, "Wing Soldier", 647, "45a0b7b081de79ed0a9b66941d2a063726ce42924cb2b1cc34abe58644d9988e",
        (2, 1, 0), (1, 2, 255, 18, 255),
        (
            ("arrow-shot", "b", "projectile", "Arrow Shot", "unresolved"),
            ("wing-sword", "b", "melee", "Wing Sword", "unresolved"),
            ("fly", "x", "special", "Fly", "unresolved"),
        ),
        "GG4E08 display names and normal level rows; Fly is not bound to the audited glide branches.",
        combat_role="speed",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Wing_Soldier?oldid=6424",
        stock_columns=(("arrow-shot", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-0-12", "0:12")),
        controller_air_resource="flight-meter",
        controller_source_type=2,
        controller_selector_family="cohort-f",
        controller_movement="other",
    ),
    "quick-valkrie": Review(
        0x0B00, "Quick Valkrie", 658, "018f728115fe696b6b63e90e8bd5ae6df9a54f41df052cb2e56dd1bbcbcd9d7a",
        (4, 1, 0), (1, 19, 8, 18, 255),
        (
            ("magic-shot", "b", "projectile", "Magic Shot", "unresolved"),
            ("stick-of-time-gold", "b", "melee", "Stick of Time (Gold)", "unresolved"),
            ("quick-spell", "b", "projectile", "Quick Spell", "charge"),
            ("fly", "x", "special", "Fly", "unresolved"),
        ),
        "GG4E08 display names and normal level rows; no Fly or Quick Spell behavior bound.",
        combat_role="support",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Quick_Valkrie?oldid=6425",
        stock_columns=(("magic-shot", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        controller_air_resource="flight-meter",
        controller_source_type=2,
        controller_selector_family="cohort-f",
        controller_movement="other",
    ),
    "beam-satellite": Review(
        0x0E01, "Beam Satellite", 681, "128b780d01f7ecc128bee81927f7b3904bae3d489f11b489d1e1f4514c1303c8",
        (0, 3, 0), (24, 255, 255, 24, 255),
        (
            ("reflector-beam", "b", "projectile", "Reflector Beam", "unresolved"),
            ("satellite-beam", "x", "projectile", "Satellite Beam", "unresolved"),
        ),
        "GG4E08 display names and B stock row; beam, support and movement behavior remain unbound.",
        combat_role="long-range",
        stock_columns=(("reflector-beam", 1),),
        clips=(),
        static_preview=True,
        controller_air_resource="unresolved",
        controller_source_type=9,
        controller_selector_family="cohort-h",
        controller_movement="other",
        controller_pose="unresolved",
    ),
    "gold-hero": Review(
        0x0802, "Gold Hero", 622, "60cdc85a8b4b3f09da6b669cce05a315e0946140bea1023391a96d1f3159f57d",
        (1, 0, 0), (1, 4, 255, 16, 255),
        (
            ("power-shot", "b", "projectile", "Power Shot", "unresolved"),
            ("ogon-ken", "b", "melee", "Ogon Ken", "unresolved"),
            ("gold-set-up", "x", "special", "Gold Set Up", "press"),
        ),
        "GG4E08 base display names and bounded form entry/return stages; no mode admission.",
        combat_role="short-range",
        stock_columns=(("power-shot", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        controller_air_resource="air-jumps:2",
        controller_selector_family="unresolved",
        forms=(("base", "Base"), ("powered", "Powered")),
        actor_records=2,
        transitions=(
            FormTransitionReview("power-up", "base", "powered", "temporary-entry", "move-event",
                                 "lower-lane-complete", "paid-entry-pending", "powered-life"),
            FormTransitionReview("expire-powered", "powered", "base", "return", "expiry",
                                 "return-request", "deferred-expiry-guard", "powered-life"),
        ),
        move_evidence=(MoveEvidence("gold-set-up", (
            ("paid-entry-request", "conditioned-x", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
            ("rebind-prefix", "lower-lane-complete", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
        )),),
    ),
    "titan-robot": Review(
        0x0604, "Titan Robot", 578, "a0bdd52fb5511cdc3b77e3297c73bccdff77551e30c48dbdf36878826376c1a1",
        (1, 0, 0), (1, 22, 22, 12, 255),
        (
            ("titan-vulcan", "b", "projectile", "Titan Vulcan", "unresolved"),
            ("beam-javelin", "b", "unresolved", "Beam Javelin", "unresolved"),
            ("javelin-shot", "b", "projectile", "Javelin Shot", "charge"),
            ("transform", "x", "special", "Transform", "press"),
            ("transform-return", "x", "special", "Transform (Tank form review)", "press"),
        ),
        "GG4E08 base display names and scoped X cycle in both directions; not retail Titan Tank.",
        combat_role="short-range",
        stock_columns=(("titan-vulcan", 1),),
        clips=(),  # Published record 7 contains a shape POBJ the shared loader refuses.
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        forms=(("base", "Robot"), ("tank", "Tank (internal form)")),
        actor_records=2,
        transitions=(
            FormTransitionReview("select-tank", "base", "tank", "selected", "move-event",
                                 "lower-lane-complete", "opposite-form-staged", "none"),
            FormTransitionReview("select-robot", "tank", "base", "selected", "move-event",
                                 "lower-lane-complete", "opposite-form-staged", "none"),
            FormTransitionReview("flagged-return", "tank", "base", "return", "unresolved",
                                 "class-return-request", "source-recovery-flag"),
        ),
        move_evidence=(
            MoveEvidence("transform", (
                ("select-tank", "conditioned-x", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
            ), stockless=True),
            MoveEvidence("transform-return", (
                ("select-robot", "conditioned-x", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
            ), stockless=True, form="tank"),
        ),
    ),
    "copy-man": Review(
        0x0806, "Copy Man", 625, "3ff1df4811d25ff8f93704058ce6c4c06ae3d3a654fcc6e224ffe088eda8d203",
        (1, 0, 0), (255, 255, 255, 255, 11),
        (("copy-attack", "x", "special", "Copy Attack", "charge"),),
        "GG4E08 display name and checked charged-X same-slot reload path.",
        combat_role="short-range",
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        move_evidence=(MoveEvidence("copy-attack", (
            ("head-request", "program-event-1-1", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
            ("capture-donor-identity", "qualified-head-contact", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        ), (("same-slot-reload", "qualified-head-contact", "transform", "self", "unresolved",
             "unresolved"),)),),
    ),
    "flame-dragon": Review(
        0x0500, "Flame Dragon", 553, "0639a38718e69f28bf60a16fcb3fd109b94d14468c1fb1d8e1c7e0e613ed80db",
        (0, 0, 0), (7, 255, 255, 7, 255),
        (("flame-bomb", "b", "projectile", "Flame Bomb", "unresolved"),
         ("flame-breath", "x", "unresolved", "Flame Breath", "unresolved")),
        "GG4E08 display names and level rows; separate reviewed B handoff and X child.",
        combat_role="long-range",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Flame_Dragon?oldid=6670",
        stock_columns=(("flame-bomb", 1), ("flame-breath", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        controller_air_resource="flight-meter",
        controller_source_type=4,
        controller_selector_family="cohort-d",
        controller_recovery="grounded",
        move_evidence=(
            MoveEvidence("flame-bomb", DRAGON_BOMB_PHASES),
            MoveEvidence("flame-breath", DRAGON_BREATH_PHASES),
        ),
    ),
    "ice-dragon": Review(
        0x0516, "Ice Dragon", 573, "710753b7aaec678994f2c9f5e3f4f8fd6630a7f423d3040f66bd51e1febc7e55",
        (0, 0, 0), (15, 255, 255, 15, 255),
        (("ice-bomb", "b", "projectile", "Ice Bomb", "unresolved"),
         ("ice-breath", "x", "unresolved", "Ice Breath", "unresolved")),
        "GG4E08 display names and level rows; separate reviewed B handoff and X child.",
        combat_role="long-range",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Ice_Dragon?oldid=6625",
        stock_columns=(("ice-bomb", 1), ("ice-breath", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        move_evidence=(
            MoveEvidence("ice-bomb", DRAGON_BOMB_PHASES),
            MoveEvidence("ice-breath", DRAGON_BREATH_PHASES),
        ),
    ),
    "demon-samurai": Review(
        0x0701, "Demon Samurai", 607, "1107713632629b33235cab043d1203d6da66b3e0c436fb337ad4e52b3cfb3ad2",
        (1, 0, 0), (1, 3, 255, 3, 255),
        (
            ("soul-ball", "b", "projectile", "Soul Ball", "unresolved"),
            ("demon-nitoryu", "b", "melee", "Demon Nitoryu", "unresolved"),
            ("random-slash", "x", "melee", "Random Slash", "press"),
        ),
        "GG4E08 display names and scoped orbit/return and shape requests; Random Slash does not imply RNG.",
        combat_role="short-range",
        display_jump="air:3",
        catalog_note="Air-jump display label: https://gotchaforce.fandom.com/wiki/Demon_Samurai?oldid=6580",
        stock_columns=(("soul-ball", 1),),
        clips=(("idle", "0:2"), ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="air-jumps:3",
        move_evidence=(
            MoveEvidence("soul-ball", (
                ("orbit-activation", "paid-program-cue", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
                ("outbound-request", "active-orbit-child", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
                ("return-gate", "both-owner-flags-clear", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
            )),
            MoveEvidence("random-slash", (
                ("shape-request", "lane-gated-program-command", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            ), stockless=True),
        ),
    ),
    "shuriken-ninja": Review(
        0x0001, "Shuriken Ninja", 486, "6c80cc9dcd346dc72d321ec3907aaf14d658162eb4933f90050eab0cd5e9c54c",
        (1, 0, 0), (1, 4, 255, 0, 255),
        (
            ("shuriken", "b", "projectile", "Shuriken", "unresolved"),
            ("shinobi-physical-attack", "b", "melee", "Shinobi Physical Attack", "unresolved"),
            ("shuriken-cutter", "x", "unresolved", "Shuriken Cutter", "unresolved"),
        ),
        "GG4E08 display names and bounded Cutter pose, part-register, and separate child observations.",
        combat_role="short-range",
        stock_columns=(("shuriken", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-4-0", "4:0"), ("clip-4-1", "4:1"), ("clip-4-2", "4:2")),
        move_evidence=(MoveEvidence("shuriken-cutter", (
            ("hand-record-selection", "program-part-selection", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
            ("mounted-child-phase", "parent-presentation-phase", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
        )),),
    ),
    "beam-gunner": Review(
        0x0104, "Beam Gunner", 502, "d285e9c18d9266e8629fe5a476418df2740b4e0540024c6617d51023d27e924f",
        (0, 0, 0), (24, 4, 24, 24, 255),
        (
            ("beam-shot", "b", "projectile", "Beam Shot", "unresolved"),
            ("punch-kick", "b", "melee", "Punch Kick", "unresolved"),
            ("shoulder-beam", "b", "projectile", "Shoulder Beam", "charge"),
            ("giga-beam-cannon", "x", "projectile", "Giga Beam Cannon", "unresolved"),
        ),
        "GG4E08 display names and separate scoped gun-child and beam requests.",
        combat_role="long-range",
        stock_columns=(("beam-shot", 1), ("giga-beam-cannon", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-3-0", "3:0"),
               ("clip-4-0", "4:0"), ("clip-4-1", "4:1"), ("clip-4-2", "4:2")),
        move_evidence=(MoveEvidence("giga-beam-cannon", (
            ("weapon-deploy-request", "upper-lane-presentation-phase", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
            ("beam-request", "accepted-late-debit", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
            ("recovery", "sustain-clock-expiry", "unresolved", "unresolved", "unresolved", "not-applicable",
             "not-applicable", "not-applicable"),
        )),),
    ),
    "neo-g-red": Review(
        0x0629, "Neo G-Red", 604, "2df140fa79f98c6b3343c89653b6d91086bec77101f632909930b33582e1bf82",
        (3, 0, 0), (24, 2, 24, 2, 255),
        (
            ("beam-gatling", "b", "projectile", "Beam Gatling", "unresolved"),
            ("plasma-blade", "b", "melee", "Plasma Blade", "unresolved"),
            ("g-buster", "b", "projectile", "G Buster", "charge"),
            ("neo-g-crash", "x", "melee", "Neo G Crash", "unresolved"),
        ),
        "GG4E08 display names and checked Neo G Crash opening/shape/recovery requests; not normal G-Red rules.",
        combat_role="all-round",
        stock_columns=(("beam-gatling", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-4-0", "4:0"), ("clip-4-1", "4:1"), ("clip-4-3", "4:3")),
        move_evidence=(MoveEvidence("neo-g-crash", (
            ("entry-request", "conditioned-x-route", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
            ("shape-request", "authored-opening", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
            ("recovery", "accepted-exit-gate", "unresolved", "unresolved", "unresolved", "not-applicable",
             "not-applicable", "not-applicable"),
        ), stockless=True),),
    ),
    "machine-head": Review(
        0x0511, "Machine Head", 569, "b1dcaf32fe614214ff075db5ea6a15b1a898dcdc7c7f45fdeebf345c75502188",
        (0, 0, 0), (24, 255, 255, 24, 255),
        (("machine-beam", "b", "projectile", "Machine Beam", "unresolved"),
         ("machine-breath", "x", "unresolved", "Machine Breath", "unresolved")),
        "GG4E08 display names and normal level rows; battle-fusion review is a separate facet.",
        combat_role="long-range",
        stock_columns=(("machine-beam", 1), ("machine-breath", 5)),
        static_preview=True,
        clips=(),
        fusion_review=MACHINE_DRAGON_FUSION_REVIEW,
    ),
    "mechanical-dragon": Review(
        0x0512, "Mechanical Dragon", 570, "74d8f29e05cb63e221e808eb1329a3c739d82dd0c81382f511148e3e46c8ab95",
        (0, 0, 0), (24, 255, 255, 24, 255),
        (("machine-destruction", "b", "unresolved", "Machine Destruction", "unresolved"),
         ("mechanical-beam", "x", "projectile", "Mechanical Beam", "unresolved")),
        "GG4E08 display names and normal level rows; no battle transition or weapon binding.",
        combat_role="long-range",
        stock_columns=(("machine-destruction", 1), ("mechanical-beam", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
    ),
    "machine-dragon": Review(
        0x050F, "Machine Dragon", 567, "83e1b3c437bc6f3558073ef2136195c3312140252688c1fcaa39fa17c001e2f8",
        (0, 0, 0), (255, 255, 255, 255, 255), (),
        "",
        combat_role="long-range",
        catalog_note="All authored display slots are disabled on the inspected 050F record. Not a battle result alias.",
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
    ),
    "flying-saucer": Review(
        0x0D03, "Flying Saucer", 675, "ffd2ee7b9a9aec0fe7a31702a5abcb853820cdf3770a234c6ed41d978c62c2a1",
        (2, 2, 0), (24, 255, 255, 24, 255),
        (("maser", "b", "projectile", "Maser", "unresolved"),
         ("devastation-laser", "x", "projectile", "Devastation Laser", "unresolved")),
        "GG4E08 display names/levels and scoped X scratch/segment observations only.",
        combat_role="speed",
        stock_columns=(("maser", 1), ("devastation-laser", 5)),
        static_preview=True,
        clips=(),
        controller_air_resource="unresolved",
        controller_source_type=18,
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        controller_pose="unresolved",
        move_evidence=(MoveEvidence("devastation-laser", (
            ("beam-request", "qualified-x-route", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
            ("segment-request", "qualified-scratch-endpoints", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
    ),
    "death-head": Review(
        0x0505, "Death Head", 558, "1fe917b5439c48924451163cd4d4e62d6a4553a66d5cb2e4181a3e3d1a119f10",
        (0, 0, 0), (24, 255, 255, 24, 255),
        (("death-beam", "b", "projectile", "Death Beam", "unresolved"),
         ("death-breath", "x", "projectile", "Death Breath", "unresolved")),
        "GG4E08 display names/levels and scoped X accumulator/segment observations only.",
        combat_role="long-range",
        stock_columns=(("death-beam", 1), ("death-breath", 5)),
        static_preview=True,
        clips=(),
        controller_air_resource="unresolved",
        controller_source_type=2,
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        controller_pose="unresolved",
        move_evidence=(MoveEvidence("death-breath", (
            ("beam-request", "paid-accumulator-gate", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
            ("segment-request", "model-relative-endpoints", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
    ),
    "galactic-emperor": Review(
        0x0E04, "Galactic Emperor", 684, "700d4f89a1d97cd2be5f06ba93dd342d8eeb05bda4b3bad3fa0f8ec9685ef69c",
        (0, 3, 0), (255, 255, 255, 255, 255), (),
        "",
        combat_role="long-range",
        catalog_note="Authored move display selectors are disabled on this row; class actions remain unresolved.",
        static_preview=True,
        clips=(),
        controller_air_resource="unresolved",
        controller_source_type=8,
        controller_selector_family="emperor-pin",
        controller_movement="other",
        controller_pose="class-owned",
    ),
    "jack": Review(
        0x080E, "Jack", 632, "29ddc26df988ebfff37b94d204b47405eaf58c756d99b78610c8ac5c9111cfec",
        (1, 0, 0), (1, 4, 255, 8, 255),
        (
            ("jelly-shot", "b", "projectile", "Jelly Shot", "unresolved"),
            ("jelly-punch", "b", "melee", "Jelly Punch", "unresolved"),
            ("jelly-scope", "x", "special", "Jelly Scope", "unresolved"),
        ),
        "GG4E08 move names, normal stock rows, and scoped Jelly Scope bind/anchor observations.",
        combat_role="short-range",
        stock_columns=(("jelly-shot", 1), ("jelly-scope", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        move_evidence=(MoveEvidence("jelly-scope", (
            ("scope-request", "qualified-x-route", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
            ("anchor-refresh", "accepted-group-bind", "unresolved", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved"),
        )),),
    ),
    "jet-hero": Review(
        0x0A01, "Jet Hero", 648, "c23a5bed3cad8c2d76a594ce6afb28c77d214e68b7ea27e4f1e5c61b7e1e2c6d",
        (2, 1, 0), (1, 2, 16, 18, 255),
        (
            ("blade-gun-shot", "b", "projectile", "Blade Gun Shot", "unresolved"),
            ("blade-gun-thrust", "b", "melee", "Blade Gun Thrust", "unresolved"),
            ("power-scope", "b", "special", "Power Scope", "charge"),
            ("fly", "x", "special", "Fly", "unresolved"),
        ),
        "GG4E08 move names, normal B stock row and scoped Power Scope carrier observations.",
        combat_role="speed",
        stock_columns=(("blade-gun-shot", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        move_evidence=(MoveEvidence("power-scope", (
            ("scope-request", "qualified-charged-b-cue", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
            ("initial-travel", "accepted-scope-registration", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        ), stockless=True),),
    ),
    "normal-ninja": Review(
        0x0000, "Normal Ninja", 485, "06134b973ea5e4843a1fa169c12e2e74415d2aa90341d3c23d720a6b6e270399",
        (3, 0, 0), (1, 2, 255, 10, 255),
        (
            ("shuriken", "b", "projectile", "Shuriken", "unresolved"),
            ("shinobi-gatana", "b", "melee", "Shinobi Gatana", "unresolved"),
            ("shinobi-bomb", "x", "projectile", "Shinobi Bomb", "unresolved"),
        ),
        "GG4E08 display names/level rows, and static Normal Ninja bomb handoff; no live contact inferred.",
        combat_role="all-round",
        display_jump="air:3",
        catalog_note="Air-jump display label: https://gotchaforce.fandom.com/wiki/Normal_Ninja?oldid=6396",
        stock_columns=(("shuriken", 1), ("shinobi-bomb", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0"),
               ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        move_evidence=(MoveEvidence("shinobi-bomb", (
            ("carrier-request", "accepted-channel-two-debit", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
            ("area-request", "carrier-contact-or-response-gate", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
    ),
    "command-girl": Review(
        0x0306, "Command Girl", 531, "1e8ba30073c24bb292fb72d890d8952d9164629157e498fba134055d8b61e4a2",
        (0, 0, 0), (24, 255, 255, 24, 255),
        (("beam", "b", "projectile", "Beam", "unresolved"),
         ("buster-laser", "x", "projectile", "Buster Laser", "unresolved")),
        "GG4E08 display names/levels and distinct scoped B/X factory requests; no impact behavior inferred.",
        combat_role="long-range",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Command_Girl?oldid=6540",
        stock_columns=(("beam", 1), ("buster-laser", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0"),
               ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(
            MoveEvidence("beam", (
                ("beam-request", "accepted-channel-zero-debit", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
            MoveEvidence("buster-laser", (
                ("laser-request", "qualified-x-state-and-debit", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
                ("existing-state-request", "qualified-active-x-branch", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
            )),
        ),
    ),
    "plasma-dragon": Review(
        0x0509, "Plasma Dragon", 561, "c989d4c057f427a6ce4c26417b2955bf75add779fbc5dd064fedc52a26855b93",
        (0, 0, 0), (14, 255, 255, 14, 255),
        (("plasma-bomb", "b", "projectile", "Plasma Bomb", "unresolved"),
         ("plasma-breath", "x", "unresolved", "Plasma Breath", "unresolved")),
        "GG4E08 display names and normal level rows, and checked type-47/30 impact descendant paths.",
        combat_role="long-range",
        stock_columns=(("plasma-bomb", 1), ("plasma-breath", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        move_evidence=(MoveEvidence("plasma-bomb", DRAGON_STORM_PHASES, DRAGON_STORM_OUTCOMES),),
    ),
    "thunder-dragon": Review(
        0x0515, "Thunder Dragon", 572, "c6e7c63cb641df4925635ff966f59fee3bfaa75b6114fe34e2b98b6d01aec40c",
        (0, 0, 0), (14, 255, 255, 14, 255),
        (("thunder-bomb", "b", "projectile", "Thunder Bomb", "unresolved"),
         ("thunder-breath", "x", "unresolved", "Thunder Breath", "unresolved")),
        "GG4E08 display names and normal level rows, and checked type-47/30 impact descendant paths.",
        combat_role="long-range",
        stock_columns=(("thunder-bomb", 1), ("thunder-breath", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        move_evidence=(MoveEvidence("thunder-bomb", DRAGON_STORM_PHASES, DRAGON_STORM_OUTCOMES),),
    ),
    "g-black": Review(
        0x062A, "G-Black", 605, "755401bae3f02e9e1494d88a31f1f39b15d7f0b8b4deb57ee8e31357fe562797",
        (3, 0, 0), (24, 2, 24, 2, 24),
        (
            ("beam-gun", "b", "projectile", "Beam Gun", "unresolved"),
            ("black-sword", "b", "melee", "Black Sword", "unresolved"),
            ("buster-laser", "b", "projectile", "Buster Laser", "charge"),
            ("black-crash", "x", "melee", "Black Crash", "unresolved"),
            ("black-buster", "x", "projectile", "Black Buster", "charge"),
        ),
        "GG4E08 display names and distinct checked charged B and charged X routes.",
        combat_role="all-round",
        stock_columns=(("beam-gun", 1),),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1"), ("clip-4-3", "4:3")),
        move_evidence=(
            MoveEvidence("buster-laser", (
                ("charged-b-request", "qualified-charged-b-cue", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
            MoveEvidence("black-buster", (
                ("charged-x-request", "qualified-charged-x-cue", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
        ),
    ),
    "ultimate-cannon": Review(
        0x0C04, "Ultimate Cannon", 669, "0a0c86e9ba0134f541ff76b57d2793aad9c54121a2049ea517961843ef835d79",
        (0, 4, 0), (1, 255, 255, 24, 24),
        (
            ("vulcan-and-missile", "b", "projectile", "Vulcan & Missile", "unresolved"),
            ("quaternary-beam", "x", "projectile", "Quaternary Beam", "unresolved"),
            ("ultimate-beam", "x", "projectile", "Ultimate Beam", "charge"),
        ),
        "GG4E08 display names and separately observed ordinary X stock and request sequence.",
        combat_role="long-range",
        stock_columns=(("vulcan-and-missile", 1), ("quaternary-beam", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_source_type=13,
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("quaternary-beam", (
            ("paid-beam-requests", "qualified-ordinary-x-gate", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
            ("release-clock", "ordinary-x-local-count", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
        )),),
    ),
    "cyber-girl-super": Review(
        0x0305, "Cyber Girl Super", 530, "435c26fae117d6c22249b9697424d954c159fef95405f95537cb40c94c251149",
        (0, 0, 0), (24, 2, 255, 24, 255),
        (
            ("quaternary-beam", "b", "projectile", "Quaternary Beam", "unresolved"),
            ("plasma-circle", "b", "melee", "Plasma Circle", "unresolved"),
            ("remote-beam", "x", "unresolved", "Remote Beam", "unresolved"),
        ),
        "GG4E08 display names/normal stock rows and scoped indexed-child/remote object observations.",
        combat_role="long-range",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Cyber_Girl_Super?oldid=6736",
        stock_columns=(("quaternary-beam", 1), ("remote-beam", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("remote-beam", CYBER_GIRL_REMOTE_PHASES),),
    ),
    "cyber-girl-hyper": Review(
        0x030A, "Cyber Girl Hyper", 535, "0f549546201e1ffce1313a035cb5d918b2c433674262ee4229324132b887c682",
        (0, 0, 0), (24, 2, 255, 24, 255),
        (
            ("quaternary-beam", "b", "projectile", "Quaternary Beam", "unresolved"),
            ("plasma-circle", "b", "melee", "Plasma Circle", "unresolved"),
            ("remote-beam", "x", "unresolved", "Remote Beam", "unresolved"),
        ),
        "GG4E08 display names/normal stock rows and scoped indexed-child/remote object observations.",
        combat_role="long-range",
        display_jump="boost",
        catalog_note="Boost jump label: https://gotchaforce.fandom.com/wiki/Cyber_Girl_Hyper?oldid=6193",
        stock_columns=(("quaternary-beam", 1), ("remote-beam", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("remote-beam", CYBER_GIRL_REMOTE_PHASES),),
    ),
    "wire-gunner": Review(
        0x0103, "Wire Gunner", 501, "fec6a01dd2e5427aa77644b7c9ae7d8a7d873d7f42f0e9d051388cbc510f19ee",
        (4, 0, 0), (1, 4, 11, 11, 255),
        (
            ("assault-shot", "b", "projectile", "Assault Shot", "unresolved"),
            ("punch-kick", "b", "melee", "Punch Kick", "unresolved"),
            ("wire-shoot", "b", "projectile", "Wire Shoot", "charge"),
            ("set-wire", "x", "special", "Set Wire", "unresolved"),
        ),
        "GG4E08 display names and normal stock rows. Wire path capture/traversal remains unresolved.",
        combat_role="support",
        stock_columns=(("assault-shot", 1), ("set-wire", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
    ),
    "wire-girl": Review(
        0x0301, "Wire Girl", 526, "0702c685b61f17fe1a0017cd9fe5290f24f701f677bf1d171dfc628304392474",
        (2, 0, 0), (1, 2, 24, 11, 255),
        (
            ("charge-gun", "b", "projectile", "Charge Gun", "unresolved"),
            ("beam-knife", "b", "melee", "Beam Knife", "unresolved"),
            ("charge-beam", "b", "projectile", "Charge Beam", "charge"),
            ("anchor-wire", "x", "special", "Anchor Wire", "unresolved"),
        ),
        "GG4E08 display names/normal stock and scoped paid-anchor/owner-return branches.",
        combat_role="speed",
        stock_columns=(("charge-gun", 1), ("anchor-wire", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"), ("clip-4-0", "4:0")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("anchor-wire", (
            ("anchor-request", "accepted-channel-two-debit", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
            ("owner-stock-return", "qualified-anchor-return", "unresolved", "unresolved", "unresolved",
             "not-applicable", "not-applicable", "not-applicable"),
        )),),
    ),
    "akuma-samurai": Review(
        0x0708, "Akuma Samurai", 614, "10faf059bb931fb9e06b8c88ff5f07bb8e6198ffc631fc1aa3b1abd33f1cc0d2",
        (1, 0, 0), (24, 3, 255, 3, 255),
        (
            ("akuma-beam", "b", "projectile", "Akuma Beam", "unresolved"),
            ("akuma-nitoryu", "b", "melee", "Akuma Nitoryu", "unresolved"),
            ("random-slash", "x", "melee", "Random Slash", "unresolved"),
        ),
        "GG4E08 display names/normal stock and checked Akuma-specific paid B request.",
        combat_role="short-range",
        stock_columns=(("akuma-beam", 1),),
        clips=(("idle", "0:2"), ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("akuma-beam", (
            ("paid-beam-request", "accepted-channel-zero-debit", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
    ),
    "titan-tank": Review(
        0x0619, "Titan Tank", 591, "ce5d5c07541b0ce61c196d98b51f94e2fdc1a58e25e63fcd4131075807125baf",
        (1, 0, 0), (1, 22, 255, 22, 255),
        (("titan-beam", "b", "projectile", "Titan Beam", "unresolved"),
         ("javelin-shot", "x", "projectile", "Javelin Shot", "unresolved")),
        "GG4E08 display names/normal stock and scoped paid Javelin Shot pair request, not a form cycle.",
        combat_role="short-range",
        stock_columns=(("titan-beam", 1), ("javelin-shot", 5)),
        clips=(),  # Published record 7 contains an unsupported shape POBJ.
        actor_records=2,
        controller_air_resource="unresolved",
        controller_source_type=6,
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("javelin-shot", (
            ("paid-javelin-pair", "accepted-channel-two-debit", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
    ),
    "time-bomber": Review(
        0x0106, "Time Bomber", 504, "115a623e6f06c7edb5b9e03004247e3f6abc43af2f44b66742c01f4fff80caab",
        (0, 0, 0), (10, 255, 255, 10, 255),
        (("set-bomb", "b", "projectile", "Set Bomb", "unresolved"),
         ("bomb-shot", "x", "projectile", "Bomb Shot", "unresolved")),
        "GG4E08 display names, stock rows and scoped paid B/X requests.",
        combat_role="long-range",
        stock_columns=(("set-bomb", 1), ("bomb-shot", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(
            MoveEvidence("set-bomb", (
                ("paid-carrier-request", "accepted-channel-zero-debit", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
            MoveEvidence("bomb-shot", (
                ("separate-shot-request", "accepted-channel-two-debit", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
        ),
    ),
    "remote-bomber": Review(
        0x0107, "Remote Bomber", 505, "d17e7b938a9a7f0e430b7ef47d7b86d73fc1a7594e618010674753cc7048f2f0",
        (0, 0, 0), (10, 255, 255, 10, 255),
        (("set-bomb", "b", "projectile", "Set Bomb", "unresolved"),
         ("detonation", "x", "special", "Detonation", "unresolved")),
        "GG4E08 display names, stock rows and scoped paid B/remote owner requests.",
        combat_role="long-range",
        stock_columns=(("set-bomb", 1),),
        clips=(),  # Exact body uses unsupported generated texture-coordinate mode 2.
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(
            MoveEvidence("set-bomb", (
                ("paid-carrier-request", "accepted-stock-parity-request", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
            MoveEvidence("detonation", (
                ("owner-latch", "qualified-command-and-stock-gap", "unresolved", "unresolved", "unresolved",
                 "not-applicable", "not-applicable", "not-applicable"),
                ("charge-consume", "active-charge-consumes-latch", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            ), stockless=True),
        ),
    ),
    "cosmic-dragon": Review(
        0x0504, "Cosmic Dragon", 557, "219354db7ad7f1b270392fce790ccab346677086c87c392c805cccf6fd52121f",
        (0, 0, 0), (24, 255, 255, 24, 23),
        (("cosmic-beam", "b", "projectile", "Cosmic Beam", "unresolved"),
         ("cosmic-breath", "x", "unresolved", "Cosmic Breath", "unresolved"),
         ("black-hole", "x", "special", "Black Hole", "charge")),
        "GG4E08 display names/stock and reviewed charged-X type-7 carrier phases; effects unresolved.",
        combat_role="long-range",
        stock_columns=(("cosmic-beam", 1), ("cosmic-breath", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_source_type=1,
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(DRAGON_BLACK_HOLE_REVIEW,),
    ),
    "space-dragon": Review(
        0x0510, "Space Dragon", 568, "16530e8e7f748e9feda808ea29ef617c349c8a78d29ceccedcb3b2373b614a62",
        (0, 0, 0), (24, 255, 255, 24, 23),
        (("space-beam", "b", "projectile", "Space Beam", "unresolved"),
         ("space-breath", "x", "unresolved", "Space Breath", "unresolved"),
         ("black-hole", "x", "special", "Black Hole", "charge")),
        "GG4E08 display names/stock and reviewed charged-X type-7 carrier phases; effects unresolved.",
        combat_role="long-range",
        stock_columns=(("space-beam", 1), ("space-breath", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_source_type=1,
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(DRAGON_BLACK_HOLE_REVIEW,),
    ),
    "machine-red": Review(
        0x0600, "Machine Red", 575, "1f89a3b6721ab88c4ab8f07630d6eac3aa7ab2daefbb8e56e789fdaf656e734a",
        (3, 0, 0), (24, 2, 255, 24, 255),
        (("beam-rifle", "b", "projectile", "Beam Rifle", "unresolved"),
         ("beam-sword", "b", "melee", "Beam Sword", "unresolved"),
         ("buster-laser", "x", "projectile", "Buster Laser", "unresolved")),
        "GG4E08 display names, normal stock and scoped B request; Seiryu battle fusion is a separate facet.",
        stock_columns=(("beam-rifle", 1), ("buster-laser", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(MoveEvidence("beam-rifle", (
            ("carrier-request", "accepted-channel-zero-debit", "unresolved", "unresolved", "unresolved",
             "unresolved", "unresolved", "unresolved"),
        )),),
        fusion_review=SEIRYU_FUSION_REVIEW,
    ),
    "machine-blue": Review(
        0x0601, "Machine Blue", 576, "aa58f7eb543df11f7246feaa6d9fbb51ef02d42c61b61cd7b3db567a3d6fb0b9",
        (0, 0, 0), (6, 4, 255, 1, 255),
        (("arm-missile", "b", "projectile", "Arm Missile", "unresolved"),
         ("punch-kick", "b", "melee", "Punch Kick", "unresolved"),
         ("shoulder-cannon", "x", "projectile", "Shoulder Cannon", "unresolved")),
        "GG4E08 display names, normal stock and scoped B/X requests; no live weapon binding.",
        stock_columns=(("arm-missile", 1), ("shoulder-cannon", 5)),
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
        move_evidence=(
            MoveEvidence("arm-missile", (
                ("indexed-carrier-request", "accepted-channel-zero-debit", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
            MoveEvidence("shoulder-cannon", (
                ("paired-carrier-requests", "accepted-channel-two-debit", "unresolved", "unresolved", "unresolved",
                 "unresolved", "unresolved", "unresolved"),
            )),
        ),
    ),
    "cyber-machine-seiryu": Review(
        0x0602, "Cyber Machine Seiryu", 577, "6c7c2e01ce1bf759b057c5e512f8571ff523ec455d7cba3d08a00768573aa39f",
        (3, 0, 1), (255, 255, 255, 255, 255), (), "",
        catalog_note="Normal primary body only. The five authored move selectors are disabled; "
                     "the internal companion 0603 is not this catalogue entry.",
        clips=(("idle", "0:0"), ("clip-0-1", "0:1"), ("clip-0-2", "0:2"),
               ("clip-3-0", "3:0"), ("clip-4-0", "4:0"), ("clip-4-1", "4:1")),
        controller_air_resource="unresolved",
        controller_selector_family="unresolved",
        controller_movement="unresolved",
    ),
}


class RangeFile:
    """Bounded, non-linked input view over an opened file."""
    def __init__(self, stream, offset, size):
        self.stream, self.offset, self.size = stream, offset, size

    def read(self, offset, size):
        if not 0 <= size <= self.size or not 0 <= offset <= self.size - size:
            raise ValueError("reviewed source extent")
        self.stream.seek(self.offset + offset)
        result = self.stream.read(size)
        if len(result) != size:
            raise ValueError("short reviewed source read")
        return result


def source_member(archive, review):
    """Join the exact named AFS member and pin its bytes before decompression."""
    with open_regular(archive) as stream:
        extent = stream.seek(0, 2)
        source = RangeFile(stream, 0, extent)
        magic, count = struct.unpack("<4sI", source.read(0, 8))
        if magic != b"AFS\0" or count != 2857:
            raise ValueError("not the reviewed GG4E08 data archive")
        end = 8 + count * 8
        pairs = tuple(struct.iter_unpack("<II", source.read(8, count * 8)))
        first = min((offset for offset, length in pairs if length), default=0)
        if first < end + 8:
            raise ValueError("invalid GG4E08 AFS table extent")
        directory, length = struct.unpack("<II", source.read(first - 8, 8))
        if directory < first or length != count * 48:
            raise ValueError("invalid GG4E08 AFS name table")
        names = source.read(directory, length)
        filename = f"pl{review.model:04x}.pzz".encode("ascii")
        matches = [index for index in range(count)
                   if names[index * 48:index * 48 + 32].split(b"\0", 1)[0] == filename]
        offset, size = pairs[review.member]
        if matches != [review.member] or not 0 < size <= 2 << 20 or offset < first or offset + size > directory:
            raise ValueError(f"missing reviewed {review.name} source member")
        packed = source.read(offset, size)
        if sha256(packed).hexdigest() != review.digest:
            raise ValueError(f"{review.name} member digest mismatch")
    return PzzArchive(RangeFile(BytesIO(packed), 0, len(packed)))


def normal_level_endpoints(dol_path, model):
    with open_regular(dol_path) as source:
        data = source.read(DOL_BYTES + 1)
    if len(data) > DOL_BYTES:
        raise ValueError("GG4E08 DOL byte budget")
    dol = DolImage(data, expected_sha256=DOL_SHA256)
    normal_offset = dol.read(0x804339E8, 1)[0]
    family_pointer, = struct.unpack(">I", dol.read(0x802F2988 + 4 * (model >> 8), 4))
    row = family_pointer + 18 * ((model & 255) * 20 + normal_offset)
    return tuple(struct.unpack(">9h", dol.read(row + level * 18, 18)) for level in (0, 9))


def displayed_moves(review, endpoints):
    columns = dict(review.stock_columns)
    evidence = {record.move: record for record in review.move_evidence}
    unknown_phase = (
        "unresolved-phase", "unresolved", "unresolved", "unresolved", "unresolved",
        "unresolved", "unresolved", "unresolved",
    )

    def ammo(move_id):
        if move_id in columns:
            col = columns[move_id]
            return f"finite:{endpoints[0][col]}|{endpoints[1][col]}"
        if move_id in evidence and evidence[move_id].stockless:
            return "none"
        return "unresolved"

    def phase_outcome_rows(move_id):
        record = evidence.get(move_id, MoveEvidence(move_id, (unknown_phase,)))
        return "".join(f"phase={'|'.join(phase)}\n" for phase in record.phases) + "".join(
            f"outcome={'|'.join(outcome)}\n" for outcome in record.outcomes)

    return (
        f"GFBORG-MOVES 1\n# {review.move_note}\n"
        "# These rows are for inspection; no executable routes are bound.\n"
        + "".join(
            f"move={ident}|{evidence[ident].form if ident in evidence else 'base'}|{button}|{kind}|{name}\n"
            f"contexts=unresolved\nfallback=unresolved\nammo={ammo(ident)}\nrefill=unresolved\n"
            f"activation={activation}\nguard=unresolved\npriority=unresolved\n"
            "replenishment=unresolved\ninterrupt=unresolved\n"
            + phase_outcome_rows(ident)
            for ident, button, kind, name, activation in review.moves
        )
    )


def forms_text(review):
    roles = ("model", "rig", "animations", "animation-map", "movement", "moves", "collision", "effects")
    return (
        "GFBORG-FORMS 1\n# Source boundary review only; all live bindings and transfers are unresolved.\n"
        "default-form=base\n"
        + "".join(f"form={ident}|{label}\n" + "".join(f"{role}=unresolved\n" for role in roles)
                  for ident, label in review.forms)
        + "".join(
            f"transition={row.id}|{row.source}|{row.destination}|{row.kind}\n"
            f"trigger={row.trigger}\nevent={row.event}\nguard={row.guard}\ninterrupt=unresolved\n"
            f"timer={row.timer}\nanimation=unresolved\neffect=unresolved\ntransfer=unresolved\n"
            "priority=unresolved\n"
            for row in review.transitions
        )
    )


def controller_text(review):
    return (
        "GFBORG-CONTROLLERS 1\n# Static/fragment review only; no live actor or full movement map.\n"
        f"controller=normal|{'base' if review.forms else 'unresolved'}\n"
        f"source=GG4E08:{review.model:04X}/0\n"
        f"source-movement-type={review.controller_source_type}\n"
        f"selector-family={review.controller_selector_family}\nmovement={review.controller_movement}\n"
        f"contact=unresolved\nair-resource={review.controller_air_resource}\n"
        f"recovery={review.controller_recovery}\n"
        f"motion-bank={'none' if review.static_preview else 'local'}\npose={review.controller_pose}\n"
        "actions=class-owned\nevidence=source-audit\n"
    )


def preview_text(review):
    header = (
        "GFBORG-PREVIEW 2\nmodel-format=hsd-whole-body-v1\nvariant=normal\n"
        "rig-prefix=visual/body.prefix\n"
    )
    if review.static_preview:
        return header + "idle=none\n"
    return (
        header + f"idle={review.clips[0][1]}\n"
        + "".join(f"clip={clip}|{raw}\n" for clip, raw in review.clips[1:])
    )


def prepare(borg, catalog, dol, archive, output):
    """Write a new reviewed normal preview; never update a placeholder in place."""
    review = REVIEWS[borg]
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("retail Borg review output already exists")
    row = next((item for item in read_catalog(catalog) if item.model == review.model), None)
    if row is None:
        raise ValueError(f"{review.name} is absent from the source catalogue")
    member = source_member(archive, review)
    actor_extent = ACTOR_BYTES * review.actor_records
    actor_block, _ = member.read(0, actor_extent)
    motions, _ = member.read(3, 2 << 20)
    wrapper, _ = member.read(4, 2 << 20)
    expected_motion = not review.static_preview
    if len(actor_block) != actor_extent or bool(motions) != expected_motion or len(wrapper) < 44:
        raise ValueError(f"{review.name} actor, motion or wrapper extent")
    actor = actor_block[:ACTOR_BYTES]  # Catalogue and base preview never borrow the other form.
    # The hash pins the exact bundle. These narrower checks document what the
    # package interprets from its authored specification, not its live actor.
    if actor[0x1A0:0x1A3] != bytes(review.jump_fields) or actor[0x1A8:0x1AD] != bytes(review.move_slots):
        raise ValueError(f"unexpected {review.name} display slots")
    at, = struct.unpack_from(">I", wrapper)
    if not 12 <= at <= min(65536, len(wrapper) - 32) or at % 4:
        raise ValueError(f"{review.name} prefix extent")
    model_bytes, = struct.unpack_from(">I", wrapper, at)
    if not 32 <= model_bytes <= len(wrapper) - at:
        raise ValueError(f"{review.name} body extent")
    endpoints = normal_level_endpoints(dol, row.model)
    hp = tuple(level[0] for level in endpoints)
    ratings = "|".join(map(str, actor[0x1A4:0x1A8]))
    _, base = package_text(row)
    capability = (
        "static body preview only" if review.static_preview else
        "body preview only" if review.clips else "metadata only; unsupported body"
    )
    manifest = (
        f"GFBORG 2\n# Reviewed normal {capability}; no playable actor.\nid={row.id}\n"
        f"name={review.name}\ncatalog=catalog.txt\n"
    )
    changes = {
        "description": f"Reviewed normal {review.name}: {capability}. No playable actor.",
        "variant": f"normal|Normal|{row.normal_cost}|placeholder",
        "combat-role": review.combat_role,
        "display-jump": review.display_jump,
        "hp": "|".join(map(str, hp)),
        "ratings": ratings,
    }
    if review.moves:
        changes["moves"] = "moves.txt"
    if review.controller_air_resource:
        changes["controllers"] = "controllers.txt"
    if review.forms:
        changes["forms"] = "forms.txt"
    if review.fusion_review:
        changes["fusions"] = "fusions.txt"
    header, *base_rows = base.splitlines()
    rows = [header]
    if review.catalog_note:
        rows.append(f"# {review.catalog_note}")
    for line in base_rows:
        key = line.partition("=")[0]
        rows.append(f"{key}={changes[key]}" if key in changes else line)
    files = {
        "catalog.txt": ("\n".join(rows) + "\n").encode("ascii"),
    }
    if review.moves:
        files["moves.txt"] = displayed_moves(review, endpoints).encode("ascii")
    if review.clips or review.static_preview:
        manifest += "model=visual/body.hsd\n"
        if motions:
            manifest += "animations=visual/body.motions\n"
            files["visual/body.motions"] = motions
        manifest += "preview=preview.txt\n"
        files.update({
            "preview.txt": preview_text(review).encode("ascii"),
            "visual/body.prefix": wrapper[:at],
            "visual/body.hsd": wrapper[at:at + model_bytes],
        })
    if review.controller_air_resource:
        files["controllers.txt"] = controller_text(review).encode("ascii")
    if review.forms:
        files["forms.txt"] = forms_text(review).encode("ascii")
    if review.fusion_review:
        files["fusions.txt"] = review.fusion_review.encode("ascii")
    # Publish the manifest after every declared file, including review metadata.
    files["borg.manifest"] = manifest.encode("ascii")
    output.mkdir()
    for name, data in files.items():
        dest = output / name
        dest.parent.mkdir(exist_ok=True)
        with dest.open("xb") as stream:
            stream.write(data)
    return row.id


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("borg", choices=REVIEWS)
    parser.add_argument("catalog", type=Path, help="qualified local GG4E08 catalog.gfbc")
    parser.add_argument("dol", type=Path, help="original GG4E08 main.dol")
    parser.add_argument("archive", type=Path, help="original GG4E08 afs_data.afs")
    parser.add_argument("output", type=Path, help="new local output folder with an existing parent")
    args = parser.parse_args()
    try:
        print(prepare(args.borg, args.catalog, args.dol, args.archive, args.output))
    except (OSError, ValueError) as exc:
        parser.exit(1, f"retail Borg review: {exc}\n")


if __name__ == "__main__":
    main()
