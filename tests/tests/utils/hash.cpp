#include "tkit/utils/hash.hpp"
#include "tkit/container/fixed_array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;

template <typename T> class Test
{
    T v1;
};

template <typename T>
concept Int = std::same_as<T, int>;

template <Int T> class Test<T>
{
    T v2;
};

TEST_CASE("Hash single values", "[Hash]")
{
    SECTION("integers")
    {
        const usz h1 = Hash(42);
        const usz h2 = std::hash<u32>()(42);
        REQUIRE(h1 == h2);
    }

    SECTION("strings")
    {
        const std::string s = "hello";
        const usz h1 = Hash(s);
        const usz h2 = std::hash<std::string>()(s);
        REQUIRE(h1 == h2);
    }

    SECTION("lvalue vs rvalue")
    {
        const u32 x = 7;
        const usz h1 = Hash(x);
        const usz h2 = Hash(7);
        REQUIRE(h1 == h2);
    }
}

TEST_CASE("Variadic Hash combines consistently", "[Hash][Variadic]")
{
    const u32 a = 1, b = 2, c = 3;
    const usz combined = Hash(a, b, c);

    // manual combination
    usz seed = TKIT_HASH_SEED;
    HashCombine(seed, a, b, c);
    REQUIRE(combined == seed);

    // order matters
    const usz different = Hash(b, a, c);
    REQUIRE(different != combined);
}

TEST_CASE("HashRange over iterators", "[HashRange]")
{
    const FixedArray<u32, 4> v{4, 5, 6, 7};
    const usz hr = HashRange(v.begin(), v.end());

    // manual equivalent
    usz seed = TKIT_HASH_SEED;
    for (u32 x : v)
        HashCombine(seed, x);
    REQUIRE(hr == seed);
}

TEST_CASE("HashCombine modifies seed", "[HashCombine]")
{
    usz seed = 12345;
    const usz before = seed;
    HashCombine(seed, 10, std::string("abc"));
    REQUIRE(seed != before);

    // combining zero values leaves seed unchanged
    usz s2 = 999;
    HashCombine(s2);
    REQUIRE(s2 == 999);
}

TEST_CASE("Mixed types hashing", "[Hash]")
{
    const char *cstr = "xyz";
    const std::string str = "xyz";
    // Hash should distinguish pointer vs string hash
    const usz hptr = Hash(cstr);
    const usz hstr = Hash(str);
    REQUIRE(hptr != hstr);

    // But combining same value twice differs from single
    const usz single = Hash(str);
    const usz doubled = Hash(str, str);
    REQUIRE(doubled != single);
}

TEST_CASE("Hash of different types in range", "[HashRange]")
{
    const FixedArray<std::string, 3> vs{"a", "b", "c"};
    const usz hr = HashRange(vs.begin(), vs.end());
    usz seed = TKIT_HASH_SEED;
    for (auto &s : vs)
        HashCombine(seed, s);
    REQUIRE(hr == seed);
}
