#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace benchmark {

enum TimeUnit {
    kNanosecond,
    kMicrosecond,
    kMillisecond,
    kSecond,
};

class State {
public:
    using CounterMap = std::map<std::string, double>;

    State(std::vector<std::int64_t> ranges, std::int64_t iterations)
        : ranges_(std::move(ranges)), iterations_(iterations) {}

    [[nodiscard]] std::int64_t range(int index) const { return ranges_.at(static_cast<std::size_t>(index)); }

    struct iterator {
        std::int64_t current;
        std::int64_t last;
        bool operator!=(const iterator &other) const { return current != other.current; }
        void operator++() { ++current; }
        int operator*() const { return 0; }
    };

    iterator begin() { return {0, iterations_}; }
    iterator end() { return {iterations_, iterations_}; }

    CounterMap counters;

private:
    std::vector<std::int64_t> ranges_;
    std::int64_t iterations_;
};

using BenchmarkFunction = void (*)(State &);

struct BenchmarkResult {
    std::string name;
    TimeUnit unit;
    std::int64_t iterations;
    double seconds_per_iteration;
};

class Benchmark {
public:
    Benchmark(std::string name, BenchmarkFunction function)
        : name_(std::move(name)), function_(function) {}

    Benchmark *Args(std::initializer_list<std::int64_t> args) {
        args_.emplace_back(args);
        return this;
    }

    Benchmark *Unit(TimeUnit unit) {
        unit_ = unit;
        return this;
    }

    [[nodiscard]] const std::string &name() const { return name_; }
    [[nodiscard]] BenchmarkFunction function() const { return function_; }
    [[nodiscard]] const std::vector<std::vector<std::int64_t>> &args() const { return args_; }
    [[nodiscard]] TimeUnit unit() const { return unit_; }

private:
    std::string name_;
    BenchmarkFunction function_;
    std::vector<std::vector<std::int64_t>> args_;
    TimeUnit unit_ = kNanosecond;
};

inline std::vector<Benchmark *> &registry() {
    static std::vector<Benchmark *> benchmarks;
    return benchmarks;
}

inline Benchmark *RegisterBenchmark(const char *name, BenchmarkFunction function) {
    auto *benchmark = new Benchmark(name, function);
    registry().push_back(benchmark);
    return benchmark;
}

template<typename T>
inline void DoNotOptimize(const T &value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(&value) : "memory");
#else
    (void)value;
#endif
}

inline void ClobberMemory() {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : : "memory");
#endif
}

inline const char *unit_name(TimeUnit unit) {
    switch (unit) {
        case kNanosecond: return "ns";
        case kMicrosecond: return "us";
        case kMillisecond: return "ms";
        case kSecond: return "s";
    }
    return "ns";
}

inline double seconds_to_unit(double seconds, TimeUnit unit) {
    switch (unit) {
        case kNanosecond: return seconds * 1e9;
        case kMicrosecond: return seconds * 1e6;
        case kMillisecond: return seconds * 1e3;
        case kSecond: return seconds;
    }
    return seconds * 1e9;
}

inline std::string full_name(const Benchmark &benchmark, const std::vector<std::int64_t> &args) {
    std::ostringstream out;
    out << benchmark.name();
    for (const auto arg: args)
        out << '/' << arg;
    return out.str();
}

inline BenchmarkResult run_one(const Benchmark &benchmark, const std::vector<std::int64_t> &args) {
    constexpr double min_seconds = 0.20;
    std::int64_t iterations = 1;
    double elapsed = 0.0;

    for (;;) {
        State state(args, iterations);
        const auto start = std::chrono::steady_clock::now();
        benchmark.function()(state);
        const auto stop = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration<double>(stop - start).count();
        if (elapsed >= min_seconds || iterations >= (std::int64_t{1} << 30))
            break;

        const double scale = min_seconds / (elapsed > 1e-12 ? elapsed : 1e-12);
        std::int64_t next = iterations * 2;
        if (scale > 2.0)
            next = static_cast<std::int64_t>(iterations * std::min(scale, 10.0));
        if (next <= iterations)
            next = iterations + 1;
        iterations = next;
    }

    return {full_name(benchmark, args), benchmark.unit(), iterations, elapsed / static_cast<double>(iterations)};
}

inline std::vector<BenchmarkResult> run_registered_benchmarks() {
    std::vector<BenchmarkResult> results;
    for (const Benchmark *benchmark: registry()) {
        const auto &all_args = benchmark->args();
        if (all_args.empty()) {
            results.push_back(run_one(*benchmark, {}));
        } else {
            for (const auto &args: all_args)
                results.push_back(run_one(*benchmark, args));
        }
    }
    return results;
}

inline void print_json(const std::vector<BenchmarkResult> &results) {
    std::cout << "{\n  \"benchmarks\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto &result = results[i];
        const double real_time = seconds_to_unit(result.seconds_per_iteration, result.unit);
        std::cout << "    {\"name\": \"" << result.name << "\", "
                  << "\"iterations\": " << result.iterations << ", "
                  << "\"real_time\": " << real_time << ", "
                  << "\"cpu_time\": " << real_time << ", "
                  << "\"time_unit\": \"" << unit_name(result.unit) << "\"}";
        if (i + 1 != results.size())
            std::cout << ',';
        std::cout << '\n';
    }
    std::cout << "  ]\n}\n";
}

inline void print_text(const std::vector<BenchmarkResult> &results) {
    for (const auto &result: results) {
        std::cout << result.name << ' ' << seconds_to_unit(result.seconds_per_iteration, result.unit)
                  << ' ' << unit_name(result.unit) << '\n';
    }
}

inline int RunSpecifiedBenchmarks(int argc, char **argv) {
    bool json = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--benchmark_format=json") == 0)
            json = true;
    }
    const auto results = run_registered_benchmarks();
    if (json)
        print_json(results);
    else
        print_text(results);
    return 0;
}

}  // namespace benchmark

#define BENCHMARK_FALLBACK_JOIN_IMPL(a, b) a##b
#define BENCHMARK_FALLBACK_JOIN(a, b) BENCHMARK_FALLBACK_JOIN_IMPL(a, b)
#define BENCHMARK(func) \
    static ::benchmark::Benchmark *BENCHMARK_FALLBACK_JOIN(benchmark_fallback_reg_, __LINE__) = \
        ::benchmark::RegisterBenchmark(#func, func)
#define BENCHMARK_MAIN() \
    int main(int argc, char **argv) { return ::benchmark::RunSpecifiedBenchmarks(argc, argv); }
