#include "tkit/math/math.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

using namespace TKit;
using namespace TKit::Alias;
using namespace TKit::Math;

TEST_CASE("Math utilities: Min, Max, Clamp, Radians, Degrees", "[Math]")
{
    REQUIRE(Min(3, 5) == 3);
    REQUIRE(Max(3, 5) == 5);
    REQUIRE(Clamp(10, 0, 5) == 5);
    REQUIRE(Clamp(-2, 0, 5) == 0);

    REQUIRE(Radians(180.0f) == Catch::Approx(3.14159f).margin(1e-4f));
    REQUIRE(Degrees(3.14159f) == Catch::Approx(180.0f).margin(0.1f));
}

TEST_CASE("Tensor-wise math utilities", "[Math][Tensor]")
{
    const u32v3 v(1, 4, 9);
    const u32v3 sq = SquareRoot(v);
    REQUIRE(sq[0] == Catch::Approx(1));
    REQUIRE(sq[1] == Catch::Approx(2));
    REQUIRE(sq[2] == Catch::Approx(3));

    const u32v3 vmin = Min(v, u32v3(2, 2, 2));
    REQUIRE(vmin[0] == 1);
    REQUIRE(vmin[1] == 2);
    REQUIRE(vmin[2] == 2);

    const u32v3 vmax = Max(v, u32v3(2, 2, 2));
    REQUIRE(vmax[0] == 2);
    REQUIRE(vmax[1] == 4);
    REQUIRE(vmax[2] == 9);

    const u32v3 vclamp = Clamp(v, 2, 4);
    REQUIRE(vclamp[0] == 2);
    REQUIRE(vclamp[1] == 4);
    REQUIRE(vclamp[2] == 4);
}
