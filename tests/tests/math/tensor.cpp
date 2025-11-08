#include "tkit/math/tensor.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

using namespace TKit;
using namespace TKit::Alias;
using namespace TKit::Math;

TEST_CASE("Tensor basic construction and indexing", "[Tensor]")
{
    const u32v3 v1(1, 2, 3);

    REQUIRE(v1.Length == 3);
    REQUIRE(v1.Rank == 1);
    REQUIRE(v1[0] == 1);
    REQUIRE(v1[1] == 2);
    REQUIRE(v1[2] == 3);

    // Fill constructor
    const u32v3 v2(5);
    REQUIRE(v2[0] == 5);
    REQUIRE(v2[1] == 5);
    REQUIRE(v2[2] == 5);
}

TEST_CASE("Tensor arithmetic operations", "[Tensor]")
{
    const i32v3 a(1, 2, 3);
    const i32v3 b(3, 2, 1);

    const i32v3 sum = a + b;
    const i32v3 diff = a - b;
    const i32v3 scaled = a * 2;
    const i32v3 divided = b / 2;

    REQUIRE(sum[0] == 4);
    REQUIRE(sum[1] == 4);
    REQUIRE(sum[2] == 4);

    REQUIRE(diff[0] == -2);
    REQUIRE(diff[1] == 0);
    REQUIRE(diff[2] == 2);

    REQUIRE(scaled[0] == 2);
    REQUIRE(scaled[1] == 4);
    REQUIRE(scaled[2] == 6);

    REQUIRE(divided[0] == 1);
    REQUIRE(divided[1] == 1);
    REQUIRE(divided[2] == 0);
}

TEST_CASE("Tensor dot, norm, and distance", "[Tensor]")
{
    const f32v3 v1(1.0f, 2.0f, 3.0f);
    const f32v3 v2(4.0f, -5.0f, 6.0f);

    const f32 dot = Dot(v1, v2);
    REQUIRE(dot == Catch::Approx(1.0f * 4.0f + 2.0f * -5.0f + 3.0f * 6.0f));

    const f32 norm = Norm(v1);
    REQUIRE(norm == Catch::Approx(std::sqrt(14.0f)));

    const f32 dist = Distance(v1, v2);
    REQUIRE(dist == Catch::Approx(std::sqrt((4 - 1) * (4 - 1) + (-5 - 2) * (-5 - 2) + (6 - 3) * (6 - 3))));
}

TEST_CASE("Tensor normalization", "[Tensor]")
{
    const f32v3 v(3.0f, 4.0f, 0.0f);
    const f32v3 normed = Normalize(v);

    float length = Norm(normed);
    REQUIRE(length == Catch::Approx(1.0f));
}

TEST_CASE("Matrix determinant", "[Tensor][Matrix]")
{
    const f32m2 m2(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(Determinant(m2) == Catch::Approx(-2.0f));

    const f32m3 m3(1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 4.0f, 5.0f, 6.0f, 0.0f);
    REQUIRE(Determinant(m3) == Catch::Approx(1.0f * (1 * 0 - 4 * 6) - 2.0f * (0 * 0 - 4 * 5) + 3.0f * (0 * 6 - 1 * 5)));
}

TEST_CASE("Matrix transpose", "[Tensor][Matrix]")
{
    const u32m2x4 m(1, 2, 3, 4, 5, 6, 7, 8);
    const u32m4x2 mt = Transpose(m);
    for (usize i = 0; i < 2; ++i)
        for (usize j = 0; j < 4; ++j)
            REQUIRE(mt[j][i] == m[i][j]);
}

TEST_CASE("Matrix multiplication", "[Tensor][Matrix]")
{
    const u32m2 a(1, 2, 3, 4);
    const u32m2 b(2, 0, 1, 2);

    const u32t<2, 2, 2> t1{a, a};
    const u32t<2, 2, 2> t2{b, b};

    const u32t<2, 2, 2> c = t1 * t2;
    for (usize i = 0; i < 2; ++i)
    {
        REQUIRE(c[i][0][0] == 1 * 2 + 3 * 0);
        REQUIRE(c[i][1][0] == 1 * 1 + 3 * 2);
        REQUIRE(c[i][0][1] == 2 * 2 + 4 * 0);
        REQUIRE(c[i][1][1] == 2 * 1 + 4 * 2);
    }
}

TEST_CASE("Tensor reshape and data access", "[Tensor]")
{
    const u32v4 v(1, 2, 3, 4);

    const u32m2 m2 = Reshape<2, 2>(v);

    REQUIRE(m2[0][0] == 1);
    REQUIRE(m2[0][1] == 2);
    REQUIRE(m2[1][0] == 3);
    REQUIRE(m2[1][1] == 4);

    const u32 *data = v.GetData();
    REQUIRE(data[2] == 3);
}

TEST_CASE("High-order matrix transpose and double-transpose identity", "[Tensor][Matrix]")
{
    const u32t<5, 5> a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25);

    const auto tr = Transpose(a);
    const auto antitr = Transpose(tr);

    for (usize i = 0; i < 5; ++i)
        for (usize j = 0; j < 5; ++j)
            REQUIRE(antitr[i][j] == Catch::Approx(a[i][j]));

    REQUIRE(tr[0][1] == a[1][0]);
    REQUIRE(tr[4][2] == a[2][4]);
}

// TEST_CASE("Tensor permutation (axes swap)", "[Tensor][Permutation]")
// {
//     const u32t<2, 3> m(1, 2, 3, 4, 5, 6);
//
//     const u32t<3, 2> permuted = Permute<1, 0>(m);
//
//     for (usize i = 0; i < 2; ++i)
//         for (usize j = 0; j < 3; ++j)
//             REQUIRE(m[i][j] == permuted[j][i]);
// }

TEST_CASE("Tensor permutation with higher-rank tensors", "[Tensor][Permutation]")
{
    const u32t<2, 3, 4> t(
        // slice 0
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        // slice 1
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24);

    SECTION("Swap first two axes")
    {
        const u32t<4, 3, 2> perm = Permute<2, 1, 0>(t);
        for (usize i = 0; i < 2; ++i)
            for (usize j = 0; j < 3; ++j)
                for (usize k = 0; k < 4; ++k)
                    REQUIRE(t[i][j][k] == perm[k][j][i]);
    }

    SECTION("Rotate axes forward")
    {
        const u32t<3, 4, 2> perm = Permute<1, 2, 0>(t);
        for (usize i = 0; i < 2; ++i)
            for (usize j = 0; j < 3; ++j)
                for (usize k = 0; k < 4; ++k)
                    REQUIRE(t[i][j][k] == perm[j][k][i]);
    }

    SECTION("Double permutation restores original tensor")
    {
        const u32t<4, 2, 3> perm1 = Permute<2, 0, 1>(t);
        const u32t<2, 3, 4> perm2 = Permute<1, 2, 0>(perm1);

        for (usize i = 0; i < 2; ++i)
            for (usize j = 0; j < 3; ++j)
                for (usize k = 0; k < 4; ++k)
                {
                    REQUIRE(t[i][j][k] == perm1[k][i][j]);
                    REQUIRE(t[i][j][k] == perm2[i][j][k]);
                }
    }
}

TEST_CASE("Tensor SubTensor extraction", "[Tensor][SubTensor]")
{
    const i32t<3, 3> m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // Remove row 1, column 1 → expected 2×2 minor:
    // [ [1, 3],
    //   [7, 9] ]
    const i32t<2, 2> minor1 = SubTensor(m, 1, 1);
    const i32t<2, 2> minor2 = SubTensor(m, 0, 1);

    REQUIRE(minor1[0][0] == 1);
    REQUIRE(minor1[0][1] == 3);
    REQUIRE(minor1[1][0] == 7);
    REQUIRE(minor1[1][1] == 9);

    REQUIRE(minor2[0][0] == 4);
    REQUIRE(minor2[0][1] == 6);
    REQUIRE(minor2[1][0] == 7);
    REQUIRE(minor2[1][1] == 9);
}

TEST_CASE("High-order square tensor determinants", "[Tensor][Matrix]")
{
    SECTION("5x5 identity determinant")
    {
        const u32t<5, 5> ident(1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

        REQUIRE(Determinant(ident) == 1);
    }

    SECTION("5x5 diagonal determinant")
    {
        const u32t<5, 5> det(2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 6);
        REQUIRE(Determinant(det) == 2 * 3 * 4 * 5 * 6);
    }

    SECTION("6x6 upper-triangular determinant equals diagonal product")
    {
        const f32t<6, 6> upper(1, 2, 3, 4, 5, 6, 0, 2, 3, 4, 5, 6, 0, 0, 3, 4, 5, 6, 0, 0, 0, 4, 5, 6, 0, 0, 0, 0, 5, 6,
                               0, 0, 0, 0, 0, 6);

        REQUIRE(Determinant(upper) == 1 * 2 * 3 * 4 * 5 * 6);
    }
}

TEST_CASE("6x6 matrix transpose and determinant sanity", "[Tensor][Matrix]")
{
    const u32t<6, 6> m(1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 0, 0, 1, 2, 3, 4, 0, 0, 0, 1, 2, 3, 0, 0, 0, 0, 1, 2, 0, 0,
                       0, 0, 0, 1);
    const u32t<6, 6> mt = Transpose(m);
    REQUIRE(mt[0][1] == 0);
    REQUIRE(mt[1][0] == 2);
    REQUIRE(mt[5][4] == 2);

    // REQUIRE(Determinant(m) == 1);
}
