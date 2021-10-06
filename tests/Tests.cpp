#define CATCH_CONFIG_MAIN

#include "bt/ByteEncDecoder.hpp"
#include "jutta_bt_proto/Utils.hpp"
#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

TEST_CASE("Empty", "[encDecBytes]") {
    const std::vector<uint8_t> data;
    uint8_t key = 42;
    const std::vector<uint8_t> result = bt::encDecBytes(data, key);
    REQUIRE(data.size() == result.size());
}

TEST_CASE("OneElement", "[encDecBytes]") {
    const std::vector<uint8_t> data{24};
    uint8_t key = 42;
    const std::vector<uint8_t> result = bt::encDecBytes(data, key);
    REQUIRE(data.size() == result.size());
    REQUIRE(result[0] == 66);
}

TEST_CASE("OneElementTest", "[encDecBytes]") {
    std::string hex = "77e93dd55381d3dba32bfa98a4a3faf9";
    std::vector<uint8_t> input = jutta_bt_proto::from_hex_string(hex);
    uint8_t key = 42;
    std::vector<uint8_t> result = bt::encDecBytes(input, key);
    std::string resultHex = jutta_bt_proto::to_hex_string(result);
    REQUIRE(input.size() == result.size());
}

TEST_CASE("ManyElements", "[encDecBytes]") {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(10, 1024);

    std::vector<uint8_t> data;
    for (size_t i = 0; i < static_cast<size_t>(dist(rng)); i++) {
        data.push_back(static_cast<uint8_t>(dist(rng)));
    }
    uint8_t key = static_cast<uint8_t>(dist(rng));
    const std::vector<uint8_t> result = bt::encDecBytes(data, key);
    REQUIRE(data.size() == result.size());
}

TEST_CASE("InverseOneElement", "[encDecBytes]") {
    const std::vector<uint8_t> data{24};
    uint8_t key = 42;
    const std::vector<uint8_t> result = bt::encDecBytes(bt::encDecBytes(data, key), key);
    REQUIRE(data.size() == result.size());
    REQUIRE(data[0] == result[0]);
}

TEST_CASE("InverseManyElements", "[encDecBytes]") {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(10, 1024);

    std::vector<uint8_t> data;
    for (size_t i = 0; i < static_cast<size_t>(dist(rng)); i++) {
        data.push_back(static_cast<uint8_t>(dist(rng)));
    }
    uint8_t key = static_cast<uint8_t>(dist(rng));
    const std::vector<uint8_t> result = bt::encDecBytes(bt::encDecBytes(data, key), key);
    REQUIRE(data.size() == result.size());
    for (size_t i = 0; i < data.size(); i++) {
        REQUIRE(data[i] == result[i]);
    }
}

TEST_CASE("InverseManyElementsRepeated", "[encDecBytes]") {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(10, 1024);

    for (size_t e = 0; e < 42; e++) {
        std::vector<uint8_t> data;
        for (size_t i = 0; i < static_cast<size_t>(dist(rng)); i++) {
            data.push_back(static_cast<uint8_t>(dist(rng)));
        }
        uint8_t key = static_cast<uint8_t>(dist(rng));
        const std::vector<uint8_t> result = bt::encDecBytes(bt::encDecBytes(data, key), key);
        REQUIRE(data.size() == result.size());
        for (size_t i = 0; i < data.size(); i++) {
            REQUIRE(data[i] == result[i]);
        }
    }
}

TEST_CASE("Uppercase", "[toFormHex]") {
    std::string s = "0123456789ABCDEF";
    const std::vector<uint8_t> tmp = jutta_bt_proto::from_hex_string(s);
    REQUIRE(tmp.size() == s.size() / 2);
    std::string result = jutta_bt_proto::to_hex_string(tmp);
    REQUIRE(result.size() == tmp.size() * 2);
    REQUIRE(result == s);
}

TEST_CASE("Lowercase", "[toFormHex]") {
    std::string s = "0123456789abcdef";
    const std::vector<uint8_t> tmp = jutta_bt_proto::from_hex_string(s);
    REQUIRE(tmp.size() == s.size() / 2);
    std::string result = jutta_bt_proto::to_hex_string(tmp);
    REQUIRE(result.size() == tmp.size() * 2);
    REQUIRE(result == "0123456789ABCDEF");
}
