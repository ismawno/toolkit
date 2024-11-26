#include "tkit/container/storage.hpp"
#include "tkit/core/alias.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
TEST_CASE("Raw storage", "[core][container][Storage]")
{
    SECTION("Trivial type")
    {
        RawStorage<16> storage;
        storage.Create<u32>(42);
        REQUIRE(*storage.Get<u32>() == 42);

        const RawStorage<16> other = storage;
        REQUIRE(*other.Get<u32>() == 42);

        storage.Destroy<u32>();
    }

    SECTION("Non-trivial type")
    {
        RawStorage<sizeof(NonTrivialData)> storage;
        storage.Create<NonTrivialData>();
        REQUIRE(NonTrivialData::Instances == 1);

        storage.Destroy<NonTrivialData>();
        REQUIRE(NonTrivialData::Instances == 0);
    }

    SECTION("Aligned type")
    {
        RawStorage<sizeof(AlignedData), alignof(AlignedData)> storage;
        AlignedData *data = storage.Create<AlignedData>();
        data->x = 1.0;
        data->y = 2.0;
        data->z = 3.0;
        data->a = 4.0;
        data->b = 5.0;
        data->c = 6.0;

        REQUIRE(data->x == 1.0);
        REQUIRE(data->y == 2.0);
        REQUIRE(data->z == 3.0);
        REQUIRE(data->a == 4.0);
        REQUIRE(data->b == 5.0);
        REQUIRE(data->c == 6.0);

        storage.Destroy<AlignedData>();
    }
}

TEST_CASE("Storage", "[core][container][Storage]")
{
    SECTION("Trivial type")
    {
        Storage<u32> storage;
        storage.Create(42);
        REQUIRE(*storage.Get() == 42);

        const Storage<u32> other = storage;
        REQUIRE(*other.Get() == 42);

        storage.Destroy();
    }

    SECTION("Non-trivial type")
    {
        Storage<NonTrivialData> storage;
        storage.Create();
        REQUIRE(NonTrivialData::Instances == 1);

        const Storage<NonTrivialData> other = storage;
        REQUIRE(NonTrivialData::Instances == 2);

        other.Destroy();
        REQUIRE(NonTrivialData::Instances == 1);

        storage.Destroy();
        REQUIRE(NonTrivialData::Instances == 0);
    }

    SECTION("Aligned type")
    {
        Storage<AlignedData> storage;
        AlignedData *data = storage.Create();
        data->x = 1.0;
        data->y = 2.0;
        data->z = 3.0;
        data->a = 4.0;
        data->b = 5.0;
        data->c = 6.0;

        REQUIRE(data->x == 1.0);
        REQUIRE(data->y == 2.0);
        REQUIRE(data->z == 3.0);
        REQUIRE(data->a == 4.0);
        REQUIRE(data->b == 5.0);
        REQUIRE(data->c == 6.0);

        storage.Destroy();
    }
}
} // namespace TKit