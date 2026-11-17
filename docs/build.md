# Building Rusty Rei

Install Git, Bun, Node.js, npm and Rustup. Access to Crust is required.

```sh
git submodule update --init --recursive
rustup target add x86_64-unknown-none aarch64-unknown-none
bash scripts/regenerate.sh
bash scripts/build.sh
```

The generation step calls Bake's shared native lowering with the Rust target and passes the resulting manifest to Crust. Generated Rust files, Cargo.toml and crust.manifest.json are copied to the repository root. Review and commit these generated files as a snapshot when updating the pins.

The build script compiles both targets at reduced process priority with two jobs and warnings denied. It can also compile either target individually by accepting its target triple.

The generated crate is built without the standard library and forbids unsafe Rust. Its platform intrinsics are abstract; no board, boot code or image layout is specified here. Native libraries can be linked by downstream platform implementations. Generic kernel functions require a concrete Platform implementation before final executable code is produced.
