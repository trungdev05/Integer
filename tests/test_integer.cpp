#include <catch2/catch_test_macros.hpp>

#include "integer.hpp"
#include "md5.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace decimal {

std::string strip_leading_zeros(std::string value) {
    const auto pos = value.find_first_not_of('0');
    if (pos == std::string::npos) {
        return "0";
    }
    return value.substr(pos);
}

int compare(const std::string &lhs, const std::string &rhs) {
    const std::string a = strip_leading_zeros(lhs);
    const std::string b = strip_leading_zeros(rhs);
    if (a.size() != b.size()) {
        return a.size() < b.size() ? -1 : 1;
    }
    if (a == b) {
        return 0;
    }
    return a < b ? -1 : 1;
}

std::string add(const std::string &lhs, const std::string &rhs) {
    const int n = static_cast<int>(lhs.size());
    const int m = static_cast<int>(rhs.size());
    const int size = std::max(n, m);
    std::string result;
    result.reserve(size + 1);
    int carry = 0;
    for (int i = 0; i < size; ++i) {
        const int a = i < n ? lhs[n - 1 - i] - '0' : 0;
        const int b = i < m ? rhs[m - 1 - i] - '0' : 0;
        int sum = a + b + carry;
        carry = sum / 10;
        sum %= 10;
        result.push_back(static_cast<char>('0' + sum));
    }
    if (carry != 0) {
        result.push_back(static_cast<char>('0' + carry));
    }
    std::reverse(result.begin(), result.end());
    return strip_leading_zeros(result);
}

std::string subtract(const std::string &lhs, const std::string &rhs) {
    // assumes lhs >= rhs
    const int n = static_cast<int>(lhs.size());
    const int m = static_cast<int>(rhs.size());
    std::string result;
    result.reserve(n);
    int borrow = 0;
    for (int i = 0; i < n; ++i) {
        int a = lhs[n - 1 - i] - '0' - borrow;
        const int b = i < m ? rhs[m - 1 - i] - '0' : 0;
        if (a < b) {
            a += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        const int diff = a - b;
        result.push_back(static_cast<char>('0' + diff));
    }
    while (result.size() > 1 && result.back() == '0') {
        result.pop_back();
    }
    std::reverse(result.begin(), result.end());
    return strip_leading_zeros(result);
}

std::string multiply(const std::string &lhs, const std::string &rhs) {
    if (lhs == "0" || rhs == "0") {
        return "0";
    }
    const int n = static_cast<int>(lhs.size());
    const int m = static_cast<int>(rhs.size());
    std::vector<int> result(n + m, 0);
    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            const int mul = (lhs[i] - '0') * (rhs[j] - '0');
            const int idx = i + j + 1;
            const int sum = mul + result[idx];
            result[idx] = sum % 10;
            result[idx - 1] += sum / 10;
        }
    }
    std::string str;
    str.reserve(result.size());
    bool leading = true;
    for (int digit : result) {
        if (leading && digit == 0) {
            continue;
        }
        leading = false;
        str.push_back(static_cast<char>('0' + digit));
    }
    return str.empty() ? std::string("0") : str;
}

std::pair<std::string, std::string> div_mod(const std::string &lhs, const std::string &rhs) {
    if (rhs == "0") {
        throw std::runtime_error("division by zero");
    }
    std::string remainder;
    remainder.reserve(lhs.size());
    std::string quotient;
    quotient.reserve(lhs.size());

    for (char c : lhs) {
        remainder.push_back(c);
        remainder = strip_leading_zeros(remainder);
        int count = 0;
        while (compare(remainder, rhs) >= 0) {
            remainder = subtract(remainder, rhs);
            ++count;
        }
        quotient.push_back(static_cast<char>('0' + count));
    }

    return {strip_leading_zeros(quotient), strip_leading_zeros(remainder)};
}

std::string multiply_scalar(const std::string &lhs, std::uint64_t scalar) {
    if (lhs == "0" || scalar == 0) {
        return "0";
    }
    std::string result;
    result.reserve(lhs.size() + 21);
    std::uint64_t carry = 0;
    for (auto it = lhs.rbegin(); it != lhs.rend(); ++it) {
        const std::uint64_t digit = static_cast<std::uint64_t>(*it - '0');
        const std::uint64_t prod = digit * scalar + carry;
        result.push_back(static_cast<char>('0' + prod % 10));
        carry = prod / 10;
    }
    while (carry > 0) {
        result.push_back(static_cast<char>('0' + carry % 10));
        carry /= 10;
    }
    std::reverse(result.begin(), result.end());
    return strip_leading_zeros(result);
}

std::pair<std::string, std::uint64_t> div_mod_scalar(const std::string &lhs, std::uint64_t scalar) {
    if (scalar == 0) {
        throw std::runtime_error("division by zero");
    }
    std::string quotient;
    quotient.reserve(lhs.size());
    std::uint64_t remainder = 0;
    for (char c : lhs) {
        remainder = remainder * 10 + static_cast<std::uint64_t>(c - '0');
        const std::uint64_t digit = remainder / scalar;
        quotient.push_back(static_cast<char>('0' + digit));
        remainder %= scalar;
    }
    return {strip_leading_zeros(quotient), remainder};
}

std::string append_zeros(const std::string &value, std::size_t zeros) {
    if (value == "0") {
        return "0";
    }
    return value + std::string(zeros, '0');
}

std::string random_digits(std::mt19937_64 &rng, std::size_t digits) {
    std::uniform_int_distribution<int> dist(0, 9);
    std::string out;
    out.reserve(digits);
    for (std::size_t i = 0; i < digits; ++i) {
        out.push_back(static_cast<char>('0' + dist(rng)));
    }
    return strip_leading_zeros(out);
}

}  // namespace decimal

TEST_CASE("md5 hash matches expected vectors", "[md5]") {
    REQUIRE(md5_hash("hello") == "5d41402abc4b2a76b9719d911017c592");
    REQUIRE(md5_hash("world") == "7d793037a0760186574b0282f2f435e7");
}

TEST_CASE("integer construction and conversion", "[integer][construction]") {
    const integer zero;
    CHECK(zero.to_string() == "0");

    const integer from_uint64(1234567890123456789ULL);
    CHECK(from_uint64.to_string() == "1234567890123456789");
    CHECK(static_cast<std::uint64_t>(from_uint64) == 1234567890123456789ULL);

    const integer from_string("98765432109876543210");
    CHECK(from_string.to_string() == "98765432109876543210");

    std::ostringstream oss;
    oss << from_string;
    CHECK(oss.str() == from_string.to_string());
}

TEST_CASE("integer comparison operators", "[integer][compare]") {
    std::mt19937_64 rng(2024u);
    const std::size_t trials = 8;

    for (std::size_t i = 0; i < trials; ++i) {
        const std::string a_digits = decimal::random_digits(rng, 80);
        const std::string b_digits = decimal::random_digits(rng, 80);

        const integer a_int(a_digits);
        const integer b_int(b_digits);
        const int cmp = decimal::compare(a_digits, b_digits);

        CHECK((a_int < b_int) == (cmp < 0));
        CHECK((a_int > b_int) == (cmp > 0));
        CHECK((a_int == b_int) == (cmp == 0));
        CHECK((a_int != b_int) == (cmp != 0));
        CHECK((a_int <= b_int) == (cmp <= 0));
        CHECK((a_int >= b_int) == (cmp >= 0));
    }
}

TEST_CASE("integer arithmetic matches reference decimal implementation", "[integer][arithmetic]") {
    std::mt19937_64 rng(1337u);
    const std::size_t trials = 3;

    for (std::size_t i = 0; i < trials; ++i) {
        const std::string a_digits = decimal::random_digits(rng, 150);
        std::string b_digits = decimal::random_digits(rng, 120);
        if (b_digits == "0") {
            b_digits = "1";
        }

        const integer a_big(a_digits);
        const integer b_big(b_digits);

        // Addition
        const std::string sum_ref = decimal::add(a_digits, b_digits);
        CHECK((a_big + b_big).to_string() == sum_ref);
        integer sum_assign = a_big;
        sum_assign += b_big;
        CHECK(sum_assign.to_string() == sum_ref);

        // Multiplication
        const std::string mul_ref = decimal::multiply(a_digits, b_digits);
        CHECK((a_big * b_big).to_string() == mul_ref);
        integer mul_assign = a_big;
        mul_assign *= b_big;
        CHECK(mul_assign.to_string() == mul_ref);

        // Ensure lhs >= rhs for subtraction/division/modulo tests
        std::string lhs_digits = a_digits;
        std::string rhs_digits = b_digits;
        if (decimal::compare(lhs_digits, rhs_digits) < 0) {
            std::swap(lhs_digits, rhs_digits);
        }
        const integer lhs_int(lhs_digits);
        const integer rhs_int(rhs_digits);

        const std::string diff_ref = decimal::subtract(lhs_digits, rhs_digits);
        CHECK((lhs_int - rhs_int).to_string() == diff_ref);
        integer diff_assign = lhs_int;
        diff_assign -= rhs_int;
        CHECK(diff_assign.to_string() == diff_ref);

        const auto [quot_ref, rem_ref] = decimal::div_mod(lhs_digits, rhs_digits);
        CHECK((lhs_int / rhs_int).to_string() == quot_ref);
        CHECK((lhs_int % rhs_int).to_string() == rem_ref);

        auto div_mod_pair = lhs_int.div_mod(rhs_int);
        CHECK(div_mod_pair.first.to_string() == quot_ref);
        CHECK(div_mod_pair.second.to_string() == rem_ref);

        integer div_assign = lhs_int;
        div_assign /= rhs_int;
        CHECK(div_assign.to_string() == quot_ref);
        integer mod_assign = lhs_int;
        mod_assign %= rhs_int;
        CHECK(mod_assign.to_string() == rem_ref);

        const std::uint64_t scalar = 37 + static_cast<std::uint64_t>(i * 11);
        const std::string scalar_mul_ref = decimal::multiply_scalar(lhs_digits, scalar);
        CHECK((lhs_int * scalar).to_string() == scalar_mul_ref);
        integer scalar_mul_assign = lhs_int;
        scalar_mul_assign *= scalar;
        CHECK(scalar_mul_assign.to_string() == scalar_mul_ref);

        const auto [scalar_quot_ref, scalar_rem_ref] = decimal::div_mod_scalar(lhs_digits, scalar);
        CHECK((lhs_int / scalar).to_string() == scalar_quot_ref);
        CHECK(lhs_int % scalar == scalar_rem_ref);
        integer scalar_div_assign = lhs_int;
        scalar_div_assign /= scalar;
        CHECK(scalar_div_assign.to_string() == scalar_quot_ref);
        integer scalar_mod_assign = lhs_int;
        scalar_mod_assign %= scalar;
        CHECK(scalar_mod_assign.to_string() == std::to_string(scalar_rem_ref));
    }
}

TEST_CASE("integer shifts and increments behave correctly", "[integer][utility]") {
    const integer value("123456789");
    const int shift = 3;
    const integer shifted = value << shift;
    const std::string expected_shift = decimal::append_zeros(value.to_string(), integer::SECTION * shift);
    CHECK(shifted.to_string() == expected_shift);

    integer inc("99");
    const integer pre_inc = ++inc;
    CHECK(pre_inc.to_string() == "100");
    CHECK(inc.to_string() == "100");
    const integer post_inc = inc++;
    CHECK(post_inc.to_string() == "100");
    CHECK(inc.to_string() == "101");

    integer dec("1000");
    const integer pre_dec = --dec;
    CHECK(pre_dec.to_string() == "999");
    CHECK(dec.to_string() == "999");
    const integer post_dec = dec--;
    CHECK(post_dec.to_string() == "999");
    CHECK(dec.to_string() == "998");
}
