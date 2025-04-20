#include "tkit/utils/hash.hpp"
#include "tkit/container/array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;

TEST_CASE("Hash single values", "[Hash]")
{
    SECTION("integers")
    {
        size_t h1 = Hash(42);
        size_t h2 = std::hash<u32>()(42);
        REQUIRE(h1 == h2);
    }

    SECTION("strings")
    {
        std::string s = "hello";
        size_t h1 = Hash(s);
        size_t h2 = std::hash<std::string>()(s);
        REQUIRE(h1 == h2);
    }

    SECTION("lvalue vs rvalue")
    {
        u32 x = 7;
        size_t h1 = Hash(x);
        size_t h2 = Hash(7);
        REQUIRE(h1 == h2);
    }
}

TEST_CASE("Variadic Hash combines consistently", "[Hash][Variadic]")
{
    u32 a = 1, b = 2, c = 3;
    size_t combined = Hash(a, b, c);

    // manual combination
    size_t seed = TKIT_HASH_SEED;
    HashCombine(seed, a, b, c);
    REQUIRE(combined == seed);

    // order matters
    size_t different = Hash(b, a, c);
    REQUIRE(different != combined);
}

TEST_CASE("HashRange over iterators", "[HashRange]")
{
    Array<u32, 4> v{4, 5, 6, 7};
    size_t hr = HashRange(v.begin(), v.end());

    // manual equivalent
    size_t seed = TKIT_HASH_SEED;
    for (u32 x : v)
        HashCombine(seed, x);
    REQUIRE(hr == seed);
}

TEST_CASE("HashCombine modifies seed", "[HashCombine]")
{
    size_t seed = 12345;
    size_t before = seed;
    HashCombine(seed, 10, std::string("abc"));
    REQUIRE(seed != before);

    // combining zero values leaves seed unchanged
    size_t s2 = 999;
    HashCombine(s2);
    REQUIRE(s2 == 999);
}

TEST_CASE("Mixed types hashing", "[Hash]")
{
    const char *cstr = "xyz";
    std::string str = "xyz";
    // Hash should distinguish pointer vs string hash
    size_t hptr = Hash(cstr);
    size_t hstr = Hash(str);
    REQUIRE(hptr != hstr);

    // But combining same value twice differs from single
    size_t single = Hash(str);
    size_t doubled = Hash(str, str);
    REQUIRE(doubled != single);
}

TEST_CASE("Hash of different types in range", "[HashRange]")
{
    Array<std::string, 3> vs{"a", "b", "c"};
    size_t hr = HashRange(vs.begin(), vs.end());
    size_t seed = TKIT_HASH_SEED;
    for (auto &s : vs)
        HashCombine(seed, s);
    REQUIRE(hr == seed);
}
