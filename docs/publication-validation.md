# Initial publication validation

## Completed on the merged tree

- Native desktop host compiled with C++20, warnings-as-errors.
- `gf_native --headless --frames 3`: exit0, no original game assets supplied.
- All native C++ headers compiled together after excluding retired pre-port
  interface/guest-memory scaffolds.
- 64 targeted native test executables passed, covering shared model/animation
  merges, menu/editor input, inventory/save, Story audio/progression, Challenge
  bootstrap and filesystem-save behavior.
- Six merged-area suites passed address/undefined-behavior/float-cast sanitizers:
  `hsd_record_selectors`, `hsd_translucent_record`, `hsd_joint_instances`,
  `story_record_selectors`, `scene_audio_pcm_source`, `pcm_queue`.
- 71 Python test cases passed across the extraction, reader and publication-audit
  suites (completed unchanged cases retained; changed cases rechecked).
  Extraction/reader tests use owned synthetic data; no original ISO was present
  for an end-to-end disc-to-playable-game validation.
- The native-only file audit checks exact staged bytes, regular-text source paths,
  excluded private/binary files and credential/merge-marker patterns.

Compilation used at most two jobs. The existing workers' worktrees/indexes were
not overwritten. This publication uses a new repository and history.

## Retained failures and corrections

The initial combined-header check exposed two obsolete duplicate interface/
guest-memory scaffolds, which were excluded from this native-only repository.
They were retained in the original research repository.

The first account-save test compile lacked its existing `--wrap=fsync` fault
injection flag. A second test was invoked without its required isolated directory
argument and exited before test work. The build/CTest configuration now includes
the original filesystem wrapper and threading/link rules. Only those failed
checks and the newly covered output-file test were subsequently run; successful
native integration mains were not repeated.

The texture-export CLI now refuses overwriting output. A fresh owned-banner
test verifies successful PNG creation, unchanged input and refused overwrite.

## Scope

These checks establish a compilable, tested consolidation of the selected native
systems. They do not establish a playable battle, complete Story integration,
physical input/audio acceptance, original hardware/pixel equivalence or complete
native-profile extraction. Device/GPU test execution was not part of publication.

CMake was unavailable on the publication host, so the direct native build runner
was used. The CMake configuration is included but was not configured/executed in
this publication run. There are additional test sources beyond the64 executed
native suites; their presence is not a claim they all ran here.
