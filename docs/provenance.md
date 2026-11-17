# Build provenance

Canonical kernel: Thistle

## Native ARM64 build inputs

| Component | Git revision |
|---|---|
| Thistle | 19a571d6da85f63256041766d5200ad50e825423 |
| Bake | 3d3d1893c6baf64f1fd771371a1e8b250ece8056 |
| Croissant | e459bfce371e3615fd1900507dc8dee04f275fe2 |

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
