#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>
#pragma GCC optimize("O3")
using namespace std;

namespace FFT {
    template<typename float_t>
    struct complex {
        float_t x, y;

        complex(float_t _x = 0, float_t _y = 0) : x(_x), y(_y) {
        }

        [[nodiscard]] float_t real() const { return x; }
        [[nodiscard]] float_t imaginary() const { return y; }

        void real(float_t _x) { x = _x; }
        void imaginary(float_t _y) { y = _y; }

        complex &operator+=(const complex &other) {
            x += other.x;
            y += other.y;
            return *this;
        }

        complex &operator-=(const complex &other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        complex operator+(const complex &other) const { return complex(*this) += other; }
        complex operator-(const complex &other) const { return complex(*this) -= other; }

        complex operator*(const complex &other) const {
            return {x * other.x - y * other.y, x * other.y + other.x * y};
        }

        complex operator*(float_t scalar) const {
            return {x * scalar, y * scalar};
        }

        friend complex conjugation(const complex &c) {
            return {c.x, -c.y};
        }
    };

    template<typename float_t>
    complex<float_t> polar(float_t magnitude, float_t angle) {
        return {magnitude * cos(angle), magnitude * sin(angle)};
    }

    using float_t = double;
    static constexpr float_t ONE = 1;
    static constexpr double PI = 3.141592653589793238462643383279502884;
    inline vector<complex<float_t> > roots = {{0, 0}, {1, 0}};
    inline vector<int> bit_reverse;

    inline int round_up_power_two(const int n) {
        int bit = n == 0 ? -1 : 31 - __builtin_clz(n);
        bit += bit < 0 || 1 << bit < n;
        return 1 << bit;
    }

    inline void bit_reorder(const int n, vector<complex<float_t> > &values) {
        if (static_cast<int>(bit_reverse.size()) != n) {
            bit_reverse.assign(n, 0);
            const int length = __builtin_ctz(n);

            for (int i = 1; i < n; i++)
                bit_reverse[i] = bit_reverse[i >> 1] >> 1 | (i & 1) << length - 1;
        }

        for (int i = 0; i < n; i++)
            if (i < bit_reverse[i])
                swap(values[i], values[bit_reverse[i]]);
    }

    inline void prepare_roots(const int n) {
        if (static_cast<int>(roots.size()) >= n)
            return;

        int length = __builtin_ctz(static_cast<int>(roots.size()));
        roots.resize(n);

        while (1 << length < n) {
            const float_t min_angle = 2 * PI / (1 << (length + 1));
            for (int i = 0; i < 1 << length - 1; i++) {
                const int index = (1 << length - 1) + i;
                roots[2 * index] = roots[index];
                roots[2 * index + 1] = polar(ONE, min_angle * (2 * i + 1));
            }
            length++;
        }
    }

    inline void fft_iterative(const int n, std::vector<complex<float_t> > &values) {
        prepare_roots(n);
        bit_reorder(n, values);

        for (int len = 1; len < n; len *= 2)
            for (int start = 0; start < n; start += 2 * len)
                for (int i = 0; i < len; i++) {
                    const complex<float_t> &even = values[start + i];
                    complex<float_t> odd = values[start + len + i] * roots[len + i];
                    values[start + len + i] = even - odd;
                    values[start + i] = even + odd;
                }
    }

    inline complex<float_t> extract(const int n, const vector<complex<float_t> > &values, const int index,
                                    const int side) {
        if (side == -1) {
            const int other = n - index & n - 1;
            return (conjugation(values[other] * values[other]) - values[index] * values[index]) * complex<float_t>(0, 0.25);
        }

        const int other = n - index & n - 1;
        const int sign = side == 0 ? +1 : -1;
        const complex<float_t> multiplier = side == 0 ? complex<float_t>(0.5, 0) : complex<float_t>(0, -0.5);
        return multiplier * complex(values[index].real() + values[other].real() * sign,
                                    values[index].imaginary() - values[other].imaginary() * sign);
    }

    inline void invert_fft(const int n, vector<complex<float_t> > &values) {
        for (int i = 0; i < n; i++)
            values[i] = conjugation(values[i]) * (ONE / n);

        for (int i = 0; i < n / 2; i++) {
            complex<float_t> first = values[i] + values[n / 2 + i];
            complex<float_t> second = (values[i] - values[n / 2 + i]) * roots[n / 2 + i];
            values[i] = first + second * complex<float_t>(0, 1);
        }

        fft_iterative(n / 2, values);

        for (int i = n - 1; i >= 0; i--)
            values[i] = i % 2 == 0 ? values[i / 2].real() : values[i / 2].imaginary();
    }

    constexpr float_t SPLIT_CUTOFF = 2e15;
    constexpr int SPLIT_BASE = 1 << 15;

    template<typename T_out, typename T_in>
    vector<T_out> square(const vector<T_in> &input) {
        if (input.empty())
            return {};

        const int n = static_cast<int>(input.size());
        int output_size = 2 * n - 1;
        const int N = round_up_power_two(n);

        const double brute_force_cost = 0.4 * n * n;
        const double fft_cost = 2.0 * N * (__builtin_ctz(N) + 3);

        if (brute_force_cost < fft_cost) {
            vector<T_out> result(output_size);

            for (int i = 0; i < n; i++) {
                result[2 * i] += static_cast<T_out>(input[i]) * static_cast<T_out>(input[i]);
                for (int j = i + 1; j < n; j++)
                    result[i + j] += 2 * static_cast<T_out>(input[i]) * static_cast<T_out>(input[j]);
            }

            return result;
        }

        prepare_roots(2 * N);
        vector<complex<float_t> > values(N, 0);

        for (int i = 0; i < n; i += 2)
            values[i / 2] = complex(static_cast<float_t>(input[i]), i + 1 < n ? static_cast<float_t>(input[i + 1]) : 0);

        fft_iterative(N, values);

        for (int i = 0; i <= N / 2; i++) {
            const int j = N - i & N - 1;
            complex<float_t> even = extract(N, values, i, 0);
            complex<float_t> odd = extract(N, values, i, 1);
            complex<float_t> aux = even * even + odd * odd * roots[N + i] * roots[N + i];
            complex<float_t> tmp = even * odd;
            values[i] = aux - complex<float_t>(0, 2) * tmp;
            values[j] = conjugation(aux) - complex<float_t>(0, 2) * conjugation(tmp);
        }

        for (int i = 0; i < N; i++)
            values[i] = conjugation(values[i]) * (ONE / N);

        fft_iterative(N, values);
        vector<T_out> result(output_size);

        for (int i = 0; i < output_size; i++) {
            float_t value = i % 2 == 0 ? values[i / 2].real() : values[i / 2].imaginary();
            result[i] = static_cast<T_out>(is_integral_v<T_out> ? round(value) : value);
        }

        return result;
    }

    template<typename T_out, typename T_in>
    vector<T_out> multiply(const vector<T_in> &left, const vector<T_in> &right, const bool circular = false) {
        if (left.empty() || right.empty())
            return {};

        if (left == right && !circular)
            return square<T_out>(left);

        const int n = static_cast<int>(left.size());
        const int m = static_cast<int>(right.size());

        int output_size = circular ? round_up_power_two(max(n, m)) : n + m - 1;
        const int N = round_up_power_two(output_size);
        const double brute_force_cost = 0.55 * n * m;
        const double fft_cost = 1.5 * N * (__builtin_ctz(N) + 3);

        if (brute_force_cost < fft_cost) {
            auto mod_output_size = [&](const int x) -> int {
                return x < output_size ? x : x - output_size;
            };
            vector<T_out> result(output_size, 0);
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < m; j++) {
                    result[mod_output_size(i + j)] += static_cast<T_out>(left[i]) * static_cast<T_out>(right[j]);
                }
            }

            return result;
        }

        vector<complex<float_t> > values(N, 0);

        for (int i = 0; i < n; i++)
            values[i].real(static_cast<float_t>(left[i]));

        for (int i = 0; i < m; i++)
            values[i].imaginary(static_cast<float_t>(right[i]));

        fft_iterative(N, values);
        for (int i = 0; i <= N / 2; i++) {
            const int j = N - i & N - 1;
            complex<float_t> product_i = extract(N, values, i, -1);
            values[i] = product_i;
            values[j] = conjugation(product_i);
        }
        invert_fft(N, values);
        vector<T_out> result(output_size, 0);
        for (int i = 0; i < output_size; i++) {
            if constexpr (is_integral_v<T_out>) {
                result[i] = static_cast<T_out>(round(values[i].real()));
            } else {
                result[i] = static_cast<T_out>(values[i].real());
            }
        }

        return result;
    }
}


struct integer {
    using value_t = uint16_t;

    static constexpr int SECTION = 4;
    static constexpr value_t BASE = 1e4;
    static constexpr int DOUBLE_DIV_SECTIONS = 5;
    static constexpr int INTEGER_FFT_CUTOFF = 1500;
    static constexpr int KARATSUBA_CUTOFF = 150;
    static constexpr uint64_t U64_BOUND = numeric_limits<uint64_t>::max() - static_cast<uint64_t>(BASE) * BASE;
    static constexpr uint64_t BASE_OVERFLOW_CUTOFF = numeric_limits<uint64_t>::max() / BASE;
    static constexpr char FIRST_NUMBER_CHAR = '0';
    static constexpr char LAST_NUMBER_CHAR = '9';
    vector<value_t> values;

    integer(const uint64_t x = 0) {
        init(x);
    }

    integer(const string &str) {
        const int len = static_cast<int>(str.size());
        const int num_values = max((len + SECTION - 1) / SECTION, 1);
        values.assign(num_values, 0);

        int counter = 0;
        int index = 0;
        value_t p10 = 1;

        for (int i = len - 1; i >= 0; i--) {
            assert(FIRST_NUMBER_CHAR <= str[i] && str[i] <= LAST_NUMBER_CHAR);
            values[index] = static_cast<value_t>(values[index] + p10 * (str[i] - FIRST_NUMBER_CHAR));

            if (++counter >= SECTION) {
                counter = 0;
                index++;
                p10 = 1;
            } else {
                p10 = static_cast<value_t>(10 * p10);
            }
        }

        trim_check();
    }

    explicit operator uint64_t() const {
        uint64_t integer_value = 0;

        for (int i = static_cast<int>(values.size()) - 1; i >= 0; i--)
            integer_value = BASE * integer_value + values[i];

        return integer_value;
    }

    void trim_check() {
        while (values.size() > 1 && values.back() == 0)
            values.pop_back();

        if (values.empty())
            values = {0};
    }

    void checked_add(const size_t position, const uint64_t add) {
        if (position >= values.size())
            values.resize(position + 1, 0);

        values[position] = static_cast<value_t>(values[position] + add);
    }

    void init(uint64_t x) {
        values.clear();

        do {
            checked_add(values.size(), x % BASE);
            x /= BASE;
        } while (x > 0);
    }

    [[nodiscard]] string to_string() const {
        string result;

        for (value_t v: values)
            for (int i = 0; i < SECTION; i++) {
                result += static_cast<char>(FIRST_NUMBER_CHAR + v % 10);
                v = static_cast<value_t>(v / 10);
            }

        while (result.size() > 1 && result.back() == FIRST_NUMBER_CHAR)
            result.pop_back();

        ranges::reverse(result);
        return result;
    }

    [[nodiscard]] int compare(const integer &other) const {
        const int n = static_cast<int>(values.size()), m = static_cast<int>(other.values.size());

        if (n != m)
            return n < m ? -1 : +1;

        for (int i = n - 1; i >= 0; i--)
            if (values[i] != other.values[i])
                return values[i] < other.values[i] ? -1 : +1;

        return 0;
    }

    bool operator<(const integer &other) const { return compare(other) < 0; }
    bool operator>(const integer &other) const { return compare(other) > 0; }
    bool operator<=(const integer &other) const { return compare(other) <= 0; }
    bool operator>=(const integer &other) const { return compare(other) >= 0; }
    bool operator==(const integer &other) const { return compare(other) == 0; }
    bool operator!=(const integer &other) const { return compare(other) != 0; }

    integer operator<<(const int p) const {
        assert(p >= 0);
        const int n = static_cast<int>(values.size());

        integer result;
        result.values.resize(n + p, 0);

        for (int i = 0; i < n; i++)
            result.values[i + p] = values[i];

        result.trim_check();
        return result;
    }

    [[nodiscard]] integer range(const int a, int b = -1) const {
        if (b == -1)
            b = static_cast<int>(values.size());

        assert(a <= b);
        integer result;
        result.values.resize(b - a);

        for (int i = 0; i < b - a; i++)
            result.values[i] = values[i + a];

        result.trim_check();
        return result;
    }

    integer &operator+=(const integer &other) {
        const int n = static_cast<int>(other.values.size());

        for (int i = 0, carry = 0; i < n || carry > 0; i++) {
            checked_add(i, (i < n ? other.values[i] : 0) + carry);

            if (values[i] >= BASE) {
                values[i] = static_cast<value_t>(values[i] - BASE);
                carry = 1;
            } else {
                carry = 0;
            }
        }

        trim_check();
        return *this;
    }

    integer &operator-=(const integer &other) {
        assert(*this >= other);
        const int n = static_cast<int>(other.values.size());

        for (int i = 0, carry = 0; i < n || carry > 0; i++) {
            const auto subtract = static_cast<value_t>((i < n ? other.values[i] : 0) + carry);

            if (values[i] < subtract) {
                values[i] = static_cast<value_t>(values[i] + BASE - subtract);
                carry = 1;
            } else {
                values[i] = static_cast<value_t>(values[i] - subtract);
                carry = 0;
            }
        }

        trim_check();
        return *this;
    }

    friend integer operator+(const integer &a, const integer &b) { return integer(a) += b; }
    friend integer operator-(const integer &a, const integer &b) { return integer(a) -= b; }

    friend integer operator*(const integer &a, const integer &b) {
        const int n = static_cast<int>(a.values.size());
        const int m = static_cast<int>(b.values.size());

        if (n > m)
            return b * a;

        if (n > KARATSUBA_CUTOFF && n + m > INTEGER_FFT_CUTOFF) {
            const vector<uint64_t> &multiplication = FFT::multiply<uint64_t>(a.values, b.values);
            const int N = static_cast<int>(multiplication.size());
            integer product = 0;
            uint64_t carry = 0;

            for (int i = 0; i < N || carry > 0; i++) {
                uint64_t value = (i < N ? multiplication[i] : 0) + carry;
                carry = value / BASE;
                value %= BASE;
                product.checked_add(i, value);
            }

            product.trim_check();
            return product;
        }

        if (n > KARATSUBA_CUTOFF) {
            const int mid = n / 2;
            const integer &a1 = a.range(0, mid);
            const integer &a2 = a.range(mid, n);
            const integer &b1 = b.range(0, mid);
            const integer &b2 = b.range(mid, m);

            const integer &x = a2 * b2;
            const integer &z = a1 * b1;
            const integer &y = (a1 + a2) * (b1 + b2) - x - z;
            return (x << 2 * mid) + (y << mid) + z;
        }

        integer product;
        product.values.resize(n + m - 1, 0);
        uint64_t value = 0, carry = 0;

        for (int index_sum = 0; index_sum < n + m - 1 || carry > 0; index_sum++) {
            value = carry % BASE;
            carry /= BASE;

            for (int i = max(0, index_sum - (m - 1)); i <= min(n - 1, index_sum); i++) {
                value += static_cast<uint64_t>(a.values[i]) * b.values[index_sum - i];

                if (value > U64_BOUND) {
                    carry += value / BASE;
                    value %= BASE;
                }
            }

            carry += value / BASE;
            value %= BASE;
            product.checked_add(index_sum, value);
        }

        product.trim_check();
        return product;
    }

    integer &operator*=(const integer &other) {
        return *this = *this * other;
    }

    integer operator*(const uint64_t scalar) const {
        if (scalar == 0)
            return 0;

        if (scalar >= BASE_OVERFLOW_CUTOFF)
            return *this * integer(scalar);

        const int n = static_cast<int>(values.size());

        integer product;
        product.values.resize(n + 1, 0);
        uint64_t carry = 0;

        for (int i = 0; i < n || carry > 0; i++) {
            uint64_t value = scalar * (i < n ? values[i] : 0) + carry;
            carry = value / BASE;
            value %= BASE;
            product.checked_add(i, value);
        }

        product.trim_check();
        return product;
    }

    integer &operator*=(const uint64_t scalar) {
        return *this = *this * scalar;
    }

    [[nodiscard]] double estimate_div(const integer &other) const {
        const int n = static_cast<int>(values.size());
        const int m = static_cast<int>(other.values.size());
        double estimate = 0, other_estimate = 0;
        int count = 0, other_count = 0;
        double p_base = 1;

        for (int i = n - 1; i >= 0 && count < DOUBLE_DIV_SECTIONS; i--) {
            estimate = estimate + p_base * static_cast<double>(values[i]);
            p_base /= BASE;
            count++;
        }

        p_base = 1;

        for (int i = m - 1; i >= 0 && other_count < DOUBLE_DIV_SECTIONS; i--) {
            other_estimate = other_estimate + p_base * static_cast<double>(other.values[i]);
            p_base /= BASE;
            other_count++;
        }

        return estimate / other_estimate * pow(BASE, n - m);
    }

    [[nodiscard]] pair<integer, integer> div_mod(const integer &other) const {
        assert(other > 0);

        const int n = static_cast<int>(values.size());
        const int m = static_cast<int>(other.values.size());
        integer quotient = 0;
        integer remainder = *this;

        for (int i = n - m; i >= 0; i--) {
            if (i >= static_cast<int>(remainder.values.size()))
                continue;

            integer chunk = remainder.range(i);

            auto div = static_cast<uint64_t>(chunk.estimate_div(other) + 1e-7);
            integer scalar = other * div;

            while (div > 0 && scalar > chunk) {
                scalar -= other;
                div--;
            }

            while (div < BASE - 1 && scalar + other <= chunk) {
                scalar += other;
                div++;
            }

            remainder -= scalar << i;
            remainder.trim_check();

            if (div > 0)
                quotient.checked_add(i, div);
        }

        quotient.trim_check();
        remainder.trim_check();
        return make_pair(quotient, remainder);
    }

    friend integer operator/(const integer &a, const integer &b) { return a.div_mod(b).first; }
    friend integer operator%(const integer &a, const integer &b) { return a.div_mod(b).second; }

    integer &operator/=(const integer &other) { return *this = *this / other; }
    integer &operator%=(const integer &other) { return *this = *this % other; }

    [[nodiscard]] pair<integer, uint64_t> div_mod(const uint64_t denominator) const {
        assert(denominator > 0);

        if (denominator >= BASE_OVERFLOW_CUTOFF) {
            auto [fst, snd] = div_mod(integer(denominator));
            return make_pair(fst, static_cast<uint64_t>(snd));
        }

        const int n = static_cast<int>(values.size());
        integer quotient = 0;
        uint64_t remainder = 0;

        for (int i = n - 1; i >= 0; i--) {
            remainder = BASE * remainder + values[i];

            if (remainder >= denominator) {
                quotient.checked_add(i, remainder / denominator);
                remainder %= denominator;
            }
        }

        quotient.trim_check();
        return make_pair(quotient, remainder);
    }

    integer operator/(const uint64_t denominator) const {
        return div_mod(denominator).first;
    }

    uint64_t operator%(const uint64_t denominator) const {
        assert(denominator > 0);

        if (BASE % denominator == 0) {
            assert(!values.empty());
            return values[0] % denominator;
        }

        if (denominator >= BASE_OVERFLOW_CUTOFF)
            return static_cast<uint64_t>(div_mod(integer(denominator)).second);

        const int n = static_cast<int>(values.size());
        uint64_t remainder = 0;

        for (int i = n - 1; i >= 0; i--) {
            remainder = BASE * remainder + values[i];

            if (remainder >= BASE_OVERFLOW_CUTOFF)
                remainder %= denominator;
        }

        remainder %= denominator;
        return remainder;
    }

    integer &operator/=(const uint64_t denominator) { return *this = *this / denominator; }
    integer &operator%=(const uint64_t denominator) { return *this = *this % denominator; }

    integer &operator++() { return *this += 1; }
    integer &operator--() { return *this -= 1; }

    integer operator++(int) {
        integer before = *this;
        ++*this;
        return before;
    }

    integer operator--(int) {
        integer before = *this;
        --*this;
        return before;
    }

    friend ostream &operator<<(ostream &os, const integer &x) {
        return os << x.to_string();
    }
};
