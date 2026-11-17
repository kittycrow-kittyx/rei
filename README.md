# Rei

**REI** stands for **Rei Executes Independently**.

Rei is the native, freestanding execution target for the canonical
Thistle kernel. The kernel remains implemented in Thistle, not in an
independently maintained C fork.

## Toolchain

The upstream repositories are Git submodules, pinned to the exact revisions
used to produce the committed generated source and native objects:

| Submodule | Purpose |
|---|---|
| `thistle/` | Canonical TypeScript kernel source |
| `bake/` | Lower Thistle for the native Clang target |
| `croissant/` | Convert Bake output into modular, freestanding C |

The headers under `include/croissant/` are **generated C declarations**,
not Croissant's compiler source. The compiler itself is checked out in the
separate, pinned `croissant/` submodule.

## Repository layout

- `src/`: Croissant-generated C kernel source, divided by module.
- `include/`: Croissant-generated C headers and ABI declarations.
- `dist/`: Captured GCC and Clang relocatable ELF objects.
- `docs/`: Architecture, source provenance and build instructions.
- `scripts/`: Provenance checks and regeneration.
- `croissant.manifest.json`: Mapping of Bake modules to generated C files.
- `thistle/`, `bake/`, `croissant/`: Pinned upstream sources.

The generated C and binaries are committed development snapshots. They must
not become an independent source of truth.

## Build

```sh
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --parallel 2
bash scripts/verify-pins.sh
```

To regenerate the source, manifest, and native object snapshots from the
pinned dependencies, see [Building Rei](docs/build.md).

## Scope

The main branch contains the platform-independent generated C kernel, declarations,
build tooling and compile-only checks. Hardware startup, drivers,
platform runtimes, linked boot images and board-specific tests belong
on hardware branches. The kernel source of truth is Thistle.

## Raspberry Pi hardware branch

This branch extends the compiled C kernel with Raspberry Pi startup and
platform services. The initial Circle-backed target is Pi 5, with the
Pi 3B direct boot and QEMU checks retained for regression testing.
See [Raspberry Pi hardware build](docs/pi5-circle.md),
[native ARM64 boot](docs/native-arm64.md) and
[Pi 3B smoke test](docs/qemu-raspi3b.md).
