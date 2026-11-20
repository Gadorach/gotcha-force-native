# Candidate Borg movement families

These are **user-observed categories to guide source recovery**, not an
implemented universal movement map or a substitute for per-Borg controllers.
The current implementation work remains scoped to normal G-Red.

| Family | Resource/contact behavior reported by the user | Examples |
|---|---|---|
| Flight meter | Refills while grounded; airborne flight and mid-flight dashes use the meter | G-Red |
| Air-jump gauge | Three jump points for Sasuke; points return on grounding; ordinary chained air dashes do not spend more points and reportedly lose height | Sasuke |
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

The [normal Blue Striker follow-up](borg-controller-audit.md#normal-blue-striker-cruise-boost-and-hud)
verified that refill checks boost inactivity before selector changes.
This does not mean an open-flight actor never probes the floor. The [normal
Sasuke follow-up](borg-controller-audit.md#normal-sasuke-air-points-and-hud) verified
his three-point limit, the conditional zero-to-one wind-up write, the later
grounded reset, and the HUD's hidden-at-zero-used rule. In the [Wing Soldier
and Quick Valkrie follow-up](borg-controller-audit.md#normal-wing-soldier-and-quick-valkrie-glide-and-contact),
the tested ordinary paths use zero floor offset. A snap can lead to grounding
if it runs the landing continuation and landing state, but the meter does
not reset on that event. The [Beam Satellite follow-up](borg-controller-audit.md#normal-beam-satellite-placement-stance-and-support)
found table-based placement and ordinary steering out of its stationary idle.
The [Galactic Emperor follow-up](borg-controller-audit.md#normal-galactic-emperor-class-pin-and-placement)
instead found a guarded class pin at absolute world Y=2000, separate from
constructor placement and projection, and no admitted ordinary steering.
Neither establishes a fixed-XYZ Fortress family. Other hover-height producers,
live Fortress placement and Sasuke's dash height over a full route remain
unverified. Do not fill animation annotations or generalize G-Red's recovered
profile from these audits.
