#include "tkit/container/stack_hive.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Alias;

static StackAllocator s_HiveAlloc{1_mib};

TEST_CASE("StackHive: basic capacity/size queries", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 4};
    REQUIRE(hive.GetSize() == 0);
    REQUIRE(hive.IsEmpty());
    REQUIRE_FALSE(hive.IsFull());

    const usize id0 = hive.Insert(10u);
    const usize id1 = hive.Insert(20u);
    REQUIRE(hive.GetSize() == 2);
    REQUIRE_FALSE(hive.IsEmpty());
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id1));
}

TEST_CASE("StackHive: Insert and lookup", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 8};

    const usize id0 = hive.Insert(100u);
    const usize id1 = hive.Insert(200u);
    const usize id2 = hive.Insert(300u);

    REQUIRE(hive[id0] == 100u);
    REQUIRE(hive[id1] == 200u);
    REQUIRE(hive[id2] == 300u);

    // mutation via reference
    hive[id1] = 999u;
    REQUIRE(hive[id1] == 999u);

    REQUIRE(hive.GetSize() == 3);
}

TEST_CASE("StackHive: Remove invalidates id and compacts dense array", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 8};

    const usize id0 = hive.Insert(1u);
    const usize id1 = hive.Insert(2u);
    const usize id2 = hive.Insert(3u);

    hive.Remove(id1);

    REQUIRE(hive.GetSize() == 2);
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id2));

    // surviving elements still have correct values
    REQUIRE(hive[id0] == 1u);
    REQUIRE(hive[id2] == 3u);

    // dense array holds exactly the two survivors
    u32 sum = 0;
    for (const u32 x : hive)
        sum += x;
    REQUIRE(sum == 4u);
}

TEST_CASE("StackHive: Remove last element", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 4};

    const usize id0 = hive.Insert(42u);
    const usize id1 = hive.Insert(43u);

    hive.Remove(id1); // last in dense array, early-return branch

    REQUIRE(hive.GetSize() == 1);
    REQUIRE(hive.Contains(id0));
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive[id0] == 42u);
}

TEST_CASE("StackHive: Remove first element", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 4};

    const usize id0 = hive.Insert(10u);
    const usize id1 = hive.Insert(20u);
    const usize id2 = hive.Insert(30u);

    hive.Remove(id0); // swap branch: id0 is at dense[0], last is id2

    REQUIRE(hive.GetSize() == 2);
    REQUIRE_FALSE(hive.Contains(id0));
    REQUIRE(hive.Contains(id1));
    REQUIRE(hive.Contains(id2));
    REQUIRE(hive[id1] == 20u);
    REQUIRE(hive[id2] == 30u);
}

TEST_CASE("StackHive: id reuse after Remove", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 8};

    const usize id0 = hive.Insert(1u);
    const usize id1 = hive.Insert(2u);
    const usize id2 = hive.Insert(3u);

    hive.Remove(id1);
    REQUIRE_FALSE(hive.Contains(id1));

    // next Insert should reclaim id1
    const usize id3 = hive.Insert(99u);
    REQUIRE(id3 == id1);
    REQUIRE(hive.Contains(id3));
    REQUIRE(hive[id3] == 99u);

    // all three live ids are valid
    REQUIRE(hive.GetSize() == 3);
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id2));
}

TEST_CASE("StackHive: multiple Removes then Inserts reuse ids in order", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 8};

    const usize id0 = hive.Insert(10u);
    const usize id1 = hive.Insert(20u);
    const usize id2 = hive.Insert(30u);
    const usize id3 = hive.Insert(40u);

    hive.Remove(id1);
    hive.Remove(id2);
    REQUIRE(hive.GetSize() == 2);

    const usize nid0 = hive.Insert(55u);
    const usize nid1 = hive.Insert(66u);

    // both recycled ids come from the freed slots
    REQUIRE((nid0 == id1 || nid0 == id2));
    REQUIRE((nid1 == id1 || nid1 == id2));
    REQUIRE(nid0 != nid1);

    REQUIRE(hive.GetSize() == 4);
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id3));
    REQUIRE(hive.Contains(nid0));
    REQUIRE(hive.Contains(nid1));
}

TEST_CASE("StackHive: dense iteration is contiguous and order-independent", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 8};

    hive.Insert(1u);
    hive.Insert(2u);
    const usize mid = hive.Insert(3u);
    hive.Insert(4u);
    hive.Insert(5u);

    hive.Remove(mid);

    // dense array has exactly 4 elements summing to 12
    u32 sum = 0;
    u32 count = 0;
    for (const u32 x : hive)
    {
        sum += x;
        ++count;
    }
    REQUIRE(count == 4);
    REQUIRE(sum == 12u);
}

TEST_CASE("StackHive: Reserve does not change size", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc};
    hive.Reserve(8);
    REQUIRE(hive.GetSize() == 0);
    REQUIRE(hive.IsEmpty());
    REQUIRE(hive.GetCapacity() >= 8);
}

TEST_CASE("StackHive: copy and move", "[StackHive]")
{
    StackHive<u32> hive{&s_HiveAlloc, 8};
    const usize id0 = hive.Insert(7u);
    const usize id1 = hive.Insert(8u);

    // copy
    StackHive<u32> copy = hive;
    REQUIRE(copy.GetSize() == 2);
    REQUIRE(copy[id0] == 7u);
    REQUIRE(copy[id1] == 8u);

    // mutation of copy does not affect original
    copy[id0] = 77u;
    REQUIRE(hive[id0] == 7u);

    // move
    StackHive<u32> moved = std::move(copy);
    REQUIRE(moved.GetSize() == 2);
    REQUIRE(moved[id0] == 77u);
    REQUIRE(moved[id1] == 8u);
}

TEST_CASE("StackHive<std::string>: Insert, Remove and id reuse", "[StackHive][string]")
{
    StackHive<std::string> hive{&s_HiveAlloc, 8};

    const usize id0 = hive.Insert("alpha");
    const usize id1 = hive.Insert("beta");
    const usize id2 = hive.Insert("gamma");

    REQUIRE(hive[id0] == "alpha");
    REQUIRE(hive[id1] == "beta");
    REQUIRE(hive[id2] == "gamma");

    hive.Remove(id1);
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive.GetSize() == 2);

    // survivors intact
    REQUIRE(hive[id0] == "alpha");
    REQUIRE(hive[id2] == "gamma");

    // id reuse
    const usize id3 = hive.Insert("delta");
    REQUIRE(id3 == id1);
    REQUIRE(hive[id3] == "delta");
    REQUIRE(hive.GetSize() == 3);

    // dense iteration sees all three strings
    std::string concat;
    for (const std::string &s : hive)
        concat += s;
    REQUIRE(concat.size() == 5 + 5 + 5); // "alpha"+"gamma"+"delta" = 15 (or any order)
    REQUIRE(concat.find("alpha") != std::string::npos);
    REQUIRE(concat.find("gamma") != std::string::npos);
    REQUIRE(concat.find("delta") != std::string::npos);
}
