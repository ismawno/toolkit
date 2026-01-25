#include "tkit/memory/ptr.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace TKit;
using namespace TKit::Alias;

// A RefCounted type to track destructor calls
struct MyRefCounted : public RefCounted<MyRefCounted>
{
    u32 Value;
    static inline u32 DtorCount = 0;
    MyRefCounted(const u32 value) : Value(value)
    {
    }
    ~MyRefCounted()
    {
        ++DtorCount;
    }
};

TEST_CASE("Ref: basic reference counting", "[Ref]")
{
    MyRefCounted::DtorCount = 0;

    {
        const auto ref1 = Ref<MyRefCounted>::Create(42);
        REQUIRE(ref1->Value == 42);
        REQUIRE(ref1->RefCount() == 1);

        const auto ref2 = ref1;
        REQUIRE(ref1->RefCount() == 2);

        Ref<MyRefCounted> ref3;
        ref3 = ref1;
        REQUIRE(ref1->RefCount() == 3);
    }

    // all Refs destroyed, object should have been deleted once
    REQUIRE(MyRefCounted::DtorCount == 1);
}

TEST_CASE("Ref: move semantics", "[Ref]")
{
    MyRefCounted::DtorCount = 0;

    {
        auto ref1 = Ref<MyRefCounted>::Create(7);
        REQUIRE(ref1->RefCount() == 1);

        const auto ref2 = std::move(ref1);
        REQUIRE(ref2->RefCount() == 1);
        REQUIRE(!ref1);
    }

    REQUIRE(MyRefCounted::DtorCount == 1);
}

TEST_CASE("Ref: boolean and get()", "[Ref]")
{
    const auto ref1 = Ref<MyRefCounted>::Create(5);
    REQUIRE(static_cast<bool>(ref1));
    REQUIRE(ref1.Get()->Value == 5);

    Ref<MyRefCounted> ref2;
    REQUIRE(!ref2);
}

TEST_CASE("Ref: hashing equal pointers", "[Ref]")
{
    const auto ref1 = Ref<MyRefCounted>::Create(1);
    const auto ref2 = ref1;
    std::hash<Ref<MyRefCounted>> hasher;
    REQUIRE(hasher(ref1) == hasher(ref2));
}

//===----------------------------------------------------------------------===//
//  Scope tests
//===----------------------------------------------------------------------===//

// A type to track destructor calls via Scope
struct MyScopeObj
{
    static inline u32 DtorCount = 0;
    u32 Value;
    MyScopeObj(const u32 value) : Value(value)
    {
    }
    ~MyScopeObj()
    {
        ++DtorCount;
    }
};

TEST_CASE("Scope: basic ownership and destruction", "[Scope]")
{
    MyScopeObj::DtorCount = 0;
    {
        const auto scope1 = Scope<MyScopeObj>::Create(10);
        REQUIRE(scope1->Value == 10);
    }
    REQUIRE(MyScopeObj::DtorCount == 1);
}

TEST_CASE("Scope: Reset and Release", "[Scope]")
{
    MyScopeObj::DtorCount = 0;

    {
        auto scope1 = Scope<MyScopeObj>::Create(20);
        scope1.Reset(new MyScopeObj(30));
        // resetting deleted the first object
        REQUIRE(MyScopeObj::DtorCount == 1);

        MyScopeObj *raw = scope1.Release();
        REQUIRE(!scope1);
        delete raw;
        REQUIRE(MyScopeObj::DtorCount == 2);
    }

    // no further deletions
    REQUIRE(MyScopeObj::DtorCount == 2);
}

TEST_CASE("Scope: move to Ref via AsRef and conversion", "[Scope]")
{
    MyRefCounted::DtorCount = 0;

    {
        auto scope1 = Scope<MyRefCounted>::Create(99);
        REQUIRE(scope1->Value == 99);

        const auto ref1 = scope1.AsRef();
        REQUIRE(!scope1);
        REQUIRE(ref1->Value == 99);
    }
    // ref went out of scope, underlying object deleted
    REQUIRE(MyRefCounted::DtorCount == 1);
}

TEST_CASE("Scope: move semantics and bool", "[Scope]")
{
    MyScopeObj::DtorCount = 0;

    auto scope1 = Scope<MyScopeObj>::Create(7);
    REQUIRE(scope1);
    auto scope2 = std::move(scope1);
    REQUIRE(scope2);
    REQUIRE(!scope1);

    scope2.Reset();
    REQUIRE(!scope2);
    REQUIRE(MyScopeObj::DtorCount == 1);
}

TEST_CASE("Scope: hashing equals underlying pointer", "[Scope]")
{
    const std::hash<Scope<MyScopeObj>> hashScope;
    const std::hash<MyScopeObj *> hashPtr;

    // Empty scope hashes the nullptr
    const Scope<MyScopeObj> empty{};
    REQUIRE(hashScope(empty) == hashPtr(nullptr));

    // Create a new object and release it from scopeA so we can construct scopeB
    auto scopeA = Scope<MyScopeObj>::Create(123);
    MyScopeObj *rawPtr = scopeA.Release();

    // Now scopeB owns the same pointer
    const Scope<MyScopeObj> scopeB(rawPtr);
    REQUIRE(scopeB); // sanity
    REQUIRE(hashScope(scopeB) == hashPtr(rawPtr));
}
