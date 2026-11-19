# Building Rei

Build the checked-in C using CMake and a native compiler.

```sh
cmake -S . -B build
cmake --build build --parallel 2
```

Regeneration requires Bash, Git, GitHub CLI authentication and Bun 1.4.5 or later. Run `bash scripts/regenerate.sh` to initialise the root pins, verify the published upstream archives, and regenerate the snapshot and checksums. Bake is owned by Croissant.

CI transpiles the source, compiles every target from the pinned upstream final validation contract, and checks the generated snapshot against the committed files. Successful main validation triggers the versioned release archives. Platform integrations supply intrinsic implementations and image linking.
