#include "kit/container/hashable_tuple.hpp"
#include "kit/container/static_array.hpp"
#include <catch2/catch_test_macros.hpp>

namespace KIT
{
TEST_CASE("HashableTuple hash consistency")
{
    const HashableTuple tuple1(1, 2.0f, std::string("3"));
    const HashableTuple tuple2(1, 2.0f, std::string("3"));
    const HashableTuple tuple3(1, 2.0f, std::string("4"));

    REQUIRE(tuple1() == tuple2());
    REQUIRE(tuple1() != tuple3());
}

TEST_CASE("HashableTuple deviation")
{
    HashableTuple<u32, u32, f32, std::string> tuple;

    constexpr usize amount = 97;
    StaticArray<u32, amount> ocurrences(amount, 0);

    constexpr u32 samples = 100000;
    for (u32 i = 0; i < samples; ++i)
    {
        tuple.Get<0>() = i;
        ++ocurrences[tuple() % amount];
    }
    for (u32 i = 0; i < samples; ++i)
    {
        tuple.Get<1>() = i;
        ++ocurrences[tuple() % amount];
    }

    for (u32 i = 0; i < samples; ++i)
    {
        tuple.Get<2>() = static_cast<f32>(i);
        ++ocurrences[tuple() % amount];
    }
    for (u32 i = 0; i < samples; ++i)
    {
        tuple.Get<3>() = std::to_string(i);
        ++ocurrences[tuple() % amount];
    }

    const f32 expected = static_cast<f32>(4 * samples) / amount;
    f32 deviation = 0.0f;
    for (u32 i = 0; i < amount; ++i)
    {
        const f32 diff = static_cast<f32>(ocurrences[i]) - expected;
        deviation += diff * diff;
    }

    deviation = sqrtf(deviation / expected);
    KIT_LOG_INFO("HashableTuple deviation ({} samples with {} ocurrences): {}", samples, amount, deviation);
}
} // namespace KIT