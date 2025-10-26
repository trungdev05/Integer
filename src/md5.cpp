#include "md5.hpp"

#include <cassert>
#include <cstring>

namespace {

constexpr std::uint32_t MD5_ROTATE_LEFT(const std::uint32_t x, const std::uint32_t n) {
    return (x << n) | (x >> (32 - n));
}

constexpr std::uint32_t MD5_ROUND_TAIL(std::uint32_t a,
                                       const std::uint32_t b,
                                       const std::uint32_t expr,
                                       const std::uint32_t k,
                                       const std::uint32_t s,
                                       const std::uint32_t t,
                                       const std::uint32_t block[16]) {
    a += expr + t + block[k];
    return b + MD5_ROTATE_LEFT(a, s);
}

constexpr std::uint32_t MD5_ROUND_0(std::uint32_t a,
                                    const std::uint32_t b,
                                    const std::uint32_t c,
                                    const std::uint32_t d,
                                    const std::uint32_t k,
                                    const std::uint32_t s,
                                    const std::uint32_t t,
                                    const std::uint32_t block[16]) {
    return MD5_ROUND_TAIL(a, b, d ^ (b & (c ^ d)), k, s, t, block);
}

constexpr std::uint32_t MD5_ROUND_1(std::uint32_t a,
                                    const std::uint32_t b,
                                    const std::uint32_t c,
                                    const std::uint32_t d,
                                    const std::uint32_t k,
                                    const std::uint32_t s,
                                    const std::uint32_t t,
                                    const std::uint32_t block[16]) {
    return MD5_ROUND_TAIL(a, b, c ^ (d & (b ^ c)), k, s, t, block);
}

constexpr std::uint32_t MD5_ROUND_2(std::uint32_t a,
                                    const std::uint32_t b,
                                    const std::uint32_t c,
                                    const std::uint32_t d,
                                    const std::uint32_t k,
                                    const std::uint32_t s,
                                    const std::uint32_t t,
                                    const std::uint32_t block[16]) {
    return MD5_ROUND_TAIL(a, b, b ^ c ^ d, k, s, t, block);
}

constexpr std::uint32_t MD5_ROUND_3(std::uint32_t a,
                                    const std::uint32_t b,
                                    const std::uint32_t c,
                                    const std::uint32_t d,
                                    const std::uint32_t k,
                                    const std::uint32_t s,
                                    const std::uint32_t t,
                                    const std::uint32_t block[16]) {
    return MD5_ROUND_TAIL(a, b, c ^ (b | ~d), k, s, t, block);
}

}  // namespace

namespace {

void md5_compress(std::uint32_t state[4], const std::uint32_t block[16]) {
    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];

    // ROUND0
    a = MD5_ROUND_0(a, b, c, d, 0, 7, 0xD76AA478, block);
    d = MD5_ROUND_0(d, a, b, c, 1, 12, 0xE8C7B756, block);
    c = MD5_ROUND_0(c, d, a, b, 2, 17, 0x242070DB, block);
    b = MD5_ROUND_0(b, c, d, a, 3, 22, 0xC1BDCEEE, block);
    a = MD5_ROUND_0(a, b, c, d, 4, 7, 0xF57C0FAF, block);
    d = MD5_ROUND_0(d, a, b, c, 5, 12, 0x4787C62A, block);
    c = MD5_ROUND_0(c, d, a, b, 6, 17, 0xA8304613, block);
    b = MD5_ROUND_0(b, c, d, a, 7, 22, 0xFD469501, block);
    a = MD5_ROUND_0(a, b, c, d, 8, 7, 0x698098D8, block);
    d = MD5_ROUND_0(d, a, b, c, 9, 12, 0x8B44F7AF, block);
    c = MD5_ROUND_0(c, d, a, b, 10, 17, 0xFFFF5BB1, block);
    b = MD5_ROUND_0(b, c, d, a, 11, 22, 0x895CD7BE, block);
    a = MD5_ROUND_0(a, b, c, d, 12, 7, 0x6B901122, block);
    d = MD5_ROUND_0(d, a, b, c, 13, 12, 0xFD987193, block);
    c = MD5_ROUND_0(c, d, a, b, 14, 17, 0xA679438E, block);
    b = MD5_ROUND_0(b, c, d, a, 15, 22, 0x49B40821, block);

    // ROUND1
    a = MD5_ROUND_1(a, b, c, d, 1, 5, 0xF61E2562, block);
    d = MD5_ROUND_1(d, a, b, c, 6, 9, 0xC040B340, block);
    c = MD5_ROUND_1(c, d, a, b, 11, 14, 0x265E5A51, block);
    b = MD5_ROUND_1(b, c, d, a, 0, 20, 0xE9B6C7AA, block);
    a = MD5_ROUND_1(a, b, c, d, 5, 5, 0xD62F105D, block);
    d = MD5_ROUND_1(d, a, b, c, 10, 9, 0x02441453, block);
    c = MD5_ROUND_1(c, d, a, b, 15, 14, 0xD8A1E681, block);
    b = MD5_ROUND_1(b, c, d, a, 4, 20, 0xE7D3FBC8, block);
    a = MD5_ROUND_1(a, b, c, d, 9, 5, 0x21E1CDE6, block);
    d = MD5_ROUND_1(d, a, b, c, 14, 9, 0xC33707D6, block);
    c = MD5_ROUND_1(c, d, a, b, 3, 14, 0xF4D50D87, block);
    b = MD5_ROUND_1(b, c, d, a, 8, 20, 0x455A14ED, block);
    a = MD5_ROUND_1(a, b, c, d, 13, 5, 0xA9E3E905, block);
    d = MD5_ROUND_1(d, a, b, c, 2, 9, 0xFCEFA3F8, block);
    c = MD5_ROUND_1(c, d, a, b, 7, 14, 0x676F02D9, block);
    b = MD5_ROUND_1(b, c, d, a, 12, 20, 0x8D2A4C8A, block);

    // ROUND2
    a = MD5_ROUND_2(a, b, c, d, 5, 4, 0xFFFA3942, block);
    d = MD5_ROUND_2(d, a, b, c, 8, 11, 0x8771F681, block);
    c = MD5_ROUND_2(c, d, a, b, 11, 16, 0x6D9D6122, block);
    b = MD5_ROUND_2(b, c, d, a, 14, 23, 0xFDE5380C, block);
    a = MD5_ROUND_2(a, b, c, d, 1, 4, 0xA4BEEA44, block);
    d = MD5_ROUND_2(d, a, b, c, 4, 11, 0x4BDECFA9, block);
    c = MD5_ROUND_2(c, d, a, b, 7, 16, 0xF6BB4B60, block);
    b = MD5_ROUND_2(b, c, d, a, 10, 23, 0xBEBFBC70, block);
    a = MD5_ROUND_2(a, b, c, d, 13, 4, 0x289B7EC6, block);
    d = MD5_ROUND_2(d, a, b, c, 0, 11, 0xEAA127FA, block);
    c = MD5_ROUND_2(c, d, a, b, 3, 16, 0xD4EF3085, block);
    b = MD5_ROUND_2(b, c, d, a, 6, 23, 0x04881D05, block);
    a = MD5_ROUND_2(a, b, c, d, 9, 4, 0xD9D4D039, block);
    d = MD5_ROUND_2(d, a, b, c, 12, 11, 0xE6DB99E5, block);
    c = MD5_ROUND_2(c, d, a, b, 15, 16, 0x1FA27CF8, block);
    b = MD5_ROUND_2(b, c, d, a, 2, 23, 0xC4AC5665, block);

    // ROUND3
    a = MD5_ROUND_3(a, b, c, d, 0, 6, 0xF4292244, block);
    d = MD5_ROUND_3(d, a, b, c, 7, 10, 0x432AFF97, block);
    c = MD5_ROUND_3(c, d, a, b, 14, 15, 0xAB9423A7, block);
    b = MD5_ROUND_3(b, c, d, a, 5, 21, 0xFC93A039, block);
    a = MD5_ROUND_3(a, b, c, d, 12, 6, 0x655B59C3, block);
    d = MD5_ROUND_3(d, a, b, c, 3, 10, 0x8F0CCC92, block);
    c = MD5_ROUND_3(c, d, a, b, 10, 15, 0xFFEFF47D, block);
    b = MD5_ROUND_3(b, c, d, a, 1, 21, 0x85845DD1, block);
    a = MD5_ROUND_3(a, b, c, d, 8, 6, 0x6FA87E4F, block);
    d = MD5_ROUND_3(d, a, b, c, 15, 10, 0xFE2CE6E0, block);
    c = MD5_ROUND_3(c, d, a, b, 6, 15, 0xA3014314, block);
    b = MD5_ROUND_3(b, c, d, a, 13, 21, 0x4E0811A1, block);
    a = MD5_ROUND_3(a, b, c, d, 4, 6, 0xF7537E82, block);
    d = MD5_ROUND_3(d, a, b, c, 11, 10, 0xBD3AF235, block);
    c = MD5_ROUND_3(c, d, a, b, 2, 15, 0x2AD7D2BB, block);
    b = MD5_ROUND_3(b, c, d, a, 9, 21, 0xEB86D391, block);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}


}  // namespace

void md5_hash(const std::uint8_t *message, const std::uint32_t len, std::uint32_t hash[4]) {
    hash[0] = 0x67452301;
    hash[1] = 0xEFCDAB89;
    hash[2] = 0x98BADCFE;
    hash[3] = 0x10325476;
    for (std::uint32_t i = 0; i + 64 <= len; i += 64) {
        md5_compress(hash, reinterpret_cast<const std::uint32_t *>(message + i));
    }
    std::uint32_t block[16] = {};
    auto *byte_block = reinterpret_cast<std::uint8_t *>(block);
    const std::uint32_t rem = len % 64;
    std::memcpy(byte_block, message + len - rem, rem);
    byte_block[rem] = 0x80;
    if (rem >= 56) {
        md5_compress(hash, block);
        std::memset(block, 0, sizeof(block));
    }
    const std::uint64_t bit_len = static_cast<std::uint64_t>(len) * 8;
    block[14] = static_cast<std::uint32_t>(bit_len);
    block[15] = static_cast<std::uint32_t>(bit_len >> 32);

    md5_compress(hash, block);
}

std::string md5_hash(const std::string &message) {
    std::uint32_t hash[4];
    md5_hash(reinterpret_cast<const std::uint8_t *>(message.c_str()), static_cast<std::uint32_t>(message.size()), hash);
    std::string result;

    for (const std::uint32_t h : hash) {
        for (int i = 0; i < 4; i++) {
            result += "0123456789abcdef"[(h >> (i * 8 + 4)) & 0xF];
            result += "0123456789abcdef"[(h >> (i * 8)) & 0xF];
        }
    }

    return result;
}

void test_hash() {
    const std::string message = "hello";
    const std::string expected = "5d41402abc4b2a76b9719d911017c592";
    const std::string result = md5_hash(message);
    assert(result == expected);
}
