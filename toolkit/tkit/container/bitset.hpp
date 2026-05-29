#pragma once

#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"

namespace TKit
{
template <typename AllocState> class BitSet
{
  public:
    using ValueType = u64;
    static constexpr usize Exponent = 6;
    static constexpr usize BitMask = (1 << Exponent) - 1;

    BitSet() = default;
    BitSet(const usize bits) : m_Bits(bits == 0 ? 0 : (asLane(bits - 1) + 1))
    {
        ClearAll();
    }

    constexpr BitSet(const BitSet &) = default;
    constexpr BitSet(BitSet &&) = default;

    constexpr BitSet &operator=(const BitSet &) = default;
    constexpr BitSet &operator=(BitSet &&) = default;

    template <typename OtherAlloc> constexpr BitSet(const BitSet<OtherAlloc> &other) : m_Bits(other.m_Bits)
    {
    }

    template <typename OtherAlloc> constexpr BitSet(BitSet<OtherAlloc> &&other) : m_Bits(std::move(other.m_Bits))
    {
    }

    template <typename OtherAlloc> constexpr BitSet &operator=(const BitSet<OtherAlloc> &other)
    {
        m_Bits = other.m_Bits;
        return *this;
    }

    template <typename OtherAlloc> constexpr BitSet &operator=(BitSet<OtherAlloc> &&other)
    {
        m_Bits = std::move(other.m_Bits);
        return *this;
    }

    constexpr bool At(const usize index) const
    {
        const usize lane = asLane(index);
        const usize bit = asLocalBit(index);

        TKIT_ASSERT(lane < m_Bits.GetSize(),
                    "[TOOLKIT][BITSET] Bit index {} matching lane {} exceeds total lane count ({})", index, lane,
                    m_Bits.GetSize());

        return (m_Bits[lane] >> bit) & 1;
    }

    constexpr bool operator[](const usize index) const
    {
        return At(index);
    }
    constexpr bool Get(const usize index) const
    {
        return At(index);
    }
    constexpr void Set(const usize index)
    {
        const usize lane = asLane(index);
        const usize bit = asLocalBit(index);

        TKIT_ASSERT(lane < m_Bits.GetSize(),
                    "[TOOLKIT][BITSET] Bit index {} matching lane {} exceeds total lane count ({})", index, lane,
                    m_Bits.GetSize());
        m_Bits[lane] |= 1ULL << bit;
    }
    constexpr void Clear(const usize index)
    {
        const usize lane = asLane(index);
        const usize bit = asLocalBit(index);

        TKIT_ASSERT(lane < m_Bits.GetSize(),
                    "[TOOLKIT][BITSET] Bit index {} matching lane {} exceeds total lane count ({})", index, lane,
                    m_Bits.GetSize());
        m_Bits[lane] &= ~(1ULL << bit);
    }
    constexpr void Clear()
    {
        m_Bits.Clear();
    }
    constexpr void ClearAll()
    {
        for (u64 &b : m_Bits)
            b = 0;
    }

    constexpr void Reserve(const usize capacity)
    {
        if (capacity != 0)
            m_Bits.Reserve(asLane(capacity - 1) + 1);
    }
    constexpr void Resize(const usize size)
    {
        if (size == 0)
        {
            m_Bits.Clear();
            return;
        }
        const usize old = m_Bits.GetSize();
        m_Bits.Resize(asLane(size) + 1);
        for (usize i = old; i < m_Bits.GetSize(); ++i)
            m_Bits[i] = 0;
    }

    constexpr const u64 *GetData() const
    {
        return m_Bits.GetData();
    }
    constexpr u64 *GetData()
    {
        return m_Bits.GetData();
    }

    // NOTE(Isma): Poor iteration method
    constexpr u64 *begin()
    {
        return m_Bits.begin();
    }
    constexpr u64 *end()
    {
        return m_Bits.end();
    }

    constexpr const u64 *begin() const
    {
        return m_Bits.begin();
    }
    constexpr const u64 *end() const
    {
        return m_Bits.end();
    }

    constexpr usize GetSize() const
    {
        return asGlobalBit(m_Bits.GetSize());
    }
    constexpr usize GetCapacity() const
    {
        return asGlobalBit(m_Bits.GetCapacity());
    }
    constexpr usz GetBytes() const
    {
        return m_Bits.GetSize() * sizeof(u64);
    }

  private:
    static constexpr usize asLane(const usize globalBit)
    {
        return globalBit >> Exponent;
    }
    static constexpr usize asLocalBit(const usize globalBit)
    {
        return globalBit & BitMask;
    }
    static constexpr usize asGlobalBit(const usize lanes)
    {
        return lanes << Exponent;
    }

    Array<u64, AllocState> m_Bits{};

    template <typename OtherAlloc> friend class BitSet;
};

using ArenaBitSet = BitSet<ArenaAllocation<u64>>;
using DynamicBitSet = BitSet<DynamicAllocation<u64>>;
using StackBitSet = BitSet<StackAllocation<u64>>;
using TierBitSet = BitSet<TierAllocation<u64>>;

template <usize Capacity> using StaticBitSet = BitSet<StaticAllocation<u64, 1 + Capacity / 64>>;
using StaticBitSet256 = StaticBitSet<256>;
using StaticBitSet512 = StaticBitSet<512>;
using StaticBitSet1024 = StaticBitSet<1024>;
using StaticBitSet2048 = StaticBitSet<2048>;
using StaticBitSet4096 = StaticBitSet<4096>;
using StaticBitSet8192 = StaticBitSet<8192>;
using StaticBitSet16384 = StaticBitSet<16384>;
using StaticBitSet32768 = StaticBitSet<32768>;
using StaticBitSet65536 = StaticBitSet<65536>;
} // namespace TKit
