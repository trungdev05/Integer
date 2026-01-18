#include "integer.hpp"
#include <iostream>
#include <string>

int main() {
    // Read operation and operands from stdin
    // Format: OP OP1 OP2
    // e.g.: + 123 456
    std::string op;
    std::string s1, s2;

    if (!(std::cin >> op >> s1 >> s2)) {
        return 0;
    }

    integer a(s1);
    integer b(s2);
    integer res;

    // Perform operation
    if (op == "+") {
        res = a + b;
    } else if (op == "-") {
        res = a - b;
    } else if (op == "*") {
        res = a * b;
    } else if (op == "/") {
        // Integer division
        res = a / b;
    } else {
        std::cerr << "Unknown operation: " << op << std::endl;
        return 1;
    }

    std::cout << res.to_string() << std::endl;
    return 0;
}
