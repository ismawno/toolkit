#include "tkit/utils/alias.hpp"
#include "tkit/utils/result.hpp"
#include "tests/data_types.hpp"
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

TEST_CASE("Result", "[utilities][result]")
{
    SECTION("Ok (int)")
    {
        auto result = Result<u32>::Ok(42);
        REQUIRE(result);
        REQUIRE(result.GetValue() == 42);
    }

    SECTION("Error (int)")
    {
        auto result = Result<u32>::Error("Error");
        REQUIRE(!result);
        REQUIRE(std::string(result.GetError()) == "Error");
    }

    SECTION("Ok (struct)")
    {
        auto result = Result<Test>::Ok(42.f, 42);
        REQUIRE(result);
        REQUIRE((result.GetValue().Value1 == 42.f && result.GetValue().Value2 == 42));
    }

    SECTION("Error (struct)")
    {
        auto result = Result<Test>::Error("Error");
        REQUIRE(!result);
        REQUIRE(std::string(result.GetError()) == "Error");
    }

    SECTION("Memory correctness")
    {
        {
            auto result1 = Result<NonTrivialData>::Ok();
            REQUIRE(NonTrivialData::Instances == 1);

            const Result<NonTrivialData> other = result1;
            REQUIRE(NonTrivialData::Instances == 2);

            auto result2 = Result<NonTrivialData>::Error("Error");
            REQUIRE(std::string(result2.GetError()) == "Error");

            REQUIRE(NonTrivialData::Instances == 2);
        }
        REQUIRE(NonTrivialData::Instances == 0);
    }
}
} // namespace TKit
