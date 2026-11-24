"""Generate the authored Workshop's local assets without any game inputs."""
from argparse import ArgumentParser
from pathlib import Path

from tools.extract.demo_borgs import arena

MANIFEST = Path(__file__).resolve().parents[2] / "examples/stages/workshop/stage.manifest"


def prepare_stage(content_root):
    stage = Path(content_root) / "stages/workshop"
    assets = arena(wall=True)
    files = {
        "stage.manifest": MANIFEST.read_bytes(),
        "art/workshop.hsd": assets["stage.hsd"],
        "collision/floor.stih": assets["floor.stih"],
        "collision/bounds.gfsr": assets["projectile-stage.gfsr"],
        "collision/walls.stih": assets["secondary.stih"],
        "collision/props.stih": assets["tertiary.stih"],
        "art/occlusion.txt": assets["view-fade.txt"].encode(),
    }
    stage.mkdir(parents=True, exist_ok=False)
    for name, data in files.items():
        file = stage / name
        file.parent.mkdir(exist_ok=True)
        with file.open("xb") as stream:
            stream.write(data)
    return stage


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("content_root", type=Path)
    args = parser.parse_args()
    try:
        print(prepare_stage(args.content_root))
    except OSError as error:
        parser.exit(2, f"{error}\n")
