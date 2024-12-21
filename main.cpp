#include "md5.cpp"
#include "integer.hpp"
#include "simd.cpp"

integer generate_large_integer(const int DIGITS) {
    string res;
    for (int i = 0; i < DIGITS; i++) {
        constexpr char FIRST_NUMBER_CHAR = '0';
        const int number = i % 10;
        const char c = FIRST_NUMBER_CHAR + static_cast<char>(number);
        res += c;
    }
    return {res};
}

vector<pair<int, string> > tests = {
    {100000, "4be25a92edc5284959fcc44dcf4ddcde"},
    {1000, "2c5fbee9a0152dca11d49124c6c6a4a3"}
};

void run_tests() {
    for (const auto &[fst, snd]: tests) {
        const integer num1 = generate_large_integer(fst);
        const integer num2 = generate_large_integer(fst);
        const integer result = num1 * num2;
        const string hash = md5_hash(result.to_string());
        assert(hash == snd);
    }
    cerr << "All tests passed!" << endl;
}

void benchmark_multiplication(const int DIGITS) {
    uint64_t avg = 0;
    for (int i = 0; i < 5; ++i) {
        const integer num1 = generate_large_integer(DIGITS);
        const integer num2 = generate_large_integer(DIGITS);
        const auto start = chrono::high_resolution_clock::now();
        const integer result = num1 * num2; // Nhân hai số
        const auto end = chrono::high_resolution_clock::now();
        const chrono::duration<double> elapsed = end - start;
        avg += chrono::duration_cast<chrono::microseconds>(elapsed).count();
    }
    avg /= 5;
    cout << "Average time: " << avg << " microseconds" << endl;
}

int main() {
    run_tests();
    constexpr int DIGITS = 100000;
    cout << "Benchmarking multiplication of two " << DIGITS << "-digit numbers..." << endl;
    benchmark_multiplication(DIGITS);

    return 0;
}
