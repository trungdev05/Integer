#include <gmpxx.h>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>

// Helper to generate a random number string of N digits
std::string generate_random_digits(int n) {
    std::string s;
    s.reserve(n);
    if (n > 0) {
        // First digit 1-9
        s.push_back('1' + (rand() % 9));
        for (int i = 1; i < n; ++i) {
            s.push_back('0' + (rand() % 10));
        }
    } else {
        s = "0";
    }
    return s;
}

int main(int argc, char* argv[]) {
    int digits = 100000;
    if (argc > 1) {
        digits = std::stoi(argv[1]);
    }

    srand(time(NULL));

    std::cout << "Benchmarking GMP multiplication with " << digits << " digits..." << std::endl;

    // Generate two large numbers
    std::string s1 = generate_random_digits(digits);
    std::string s2 = generate_random_digits(digits);

    mpz_class a(s1);
    mpz_class b(s2);
    mpz_class c;

    // Warmup
    c = a * b;

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        c = a * b;
        // Make sure it's not optimized away
        if (c < 0) std::cerr << "Impossible" << std::endl; 
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double average_ms = (duration.count() / 10.0) / 1000.0;

    std::cout << "GMP Average time: " << average_ms << " ms" << std::endl;

    return 0;
}
