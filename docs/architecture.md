# Architecture

Rei is the native execution target for the canonical Thistle kernel.

```text
thistle/ (pinned canonical TypeScript)
  |
bake/ --target=clang (pinned lowering)
  |
croissant/ (pinned compiler)
  |
src/ and include/ (generated, committed snapshots)
  |
GCC / Clang
  |
dist/ (native relocatable objects)
  |
platform-independent kernel build boundary
```

Thistle owns the kernel implementation. Rei must never keep an
independently edited C equivalent of that implementation. Croissant
generates one C source and a corresponding header per lowered module.
The include path `include/croissant/` names the generator's ABI/header
namespace; the actual compiler checkout is the Git submodule
`croissant/`.

The three Git submodule gitlinks pin exactly the source revisions used
for this snapshot. The revisions are recorded in
[provenance.md](provenance.md) and verified by
`scripts/verify-pins.sh` during CI.

Main ends at the generated C kernel and its relocatable objects.
Platform branches provide native intrinsic implementations, hardware
startup, drivers and linking without editing the generated kernel.
