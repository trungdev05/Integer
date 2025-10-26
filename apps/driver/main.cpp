#include "integer.hpp"
#include "md5.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

integer generate_large_integer(const int digits) {
    std::string res;
    res.reserve(digits);
    for (int i = 0; i < digits; i++) {
        constexpr char first_number_char = '0';
        const int number = i % 10;
        const char c = static_cast<char>(first_number_char + number);
        res += c;
    }
    return integer(res);
}

std::vector<std::pair<int, std::string> > regression_tests = {
    {100000, "4be25a92edc5284959fcc44dcf4ddcde"},
    {1000, "2c5fbee9a0152dca11d49124c6c6a4a3"}
};

void run_regressions() {
    for (const auto &[digits, expected_hash] : regression_tests) {
        const integer num1 = generate_large_integer(digits);
        const integer num2 = generate_large_integer(digits);
        const integer result = num1 * num2;
        const std::string hash = md5_hash(result.to_string());
        assert(hash == expected_hash);
    }
    std::cerr << "All regression hashes match." << std::endl;
}

void benchmark_multiplication(const int digits) {
    std::uint64_t total = 0;
    for (int i = 0; i < 5; ++i) {
        const integer num1 = generate_large_integer(digits);
        const integer num2 = generate_large_integer(digits);
        const auto start = std::chrono::high_resolution_clock::now();
        const integer result = num1 * num2;
        (void)result;
        const auto end = std::chrono::high_resolution_clock::now();
        const auto elapsed = end - start;
        total += std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    }
    total /= 5;
    std::cout << "Average time: " << total << " microseconds" << std::endl;
}

}  // namespace

int main() {
    run_regressions();
    constexpr int digits = 100000;
    std::cout << "Benchmarking multiplication of two " << digits << "-digit numbers..." << std::endl;
    benchmark_multiplication(digits);
    return 0;
}
