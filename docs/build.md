# Building Rei

Compile the checked-in C without initialising the generator submodules:

```sh
cmake -S . -B build
cmake --build build --parallel 2
```

To regenerate the source from its pinned dependencies:

```sh
git submodule update --init --recursive
bash scripts/regenerate.sh
```

Generation ends at C source, headers, CMake configuration and manifests. It does not compile kernel objects. GitHub Actions regenerates the source, uploads the source snapshot and checks for differences. Review generated changes before committing them with their corresponding pins.

Platform-specific runtimes, drivers and boot images are outside main. Compilation outputs remain ignored by Git.

## CI and release targets

Both workflows call `.github/workflows/targets.yml` to read croissant's current
main-branch compiler workflow and the target contract used by its final job.
They compile the resulting matrix, failing if the upstream contract is missing
or incompatible. Release archives contain the exact policy commit, matrix and
profile; the publishing job checks the complete matrix before publishing.
The target policy is independent of the pinned source-generation dependencies.
