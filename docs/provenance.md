# Build provenance

Canonical kernel: Thistle

## Native ARM64 build inputs

| Component | Git revision |
|---|---|
| Thistle | 0660abec7685a7500f5129454550f4c0c7703fae |
| Bake | 1ebd5d2809da7c9a4ad5e5c511522a16ad256ef6 |
| Croissant | 1fad5eebb02465d0764fcdb532e5aa40c6422398 |
| KITTYX | e62120659f5ba8de124f2a67f7d2f78eb1e12711 |

The native build generates fresh C and headers from these exact submodule
revisions in build/native-generated. The generated revision record is copied
alongside every linked image.

## Previous committed development snapshot

The existing tracked src, include and dist snapshot is retained until local
regeneration is run. It is not linked by scripts/build-arm64.sh.

| Component | Snapshot revision |
|---|---|
| Thistle | 4d0f3d6401772a48298972100e43dec1909312d6 |
| Bake | b6e10b19ae1670f2428c75f4396123e01dc07399 |
| Croissant | c2ecbeedeaece7c06fa58c4aea0162d387a46eb3 |

The snapshot objects are relocatable, not bootable executables.

KITTYX is inherited from Thistle and both compiler repositories. The
build checks that the three gitlinks agree and records the revision.
