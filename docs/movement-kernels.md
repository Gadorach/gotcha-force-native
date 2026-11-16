# Portable normal G-Red movement kernels

The headers under `engine/gameplay/character_movement*` and `gred_*` produce
bounded movement candidates. They do not construct actors, execute original
callbacks, certify collision, or grant renderer/frame receipts.

- Input heading is camera-relative; body heading is a separate state.
- Ground walk/stop/dash, jump/falling, air dash and relaunch have separate
  planners. Relaunch preserves independent lower/upper animation latches.
- Static contact uses a paired height/normal from the shared stage query.
  Steep pushes, ordinary landing callbacks and null-callback height snaps
  are distinct outcomes.
- GFMM1–8 are immutable, hash-qualified normal-G-Red operand profiles.
  Later capabilities require their corresponding version; loading newer
  operands does not authorize a caller to widen its policy.
- GFMM7 owns a separate relaunch floor-response operand: negative vertical
  speed is multiplied by 0.8 after eligible static contact. A null callback
  preserves acceleration and airborne history; it is not landing or refill.
- GFMM8 separately owns ground-loss zero/timer operands. Its correction planner
  models strict previous-height step tests and optional selector6/marker1/
  program13 metadata only. Live ledge exits remain refused until source history,
  VM flags, timer/drift/contact and native receipt ownership are connected.

Host trigonometry, square roots and floating arithmetic are native approximations,
not original hardware/MSL equivalence. Attachments, offsets, moving platforms and
unresolved contact geometry remain outside this subset. These mappings must not
be generalized to other Borgs.

CPU checks (at most two compiler jobs):

```sh
python3 -B tools/build_native.py --jobs 1 --sanitize --opt 0 \
  --tests gred_air_dash,gred_dash_jump,gred_landing_input,gred_movement_input,gred_relaunch,gred_surface_contact,gred_vertical_jump,gred_ground_correction,stage_ground_surface
```

Tests use numeric fixtures. Where a test optionally reads a qualified local
profile, it reports its absence; no game data is distributed with these kernels.

## Native gameplay acceptance

`NativeBattleMvp` supplies a separate, explicitly provisional duel policy and
bounded projectile/contact simulation. Source motion can instead be offered by
a same-world producer. Observations, producer policies and exact receipts reject
stale/foreign/copied/restarted ownership. No source-controlled tick silently
falls back to ordinary MVP movement.

The acceptance layer independently checks static surface metadata and refuses
clipped floor/push candidates. Relaunch-floor kinds remain airborne even at
exact floor height; only a separate touchdown transition grants grounded state.
The source planner/VM must commit its resource and pose state after acceptance,
not when merely preparing an offer. These are native proxy receipts, not evidence
that original whole-frame callbacks executed.
