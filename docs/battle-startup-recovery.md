# Original battle startup: readiness and first input

Static review of the qualified `GG4E08` DOL. This describes the original
game's shared decisions, not a new native implementation or a completed
startup replay. It starts at the battle coordinator; intro archive delivery
and presentation are outside this cut.

## Scene gate

`80029164` selects one handler from `scene+1` **before** running the ordinary
update at `80029434`. A write by the actor update cannot change the handler
that already ran on that visit.

| Entry `scene+1` | Original decision |
|---|---|
| 0 — readiness scan (`80029218`) | Write scene status `3`. Check six physical actor slots in slot order. Skip a slot whose active byte is zero; otherwise block if its ready byte `+1859` is zero. A blocked scan does not advance. If clear and `settings+64` is zero, select step 1 with timer **120** when `scene+5597` is nonzero, or **0** otherwise. If `settings+64` is nonzero, clear the timer and advance to step 3 instead. |
| 1 — first delay (`80029388`) | Decrement the signed scene timer. When the new value is nonpositive, select step 2 and write **90** to the timer. A zero entry timer still takes this handler on a separate visit. |
| 2 — second delay (`800293DC`) | Decrement the signed timer. When the new value is nonpositive, select step 3; do not write status `4` yet. |
| 3 — active status (`80029408`) | Write scene status `4` before the actor update in this coordinator visit. |

The coordinator also has an outer transition after the update. The table
describes the branch where that outer transition does not leave this phase.
Neither a roster entry nor a delivered resource header is a ready actor.

## Actor gate and pad handoff

1. `80029434` reaches actor order and actor dispatch only when the relevant
   `scene+102` checks allow it. Inside `80054070`, actor history is copied
   before the actor's mask is compared with `scene+210`. A masked actor
   skips phase dispatch but can still publish its world later in the visit.
2. The normal phase-1 startup path dispatches by the actor's `+25`
   subphase. In the startup countdown arm, `80054BFC` runs the shared actor
   prepass before the count is decremented. With `scene+5597` nonzero,
   scene suppression or the slot-specific input check can hold that count.
   When a permitted decrement reaches zero or below, source clears
   `+1857`, sets ready `+1859=1`, advances `+25`, and clears `+26`.
3. Subphase 1 (`80054C78`) waits for scene status **exactly 4**. It then
   advances to subphase 2, clears body mask `+626`, and runs the prepass,
   **without calling pad ingest on that branch**. On the next permitted
   dispatch, subphase 2 calls `80056900` at `80054CD0` before the prepass.
   The separate subphase-2 diversion in `8005475C` can advance to subphase
   3 before dispatch; that handler calls ingest at `80054D34`.
4. In `80056900`, local input mode comes from the actor. Mode 3 first
   resets to mode 0 and clears the derived press/release edges at the
   end. Mode setting zero permits a `scene+6185` suppression check;
   nonzero mode setting skips just that check. Scene status 5 or above
   forces local input mode 2 independently. For a human in local mode 0,
   `800569E4` reads the processed slot packet and publishes held input
   and analog bytes. The clear path instead zeros held input and those
   analog bytes; local mode 1 bypasses both of those writes. History is
   saved before the choice, then press/release are derived from the old
   and new held word. Nonhuman input has its own subsequent helper.

## What this does and does not close

- **Observed separately:** The Story reserve replay reached the source
  readiness writer and subsequent mask clear with scoped input/VM
  boundaries. G-Red's ordinary movement replay ran the prepass on a
  loaded actor after input ingest. The restart replay published poses
  with phase dispatch masked.
- **Still missing:** One continuous original actor/scene trace showing
  a blocking readiness scan, the source ready write, ordered scene
  advance, and the first ordinary neutral pad decision with its current
  pose. Record the actual dispatcher visits; do not treat a status
  write or pose alone as proof that input ran.
- **Controller comparison priority:** Normal Sasuke is the next useful
  contrast to normal G-Red. Recover his walk/turn/stop, dash, takeoff,
  ascent and landing as connected source paths. Keep his three-point
  rules, authored pose changes, and direction buffering distinct from
  G-Red's meter policy. The solo demo still uses Sasuke's provisional
  controller; this review does not change it.

Validation: verified the original DOL hash, eight dispatch-table entries,
13 call edges, and 25 state stores. No guest replay or runtime change.
