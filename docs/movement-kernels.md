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
- GFMM1–9 are immutable, hash-qualified normal-G-Red operand profiles.
  Later capabilities require their corresponding version; loading newer
  operands does not authorize a caller to widen its policy.
- GFMM7 owns a separate relaunch floor-response operand: negative vertical
  speed is multiplied by 0.8 after eligible static contact. A null callback
  preserves acceleration and airborne history; it is not landing or refill.
- GFMM8 separately owns ground-loss zero/timer operands. Its correction planner
  models strict previous-height step tests and optional selector6/marker1/
  program13 metadata only. The separate walk-only ledge owner now connects native
  frame-start history, scoped VM flag portions, delayed drift and flat landing
  through distinct receipts. Older policies retain their ledge refusal.
- GFMM9 separately owns normal-G-Red dash-recovery damping. Ordered numeric
  candidates model immediate entry motion and post-VM root speed1DAC, with the
  done/reset tail after motion. A separate ordered-ground owner now binds those
  candidates to actual AObj successors and sealed current-frame root reads.
  Its ground-only policy grants no ledge permission. A separate dash-ledge policy
  now joins ordered departure to the next priority correction/falling/flat landing,
  retaining post-motion scalar rather than inferring displacement/root momentum.
  A further combined ground-ledge policy admits both origins with their distinct
  walking-tangent versus owned-dash checks in one VM/driver/producer. Older
  walk-only, dash-only and ground-only policies retain their restrictions.
- A separate ground-jump policy joins reviewed takeoff/ascent/release-or-depletion/
  falling/flat landing to that controller. It checks jump kinematics and lineage
  independently and admits dash interruption before old root/motion dispatch.
  Its capability does not inherit air-dash/relaunch or landing cancellation.
- A further accepted-landing-entry policy admits target4/12 jump/owned-dash
  interruption only after a matching prior landing callback receipt, never on
  touchdown. It reuses source input/resource/VM/root paths; older jump and ground
  policies retain their refusals and no air-dash/relaunch capability is inherited.
- A separate combined ground-air policy now admits source air-dash wind-up/
  active/recovery with exact scalar/timer/resource/endpoint evidence and distinct
  post-air falling lineage. Relaunch, floor/push responses and positive ground-
  loss698 overlap remain refused; no earlier policy is widened.
- A separately scoped ground-air-floor policy admits wind-up-update and early-
  active null-callback snaps using both exact air integration and selected floor
  evidence. Wind-up clears48; active retains48/50. Both keep airborne history
  until the minimum timer installs a real landing callback. Wind-up entry snaps,
  relaunch, push and positive698 overlap remain refused in this combined policy.
- A separate combined ground-relaunch policy adds airborne selector8 with
  additive15 entry, independent lower/upper14/15 latches, exact numeric/resource
  evidence and distinct released-relaunch21–24 falling lineage. It reuses air-
  dash floor responses but grants no relaunch-floor, wind-up entry-floor or
  push capability. Ground-loss relaunch and positive698 overlap remain refused.
- A separate combined ground-relaunch-floor policy adds paired pre-response
  relaunch integration and static-floor evidence. Null snaps retain50/latches/
  airborne history; only negative48 receives .8. Positive crossing/lift, later
  depletion/release and callback landing remain distinct. Steep push and wind-up
  entry-floor permissions are not inherited, even on genuinely reachable routes.
- A separate combined ground-windup-entry-floor policy admits the now-qualified
  negative/positive entry snaps through genuine relaunch/flick routes. Entry
  halves44/48 without movement or gravity, then contact zeros48;50/cache/timer
  persist, with no entry cost. Handoff450 occurs after later wind-up movement
  but before contact. Push and positive698 overlap remain refused.
- A separate combined ground-surface policy admits callback landing on eligible
  selected static planes through an exact pre-response integration/height/normal/
  triangle/angle payload. It uses the qualified zero698 gate and displacement
  approach test, not a guessed UP or equal-height floor. Ground-loss landing
  retains its existing flat-only subset; steep push and positive698 overlap
  remain refused. Touchdown, landing entry and recovery are still separate.
- A separate combined ground-loss-surface policy binds callback plane evidence
  to the owned frame-history countdown: positive698 selects6144, expired698
  selects10240. Touchdown retains that timer; landing8 continues its decay.
  Positive698 air overlap and landing cancellation stay refused. This does not
  enable steep push or generalize the gate to other Borgs.
- A separate ground-loss-jump policy permits target4 jump only after accepted
  landing entry while698 remains positive. Every offered role carries the
  native-accepted timer chain, including takeoff without ground-loss metadata.
  Selector changes do not clear it. Positive698 target12 dash/flight/relaunch
  stays refused; actual ascent alone spends jump resource.
- A separate ground-loss-landing policy also admits target12 dash after accepted
  landing entry while698 persists, requiring the same all-role timer witness
  plus privately sealed actual AObj/VM root observations. Entry motion25 then
  retained24.5, active/recovery, root holds and done/reset order are unchanged.
  Nonflat ordered-dash terrain, positive698 flight and push remain refused.
- A separate ground-loss-air policy admits air-dash handoff with retained698.
  Wind-up/active contact now receives explicit caller timer evidence; default0
  remains the legacy contract. Positive698 floor/callback contacts use6144,
  expired contacts10240. Recovery suppresses direct drift while positive, as
  verified in617E4. Relaunch and steep-push permissions are not inferred.
- A separate ground-loss-relaunch policy joins verified additive lift/latches
  and null-floor responses to the actual698 chain. Explicit timer contact keeps
  AE/AC gating and negative-only .8 tail. In this new scope, the source wrapper
  clamps planar speed8 only for positive acceleration; neutral inherited scalar
  and released-relaunch lineage may retain more. Old policies remain unchanged.
- A separate ground-fall-push policy adds only ordinary downward falling6 at
  zero698, without ground-loss/post-air/relaunch provenance. It pairs the exact
  fall integration with the selected-plane projection, then damps retained
  scalar and vertical speed by .9 without changing gravity or airborne cache.
  Projection is not landing/refill; callback and landing8 remain later accepted
  roles. Native pre/post-contact bounds/body/penetration checks remain mandatory.
  Ground-loss, post-air, relaunch, dash and positive698 pushes stay refused.
- A separate ground-air-fall-push policy adds post-air falling13 only after an
  accepted recovery lower-done/next6 receipt, including immediate falling entry.
  It checks that provenance and inherited scalar before the existing .99
  integration/.9 projection tail. Both animation lanes retain program13 and
  cached87 until separate callback/landing8. Ordinary push support is inherited;
  recovery/active/wind-up callback, relaunch, ground-loss and positive698 pushes
  are not. Native raised-side and post-projection penetration refusals persist.
- The ground-relaunch-fall-push policy adds released-relaunch falling at
  zero698. It keeps the accepted coarse direction and program21–24 lineage,
  verifies the accepted meter for the release/depletion transition, and reuses
  the falling integration and selected-plane .9 response. Active relaunch and
  air-dash callback pushes are still separate and refused.
- The ground-recovery-push policy adds descending airborne recovery17/program12
  pushes at zero698. It pairs the pre-contact recovery motion with the exact
  projected endpoint and .9 retained damping. The next recovery step inherits
  that scalar and velocity but not the cleared planar acceleration. Completion
  remains after contact, and the push itself cannot land or refill.
- The ground-windup-push policy adds descending continued wind-up15 pushes at
  zero698. It preserves the exact decrement, deferred handoff cost, and
  .9 steep response rather than using the floor snap's vertical reset.
  Wind-up entry and active dash have separate push permissions and remain
  refused. Timer expiry on a verified push can hand off to active or relaunch
  on the following update without replaying the wind-up entry.
- The ground-active-push policy adds descending continued active16 pushes at
  zero698. Native acceptance verifies cost, timer, readiness and depletion
  independently of surface response. A subsequent active or recovery update
  uses the damped scalar and cleared planar acceleration. Neither dash entry
  acquires push permission, and landing still requires separate contact.
- The ground-relaunch-push policy adds descending continued relaunch8 pushes
  at zero698. It validates both lane latches, continued cost, and bounded
  relaunch integration before the .9 steep response. It does not admit
  entry pushes or replay entry lift, and released falling inherits the
  accepted damped motion.
- The ground-relaunch-entry-push policy also admits a relaunch entry push
  at zero698 if the approach is still nonpositive after the +15 lift.
  It verifies the entry cost, both lane requests and the exact post-lift
  displacement before the steep response, without replay or a landing.
- The ground-windup-entry-push policy also admits zero698 wind-up entry
  projection with nonpositive incoming vertical speed. It validates the
  half-speed motion at the unintegrated position before the .9 steep response,
  without gravity, timer decrement, or the deferred handoff cost.

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
