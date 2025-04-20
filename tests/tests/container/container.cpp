#include "tkit/container/container.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <algorithm>
#include <cstring>
#include <utility>

using namespace TKit::Container;
using namespace TKit::Alias;

template <typename T> struct RawArrayTraits
{
    using ValueType = T;
    using SizeType = usize;
    using Iterator = T *;
    using ConstIterator = const T *;
};

template <typename T> using Tools = ArrayTools<RawArrayTraits<T>>;

struct CopyOnly
{
    u32 Value;
    CopyOnly() : Value(0)
    {
    }
    CopyOnly(u32 p_Value) : Value(p_Value)
    {
    }
    CopyOnly(const CopyOnly &o) : Value(o.Value)
    {
    }
    CopyOnly &operator=(const CopyOnly &o)
    {
        Value = o.Value;
        return *this;
    }
};

struct MoveOnly
{
    u32 Value;
    MoveOnly() : Value(0)
    {
    }
    MoveOnly(u32 v) : Value(v)
    {
    }
    MoveOnly(const MoveOnly &) = delete;
    MoveOnly &operator=(const MoveOnly &) = delete;
    MoveOnly(MoveOnly &&o) noexcept : Value(o.Value)
    {
        o.Value = Limits<u32>::max();
    }
    MoveOnly &operator=(MoveOnly &&o) noexcept
    {
        Value = o.Value;
        o.Value = Limits<u32>::max();
        return *this;
    }
};

TEST_CASE("CopyConstructFromRange", "[CopyConstructFromRange]")
{
    SECTION("trivial type")
    {
        u32 src[] = {1, 2, 3, 4, 5};
        u32 dst[5];
        Tools<u32>::CopyConstructFromRange(dst, src, src + 5);
        REQUIRE(std::equal(dst, dst + 5, src));
    }

    SECTION("non-trivial copyable type")
    {
        CopyOnly src[] = {10, 20, 30};
        // raw uninitialized storage for 3 CopyOnly
        alignas(CopyOnly) std::byte storage[sizeof(CopyOnly) * 3];
        auto dst = reinterpret_cast<CopyOnly *>(storage);

        Tools<CopyOnly>::CopyConstructFromRange(dst, src, src + 3);
        for (u32 i = 0; i < 3; ++i)
            REQUIRE(dst[i].Value == src[i].Value);

        // manually destroy
        for (u32 i = 0; i < 3; ++i)
            dst[i].~CopyOnly();
    }
}

TEST_CASE("MoveConstructFromRange", "[MoveConstructFromRange]")
{
    SECTION("trivial type")
    {
        u32 src[] = {10, 20, 30};
        u32 dst[3] = {0, 0, 0};
        Tools<u32>::MoveConstructFromRange(dst, src, src + 3);

        // dest got copied
        REQUIRE(std::equal(dst, dst + 3, src));
        // trivial move leaves source unchanged
        REQUIRE(src[1] == 20);
    }

    SECTION("move-only type")
    {
        MoveOnly src[] = {MoveOnly(7), MoveOnly(14), MoveOnly(21)};
        alignas(MoveOnly) std::byte storage[sizeof(MoveOnly) * 3];
        auto dst = reinterpret_cast<MoveOnly *>(storage);

        Tools<MoveOnly>::MoveConstructFromRange(dst, src, src + 3);

        // dest values correct
        REQUIRE(dst[0].Value == 7);
        REQUIRE(dst[1].Value == 14);
        REQUIRE(dst[2].Value == 21);
        // moved‑from source should be Limits<u32>::max()
        REQUIRE(src[0].Value == Limits<u32>::max());
        REQUIRE(src[1].Value == Limits<u32>::max());
        REQUIRE(src[2].Value == Limits<u32>::max());

        for (u32 i = 0; i < 3; ++i)
            dst[i].~MoveOnly();
    }
}

TEST_CASE("CopyAssignFromRange", "[CopyAssignFromRange]")
{
    SECTION("trivial: src < dst")
    {
        u32 dst[5] = {1, 2, 3, 4, 5};
        u32 src[3] = {9, 8, 7};
        Tools<u32>::CopyAssignFromRange(dst, dst + 5, src, src + 3);
        // first 3 replaced
        REQUIRE(dst[0] == 9);
        REQUIRE(dst[1] == 8);
        REQUIRE(dst[2] == 7);
        // rest untouched
        REQUIRE(dst[3] == 4);
        REQUIRE(dst[4] == 5);
    }

    SECTION("trivial: src == dst")
    {
        u32 dst[4] = {1, 2, 3, 4};
        u32 src[4] = {5, 6, 7, 8};
        Tools<u32>::CopyAssignFromRange(dst, dst + 4, src, src + 4);
        REQUIRE(std::equal(dst, dst + 4, src));
    }

    SECTION("non-trivial copyable: src > dst")
    {
        // use std::array with extra capacity
        std::array<CopyOnly, 10> buf;
        // initialize first 3 slots
        for (u32 i = 0; i < 3; ++i)
            buf[i] = CopyOnly(i + 1);
        CopyOnly src[] = {100, 200, 300, 400, 500};

        // assign into 3‑slot region, but src is length 5 -> will construct 2 more
        Tools<CopyOnly>::CopyAssignFromRange(buf.data(), buf.data() + 3, src, src + 5);

        // now buf[0..4] == src[0..4]
        for (u32 i = 0; i < 5; ++i)
            REQUIRE(buf[i].Value == src[i].Value);
    }
}

TEST_CASE("MoveAssignFromRange", "[MoveAssignFromRange]")
{
    SECTION("trivial: src < dst")
    {
        u32 dst[4] = {5, 6, 7, 8};
        u32 src[2] = {1, 2};
        Tools<u32>::MoveAssignFromRange(dst, dst + 4, src, src + 2);
        REQUIRE(dst[0] == 1);
        REQUIRE(dst[1] == 2);
        REQUIRE(dst[2] == 7);
        REQUIRE(dst[3] == 8);
    }

    SECTION("trivial: src == dst")
    {
        u32 dst[3] = {9, 8, 7};
        u32 src[3] = {3, 2, 1};
        Tools<u32>::MoveAssignFromRange(dst, dst + 3, src, src + 3);
        REQUIRE(std::equal(dst, dst + 3, src));
    }

    SECTION("move-only: src > dst")
    {
        std::array<MoveOnly, 8> buf;
        // default‑construct first 2 slots in raw storage
        TKit::Memory::Construct<MoveOnly>(&buf[0], 0);
        TKit::Memory::Construct<MoveOnly>(&buf[1], 0);

        MoveOnly src[] = {11, 22, 33, 44};
        // move‑assign into 2‑slot region -> will construct 2 more at buf[2],buf[3]
        Tools<MoveOnly>::MoveAssignFromRange(buf.data(), buf.data() + 2, src, src + 4);

        REQUIRE(buf[0].Value == 11);
        REQUIRE(buf[1].Value == 22);
        REQUIRE(buf[2].Value == 33);
        REQUIRE(buf[3].Value == 44);
        // moved‑from
        REQUIRE(src[0].Value == Limits<u32>::max());
        REQUIRE(src[1].Value == Limits<u32>::max());
        REQUIRE(src[2].Value == Limits<u32>::max());
        REQUIRE(src[3].Value == Limits<u32>::max());
    }
}

TEST_CASE("Insert single element", "[Insert]")
{
    SECTION("trivial at beginning")
    {
        std::array<u32, 5> arr = {1, 2, 3, 0, 0};
        u32 currSize = 3;
        Tools<u32>::Insert(arr.begin() + currSize, arr.begin(), 99);
        // now [99,1,2,3,...]
        REQUIRE(arr[0] == 99);
        REQUIRE(arr[1] == 1);
        REQUIRE(arr[2] == 2);
        REQUIRE(arr[3] == 3);
    }

    SECTION("trivial at middle")
    {
        std::array<u32, 5> arr = {1, 2, 3, 0, 0};
        u32 currSize = 3;
        auto pos = arr.begin() + 1;
        Tools<u32>::Insert(arr.begin() + currSize, pos, 42);
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 42);
        REQUIRE(arr[2] == 2);
        REQUIRE(arr[3] == 3);
    }

    SECTION("trivial at end")
    {
        std::array<u32, 4> arr = {10, 20, 0, 0};
        u32 currSize = 2;
        Tools<u32>::Insert(arr.begin() + currSize, arr.begin() + currSize, 30);
        REQUIRE(arr[2] == 30);
    }

    SECTION("move-only in raw buffer")
    {
        // raw buffer for 3 MoveOnly
        alignas(MoveOnly) std::byte storage[sizeof(MoveOnly) * 3];
        auto arr = reinterpret_cast<MoveOnly *>(storage);
        // placement‐new first two
        TKit::Memory::Construct<MoveOnly>(&arr[0], 5);
        TKit::Memory::Construct<MoveOnly>(&arr[1], 6);

        MoveOnly val(7);
        // insert at position 1
        Tools<MoveOnly>::Insert(arr + 2, arr + 1, std::move(val));

        REQUIRE(arr[1].Value == 7);
        REQUIRE(arr[2].Value == 6);

        // destroy all three
        for (u32 i = 0; i < 3; ++i)
            arr[i].~MoveOnly();
    }
}

TEST_CASE("Insert range of elements", "[Insert][Range]")
{
    SECTION("trivial: tail > count")
    {
        std::array<u32, 8> arr = {1, 2, 3, 4, 0, 0, 0, 0};
        u32 size = 4;
        u32 src[] = {10, 20};
        auto added = Tools<u32>::Insert(arr.begin() + size, arr.begin() + 1, src, src + 2);
        REQUIRE(added == 2);
        // arr => 1,10,20,2,3,4
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 10);
        REQUIRE(arr[2] == 20);
        REQUIRE(arr[3] == 2);
        REQUIRE(arr[4] == 3);
        REQUIRE(arr[5] == 4);
    }

    SECTION("trivial: tail < count")
    {
        std::array<u32, 8> arr = {1, 2, 3, 0, 0, 0, 0, 0};
        u32 size = 3;
        u32 src[] = {5, 6, 7, 8, 9};
        auto added = Tools<u32>::Insert(arr.begin() + size, arr.begin() + 1, src, src + 5);
        REQUIRE(added == 5);
        // arr => 1,5,6,7,8,9,2,3
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 5);
        REQUIRE(arr[2] == 6);
        REQUIRE(arr[3] == 7);
        REQUIRE(arr[4] == 8);
        REQUIRE(arr[5] == 9);
        REQUIRE(arr[6] == 2);
        REQUIRE(arr[7] == 3);
    }

    SECTION("trivial: tail == count")
    {
        std::array<u32, 6> arr = {1, 2, 3, 0, 0, 0};
        u32 size = 3;
        u32 src[] = {7, 8, 9};
        auto added = Tools<u32>::Insert(arr.begin() + size, arr.begin() + 3, src, src + 3);
        REQUIRE(added == 3);
        REQUIRE(arr[3] == 7);
        REQUIRE(arr[4] == 8);
        REQUIRE(arr[5] == 9);
    }

    SECTION("non-trivial copyable tail > count")
    {
        std::array<CopyOnly, 8> arr;
        // init first 4 slots
        for (u32 i = 0; i < 4; ++i)
            arr[i] = CopyOnly(i + 1);
        CopyOnly src[] = {100, 200};
        auto added = Tools<CopyOnly>::Insert(arr.data() + 4, arr.data() + 1, src, src + 2);
        REQUIRE(added == 2);
        REQUIRE(arr[1].Value == 100);
        REQUIRE(arr[2].Value == 200);
        REQUIRE(arr[3].Value == 2);
        REQUIRE(arr[4].Value == 3);
    }

    SECTION("non-trivial copyable tail < count")
    {
        std::array<CopyOnly, 8> arr;
        // init first 3 slots
        for (u32 i = 0; i < 3; ++i)
            arr[i] = CopyOnly(i + 1);
        CopyOnly src[] = {100, 200, 300, 400, 500};
        auto added = Tools<CopyOnly>::Insert(arr.data() + 3, arr.data() + 1, src, src + 5);
        REQUIRE(added == 5);
        REQUIRE(arr[0].Value == 1);
        REQUIRE(arr[1].Value == 100);
        REQUIRE(arr[2].Value == 200);
        REQUIRE(arr[3].Value == 300);
        REQUIRE(arr[4].Value == 400);
        REQUIRE(arr[5].Value == 500);
        REQUIRE(arr[6].Value == 2);
        REQUIRE(arr[7].Value == 3);
    }
    SECTION("non-trivial copyable tail == count")
    {
        std::array<CopyOnly, 6> arr;
        // init first 3 slots
        for (u32 i = 0; i < 3; ++i)
            arr[i] = CopyOnly(i + 1);
        CopyOnly src[] = {100, 200, 300};
        auto added = Tools<CopyOnly>::Insert(arr.data() + 3, arr.data() + 1, src, src + 3);
        REQUIRE(added == 3);
        REQUIRE(arr[0].Value == 1);
        REQUIRE(arr[1].Value == 100);
        REQUIRE(arr[2].Value == 200);
        REQUIRE(arr[3].Value == 300);
        REQUIRE(arr[4].Value == 2);
        REQUIRE(arr[5].Value == 3);
    }
}

TEST_CASE("RemoveOrdered single element", "[RemoveOrdered]")
{
    SECTION("trivial type")
    {
        std::array<u32, 5> arr = {1, 2, 3, 4, 5};
        u32 size = 5;
        Tools<u32>::RemoveOrdered(arr.begin() + size, arr.begin() + 1);
        // arr => 1,3,4,5,...
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 3);
        REQUIRE(arr[2] == 4);
        REQUIRE(arr[3] == 5);
    }

    SECTION("non-trivial copyable")
    {
        std::array<CopyOnly, 5> arr;
        for (u32 i = 0; i < 5; ++i)
            arr[i] = CopyOnly(i + 1);
        Tools<CopyOnly>::RemoveOrdered(arr.begin() + 5, arr.begin() + 2);
        // element 2 removed: arr[2] was 3, now becomes 4
        REQUIRE(arr[2].Value == 4);
        REQUIRE(arr[3].Value == 5);
    }
}

TEST_CASE("RemoveOrdered range of elements", "[RemoveOrdered][Range]")
{
    SECTION("trivial type")
    {
        std::array<u32, 6> arr = {1, 2, 3, 4, 5, 6};
        u32 size = 6;
        auto removed = Tools<u32>::RemoveOrdered(arr.begin() + size, arr.begin() + 1, arr.begin() + 4);
        REQUIRE(removed == 3);
        // arr => 1,5,6,...
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 5);
        REQUIRE(arr[2] == 6);
    }

    SECTION("non-trivial copyable")
    {
        std::array<CopyOnly, 6> arr;
        for (u32 i = 0; i < 6; ++i)
            arr[i] = CopyOnly(i + 1);
        auto removed = Tools<CopyOnly>::RemoveOrdered(arr.begin() + 6, arr.begin() + 2, arr.begin() + 5);
        REQUIRE(removed == 3);
        // removed 3 elements at [2,5) → element at 2 becomes old 5
        REQUIRE(arr[2].Value == 6);
    }
}

TEST_CASE("RemoveUnordered", "[RemoveUnordered]")
{
    SECTION("trivial type")
    {
        std::array<u32, 4> arr = {10, 20, 30, 40};
        u32 size = 4;
        Tools<u32>::RemoveUnordered(arr.begin() + size, arr.begin() + 1);
        // pos1 replaced by last element
        REQUIRE(arr[1] == 40);
    }

    SECTION("non-trivial copyable")
    {
        std::array<CopyOnly, 4> arr;
        for (u32 i = 0; i < 4; ++i)
            arr[i] = CopyOnly(i + 1);
        Tools<CopyOnly>::RemoveUnordered(arr.begin() + 4, arr.begin() + 0);
        // arr[0] should now hold old arr[3]
        REQUIRE(arr[0].Value == 4);
    }
}

TEST_CASE("ArrayTools<std::string>: CopyConstructFromRange", "[ArrayTools][string]")
{
    std::vector<std::string> src{"hello", "world", "foo"};
    std::array<std::string, 3> dst; // default‐constructed empty strings
    Tools<std::string>::CopyConstructFromRange(dst.begin(), src.begin(), src.end());

    REQUIRE(dst[0] == "hello");
    REQUIRE(dst[1] == "world");
    REQUIRE(dst[2] == "foo");
    // source untouched
    REQUIRE(src[1] == "world");
}

TEST_CASE("ArrayTools<std::string>: MoveConstructFromRange", "[ArrayTools][string]")
{
    std::vector<std::string> src{"a", "b", "c"};
    std::array<std::string, 3> dst;
    // capture original src values for later comparison
    auto orig = src;

    Tools<std::string>::MoveConstructFromRange(dst.begin(), src.begin(), src.end());

    // dest got the expected strings
    REQUIRE(dst[0] == "a");
    REQUIRE(dst[1] == "b");
    REQUIRE(dst[2] == "c");

    // moved-from src is guaranteed valid but unspecified.
    // We at least know it's not throwing and still has a string.
}

TEST_CASE("ArrayTools<std::string>: Insert single element", "[ArrayTools][string]")
{
    std::array<std::string, 5> arr = {"one", "two", "three", "", ""};
    usize size = 3; // valid data at [0,1,2]
    auto begin = arr.begin();
    auto end = begin + size;

    // insert at position 1
    Tools<std::string>::Insert(end, begin + 1, std::string("X"));

    // memory layout is now: one, X, two, three, <moved-from last>
    REQUIRE(arr[0] == "one");
    REQUIRE(arr[1] == "X");
    REQUIRE(arr[2] == "two");
    REQUIRE(arr[3] == "three");

    // we don't touch arr[4]; it's whatever the moved-from std::string left behind
}

TEST_CASE("ArrayTools<std::string>: Insert range of elements", "[ArrayTools][string]")
{
    std::array<std::string, 8> arr = {"a", "b", "c", "d", "", "", "", ""};
    usize size = 4;
    auto begin = arr.begin();
    auto end = begin + size;
    std::vector<std::string> src{"X", "Y", "Z"};

    // insert 3 elements at pos 2
    auto count = Tools<std::string>::Insert(end, begin + 2, src.begin(), src.end());
    REQUIRE(count == 3);

    // expected sequence: a, b, X, Y, Z, c, d, ...
    std::vector<std::string> expected = {"a", "b", "X", "Y", "Z", "c", "d"};
    for (size_t i = 0; i < expected.size(); ++i)
        REQUIRE(arr[i] == expected[i]);
}

TEST_CASE("ArrayTools<std::string>: RemoveOrdered single element", "[ArrayTools][string]")
{
    std::array<std::string, 5> arr = {"red", "green", "blue", "yellow", ""};
    usize size = 4;
    auto begin = arr.begin();
    auto end = begin + size;

    // remove the element at index 1 ("green")
    Tools<std::string>::RemoveOrdered(end, begin + 1);

    // now: red, blue, yellow, (destroyed)
    REQUIRE(arr[0] == "red");
    REQUIRE(arr[1] == "blue");
    REQUIRE(arr[2] == "yellow");
}

TEST_CASE("ArrayTools<std::string>: RemoveOrdered range of elements", "[ArrayTools][string]")
{
    std::array<std::string, 6> arr = {"p", "q", "r", "s", "t", ""};
    usize size = 5;
    auto begin = arr.begin();
    auto end = begin + size;

    // remove range [1,4) => {"q","r","s"}
    auto removed = Tools<std::string>::RemoveOrdered(end, begin + 1, begin + 4);
    REQUIRE(removed == 3);

    // now: p, t, (destroyed...)
    REQUIRE(arr[0] == "p");
    REQUIRE(arr[1] == "t");
}

TEST_CASE("ArrayTools<std::string>: RemoveUnordered", "[ArrayTools][string]")
{
    std::array<std::string, 4> arr = {"alpha", "beta", "gamma", "delta"};
    usize size = 4;
    auto begin = arr.begin();
    auto end = begin + size;

    // remove at pos 1 by swapping in last element
    Tools<std::string>::RemoveUnordered(end, begin + 1);

    REQUIRE(arr[1] == "delta"); // last element moved into pos 1
    // arr[0] and arr[2] remain one of the originals
}
