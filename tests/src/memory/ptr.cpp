#include "kit/memory/ptr.hpp"
#include <catch2/catch_test_macros.hpp>

KIT_NAMESPACE_BEGIN

class TestRefCounted : public RefCounted<TestRefCounted>
{
  public:
    TestRefCounted()
    {
        ++Instances;
    }
    ~TestRefCounted()
    {
        --Instances;
    }

    static inline i32 Instances = 0;
};

class TestBase : public RefCounted<TestBase>
{
  public:
    TestBase()
    {
        ++BaseInstances;
    }
    virtual ~TestBase()
    {
        --BaseInstances;
    }

    static inline i32 BaseInstances = 0;
};

class TestDerived : public TestBase
{
  public:
    using Base = TestBase;
    TestDerived()
    {
        ++DerivedInstances;
    }
    ~TestDerived() override
    {
        --DerivedInstances;
    }

    static inline i32 DerivedInstances = 0;
};

TEST_CASE("Reference counting basic operations", "[memory][ptr]")
{
    SECTION("Ref copy ctor")
    {
        {
            Ref<TestRefCounted> ref1(new TestRefCounted());
            Ref<TestRefCounted> ref2(ref1);
            REQUIRE(TestRefCounted::Instances == 1);
            REQUIRE(ref1->RefCount() == 2);
        }
        REQUIRE(TestRefCounted::Instances == 0);
    }

    SECTION("Ref copy assignment")
    {
        {
            Ref<TestRefCounted> ref;
            REQUIRE(!ref);
            {
                Ref<TestRefCounted> ref1(new TestRefCounted());
                ref = ref1;
                REQUIRE(TestRefCounted::Instances == 1);
                REQUIRE(ref1->RefCount() == 2);
            }
            REQUIRE(ref->RefCount() == 1);
            REQUIRE(TestRefCounted::Instances == 1);
        }
        REQUIRE(TestRefCounted::Instances == 0);
    }

    SECTION("Ref<const T>")
    {
        {
            Ref<const TestRefCounted> ref(new TestRefCounted());
            REQUIRE(TestRefCounted::Instances == 1);
            REQUIRE(ref->RefCount() == 1);
            REQUIRE(std::is_same_v<const TestRefCounted *, decltype(ref.Get())>);
        }
        REQUIRE(TestRefCounted::Instances == 0);
    }
}

TEST_CASE("Reference counting with containers", "[memory][ptr][container]")
{
    SECTION("Ref in vector")
    {
        {
            std::vector<Ref<TestRefCounted>> vec;
            for (usz i = 0; i < 1000; ++i)
                if (i % 2 == 0)
                    vec.emplace_back(new TestRefCounted());
                else
                    vec.push_back(vec[i - 1]);
            REQUIRE(TestRefCounted::Instances == 500);
        }
        REQUIRE(TestRefCounted::Instances == 0);
    }

    SECTION("Ref in map")
    {
        {
            HashMap<u32, Ref<TestRefCounted>> map;
            for (u32 i = 0; i < 1000; ++i)
                if (i % 2 == 0)
                    map.emplace(i, new TestRefCounted());
                else
                    map.emplace(i, map[i - 1]);
            REQUIRE(TestRefCounted::Instances == 500);
        }
        REQUIRE(TestRefCounted::Instances == 0);
    }
}

TEST_CASE("Reference counting with inheritance", "[memory][ptr]")
{

    SECTION("Ref<TestBase> to Ref<TestDerived>")
    {
        {
            Ref<TestBase> base(new TestDerived());
            REQUIRE(TestBase::BaseInstances == 1);
            REQUIRE(TestDerived::DerivedInstances == 1);
        }
        REQUIRE(TestBase::BaseInstances == 0);
        REQUIRE(TestDerived::DerivedInstances == 0);
    }

    SECTION("Ref<TestDerived> to Ref<TestBase>")
    {
        {
            Ref<TestDerived> derived(new TestDerived());
            Ref<TestBase> base(derived);

            // This should leave refcount unaffected
            base = derived;
            Ref<TestBase> extraBase(new TestBase());
            REQUIRE(TestBase::BaseInstances == 2);
            REQUIRE(TestDerived::DerivedInstances == 1);
        }
        REQUIRE(TestBase::BaseInstances == 0);
        REQUIRE(TestDerived::DerivedInstances == 0);
    }

    SECTION("Ref in vector (virtual)")
    {
        {
            std::vector<Ref<TestBase>> vec;
            for (usz i = 0; i < 1000; ++i)
                if (i % 2 == 0)
                {
                    Ref<TestDerived> derived(new TestDerived());
                    vec.emplace_back(derived);
                }
                else
                    vec.push_back(vec[i - 1]);
            REQUIRE(TestDerived::DerivedInstances == 500);
            REQUIRE(TestBase::BaseInstances == 500);
        }
        REQUIRE(TestDerived::DerivedInstances == 0);
        REQUIRE(TestBase::BaseInstances == 0);
    }

    SECTION("Ref in map (virtual)")
    {
        {
            HashMap<u32, Ref<TestBase>> map;
            for (u32 i = 0; i < 1000; ++i)
                if (i % 2 == 0)
                {
                    Ref<TestDerived> derived(new TestDerived());
                    map.emplace(i, derived);
                }
                else
                    map.emplace(i, map[i - 1]);
            REQUIRE(TestDerived::DerivedInstances == 500);
            REQUIRE(TestBase::BaseInstances == 500);
        }
        REQUIRE(TestDerived::DerivedInstances == 0);
        REQUIRE(TestBase::BaseInstances == 0);
    }
}

KIT_NAMESPACE_END