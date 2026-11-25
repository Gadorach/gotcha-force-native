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

static const ModernGekkoModHook hooks[] = {
    RECOMP_HOOK(0x8002E660u, observe_pair),
    RECOMP_HOOK(0x8002F108u, observe_amount),
    RECOMP_HOOK(0x8002F110u, observe_tail),
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
