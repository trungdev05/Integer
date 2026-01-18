#!/usr/bin/env python3
import argparse
import subprocess
import sys
import random
from pathlib import Path

# Increase integer string conversion limit for large number tests
sys.set_int_max_str_digits(0)

def run_process(command, cwd=None, input_str=None):
    try:
        return subprocess.run(command, cwd=cwd, input=input_str, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {e.cmd}")
        print(f"Stdout: {e.stdout}")
        print(f"Stderr: {e.stderr}")
        raise

def generate_large_int(digits):
    if digits <= 1:
        return random.randint(0, 9)
    start = 10**(digits-1)
    end = (10**digits) - 1
    return random.randint(start, end)

def find_executable(build_dir, name):
    # Try common locations
    candidates = [
        build_dir / "apps" / "calculator" / name,
        build_dir / "apps" / "calculator" / f"{name}.exe",
        build_dir / name,
        build_dir / f"{name}.exe"
    ]
    for c in candidates:
        if c.exists() and c.is_file():
            return c.absolute()
    return None

def test_operation(calculator_bin, op, a, b):
    # Calculate in Python
    if op == "+":
        expected = a + b
    elif op == "-":
        expected = a - b
    elif op == "*":
        expected = a * b
    elif op == "/":
        expected = a // b
    else:
        return False
    
    input_str = f"{op} {a} {b}\n"
    res = run_process([str(calculator_bin)], input_str=input_str)
    output = res.stdout.strip()
    
    if str(expected) != output:
        print(f"\nFAILED TEST:")
        print(f"Operation: {a} {op} {b}")
        print(f"Expected:  {expected}")
        print(f"Got:       {output}")
        return False
    return True

def main():
    parser = argparse.ArgumentParser(description="Verify integer arithmetic correctness against Python")
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--num-tests", type=int, default=50)
    parser.add_argument("--digits", type=int, default=100000, help="Approximate number of digits for test numbers")
    args = parser.parse_args()
    
    if not args.skip_build:
        print(f"Building calculator in {args.build_dir}...")
        # Ensure build dir exists (configure if needed)
        if not args.build_dir.exists():
             run_process(["cmake", "-S", ".", "-B", str(args.build_dir), "-DCMAKE_BUILD_TYPE=Release"])
        
        run_process(["cmake", "--build", str(args.build_dir), "--target", "calculator"])

    calculator_bin = find_executable(args.build_dir, "calculator")
    if not calculator_bin:
        print("Could not find 'calculator' executable. Please build it first.")
        sys.exit(1)
        
    print(f"Using executable: {calculator_bin}")
    import time

    print(f"Running {args.num_tests} random tests per operation with approx {args.digits} digits...")
    
    start_time = time.perf_counter()
    ops = ["+", "-", "*", "/"]
    
    for i in range(args.num_tests):
        # Generate random digits count close to the requested size (variance +- 10%)
        d_min = int(args.digits * 0.9)
        d_max = int(args.digits * 1.1)
        if d_min < 1: d_min = 1
        
        digits_a = random.randint(d_min, d_max)
        digits_b = random.randint(d_min, d_max)

        a = generate_large_int(digits_a)
        b = generate_large_int(digits_b)
        
        # Test Addition
        if not test_operation(calculator_bin, "+", a, b): return 1
        
        # Test Subtraction 
        # We ensure a >= b just in case the library does not support negative numbers yet
        if a < b: a, b = b, a
        if not test_operation(calculator_bin, "-", a, b): return 1
        
        # Test Multiplication
        if not test_operation(calculator_bin, "*", a, b): return 1
        
        # Test Division
        if b == 0: b = 1
        if not test_operation(calculator_bin, "/", a, b): return 1
        
        if (i+1) % 10 == 0:
            print(f"Completed {i+1}/{args.num_tests} tests...")

    end_time = time.perf_counter()
    total_time = end_time - start_time
    print(f"\nAll tests passed successfully in {total_time:.2f} seconds!")
    return 0

if __name__ == "__main__":
    main()
