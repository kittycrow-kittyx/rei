# Rei

**REI** stands for **Rei Executes Independently**.

Rei is a platform-independent, freestanding C kernel. Its generated C and headers are committed source snapshots, not an independently maintained kernel.

## Build

```sh
cmake -S . -B build
cmake --build build --parallel 2
```

Compilation produces local relocatable objects. No compiled objects, binaries, images or platform implementations are committed to main.

## Source generation

The pinned Thistle, Bake and Croissant submodules define the source and generator revisions. GitHub Actions regenerates the C source and verifies it against the committed snapshot. See [build instructions](docs/build.md) and [provenance](docs/provenance.md).
