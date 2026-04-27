#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace catch2_fallback {

struct test_failure : std::exception {
    const char *what() const noexcept override { return "test requirement failed"; }
};

struct test_case_entry {
    std::string name;
    void (*function)();
};

inline std::vector<test_case_entry> &registry() {
    static std::vector<test_case_entry> tests;
    return tests;
}

inline int &failure_count() {
    static int count = 0;
    return count;
}

inline int &current_test_failures() {
    static int count = 0;
    return count;
}

struct registrar {
    registrar(const char *name, void (*function)()) {
        registry().push_back({name, function});
    }
};

inline void report_assertion(bool passed, const char *expr, const char *file, int line, bool fatal) {
    if (passed)
        return;

    ++failure_count();
    ++current_test_failures();
    std::cerr << file << ':' << line << ": " << (fatal ? "REQUIRE" : "CHECK")
              << " failed: " << expr << '\n';
    if (fatal)
        throw test_failure();
}

inline int run_all_tests() {
    int failed_tests = 0;
    for (const auto &test: registry()) {
        current_test_failures() = 0;
        try {
            test.function();
        } catch (const test_failure &) {
            // Already reported by REQUIRE.
        } catch (const std::exception &ex) {
            ++failure_count();
            ++current_test_failures();
            std::cerr << test.name << ": unexpected exception: " << ex.what() << '\n';
        } catch (...) {
            ++failure_count();
            ++current_test_failures();
            std::cerr << test.name << ": unexpected non-standard exception\n";
        }

        if (current_test_failures() != 0) {
            ++failed_tests;
            std::cerr << "[failed] " << test.name << " (" << current_test_failures() << " assertion failure(s))\n";
        } else {
            std::cerr << "[passed] " << test.name << '\n';
        }
    }

    if (failed_tests == 0) {
        std::cerr << "All tests passed (" << registry().size() << " test case(s)).\n";
    } else {
        std::cerr << failed_tests << " test case(s) failed, " << failure_count() << " assertion failure(s).\n";
    }
    return failed_tests == 0 ? 0 : 1;
}

}  // namespace catch2_fallback

#define CATCH2_FALLBACK_JOIN_IMPL(a, b) a##b
#define CATCH2_FALLBACK_JOIN(a, b) CATCH2_FALLBACK_JOIN_IMPL(a, b)

#define TEST_CASE(name, tags) \
    static void CATCH2_FALLBACK_JOIN(catch2_fallback_test_, __LINE__)(); \
    static ::catch2_fallback::registrar CATCH2_FALLBACK_JOIN(catch2_fallback_reg_, __LINE__)( \
        name, &CATCH2_FALLBACK_JOIN(catch2_fallback_test_, __LINE__)); \
    static void CATCH2_FALLBACK_JOIN(catch2_fallback_test_, __LINE__)()

#define CHECK(expr) \
    do { ::catch2_fallback::report_assertion(static_cast<bool>(expr), #expr, __FILE__, __LINE__, false); } while (false)

#define REQUIRE(expr) \
    do { ::catch2_fallback::report_assertion(static_cast<bool>(expr), #expr, __FILE__, __LINE__, true); } while (false)
