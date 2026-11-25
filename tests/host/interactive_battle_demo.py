"""Exercise the real paginated battle dropdown and shared scripted combat."""
import argparse
from pathlib import Path
import re
from shutil import copyfile, copytree
import shlex
import subprocess
from tempfile import TemporaryDirectory


def check(root, *, opengl=False, equipped=False, trailing=False, knight_expansion=False, girl_worm=False, reflection_bodies=False,
          close_expansion=False, blue_ninja=False, robot_diver=False, wing_valkyrie=False, robot_bomber=False,
          ground_close=False, wire_shuriken=False, late_close=False, tank_shot=False, paired_ground=False,
          single_ground_shot=False, dragon_breath=False, roach_bomb=False, zeta_field=False, charge_field=False,
          icbm=False, spawner_shot=False, flying_boss=False):
    def launch(script,*args):
        cmd=[str(root/script),*args]
        if not opengl:
            cmd.append("--headless")
        got=subprocess.run(cmd,cwd=root,capture_output=True,text=True,timeout=35)
        if got.returncode:
            raise ValueError(got.stderr)
        return got.stderr

    def pick(id,*options):
        output=launch("play.sh","--demo-battle-pick",id,"--trace-package-battle","--frames","72","--no-audio",*options)
        if f"battle-picker: active id={id} variant=normal (new round)" not in output or "event=start " in output:
            raise ValueError(output)
        return re.findall(r"battle-picker: row=\d+ id=(\S+)",output)

    rows=pick("gg4e08:050b")
    if rows[-2:]!=["example:flight-stick","example:jump-stick"] or "test:target" in rows or len(rows)!=len(set(rows)):
        raise ValueError(f"incorrect combat dropdown: {rows}")
    for id in ("gg4e08:0200","gg4e08:0204","gg4e08:0208","gg4e08:020f","example:flight-stick","example:jump-stick"):
        if pick(id)!=rows:
            raise ValueError("dropdown ordering changed")
    if pick("example:jump-stick","--size","320","160")!=rows:
        raise ValueError("short viewport omitted rows")
    gamma_events=[]
    melee=("0200","020a","020f","0204","020d","0208","020e")
    for id,count in (("gg4e08:0500",2),("gg4e08:050b",2),*((f"gg4e08:{m}",0) for m in melee)):
        trace=launch("show-attacks.sh",id,"--no-audio")
        if f"shots={count} hits=2" not in trace:
            raise ValueError(trace)
        if count==0 and (trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
            raise ValueError(trace)
        if id=="gg4e08:020f":
            gamma_events=[line for line in trace.splitlines() if "borg-api:" in line]
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                if not hit or not re.search(rf"borg-parts: round={rnd} tick={hit[1]} actor=0 draws=[1-9]",trace):
                    raise ValueError("Gamma II sword did not draw at contact\n"+trace)
        if id=="gg4e08:0500" and "impact-record:" not in trace:
            raise ValueError(trace)
        if id in ("gg4e08:0204","gg4e08:020d"):
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                if not hit:
                    raise ValueError(trace)
                draws={int(t):int(n) for t,n in re.findall(rf"borg-parts: round={rnd} tick=(\d+) actor=0 draws=(\d+)",trace)}
                if equipped:
                    if not all(draws.get(t,0)>=8 for t in (1,int(hit[1])-1,int(hit[1]),int(hit[1])+7)):
                        raise ValueError("Axe equipment must also draw outside active frames\n"+trace)
                elif not all(draws.get(t,0)>draws.get(int(hit[1])-1,0)>0 for t in range(int(hit[1]),int(hit[1])+6)):
                    raise ValueError("Axe visual does not follow the accepted six-frame window\n"+trace)
    if equipped:
        trace=launch("show-attacks.sh","gg4e08:0007","--no-audio")
        for rnd in (0,1):
            draws={int(t):int(n) for t,n in re.findall(rf"borg-parts: round={rnd} tick=(\d+) actor=0 draws=(\d+)",trace)}
            if not all(draws.get(t,0)>=4 for t in (1,2,7,24)):
                raise ValueError("Cyber Ninja arm equipment not submitted\n"+trace)
        for id in ("0208","020e"):
            if "part-hierarchy=" in (root/f"borgs/gg4e08-{id}/gameplay.txt").read_text():
                raise ValueError(f"{id} candidates must not be promoted before visible qualification")
        for id in ("gg4e08:0602","gg4e08:0500"):
            for w,h in (("900","460"),("460","900")):
                pick(id,"--size",w,h)
    for id,move in (("example:flight-stick","paint-stream"),("example:jump-stick","lined-shot")):
        trace=launch("show-attacks.sh",id,"--no-audio")
        if f"move={move} event=socket-launch " not in trace:
            raise ValueError(trace)
    if trailing:
        for id in ("0500","0509","050a","050c","0515","0516"):
            trace=launch("show-attacks.sh",f"gg4e08:{id}","--no-audio","--exercise-battle-mobility")
            if "shots=2 hits=2" not in trace or trace.count("event=socket-launch ")!=2:
                raise ValueError(f"{id} lost its authored socket launch or contact\n"+trace)
            frames=re.findall(r"borg-chain: round=(\d+) tick=(\d+) actor=0 grounded=(\d) landing=(\d+) contacts=(\d+) tip=(\S+)",trace)
            if not any(row[0]=="0" and row[2]=="0" for row in frames) or not any(
                    row[0]=="0" and row[2]=="1" and int(row[3])>0 for row in frames):
                raise ValueError(f"{id} must jump and land during the first round\n"+trace)
            for rnd in ("0","1"):
                samples=[row for row in frames if row[0]==rnd]
                if len({row[5] for row in samples})<8:
                    raise ValueError(f"{id} must publish a moving tail in round {rnd}\n"+trace)
                profile=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
                fitted="animated-translation-percent=50\n" in profile
                if (fitted or id in ("0500","0509","050a")) and not any(row[2]=="1" and int(row[4])>0 for row in samples):
                    raise ValueError(f"{id} tail must bend against the demo floor in round {rnd}\n"+trace)
            spawn=[row[5] for row in frames if row[1]=="1"]
            if len(spawn)!=2 or spawn[0]!=spawn[1]:
                raise ValueError(f"{id} retained trailing-chain state across restart\n"+trace)
    if knight_expansion:
        for id,clip,start in (("0202",4,0),("0203",1,8),("020b",4,0),("020c",1,8)):
            stable=f"gg4e08:{id}"
            if stable not in rows:
                raise ValueError(f"{stable} missing from the dropdown")
            profile=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (f"move=opening-slash|b|10|grounded|24|1:{clip}\n" not in profile or
                    ("pose-start=opening-slash|8\n" in profile)!=(start==8) or
                    profile.count("body-mount=")!=(15 if clip==4 else 0)):
                raise ValueError(f"wrong exact opening or assembly for {stable}")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            if "shots=0 hits=2" not in trace or trace.count("event=melee-armed ")!=2 or "event=debit " in trace:
                raise ValueError(trace)
    if girl_worm:
        if "gg4e08:0308" in rows:
            rules=(root/"borgs/gg4e08-0308/gameplay.txt").read_text()
            if ("move=charge-hit|b|10|grounded|28|none" not in rules or
                    "move=opening-slash" in rules):
                raise ValueError("Shadow Girl must not substitute a missing normal clip")
        for id in ("0300","0304","0305","0307","0309","030a","030b","030d"):
            stable=f"gg4e08:{id}"
            profile=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if stable not in rows or "move=opening-slash|b|10|grounded|24|1:0\n" not in profile:
                raise ValueError(f"{stable} missing its exact local opening")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            if "shots=0 hits=2" not in trace or trace.count("event=melee-armed ")!=2 or "event=debit " in trace:
                raise ValueError(trace)
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                if not hit or not re.search(rf"borg-body: round={rnd} tick={hit[1]} actor=0 draws=[1-9]\d*",trace):
                    raise ValueError(f"{stable} did not submit a body at contact")
        for id,pobj in (("0501",None),("050b","15c98"),("050d","15cd8"),("0517","15cd8")):
            profile=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (pobj and f"zero-reflection={pobj}|7\n" not in profile) or (not pobj and "zero-reflection=" in profile):
                raise ValueError(f"{id} wrong authored reflection scope")
            trace=launch("show-attacks.sh",f"gg4e08:{id}","--no-audio")
            bodies=re.findall(r"borg-body: round=(\d+) tick=(\d+) actor=0 draws=(\d+)",trace)
            if ("shots=2 hits=2" not in trace or len(bodies)!=150 or any(int(count)==0 for _,_,count in bodies) or
                    {rnd for rnd,_,_ in bodies}!={"0","1"}):
                raise ValueError(f"{id} did not draw its own live Worm body throughout")
    if reflection_bodies:
        for id,expected,shots in (("0208",["11428|20"],0),("020e",["11488|20","129e8|44"],0),("0801",["ea60|14"],2)):
            profile=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if re.findall(r"(?m)^zero-reflection=(.*)$",profile)!=expected:
                raise ValueError(f"{id} does not declare every exact reflection mesh")
            trace=launch("show-attacks.sh",f"gg4e08:{id}","--no-audio")
            frame_counts=re.findall(r"borg-body: round=(\d+) tick=(\d+) actor=0 draws=(\d+)",trace)
            if (f"shots={shots} hits=2" not in trace or len(frame_counts)!=150 or
                    any(int(rows[2])==0 for rows in frame_counts) or {row[0] for row in frame_counts}!={"0","1"}):
                raise ValueError(f"{id} missing body or attack contact\n"+trace)
    if close_expansion:
        if not wire_shuriken and "gg4e08:0301" in rows:
            raise ValueError("Wire Girl's strict action-clip refusal must stay closed")
        for id,clip,start in (("0201",4,0),("0206",1,8),("0207",1,8),("030c",0,0)):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (stable not in rows or f"move=opening-slash|b|10|grounded|24|1:{clip}\n" not in rules or
                    ("pose-start=opening-slash|8\n" in rules)!=(start==8)):
                raise ValueError(f"{stable} does not have its scoped neutral opening")
            if id=="0201" and not all(r in rules for r in ("part-record=right-hand|7|2\n","part-hierarchy=2\n")):
                raise ValueError("Chainsaw Knight needs its own record-2 tool")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or "event=debit " in trace:
                raise ValueError(f"{stable} did not draw or land its authored contact\n"+trace)
            if id=="0201":
                for rnd in (0,1):
                    hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                    if not hit or not re.search(rf"borg-parts: round={rnd} tick={hit[1]} actor=0 draws=[1-9]",trace):
                        raise ValueError("Chainsaw presentation did not draw at contact")
    if blue_ninja:
        for id in ("0006","0601","0609","0617","061d"):
            stable=f"gg4e08:{id}"
            package=root/f"borgs/gg4e08-{id}"
            rules=(package/"gameplay.txt").read_text()
            if (stable not in rows or "move=opening-slash|b|10|grounded|24|1:1\n" not in rules or "pose-start=" in rules or
                    rules.count("part-record=")!=(0 if id=="0006" else 2)):
                raise ValueError(f"{stable} lost its neutral opening or exact hand presentation")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
                raise ValueError(f"{stable} did not draw or land its stockless opening\n"+trace)
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                if not hit or bool(re.search(rf"borg-parts: round={rnd} tick={hit[1]} actor=0 draws=[1-9]",trace))!=(id!="0006"):
                    raise ValueError(f"{stable} did not draw the expected exact hands at contact")
    if robot_diver:
        ids=("0400","0402","0405","0407","0408","0409","040a","040b","040c","040d","0805","080d","080e")
        for id in ids:
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (stable not in rows or "move=opening-slash|b|10|grounded|24|1:1\n" not in rules or
                    ("walk-loop=authored-bit\n" in rules)!=id.startswith("04") or "idle-loop=" in rules):
                raise ValueError(f"{stable} does not have the exact neutral move or walking policy")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
                raise ValueError(f"{stable} did not draw or land its stockless opening\n"+trace)
            if id in ("0402","0407") and ("part-record=left-hand|11|4\npart-hierarchy=4\n" not in rules or
                                         trace.count("borg-parts:")!=150):
                raise ValueError(f"{stable} missing its own tool during the live round")
    if wing_valkyrie:
        ids=[f"{m:04x}" for m in (*range(0x0a00,0x0a0b),*range(0x0b00,0x0b08))]
        for id in ids:
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (stable not in rows or "move=opening-slash|b|10|grounded|24|1:1\n" not in rules or
                    ("pose-start=opening-slash|10\n" in rules)!=(id in ("0a03","0a06"))):
                raise ValueError(f"{stable} does not have its exact starting clip/frame")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(bodies)!=150 or "0" in bodies or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
                raise ValueError(f"{stable} did not render or accept both stockless contacts\n"+trace)
            if id in ("0a02","0b03") and ("part-hierarchy=1\npart-hierarchy=5\n" not in rules or
                                         trace.count("borg-parts:")!=150):
                raise ValueError(f"{stable} missing its own static hand hierarchies")
    if robot_bomber:
        for id,choice in (("0106",0),("0107",0),("0109",1),("0401",1),("0403",1),("0404",1),("0406",1),("0807",1)):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (stable not in rows or f"move=opening-slash|b|10|grounded|24|1:{choice}\n" not in rules or
                    ("walk-loop=authored-bit\n" in rules)!=id.startswith("04") or
                    ("idle-loop=authored-bit\n" in rules)!=(id=="0401")):
                raise ValueError(f"{stable} has the wrong normal clip or loop declaration")
            if (id=="0107")!=("body-material=immutable-highlight\n" in rules):
                raise ValueError(f"{stable} has the wrong body material admission")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
                raise ValueError(f"{stable} did not draw or accept both stockless contacts\n"+trace)
    if roach_bomb:
        stable="gg4e08:0f05"
        rules=(root/f"borgs/gg4e08-0f05/gameplay.txt").read_text()
        for line in ("body-motion=static-grounded","stock=mine-stock|3|on-return",
                     "projectile=mine|bullet|1|1|0|1|0|16|70|0|land-on-floor",
                     "blast=mine|expiry|310|12|opponent","bomb-proximity=mine|150|30|blast",
                     "return-stock=mine|expiry-or-proximity","move=mine|b|10|grounded|28|none",
                     "release=mine|12|mine-stock|1|mine"):
            if line not in rules:
                raise ValueError(f"{stable} lost its authored bomb policy: {line}")
        trace=launch("show-attacks.sh",stable,"--frames","150","--no-audio")
        if ("event=debit " not in trace or "event=projectile-landed " not in trace or
                "event=bomb-proximity " not in trace or
                not re.search(r"event=blast-hit .* value=12\b",trace) or
                not re.search(r"event=stock-returned .* value=3\b",trace) or
                "event=retire " not in trace):
            raise ValueError(f"{stable} did not place, arm, blast or return its bomb stock\n"+trace)
        bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
        if not bodies or "0" in bodies:
            raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
    if spawner_shot:
        stable="gg4e08:010a"
        rules=(root/"borgs/gg4e08-010a/gameplay.txt").read_text()
        for line in ("stock=spawn-stock|1|180",
                     "projectile=spawn-a|bullet|1|25|0|0|0|16|60|8|ignore-floor",
                     "projectile=spawn-b|bullet|1|25|0|0|0|16|60|8|ignore-floor",
                     "origin=spawn-a|12|0|0|60|target-height","origin=spawn-b|12|0|0|60|target-height",
                     "move=spawn-shot|b|10|grounded|28|none",
                     "release=spawn-shot|12|spawn-stock|1|spawn-a",
                     "release-followup=spawn-shot|12|spawn-b"):
            if line not in rules:
                raise ValueError(f"{stable} lost its authored spawner policy: {line}")
        trace=launch("show-attacks.sh",stable,"--frames","150","--no-audio")
        if ("event=debit " not in trace or "event=emit " not in trace or
                "event=extra-emit " not in trace or "event=hit " not in trace):
            raise ValueError(f"{stable} did not pay for and fire both ordered hitboxes\n"+trace)
        bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
        if not bodies or "0" in bodies:
            raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
        stable="gg4e08:0f04"
        rules=(root/"borgs/gg4e08-0f04/gameplay.txt").read_text()
        for line in ("body-motion=static-grounded",
                     "projectile=spawn|bullet|1|25|0|0|0|16|60|8|ignore-floor",
                     "origin=spawn|2|0|0|60|target-height",
                     "move=spawn-shot|b|10|grounded|28|none",
                     "free-release=spawn-shot","release=spawn-shot|12|none|0|spawn"):
            if line not in rules:
                raise ValueError(f"{stable} lost its authored static spawner policy: {line}")
        trace=launch("show-attacks.sh",stable,"--frames","150","--no-audio")
        if "event=emit " not in trace or "event=hit " not in trace:
            raise ValueError(f"{stable} did not fire its free spawner hitbox\n"+trace)
        bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
        if not bodies or "0" in bodies:
            raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
    if icbm:
        for id in ("0d06","0d07"):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            for line in ("body-motion=static-grounded",
                         "projectile=icbm|bullet|1|10|0|1|0|40|30|0|land-on-floor",
                         "actor-contact=icbm|ignore","blast=icbm|expiry|300|20|opponent",
                         "move=icbm|x|10|grounded|40|none","free-release=icbm",
                         "release=icbm|12|none|0|icbm"):
                if line not in rules:
                    raise ValueError(f"{stable} lost its authored ICBM policy: {line}")
            trace=launch("show-attacks.sh",stable,"--frames","200","--no-audio")
            if ("event=emit " not in trace or "event=projectile-landed " not in trace or
                    trace.count("event=blast-hit ")<2):
                raise ValueError(f"{stable} did not launch and land both ICBM rounds\n"+trace)
            bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if not bodies or "0" in bodies:
                raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
    if charge_field:
        for id in ("0806","0809","080f","080a","0308","0004"):
            stable=f"gg4e08:{id}"
            button="b" if id in ("080a","0308") else "x"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            for line in ("field=charge-hit|3|sphere|240|240|240|30|0|40|190|90|220|120",
                         "field-follow=charge-hit|source","field-core-damage=charge-hit|enemies|240|10|6",
                         f"move=charge-hit|{button}|10|grounded|28|none","move-charge=charge-hit|20"):
                if line not in rules:
                    raise ValueError(f"{stable} lost its authored charge policy: {line}")
            trace=launch("show-attacks.sh",stable,"--frames","150","--no-audio")
            if ("event=field-placed " not in trace or
                    not re.search(r"event=hit .* value=10\b",trace) or
                    trace.count("event=field-placed ")<2):
                raise ValueError(f"{stable} did not place and land its charge field\n"+trace)
            bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if not bodies or "0" in bodies:
                raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
    if flying_boss:
        for id in ("0e01","0e02","0e03"):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            for line in ("body-motion=static-cruise","cruise-boost=50|10000|1|2000|0|30",
                         "field-follow=wing-hit|source",
                         "field-core-damage=wing-hit|enemies|240|10|6",
                         "move=wing-hit|b|10|airborne|28|none"):
                if line not in rules:
                    raise ValueError(f"{stable} lost its authored flying-boss policy: {line}")
            trace=launch("show-attacks.sh",stable,"--frames","150","--no-audio")
            if ("event=field-placed " not in trace or
                    not re.search(r"event=hit .* value=10\b",trace)):
                raise ValueError(f"{stable} did not place and land its flying field\n"+trace)
            bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if not bodies or "0" in bodies:
                raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
    if zeta_field:
        for id in ("0705","070b","070c","070d"):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            for line in ("body-material=diffuse-overwrite-pair","idle-loop=authored-bit",
                         "walk-loop=authored-bit","stock=z-stock|1|180",
                         "field=z-hit|3|sphere|90|90|90|30|80|40|190|90|220|120",
                         "field-follow=z-hit|source","field-core-damage=z-hit|enemies|90|10|6",
                         "field-debit=z-hit|z-stock|1","move=z-hit|b|10|grounded|28|none"):
                if line not in rules:
                    raise ValueError(f"{stable} lost its authored field policy: {line}")
            trace=launch("show-attacks.sh",stable,"--frames","150","--no-audio")
            if ("event=debit " not in trace or "event=field-placed " not in trace or
                    not re.search(r"event=hit .* value=10\b",trace) or
                    trace.count("event=field-placed ")<2):
                raise ValueError(f"{stable} did not debit, place and land its following field\n"+trace)
            bodies=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if not bodies or "0" in bodies:
                raise ValueError(f"{stable} entered the round without a visible body\n"+trace)
    if ground_close:
        for id,clip,start in (("0209",1,8),("0302",0,0),("0306",0,0),("0808",30,0),("0904",6,0),("090b",6,0)):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
            if (stable not in rows or f"move=opening-slash|b|10|grounded|24|1:{clip}\n" not in rules or
                    ("pose-start=opening-slash|8\n" in rules)!=(start==8) or
                    re.search(r"(?m)^(projectile|guard|release|field)=",rules)):
                raise ValueError(f"{stable} does not retain its exact close-only recipe")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
                raise ValueError(f"{stable} did not draw or land both stockless contacts\n"+trace)
    if wire_shuriken:
        for id,choice,rule in (("0001",1,"zero-reflection=127e8|28"),("0301",0,"pose-loop-window=opening-slash|authored-bit")):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text().splitlines()
            if (stable not in rows or [s for s in rules if s.startswith("move=")]!=[
                    f"move=opening-slash|b|10|grounded|24|1:{choice}"] or rule not in rules):
                raise ValueError(f"{stable} lost its exact scoped opening")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or trace.count("borg-parts:")!=150:
                raise ValueError(f"{stable} did not draw its own body and hands or land both contacts\n"+trace)
    if late_close:
        for id in ("0101","0108","0804"):
            stable=f"gg4e08:{id}"
            package=root/f"borgs/gg4e08-{id}"
            rules=(package/"gameplay.txt").read_text().splitlines()
            if (stable not in rows or [s for s in rules if s.startswith("move=")]!=[
                    "move=opening-slash|b|10|grounded|24|1:1"] or
                    ("zero-reflection=c8d0|18" in rules)!=(id=="0101")):
                raise ValueError(f"{stable} lost its exact later close-B pose or body policy")
            trace=launch("show-attacks.sh",stable,"--no-audio")
            counts=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(counts)!=150 or "0" in counts or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace or trace.count("borg-parts:")!=150):
                raise ValueError(f"{stable} did not draw its own body and hands or land both contacts\n"+trace)
    if tank_shot:
        for id,bone,stock in (("0c00",7,20),("0c01",11,50),("0c05",6,20),("0c06",7,20)):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text().splitlines()
            if (stable not in rows or [s for s in rules if s.startswith("move=")]!=[
                    "move=basic-shot|b|10|grounded|24|none"] or f"stock=shot-stock|{stock}|180" not in rules or
                    not any(s.startswith(f"origin=shot|{bone}|") for s in rules) or any(s.startswith("melee=") for s in rules)):
                raise ValueError(f"{stable} needs its distinct paid single-shot recipe")
            trace=launch("show-attacks.sh",stable)
            body=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(body)!=150 or "0" in body or "shots=2 hits=2" not in trace or trace.count("event=launch-sound ")!=2 or
                    not re.search(r"borg-launch-audio: starts=2 blocks=[1-9]\d* nonzero=1",trace)):
                raise ValueError(f"{stable} needs two visible and audible paid shots\n"+trace)
            for rnd in (0,1):
                for event,value in (("socket-launch",bone),("debit",stock-1)):
                    if not re.search(rf"round={rnd} tick=\d+ actor=0 move=basic-shot event={event} .* value={value}\b",trace):
                        raise ValueError(f"{stable} lost its exact muzzle or debit in round {rnd}\n"+trace)
    if paired_ground:
        for id,second,stock in (("0619",23,5),("061f",27,10),("0625",27,10)):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text().splitlines()
            if (stable not in rows or f"stock=shot-stock|{stock}|180" not in rules or
                    [s for s in rules if s.startswith(("move=","release=","release-followup="))]!=[
                        "move=basic-shot|b|10|grounded|28|none","release=basic-shot|12|shot-stock|1|shot",
                        "release-followup=basic-shot|12|shot-second"]):
                raise ValueError(f"{stable} needs its exact ordered, single-debit recipe")
            trace=launch("show-attacks.sh",stable)
            body=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(body)!=150 or "0" in body or "shots=4 hits=4" not in trace or
                    not re.search(r"borg-launch-audio: starts=4 blocks=[1-9]\d* nonzero=1",trace)):
                raise ValueError(f"{stable} did not draw, hit or mix its two pairs\n"+trace)
            for rnd in (0,1):
                def events(event):
                    return [tuple(map(int,m)) for m in re.findall(
                        rf"round={rnd} tick=(\d+) actor=0 move=basic-shot event={event} "
                        r"frame=\d+ object=(\d+) recipient=(\d+) value=(\d+)",trace)]
                paid, sockets, cues, hits = (events(k) for k in ("debit","socket-launch","launch-sound","hit"))
                if len(paid)!=1:
                    raise ValueError(f"{stable} needs one debit per pair\n"+trace)
                tick=paid[0][0]
                if (paid!=[(tick,0,255,stock-1)] or len(sockets)!=2 or sockets[0][1]==sockets[1][1] or
                        [(t,bone) for t,_,_,bone in sockets]!=[(tick,25),(tick,second)] or
                        [(t,obj) for t,obj,_,_ in cues]!=[(t,obj) for t,obj,_,_ in sockets] or
                        sorted((obj,to,damage) for _,obj,to,damage in hits)!=sorted((obj,1,8) for _,obj,_,_ in sockets)):
                    raise ValueError(f"{stable} lost muzzle order, single debit, launch cue or target damage\n"+trace)
    if dragon_breath:
        # The two mechanical Dragons also fire a downward chest beam in flight.
        # Nothing stands below the scripted jump, so this asserts the launch,
        # debit, chest socket and floor retirement rather than a contact.
        command=shlex.split((root/"play.sh").read_text().splitlines()[-1])
        executable=command[1]
        stage=command[command.index("--battle-mvp-assets")+1]
        practice=command[command.index("--source-ground-assets")+1]
        for id in ("0506","0512"):
            trace=subprocess.run([executable,"--battle-borg",f"gg4e08:{id}","--against-borg","test:target",
                                  "--battle-mvp-assets",stage,"--source-ground-assets",practice,
                                  "--demo-package-battle","--trace-package-battle",
                                  "--exercise-battle-mobility","--no-audio","--headless"],
                                 cwd=root,capture_output=True,text=True,timeout=35).stderr
            if (not re.search(r"move=breath-dive event=debit .* value=4\b",trace) or
                    not re.search(r"move=breath-dive event=socket-launch .* value=12\b",trace) or
                    not re.search(r"move=breath-dive event=retire .* value=2\b",trace) or
                    "move=breath-dive event=hit " in trace):
                raise ValueError(f"gg4e08:{id} did not fire its downward chest beam\n"+trace)
        for id in ("0502","0503","0504","050e","050f","0510"):
            trace=subprocess.run([executable,"--battle-borg",f"gg4e08:{id}","--against-borg","test:target",
                                  "--battle-mvp-assets",stage,"--source-ground-assets",practice,
                                  "--demo-package-battle","--trace-package-battle",
                                  "--exercise-battle-mobility","--no-audio","--headless"],
                                 cwd=root,capture_output=True,text=True,timeout=35).stderr
            if (not re.search(r"move=breath-sweep event=debit .* value=\d+\b",trace) or
                    not re.search(r"move=breath-sweep event=emit ",trace) or
                    not re.search(r"move=breath-sweep event=stage-impact ",trace) or
                    not re.search(r"move=breath-sweep event=hit .* recipient=1 value=8\b",trace)):
                raise ValueError(f"gg4e08:{id} did not land its airborne sweep\n"+trace)
        for id,hp,pose in (("0503",2500,"0:1"),("050f",1250,"0:1"),("0504",1300,"none"),
                           ("0510",650,"none"),("0506",2000,"none"),("0512",1000,"none")):
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text().splitlines()
            expected=["move=breath-test|b|10|grounded|48|"+pose]
            if id in ("0506","0512"):
                expected.append("move=breath-dive|b|11|airborne|40|none")
                if ("projectile=dive|beam|1|30|-100|0|0|60|25|40|stop-on-floor" not in rules or
                        "origin=dive|12|0|0|0|fixed" not in rules or
                        "release=breath-dive|8|breath-stock|1|dive" not in rules):
                    raise ValueError(f"{stable} needs its authored downward chest beam")
            else:
                expected.append("move=breath-sweep|b|11|airborne|60|none")
                if ("projectile=sweep|beam|1|1|0|0|0|60|40|8|clip-on-stage" not in rules or
                        "sustained-beam=sweep|2400|6|0" not in rules or
                        "beam-sweep=sweep|1200|300|180" not in rules or
                        "origin=sweep|0|0|40|60|fixed" not in rules or
                        "release=breath-sweep|8|breath-stock|1|sweep" not in rules):
                    raise ValueError(f"{stable} needs its authored airborne sweep")
            if (stable not in rows or f"health={hp}" not in rules or
                    [s for s in rules if s.startswith("move=")]!=expected or
                    "projectile=breath|beam|1|1|0|0|0|60|30|6|ignore-floor" not in rules or
                    "sustained-beam=breath|1500|8|0" not in rules or
                    "look=breath|160|255|150|60" not in rules or
                    "origin=breath|0|0|40|60|target-height" not in rules or
                                        "activation=breath-test|release-window|24|90" not in rules or
                    "release=breath-test|12|breath-stock|1|breath" not in rules or
                    "trailing-chain=27|28|29|30|31" not in rules or "dash-policy=disabled" not in rules or
                    any(s.startswith("field") for s in rules)):
                raise ValueError(f"{stable} needs its authored sustained-beam recipe")
            trace=launch("show-attacks.sh",stable)
            body=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if (len(body)!=150 or "0" in body or "shots=2 hits=" not in trace or
                    not re.search(r"borg-chain: round=\d+ tick=\d+ actor=0 grounded=1",trace)):
                raise ValueError(f"{stable} did not fire or draw its beam body\n"+trace)
            for rnd in (0,1):
                hits=[int(n) for n in re.findall(rf"round={rnd} tick=\d+ actor=0 move=breath-test "
                                                 r"event=hit .* value=(\d+)",trace)]
                if len(hits)<2 or any(n!=6 for n in hits):
                    raise ValueError(f"{stable} lost its beam contact ticks in round {rnd}\n"+trace)
                if not re.search(rf"round={rnd} tick=\d+ actor=0 move=breath-test event=debit .* value=4\b",trace):
                    raise ValueError(f"{stable} lost its paid beam debit in round {rnd}\n"+trace)
                if not re.search(rf"round={rnd} tick=\d+ actor=0 move=breath-test event=button-release .* value=\d+",trace):
                    raise ValueError(f"{stable} lost its charged release in round {rnd}\n"+trace)
    if single_ground_shot:
        cases=(("0104","declined",24,(7,),(5,),2,"basic-shot|b"),("0802","bound",24,(12,),(10,),2,"basic-shot|b"),
               ("0d02","static",28,(8,),(40,),2,"basic-shot|b"),("0d01","static",28,(3,4),(2,2),4,"basic-shot|b"),
               ("0f06","static",28,(1,),(5,),2,"basic-shot|b"),("0c04","declined",24,(5,5,6,6),(200,),None,"basic-shot|b"),
               ("0303","declined",24,(7,),(6,),2,"x-shot|x"),("0c02","bound",24,(9,10),(50,),4,"x-shot|x"))
        for id,dash,length,sockets,stocks,total,move in cases:
            stable=f"gg4e08:{id}"
            rules=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text().splitlines()
            expected_moves=[f"move={move}|10|grounded|{length}|none"]
            if id=="0c04":
                expected_moves.append("move=cannon-beam|x|12|grounded|60|4:0")
                if ("projectile=cannon-beam|beam|1|1|0|0|0|60|40|10|ignore-floor" not in rules or
                        "sustained-beam=cannon-beam|1800|6|0" not in rules or
                        "look=cannon-beam|180|180|220|255" not in rules or
                        "origin=cannon-beam|0|0|45|70|target-height" not in rules or
                        "activation=cannon-beam|release-window|20|90" not in rules or
                        "release=cannon-beam|20|shot-stock|2|cannon-beam" not in rules):
                    raise ValueError(f"{stable} needs its authored charged X beam")
            if (stable not in rows or
                    [s for s in rules if s.startswith("move=")]!=expected_moves or
                    not all(f"stock=shot-stock|{stock}|180" in rules for stock in stocks) or
                    not any(s.startswith(f"origin=shot|{sockets[0]}|") for s in rules) or
                    any(s.startswith("melee=") for s in rules)):
                raise ValueError(f"{stable} needs its exact single-shot recipe")
            if dash=="declined":
                if ("dash-policy=disabled" not in rules or
                        any(s.startswith(("dash=","air-dash","dash-back","dash-left","dash-right")) for s in rules)):
                    raise ValueError(f"{stable} must decline dash without borrowing a clip")
            elif dash=="static":
                if ("body-motion=static-grounded" not in rules or
                        any(s.startswith(("idle=","walk=","air=","dash=","dash-policy","takeoff-clip","landing-clip"))
                            for s in rules) or "stock-clock" in rules):
                    raise ValueError(f"{stable} must stay a static turret with no borrowed locomotion")
            elif "dash=0:2" not in rules or "dash-policy" in rules or "stock-clock" in rules:
                raise ValueError(f"{stable} must keep its bound locomotion poses")
            if len([s for s in rules if s.startswith("release-followup=")])!=len(sockets)-1:
                raise ValueError(f"{stable} lost its ordered release list")
            trace=launch("show-attacks.sh",stable)
            body=re.findall(r"borg-body: round=\d+ tick=\d+ actor=0 draws=(\d+)",trace)
            if len(body)!=150 or "0" in body:
                raise ValueError(f"{stable} did not draw its body\n"+trace)
            if total is None:
                # Ultimate Cannon fires its four-shot volley and one charged X beam.
                beams=[int(v) for v in re.findall(r"move=cannon-beam event=hit .* value=(\d+)",trace)]
                if (trace.count("move=basic-shot event=socket-launch ")!=8 or
                        trace.count("move=basic-shot event=hit ")!=8 or
                        "move=cannon-beam event=button-release" not in trace or
                        not re.search(r"move=cannon-beam event=debit .* value=197\b",trace) or
                        len(beams)<2 or any(v!=10 for v in beams)):
                    raise ValueError(f"{stable} lost its volley or charged X beam\n"+trace)
                continue
            shots,hits=(4,4) if len(sockets)>1 else (2,2)
            starts=4 if len(sockets)>1 else 2
            if (f"shots={shots} hits={hits}" not in trace or
                    trace.count("event=launch-sound ")!=starts or
                    not re.search(rf"borg-launch-audio: starts={starts} blocks=[1-9]\d* nonzero=1",trace)):
                raise ValueError(f"{stable} did not fire or mix its paid shot\n"+trace)
            per_round=total//2
            name=move.split("|")[0]
            for rnd in (0,1):
                spent=int(stocks[0])-1
                if not re.search(rf"round={rnd} tick=\d+ actor=0 move={name} event=debit .* value={spent}\b",trace):
                    raise ValueError(f"{stable} lost its exact debit in round {rnd}\n"+trace)
                launched=re.findall(rf"round={rnd} tick=\d+ actor=0 move={name} event=socket-launch "
                                    r".* value=(\d+)",trace)
                if launched!=[str(bone) for bone in sockets] or len(launched)!=per_round:
                    raise ValueError(f"{stable} lost its exact muzzle order in round {rnd}\n"+trace)
    with TemporaryDirectory(prefix="gf-battle-picker-refusal-") as temp:
        trial=Path(temp)
        script=trial/"play.sh"
        copyfile(root/"play.sh",script);script.chmod(0o755)
        for name in ("gg4e08-0615","gg4e08-050b","training-target"):
            copytree(root/"borgs"/name,trial/"borgs"/name)
        (trial/"borgs/gg4e08-050b/visual/body.motions").write_bytes(b"invalid")
        bad=subprocess.run([script,"--demo-battle-pick","gg4e08:050b","--trace-package-battle","--frames","24","--no-audio",
                            *([] if opengl else ["--headless"])],cwd=trial,capture_output=True,text=True,timeout=35)
        if (bad.returncode!=2 or "battle-picker: refused id=gg4e08:050b" not in bad.stderr or
                "battle-picker: active id=" in bad.stderr or "frames=24 actors=2 restarts=0" not in bad.stderr):
            raise ValueError(bad.stderr)
        copytree(root/"borgs/gg4e08-020f",trial/"borgs/gg4e08-020f")
        gamma=trial/"borgs/gg4e08-020f/gameplay.txt"
        rules=gamma.read_text()
        replacement=rules.replace("part-record=sword|10|1\n","part-record=sword|10|255\n")
        if replacement==rules:
            raise ValueError("Gamma II exact sword declaration missing")
        gamma.write_text(replacement)
        bad=subprocess.run([script,"--demo-battle-pick","gg4e08:020f","--frames","24","--no-audio",
                            *([] if opengl else ["--headless"])],cwd=trial,capture_output=True,text=True,timeout=35)
        if (bad.returncode!=2 or "unavailable rigid part-record for sword" not in bad.stderr or
                "battle-picker: active id=" in bad.stderr or "frames=24 actors=2 restarts=0" not in bad.stderr):
            raise ValueError(bad.stderr)
        gamma.write_text("".join(line for line in rules.splitlines(keepends=True) if not line.startswith("part-")))
        showcase=trial/"show-attacks.sh"
        copyfile(root/"show-attacks.sh",showcase);showcase.chmod(0o755)
        if wire_shuriken:
            wire=trial/"borgs/gg4e08-0301"
            copytree(root/"borgs/gg4e08-0301",wire)
            manifest=wire/"gameplay.txt"; rules=manifest.read_text()
            for rejected in (rules.replace("pose-loop-window=opening-slash|authored-bit\n",""),
                             rules.replace("opening-slash|b|10|grounded|24|1:0","opening-slash|b|10|grounded|40|1:0")):
                manifest.write_text(rejected)
                bad=subprocess.run([showcase,"gg4e08:0301","--no-audio",*([] if opengl else ["--headless"])],
                                   cwd=trial,capture_output=True,text=True,timeout=35)
                if bad.returncode!=2 or "unavailable action clip or active frame outside clip" not in bad.stderr:
                    raise ValueError("Wire Girl requires the exact bounded native action window\n"+bad.stderr)
        if girl_worm:
            worm=trial/"borgs/gg4e08-050d"
            copytree(root/"borgs/gg4e08-050d",worm)
            profile=worm/"gameplay.txt"
            profile.write_text(profile.read_text().replace("zero-reflection=15cd8|7","zero-reflection=15cd8|8"))
            refusal=subprocess.run([showcase,"gg4e08:050d","--no-audio",*([] if opengl else ["--headless"])],
                                   cwd=trial,capture_output=True,text=True,timeout=35)
            if refusal.returncode!=2 or "zero-reflection mesh or authored normal count mismatch" not in refusal.stderr:
                raise ValueError("a wrong Worm declaration must refuse\n"+refusal.stderr)
        if single_ground_shot:
            beam=trial/"borgs/gg4e08-0104"
            copytree(root/"borgs/gg4e08-0104",beam)
            profile=beam/"gameplay.txt"
            rules=profile.read_text()
            if "dash-policy=disabled\n" not in rules:
                raise ValueError("Beam Gunner exact declined dash missing")
            profile.write_text(rules.replace("dash-policy=disabled\n","dash=0:2\n"))
            refusal=subprocess.run([showcase,"gg4e08:0104","--no-audio",*([] if opengl else ["--headless"])],
                                   cwd=trial,capture_output=True,text=True,timeout=35)
            if refusal.returncode!=2 or "AnimJoint" not in refusal.stderr:
                raise ValueError("the unbound source dash pose must refuse, not borrow a clip\n"+refusal.stderr)
        if reflection_bodies:
            ruby=trial/"borgs/gg4e08-020e"
            copytree(root/"borgs/gg4e08-020e",ruby)
            profile=ruby/"gameplay.txt"
            profile.write_text(profile.read_text().replace("zero-reflection=129e8|44\n",""))
            refusal=subprocess.run([showcase,"gg4e08:020e","--no-audio",*([] if opengl else ["--headless"])],
                                   cwd=trial,capture_output=True,text=True,timeout=35)
            if refusal.returncode!=2 or "invalid reflection normal/camera" not in refusal.stderr:
                raise ValueError("one declared mesh must not cover the other\n"+refusal.stderr)
        if trailing:
            id="gg4e08:0500"
            dragon=trial/"borgs/gg4e08-0500"
            copytree(root/"borgs/gg4e08-0500",dragon)
            profile=dragon/"gameplay.txt"
            rules=profile.read_text()
            declaration="trailing-chain=27|28|29|30|31\n"
            if declaration not in rules:
                raise ValueError("missing exact Dragon tail declaration")
            profile.write_text(rules.replace(declaration,"trailing-chain=27|28|30\n"))
            run_args=[showcase,id,"--no-audio",*([] if opengl else ["--headless"])]
            bad=subprocess.run(run_args,cwd=trial,capture_output=True,text=True,timeout=35)
            if bad.returncode!=2 or "must bind one exact terminal rig branch" not in bad.stderr:
                raise ValueError("detached chain must refuse\n"+bad.stderr)
            profile.write_text(rules.replace(declaration,""))
            rigid=subprocess.run(run_args,cwd=trial,capture_output=True,text=True,timeout=35)
            chain_live=launch("show-attacks.sh",id,"--no-audio")
            def events(log):
                return [line for line in log.splitlines() if "borg-api:" in line]
            if rigid.returncode or "borg-chain:" in rigid.stderr or events(rigid.stderr)!=events(chain_live):
                raise ValueError("tail changed Flame Dragon's combat events\n"+rigid.stderr)
        hidden=subprocess.run([showcase,"gg4e08:020f","--no-audio",*([] if opengl else ["--headless"])],
                              cwd=trial,capture_output=True,text=True,timeout=35)
        hidden_events=[line for line in hidden.stderr.splitlines() if "borg-api:" in line]
        if hidden.returncode or "borg-parts:" in hidden.stderr or not gamma_events or hidden_events!=gamma_events:
            raise ValueError("Gamma II's cosmetic mount changed combat events\n"+hidden.stderr)
        copytree(root/"borgs/gg4e08-0204",trial/"borgs/gg4e08-0204")
        axe=trial/"borgs/gg4e08-0204/gameplay.txt"
        rules=axe.read_text()
        if "walk-loop=authored-bit\n" not in rules:
            raise ValueError("Axe Knight needs an explicit walk policy")
        axe.write_text(rules.replace("walk-loop=authored-bit\n",""))
        bad=subprocess.run([showcase,"gg4e08:0204","--no-audio",*([] if opengl else ["--headless"])],
                            cwd=trial,capture_output=True,text=True,timeout=35)
        if bad.returncode!=2 or "unsupported AObj flags" not in bad.stderr:
            raise ValueError("unscoped Axe walk should still refuse\n"+bad.stderr)
    print(f"{len(rows)} combat selections; cross-page switches, final sticks, ranged, Dragon impact and stockless melee passed")


if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("root",type=Path)
    p.add_argument("--opengl",action="store_true")
    p.add_argument("--equipped",action="store_true")
    p.add_argument("--trailing",action="store_true")
    p.add_argument("--knight-expansion",action="store_true")
    p.add_argument("--girl-worm",action="store_true")
    p.add_argument("--reflection-bodies",action="store_true")
    p.add_argument("--close-expansion",action="store_true")
    p.add_argument("--blue-ninja",action="store_true")
    p.add_argument("--robot-diver",action="store_true")
    p.add_argument("--wing-valkyrie",action="store_true")
    p.add_argument("--robot-bomber",action="store_true")
    p.add_argument("--ground-close",action="store_true")
    p.add_argument("--wire-shuriken",action="store_true")
    p.add_argument("--late-close",action="store_true")
    p.add_argument("--tank-shot",action="store_true")
    p.add_argument("--paired-ground",action="store_true")
    p.add_argument("--single-ground-shot",action="store_true")
    p.add_argument("--dragon-breath",action="store_true")
    p.add_argument("--roach-bomb",action="store_true")
    p.add_argument("--zeta-field",action="store_true")
    p.add_argument("--charge-field",action="store_true")
    p.add_argument("--icbm",action="store_true")
    p.add_argument("--spawner-shot",action="store_true")
    p.add_argument("--flying-boss",action="store_true")
    args=p.parse_args()
    check(args.root.resolve(strict=True),opengl=args.opengl,equipped=args.equipped,trailing=args.trailing,
          knight_expansion=args.knight_expansion,girl_worm=args.girl_worm,reflection_bodies=args.reflection_bodies,
          close_expansion=args.close_expansion,blue_ninja=args.blue_ninja,robot_diver=args.robot_diver,
          wing_valkyrie=args.wing_valkyrie,robot_bomber=args.robot_bomber,ground_close=args.ground_close,
          wire_shuriken=args.wire_shuriken,late_close=args.late_close,tank_shot=args.tank_shot,paired_ground=args.paired_ground,single_ground_shot=args.single_ground_shot,dragon_breath=args.dragon_breath,roach_bomb=args.roach_bomb,zeta_field=args.zeta_field,charge_field=args.charge_field,icbm=args.icbm,spawner_shot=args.spawner_shot,flying_boss=args.flying_boss)
