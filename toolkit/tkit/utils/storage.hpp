#pragma once

#include "tkit/memory/arena_allocator.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/tier_allocator.hpp"
#include "tkit/utils/non_copyable.hpp"

namespace TKit
{
enum StorageType : u8
{
    Storage_Static,
    Storage_Dynamic,
    Storage_Arena,
    Storage_Stack,
    Storage_Tier
};
template <usize Capacity, usize Alignment = alignof(std::max_align_t)> class StaticStorage
{
  public:
    static constexpr StorageType Type = Storage_Static;
    template <typename T, typename... Args> constexpr T &Construct(Args &&...args)
    {
        static_assert(sizeof(T) <= Capacity, "[TOOLKIT][STATIC-STORAGE] Object does not fit in the local buffer");
        static_assert(alignof(T) <= Alignment, "[TOOLKIT][STATIC-STORAGE] Object has incompatible alignment");
        return *TKit::Construct(&Get<T>(), std::forward<Args>(args)...);
    }

    template <typename T> constexpr void Destruct() const
    {
        static_assert(sizeof(T) <= Capacity, "[TOOLKIT][STATIC-STORAGE] Object does not fit in the local buffer");
        static_assert(alignof(T) <= Alignment, "[TOOLKIT][STATIC-STORAGE] Object has incompatible alignment");
        if constexpr (!std::is_trivially_destructible_v<T>)
            TKit::Destruct(&Get<T>());
    }

    template <typename T> constexpr const T &Get() const
    {
        return *rcast<const T *>(m_Data);
    }

    template <typename T> constexpr T &Get()
    {
        return *rcast<T *>(m_Data);
    }

    constexpr usize GetCapacity() const
    {
        return Capacity;
    }
    constexpr usize GetAlignment() const
    {
        return Alignment;
    }

  private:
    alignas(Alignment) std::byte m_Data[Capacity];
};

class DynamicStorage
{
    TKIT_NON_COPYABLE(DynamicStorage)
  public:
    static constexpr StorageType Type = Storage_Dynamic;

    DynamicStorage() = default;
    DynamicStorage(const usz capacity, const usize alignment = alignof(std::max_align_t))
        : m_Capacity(capacity), m_Alignment(alignment)
    {
        m_Data = scast<std::byte *>(AllocateAligned(capacity, alignment));
    }

    DynamicStorage(DynamicStorage &&other)
        : m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_Alignment(other.m_Alignment)
    {
        other.m_Capacity = 0;
        other.m_Alignment = 0;
        other.m_Data = nullptr;
    }

    ~DynamicStorage()
    {
        DeallocateAligned(m_Data);
    }

    DynamicStorage &operator=(DynamicStorage &&other)
    {
        if (&other != this)
        {
            DeallocateAligned(m_Data);
            m_Data = other.m_Data;
            m_Capacity = other.m_Capacity;
            m_Alignment = other.m_Alignment;
            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_Alignment = 0;
        }
        return *this;
    }

    template <typename T, typename... Args> constexpr T &Construct(Args &&...args)
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][DYNAMIC-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})",
                    sizeof(T), m_Capacity);
        TKIT_ENSURE(
            alignof(T) <= m_Alignment,
            "[TOOLKIT][DYNAMIC-STORAGE] Object (alignment: {}) has incompatible alignment with the buffer's ({})",
            alignof(T), m_Alignment);
        return *TKit::Construct(&Get<T>(), std::forward<Args>(args)...);
    }

    template <typename T> constexpr void Destruct() const
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][DYNAMIC-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})",
                    sizeof(T), m_Capacity);
        TKIT_ENSURE(
            alignof(T) <= m_Alignment,
            "[TOOLKIT][DYNAMIC-STORAGE] Object (alignment: {}) has incompatible alignment with the buffer's ({})",
            alignof(T), m_Alignment);
        if constexpr (!std::is_trivially_destructible_v<T>)
            TKit::Destruct(&Get<T>());
    }

    template <typename T> constexpr const T &Get() const
    {
        return *rcast<const T *>(m_Data);
    }

    template <typename T> constexpr T &Get()
    {
        return *rcast<T *>(m_Data);
    }

    constexpr operator bool() const
    {
        return m_Data;
    }

    constexpr usz GetCapacity() const
    {
        return m_Capacity;
    }
    constexpr usize GetAlignment() const
    {
        return m_Alignment;
    }

  private:
    std::byte *m_Data = nullptr;
    usz m_Capacity = 0;
    usize m_Alignment = 0;
};

class ArenaStorage
{
    TKIT_NON_COPYABLE(ArenaStorage)
  public:
    static constexpr StorageType Type = Storage_Arena;
    ArenaStorage() = default;
    ArenaStorage(const usz capacity) : ArenaStorage(GetArena(), capacity)
    {
    }
    ArenaStorage(ArenaAllocator *alloc, const usz capacity)
        : m_Data(scast<std::byte *>(alloc->Allocate(capacity))), m_Capacity(capacity)
    {
    }

    ArenaStorage(ArenaStorage &&other) : m_Data(other.m_Data), m_Capacity(other.m_Capacity)
    {
        other.m_Data = nullptr;
        other.m_Capacity = 0;
    }

    ArenaStorage &operator=(ArenaStorage &&other)
    {
        if (this != &other)
        {
            m_Data = other.m_Data;
            m_Capacity = other.m_Capacity;
            other.m_Data = nullptr;
            other.m_Capacity = 0;
        }
        return *this;
    }

    template <typename T, typename... Args> constexpr T &Construct(Args &&...args)
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][ARENA-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})", sizeof(T),
                    m_Capacity);
        return *TKit::Construct(&Get<T>(), std::forward<Args>(args)...);
    }

    template <typename T> constexpr void Destruct() const
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][ARENA-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})", sizeof(T),
                    m_Capacity);
        if constexpr (!std::is_trivially_destructible_v<T>)
            TKit::Destruct(&Get<T>());
    }

    template <typename T> constexpr const T &Get() const
    {
        return *rcast<const T *>(m_Data);
    }

    template <typename T> constexpr T &Get()
    {
        return *rcast<T *>(m_Data);
    }

    constexpr operator bool() const
    {
        return m_Data;
    }

    constexpr usz GetCapacity() const
    {
        return m_Capacity;
    }

  private:
    std::byte *m_Data = nullptr;
    usz m_Capacity = 0;
};

class StackStorage
{
    TKIT_NON_COPYABLE(StackStorage)
  public:
    static constexpr StorageType Type = Storage_Stack;
    StackStorage() = default;
    StackStorage(const usz capacity) : StackStorage(GetStack(), capacity)
    {
    }
    StackStorage(StackAllocator *alloc, const usz capacity)
        : m_Data(scast<std::byte *>(alloc->Allocate(capacity))), m_Allocator(alloc), m_Capacity(capacity)
    {
    }

    ~StackStorage()
    {
        if (m_Data)
            m_Allocator->Deallocate(scast<void *>(m_Data), m_Capacity);
    }

    StackStorage(StackStorage &&other)
        : m_Data(other.m_Data), m_Allocator(other.m_Allocator), m_Capacity(other.m_Capacity)
    {
        other.m_Data = nullptr;
        other.m_Allocator = nullptr;
        other.m_Capacity = 0;
    }

    StackStorage &operator=(StackStorage &&other)
    {
        if (this != &other)
        {
            if (m_Data)
                m_Allocator->Deallocate(scast<void *>(m_Data), m_Capacity);
            m_Data = other.m_Data;
            m_Allocator = other.m_Allocator;
            m_Capacity = other.m_Capacity;
            other.m_Data = nullptr;
            other.m_Allocator = nullptr;
            other.m_Capacity = 0;
        }
        return *this;
    }

    template <typename T, typename... Args> constexpr T &Construct(Args &&...args)
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][STACK-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})", sizeof(T),
                    m_Capacity);
        return *TKit::Construct(&Get<T>(), std::forward<Args>(args)...);
    }

    template <typename T> constexpr void Destruct() const
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][STACK-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})", sizeof(T),
                    m_Capacity);
        if constexpr (!std::is_trivially_destructible_v<T>)
            TKit::Destruct(&Get<T>());
    }

    template <typename T> constexpr const T &Get() const
    {
        return *rcast<const T *>(m_Data);
    }

    template <typename T> constexpr T &Get()
    {
        return *rcast<T *>(m_Data);
    }

    constexpr operator bool() const
    {
        return m_Data;
    }
    constexpr usz GetCapacity() const
    {
        return m_Capacity;
    }

  private:
    std::byte *m_Data = nullptr;
    StackAllocator *m_Allocator = nullptr;
    usz m_Capacity = 0;
};

class TierStorage
{
    TKIT_NON_COPYABLE(TierStorage)
  public:
    static constexpr StorageType Type = Storage_Tier;

    TierStorage() = default;
    TierStorage(const usz capacity) : TierStorage(GetTier(), capacity)
    {
    }
    TierStorage(TierAllocator *alloc, const usz capacity)
        : m_Data(scast<std::byte *>(alloc->Allocate(capacity))), m_Allocator(alloc), m_Capacity(capacity)
    {
    }

    ~TierStorage()
    {
        if (m_Data)
            m_Allocator->Deallocate(scast<void *>(m_Data), m_Capacity);
    }

    TierStorage(TierStorage &&other)
        : m_Data(other.m_Data), m_Allocator(other.m_Allocator), m_Capacity(other.m_Capacity)
    {
        other.m_Data = nullptr;
        other.m_Allocator = nullptr;
        other.m_Capacity = 0;
    }

    TierStorage &operator=(TierStorage &&other)
    {
        if (this != &other)
        {
            if (m_Data)
                m_Allocator->Deallocate(scast<void *>(m_Data), m_Capacity);
            m_Data = other.m_Data;
            m_Allocator = other.m_Allocator;
            m_Capacity = other.m_Capacity;
            other.m_Data = nullptr;
            other.m_Allocator = nullptr;
            other.m_Capacity = 0;
        }
        return *this;
    }

    template <typename T, typename... Args> constexpr T &Construct(Args &&...args)
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][TIER-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})", sizeof(T),
                    m_Capacity);
        return *TKit::Construct(&Get<T>(), std::forward<Args>(args)...);
    }

    template <typename T> constexpr void Destruct() const
    {
        TKIT_ENSURE(sizeof(T) <= m_Capacity,
                    "[TOOLKIT][TIER-STORAGE] Object (size: {}) does not fit in the local buffer (size: {})", sizeof(T),
                    m_Capacity);
        if constexpr (!std::is_trivially_destructible_v<T>)
            TKit::Destruct(&Get<T>());
    }

    template <typename T> constexpr const T &Get() const
    {
        return *rcast<const T *>(m_Data);
    }

    template <typename T> constexpr T &Get()
    {
        return *rcast<T *>(m_Data);
    }

    constexpr operator bool() const
    {
        return m_Data;
    }
    constexpr usz GetCapacity() const
    {
        return m_Capacity;
    }

  private:
    std::byte *m_Data = nullptr;
    TierAllocator *m_Allocator = nullptr;
    usz m_Capacity = 0;
};

template <typename T> class Storage
{
  public:
    template <typename... Args> constexpr T &Construct(Args &&...args)
    {
        return m_Storage.template Construct<T>(std::forward<Args>(args)...);
    }

    constexpr void Destruct() const
    {
        m_Storage.template Destruct<T>();
    }

    constexpr const T &Get() const
    {
        return m_Storage.template Get<T>();
    }
    constexpr T &Get()
    {
        return m_Storage.template Get<T>();
    }

    constexpr const T *operator->() const
    {
        return &Get();
    }
    constexpr T *operator->()
    {
        return &Get();
    }

    constexpr const T &operator*() const
    {
        return Get();
    }
    constexpr T &operator*()
    {
        return Get();
    }

  private:
    StaticStorage<sizeof(T), alignof(T)> m_Storage;
};

}; // namespace TKit
