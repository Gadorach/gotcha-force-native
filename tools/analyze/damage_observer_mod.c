/*
 * Read-only damage observer for the original damage writer.
 *
 * The native port cannot recover the damage amount statically: `8003D358` is
 * the `HP - r4` site, it has no static caller, and its address is never
 * materialised in main.dol. This mod attaches an entry hook there and prints the
 * target pointer, the amount and the type byte, so one bounded original run
 * settles the value for a known attacker/defender pair.
 *
 * It only observes: no patch, no register write, no replacement. Build with
 *
 *   cc -shared -fPIC -I <GotchaForce>/external/ModernGekko/include \
 *      -o damage_observer.so tools/analyze/damage_observer_mod.c
 *
 * then pass `--mods <dir containing damage_observer.so>` to moderngekko-run.
 */
#include <stdint.h>
#include <stdio.h>

#include "moderngekko/mod_abi.h"

static void observe_damage(CPUState* state)
{
    const uint32_t target = moderngekko_mod_arg_u32(state, 0); /* r3 */
    const uint32_t amount = moderngekko_mod_arg_u32(state, 1); /* r4 */
    const uint32_t kind = moderngekko_mod_arg_u32(state, 2);   /* r5 */
    fprintf(stderr, "damage-observer: target=%08X amount=%u kind=%u\n",
            target, amount, kind);
}

static const ModernGekkoModHook hooks[] = {
    RECOMP_HOOK(0x8003D358u, observe_damage),
};

static const ModernGekkoModDesc descriptor = {
    .abi_version = MODERNGEKKO_MOD_ABI_VERSION,
    .cpu_abi_version = MODERNGEKKO_CPU_ABI_VERSION,
    .cpu_state_size = sizeof(CPUState),
    .game_id = "GG4E08",
    .id = "native-port-damage-observer",
    .version = "1.0.0",
    .display_name = "Native port damage observer",
    .hooks = hooks,
    .num_hooks = sizeof(hooks) / sizeof(hooks[0]),
};

MODERNGEKKO_MOD_EXPORT const ModernGekkoModDesc* moderngekko_get_mod(void)
{
    return &descriptor;
}
