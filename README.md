# Rusty Rei

**REI** stands for **Rei Executes Independently**.

Rusty Rei is the platform-independent Rust native kernel source generated from canonical Thistle by Bake and Crust. It contains no C kernel, board support, bootloader or hardware-specific runtime.

```sh
git submodule update --init --recursive
rustup target add x86_64-unknown-none aarch64-unknown-none
bash scripts/regenerate.sh
bash scripts/build.sh
```

Regeneration produces the Rust crate in `src/` with its Cargo manifest. The release build produces freestanding Rust libraries for both targets. A downstream platform can instantiate the generated `Platform` trait and link its own image. A standalone boot image is not part of this branch.

See [build provenance](docs/provenance.md) and [build instructions](docs/build.md).
