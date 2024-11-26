#include "kit/memory/ptr.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <thread>

namespace TKit
{
class TestRefCounted : public RefCounted<TestRefCounted>
{
  public:
    TestRefCounted()
    {
        Instances.fetch_add(1, std::memory_order_relaxed);
    }
    ~TestRefCounted()
    {
        Instances.fetch_sub(1, std::memory_order_relaxed);
    }

    // Because they are used in threaded tests
    static inline std::atomic_int32_t Instances{0};
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

    static inline std::atomic_int32_t BaseInstances{0};
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
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
            REQUIRE(ref1->RefCount() == 2);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
    }

    SECTION("Ref copy assignment")
    {
        {
            Ref<TestRefCounted> ref;
            REQUIRE(!ref);
            {
                Ref<TestRefCounted> ref1(new TestRefCounted());
                ref = ref1;
                REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
                REQUIRE(ref1->RefCount() == 2);
            }
            REQUIRE(ref->RefCount() == 1);
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
    }

    SECTION("Ref<const T>")
    {
        {
            Ref<const TestRefCounted> ref(new TestRefCounted());
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
            REQUIRE(ref->RefCount() == 1);
            REQUIRE(std::is_same_v<const TestRefCounted *, decltype(ref.Get())>);

            Ref<TestRefCounted> ref2 = new TestRefCounted();
            Ref<const TestRefCounted> ref3 = ref2;
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
    }
}

TEST_CASE("Reference counting with containers", "[memory][ptr][container]")
{
    SECTION("Ref in vector")
    {
        {
            std::vector<Ref<TestRefCounted>> vec;
            for (usize i = 0; i < 1000; ++i)
                if (i % 2 == 0)
                    vec.emplace_back(new TestRefCounted());
                else
                    vec.push_back(vec[i - 1]);
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 500);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
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
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 500);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
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
            for (usize i = 0; i < 1000; ++i)
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

TEST_CASE("Reference counting from multiple threads", "[memory][ptr]")
{
    const auto createRefs = [](const std::array<Ref<TestRefCounted>, 100> &p_Refs) {
        std::array<Ref<TestRefCounted>, 100> refs;
        for (i32 i = 0; i < 100; ++i)
        {
            if (i % 2 == 0)
                refs[i] = new TestRefCounted();
            else
                refs[i] = p_Refs[i];
        }
    };
    {
        std::array<Ref<TestRefCounted>, 100> refs;
        for (i32 i = 0; i < 100; ++i)
            refs[i] = new TestRefCounted;

        std::array<std::thread, 8> threads;
        for (std::thread &thread : threads)
            thread = std::thread(createRefs, refs);
        for (std::thread &thread : threads)
            thread.join();
    }
    REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
}

TEST_CASE("Scope basic operations", "[memory][ptr]")
{
    SECTION("Scope")
    {
        {
            Scope<TestRefCounted> scope(new TestRefCounted());
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
    }

    SECTION("Scope with move")
    {
        {
            Scope<TestRefCounted> scope(new TestRefCounted());
            Scope<TestRefCounted> scope2(std::move(scope));
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
    }

    SECTION("Scope with move assignment")
    {
        {
            Scope<TestRefCounted> scope(new TestRefCounted());
            Scope<TestRefCounted> scope2;
            scope2 = std::move(scope);
            REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 1);
        }
        REQUIRE(TestRefCounted::Instances.load(std::memory_order_relaxed) == 0);
    }
}

TEST_CASE("Scope ptr with inheritance", "[memory][ptr]")
{
    SECTION("Scope<TestBase> to Scope<TestDerived>")
    {
        {
            Scope<TestBase> base(new TestDerived());
            REQUIRE(TestBase::BaseInstances == 1);
            REQUIRE(TestDerived::DerivedInstances == 1);
        }
        REQUIRE(TestBase::BaseInstances == 0);
        REQUIRE(TestDerived::DerivedInstances == 0);
    }

    SECTION("Scope<TestDerived> to Scope<TestBase>")
    {
        {
            Scope<TestDerived> derived(new TestDerived());
            Scope<TestBase> base(new TestBase());

            // This should leave refcount unaffected
            base = std::move(derived);
            Scope<TestBase> extraBase(new TestBase());
            REQUIRE(TestBase::BaseInstances == 2);
            REQUIRE(TestDerived::DerivedInstances == 1);
        }
        REQUIRE(TestBase::BaseInstances == 0);
        REQUIRE(TestDerived::DerivedInstances == 0);
    }
}

} // namespace TKit