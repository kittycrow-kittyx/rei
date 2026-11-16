# Architecture

Rei is the native execution target for canonical Thistle.

    Thistle
       |
      Bake (clang target)
       |
    Croissant
       |
    C source and headers
       |
    GCC / Clang
       |
    Native relocatable objects
       |
    Native intrinsics and platform layer (pending)
       |
    Linker and boot entry (pending)
       |
    Rei

The kernel implementation belongs to Thistle. Rei does not maintain
an independent implementation of the kernel.

The files in src/ and include/ are committed generated snapshots.
They must be regenerated from canonical source, not independently edited.

The native platform layer, boot implementation and linker configuration
will belong to Rei.
