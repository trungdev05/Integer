#!/usr/bin/env python3
"""
Test-suite + benchmark for C++ `integer` class (FFT/Karatsuba big-int).

• Generates two 5 000-digit numbers → checks *, /, +, – against Python `int`
• Benchmarks multiplication of two 100 000-digit numbers (different seed)
  and prints speed ratio (integer / Python).

Only this file is needed; it writes a tiny C++ driver at run-time, compiles it
(`g++ -std=c++20 -O3`), then streams operands through stdin.
Put `integer.h` (the code you pasted) in the same directory.
"""
import os, random, subprocess, tempfile, time, textwrap, argparse, sys
from datetime import datetime
import json

# Increase the limit for integer string conversion to handle large numbers
sys.set_int_max_str_digits(2000000)

TMP = tempfile.TemporaryDirectory()

DRIVER_SRC = textwrap.dedent("""
    #include <bits/stdc++.h>
    #include "integer.hpp"
    using namespace std;
    int main() {
        ios::sync_with_stdio(false); cin.tie(nullptr);
        string op, A, B; if(!(cin>>op>>A>>B)) return 0;
        integer a(A), b(B);
        if(op=="mul") cout<<a*b;
        else if(op=="div") cout<<a/b;
        else if(op=="add") cout<<a+b;
        else if(op=="sub") cout<<a-b;
        cout << '\\n';
        return 0;
    }
""")

def build_driver():
    src = os.path.join(TMP.name, "calc.cpp")
    header = os.path.join(TMP.name, "integer.hpp")

    # Copy integer.hpp to temporary directory
    import shutil
    shutil.copy("integer.hpp", header)

    with open(src, "w") as f: f.write(DRIVER_SRC)
    exe = os.path.join(TMP.name, "calc")
    subprocess.check_call(
        ["g++", "-std=c++20", "-O3", "-march=native", src, "-o", exe]
    )
    return exe

CALC = build_driver()

def cpp_op(op: str, a: str, b: str) -> str:
    proc = subprocess.run(
        [CALC],
        input=f"{op}\n{a}\n{b}\n".encode(),
        stdout=subprocess.PIPE,
        check=True,
    )
    return proc.stdout.strip().decode()

def rand_digits(digits: int, rng: random.Random) -> str:
    s = "".join(str(rng.randint(0, 9)) for _ in range(digits)).lstrip("0")
    return s or "0"

def check_exact(rng_seed: int = 0xBEEF):
    rng = random.Random(rng_seed)
    a = rand_digits(5_000, rng)
    b = rand_digits(5_000, rng)
    if int(a) < int(b): a, b = b, a  # ensure non-negative for subtraction/div
    for op in ("mul", "div", "add", "sub"):
        py = {
            "mul": int(a) * int(b),
            "div": int(a) // int(b),
            "add": int(a) + int(b),
            "sub": int(a) - int(b),
        }[op]
        cpp = cpp_op(op, a, b)
        assert str(py) == cpp, f"{op} mismatch"
    print("✔ Chính xác: *, /, +, - với 5 000 chữ số")

def bench_mult(seed: int = 1234, digits: int = 100_000, baseline_data=None):
    rng = random.Random(seed)
    a = rand_digits(digits, rng)
    b = rand_digits(digits, rng)

    # High precision timing for Python
    t0 = time.perf_counter()
    _ = int(a) * int(b)
    py_time = time.perf_counter() - t0

    # High precision timing for C++
    t0 = time.perf_counter()
    _ = cpp_op("mul", a, b)
    cpp_time = time.perf_counter() - t0

    speedup = py_time / cpp_time if cpp_time > 0 else float('inf')
    
    # Calculate score based on baseline
    if baseline_data and str(digits) in baseline_data['baseline_times']:
        baseline_time = baseline_data['baseline_times'][str(digits)]
        baseline_score = baseline_data['scoring_system']['baseline_score']
        max_score = baseline_data['scoring_system']['max_score']
        
        # Score = baseline_score * (baseline_time / current_time)
        # Better performance (lower time) = higher score
        score = min(max_score, int(baseline_score * (baseline_time / cpp_time)))
        score_type = "baseline"
    else:
        # Fallback to old scoring system
        score = min(200, int(speedup * 20))
        score_type = "speedup"
    
    return {
        'digits': digits,
        'py_time': py_time,
        'cpp_time': cpp_time,
        'speedup': speedup,
        'score': score,
        'score_type': score_type,
        'baseline_time': baseline_data['baseline_times'][str(digits)] if baseline_data and str(digits) in baseline_data['baseline_times'] else None
    }

def save_results_to_file(results, seed, baseline_data=None, filename=None):
    """Save benchmark results to a timestamped file"""
    if filename is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"results/benchmark_{timestamp}_seed{seed}.txt"
    
    os.makedirs("results", exist_ok=True)
    
    with open(filename, 'w') as f:
        f.write(f"INTEGER MULTIPLICATION BENCHMARK RESULTS\n")
        f.write(f"=========================================\n")
        f.write(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Seed: {seed}\n")
        f.write(f"System: Linux x86_64\n")
        f.write(f"Compiler: g++ -std=c++20 -O3 -march=native\n")
        
        if baseline_data:
            f.write(f"Baseline: {baseline_data['baseline_info']['timestamp']} (seed {baseline_data['baseline_info']['seed']})\n")
            f.write(f"Scoring: Baseline-based (200 = baseline, max 1000)\n\n")
            
            f.write(f"{'Digits':<12} {'Python(s)':<15} {'C++(s)':<15} {'Baseline(s)':<15} {'vs Base':<10} {'Score':<10}\n")
            f.write("-" * 95 + "\n")
        else:
            f.write(f"Scoring: Speedup-based (fallback)\n\n")
            f.write(f"{'Digits':<12} {'Python(s)':<15} {'C++(s)':<15} {'Speedup':<12} {'Score':<10}\n")
            f.write("-" * 85 + "\n")
        
        total_score = 0
        for r in results:
            if baseline_data and r['baseline_time']:
                vs_base = r['baseline_time'] / r['cpp_time']
                f.write(f"{r['digits']:<12,} {r['py_time']:<15.6f} {r['cpp_time']:<15.6f} "
                       f"{r['baseline_time']:<15.6f} {vs_base:<10.3f}x {r['score']:<10}/1000\n")
            else:
                f.write(f"{r['digits']:<12,} {r['py_time']:<15.6f} {r['cpp_time']:<15.6f} "
                       f"{r['speedup']:<12.3f}x {r['score']:<10}/1000\n")
            total_score += r['score']
        
        avg_score = total_score / len(results)
        
        f.write("-" * (95 if baseline_data else 85) + "\n")
        f.write(f"Total Score:   {total_score}/7000 điểm\n")
        f.write(f"Average Score: {avg_score:.1f}/1000 điểm\n")
        
        if avg_score >= 800:
            assessment = "XUẤT SẮC - Thuật toán rất tối ưu!"
        elif avg_score >= 600:
            assessment = "TỐT - Hiệu suất khá ổn"
        elif avg_score >= 400:
            assessment = "TRUNG BÌNH - Cần cải thiện"
        elif avg_score >= 200:
            assessment = "BASELINE - Hiệu suất chuẩn"
        else:
            assessment = "CẦN NÂNG CẤP - Hiệu suất thấp"
        
        f.write(f"Assessment:    {assessment}\n")
        
        # Additional analysis
        f.write(f"\nDETAILED ANALYSIS:\n")
        f.write(f"==================\n")
        best_speedup = max(r['speedup'] for r in results)
        worst_speedup = min(r['speedup'] for r in results)
        f.write(f"Best speedup:  {best_speedup:.3f}x\n")
        f.write(f"Worst speedup: {worst_speedup:.3f}x\n")
        
        if baseline_data:
            improvements = []
            regressions = []
            for r in results:
                if r['baseline_time']:
                    vs_base = r['baseline_time'] / r['cpp_time']
                    if vs_base > 1.0:
                        improvements.append((r['digits'], vs_base))
                    elif vs_base < 1.0:
                        regressions.append((r['digits'], vs_base))
            
            if improvements:
                f.write(f"Improvements over baseline:\n")
                for digits, ratio in improvements:
                    f.write(f"  {digits:,} digits: {ratio:.3f}x faster\n")
            
            if regressions:
                f.write(f"Regressions from baseline:\n")
                for digits, ratio in regressions:
                    f.write(f"  {digits:,} digits: {1/ratio:.3f}x slower\n")
        
        # Find crossover point where C++ becomes faster
        crossover = None
        for r in results:
            if r['speedup'] >= 1.0:
                crossover = r['digits']
                break
        
        if crossover:
            f.write(f"C++ faster than Python from: {crossover:,} digits\n")
        else:
            f.write(f"C++ never faster than Python in this test range\n")
    
    return filename

def comprehensive_benchmark(seed: int = 1024):
    print(f"🚀 Comprehensive Benchmark (Seed: {seed})")
    print("=" * 100)
    
    # Load baseline data
    baseline_data = load_baseline()
    if baseline_data:
        print(f"📊 Using baseline from {baseline_data['baseline_info']['timestamp']} (seed {baseline_data['baseline_info']['seed']})")
        print(f"🎯 Baseline = 200 points, Max = 1000 points")
    else:
        print(f"📊 Using fallback speedup-based scoring")
    print("")
    
    sizes = [10_000, 20_000, 50_000, 100_000, 200_000, 500_000, 1_000_000]
    results = []
    
    for digits in sizes:
        print(f"\n📊 Testing {digits:,} digits...")
        result = bench_mult(seed ^ (digits // 1000), digits, baseline_data)
        results.append(result)
        
        print(f"    Python:   {result['py_time']:.6f}s")
        print(f"    C++:      {result['cpp_time']:.6f}s")
        if result['baseline_time']:
            print(f"    Baseline: {result['baseline_time']:.6f}s")
            performance = "📈" if result['cpp_time'] < result['baseline_time'] else "📉"
            print(f"    vs Base:  {performance} {result['baseline_time']/result['cpp_time']:.3f}x")
        print(f"    Speedup:  {result['speedup']:.3f}x")
        print(f"    Score:    {result['score']}/1000 điểm ({result['score_type']})")
    
    print("\n" + "=" * 100)
    print("📈 SUMMARY RESULTS")
    print("=" * 100)
    if baseline_data:
        print(f"{'Digits':<12} {'Python(s)':<15} {'C++(s)':<15} {'Baseline(s)':<15} {'vs Base':<10} {'Score':<10}")
        print("-" * 100)
    else:
        print(f"{'Digits':<12} {'Python(s)':<15} {'C++(s)':<15} {'Speedup':<12} {'Score':<10}")
        print("-" * 100)
    
    total_score = 0
    for r in results:
        if baseline_data and r['baseline_time']:
            vs_base = r['baseline_time'] / r['cpp_time']
            print(f"{r['digits']:<12,} {r['py_time']:<15.6f} {r['cpp_time']:<15.6f} "
                  f"{r['baseline_time']:<15.6f} {vs_base:<10.3f}x {r['score']:<10}/1000")
        else:
            print(f"{r['digits']:<12,} {r['py_time']:<15.6f} {r['cpp_time']:<15.6f} "
                  f"{r['speedup']:<12.3f}x {r['score']:<10}/1000")
        total_score += r['score']
    
    avg_score = total_score / len(results)
    
    print("-" * 100)
    print(f"Total Score:   {total_score}/7000 điểm")
    print(f"Average Score: {avg_score:.1f}/1000 điểm")
    
    # Performance assessment for new 1000-point scale
    if avg_score >= 800:
        assessment = "🌟 XUẤT SẮC - Thuật toán rất tối ưu!"
    elif avg_score >= 600:
        assessment = "⭐ TỐT - Hiệu suất khá ổn"
    elif avg_score >= 400:
        assessment = "⚡ TRUNG BÌNH - Cần cải thiện"
    elif avg_score >= 200:
        assessment = "📊 BASELINE - Hiệu suất chuẩn"
    else:
        assessment = "🐌 CẦN NÂNG CẤP - Hiệu suất thấp"
    
    print(f"Assessment:    {assessment}")
    print("=" * 100)
    
    # Save results to file
    filename = save_results_to_file(results, seed, baseline_data)
    print(f"\n💾 Results saved to: {filename}")
    
    return results, filename

def load_baseline():
    """Load baseline performance data from JSON file"""
    try:
        with open('baseline.json', 'r') as f:
            baseline_data = json.load(f)
        return baseline_data
    except FileNotFoundError:
        print("⚠️  baseline.json not found, using default scoring")
        return None
    except json.JSONDecodeError:
        print("⚠️  Invalid baseline.json format, using default scoring") 
        return None

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=1024,
                        help="seed cho RNG (mặc định 1024)")
    parser.add_argument("--quick", action="store_true",
                        help="chỉ test nhanh với 5k digits")
    args = parser.parse_args()
    
    seed = args.seed
    print(f"Seed: {seed}")
    
    if args.quick:
        check_exact(seed)
        baseline_data = load_baseline()
        result = bench_mult(seed ^ 0xDEADBEEF, 5_000, baseline_data)
        print(f"✔ Chính xác: *, /, +, - với 5 000 chữ số")
        print(f"⏱ 5 000-digit multiply:")
        print(f"    Python: {result['py_time']:.6f}s  |  C++: {result['cpp_time']:.6f}s")
        print(f"    Speedup: {result['speedup']:.3f}x  |  Score: {result['score']}/1000 ({result['score_type']})")
        
        # Save quick test result
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"results/quick_test_{timestamp}_seed{seed}.txt"
        save_results_to_file([result], seed, baseline_data, filename)
        print(f"\n💾 Quick test result saved to: {filename}")
    else:
        check_exact(seed)
        results, filename = comprehensive_benchmark(seed)

if __name__ == "__main__":
    try:
        main()
    except AssertionError as e:
        print("❌", e)
        sys.exit(1)
