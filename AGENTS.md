# AGENTS MANUAL

This document outlines project structure, tooling, and expected workflows so AI agents can execute tasks safely and consistently.

## Repository Map

- `include/integer.hpp`: Single translation unit containing the big integer implementation. **Do not split** unless instructed; other code must adapt around this file.
- `include/md5.hpp` + `src/md5.cpp`: MD5 helper functions for regression hashes.
- `apps/driver/main.cpp`: Lightweight CLI that runs hash regressions and a micro-benchmark; useful for smoke tests.
- `tests/`: Catch2-based test suite (`test_integer.cpp`) that validates arithmetic against a decimal reference implementation and checks MD5 vectors.
- `benchmarks/bench_integer.cpp`: Google Benchmark harness for multiply/add/sub/div across large digit counts.
- `scripts/bench.py`: Runs the benchmarks, compares against `data/baseline.json`, and writes timestamped reports to `results/`.
- `scripts/update_baseline.py`: Refreshes `data/baseline.json` with current measurements (only run after verifying improvements).
- `data/baseline.json`: Reference timings used for scoring benchmark runs.
- `docs/DEVELOPMENT.md`: Human-readable development workflow (configure/build/test/benchmark).

## Build & Test Commands

1. Configure:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build:
   ```bash
   cmake --build build
   ```
3. Run tests:
   ```bash
   ctest --test-dir build
   ```
4. Run benchmark & report:
   ```bash
   scripts/bench.py --build-dir build
   ```

Sanitizer builds can be enabled with `-DINTEGER_ENABLE_SANITIZERS=ON` during configuration.

## CI/Automation Expectations

- Always run unit tests before modifying `data/baseline.json`.
- Benchmark reports are stored under `results/` (already git-ignored).
- Scripts are Python 3; no external dependencies beyond the standard library.

## Coding Guidelines

- C++20 standard with warnings on (`CompilerSettings.cmake` handles flags).
- Keep `integer.hpp` self-contained; avoid introducing new global state in headers.
- Prefer deterministic tests (fixed RNG seed) and avoid UB-prone constructs.
- When adding benchmarks, ensure they log digit counts and reuse existing helpers.
- Update documentation (`README.md`, `docs/DEVELOPMENT.md`, `AGENTS.md`) if workflows change.

## Checklist for Agents

- [ ] Verify CMake is installed before running builds/tests.
- [ ] Use `cmake --build` targets; do not compile sources manually.
- [ ] Keep baseline updates deliberate—only run `scripts/update_baseline.py` after confirming improvements.
- [ ] Respect directory structure; do not split `integer.hpp` unless a user explicitly requests it.
- [ ] Capture benchmark output via `--benchmark_format=json` if custom tooling is needed.

Following this guide ensures automated contributions align with the project’s layout and performance tracking requirements.

## Reference Notes

- 2025-10-26: Verified that the detailed technical review of `include/integer.hpp` (FFT packing tricks, mixed multiplication regimes, long-division strategy, rounding/precision guards) matches the current implementation. Clarifications: there is no exposed correlation API despite internal support, and `FFT::square` omits the final conjugation because the unpacked outputs are real-valued by construction.

Here’s the technical read—no fluff—of what this code does and how it works under the hood.

# 1) High-level picture

* **Purpose.** Two parts:

  1. `namespace FFT`: a fast, allocation-light implementation of complex FFT plus a set of tricks to compute *real* convolutions and correlations using **one** complex FFT (packing tricks), with iterative Cooley–Tukey butterflies and precomputed roots.
  2. `struct integer`: a base-10⁴ big integer with **three** multiplication regimes:

     * schoolbook (small sizes),
     * Karatsuba (medium),
     * FFT-based convolution (large).
       Division is long-division with a floating estimate. Everything is tuned to avoid overflow and keep floating-point error within safe bounds.

---

# 2) FFT core (data structures and primitives)

## 2.1 Custom complex

* Minimal `complex<float_t>`: only real `x`, imaginary `y`, plus `+`, `-`, real/imag setters, multiplication, and `conjugation()`.
* `polar(mag, angle)` builds a root of unity without relying on `<complex>`. This keeps the compiler free to inline aggressively and avoids libstdc++ overhead.

## 2.2 Global tables

* `roots`: precomputed complex roots of unity arranged in a layout that matches the iterative butterfly loops.
* `bit_reverse`: the standard bit-reversal permutation for sizes that are exact powers of two.
* Everything is **reused** across calls (amortized), but **not thread-safe** (global mutables).

## 2.3 Power-of-two sizing and permutations

* `round_up_power_two(n)` returns the next power of two (or `n` if already one) using `__builtin_clz`.
* `bit_reorder(n, values)` computes/uses the bit-reversal permutation for length `n` and swaps `values` in-place.

## 2.4 Root generation

* `prepare_roots(n)` grows `roots` up to `n` once. It fills level by level:

  * for stage `length` it sets even positions by copying, odd positions with `exp(i * angle)` where `angle = 2π / 2^{length+1}` times an odd index.
  * Layout is chosen so butterflies can access `roots[len + i]` directly during the stage with block size `2*len`.

## 2.5 Iterative FFT

* `fft_iterative(n, values)`:

  1. `prepare_roots(n)`, `bit_reorder(n, values)`
  2. For `len = 1, 2, 4, ...`:

     ```
     for each block of 2*len:
       for i in 0..len-1:
         even = v[start+i]
         odd  = v[start+len+i] * roots[len+i]
         v[start+i]       = even + odd
         v[start+len+i]   = even - odd
     ```
* This is a standard radix-2 DIT Cooley–Tukey arranged to hit cache well and minimize branches.

---

# 3) Single-FFT packing tricks (the interesting part)

The code uses two related identities to avoid doing two FFTs when convolving real arrays.

Let `A` and `B` be real sequences, length padded to `N` (power of two). Define

```
X_k = FFT( A + i B )_k
Y_k = FFT(A)_k,  Z_k = FFT(B)_k
```

Then, using Hermitian symmetry,

```
Y_k = (X_k + conj(X_{-k}))/2
Z_k = (X_k - conj(X_{-k}))/(2i)
```

From this, the pointwise product `Y_k * Z_k` (needed for convolution) is

```
Y_k * Z_k = (i/4) * ( conj(X_{-k})^2 - X_k^2 )
```

The code implements that algebra in `extract(..., side = -1)`:

```cpp
(i/4) * ( conj(values[j])^2 - values[i]^2 )
```

where `j = (-i) mod N` computed as `(N - i) & (N - 1)`.

There’s also a second packing used for *squaring* a single real sequence (halve work and memory): pair adjacent samples `(a0,a1), (a2,a3), ...` into complex numbers `a0 + i a1`, `a2 + i a3`, …; run one FFT of length `N`; then separate “even” and “odd” parts by linear combinations of `X_k` and `conj(X_{-k})`. That’s what `extract(..., side = 0/1)` returns: the two separated spectra needed to reconstruct the square without a second FFT.

---

# 4) Inverse transforms (two flavors)

* **General inverse via forward FFT:** `square()` uses the identity
  `IFFT(V) = (1/N) * conj( FFT( conj(V) ) )`
  It multiplies by `1/N` and conjugates, runs the forward FFT once more, then disentangles the interleaved outputs.

* **Specialized inverse for the single-FFT product path:** `invert_fft(n, values)` expects the frequency array built as in `multiply()` and performs a tailored unmixing:

  1. Conjugate and scale by `1/n`.
  2. Combine the first and second halves with twiddles (`roots[n/2 + i]`) to undo the packing.
  3. Run `fft_iterative(n/2, values)`.
  4. Expand to length `n` by interleaving real/imag parts: even indices get real, odd get imag.

End result: time-domain real convolution lands in the **real parts**; the code then rounds when the requested `T_out` is integral.

---

# 5) Convolution APIs

## 5.1 `FFT::multiply<T_out>(left, right, circular=false)`

* **Brute-force vs FFT chooser.** It estimates costs:

  * brute: `≈ 0.55 * n * m`
  * FFT:   `≈ 1.5 * N * (log2 N + 3)` with `N = round_up_power_two(output_size)`.
  * If `circular == false`, `output_size = n + m - 1` (linear convolution); else `output_size` is a power of two large enough for the desired circular length (so FFT naturally wraps).
* **Single-FFT packing.** Builds a complex array `values` of length `N`:

  * `values[i].real(left[i])`, `values[i].imag(right[i])`.
  * One forward FFT.
  * For each `i ∈ [0..N/2]`:

    * `j = (-i) mod N`.
    * `product_i = extract(N, values, i, -1)` computes `FFT(left)_i * FFT(right)_i` directly from `X_i` and `X_j`.
    * Fill `values[i] = product_i`, `values[j] = conj(product_i)` to enforce Hermitian symmetry of a real output.
  * `invert_fft(N, values)` to get the time signal; copy the real parts to the result and `round()` if `T_out` is integral.
* **Circular vs linear.** Linear needs `N ≥ n+m−1`. The code sets that when `circular == false`. For circular, it uses a shorter `N`, so the natural wrap-around of FFT implements circular convolution.

## 5.2 `FFT::square<T_out>(input)`

* **Brute-force vs FFT chooser.** Similar heuristic, with tuned constants.
* **Pair-packing for squaring.**

  * Pack `(a[0],a[1])` as `a[0] + i a[1]` into `values[0]`, `(a[2],a[3])` into `values[1]`, etc.
  * One forward FFT on length `N`.
  * For each `i` and `j = (-i) mod N`, call:

    * `even = extract(N, values, i, 0)` and `odd = extract(N, values, i, 1)` to recover the two relevant spectra.
    * Algebra combines these to form the spectrum of the *square* (the code computes `aux`, `tmp` and writes both `i` and `j` entries respecting conjugate symmetry).
  * Apply inverse via the “forward-again” trick (conjugate, scale `1/N`, forward FFT).
  * Unpack: output coefficients are interleaved real/imag of the FFT result; map even indices to `real`, odd to `imag`, and round if integral.

---

# 6) Numerical considerations

* **Precision.** Uses `double`. The code rounds outputs (`round(...)`) for integer `T_out`; that, plus the modest per-digit base in the big integer (10⁴), keeps the largest convolution coefficients far below the `2^53` exact-integer limit of doubles for the configured thresholds.
* **Guard rails.**

  * `INTEGER_FFT_CUTOFF = 1500` and `KARATSUBA_CUTOFF = 150` are chosen so that when FFT is used, the peak sums (≈ `#terms * BASE^2`) stay in the safe integer-exact range of `double`.
  * There are vestigial constants `SPLIT_CUTOFF` and `SPLIT_BASE` for coefficient splitting (classic technique to push precision further) but they’re **not used** here.

---

# 7) Big integer (`struct integer`) design

## 7.1 Storage

* Base **10⁴** (`BASE = 1e4`) in little-endian blocks in `vector<uint16_t> values`.
* `SECTION = 4` tells string I/O to print each block as 4 decimal digits (with leading zeros except for the most significant block).
* `trim_check()` removes leading zeros and keeps a single zero at minimum.

## 7.2 Basic ops

* **Comparison** scans from most significant block.
* **Shift** (`operator<<(p)`) moves blocks (i.e., multiply by `BASE^p`).
* **Range slicing** supports Karatsuba splitting: `range(a,b)` returns `[a,b)`.
* **Add/Sub** does base-10⁴ arithmetic with carry/borrow.

## 7.3 Multiplication (three regimes)

Let `n = |a|`, `m = |b|` in blocks; the code ensures `n ≤ m`.

1. **FFT-based (largest):**
   Triggered when `n > KARATSUBA_CUTOFF` **and** `n + m > INTEGER_FFT_CUTOFF`.
   Steps:

   * Convolve `a.values` and `b.values` with `FFT::multiply<uint64_t>`.
   * Normalize: propagate carries in base 10⁴ into a fresh `integer product`.
   * Rationale: O((n+m) log(n+m)) with low constant thanks to the single-FFT trick.

2. **Karatsuba (medium):**
   Triggered when `n > KARATSUBA_CUTOFF` but below the FFT threshold.

   * Split each operand at `mid = n/2`: `a = a2·B^mid + a1`, `b = b2·B^mid + b1`.
   * Compute `x = a2*b2`, `z = a1*b1`, `y = (a1+a2)*(b1+b2) - x - z`.
   * Recombine: `(x << 2*mid) + (y << mid) + z`.
   * Complexity: ~O(n^log₂3) ≈ O(n^1.585).

3. **Schoolbook (small):**
   Diagonal accumulation across `index_sum = 0..n+m-2`:

   * Accumulates partial products into `value`/`carry`, flushing to avoid `uint64_t` overflow using:

     * `U64_BOUND = max_u64 - BASE*BASE` (keeps headroom),
     * `BASE_OVERFLOW_CUTOFF = max_u64 / BASE` (decides when scalar multiplication must delegate to big-int path).
   * Complexity: O(nm) with tight inner loop.

## 7.4 Scalar multiply

* `operator*(uint64_t)` uses the schoolbook path unless the scalar is too large (`scalar >= BASE_OVERFLOW_CUTOFF`), in which case it converts the scalar to `integer` and falls back to full multiplication. This avoids 64-bit overflow during per-digit scaling.

## 7.5 Division and modulus

### Division by big integer

* **Algorithm:** normalized long division with a floating estimate.

  * `estimate_div(other)` computes a double-precision estimate of the leading quotient digit using up to `DOUBLE_DIV_SECTIONS = 5` most significant blocks of dividend/divisor (as a fixed-point fraction scaled by `BASE^{n-m}`).
  * Adjusts the estimate *down or up by 1* in a tight loop until `scalar ≤ chunk` and `scalar + other > chunk`. This bounds correction work.
  * Subtracts `scalar << i` from the remainder and writes `div` to the quotient at position `i`.
* **Complexity:** ≈ O(nm) in the worst case; constants are reduced by the decent initial estimate.

### Division by `uint64_t`

* Fast path when `denominator < BASE_OVERFLOW_CUTOFF`:

  * Scans from most significant block, updating `remainder = BASE * remainder + values[i]` and emitting quotient blocks when possible.
* Slow path delegates to big-int division if the denominator is too large to keep `uint64_t` intermediates safe.

---

# 8) Error control, rounding, and stability

* **Rounding:** Whenever the requested output type is integral (`if constexpr (is_integral_v<T_out>)`), the code applies `round(...)` to the real part after the inverse. This is the standard fix for harmless ±1 ulp jitter from floating arithmetic.
* **Base choice:** With base 10⁴, the largest middle coefficient in a product of length ~1500 is about `≈ 1500 * (BASE−1)^2 < 1.5e11`, which is **exactly representable** in `double` (safe up to 2^53 ≈ 9e15). That’s why the FFT cutoff is set where it is.
* **Carry discipline:** The schoolbook path uses `U64_BOUND` to flush carries before `uint64_t` overflows.

---

# 9) Complexity summary

* `FFT::fft_iterative`: O(N log N).
* `FFT::multiply` / `square` (FFT route): O(N log N) time, O(N) memory; single FFT thanks to packing.
* Big-int:

  * Add/Sub/Compare: O(n)
  * Mul:

    * small: O(nm)
    * medium (Karatsuba): ~O(n^1.585)
    * large (FFT): O(n log n)
  * Div/Mod: ~O(nm) with good constants.

---

# 10) Notable engineering details and caveats

* **Portability:** Uses GCC/Clang builtins (`__builtin_clz`, `__builtin_ctz`) and `#pragma GCC optimize("O3")`. Non-GCC compilers may need tweaks.
* **Thread safety:** `roots` and `bit_reverse` are global and mutated; concurrent calls race unless externally synchronized.
* **Indexing trick:** `(N - i) & (N - 1)` computes `(-i) mod N` because `N` is a power of two; it’s cheaper than `% N`.
* **Inverse path variants:** Two inverse strategies are used:

  * general “conj + forward FFT” identity (`square()`),
  * custom inverse that unpacks interleaved components efficiently (`invert_fft()`).
* **Dead code hints:** `SPLIT_CUTOFF` and `SPLIT_BASE` suggest planned coefficient splitting for ultra-large operands; currently unused.

---

## Bottom line

* The FFT layer is a tight, iterative radix-2 engine with smart packing to compute real convolutions (and squares) using **one** complex FFT.
* The big-int layer rides on top, switching between schoolbook, Karatsuba, and the FFT engine based on operand size to stay both fast and numerically safe.

