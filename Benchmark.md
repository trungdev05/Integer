# **Benchmark: Multiplication of Large Numbers**

## **Overview**
This document benchmarks the performance of multiplying two large numbers (100,000 digits each) across different programming languages. The results include the time taken in microseconds and the MD5 hash of the result.

---

## **Device**
- **Processor:** AMD Ryzen 7 8840HS
- **Memory:** 16 GB DDR5
- **OS:** Windows 11
- **IDE:** Clion
- **Compiler:** MinGW

## **Benchmark Setup**

### **Input Details**
- **Number of Digits:** 100,000
- **Number Pattern:** `0123456789` repeated.

### **Hash Function**
- **Algorithm:** MD5
- **Purpose:** Verify correctness of the result across languages.

---

## **Benchmark Results**

| Language    | Time (µs) | MD5 Hash                           | Notes                        |
|-------------|-----------|------------------------------------|------------------------------|
| **C#**      | 8055      | `4be25a92edc5284959fcc44dcf4ddcde` | Implemented with BigInteger. |
| **Python**  | 12390     | `4be25a92edc5284959fcc44dcf4ddcde` | Using built-in `int`.        |
| **C++(O3)** | 1585      | `4be25a92edc5284959fcc44dcf4ddcde` | Use my template              |
| **C++**     | 21151     | `4be25a92edc5284959fcc44dcf4ddcde` | Use my template              |
---

## **Code Snippets**

### **C#**
```csharp
using System;
using System.Diagnostics;
using System.Numerics;
using System.Security.Cryptography;
using System.Text;

int digits = 100000; // Number of digits
Console.WriteLine($"Benchmarking multiplication of two {digits}-digit numbers...");

BenchmarkMultiplication(digits);

void BenchmarkMultiplication(int digits)
{
    BigInteger num1 = GenerateLargeInteger(digits);
    BigInteger num2 = GenerateLargeInteger(digits);

    Stopwatch stopwatch = new Stopwatch();
    stopwatch.Start();
    BigInteger result = num1 * num2;
    stopwatch.Stop();

    string resultHash = ComputeMd5Hash(result.ToString());

    Console.WriteLine($"Hash: {resultHash}");
    Console.WriteLine($"Time: {stopwatch.Elapsed.TotalMilliseconds * 1000:F0} microseconds");
}

BigInteger GenerateLargeInteger(int digits)
{
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < digits; i++)
    {
        sb.Append(i % 10);
    }
    return BigInteger.Parse(sb.ToString());
}

string ComputeMd5Hash(string input)
{
    using (MD5 md5 = MD5.Create())
    {
        byte[] inputBytes = Encoding.UTF8.GetBytes(input);
        byte[] hashBytes = md5.ComputeHash(inputBytes);

        StringBuilder sb = new StringBuilder();
        foreach (byte b in hashBytes)
        {
            sb.Append(b.ToString("x2"));
        }
        return sb.ToString();
    }
}
```

### **Python#**
```python
import hashlib
import time
import sys

def generate_large_integer(digits: int) -> int:
    res = ''.join(str(i % 10) for i in range(digits))
    return int(res)

def md5_hash(value: str) -> str:
    return hashlib.md5(value.encode()).hexdigest()

def benchmark_multiplication(digits: int):
    num1 = generate_large_integer(digits)
    num2 = generate_large_integer(digits)
    start = time.time()
    result = num1 * num2
    end = time.time()
    elapsed_microseconds = (end - start) * 1e6
    result_hash = md5_hash(str(result))
    print(f"Hash: {result_hash}")
    print(f"Time: {elapsed_microseconds:.0f} microseconds")

if __name__ == "__main__":
    sys.set_int_max_str_digits(10000000)
    DIGITS = 100000  # Adjust the number of digits as needed
    print(f"Benchmarking multiplication of two {DIGITS}-digit numbers...")
    benchmark_multiplication(DIGITS)
```