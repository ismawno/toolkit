#include "tkit/utils/hash.hpp"
#include "tkit/container/static_array.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
struct Example
{
    u32 elm0 = 0;
    u32 elm1 = 0;
    f32 elm2 = 0.0f;
    std::string elm3 = "";
};
} // namespace TKit

namespace std
{
template <> struct hash<TKit::Example>
{
    size_t operator()(const TKit::Example &p_Example) const noexcept
    {
        return TKit::Hash(p_Example.elm0, p_Example.elm1, p_Example.elm2, p_Example.elm3);
    }
};
} // namespace std

namespace TKit
{
TEST_CASE("Hash consistency")
{
    const size_t hash1 = Hash(1, 2.f, std::string("3"));
    const size_t hash2 = Hash(1, 2.f, std::string("4"));

    REQUIRE(hash1 != hash2);
}

TEST_CASE("Hash deviation")
{
    constexpr usize amount = 97;
    StaticArray<u32, amount> ocurrences(amount, 0);

    Example ex;

    constexpr u32 samples = 100000;
    for (u32 i = 0; i < samples; ++i)
    {
        ex.elm0 = i;
        ++ocurrences[Hash(ex) % amount];
    }
    for (u32 i = 0; i < samples; ++i)
    {
        ex.elm1 = i;
        ++ocurrences[Hash(ex) % amount];
    }

    for (u32 i = 0; i < samples; ++i)
    {
        ex.elm2 = static_cast<f32>(i);
        ++ocurrences[Hash(ex) % amount];
    }
    for (u32 i = 0; i < samples; ++i)
    {
        ex.elm3 = std::to_string(i);
        ++ocurrences[Hash(ex) % amount];
    }

    const f32 expected = static_cast<f32>(4 * samples) / amount;
    f32 deviation = 0.0f;
    for (u32 i = 0; i < amount; ++i)
    {
        const f32 diff = static_cast<f32>(ocurrences[i]) - expected;
        deviation += diff * diff;
    }

    deviation = sqrtf(deviation / expected);
    TKIT_LOG_INFO("[TOOLKIT] Hash deviation ({} samples with {} ocurrences): {}", samples, amount, deviation);
}
} // namespace TKit