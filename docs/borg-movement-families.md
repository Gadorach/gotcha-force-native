# Candidate Borg movement families

These are **user-observed categories to guide source recovery**, not an
implemented universal movement map or a substitute for per-Borg controllers.
The current implementation work remains scoped to normal G-Red.

| Family | Resource/contact behavior reported by the user | Examples |
|---|---|---|
| Flight meter | Refills while grounded; airborne flight and mid-flight dashes use the meter | G-Red |
| Air-jump gauge | Three jump points for Sasuke; resets on landing; air dashes do not spend those points and gradually lose height | Sasuke |
| Fixed airborne stance | Static aerial position; jumping/flying are unavailable | Fortress Borgs |
| Floating/hovering | Flight-meter movement with glide; minimum hover height counts as **ground contact** and triggers the gauge reset | Mainly fairy/angel Borgs |
| Perpetual flight with boost | Gauge boosts movement speed; recovers gradually while **not boosting**; ground contact is neither required nor expected | Planes, flight transformations and some special flying Borgs |

## Important distinctions

- **Physical floor intersection is not universal ground contact.** Floating
  Borgs can reach their minimum hover height without standing on the floor;
  that state must still deliver the controller's contact/reset semantics.
- **Not boosting is not the same as grounded.** Perpetual-flight regeneration
  must not be forced through G-Red's grounded-only flight-meter recovery.
- **Jump points are not a flight meter.** Shared animation intent does not
  authorize charging Sasuke's air dashes against his three-jump gauge.
- **A similar pose is not a common rule.** Clip layouts, authored timing,
  vertical drift, resource cost and admission conditions still need per-Borg
  evidence.
- Transformations may change movement family. Which resource state survives a
  transformation, and which contact/boost producer owns it, remain unverified.

G-Red's recovered source supports a distinct metered policy. Source verification
of Sasuke's jump-point count, dash height loss and the remaining families is
deferred. Do not auto-fill animation annotations or generalize G-Red's recovered
profile from this table.
