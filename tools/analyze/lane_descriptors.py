"""Bounded offline lane metadata; no playback, bank fallback or actor ownership."""
import struct


def _mask(mask, lanes):
    if (type(lanes) is not int or not 1 <= lanes <= 4 or type(mask) is not int
            or mask <= 0 or mask & ~((1 << lanes) - 1)):
        raise ValueError('lane mask/domain (unsupported bits are not dropped)')


def descriptor_lanes(read, address, mask, *, supported_lanes=4):
    """Read only used fields in the source header + 8-byte-per-lane layout.

    Lane0 includes model bytes9/10; lane1 includes controls17..19. Lanes2/3
    consume fields20..24/28..32. Padding is not scanned or assigned semantics.
    ``read`` must itself enforce initialized-source bounds; short reads refuse.
    """
    _mask(mask, supported_lanes)
    if type(address) is not int or not 0 < address <= 0xffffffff:
        raise ValueError('descriptor address')
    reads = []

    def take(offset, size):
        at = address + offset
        if at + size > 1 << 32:
            raise ValueError('descriptor address wrap')
        raw = read(at, size)
        if not isinstance(raw, bytes) or len(raw) != size:
            raise ValueError('short descriptor source read')
        reads.append(dict(offset=offset, raw=raw.hex()))
        return raw

    root = struct.unpack('>b', take(0, 1))[0]
    result = []
    for lane in range(supported_lanes):
        if not mask & (1 << lane):
            continue
        offset = 4 + lane * 8
        raw = take(offset, 5)
        row = dict(lane=lane, offset=offset, group=raw[0], index=raw[1],
                   flags=raw[2], loop_start=raw[3], event=struct.unpack('>b', raw[4:])[0])
        if lane == 0:
            row['model_bytes'] = list(take(9, 2))
        elif lane == 1:
            row['control_bytes'] = list(take(17, 3))
        result.append(row)
    return dict(root_selector=root, lanes=result, reads=reads,
                required_end=max(r['offset'] + len(bytes.fromhex(r['raw'])) for r in reads))


def lane_partition(mask, count, cuts, order, mirror):
    """Validate a supplied static partition, NOT runtime frame freshness.

    Inactive slots may retain prior state; an active empty slice remains explicit.
    Mirroring must be a permutation inside each active lane, never another lane.
    """
    _mask(mask, 4)
    if (type(count) is not int or not 1 <= count <= 32 or len(cuts) != 3
            or len(order) != count or len(mirror) != count):
        raise ValueError('partition extents')
    boundaries = [0, *cuts, count]
    if any(type(x) is not int or not 0 <= x <= count for x in boundaries):
        raise ValueError('partition boundary domain')
    if boundaries != sorted(boundaries):
        raise ValueError('partition boundaries not monotone')
    if any(type(x) is not int or not 0 <= x < count for x in [*order, *mirror]):
        raise ValueError('partition bone index')
    used = set()
    rows = []
    for lane in range(4):
        if not mask & (1 << lane):
            continue
        slots = list(order[boundaries[lane]:boundaries[lane + 1]])
        if len(set(slots)) != len(slots) or used.intersection(slots):
            raise ValueError('duplicate active bone ownership')
        targets = [mirror[x] for x in slots]
        if sorted(targets) != sorted(slots):
            raise ValueError('mirror leaves active lane or is not bijective')
        used.update(slots)
        rows.append(dict(lane=lane, begin=boundaries[lane], end=boundaries[lane + 1],
                         slots=slots, mirror_targets=targets))
    return dict(lanes=rows, retained_slots=sorted(set(range(count)) - used))
