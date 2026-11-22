# Normal G-Red and Sasuke: movement sound and effect evidence

The `GG4E08` console opening is `op.sfd`, member 2114 of
`poq_adx_usa.afs`. The reviewed member is 113,735,680 bytes, SHA-256
`9672387eaffe10e8bde1d940488dcb6eaf9df6e3ead98b2a5b699df16aba52fe`.
It has a 640×448 MPEG-1 picture stream and an ADX soundtrack. Local frame
sampling confirms this is a pre-rendered stylized battle montage, not a
bank of individual effects, rig attachments, or dry samples. Use its
picture as appearance reference only; the battle trigger and exact timing
come from the game code. Don't isolate effects by cutting sound out of the
intro soundtrack.

## Qualified original movement samples

Read-only inspection of the SHA-qualified main DOL and the exact normal
actor-data blocks gave this split:

| Cue | Observed dispatch/data | Source audio sample |
| --- | --- | --- |
| Normal Sasuke jump | Sasuke's installed movement byte at `data+65` is zero. The ordinary ascent and relaunch branches at `8005FD34–38` and `800606BC–C0` have a noncontinuous arm requesting sound ID **30** through `800F036C`. These are distinct from the branches that set an actor effect bit. | Common bank 0, tone **30**, sample/DSP **29** (mono, 44.1 kHz, one pass). |
| Normal G-Red powered movement | G-Red's corresponding byte is one. The observed branches can set bit 0 of actor byte `+0x709`. The shared `800559F8` test combines it with the enabled and previous bits; on a new qualifying edge `80055A28` calls `800F0104` with selector 0 and channel 5. Row `8031B444` selects **40** for start and **41** for stop. | Bank 0 tone **40**, sample/DSP **27**, looped; tone **41** is the stop command for the same sample. This does not make every positive vertical-speed tick an original powered tick. |
| Normal Sasuke/G-Red landing | The shared lower landing entry requests its animation, then kind **2** presentation child at `80060304 → 800B2190`. A separate guarded sound arm `80060318 → 800F07E8` indexes by the actor model's group and member. The two exact normal table entries map to common sound **15** on that arm. | Bank 0 tone **15**, sample/DSP **15** (mono, 44.1 kHz, one pass). Different landing guards and stage audio modifiers remain separate. |

Bank 0 here is `snd_com01.tsb/chd/dpk`, members **2839–2841** in `afs_data.afs`.
`tools/extract/borg_movement_sfx.py` checks those exact names, extents and
hashes before using the shared DSP decoder. Its prepared local PCM preserves
each dry decoded sample (the authored sustained cue uses only the source
loop segment of tone 40), with native-rate conversion and stereo duplication.
It does not reconstruct original pitch, gain, spatialization, voice queues,
or source start/stop timing. The package uses existing accepted `takeoff`,
`air-jump`, `landing` and `rise` cues instead; Sasuke movement is still
provisional.

## Visual effect boundary

- The shared `800B2190` factory requests a presentation object. On
  allocation success it records a parent and kind; on null it publishes
  nothing. Kind 2 selects the row at `802FC764`, with lifetime operand
  **25**, and its initial handler may copy the parent position and
  perform a support query before setup. Do not alias that row to
  every enemy or infer rendered dust in an allocation-refused trace.
- `ptcl00.ptl/ref/txg` are separate original archive members (**2836–2838**);
  they are candidates, not an authenticated mapping to Sasuke's dust.
  The exact G-Red jet bones, sprites and particle state and the full
  Sasuke landing/air-jump visual lifecycle are still unresolved.
- Keep the existing authored jet and puff visuals until the original
  emitter/texture ownership and accepted calls are joined. The new
  audio doesn't authorize visual reuse from the intro.
