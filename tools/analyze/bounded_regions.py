"""Checked 32-bit aligned region planning; metadata only, never memory access.

Every positive segment starts aligned and reserves alignment padding after it.
Zero segments are absent, not zero-sized owners. Refusal returns no partial plan.
This intentionally checks the whole supplied extent, including front padding;
it is not an implementation of an unchecked source allocator or a live lease.
"""
from dataclasses import dataclass


@dataclass(frozen=True)
class Region:
    start: int
    size: int

    @property
    def end(self):
        return self.start + self.size


@dataclass(frozen=True)
class RegionLayout:
    aligned_start: int
    front_padding: int
    reserved_bytes: int
    segments: tuple


def aligned_layout(start, capacity, sizes, *, alignment=32):
    limit = 1 << 32
    if (type(start) is not int or not 0 <= start < limit
            or type(capacity) is not int or not 0 <= capacity <= limit
            or start + capacity > limit):
        raise ValueError('32-bit enclosing region')
    if (type(alignment) is not int or not 1 <= alignment <= 1 << 31
            or alignment & (alignment - 1)):
        raise ValueError('power-of-two alignment')
    if not isinstance(sizes, (tuple, list)) or len(sizes) > 256:
        raise ValueError('bounded segment sequence')
    if any(type(size) is not int or not 0 <= size < limit for size in sizes):
        raise ValueError('32-bit segment size')
    align = lambda value: (value + alignment - 1) & -alignment
    first = align(start)
    if first >= limit:
        raise ValueError('aligned start exceeds guest pointer domain')
    cursor = first
    segments = []
    for size in sizes:
        if not size:
            segments.append(None)
            continue
        if cursor >= limit or cursor + size > limit:
            raise ValueError('segment exceeds guest pointer domain')
        segments.append(Region(cursor, size))
        cursor = align(cursor + size)
        if cursor > limit:
            raise ValueError('padding exceeds guest pointer domain')
    reserved = cursor - start
    if reserved > capacity:
        raise ValueError('region capacity includes front and trailing padding')
    return RegionLayout(first, first-start, reserved, tuple(segments))
