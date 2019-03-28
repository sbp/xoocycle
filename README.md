# Xoocycle

**Xoocycle** is an implementation of the Keccak Team's [Xoodyak](https://eprint.iacr.org/2018/767.pdf) hash and AEAD construction in under 275 lines of standard C, compatible with C90 and later versions.

This code has not been audited, either for general integrity, cryptographic suitability, or compliance to the Xoodyak specification. It does pass the Xoodyak test vectors, and has been analysed with Flawfinder, Splint, Infer, Clang SA, and Valgrind to protect against some trivial errors. Only little-endian platforms are supported.

The header file `xoocycle.h` defines twenty exported names, seventeen of which use the standard prefix `xoocycle`. The remaining three are the modern type names `u8`, `u32`, and `size`.

The code is available under the Apache License 2.0.

## Xoodyak examples

The file `xoohash.c` is a full script that takes the Xoodyak-256 hash of stdin. The file `xootest.c` contains an AEAD example. This is an excerpt from the former script:

```c
  xoocycle_cyclist(&cyc, xoocycle_empty, 0, xoocycle_empty, 0,
                   xoocycle_empty, 0);
  while (1) {
    len = read(STDIN_FILENO, io, IO);
    if (len < 0) {
      fprintf(stderr, "error\n");
      exit(EXIT_FAILURE);
    }
    if (len == 0) {
      break;
    }
    xoocycle_absorb(&cyc, io, len);
  }
  xoocycle_squeeze(&cyc, io, HASH);
  print8(io, HASH);
  xoocycle_erase(&cyc);
```

When `xoohash.c` is run on itself, it produces:

```
51f1608c0a2ccc73f72d3403e32414c2fcda33dc21a475b20d2c3e66081c4ee1
```

## Acknowledgements

Xoodyak was created by the [Keccak Team](https://keccak.team/). The `xoodoo32` permutation function in `xoocycle.c` is based on a public domain [version](https://tinycrypt.wordpress.com/2018/02/06/xoodoo-permutation-function/) by [Odzhan](https://github.com/odzhan).
