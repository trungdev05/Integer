#include "integer.hpp"

#include <benchmark/benchmark.h>

#include <string>

namespace {

std::string generate_digits(std::size_t digits) {
    std::string s;
    s.reserve(digits);
    for (std::size_t i = 0; i < digits; ++i) {
        s.push_back(static_cast<char>('0' + (i % 10)));
    }
    if (s.empty()) {
        s = "0";
    }
    return s;
}

struct operands {
    integer lhs;
    integer rhs;
};

operands make_operands(std::size_t digits) {
    const auto base = generate_digits(digits);
    return operands{integer(base), integer(base)};
}

void log_digits(benchmark::State &state, std::size_t digits) {
    state.counters["digits"] = static_cast<double>(digits);
}

}  // namespace

static void BM_IntegerMultiply(benchmark::State &state) {
    const std::size_t digits = static_cast<std::size_t>(state.range(0));
    const operands ops = make_operands(digits);
    log_digits(state, digits);
    for (auto _ : state) {
        auto result = ops.lhs * ops.rhs;
        benchmark::DoNotOptimize(result);
    }
}

static void BM_IntegerAdd(benchmark::State &state) {
    const std::size_t digits = static_cast<std::size_t>(state.range(0));
    const operands ops = make_operands(digits);
    log_digits(state, digits);
    for (auto _ : state) {
        auto result = ops.lhs + ops.rhs;
        benchmark::DoNotOptimize(result);
    }
}

static void BM_IntegerSubtract(benchmark::State &state) {
    const std::size_t digits = static_cast<std::size_t>(state.range(0));
    operands ops = make_operands(digits);
    log_digits(state, digits);
    for (auto _ : state) {
        auto result = ops.lhs - ops.rhs;
        benchmark::DoNotOptimize(result);
    }
}

static void BM_IntegerDivide(benchmark::State &state) {
    const std::size_t digits = static_cast<std::size_t>(state.range(0));
    operands ops = make_operands(digits);
    log_digits(state, digits);
    for (auto _ : state) {
        auto result = ops.lhs / ops.rhs;
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_IntegerMultiply)
    ->Args({10000})
    ->Args({20000})
    ->Args({50000})
    ->Args({100000})
    ->Args({200000})
    ->Args({500000})
    ->Args({1000000})
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_IntegerAdd)
    ->Args({10000})
    ->Args({20000})
    ->Args({50000})
    ->Args({100000})
    ->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IntegerSubtract)
    ->Args({10000})
    ->Args({20000})
    ->Args({50000})
    ->Args({100000})
    ->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_IntegerDivide)
    ->Args({1000})
    ->Args({5000})
    ->Args({10000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
