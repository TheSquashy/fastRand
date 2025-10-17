# Compilation

You **must** have Google Highway installed for 64ProMax to compile.

Make sure to compile with flags `-march=native -O3` at all times.
Edit the include line in `main.cpp` to use the requisite engine.

| Engine          | Description |
|:----------------|:-----------------------------------|
| fastRand32      | Uses Xoshiro128++ |
| fastRand64      | Uses Xoshiro256++ |
| fastRand64Ultra | Uses marginally faster Xoshiro256+ |
| fastRand64ProMax| Vectorized Xoshiro256+ |
