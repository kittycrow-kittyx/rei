# Rei

**REI** stands for **Rei Executes Independently**.

Rei is the native, freestanding execution target for the canonical
Thistle kernel.

## Architecture

Thistle provides the kernel implementation. Bake lowers the canonical
TypeScript into a C-compatible intermediate form. Croissant translates
that into modular C sources and headers. GCC or Clang compiles these
into native machine-code objects.

Rei will provide the native platform integration, runtime intrinsics,
linker configuration and architecture-specific boot implementation.

No separately maintained kernel implementation is permitted.

## Repository

- `src/`: Croissant-generated C kernel implementation.
- `include/`: Generated C headers and native ABI declarations.
- `dist/`: GCC and Clang native compilation artefacts.
- `docs/`: Architecture and source provenance.
- `croissant.manifest.json`: Generated module manifest.

The generated source is committed as an initial reproducible snapshot.
It is not independently maintained source code.

## Build

    cmake -S . -B build
    cmake --build build --parallel

## Current status

The complete canonical Thistle native source compiles into separate
C translation units.

GCC and Clang have produced relocatable native ELF objects.

Native runtime intrinsics remain unresolved. Rei cannot yet be linked
into a standalone executable or booted on hardware.

The next milestone is implementing and testing the native intrinsic ABI.
