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
