"""Pinned elemental Dragon ordinary-B carriers. This does not run source code."""
from hashlib import sha256

DRAGON_KEYS = {0x0500:0,0x0509:1,0x050A:2,0x050C:3,0x0515:4,0x0516:5}
DRAGON_MOTION_SHA="7caf421dadc4f174bcb077550d38bf6a4e5d57efc9bc1123db1f868a8a100e94"
FACTORY = 0x800BE948
CONSUMER = 0x80075BF8
RELEASE = 0x80075CA4
DEBIT = 0x80075C38
DISPATCH = 0x80075C70
JUMP_TABLE = 0x802D56F8
CASE_TARGETS = (0x80075C74,0x80075C7C,0x80075C84,0x80075C8C,0x80075C94,0x80075C9C)
WRAPPER = 0x80075AA4
PHASE_TABLE = 0x802D56EC
POSE_ENTRY = 0x80075B68
POSE_REQUEST = 0x80075BE0
TEXT = (
    (WRAPPER,0x3C,"37549c554719c270cf7a795a60b13411fca96da1d58b54c3d7dbd0aca1d21f25"),
    (POSE_ENTRY,0x90,"a7ae04bac4a3b3bf1380c1d263bfbe3e3a4ea80a21a9866da8322c19edb9cf23"),
    (CONSUMER,0x128,"11ead1cd39dcc794f17f0932e07c2ad9f11612d14350b29b06c6a098125e6c7f"),
    (FACTORY,0x2D8,"4a1ffebf2d90e3f24a84988d792a160920b05cc1e05a7cf7ff1167cc5c448042"),
)

def trailing_chain(joints):
    """Verify the exact terminal tail branch for the six elemental B pilots."""
    bones=(27,28,29,30,31)
    parent=20
    lengths=[]
    for i,bone in enumerate(bones):
        if len(joints)!=32 or joints[bone]["index"]!=bone:
            raise ValueError("Dragon tail rig extent")
        joint=joints[bone]
        child=bones[i+1] if i+1<len(bones) else 255
        x,y,z=joint["local_translation"]
        if tuple(joint["child_next_parent"])!=(child,255,parent) or not (-1000<y<-1 and abs(x)<.01 and abs(z)<.01):
            raise ValueError(f"Dragon tail rig topology or link length at joint {bone}")
        lengths.append((x*x+y*y+z*z)**.5)
        parent=bone
    return {"bones":bones,"bind_local_lengths":lengths,
            "runtime":"native trailing pose with floor contact; not indexed source solver",
            "gameplay_collision":False,"source_runtime_verified":False}

def translation_scale(model,joints,motion):
    key=DRAGON_KEYS[model]
    scale=100 if key<3 else 50
    # Both cohorts use a byte-identical movement/attack bank. Check the
    # proportion of the torso, legs and tail against the reviewed larger rig.
    large={1:613.635437,21:-134.749,22:-178.150,23:-203.350,24:-134.749,
           25:-178.150,26:-203.350,27:-179.100,28:-186.150,29:-275.149,30:-267.442,31:-266.992}
    if len(joints)!=32 or sha256(motion).hexdigest()!=DRAGON_MOTION_SHA or any(
            abs(joints[i]["local_translation"][1]-y*scale/100)>.01 for i,y in large.items()):
        raise ValueError("Dragon motion translation scale needs the qualified shared bank and proportional rib, feet and tail")
    return scale


def source_matches(linker):
    # Check the full bounded dispatch table, not just the selected branch.
    return (all(linker.matches_text(*span) for span in TEXT) and
            linker.dispatch(WRAPPER,0x540)==PHASE_TABLE and
            tuple(linker.leaf(WRAPPER,0x540,i) for i in range(3))==(0x80075AE0,POSE_ENTRY,CONSUMER) and
            tuple(linker.read_word(JUMP_TABLE+i*4) for i in range(23))==
            tuple(CASE_TARGETS[DRAGON_KEYS.get(0x0500+i,0)] for i in range(23)))


def dragon_shot(linker, model, handler, row, roots, local):
    key=DRAGON_KEYS.get(model)
    if (key is None or handler!=WRAPPER or (row["state"],row["substate"],row["selector"])!=(0,0,44)
            or not source_matches(linker)):
        return None
    graph=linker.graph(CONSUMER,0x900,480,reviewed_dispatch=(DISPATCH,CASE_TARGETS[key]))
    words,incoming,_,calls,boundary,exhausted=graph
    if (boundary or exhausted or calls.get(RELEASE)!=FACTORY or words.get(CASE_TARGETS[key])!=(0x38800000|key)
            or not incoming.get(RELEASE)):
        return None
    pose=None
    requests,_,exhausted=linker.calls(POSE_ENTRY)
    if not exhausted and (POSE_REQUEST,"program",(15,2,0),"literal-block") in requests:
        pose=linker.entry_clip_binding(POSE_ENTRY,POSE_REQUEST,(15,2,0),roots,local)
    return key,pose
