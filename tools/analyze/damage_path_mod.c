/*
 * Read-only observer for the original damage path.
 *
 * The HP field is `actor+0x1C6` and the last applied amount is `actor+0x41C`.
 * The bounded audit fixture executes that tail with supplied operands
 * (`retail-0600-rifle-outcome-v1.py`), so the tail itself is known; what is not
 * known is where the amount comes from in a live battle. These three function
 * entries bracket it:
 *
 *   8002F108  shared amount computation (three integer divisions)
 *   8002F110  HP tail caller
 *   8003D344  HP subtract tail (actor in r3, amount in r4)
 *
 * The mod only observes: no patch, no register or memory write.
 */
#include <stdint.h>
#include <stdio.h>

#include "moderngekko/mod_abi.h"

static void observe_amount(CPUState* state)
{
    static unsigned count;
    if (count++ < 8u)
        fprintf(stderr, "damage-path: amount-entry #%u r3=%08X r4=%08X r5=%08X "
                        "r6=%08X r7=%08X\n",
                count, state->gpr[3], state->gpr[4], state->gpr[5],
                state->gpr[6], state->gpr[7]);
}

static void observe_tail(CPUState* state)
{
    static unsigned count;
    if (count++ < 8u)
        fprintf(stderr, "damage-path: hp-tail #%u r3=%08X r4=%08X r5=%08X\n",
                count, state->gpr[3], state->gpr[4], state->gpr[5]);
}

static void observe_apply(CPUState* state)
{
    static unsigned count;
    const uint32_t actor = state->gpr[3];
    const int16_t hp = (int16_t)moderngekko_mod_read(state, actor + 454u, 2);
    const int32_t ledger = (int32_t)moderngekko_mod_read(state, actor + 1052u, 4);
    if (count++ < 24u)
        fprintf(stderr, "damage-path: apply #%u caller=%08X actor=%08X amount=%d "
                        "hp=%d ledger=%d r5=%08X r6=%08X r7=%08X\n",
                count, state->lr, actor, (int32_t)state->gpr[4], hp, ledger,
                state->gpr[5], state->gpr[6], state->gpr[7]);
}

static int actor_id(CPUState* state, uint32_t address)
{
    if (address < 0x80000000u || address >= 0x81700000u || (address & 3u))
        return -1;
    return (int)moderngekko_mod_read(state, address + 0x3E8u, 2);
}

static int actor_hp(CPUState* state, uint32_t address)
{
    if (address < 0x80000000u || address >= 0x81700000u || (address & 3u))
        return -1;
    return (int16_t)moderngekko_mod_read(state, address + 0x1C6u, 2);
}

static void pair_entry(CPUState* state)
{
    static unsigned count;
    if (count++ < 8u)
    {
        const uint32_t records[] = {0x80365EC0u, 0x80366058u, 0x803661F0u,
                                    0x802D4160u};
        fprintf(stderr, "damage-path: pair-entry r3=%08X r4=%08X r5=%08X "
                        "r6=%08X r7=%08X lr=%08X\n",
                state->gpr[3], state->gpr[4], state->gpr[5], state->gpr[6],
                state->gpr[7], state->lr);
        for (unsigned which = 0; which < 3u; ++which)
        {
            const uint32_t base = state->gpr[3u + which];
            if (base < 0x80000000u || base >= 0x81700000u)
                continue;
            for (int offset = -0x100; offset <= 0x200; offset += 4)
            {
                const uint32_t at = base + (uint32_t)offset;
                if (at < 0x80000000u || at >= 0x81700000u)
                    continue;
                const uint32_t value = (uint32_t)moderngekko_mod_read(state, at, 4);
                for (unsigned r = 0; r < 4u; ++r)
                    if (value == records[r])
                        fprintf(stderr, "damage-path: pair-link base=%08X "
                                        "offset=%+d record=%08X\n",
                                base, offset, value);
            }
        }
    }
}

static void observe_pair(CPUState* state)
{
    static unsigned count;
    if (count++ < 8u)
        fprintf(stderr, "damage-path: pair #%u r3=%08X r4=%08X r5=%08X "
                        "r6=%08X | r4 id=%d hp=%d | r5 id=%d hp=%d\n",
                count, state->gpr[3], state->gpr[4], state->gpr[5],
                state->gpr[6], actor_id(state, state->gpr[4]),
                actor_hp(state, state->gpr[4]), actor_id(state, state->gpr[5]),
                actor_hp(state, state->gpr[5]));
}

static void observe_amount_call(CPUState* state)
{
    static unsigned count;
    const uint32_t credit = state->gpr[3];
    const uint32_t carrier = state->gpr[4];
    const uint32_t owner = (uint32_t)moderngekko_mod_read(state, carrier + 144u, 4);
    const uint32_t settings =
        (uint32_t)moderngekko_mod_read(state, state->gpr[13] - 31824u, 4);
    const uint32_t relative_bits =
        (uint32_t)moderngekko_mod_read(state, carrier + 180u, 4);
    float relative = 0.0f;
    __builtin_memcpy(&relative, &relative_bits, sizeof(relative));
    const uint32_t owner_bits =
        (uint32_t)moderngekko_mod_read(state, owner + 180u, 4);
    float owner_relative = 0.0f;
    __builtin_memcpy(&owner_relative, &owner_bits, sizeof(owner_relative));
    const uint32_t receiver_bits =
        (uint32_t)moderngekko_mod_read(state, state->gpr[5] + 180u, 4);
    float receiver_relative = 0.0f;
    __builtin_memcpy(&receiver_relative, &receiver_bits, sizeof(receiver_relative));
    if (count++ < 8u)
        fprintf(stderr,
                "damage-path: amount-call #%u credit=%08X carrier=%08X "
                "receiver=%08X | base=%u credit6=%u owner=%08X owner998=%d "
                "owner136=%d owner996=%d owner452=%d owner454=%d "
                "difficulty=%d relative=%.6f owner180=%.6f receiver180=%.6f "
                "| r5 id=%d hp=%d\n",
                count, credit, carrier, state->gpr[5],
                (unsigned)moderngekko_mod_read(state, credit, 2),
                (unsigned)moderngekko_mod_read(state, credit + 6u, 1),
                owner,
                (int)moderngekko_mod_read(state, owner + 998u, 1),
                (int)moderngekko_mod_read(state, owner + 136u, 1),
                (int)moderngekko_mod_read(state, owner + 996u, 1),
                (int)moderngekko_mod_read(state, owner + 452u, 2),
                (int)moderngekko_mod_read(state, owner + 454u, 2),
                settings ? (int)moderngekko_mod_read(state, settings + 50u, 1) : -1,
                (double)relative, (double)owner_relative,
                (double)receiver_relative,
                actor_id(state, state->gpr[5]), actor_hp(state, state->gpr[5]));
}

static void observe_division(CPUState* state, unsigned site)
{
    static unsigned count;
    if (count++ < 24u)
        fprintf(stderr, "damage-path: division site=%03X caller=%08X r3=%08X "
                        "r4=%08X r5=%08X r6=%08X\n",
                site, state->lr, state->gpr[3], state->gpr[4], state->gpr[5],
                state->gpr[6]);
}

static void observe_div_cf70(CPUState* state) { observe_division(state, 0xCF70); }
static void observe_div_d1b8(CPUState* state) { observe_division(state, 0xD1B8); }
static void observe_div_d214(CPUState* state) { observe_division(state, 0xD214); }

static void observe_return(CPUState* state, unsigned site)
{
    static unsigned count;
    if (count++ < 24u)
        fprintf(stderr, "damage-path: return site=%03X r3=%08X r4=%08X\n",
                site, state->gpr[3], state->gpr[4]);
}

static void observe_ret_cf70(CPUState* state) { observe_return(state, 0xF10C); }
static void observe_ret_d00c(CPUState* state) { observe_return(state, 0xD00C); }

static void observe_factor(CPUState* state, unsigned site)
{
    static unsigned count;
    if (count++ < 60u)
        fprintf(stderr, "damage-path: factor site=%03X f31=%.6f\n",
                site, state->fpr[31]);
}

#define FACTOR_FN(name, site) \
    static void name(CPUState* state) { observe_factor(state, site); }

FACTOR_FN(factor_d024, 0xD024)
FACTOR_FN(factor_d0b4, 0xD0B4)
static void matchup_select(CPUState* state)
{
    static unsigned count;
    const unsigned row = state->gpr[3];
    const unsigned column = state->gpr[28];
    const uint32_t address = 0x802C5D60u + row * 80u + column * 4u;
    const uint32_t bits = (uint32_t)moderngekko_mod_read(state, address, 4);
    float value = 0.0f;
    __builtin_memcpy(&value, &bits, sizeof(value));
    if (count++ < 4u)
        fprintf(stderr, "damage-path: matchup row=%u column=%u value=%.6f\n",
                row, column, (double)value);
}

static void scalar_index(CPUState* state)
{
    static unsigned count;
    const uint32_t settings = state->gpr[7];
    const uint32_t team = state->gpr[4];
    const int index = (int)moderngekko_mod_read(state, settings + team, 1);
    if (count++ < 4u)
        fprintf(stderr, "damage-path: scalar settings=%08X team=%08X index=%d\n",
                settings, team, index);
}

static void chain_d0d8(CPUState* state)
{
    static unsigned count;
    if (count++ < 4u)
        fprintf(stderr, "damage-path: chain D0D8 table=%08X offset=%08X ptr=%08X\n",
                state->gpr[9], state->gpr[0], state->gpr[10]);
}

static void chain_d0ec(CPUState* state)
{
    static unsigned count;
    if (count++ < 4u)
        fprintf(stderr, "damage-path: chain D0EC row=%08X index=%08X outer=%08X\n",
                state->gpr[10], state->gpr[9], state->gpr[8]);
}

static void chain_d0f8(CPUState* state)
{
    static unsigned count;
    if (count++ < 4u)
        fprintf(stderr, "damage-path: chain D0F8 row=%08X offset=%08X\n",
                state->gpr[10], state->gpr[8]);
}

static void factor_d100(CPUState* state)
{
    static unsigned count;
    if (count++ < 8u)
        fprintf(stderr, "damage-path: d100 f31=%.6f f0=%.6f index=%d actor=%08X\n",
                state->fpr[31], state->fpr[0], (int)state->gpr[8],
                state->gpr[6]);
}
FACTOR_FN(factor_d128, 0xD128)
FACTOR_FN(factor_d16c, 0xD16C)

static void factor_d134(CPUState* state)
{
    static unsigned count;
    const uint32_t actor = state->gpr[6];
    const uint32_t bits =
        (uint32_t)moderngekko_mod_read(state, actor + 180u, 4);
    float relative = 0.0f;
    __builtin_memcpy(&relative, &bits, sizeof(relative));
    if (count++ < 8u)
        fprintf(stderr, "damage-path: relative-source actor=%08X id=%d "
                        "plus180=%.6f\n",
                actor, actor_id(state, actor), (double)relative);
}
FACTOR_FN(factor_d1ec, 0xD1EC)
FACTOR_FN(factor_d260, 0xD260)
FACTOR_FN(factor_d270, 0xD270)
static void factor_d290(CPUState* state)
{
    static unsigned count;
    const uint32_t actor = state->gpr[6];
    const int tier = (int8_t)moderngekko_mod_read(state, actor + 1738u, 1);
    if (count++ < 8u)
        fprintf(stderr, "damage-path: d290 f31=%.6f f0=%.6f index=%d tier6ca=%d "
                        "actor=%08X\n",
                state->fpr[31], state->fpr[0], (int)state->gpr[0], tier, actor);
}
FACTOR_FN(factor_d2a4, 0xD2A4)
FACTOR_FN(factor_d31c, 0xD31C)

static const ModernGekkoModHook hooks[] = {
    RECOMP_HOOK(0x8002E2A8u, pair_entry),
    RECOMP_HOOK(0x8002E660u, observe_pair),
    RECOMP_HOOK(0x8003CD5Cu, observe_amount_call),
    RECOMP_HOOK(0x8002F108u, observe_amount),
    RECOMP_HOOK(0x8002F110u, observe_tail),
    RECOMP_HOOK(0x8003D024u, factor_d024),
    RECOMP_HOOK(0x8003D0B4u, factor_d0b4),
    RECOMP_HOOK(0x8003D00Cu, matchup_select),
    RECOMP_HOOK(0x8003D0E0u, scalar_index),
    RECOMP_HOOK(0x8003D0D8u, chain_d0d8),
    RECOMP_HOOK(0x8003D0ECu, chain_d0ec),
    RECOMP_HOOK(0x8003D0F8u, chain_d0f8),
    RECOMP_HOOK(0x8003D100u, factor_d100),
    RECOMP_HOOK(0x8003D128u, factor_d128),
    RECOMP_HOOK(0x8003D134u, factor_d134),
    RECOMP_HOOK(0x8003D16Cu, factor_d16c),
    RECOMP_HOOK(0x8003D1ECu, factor_d1ec),
    RECOMP_HOOK(0x8003D260u, factor_d260),
    RECOMP_HOOK(0x8003D270u, factor_d270),
    RECOMP_HOOK(0x8003D290u, factor_d290),
    RECOMP_HOOK(0x8003D2A4u, factor_d2a4),
    RECOMP_HOOK(0x8003D31Cu, factor_d31c),
    RECOMP_HOOK(0x8003CF70u, observe_div_cf70),
    RECOMP_HOOK(0x8003D1B8u, observe_div_d1b8),
    RECOMP_HOOK(0x8003D214u, observe_div_d214),
    RECOMP_HOOK(0x8002F10Cu, observe_ret_cf70),
    RECOMP_HOOK(0x8003D00Cu, observe_ret_d00c),
    RECOMP_HOOK(0x8003D344u, observe_apply),
};

static const ModernGekkoModDesc descriptor = {
    .abi_version = MODERNGEKKO_MOD_ABI_VERSION,
    .cpu_abi_version = MODERNGEKKO_CPU_ABI_VERSION,
    .cpu_state_size = sizeof(CPUState),
    .game_id = "GG4E08",
    .id = "native-port-damage-path",
    .version = "1.0.0",
    .display_name = "Native port damage path observer",
    .hooks = hooks,
    .num_hooks = sizeof(hooks) / sizeof(hooks[0]),
};

MODERNGEKKO_MOD_EXPORT const ModernGekkoModDesc* moderngekko_get_mod(void)
{
    return &descriptor;
}
