#pragma once

#include "tkit/utils/alias.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <array>

namespace TKit
{
template <typename Wide, usize N, usize Lanes> void TestGatherScatter()
{
    using T = typename Wide::ValueType;
    std::array<T, N> scattered[Lanes];

    for (usize i = 0; i < Lanes; ++i)
        for (usize j = 0; j < N; ++j)
            scattered[i][j] = i * N + j;

    const auto w = Wide::template Gather<N>(&scattered[0][0]);
    for (usize i = 0; i < Lanes; ++i)
        for (usize j = 0; j < N; ++j)
            REQUIRE(w[j][i] == scattered[i][j]);

    std::array<T, N> recovered[Lanes];
    Wide::template Scatter<N>(&recovered[0][0], w);
    for (usize i = 0; i < Lanes; ++i)
        for (usize j = 0; j < N; ++j)
            REQUIRE(recovered[i][j] == scattered[i][j]);
}
template <typename Wide> void RunWideTests()
{
    using T = typename Wide::ValueType;
    using SizeType = typename Wide::SizeType;
    using Mask = typename Wide::Mask;
    using BitMask = typename Wide::BitMask;

    constexpr SizeType Lanes = Wide::Lanes;
    constexpr SizeType Alignment = Wide::Alignment;

    alignas(Alignment) T src[Lanes];
    for (SizeType i = 0; i < Lanes; ++i)
        src[i] = i + 1;

    SECTION("Construction from pointer")
    {
        Wide w{src};
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(w[i] == src[i]);
    }

    SECTION("Construction from scalar")
    {
        const T val = static_cast<T>(3.5);
        Wide w{val};
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(w[i] == val);
    }

    SECTION("Construction from callable")
    {
        Wide w{[](const SizeType p_Index) { return p_Index * 2; }};
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(w[i] == static_cast<T>(i * 2));
    }

    SECTION("Element access and At()")
    {
        Wide w{src};
        for (SizeType i = 0; i < Lanes; ++i)
        {
            REQUIRE(w[i] == src[i]);
            REQUIRE(w.At(i) == src[i]);
        }
    }

    SECTION("Store()")
    {
        Wide w{src};
        alignas(Alignment) T dst1[Lanes]{};
        w.StoreAligned(dst1);
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(dst1[i] == src[i]);

        T dst2[Lanes]{};
        w.StoreUnaligned(dst2);
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(dst2[i] == src[i]);
    }

    struct Spread
    {
        T Relevant;
        u8 Garbage1;
        u16 Garbage2;
    };

    SECTION("Gather()")
    {
        Spread spread[Lanes];
        for (SizeType i = 0; i < Lanes; ++i)
            spread[i] = {static_cast<T>(i + 1), static_cast<u8>(i + 100), static_cast<u16>(i + 200)};

        const Wide w = Wide::Gather(&spread[0].Relevant, sizeof(Spread));
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(w[i] == spread[i].Relevant);
    }

    SECTION("Scatter()")
    {
        const Wide w{[](const SizeType p_Index) { return p_Index + 1; }};

        Spread spread[Lanes];
        w.Scatter(&spread[0].Relevant, sizeof(Spread));
        for (SizeType i = 0; i < Lanes; ++i)
            REQUIRE(w[i] == spread[i].Relevant);
    }

    SECTION("Gather() and Scatter() uniform")
    {
        TestGatherScatter<Wide, 2, Lanes>();
        TestGatherScatter<Wide, 3, Lanes>();
        TestGatherScatter<Wide, 4, Lanes>();
    }

    SECTION("Arithmetic operators")
    {
        const Wide a{[](const SizeType p_Index) { return p_Index + 1; }};
        const Wide b{[](const SizeType p_Index) { return p_Index + 2; }};

        const Wide add = a + b;
        const Wide sub = b - a;
        const Wide mul = a * b;
        const Wide div = b / a;
        const Wide neg = -a;

        for (SizeType i = 0; i < Lanes; ++i)
        {
            REQUIRE(add[i] == a[i] + b[i]);
            REQUIRE(sub[i] == b[i] - a[i]);
            REQUIRE(mul[i] == static_cast<T>(a[i] * b[i]));
            REQUIRE(div[i] == Catch::Approx(b[i] / a[i]));
            REQUIRE(neg[i] == static_cast<T>(-a[i]));
        }
    }
    SECTION("Scalar operators")
    {
        const Wide a{[](const SizeType p_Index) { return p_Index + 1; }};
        const T b{static_cast<T>(10)};

        const Wide add = a + b;
        const Wide sub = b - a;
        const Wide mul = a * b;
        const Wide div = b / a;

        for (SizeType i = 0; i < Lanes; ++i)
        {
            REQUIRE(add[i] == a[i] + b);
            REQUIRE(sub[i] == static_cast<T>(b - a[i]));
            REQUIRE(mul[i] == static_cast<T>(a[i] * b));
            REQUIRE(div[i] == Catch::Approx(b / a[i]));
        }
    }

    SECTION("Comparison operators")
    {
        const Wide a{[](const SizeType p_Index) { return p_Index; }};
        const Wide b{[](const SizeType p_Index) { return p_Index + 1; }};

        const BitMask eq = Wide::PackMask(a == a);
        const BitMask ne = Wide::PackMask(a != b);
        const BitMask lt = Wide::PackMask(a < b);
        const BitMask gt = Wide::PackMask(b > a);
        const BitMask le = Wide::PackMask(a <= b);
        const BitMask ge = Wide::PackMask(b >= a);

        for (SizeType i = 0; i < Lanes; ++i)
        {
            REQUIRE(((eq >> i) & 1u) == 1u);
            REQUIRE(((ne >> i) & 1u) == 1u);
            REQUIRE(((lt >> i) & 1u) == 1u);
            REQUIRE(((gt >> i) & 1u) == 1u);
            REQUIRE(((le >> i) & 1u) == 1u);
            REQUIRE(((ge >> i) & 1u) == 1u);
        }
    }

    SECTION("Min and Max")
    {
        const Wide a{[](const SizeType p_Index) { return p_Index + 2; }};
        const Wide b{[](const SizeType p_Index) { return p_Index + 3; }};

        const Wide mn = Wide::Min(a, b);
        const Wide mx = Wide::Max(a, b);

        for (SizeType i = 0; i < Lanes; ++i)
        {
            REQUIRE(mn[i] == std::min(a[i], b[i]));
            REQUIRE(mx[i] == std::max(a[i], b[i]));
        }
    }

    SECTION("Select()")
    {
        const Wide a{[](const SizeType p_Index) { return p_Index + 1; }};
        const Wide b{[](const SizeType p_Index) { return p_Index + 100; }};

        const BitMask bmask = static_cast<BitMask>(0b01010101010101);
        const Mask mask = Wide::WidenMask(bmask);
        const Wide sel = Wide::Select(a, b, mask);

        for (SizeType i = 0; i < Lanes; ++i)
        {
            const bool chooseA = (bmask >> i) & 1u;
            REQUIRE(sel[i] == (chooseA ? a[i] : b[i]));
        }
    }

    SECTION("Reduce()")
    {
        const Wide w{[](const SizeType p_Index) { return p_Index + 1; }};
        T expected = static_cast<T>(0);
        for (SizeType i = 0; i < Lanes; ++i)
            expected += w[i];
        const T result = Wide::Reduce(w);
        REQUIRE(result == Catch::Approx(expected));
    }
    if constexpr (std::is_integral_v<T>)
    {
        SECTION("Bit shift operators")
        {
            const Wide base{[](const SizeType p_Index) { return 255 - p_Index; }};

            const T shiftLeft = 3;
            const Wide shiftedLeft = base << shiftLeft;

            for (SizeType i = 0; i < Lanes; ++i)
                REQUIRE(shiftedLeft[i] == static_cast<T>(base[i] << shiftLeft));

            const T shiftRight = 7;
            const Wide shiftedRight = base >> shiftRight;

            for (SizeType i = 0; i < Lanes; ++i)
                REQUIRE(shiftedRight[i] == static_cast<T>(base[i] >> shiftRight));
        }
        SECTION("Bitwise AND and OR operators")
        {
            const Wide a{[](const SizeType p_Index) { return static_cast<T>((p_Index % 2) ? 0xFF : 0x0F); }};
            const Wide b{[](const SizeType p_Index) { return static_cast<T>((p_Index % 2) ? 0xF0 : 0x0F); }};

            const Wide band = a & b;
            const Wide bor = a | b;

            for (SizeType i = 0; i < Lanes; ++i)
            {
                REQUIRE(band[i] == static_cast<T>(a[i] & b[i]));
                REQUIRE(bor[i] == static_cast<T>(a[i] | b[i]));
            }
        }
    }
}

} // namespace TKit
