#!/usr/bin/env python3
"""Run integer benchmarks and compare against the stored baseline."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List


TIME_FACTORS = {
    "ns": 1e-9,
    "us": 1e-6,
    "ms": 1e-3,
    "s": 1.0,
}


@dataclass
class BenchmarkResult:
    digits: int
    time_seconds: float


@dataclass
class Comparison:
    digits: int
    measured: float
    baseline: float | None
    score: int
    ratio: float | None


def run_process(command: List[str], cwd: Path | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(command, cwd=cwd, check=True, capture_output=True, text=True)


def load_baseline(path: Path) -> Dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def parse_benchmark_payload(payload: Dict) -> List[BenchmarkResult]:
    results: List[BenchmarkResult] = []
    for entry in payload.get("benchmarks", []):
        name = entry.get("name", "")
        if not name.startswith("BM_IntegerMultiply/"):
            continue
        try:
            digits = int(name.split("/")[1])
        except (ValueError, IndexError):
            continue
        time_unit = entry.get("time_unit", "ns")
        factor = TIME_FACTORS.get(time_unit, 1.0)
        time_seconds = entry.get("real_time", 0.0) * factor
        results.append(BenchmarkResult(digits=digits, time_seconds=time_seconds))
    return results


def compute_scores(results: List[BenchmarkResult], baseline: Dict) -> List[Comparison]:
    scores: List[Comparison] = []
    baseline_times: Dict[str, float] = baseline.get("baseline_times", {})
    scoring_system = baseline.get("scoring_system", {})
    baseline_score = scoring_system.get("baseline_score", 200)
    max_score = scoring_system.get("max_score", 1000)

    for result in results:
        base_time = baseline_times.get(str(result.digits))
        ratio = None
        score = 0
        if base_time:
            ratio = base_time / result.time_seconds if result.time_seconds else float("inf")
            score = min(max_score, int(baseline_score * ratio))
        else:
            score = min(max_score, 200)
        scores.append(Comparison(
            digits=result.digits,
            measured=result.time_seconds,
            baseline=base_time,
            score=score,
            ratio=ratio,
        ))
    return scores


def write_report(comparisons: List[Comparison], baseline_meta: Dict, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8") as handle:
        handle.write("INTEGER BENCHMARK REPORT\n")
        handle.write("========================\n")
        handle.write(f"Generated: {datetime.now():%Y-%m-%d %H:%M:%S}\n")
        handle.write("\n")
        if baseline_meta:
            info = baseline_meta.get("baseline_info", {})
            handle.write("Baseline Reference\n")
            handle.write(f"  Timestamp: {info.get('timestamp', 'n/a')}\n")
            handle.write(f"  Seed:      {info.get('seed', 'n/a')}\n")
            handle.write("\n")
        handle.write(f"{'Digits':>12} {'Measured(s)':>15} {'Baseline(s)':>15} {'vs Base':>9} {'Score':>7}\n")
        handle.write("-" * 64 + "\n")
        total = 0
        for comparison in comparisons:
            ratio_display = f"{comparison.ratio:>7.2f}x" if comparison.ratio is not None else "   n/a"
            baseline_display = f"{comparison.baseline:>15.6f}" if comparison.baseline else f"{0:>15.6f}"
            handle.write(
                f"{comparison.digits:12d} {comparison.measured:15.6f} {baseline_display} {ratio_display:>9} {comparison.score:7d}\n"
            )
            total += comparison.score
        avg = total / len(comparisons) if comparisons else 0
        handle.write("-" * 64 + "\n")
        handle.write(f"Total Score:   {total}\n")
        handle.write(f"Average Score: {avg:.1f}\n")


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="Run integer benchmark suite")
    parser.add_argument("--build-dir", type=Path, default=Path("build"), help="CMake build directory")
    parser.add_argument("--preset", help="Optional CMake preset to configure before building")
    parser.add_argument("--binary", type=Path, help="Path to integer_benchmark executable")
    parser.add_argument("--baseline", type=Path, default=Path("data/baseline.json"))
    parser.add_argument("--results-dir", type=Path, default=Path("results"))
    parser.add_argument("--skip-build", action="store_true", help="Skip the cmake build step")
    args = parser.parse_args(argv)

    build_dir = args.build_dir
    if not args.skip_build:
        if args.preset:
            run_process(["cmake", "--preset", args.preset])
        run_process(["cmake", "--build", str(build_dir), "--target", "integer_benchmark"])

    if args.binary:
        benchmark_binary = args.binary
    else:
        benchmark_binary = build_dir / "benchmarks" / "integer_benchmark"
    if sys.platform == "win32":
        benchmark_binary = benchmark_binary.with_suffix(".exe")

    if not benchmark_binary.exists():
        raise FileNotFoundError(f"Benchmark executable not found: {benchmark_binary}")

    completed = run_process([str(benchmark_binary), "--benchmark_format=json"])
    payload = json.loads(completed.stdout)
    results = parse_benchmark_payload(payload)

    if not results:
        print("No benchmark results found in payload", file=sys.stderr)
        return 1

    baseline = load_baseline(args.baseline) if args.baseline.exists() else {}
    comparisons = compute_scores(results, baseline)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = args.results_dir / f"benchmark_{timestamp}.txt"
    write_report(comparisons, baseline, report_path)
    print(f"Report written to {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
