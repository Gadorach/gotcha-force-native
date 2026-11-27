# Stage obstacle fade is viewport presentation

The player reports that a nearby stage obstacle becomes about 30% opaque
and looks black for the affected player, while another player sees the
normal obstacle. “About one metre” is a visual estimate, not a qualified
distance in source units. This may make aiming easier for pass-through
beams, but no Borg-specific rule or design intent is established.

## What the original callback confirms

Seven bounded `GG4E08` cases ran the side-object draw callback `800820C8`
with a supplied viewer index, object/view bit, prior fade factors and
model pointers. The model and renderer services were observed at their
call boundaries; the producer of the visibility bit was not executed.

- One view takes fade state from its own entry in the side object's
  array. With its bit set, the tested branch first caps factors above
  **0.5**, then decreases by **0.0075** per callback toward **0.25**.
  Starting at 0.30 produced an alpha request of about 0.2925. These are
  callback values, not screen opacity or game-frame rates.
- With the bit clear, a factor below 0.5 increases by 0.0075; from
  0.5 the checked branch returns it to 1.0. If an alternate descriptor
  is present and the factor differs from 1.0, it receives the alpha and
  world-matrix requests. At 1.0 the normal descriptor is selected again.
- On the same obstacle, viewer 0 requested the alternate descriptor
  while viewer 1 requested the normal descriptor and alpha 1.0.
  Visiting viewer 1 did not change viewer 0's fade factor. The checked
  negative-view path selected the normal descriptor without changing
  either factor.

The [spawn-visibility trace](battle-scene-mvp.md#spawn-contacts-and-active-view-visibility)
also identifies a producer for that bit: `80036E24` casts the selected
view's **camera-eye to camera-target segment** against eligible stage
colliders. A hit sets the corresponding object view bit; the inside
result controls a different mask. In the checked scene, side object 0
was hit only for viewer 1. That is evidence for occlusion in this path,
not a universal actor-to-wall distance threshold.

The alternate side root is prepared independently. Static DOL readback
joins constructor `80081B9C` to `80007C54`, the diffuse write
`80007CD0(0,0,0)`, and `80007F38 → 80009598`, which clears the
texture-object pointer on the supported default-material path. The
checked branch therefore uses a **texture-disabled, black-diffuse
alternate**, not a replacement black texture image. Final pixel output,
other prop classes, and the running retail fade are not qualified by
that readback. Native retail side-object publication still admits the
unoccluded branch only.

## Portable stage requirements

- Stage content declares which obstacles support a proximity fade and
  the normal and alternate appearance (or a supported tint/alpha
  policy). Do not infer it from a Borg's weapon or from every collider.
- Evaluate against the current viewpoint and its controlled actor.
  Store fade progress by **viewport and obstacle**; render passes must
  not advance it repeatedly. Do not mutate the shared asset or leave
  an override behind for the next viewport.
- A faded obstacle retains its collision, coverage and stage-object
  identity. Projectile clipping, beam pass-through and damage remain
  separate gameplay policies, independent of what each player sees.

## Authored wall demo

The [asset-free wall arena](demo-borgs.md) opts in with `view-fade.txt`:

```text
GFSTAGE-FADE 3
obstacle=occlusion|2|3|0|0.25|0.5|0.025|-620|0|-6|620|520|6
obstacle=occlusion|4|5|0|0.30|0.6|0.025|195|135|375|265|265|425
```

Each version-3 row names `gate | normal record | alternate record |
distance | minimum alpha | entry alpha | step per presented tick |
min X/Y/Z | max X/Y/Z`, with each coordinate in its own field. Declare
up to eight non-aliasing record pairs. Records 0/1 remain reserved for
the stage base; record 2 remains the default scenery when it is not
declared. Additional normal records are drawn only when declared.
Alternates are separate static, depth-tested blended records with
depth writes off. The example uses authored materials, not shared
texture mutation. The stage-query grids do not use this descriptor.

`occlusion` checks the camera segment against the declared box and
requires distance zero; `near` checks the controlled actor's distance
to the box. Legacy versions 1 (distance) and 2 (occlusion) still bind
only normal record 2. Neither choice reads retail collider masks.
Every viewport/obstacle pair has independent progress, and redrawing a
tick does not advance it. Desktop Battle Test still presents one
viewport; this does not add split-screen. The native retail stage path
still refuses the authored descriptor.

In a shared roster viewport, `occlusion` checks the eye-to-body-centre
segment for each living local in frame. It falls back to the ordinary
camera-target segment when no local is in view. `near` checks either
living local's position. Other fighters do not control the shared fade,
and the combined result still advances each obstacle only once per tick.
This is authored presentation, not per-pixel or retail visibility.

`stage_viewport_render` generates the wall and rigs, then submits blocked,
clear, and blocked-again views of the **same simulation tick** to a
recording renderer. One camera fades the wall, the other fades a
separate decorative obstacle, and both keep the other obstacle opaque.
The test checks actual vertex colour, alpha, blending, and depth writes,
then checks that the next tick advances only the drawn view. It compares
the wall's collision query before and after the draws. This tests isolation
without adding a desktop split-screen layout.

The next retail boundary is publishing the selected styled root through
the actual running view and draw schedule, beyond the existing spawn
prefix. The authored box and fade speed aren't retail settings.
