# Integer

High-performance big integer arithmetic library featuring FFT-based multiplication, with a modern CMake build, unit tests, and repeatable benchmarks.

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

Run the benchmark suite and compare against the stored baseline:

```bash
scripts/bench.py --build-dir build
```

To refresh the baseline after confirmed improvements:

```bash
scripts/update_baseline.py --build-dir build
```

## Project layout

- `include/` – single-header `integer.hpp` plus supporting public headers.
- `src/` – implementation files compiled into reusable libraries.
- `apps/` – small command-line utilities (e.g. regression driver).
- `tests/` – Catch2-based unit tests driven by CTest.
- `benchmarks/` – Google Benchmark harness measuring large-digit arithmetic.
- `scripts/` – Python helpers for benchmarking and baseline management.
- `data/` – persisted benchmark metadata (baseline JSON).

See `docs/DEVELOPMENT.md` for detailed workflows and contribution guidelines.

## License

This project is released under the MIT License. See `LICENSE` for details.
