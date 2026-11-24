"""Find bounded literal movement-configuration candidates in GG4E08 data.

This is static extraction, not an instruction runner or emulated install. A
literal is accepted only within one straight-line store window. Conditional
stores are retained separately and do not imply that a form is reachable.
"""
import struct


BETA_INSTALLER = 0x80198490
BETA_MODEL_CHECK = 0x80198588
BETA_OVERRIDE = (0xA80303E8,0x2C00010A,0x4C820020,0x3CA08037,0x3C808037,
                 0x3805FD68,0x900304B0,0x3804FFB8,0x900304EC,0x4E800020)


def beta_override_arm(words, installer, index, model):
    """Pin the exact model comparison and suffix, never infer from config names."""
    if (model is not None and installer==BETA_INSTALLER and installer+4*index==BETA_MODEL_CHECK and
            tuple(words[index:index+len(BETA_OVERRIDE)])==BETA_OVERRIDE):
        return model==0x010A
    return None


def signed16(word):
    value = word & 0xFFFF
    return value - 65536 if value >= 32768 else value


def written_register(word):
    opcode, rt, ra, xo = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 1) & 1023
    if opcode in (14, 15, 32, 34, 40, 42) or opcode == 31 and xo in (23, 339):
        return rt
    if opcode == 21 or opcode == 31 and xo in (444, 954):
        return ra
    if opcode in (10, 11, 36) or opcode == 31 and xo == 467:
        return None
    raise ValueError("unqualified instruction between literal and store")


def prior_writer(words, end, reg):
    for index in range(end-1, max(-1, end-17), -1):
        if written_register(words[index]) == reg:
            return index, words[index]
    raise ValueError("missing local literal definition")


def literal_at_store(words, index):
    register = words[index] >> 21 & 31
    add_at, add = prior_writer(words, index, register)
    base = add >> 16 & 31
    if add >> 26 == 14 and not base and not signed16(add):
        high_at, address = add_at, 0
    else:
        if add >> 26 != 14 or not base:
            raise ValueError("store value is not a literal address")
        high_at, high = prior_writer(words, add_at, base)
        if high >> 26 != 15 or high >> 16 & 31:
            raise ValueError("base is not a literal address")
        address = ((signed16(high) << 16) + signed16(add)) & 0xFFFFFFFF
    for site, word in enumerate(words):
        op = word >> 26
        if op not in (16, 18) or word & 2:
            continue
        bits = 16 if op == 16 else 26
        displacement = word & ((1 << bits) - 4)
        if displacement & (1 << (bits-1)):
            displacement -= 1 << bits
        if (displacement % 4 == 0 and high_at < site + displacement//4 <= index
                and not high_at <= site <= index):
            raise ValueError("branch enters the middle of the literal sequence")
    return address


def movement_config_candidates(dol, installer, model=None):
    """Collect data identities only from explicit stores to actor +0x4B0."""
    words = []
    for i in range(144):
        word, = struct.unpack(">I", dol.read(installer+4*i, 4, "text"))
        if word == 0x4E800020:  # End of the local installer fragment.
            words.append(word)
            break
        words.append(word)
    else:
        return {"status": "no bounded installer return", "candidates": []}
    candidates, refused = {}, 0
    for i, word in enumerate(words):
        if beta_override_arm(words,installer,i,model) is False:
            break
        if word & 0xFC1FFFFF != 0x900304B0:
            continue
        try:
            address = literal_at_store(words, i)
            data = dol.read(address, 192, "data")
            if data[1] > 11:
                raise ValueError("unknown source movement type")
        except ValueError:
            refused += 1
            continue
        field = (data[1], struct.unpack_from(">b", data, 17)[0], data[8:12].hex())
        candidates.setdefault((address, field), []).append(f"{installer+4*i:08X}")
    rows = [{"config": f"{address:08X}", "source_type": typ, "raw_policy": policy, "flags": flags,
             "store_sites": sites} for (address, (typ, policy, flags)), sites in sorted(candidates.items())]
    types = sorted({r["source_type"] for r in rows})
    return {"status": "static candidates" if rows else "unresolved",
            "candidates": rows, "unresolved_stores": refused, "candidate_types": types,
            "store_reachability_verified": False}
