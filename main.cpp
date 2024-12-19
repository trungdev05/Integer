#include <iostream>

#include "integer.hpp"

void deterministic_multiply() {
    constexpr int DIGITS = 50000; // Bạn có thể tăng lên 500000 khi đủ bộ nhớ

    auto generate_number = [&](const int start_digit) -> string {
        string num(DIGITS, '0');
        num[0] = '1' + (start_digit % 9);
        for (int i = 1; i < DIGITS; i++) {
            num[i] = '0' + ((start_digit + i) % 10);
        }
        return num;
    };

    const string num1 = generate_number(1);
    const string num2 = generate_number(2);

    const integer a = num1;
    const integer b = num2;

    const auto start = chrono::high_resolution_clock::now();
    const integer result = a * b;
    const auto end = chrono::high_resolution_clock::now();

    const chrono::duration<double> elapsed = end - start;

    const uint64_t hash_result = result.compute_hash();

    cout << "Thoi gian nhan 2 so co  " << DIGITS << " chu so: "
         << elapsed.count() << " giay\n";
    cout << "Hash cua ket qua nhan: " << hash_result << "\n";
}

int main() {
    deterministic_multiply();
}