"""Test exact projectile art with an authored, scratch-only fixture, not a retail moveset."""
import argparse
import json
from pathlib import Path
import re
from shutil import copyfile
import struct
import subprocess
from tempfile import TemporaryDirectory

from tools.analyze.retail_basic_shot_test import whole_body_pose
from tools.extract.portable_borg_preview import prepare


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable",type=Path)
    parser.add_argument("rostertop",type=Path)
    parser.add_argument("stage",type=Path)
    parser.add_argument("--opengl",action="store_true")
    args=parser.parse_args()
    exe,roster,stage=(p.resolve(strict=True) for p in (args.executable,args.rostertop,args.stage))
    common=("GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=300\nmovement=grounded-v1|6|13|15\n"
            "body-record=0\nbody-motion=static-grounded\nrig-prefix=visual/body.prefix\npose-profile=visual/body.pose\n"
            "stock=main|4|180\nprojectile=orb|orb|1|32|0|0|0|12|70|8|ignore-floor\n"
            "move=shot|b|10|grounded|28|none\nrelease=shot|12|main|1|orb\n")
    with TemporaryDirectory(prefix="gf-projectile-record-") as scratch:
        dest=Path(scratch)
        prepare("target",stage,dest/"borgs/training-target")
        pkg=dest/"borgs/record-orb"
        (pkg/"visual").mkdir(parents=True)
        (pkg/"borg.manifest").write_text("GFBORG 2\nid=test:record-orb\nname=Record Orb Test\n"
                                         "model=visual/body.hsd\ncatalog=catalog.txt\npreview=preview.txt\ngameplay=gameplay.txt\n")
        (pkg/"catalog.txt").write_text("GFBORG-CATALOG 1\ndescription=Authored visual test.\n"
                                       "tribe=test:training\ndefault-variant=normal\nvariant=normal|Normal|1|placeholder\n")
        (pkg/"preview.txt").write_text("GFBORG-PREVIEW 2\nmodel-format=hsd-whole-body-v1\n"
                                       "variant=normal\nrig-prefix=visual/body.prefix\nidle=none\n")
        def load_body(model):
            source=roster/"borgs"/f"gg4e08-{model}"
            for suffix in ("hsd","prefix"):
                copyfile(source/f"visual/body.{suffix}",pkg/f"visual/body.{suffix}")
            data=json.loads((source/"source-index.json").read_bytes())
            (pkg/"visual/body.pose").write_bytes(whole_body_pose(data["logical_joints"]))
        def run(record,trail=None,impact=None,miss=False,sound=False,growth=False):
            flight=common.replace("|32|0|0|0|12|70", "|32|100|0|0|12|2") if miss else common
            (pkg/"gameplay.txt").write_text(flight+(f"projectile-record=orb|{record}|10\n" if record is not None else "")+
                (f"projectile-record-trail=orb|{trail}|5|1|6\n" if trail is not None else "")+
                (f"impact-record-bank=visual/impact.hsd\nimpact-record=orb|actor|{impact}|10|18\n" if impact is not None else "")+
                ("impact-record-growth=orb|25\n" if growth else "")+
                ("impact-sound=orb|actor|audio/test-impact.pcm\n" if sound else ""))
            cmd=[exe,"--battle-borg","test:record-orb","--against-borg","test:target","--demo-package-battle",
                 "--trace-package-battle","--frames","150","--battle-mvp-assets",stage]
            if not args.opengl:
                cmd.append("--headless")
            return subprocess.run(cmd,cwd=dest,text=True,capture_output=True,timeout=25)
        load_body("0500")
        good=run(8)
        if good.returncode or "shots=2 hits=2" not in good.stderr or not all(
                re.search(rf"projectile-record: round={rnd} tick=[0-9]+ draws=[1-9]",good.stderr) for rnd in (0,1)):
            raise ValueError(good.stderr)
        primitive=run(None)
        def events(result):
            return [line for line in result.stderr.splitlines() if "borg-api:" in line]
        if primitive.returncode or not events(good) or events(good)!=events(primitive) or "projectile-record:" in primitive.stderr:
            raise ValueError("record appearance must not change accepted battle events")
        trailed=run(8,9)
        trail_draws=[(int(rnd),int(n)) for rnd,n in re.findall(
            r"projectile-record-trail: round=(\d+) tick=\d+ draws=(\d+)",trailed.stderr)]
        # Flame Dragon record 9 has two draws per visible sample.
        if trailed.returncode or events(trailed)!=events(good) or {rnd for rnd,_ in trail_draws}!={0,1} or any(
                not 1<=n<=10 for _,n in trail_draws):
            raise ValueError(trailed.stderr)
        missing_bank=run(8,impact=22)
        if missing_bank.returncode!=2 or "required impact record archive missing" not in missing_bank.stderr:
            raise ValueError(missing_bank.stderr)
        archive=roster/"borgs/effects/common-single.hsd"
        impact_path=pkg/"visual/impact.hsd"
        copyfile(archive,impact_path)
        for growth in (False,True):
            effect=run(8,impact=22,growth=growth)
            if effect.returncode or events(effect)!=events(good):
                raise ValueError(effect.stderr)
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) .*event=hit ",effect.stderr)
                ticks=[int(t) for t in re.findall(rf"impact-record: round={rnd} tick=(\d+) draws=[1-9]\d*",effect.stderr)]
                if not hit or set(ticks)!=set(range(int(hit[1]),int(hit[1])+18)):
                    raise ValueError(effect.stderr)
        # Scratch-only negative fixture: record 22's first explicit PE descriptor
        # gains depth writes. The plain record still binds; the fade must refuse.
        depth_write=bytearray(archive.read_bytes())
        pe=32+0xb810
        if depth_write[pe]!=0x19:
            raise ValueError("unexpected fixture PE descriptor")
        depth_write[pe]|=0x20
        impact_path.write_bytes(depth_write)
        opaque=run(8,impact=22)
        rejected=run(8,impact=22,growth=True)
        if opaque.returncode or rejected.returncode!=2 or "impact growth requires material-alpha blending" not in rejected.stderr:
            raise ValueError(opaque.stderr+rejected.stderr)
        copyfile(archive,impact_path)
        missing_effect=run(8,impact=255)
        if missing_effect.returncode!=2 or "hsd record selection:" not in missing_effect.stderr:
            raise ValueError(missing_effect.stderr)
        expired=run(8,impact=22,miss=True,growth=True)
        if (expired.returncode!=2 or "shots=2 hits=0" not in expired.stderr or "impact-record:" in expired.stderr
                or expired.stderr.count("event=retire frame=0 object=1 recipient=255 value=4")!=2):
            raise ValueError(expired.stderr)
        (pkg/"audio").mkdir()
        (pkg/"audio/test-impact.pcm").write_bytes(struct.pack("<hh",1000,-1000)*800)
        sound=run(8,sound=True)
        if (sound.returncode or "borg-impact-audio: starts=2" not in sound.stderr
                or sound.stderr.count("event=impact-sound ")!=2 or "impact-record:" in sound.stderr
                or [v for v in events(sound) if "event=impact-sound" not in v]!=events(good)):
            raise ValueError(sound.stderr)
        expire_sound=run(8,miss=True,sound=True)
        if expire_sound.returncode!=2 or "event=impact-sound " in expire_sound.stderr or "borg-impact-audio: starts=0" not in expire_sound.stderr:
            raise ValueError(expire_sound.stderr)
        missing=run(31)
        if missing.returncode!=2 or "hsd record selection:" not in missing.stderr:
            raise ValueError(missing.stderr)
        load_body("0509")
        empty=run(9)
        if empty.returncode!=2 or "empty selected geometry" not in empty.stderr:
            raise ValueError(empty.stderr)
        empty_trail=run(8,9)
        if empty_trail.returncode!=2 or "empty selected geometry" not in empty_trail.stderr:
            raise ValueError(empty_trail.stderr)
    print("Exact projectile record drawn before and after restart; missing and empty records refused")


if __name__=="__main__":
    main()
