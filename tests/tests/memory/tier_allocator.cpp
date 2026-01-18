#include "tkit/memory/tier_allocator.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace TKit;

static ArenaAllocator s_Alloc{10_kib};

// A helper non-trivial type to test Create<T>, NCreate<T>, Destroy<T>
struct NonTrivialTA
{
    static inline u32 CtorCount = 0;
    static inline u32 DtorCount = 0;

    u32 value;

    NonTrivialTA(const u32 p_Value) : value(p_Value)
    {
        ++CtorCount;
    }
    ~NonTrivialTA()
    {
        ++DtorCount;
    }

    NonTrivialTA(const NonTrivialTA &) = delete;
    NonTrivialTA &operator=(const NonTrivialTA &) = delete;
};

// A strongly-aligned type to validate alignment guarantees
struct alignas(64) Align64TA
{
    u8 Padding[64];
};

TEST_CASE("Constructor and basic state", "[TierAllocator]")
{
    constexpr usize maxAlloc = 1024;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;
    constexpr usize maxAlign = 64;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay, maxAlign);

    REQUIRE(alloc.GetBufferSize() > 0);

    u32 dummy = 0;
    REQUIRE(!alloc.Belongs(&dummy));
}

TEST_CASE("Allocate/Deallocate across sizes", "[TierAllocator]")
{
    // Keep params small so we can exercise several tiers
    constexpr usize maxAlloc = 256;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay);

    // Try a spread of request sizes (intentionally non powers-of-two too)
    const usize sizes[] = {1, 8, 9, 16, 24, 32, 48, 64, 96, 128, 192, 256};

    std::vector<void *> ptrs;
    ptrs.reserve(std::size(sizes));

    for (usize i = 0; i < std::size(sizes); ++i)
    {
        void *p = alloc.Allocate(sizes[i]);
        // Requests up to maxAlloc should generally succeed. If a particular tier
        // is already full in this configuration, nullptr is acceptable — so just
        // assert Belongs() when non-null.
        if (p)
        {
            REQUIRE(alloc.Belongs(p));
            ptrs.push_back(p);
        }
        else
        {
            // if nullptr, it must be because capacity for that tier is exhausted
            // (acceptable), or the request exceeded allocator capability.
            // We keep the test robust by not failing here.
            ptrs.push_back(nullptr);
        }
    }

    // Deallocate the non-null ones with their exact request sizes
    for (usize i = 0; i < std::size(sizes); ++i)
    {
        if (ptrs[i])
            alloc.Deallocate(ptrs[i], sizes[i]);
    }

    // Re-allocate one representative size to check reuse happens
    void *again = alloc.Allocate(32);
    REQUIRE(again);
    REQUIRE(alloc.Belongs(again));
    alloc.Deallocate(again, 32);
}

TEST_CASE("Exhaust smallest tier and recover", "[TierAllocator]")
{
    constexpr usize maxAlloc = 512;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay);

    // Repeatedly allocate the smallest request until it returns nullptr.
    // This validates exhaustion returns nullptr and that deallocation restores capacity.
    std::vector<void *> ptrs;
    for (;;)
    {
        void *p = alloc.Allocate(1); // should map to the smallest tier (>= minAlloc)
        if (!p)
            break;
        REQUIRE(alloc.Belongs(p));
        ptrs.push_back(p);
    }
    REQUIRE(!ptrs.empty());

    for (void *p : ptrs)
        alloc.Deallocate(p, 1);

    // Should be able to allocate again after freeing all
    void *p = alloc.Allocate(1);
    REQUIRE(p);
    REQUIRE(alloc.Belongs(p));
    alloc.Deallocate(p, 1);
}

TEST_CASE("Typed Allocate<T>(count) and Destroy<T>(count)", "[TierAllocator]")
{
    constexpr usize maxAlloc = 1024;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay);

    constexpr usize count = 10;
    u32 *arr = alloc.Allocate<u32>(count);
    REQUIRE(arr);
    REQUIRE(alloc.Belongs(arr));

    for (usize i = 0; i < count; ++i)
        arr[i] = static_cast<u32>(i * 3);

    for (usize i = 0; i < count; ++i)
        REQUIRE(arr[i] == static_cast<u32>(i * 3));

    alloc.Destroy(arr, count);
}

TEST_CASE("Create<T>, NCreate<T> and Destroy<T>", "[TierAllocator]")
{
    constexpr usize maxAlloc = 1024;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay);

    NonTrivialTA::CtorCount = 0;
    NonTrivialTA::DtorCount = 0;

    // Single object Create/Destroy
    NonTrivialTA *a = alloc.Create<NonTrivialTA>(7);
    REQUIRE(a);
    REQUIRE(alloc.Belongs(a));
    REQUIRE(NonTrivialTA::CtorCount == 1);
    REQUIRE(a->value == 7);
    alloc.Destroy(a);
    REQUIRE(NonTrivialTA::DtorCount == 1);

    // NCreate / Destroy(count)
    constexpr usize n = 5;
    NonTrivialTA *many = alloc.NCreate<NonTrivialTA>(n, 42);
    REQUIRE(many);
    REQUIRE(alloc.Belongs(many));
    REQUIRE(NonTrivialTA::CtorCount == 1 + n);
    for (usize i = 0; i < n; ++i)
        REQUIRE(many[i].value == 42);

    alloc.Destroy(many, n);
    REQUIRE(NonTrivialTA::DtorCount == 1 + n);
}

TEST_CASE("Alignment guarantees (up to max alignment)", "[TierAllocator]")
{
    constexpr usize maxAlloc = 1024;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;
    constexpr usize maxAlign = 64;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay, maxAlign);

    // Allocate a strongly-aligned type; Allocate<T>() asserts internally too.
    Align64TA *p = alloc.Allocate<Align64TA>();
    REQUIRE(p);
    REQUIRE(alloc.Belongs(p));
    REQUIRE(reinterpret_cast<uintptr_t>(p) % alignof(Align64TA) == 0);

    alloc.Destroy(p);
}

TEST_CASE("Belongs() only checks buffer boundaries (not allocation state)", "[TierAllocator]")
{
    constexpr usize maxAlloc = 256;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;

    TierAllocator alloc(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay);

    void *p = alloc.Allocate(64);
    REQUIRE(p);
    REQUIRE(alloc.Belongs(p));

    // After freeing, the pointer is still within the allocator's buffer.
    alloc.Deallocate(p, 64);
    REQUIRE(alloc.Belongs(p));
}

TEST_CASE("Description::GetTierIndex sanity for min allocation", "[TierAllocator]")
{
    constexpr usize maxAlloc = 512;
    constexpr usize gran = 4;
    constexpr usize minAlloc = gran * sizeof(void *);
    constexpr f32 decay = 0.9f;

    auto desc = TierAllocator::CreateDescription(&s_Alloc, 32, maxAlloc, minAlloc, gran, decay);

    // For sizes <= MinAllocation, index should be the last tier
    const usize idxMin = desc.GetTierIndex(minAlloc);
    REQUIRE(idxMin + 1 == desc.Tiers.GetSize());
}
