# Build provenance

Canonical kernel: Thistle

## Native ARM64 build inputs

| Component | Git revision |
|---|---|
| Thistle | bd7667ec7411cedb959689d21002eaa65eb11fd5 |
| Bake | b6d6ad1c823f6a06b403a51e7deb87cf159e98f1 |
| Croissant | 07f172a71a343e2d86ea5c0db920fc8d0df3795b |
| KITTYX | 6d6dbac796d59e8ea07c52cbcd3a0b7f181da163 |

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
