#include "tkit/container/storage.hpp"
#include "tkit/core/alias.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
TEST_CASE("Raw storage", "[core][container][raw_storage]")
{
    SECTION("Trivial type")
    {
        RawStorage<16> storage;
        storage.Construct<u32>(42);
        REQUIRE(*storage.Get<u32>() == 42);

        const RawStorage<16> other = storage;
        REQUIRE(*other.Get<u32>() == 42);

        storage.Destruct<u32>();
    }

    SECTION("Non-trivial type")
    {
        RawStorage<sizeof(NonTrivialData)> storage;
        storage.Construct<NonTrivialData>();
        REQUIRE(NonTrivialData::Instances == 1);

        storage.Destruct<NonTrivialData>();
        REQUIRE(NonTrivialData::Instances == 0);
    }

    SECTION("Aligned type")
    {
        RawStorage<sizeof(AlignedData), alignof(AlignedData)> storage;
        AlignedData *data = storage.Construct<AlignedData>();
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

        storage.Destruct<AlignedData>();
    }
}

TEST_CASE("Storage", "[core][container][storage]")
{
    SECTION("Trivial type")
    {
        Storage<u32> storage;
        storage.Construct(42);
        REQUIRE(*storage.Get() == 42);

        const Storage<u32> other = storage;
        REQUIRE(*other.Get() == 42);

        storage.Destruct();
    }

    SECTION("Non-trivial type")
    {
        Storage<NonTrivialData> storage;
        storage.Construct();
        REQUIRE(NonTrivialData::Instances == 1);

        const Storage<NonTrivialData> other = storage;
        REQUIRE(NonTrivialData::Instances == 2);

        other.Destruct();
        REQUIRE(NonTrivialData::Instances == 1);

        storage.Destruct();
        REQUIRE(NonTrivialData::Instances == 0);
    }

    SECTION("Aligned type")
    {
        Storage<AlignedData> storage;
        AlignedData *data = storage.Construct();
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

        storage.Destruct();
    }
}
} // namespace TKit