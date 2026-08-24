#include "tkit/container/ecs.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace TKit;
using namespace TKit::Alias;

struct Test_ComponentA
{
    u32 Data;
};
struct Test_ComponentB
{
    u32 Data0;
    u32 Data1;
};
struct Test_ComponentC
{
    std::vector<u32> Elements;
};

static ArenaAllocator s_Arena{4_mib};
static StackAllocator s_Stack{4_mib};
static TierAllocator s_Tier{{.Allocator = &s_Arena, .MaxAllocation = 128_kib}};

TEST_CASE("ECS: Entity creation and recycling", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        SECTION("Basic entity creation")
        {
            const Entity e0 = r.CreateEntity();
            const Entity e1 = r.CreateEntity();
            REQUIRE(e0.Index != e1.Index);
#ifdef TKIT_ENABLE_ENSURE
            REQUIRE(r.IsAlive(e0));
            REQUIRE(r.IsAlive(e1));
#endif
        }

#ifdef TKIT_ENABLE_ENSURE
        SECTION("Null entity is not alive")
        {
            REQUIRE_FALSE(r.IsAlive(NullEntity));
        }
#endif

        SECTION("Multiple entities have sequential indices")
        {
            const Entity e0 = r.CreateEntity();
            const Entity e1 = r.CreateEntity();
            const Entity e2 = r.CreateEntity();
            REQUIRE(e0.Index == 0);
            REQUIRE(e1.Index == 1);
            REQUIRE(e2.Index == 2);
        }
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Single component add and get", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        const Entity e = r.CreateEntity();

        SECTION("Add and retrieve a component")
        {
            Test_ComponentA *a = r.AddComponent<Test_ComponentA>(e, 42);
            REQUIRE(a != nullptr);
            REQUIRE(a->Data == 42);
            REQUIRE(r.GetComponent<Test_ComponentA>(e) == a);
        }

        SECTION("HasComponent returns correct results")
        {
            REQUIRE_FALSE(r.HasComponent<Test_ComponentA>(e));
            r.AddComponent<Test_ComponentA>(e, 10);
            REQUIRE(r.HasComponent<Test_ComponentA>(e));
            REQUIRE_FALSE(r.HasComponent<Test_ComponentB>(e));
        }

        SECTION("Component with non-trivial type")
        {
            Test_ComponentC *c = r.AddComponent<Test_ComponentC>(e, std::vector<u32>{1, 2, 3, 4});
            REQUIRE(c->Elements.size() == 4);
            REQUIRE(c->Elements[0] == 1);
            REQUIRE(c->Elements[3] == 4);
        }

        SECTION("Entity with no components returns null")
        {
            REQUIRE(r.GetComponent<Test_ComponentA>(e) == nullptr);
        }
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Multiple components on one entity", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        const Entity e = r.CreateEntity();
        [[maybe_unused]] Test_ComponentA *a = r.AddComponent<Test_ComponentA>(e, 10);
        [[maybe_unused]] Test_ComponentB *b = r.AddComponent<Test_ComponentB>(e, 20, 30);
        [[maybe_unused]] Test_ComponentC *c = r.AddComponent<Test_ComponentC>(e, std::vector<u32>{5, 6});

        SECTION("All components retrievable")
        {
            REQUIRE(r.GetComponent<Test_ComponentA>(e)->Data == 10);
            REQUIRE(r.GetComponent<Test_ComponentB>(e)->Data0 == 20);
            REQUIRE(r.GetComponent<Test_ComponentB>(e)->Data1 == 30);
            REQUIRE(r.GetComponent<Test_ComponentC>(e)->Elements.size() == 2);
        }

        SECTION("Adding second component does not invalidate first's data")
        {
            const Entity e2 = r.CreateEntity();
            r.AddComponent<Test_ComponentA>(e2, 100);
            r.AddComponent<Test_ComponentB>(e2, 200, 300);
            REQUIRE(r.GetComponent<Test_ComponentA>(e)->Data == 10);
            REQUIRE(r.GetComponent<Test_ComponentA>(e2)->Data == 100);
        }
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Remove component", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        SECTION("Remove moves entity to correct archetype")
        {
            const Entity e = r.CreateEntity();
            r.AddComponent<Test_ComponentA>(e, 10);
            r.AddComponent<Test_ComponentB>(e, 20, 30);

            r.RemoveComponent<Test_ComponentA>(e);
            REQUIRE_FALSE(r.HasComponent<Test_ComponentA>(e));
            REQUIRE(r.HasComponent<Test_ComponentB>(e));
            REQUIRE(r.GetComponent<Test_ComponentB>(e)->Data0 == 20);
        }

        SECTION("Remove preserves other entities in same archetype")
        {
            const Entity e0 = r.CreateEntity();
            const Entity e1 = r.CreateEntity();

            r.AddComponent<Test_ComponentA>(e0, 10);
            r.AddComponent<Test_ComponentB>(e0, 20, 30);

            r.AddComponent<Test_ComponentA>(e1, 100);
            r.AddComponent<Test_ComponentB>(e1, 200, 300);

            r.RemoveComponent<Test_ComponentA>(e0);

            REQUIRE(r.GetComponent<Test_ComponentA>(e1)->Data == 100);
            REQUIRE(r.GetComponent<Test_ComponentB>(e1)->Data0 == 200);
            REQUIRE(r.GetComponent<Test_ComponentB>(e0)->Data0 == 20);
        }

        SECTION("Remove non-trivial component cleans up properly")
        {
            const Entity e = r.CreateEntity();
            r.AddComponent<Test_ComponentA>(e, 5);
            r.AddComponent<Test_ComponentC>(e, std::vector<u32>{10, 20, 30});

            r.RemoveComponent<Test_ComponentC>(e);
            REQUIRE_FALSE(r.HasComponent<Test_ComponentC>(e));
            REQUIRE(r.GetComponent<Test_ComponentA>(e)->Data == 5);
        }
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Archetype transitions", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        SECTION("Entities with same components share archetype")
        {
            const Entity e0 = r.CreateEntity();
            const Entity e1 = r.CreateEntity();

            r.AddComponent<Test_ComponentA>(e0, 1);
            r.AddComponent<Test_ComponentB>(e0, 2, 3);

            r.AddComponent<Test_ComponentA>(e1, 4);
            r.AddComponent<Test_ComponentB>(e1, 5, 6);

            REQUIRE(r.GetComponent<Test_ComponentA>(e0)->Data == 1);
            REQUIRE(r.GetComponent<Test_ComponentA>(e1)->Data == 4);
        }

        SECTION("Add then remove returns to original archetype shape")
        {
            const Entity e = r.CreateEntity();
            r.AddComponent<Test_ComponentA>(e, 10);
            r.AddComponent<Test_ComponentB>(e, 20, 30);
            r.RemoveComponent<Test_ComponentB>(e);

            REQUIRE(r.HasComponent<Test_ComponentA>(e));
            REQUIRE_FALSE(r.HasComponent<Test_ComponentB>(e));
            REQUIRE(r.GetComponent<Test_ComponentA>(e)->Data == 10);
        }
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Query Each", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        const Entity e0 = r.CreateEntity();
        const Entity e1 = r.CreateEntity();
        const Entity e2 = r.CreateEntity();

        r.AddComponent<Test_ComponentA>(e0, 1);
        r.AddComponent<Test_ComponentB>(e0, 10, 11);

        r.AddComponent<Test_ComponentA>(e1, 2);
        r.AddComponent<Test_ComponentB>(e1, 20, 21);

        r.AddComponent<Test_ComponentA>(e2, 3);

        SECTION("Each iterates matching entities only")
        {
            u32 count = 0;
            u32 sum = 0;
            r.Query<Test_ComponentA, Test_ComponentB>().Each([&](Test_ComponentA &a, Test_ComponentB &b) {
                ++count;
                sum += a.Data + b.Data0;
            });
            REQUIRE(count == 2);
            REQUIRE(sum == 1 + 10 + 2 + 20);
        }

        SECTION("Single component query matches all entities with that component")
        {
            u32 count = 0;
            r.Query<Test_ComponentA>().Each([&](Test_ComponentA &) { ++count; });
            REQUIRE(count == 3);
        }

        SECTION("Each can mutate components")
        {
            r.Query<Test_ComponentA>().Each([](Test_ComponentA &a) { a.Data *= 10; });
            REQUIRE(r.GetComponent<Test_ComponentA>(e0)->Data == 10);
            REQUIRE(r.GetComponent<Test_ComponentA>(e1)->Data == 20);
            REQUIRE(r.GetComponent<Test_ComponentA>(e2)->Data == 30);
        }
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Query iteration with range-for", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB>();

        const Entity e0 = r.CreateEntity();
        const Entity e1 = r.CreateEntity();

        r.AddComponent<Test_ComponentA>(e0, 5);
        r.AddComponent<Test_ComponentB>(e0, 50, 51);

        r.AddComponent<Test_ComponentA>(e1, 6);
        r.AddComponent<Test_ComponentB>(e1, 60, 61);

        u32 count = 0;

        const auto &query = r.Query<Test_ComponentA, Test_ComponentB>();
        for (const auto row : query)
        {
            const Test_ComponentA &a = row.GetComponent<Test_ComponentA>();
            const Test_ComponentB &b = row.GetComponent<Test_ComponentB>();
            REQUIRE(a.Data > 0);
            REQUIRE(b.Data0 > 0);
            ++count;
        }
        REQUIRE(count == 2);
        count = 0;
        for (const auto &arch : query.Archetypes())
            for (const auto row : arch)
            {
                const Test_ComponentA &a = row.GetComponent<Test_ComponentA>();
                const Test_ComponentB &b = row.GetComponent<Test_ComponentB>();
                REQUIRE(a.Data > 0);
                REQUIRE(b.Data0 > 0);
                ++count;
            }
        REQUIRE(count == 2);
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Query matches across archetypes", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        const Entity e0 = r.CreateEntity();
        r.AddComponent<Test_ComponentA>(e0, 1);
        r.AddComponent<Test_ComponentB>(e0, 2, 3);

        const Entity e1 = r.CreateEntity();
        r.AddComponent<Test_ComponentA>(e1, 4);
        r.AddComponent<Test_ComponentB>(e1, 5, 6);
        r.AddComponent<Test_ComponentC>(e1, std::vector<u32>{7});

        u32 count = 0;
        r.Query<Test_ComponentA, Test_ComponentB>().Each([&](Test_ComponentA &, Test_ComponentB &) { ++count; });
        REQUIRE(count == 2);
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Query caching picks up new archetypes", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB, Test_ComponentC>();

        const Entity e0 = r.CreateEntity();
        r.AddComponent<Test_ComponentA>(e0, 1);

        u32 count = 0;
        r.Query<Test_ComponentA>().Each([&](Test_ComponentA &) { ++count; });
        REQUIRE(count == 1);

        const Entity e1 = r.CreateEntity();
        r.AddComponent<Test_ComponentA>(e1, 2);
        r.AddComponent<Test_ComponentB>(e1, 3, 4);

        count = 0;
        r.Query<Test_ComponentA>().Each([&](Test_ComponentA &) { ++count; });
        REQUIRE(count == 2);
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Swap remove bookkeeping", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB>();

        const Entity e0 = r.CreateEntity();
        const Entity e1 = r.CreateEntity();
        const Entity e2 = r.CreateEntity();

        r.AddComponent<Test_ComponentA>(e0, 10);
        r.AddComponent<Test_ComponentA>(e1, 20);
        r.AddComponent<Test_ComponentA>(e2, 30);

        r.AddComponent<Test_ComponentB>(e0, 100, 101);

        REQUIRE(r.GetComponent<Test_ComponentA>(e1)->Data == 20);
        REQUIRE(r.GetComponent<Test_ComponentA>(e2)->Data == 30);
    }
    PopStack();
    PopTier();
}

TEST_CASE("ECS: Many entities stress test", "[ECS]")
{
    PushTier(&s_Tier);
    PushStack(&s_Stack);
    {
        Registry r{};
        r.RegisterComponents<Test_ComponentA, Test_ComponentB>();

        constexpr u32 N = 1000;
        struct SomeData
        {
            Entity Ent = NullEntity;
            Test_ComponentA A{TKIT_U32_MAX};
            Test_ComponentB B{TKIT_U32_MAX, TKIT_U32_MAX};
        };
        std::vector<SomeData> entities{};
        for (u32 i = 0; i < N; ++i)
        {
            const Entity e = r.CreateEntity();
            Test_ComponentA *a = r.AddComponent<Test_ComponentA>(e, i);
            if (i % 2 == 0)
            {
                Test_ComponentB *b = r.AddComponent<Test_ComponentB>(e, i * 10, i * 10 + 1);
                entities.emplace_back(e, *a, *b);
            }
            else
                entities.emplace_back(e, *a);
        }

        u32 countA = 0;
        r.Query<Test_ComponentA>().Each([&](const Entity e, const Test_ComponentA &a) {
            for (const SomeData &sd : entities)
                if (sd.Ent == e)
                {
                    REQUIRE(sd.A.Data == a.Data);
                    ++countA;
                    return;
                }
            REQUIRE(false);
        });
        REQUIRE(countA == N);

        u32 countAB = 0;
        r.Query<Test_ComponentB, Test_ComponentA>().Each(
            [&](const Entity e, const Test_ComponentB &b, const Test_ComponentA &a) {
                for (const SomeData &sd : entities)
                    if (sd.Ent == e)
                    {
                        REQUIRE(sd.A.Data == a.Data);
                        REQUIRE(sd.B.Data0 == b.Data0);
                        REQUIRE(sd.B.Data1 == b.Data1);
                        ++countAB;
                        return;
                    }

                REQUIRE(false);
            });

        REQUIRE(countAB == N / 2);

        for (u32 i = 0; i < N; ++i)
        {
            REQUIRE(r.GetComponent<Test_ComponentA>(entities[i].Ent)->Data == i);
            if (i % 2 == 0)
            {
                REQUIRE(r.GetComponent<Test_ComponentB>(entities[i].Ent)->Data0 == i * 10);
                REQUIRE(r.GetComponent<Test_ComponentB>(entities[i].Ent)->Data1 == i * 10 + 1);
            }
        }
    }
    PopStack();
    PopTier();
}
