#pragma once

#include <cstdint>
#include <string>

void md5_hash(const std::uint8_t *message, std::uint32_t len, std::uint32_t hash[4]);
std::string md5_hash(const std::string &message);

void test_hash();
