#include "tkit/utils/logging.hpp"
#include "tkit/memory/ptr.hpp"
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>

using namespace TKit;
using namespace TKit::Alias;

// A RefCounted type to track destructor calls
struct MyRefCounted : public RefCounted<MyRefCounted>
{
    int value;
    static inline int dtorCount = 0;
    MyRefCounted(int v) : value(v)
    {
    }
    ~MyRefCounted()
    {
        ++dtorCount;
    }
};

TEST_CASE("Ref: basic reference counting", "[Ref]")
{
    MyRefCounted::dtorCount = 0;

    {
        auto ref1 = Ref<MyRefCounted>::Create(42);
        REQUIRE(ref1->value == 42);
        REQUIRE(ref1->RefCount() == 1);

        auto ref2 = ref1;
        REQUIRE(ref1->RefCount() == 2);

        Ref<MyRefCounted> ref3;
        ref3 = ref1;
        REQUIRE(ref1->RefCount() == 3);
    }

    // all Refs destroyed, object should have been deleted once
    REQUIRE(MyRefCounted::dtorCount == 1);
}

TEST_CASE("Ref: move semantics", "[Ref]")
{
    MyRefCounted::dtorCount = 0;

    {
        auto ref1 = Ref<MyRefCounted>::Create(7);
        REQUIRE(ref1->RefCount() == 1);

        auto ref2 = std::move(ref1);
        REQUIRE(ref2->RefCount() == 1);
        REQUIRE(!ref1);
    }

    REQUIRE(MyRefCounted::dtorCount == 1);
}

TEST_CASE("Ref: boolean and get()", "[Ref]")
{
    auto ref1 = Ref<MyRefCounted>::Create(5);
    REQUIRE(static_cast<bool>(ref1));
    REQUIRE(ref1.Get()->value == 5);

    Ref<MyRefCounted> ref2;
    REQUIRE(!ref2);
}

TEST_CASE("Ref: hashing equal pointers", "[Ref]")
{
    auto ref1 = Ref<MyRefCounted>::Create(1);
    auto ref2 = ref1;
    std::hash<Ref<MyRefCounted>> hasher;
    REQUIRE(hasher(ref1) == hasher(ref2));
}

//===----------------------------------------------------------------------===//
//  Scope tests
//===----------------------------------------------------------------------===//

// A type to track destructor calls via Scope
struct MyScopeObj
{
    static inline int dtorCount = 0;
    int value;
    MyScopeObj(int v) : value(v)
    {
    }
    ~MyScopeObj()
    {
        ++dtorCount;
    }
};

TEST_CASE("Scope: basic ownership and destruction", "[Scope]")
{
    MyScopeObj::dtorCount = 0;
    {
        auto scope1 = Scope<MyScopeObj>::Create(10);
        REQUIRE(scope1->value == 10);
    }
    REQUIRE(MyScopeObj::dtorCount == 1);
}

TEST_CASE("Scope: Reset and Release", "[Scope]")
{
    MyScopeObj::dtorCount = 0;

    {
        auto scope1 = Scope<MyScopeObj>::Create(20);
        scope1.Reset(new MyScopeObj(30));
        // resetting deleted the first object
        REQUIRE(MyScopeObj::dtorCount == 1);

        MyScopeObj *raw = scope1.Release();
        REQUIRE(!scope1);
        delete raw;
        REQUIRE(MyScopeObj::dtorCount == 2);
    }

    // no further deletions
    REQUIRE(MyScopeObj::dtorCount == 2);
}

TEST_CASE("Scope: move to Ref via AsRef and conversion", "[Scope]")
{
    MyRefCounted::dtorCount = 0;

    {
        auto scope1 = Scope<MyRefCounted>::Create(99);
        REQUIRE(scope1->value == 99);

        auto ref1 = scope1.AsRef();
        REQUIRE(!scope1);
        REQUIRE(ref1->value == 99);
    }
    // ref went out of scope, underlying object deleted
    REQUIRE(MyRefCounted::dtorCount == 1);
}

TEST_CASE("Scope: move semantics and bool", "[Scope]")
{
    MyScopeObj::dtorCount = 0;

    auto scope1 = Scope<MyScopeObj>::Create(7);
    REQUIRE(scope1);
    auto scope2 = std::move(scope1);
    REQUIRE(scope2);
    REQUIRE(!scope1);

    scope2.Reset();
    REQUIRE(!scope2);
    REQUIRE(MyScopeObj::dtorCount == 1);
}

TEST_CASE("Scope: hashing equals underlying pointer", "[Scope]")
{
    std::hash<Scope<MyScopeObj>> hashScope;
    std::hash<MyScopeObj *> hashPtr;

    // Empty scope hashes the nullptr
    Scope<MyScopeObj> empty{};
    REQUIRE(hashScope(empty) == hashPtr(nullptr));

    // Create a new object and release it from scopeA so we can construct scopeB
    auto scopeA = Scope<MyScopeObj>::Create(123);
    MyScopeObj *rawPtr = scopeA.Release();

    // Now scopeB owns the same pointer
    Scope<MyScopeObj> scopeB(rawPtr);
    REQUIRE(scopeB); // sanity
    REQUIRE(hashScope(scopeB) == hashPtr(rawPtr));

    // Clean up
    delete scopeB.Release();
}
