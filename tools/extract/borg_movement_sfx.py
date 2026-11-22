"""Export reviewed normal G-Red/Sasuke movement PCM from a local GG4E08 AFS."""
from argparse import ArgumentParser
from hashlib import sha256
import json
from pathlib import Path
import struct
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.source_image import ReadOnlyFile


BANK = (
    (2839, "snd_com01.tsb", 2064, "da239f349452388235c622390c17e0b536dfe7fa1f4f75f281425221a2f0431e"),
    (2840, "snd_com01.chd", 13360, "b29e8675a33bed2afaac8e89f167afc86654a4d66a0f16d7a3e45e8f44dcfd41"),
    (2841, "snd_com01.dpk", 3289888, "b69da72a62b797c38f4f3ae8a348050eaf7a57077b1280262984e869f3ae5750"),
)
DOL = "21454a8880c6254616662710d6ec626e43d853999da2cdfad700030960a94836"
TONES = {"landing": 15, "jump": 30, "thrust": 40}


def extract_bank(source):
    magic, count = struct.unpack("<4sI", source.read(0, 8))
    if magic not in (b"AFS\0", b"AFS ") or count != 2857:
        raise ValueError("not the reviewed GG4E08 data archive")
    end = 8 + count * 8
    first, _ = struct.unpack("<II", source.read(8, 8))
    directory = extent = 0
    for slot in dict.fromkeys((end, first - 8)):
        if end <= slot and slot + 8 <= first:
            directory, extent = struct.unpack("<II", source.read(slot, 8))
            if directory or extent:
                break
    if directory < end + 8 or extent != count * 48:
        raise ValueError("missing reviewed AFS name table")
    members = {}
    for index, name, size, pin in BANK:
        found = source.read(directory + index * 48, 32).split(b"\0", 1)[0]
        offset, length = struct.unpack("<II", source.read(8 + 8 * index, 8))
        if found != name.encode("ascii") or length != size or offset < end + 8:
            raise ValueError(f"not the reviewed AFS member: {index}")
        data = source.read(offset, length)
        if sha256(data).hexdigest() != pin:
            raise ValueError(f"audio bank content mismatch: {name}")
        members[name] = data
    return members


def prepare(archive, decoder, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("movement audio output already exists")
    decoder = Path(decoder).resolve(strict=True)
    with ReadOnlyFile(archive) as source:
        banks = extract_bank(source)
    outputs = {}
    with TemporaryDirectory(prefix="gf-movement-bank-") as scratch:
        files = []
        for name, data in banks.items():
            dest = Path(scratch) / name
            with dest.open("xb") as stream:
                stream.write(data)
            files.append(dest)
        for name in TONES:
            blob = run([decoder, *files, name], capture_output=True, check=True, timeout=10).stdout
            if not blob or len(blob) > 48000 * 4 * 3 or len(blob) % 4:
                raise ValueError("invalid movement PCM output")
            outputs[f"{name}.pcm"] = blob
    manifest = {
        "kind": "reviewed original common-bank movement samples; native cue/gain policy",
        "source_dol": DOL,
        "format": "stereo 48 kHz s16le",
        "tones": TONES,
        "files": {name: {"bytes": len(data), "sha256": sha256(data).hexdigest()} for name, data in outputs.items()},
    }
    output.mkdir(parents=True, exist_ok=False)
    for name, data in (*outputs.items(), ("manifest.json", (json.dumps(manifest, indent=2) + "\n").encode())):
        with (output / name).open("xb") as stream:
            stream.write(data)


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("decoder", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    prepare(args.archive, args.decoder, args.output)
