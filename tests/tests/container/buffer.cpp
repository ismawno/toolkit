#include "tkit/container/buffer.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
struct SomeRandomData
{
    i32 a;
    i32 b;
    bool c;
    u8 d[3];
    f32 e;
};

template <typename T> void RunBufferTest()
{
    const usize alignment = 128;
    SECTION("Common methods")
    {
        const Buffer<T> ordinary(5);
        REQUIRE(ordinary.GetSize() == 5 * sizeof(T));
        REQUIRE(ordinary.GetInstanceAlignedSize() == sizeof(T));
        REQUIRE(ordinary.GetInstanceSize() == sizeof(T));

        const Buffer<T> aligned(5, alignment);
        for (usize i = 0; i < ordinary.GetInstanceCount(); ++i)
            REQUIRE(reinterpret_cast<const uptr>(aligned.ReadAt(i)) % alignment == 0);
    }

    SECTION("Copy constructors")
    {
        const Buffer<T> ordinary(5);
        const Buffer<T> copy(ordinary);
        REQUIRE(copy.GetSize() == ordinary.GetSize());
        REQUIRE(copy.GetInstanceCount() == ordinary.GetInstanceCount());
        REQUIRE(copy.GetInstanceSize() == ordinary.GetInstanceSize());
        REQUIRE(copy.GetInstanceAlignedSize() == ordinary.GetInstanceAlignedSize());
        if constexpr (!std::is_same_v<T, SomeRandomData>)
        {
            for (usize i = 0; i < ordinary.GetInstanceCount(); ++i)
                REQUIRE(copy[i] == ordinary[i]);
            T val{19};
            Buffer<T> buff{5};
            buff[2] = 2 * val;
            buff.WriteAt(1, &val);
            REQUIRE(buff[2] == 2 * val);
            REQUIRE(buff[1] == val);
        }
    }
}

TEST_CASE("Buffer (i32)", "[core][container][buffer]")
{
    RunBufferTest<i32>();
}
TEST_CASE("Buffer (f32)", "[core][container][buffer]")
{
    RunBufferTest<f32>();
}
TEST_CASE("Buffer (SomeRandomData)", "[core][container][buffer]")
{
    RunBufferTest<SomeRandomData>();
}

} // namespace TKit