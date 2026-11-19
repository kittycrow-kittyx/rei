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
