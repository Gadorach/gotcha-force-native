"""Convert prepared local art into normal previews and opt-in Borg API test packages."""
import argparse
from hashlib import sha256
from pathlib import Path

from tools.extract.native_battle_mvp import read


ART = {
    "g-red": (
        "G-Red", "machine", 300,
        (
            "a2082d8cf7f3d1733ca6632b5c744af42b1e13c1eff7ef5be872a49fb8335e8d",
            "273480270a527be24c519ec84ebdb35d3c429f3c68af1cce79c81192a49903e0",
            "c4394ed99a344661aca542329a435e25a99198a890580f83118d6420c8a009e6",
            "9f9ef70bf03921bf2e3bc93d3a9b05391bfc17566cac48d78740698f3bcb6219",
        ),
    ),
    "sasuke": (
        "Sasuke", "ninja", 150,
        (
            "f279a2e99c6a3a81f0b96a1bdd500b93d964355a8c381ab4c3ec7ea0187f2543",
            "968831a19a0dd1cb6bab022f4c4e5a349d4876c877e4818117220dfa992b3646",
            "b3013f4d1f1fc8f4bcfa68dc3fe4bbbee45e1f9ca29edca5d947679906abea8f",
            "965b57fb4994a6274c3c5e0d7f77fc67eabb9326617db84f5a08f6f4de9ae284",
        ),
    ),
}
PRACTICE = (
    ("locomotion", "locomotion.gfmp", "863d8936ec0e6156611ae4bdbb7dce72466307ccba960f548bba522701ef92b5"),
    ("pose-height", "pose-height.gfhc", "8039848a33d755509ab49623e468cdf9bf2922a7e1f5ceca1c25d01fc8209336"),
    ("movement-input", "movement.gfmi", "96039c7257fb2c177c093826dfe48e86ea13fa34087b9a6b70fff3c270d8b6ee"),
    ("movement-motion", "motion.gfmm", "a3b383e875881749f1702ee65f424d10d830a83dbc2e586c3bdd567565cc78fc"),
)

MOVEMENT_PCM = {
    "landing": "4c612718f46e256fda7fbc0360a8ada8553c415d71228e28145de87245411cdb",
    "jump": "d7bd44d72badb02d7ff7dfa250d23f83ea6e0ec189a7c5e9601ad9a7cb01eb0f",
    "thrust": "e073d3007f7e99ce88da5bb70fcc46d42bbedbba23265a63c2fbd0afadeba725",
}
MOVEMENT_CUES = {
    "g-red": (("rise", "thrust"), ("landing", "landing")),
    "sasuke": (("takeoff", "jump"), ("air-jump", "jump"), ("landing", "landing")),
}

TEST_BATTLES = {
    "g-red": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=normal-gred-gfmm9\nhealth=200\n"
        "# Recovered debit order and stock seed, authored shot delivery/aim/timing.\n"
        "stock=beam-stock|5|180\n"
        "projectile=test-beam|beam|1|60|0|0|0|8|120|19|ignore-floor\n"
        "origin=test-beam|7|-106.8|-3.6|30|target-height\n"
        "# Authored exhaust markers; intro attachment assets are not bound yet.\n"
        "movement-effect=jet-left|rise|jet|0|-25|22|-42|54|0\n"
        "movement-effect=jet-right|rise|jet|0|25|22|-42|54|0\n"
        "move=beam-test|b|20|grounded|34|3:11\n"
        "release=beam-test|16|beam-stock|1|test-beam\n"
    ),
    "sasuke": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|8|13|15\nhealth=180\nair-jump-points=3\n"
        "movement-effect=jump-dust|takeoff|puff|0|0|0|0|58|16\n"
        "movement-effect=air-puff|air-jump|puff|0|0|-8|0|66|18\n"
        "movement-effect=landing-dust|landing|puff|0|0|0|0|72|20\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\nidle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "# One paid shot per release and depletion refill; other values are test-authored.\n"
        "stock=shuriken-stock|8|90\n"
        "projectile=shuriken|shuriken|1|42|0|0|0|8|120|9|ignore-floor\n"
        "origin=shuriken|7|-50|0|20|target-height\nlook=shuriken|64|122|218|255\n"
        "projectile-animation=shuriken|0.36|0|0\n"
        "move=shuriken-test|b|20|grounded|24|3:0\npose-start=shuriken-test|7\n"
        "release=shuriken-test|3|shuriken-stock|1|shuriken\n"
        "repeat=shuriken-test|5\n"
        "# Close B priority is source-informed; motion, shape and damage are authored.\n"
        "move=close-b-test|b|30|grounded|18|0:2\nrange=close-b-test|145\n"
        "melee=close-b-test|4|8|115|12|60|24|14\n"
    ),
    "arc-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=arc-stock|2|150\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=arc|bullet|2|52|5|0.5|0.06|8|100|12|stop-on-floor\n"
        "on-hit=arc|walk-speed|60|30\n"
        "move=primary|b|10|any|14|0:1\nmove=arc-twin|x|20|grounded|24|0:2\n"
        "release=primary|3|shot-stock|1|straight\nrelease=arc-twin|6|arc-stock|1|arc\n"
    ),
    "grapple-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=grapple-stock|2|150\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=grapple|beam|1|50|0|0|0|8|100|8|ignore-floor\n"
        "on-hit=grapple|pull-self|36\n"
        "move=primary|b|10|any|14|0:1\nmove=grapple-throw|x|20|grounded|24|0:2\n"
        "release=primary|3|shot-stock|1|straight\nrelease=grapple-throw|6|grapple-stock|1|grapple\n"
    ),
    "charge-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=primary-stock|4|90\nstock=pulse-stock|3|150\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=pulse|bullet|1|80|0|0|0|8|80|8|ignore-floor\n"
        "projectile=surge|beam|3|125|0|0|0.025|8|80|11|ignore-floor\n"
        "move=primary|b|10|any|14|0:1\nmove=pulse-tap|x|20|grounded|20|0:2\nmove=pulse-charge|x|30|grounded|24|0:2\n"
        "activation=pulse-tap|release-window|0|7\nactivation=pulse-charge|release-window|8|600\n"
        "release=primary|3|primary-stock|1|straight\nrelease=pulse-tap|3|pulse-stock|1|pulse\n"
        "release=pulse-charge|3|none|0|surge\n"
    ),
    "blade-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=primary-stock|4|90\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "move=primary|b|10|any|14|0:1\nrelease=primary|3|primary-stock|1|straight\n"
        "move=close-cut|b|20|grounded|20|0:2\nrange=close-cut|145\n"
        "melee=close-cut|4|8|115|12|60|30|20\n"
    ),
    "rotary-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|6|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=rotary-stock|40|110\n"
        "projectile=rotary-shot|bullet|1|75|0|0|0|6|100|5|ignore-floor\n"
        "move=rotary-b|b|10|grounded|12|0:1\n"
        "held-fire=rotary-b|rotary-stock|1|rotary-shot|18|9|3|18\n"
    ),
    "bomb-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=bomb-stock|2|150\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=fuse-bomb|bullet|1|37|4|0.5|0|8|19|0|ignore-floor\n"
        "actor-contact=fuse-bomb|ignore\nblast=fuse-bomb|expiry|110|32|opponent\n"
        "move=primary|b|10|any|14|0:1\nmove=bomb-throw|x|20|grounded|24|0:2\n"
        "release=primary|3|shot-stock|1|straight\nrelease=bomb-throw|3|bomb-stock|1|fuse-bomb\n"
    ),
    "remote-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=charge-stock|3|on-return\n"
        "projectile=charge|bullet|1|52|0|0.7|0|8|90|0|land-on-floor\n"
        "actor-contact=charge|ignore\nblast=charge|remote|150|26|opponent\nreturn-stock=charge|remote\n"
        "move=deploy|b|10|grounded|20|0:1\nrelease=deploy|3|charge-stock|1|charge\n"
        "move=trigger|x|20|any|14|0:2\ndetonate=trigger|3|charge|landed\n"
    ),
    "seeker-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=seeker-stock|2|150\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=seeker|beam|1|55|0|0|0|8|100|16|ignore-floor\n"
        "guidance=seeker|tracking|0.12|24\n"
        "move=primary|b|10|any|14|0:1\nmove=seeker-shot|x|20|grounded|24|0:2\n"
        "release=primary|3|shot-stock|1|straight\nrelease=seeker-shot|3|seeker-stock|1|seeker\n"
    ),
    "guard-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\n"
        "projectile=straight|bullet|1|58|0|0|0|8|100|15|ignore-floor\n"
        "move=primary|b|10|any|14|0:1\nrelease=primary|3|shot-stock|1|straight\n"
        "move=brace|x|20|grounded|18|0:2\nguard=brace|2|13|1.0|1|direct\n"
    ),
    "impact-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=beam-stock|4|90\nstock=arc-stock|2|150\n"
        "projectile=target-beam|beam|1|105|0|0|0|8|100|0|stop-on-stage\n"
        "projectile=impact-arc|bullet|1|56|4|2|0|8|100|0|stop-on-stage\n"
        "blast=target-beam|actor|94|22|opponent\nblast=impact-arc|impact|170|28|opponent\n"
        "move=beam|b|10|any|14|0:1\nmove=bomb|x|20|grounded|24|0:2\n"
        "release=beam|3|beam-stock|1|target-beam\nrelease=bomb|3|arc-stock|1|impact-arc\n"
    ),
    "wingmate-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=beam-stock|4|90\n"
        "projectile=beam|beam|1|63|0|0|0|8|100|15|ignore-floor\n"
        "projectile=wing-beam|beam|1|63|0|0|0|8|100|7|ignore-floor\n"
        "move=primary|b|10|any|14|0:1\nrelease=primary|3|beam-stock|1|beam\n"
        "move=deploy|x|20|grounded|14|0:2\n"
        "emitter=left|primary|wing-beam|-35|82|28|35\nemitter=right|primary|wing-beam|35|82|28|35\n"
        "deploy-emitter=deploy|2|left\ndeploy-emitter=deploy|2|right\n"
    ),
    "rewind-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=anchor-stock|4|150\nanchor-set=wire|4\n"
        "projectile=straight|beam|1|58|0|0|0|8|100|15|ignore-floor\n"
        "move=primary|b|10|any|12|0:1\nrelease=primary|3|shot-stock|1|straight\n"
        "move=place|x|20|any|6|0:2\nactivation=place|tap|5|6\n"
        "place-anchor=place|2|wire|anchor-stock|1\n"
        "move=rewind|x|30|grounded|12|0:2\nactivation=rewind|double-tap|5|6\n"
        "rewind=rewind|2|wire|16\n"
    ),
    "wire-trail-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=anchor-stock|4|150\nanchor-set=wire|4\n"
        "projectile=straight|beam|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=wire-shot|beam|1|42|0|0|0|8|100|9|ignore-floor\n"
        "projectile=wire-plain|beam|1|42|0|0|0|8|100|9|ignore-floor\n"
        "on-hit=wire-shot|rewind-recipient|wire|16\n"
        "empty-anchors=wire-shot|wire-plain\n"
        "move=primary|b|10|any|12|0:1\nactivation=primary|release-window|0|7\nrelease=primary|3|shot-stock|1|straight\n"
        "move=place|x|20|any|6|0:2\nplace-anchor=place|2|wire|anchor-stock|1\n"
        "move=wire-release|b|30|any|20|0:2\nactivation=wire-release|release-window|8|600\nrelease=wire-release|3|none|0|wire-shot\n"
    ),
    "anchor-arrow-scout": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|13|14\nhealth=180\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\n"
        "stock=shot-stock|4|90\nstock=plant-stock|3|150\nanchor-set=arrow|1\n"
        "projectile=straight|beam|1|58|0|0|0|8|100|15|ignore-floor\n"
        "projectile=plant-arrow|bullet|1|48|4|2|0|8|100|0|stop-on-stage\n"
        "actor-contact=plant-arrow|ignore\nplant-anchor=plant-arrow|arrow\n"
        "projectile=link-arrow|beam|1|42|0|0|0|8|100|9|ignore-floor\n"
        "on-hit=link-arrow|rewind-recipient|arrow|16\n"
        "move=primary|b|10|any|12|0:1\nactivation=primary|release-window|0|7\nrelease=primary|3|shot-stock|1|straight\n"
        "move=plant|x|20|any|12|0:2\nrelease=plant|3|plant-stock|1|plant-arrow\n"
        "move=wire-release|b|30|any|20|0:2\nactivation=wire-release|release-window|8|600\nrelease=wire-release|3|none|0|link-arrow\n"
    ),
    "target": (
        "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=stationary-v1\nhealth=500\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\nidle=0:0\n"
        "damage-model=10\n"
    ),
}
CUSTOM_NAMES = {"arc-scout": "Arc Scout", "grapple-scout": "Grapple Scout",
                "charge-scout": "Charge Scout", "blade-scout": "Blade Scout", "rotary-scout": "Rotary Scout",
                "bomb-scout": "Bomb Scout", "remote-scout": "Remote Scout", "seeker-scout": "Seeker Scout",
                "guard-scout": "Guard Scout", "impact-scout": "Impact Scout", "wingmate-scout": "Wingmate Scout",
                "rewind-scout": "Rewind Scout", "wire-trail-scout": "Wire Trail Scout",
                "anchor-arrow-scout": "Anchor Arrow Scout", "target": "Training Target"}


def checked(path, pin):
    data = read(path, 1 << 20)
    if sha256(data).hexdigest() != pin:
        raise ValueError(f"not the reviewed normal input: {path.name}")
    return data


def prepare(model, art, output, practice=None, battle_test=False, movement_sfx=None, *, gameplay_transform=None):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("portable Borg output already exists")
    if practice is not None and model != "g-red":
        raise ValueError("only normal G-Red has a qualified practice controller")
    custom = model in CUSTOM_NAMES
    if battle_test and not (custom or model=="sasuke" or (model == "g-red" and practice is not None)):
        raise ValueError("this retail model has no test combat profile or lacks its qualified movement pack")
    if movement_sfx is not None and (not battle_test or model not in MOVEMENT_CUES):
        raise ValueError("reviewed original movement sounds only bind normal G-Red or Sasuke Battle Test")
    if gameplay_transform is not None and not battle_test:
        raise ValueError("a gameplay transform requires explicit test rules")
    art_model = "sasuke" if custom else model
    name, tribe, cost, pins = ART[art_model]
    if custom:
        name, tribe, identity = CUSTOM_NAMES[model], "training", f"test:{model}"
    else:
        identity = f"gg4e08:{'0615' if model == 'g-red' else '000a'}"
    files = {
        f"visual/body.{suffix}": checked(Path(art)/f"{art_model}.{suffix}", pin)
        for suffix, pin in zip(("hsd", "prefix", "pose", "motions"), pins, strict=True)
    }
    manifest = (
        f"GFBORG 2\nid={identity}\nname={name}\nmodel=visual/body.hsd\n"
        "animations=visual/body.motions\ncatalog=catalog.txt\npreview=preview.txt\n"
    )
    files["catalog.txt"] = (
        f"GFBORG-CATALOG 1\ndescription={'Authored API fixture using local artwork' if custom else 'Local normal ' + name}.\n"
        f"tribe={'test' if custom else 'gg4e08'}:{tribe}\ndefault-variant=normal\nvariant=normal|Normal|{cost}|placeholder\n"
    ).encode()
    final_clips = "clip=takeoff|0:6\n" if model == "g-red" else "clip=walk-transition|0:6\nclip=ascent|0:7\n"
    files["preview.txt"] = (
        "GFBORG-PREVIEW 2\nmodel-format=hsd-body-v1\nvariant=normal\n"
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        "idle=0:0\nclip=walk|0:1\nclip=dash-0|0:2\nclip=dash-1|0:4\n"
        "clip=dash-2|0:3\nclip=dash-3|0:5\n" + final_clips
    ).encode()
    if practice is not None:
        manifest += "rules=practice.txt\n"
        rules = (
            "GFBORG-PRACTICE 1\ncontroller=normal-gred-gfmm9\nvariant=normal\n"
            "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
        )
        for role, asset, pin in PRACTICE:
            files[f"movement/{asset}"] = checked(Path(practice)/asset, pin)
            rules += f"{role}=movement/{asset}\n"
        files["practice.txt"] = rules.encode()
    if battle_test or custom:
        manifest += "gameplay=gameplay.txt\n"
        gameplay = TEST_BATTLES[model]
        if movement_sfx is not None:
            for cue, clip in MOVEMENT_CUES[model]:
                path = f"audio/{clip}.pcm"
                if path not in files:
                    files[path] = checked(Path(movement_sfx)/f"{clip}.pcm", MOVEMENT_PCM[clip])
                gameplay += f"movement-sound={cue}|{path}\n"
        encoded=gameplay.encode()
        files["gameplay.txt"] = gameplay_transform(encoded) if gameplay_transform is not None else encoded
    # All inputs passed the content checks. Publish the manifest last.
    output.mkdir(parents=True, exist_ok=False)
    for name, data in (*files.items(), ("borg.manifest", manifest.encode())):
        dest = output/name
        dest.parent.mkdir(exist_ok=True)
        with dest.open("xb") as stream:
            stream.write(data)
    return identity


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", choices=(*ART, *CUSTOM_NAMES))
    parser.add_argument("output", type=Path)
    parser.add_argument("--art", type=Path, default=Path("local/content/native-battle-mvp-v1"))
    parser.add_argument("--practice", type=Path, help="qualified local G-Red movement pack")
    parser.add_argument("--battle-test", action="store_true", help="add explicit test rules to normal G-Red or Sasuke")
    parser.add_argument("--movement-sfx", type=Path, help="bind the qualified original movement PCM pack")
    args = parser.parse_args()
    print(prepare(args.model, args.art, args.output, args.practice, args.battle_test, args.movement_sfx))
