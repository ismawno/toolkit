#include "kit/container/hashable_tuple.hpp"
#include "kit/container/static_array.hpp"
#include <catch2/catch_test_macros.hpp>

KIT_NAMESPACE_BEGIN

TEST_CASE("HashableTuple hash consistency")
{
    const HashableTuple tuple1(1, 2.0f, String("3"));
    const HashableTuple tuple2(1, 2.0f, String("3"));
    const HashableTuple tuple3(1, 2.0f, String("4"));

    REQUIRE(tuple1() == tuple2());
    REQUIRE(tuple1() != tuple3());
}

TEST_CASE("HashableTuple Chi Square")
{
    HashableTuple<uint32_t, uint32_t, float, String> tuple;

    constexpr size_t amount = 97;
    StaticArray<uint32_t, amount> ocurrences(amount, 0);

    constexpr uint32_t samples = 100000;
    for (uint32_t i = 0; i < samples; ++i)
    {
        tuple.Get<0>() = i;
        ++ocurrences[tuple() % amount];
    }
    for (uint32_t i = 0; i < samples; ++i)
    {
        tuple.Get<1>() = i;
        ++ocurrences[tuple() % amount];
    }

    for (uint32_t i = 0; i < samples; ++i)
    {
        tuple.Get<2>() = static_cast<float>(i);
        ++ocurrences[tuple() % amount];
    }
    for (uint32_t i = 0; i < samples; ++i)
    {
        tuple.Get<3>() = std::to_string(i);
        ++ocurrences[tuple() % amount];
    }

    const float expected = static_cast<float>(4 * samples) / amount;
    float deviation = 0.0f;
    for (uint32_t i = 0; i < amount; ++i)
    {
        const float diff = static_cast<float>(ocurrences[i]) - expected;
        deviation += diff * diff;
    }

    deviation = sqrtf(deviation / expected);
    KIT_LOG_INFO("HashableTuple deviation ({} samples with {} ocurrences): {}", samples, amount, deviation);
}

KIT_NAMESPACE_END