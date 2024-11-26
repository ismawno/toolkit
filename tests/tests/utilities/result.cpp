#include "tkit/core/alias.hpp"
#include "tkit/utilities/result.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace TKit
{
struct Test
{
    Test(const f32 p_Value1, const u32 p_Value2) noexcept : Value1(p_Value1), Value2(p_Value2)
    {
    }

    Test() = default;

    f32 Value1;
    u32 Value2;
};
} // namespace TKit

TEST_CASE("Result", "[utilities][result]")
{
    SECTION("Ok")
    {
        auto result = TKit::Result<int>::Ok(42);
        REQUIRE(result);
        REQUIRE(result.Value() == 42);
    }

    SECTION("Error")
    {
        auto result = TKit::Result<int>::Error("Error");
        REQUIRE(!result);
        REQUIRE(std::string(result.Error()) == "Error");
    }

    SECTION("Ok")
    {
        auto result = TKit::Result<TKit::Test>::Ok(42.f, 42);
        REQUIRE(result);
        REQUIRE((result.Value().Value1 == 42.f && result.Value().Value2 == 42));
    }

    SECTION("Error")
    {
        auto result = TKit::Result<TKit::Test>::Error("Error");
        REQUIRE(!result);
        REQUIRE(std::string(result.Error()) == "Error");
    }
}