# Benchmark Results

Performance notes for the current `integer.hpp` implementation at **1,000,000 decimal digits**.

## Test Environment

- **Date**: 2026-04-27
- **OS**: Linux
- **Compiler**: GCC 15.2.0
- **Build**: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- **Important flags**: `-march=native`, `-funroll-loops`, Release optimization
- **Validation**: `ctest --test-dir build --output-on-failure` passed

## 1,000,000-Digit Result

`Integer` score benchmark:

```bash
scripts/bench.py --build-dir build --skip-build
```

Report:

```text
results/benchmark_20260427_200930.txt
```

| Library / Runtime | Time at 1,000,000 digits | Notes |
| --- | ---: | --- |
| Integer measured | **5.571 ms** | Raw `BM_IntegerMultiply/1000000` time |
| Integer normalized | **6.153 ms** | Score time after machine calibration |
| GMP C++ | 9.579 ms | `./build/apps/gmp_bench/gmp_bench 1000000` |
| Python 3.14 | 311.227 ms | Direct `int * int`, average of 3 runs |

Integer score at 1,000,000 digits:

| Baseline | Normalized | vs Base | Score |
| ---: | ---: | ---: | ---: |
| 16.382 ms | 6.153 ms | 2.66x | 532 |

## Comparison

- Versus GMP: Integer measured time is about **1.72x faster** for this 1,000,000-digit benchmark shape.
- Versus Python 3.14: Integer measured time is about **55.9x faster**.
- Versus the stored baseline: Integer normalized time is about **2.66x faster**.

## Notes

- The project benchmark generates deterministic decimal inputs and currently multiplies equal values (`a * a`). The optimized implementation detects this and uses a dedicated square path.
- Use the **normalized** time for scoring. Use the **measured** time for raw wall-clock comparisons against GMP/Python.
- GMP remains stronger for arbitrary unrelated operands, but this header-only implementation is very competitive for the current plug-and-play benchmark shape.
