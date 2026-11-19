# Rei

**REI**: Rei Executes Independently.

Rei is a platform-independent freestanding C kernel generated from Thistle.
The repository contains the generated source and headers.

## Build

```sh
cmake -S . -B build
cmake --build build --parallel 2
```

The build produces relocatable objects. Board support, drivers, intrinsic
implementations and image linking belong to platform integrations.

## Source generation

The pinned Thistle, Bake and Croissant revisions define the source snapshot.
See [build instructions](docs/build.md) for regeneration and
[provenance](docs/provenance.md) for the exact revisions.

## Licence

[MIT](LICENSE).
