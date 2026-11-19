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

The pinned Thistle and Croissant releases define the source snapshot. Bake is inherited through Croissant. Regeneration uses Bash and Bun 1.4.5 or later to execute upstream TypeScript; native compilation remains independent.
See [build instructions](docs/build.md) for regeneration and
[provenance](docs/provenance.md) for the exact revisions.

## Releases

[GitHub Releases](https://github.com/kittycrow-kittyx/rei/releases) distributes
freestanding architecture-specific archives for every target validated by
[croissant CI's final stage](https://github.com/kittycrow-kittyx/croissant/blob/7625aeb83dddfb9ce3a0afeb7be67378c48ed66e/.github/workflows/compiler.yml).
The [upstream target contract](https://github.com/kittycrow-kittyx/croissant/blob/7625aeb83dddfb9ce3a0afeb7be67378c48ed66e/targets.json)
is read from the pinned compiler release, not duplicated here. The release version
is stored in `VERSION`, independently of generator-owned files.

Each archive is named `rei-v<version>-<architecture>.tar.gz` and
contains the static library lib/librei.a and public headers, compiler
information, source provenance and the MIT licence. The platform supplies the intrinsic implementations and board support.
Each package records the upstream target-policy revision and profile.
These are kernel libraries, not bootable images.

The release workflow compiles the committed source without regenerating it,
checks the ELF architecture, and publishes the complete discovered set plus
`SHA256SUMS` only after every target succeeds. Successful validation of a push to `main` triggers the release for that exact commit and creates the tag matching `VERSION`. A manual workflow run builds archives without publishing. Use a new `VERSION` for subsequent releases.

## Licence

[MIT](LICENSE).
