# Architecture

```text
Thistle -> Bake --target=clang -> Croissant -> generated C and headers
```

The committed source is platform-independent. The generated intrinsic ABI is supplied by downstream platform implementations. A local CMake build compiles relocatable objects; linking and imaging are outside this branch. No compiled outputs are tracked.
