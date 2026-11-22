"""Write portable Borg add-on declarations, optionally with exact normal visual assets."""
import argparse
from hashlib import sha256
from pathlib import Path
from struct import unpack_from

from tools.extract.pzz import PzzArchive
from tools.extract.source_image import ReadOnlyFile


# These distinguish source-backed planner slices from a complete controller.
# No visual mapping, form transition, or battle permission is inferred.
MODELS = {
    "0005": ("Teleport Ninja", "ninja", 400, 0, "grounded", "unresolved",
             "normal-teleport-ninja-history"),
    "0009": ("Switching Ninja", "ninja", 380, 0, "grounded", "unresolved",
             "normal-switching-ninja-exchange"),
    "0d06": ("Death ICBM", "air", 400, 3, "continuous-flight", "boost-meter",
             "normal-death-icbm-impact"),
}

# Exact normal wrapper and local motion-bank hashes, not cross-variant aliases.
VISUALS = {
    "0005": ("a12dec2318b2b0c12ba80e172fdf21e59a1ec2fb4344c0b2dd7fb1a0363167ed",
             "8439adb2e4cdaa58161946883b9ea00bb734f5f7e95a9be0522b0dd8fb70b4e9"),
    "0009": ("fbb3434e3b9bfecc5d3c0283714f12b9c6aefc89a8548524656fcb7bc3981cd4",
             "8439adb2e4cdaa58161946883b9ea00bb734f5f7e95a9be0522b0dd8fb70b4e9"),
    "0d06": ("1022a2c4ca2d09b38ae0c9d788cf04989fafe7140201512e67cce7fbb19a2380",
             "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
}


def preview(model, pzz_path):
    with ReadOnlyFile(pzz_path) as source:
        if not 0 < source.size <= 4 << 20:
            raise ValueError("character member exceeds the input budget")
        archive = PzzArchive(source)
        wrapper, _ = archive.read(4, 2 << 20)
        motions, _ = archive.read(3, 2 << 20)
    if (sha256(wrapper).hexdigest(), sha256(motions).hexdigest()) != VISUALS[model]:
        raise ValueError("exact normal source wrapper and motion bank required")
    if len(wrapper) < 32:
        raise ValueError("normal wrapper extent")
    offset, = unpack_from(">I", wrapper)
    if not 12 <= offset <= min(65536, len(wrapper)-32) or offset % 4:
        raise ValueError("normal wrapper prefix extent")
    size, = unpack_from(">I", wrapper, offset)
    if not 32 <= size <= len(wrapper)-offset:
        raise ValueError("normal HSD archive extent")
    manifest = "model=visual/body.hsd\npreview=preview.txt\n"
    files = {"visual/body.hsd": wrapper[offset:offset+size], "visual/body.prefix": wrapper[:offset]}
    binding = (
        "GFBORG-PREVIEW 2\nmodel-format=hsd-whole-body-v1\n"
        "variant=normal\nrig-prefix=visual/body.prefix\n"
    )
    if motions:
        manifest += "animations=visual/body.motions\n"
        files["visual/body.motions"] = motions
        # Checked exact shared normal local 0:0, 0:1 and 0:2, not an action map.
        binding += "idle=0:0\nclip=walk|0:1\nclip=dash-0|0:2\n"
    else:
        binding += "idle=none\n"
    files["preview.txt"] = binding.encode()
    return manifest, files


def prepare(model, output, member=None):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("portable Borg output already exists")
    name, tribe, cost, movement_type, movement, resource, add_on = MODELS[model]
    identity = f"gg4e08:{model}"
    manifest = f"GFBORG 2\nid={identity}\nname={name}\ncatalog=catalog.txt\n"
    catalogue = (
        f"GFBORG-CATALOG 2\ndescription={name} controller audit. No playable actor.\n"
        f"tribe=gg4e08:{tribe}\ndefault-variant=normal\nvariant=normal|Normal|{cost}|placeholder\n"
        "collection-number=unresolved\ncombat-role=unresolved\nrarity-grade=unresolved\n"
        "crystals=unresolved\nalternate=unresolved\nlevel-schedule=unresolved\nexperience=unresolved\n"
        "display-jump=unresolved\nhp=unresolved|unresolved\nratings=unresolved|unresolved|unresolved|unresolved\n"
        "moves=unresolved\ntraits=unresolved\nforms=none\nfusions=unresolved\nacquisition=unresolved\n"
        "effects=unresolved\nvariant-overrides=unresolved\ncontrollers=controllers.txt\n"
    )
    recovery = "not-boosting" if movement_type == 3 else "unresolved"
    controller = (
        f"GFBORG-CONTROLLERS 2\ncontroller=normal|none\nsource=GG4E08:{model.upper()}/0\n"
        f"source-movement-type={movement_type}\nselector-family=unresolved\n"
        f"movement={movement}\ncontact=unresolved\nair-resource={resource}\nrecovery={recovery}\n"
        f"motion-bank={'none' if model == '0d06' else 'local' if member is not None else 'unresolved'}\n"
        "pose=unresolved\nactions=class-owned\nevidence=source-audit\n"
        f"add-on={add_on}\n"
    )
    files = {"catalog.txt": catalogue.encode(), "controllers.txt": controller.encode()}
    if member is not None:
        additions, assets = preview(model, member)
        manifest += additions
        files.update(assets)
    output.mkdir(parents=True, exist_ok=False)
    for name, data in (*files.items(), ("borg.manifest", manifest.encode())):
        dest = output/name
        dest.parent.mkdir(exist_ok=True)
        with dest.open("xb") as file:
            file.write(data)
    return identity


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_model", choices=MODELS)
    parser.add_argument("output", type=Path)
    parser.add_argument("--member", type=Path, help="exact extracted original plHHHH.pzz member for model preview")
    args = parser.parse_args()
    print(prepare(args.source_model, args.output, args.member))
