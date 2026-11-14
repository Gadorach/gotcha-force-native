"""Strict offline HSD archive table validation, not a model/scene decoder.

Layout reference: noclip.website SYSDOLPHIN.ts, revision pinned in dependencies.
No reference implementation code or original game structures are reproduced.
All offsets stay relative to the data block; no host-pointer relocation occurs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from formats import region

ROOT = Path(__file__).resolve().parents[2]
MAX_INPUT_BYTES = 64*1024*1024  # Offline resource bound, not a game-format constant.


def archive_tables(data):
    view = memoryview(data)
    header = struct.unpack(">8I", region(view, 0, 0x20))
    file_size, data_size, reloc_count, public_count, extern_count, version, reserved0, reserved1 = header
    if file_size < 0x20 or file_size > len(view):
        raise ValueError("invalid HSD declared size")
    if reserved0 or reserved1:
        raise ValueError("unsupported nonzero reserved header words")
    if any(view[file_size:]):
        raise ValueError("nonzero bytes after declared archive; specify an exact container slice")
    view = view[:file_size]
    block = region(view, 0x20, data_size)
    reloc_start = 0x20+data_size
    reloc = region(view, reloc_start, reloc_count*4)
    public_start = reloc_start+len(reloc)
    symbols = region(view, public_start, (public_count+extern_count)*8)
    string_start = public_start+len(symbols)
    strings = bytes(view[string_start:])
    pointers = []
    locations = set()
    for (location,) in struct.iter_unpack(">I", reloc):
        if location & 3 or location in locations:
            raise ValueError("unaligned or duplicate relocation field")
        locations.add(location)
        target = struct.unpack(">I", region(block, location, 4))[0]
        if target >= data_size:
            raise ValueError("relocation target outside archive data")
        pointers.append({"field_offset": location, "target_offset": target})
    exports, imports = [], []
    for index, (offset, name_offset) in enumerate(struct.iter_unpack(">II", symbols)):
        if offset >= data_size or name_offset >= len(strings):
            raise ValueError("symbol offset/name outside archive")
        end = strings.find(b"\0", name_offset)
        if end < 0:
            raise ValueError("unterminated archive symbol")
        name = strings[name_offset:end]
        if not name or any(byte < 0x20 or byte > 0x7e for byte in name):
            raise ValueError("unsupported empty/non-ASCII archive symbol")
        if index >= public_count:
            # External resolution uses a link chain. Do not follow or guess it.
            region(block, offset, 4)
        record = {"offset": offset, "name": name.decode("ascii")}
        (exports if index < public_count else imports).append(record)
    return {"declared_bytes": file_size, "container_bytes": len(data),
            "zero_padding_bytes": len(data)-file_size, "data_bytes": data_size,
            "version_word": version, "relocation_count": reloc_count,
            "publics": exports, "externs": imports, "relocations": pointers,
            "scope": "tables and pointer bounds only; symbol targets not assumed to be joints/models"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--offset", type=lambda x: int(x, 0), default=0)
    parser.add_argument("--length", type=lambda x: int(x, 0))
    args = parser.parse_args()
    if args.input.stat().st_size > MAX_INPUT_BYTES:
        raise ValueError("input exceeds offline resource bound")
    output = args.output.resolve()
    if not output.is_relative_to(ROOT/"local") or output == ROOT/"local":
        raise ValueError("original asset metadata must stay in private local storage")
    data = args.input.read_bytes()
    length = len(data)-args.offset if args.length is None else args.length
    selected = region(data, args.offset, length)
    result = archive_tables(selected)
    result.update(input_sha256=hashlib.sha256(data).hexdigest(), container_offset=args.offset)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print(json.dumps({k: result[k] for k in ("declared_bytes", "data_bytes", "relocation_count", "publics", "externs")}, indent=2))
    print("Private metadata:", output.relative_to(ROOT))


if __name__ == "__main__":
    main()
