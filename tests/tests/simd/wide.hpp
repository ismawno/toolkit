#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

namespace TKit
{
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
        const Wide sel = Wide::Select(mask, a, b);

        for (SizeType i = 0; i < Lanes; ++i)
        {
            const bool chooseA = (mask >> i) & 1u;
            REQUIRE(sel[i] == (chooseA ? a[i] : b[i]));
        }
    }

    SECTION("Reduce()")
    {
        const Wide w{[](const SizeType p_Index) { return p_Index + 1; }};
        const T expected = static_cast<T>((Lanes * (Lanes + 1)) / 2); // sum(1..Lanes)
        REQUIRE(Wide::Reduce(w) == Catch::Approx(expected));
    }
}

} // namespace TKit
