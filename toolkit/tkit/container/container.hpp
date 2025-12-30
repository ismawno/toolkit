#pragma once

#include "tkit/memory/memory.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit::Container
{

template <typename T>
concept Iterable = requires(T a) {
    { a.begin() } -> std::input_iterator;
    { a.end() } -> std::input_iterator;
};

template <typename T, typename SType = usize> struct ArrayTraits
{
    using ValueType = T;
    using SizeType = SType;
    using Iterator = T *;
    using ConstIterator = const T *;
};

template <typename Traits> struct ArrayTools
{
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;

    template <typename It>
    static constexpr void CopyConstructFromRange(const Iterator p_DstBegin, const It p_SrcBegin, const It p_SrcEnd)
    {
        using T = decltype(*p_SrcBegin);
        if constexpr (std::is_trivially_copyable_v<ValueType> &&
                      std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<ValueType>>)
            Memory::ForwardCopy(p_DstBegin, p_SrcBegin, p_SrcEnd);
        else
            Memory::ConstructRangeCopy(p_DstBegin, p_SrcBegin, p_SrcEnd);
    }
    template <typename It>
    static constexpr void MoveConstructFromRange(const Iterator p_DstBegin, const It p_SrcBegin, const It p_SrcEnd)
    {
        using T = decltype(*p_SrcBegin);
        if constexpr (std::is_trivially_copyable_v<ValueType> && std::is_trivially_move_constructible_v<ValueType> &&
                      std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<ValueType>>)
            Memory::ForwardMove(p_DstBegin, p_SrcBegin, p_SrcEnd);
        else
            Memory::ConstructRangeMove(p_DstBegin, p_SrcBegin, p_SrcEnd);
    }

    template <typename It>
    static constexpr void CopyAssignFromRange(const Iterator p_DstBegin, const Iterator p_DstEnd, const It p_SrcBegin,
                                              const It p_SrcEnd)
    {
        const SizeType dstSize = static_cast<SizeType>(std::distance(p_DstBegin, p_DstEnd));
        const SizeType srcSize = static_cast<SizeType>(std::distance(p_SrcBegin, p_SrcEnd));

        using T = decltype(*p_SrcBegin);
        if constexpr (std::is_trivially_copyable_v<ValueType> &&
                      std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<ValueType>>)
        {
            Memory::ForwardCopy(p_DstBegin, p_SrcBegin, p_SrcEnd);
            if constexpr (!std::is_trivially_destructible_v<ValueType>)
            {
                if (srcSize < dstSize)
                    Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
            }
        }
        else
        {
            const SizeType minSize = dstSize < srcSize ? dstSize : srcSize;
            Memory::ForwardCopy(p_DstBegin, p_SrcBegin, p_SrcBegin + minSize);

            if (srcSize > dstSize)
                Memory::ConstructRangeCopy(p_DstEnd, p_SrcBegin + dstSize, p_SrcEnd);
            else if (srcSize < dstSize)
                Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
        }
    }
    template <typename It>
    static constexpr void MoveAssignFromRange(const Iterator p_DstBegin, const Iterator p_DstEnd, const It p_SrcBegin,
                                              const It p_SrcEnd)
    {
        const SizeType dstSize = static_cast<SizeType>(std::distance(p_DstBegin, p_DstEnd));
        const SizeType srcSize = static_cast<SizeType>(std::distance(p_SrcBegin, p_SrcEnd));
        if constexpr (std::is_trivially_copyable_v<ValueType>)
        {
            Memory::ForwardMove(p_DstBegin, p_SrcBegin, p_SrcEnd);
            if constexpr (!std::is_trivially_destructible_v<ValueType>)
            {
                if (srcSize < dstSize)
                    Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
            }
        }
        else
        {
            const SizeType minSize = dstSize < srcSize ? dstSize : srcSize;
            Memory::ForwardMove(p_DstBegin, p_SrcBegin, p_SrcBegin + minSize);

            if (srcSize > dstSize)
                Memory::ConstructRangeMove(p_DstEnd, p_SrcBegin + dstSize, p_SrcEnd);
            else if (srcSize < dstSize)
                Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
        }
    }

    template <typename T> static constexpr void Insert(const Iterator p_End, const Iterator p_Pos, T &&p_Value)
    {
        if (p_Pos == p_End) [[unlikely]]
        {
            Memory::ConstructFromIterator(p_Pos, std::forward<T>(p_Value));
            return;
        }

        if constexpr (!std::is_trivially_constructible_v<ValueType>)
        { // Current p_End pointer is uninitialized, so it must be handled manually
            Memory::ConstructFromIterator(p_End, std::move(*(p_End - 1)));

            if (const Iterator shiftedEnd = p_End - 1; p_Pos < shiftedEnd)
                Memory::BackwardMove(p_End, p_Pos, shiftedEnd);
        }
        else
            Memory::BackwardMove(p_End + 1, p_Pos, p_End);

        *p_Pos = std::forward<T>(p_Value);
    }

    template <typename It>
    static constexpr SizeType Insert(const Iterator p_End, const Iterator p_Pos, const It p_SrcBegin, const It p_SrcEnd)
    {
        TKIT_ASSERT(p_SrcBegin <= p_SrcEnd, "[TOOLKIT][CONTAINER] Begin iterator is greater than end iterator");
        if (p_Pos == p_End)
        {
            CopyConstructFromRange(p_Pos, p_SrcBegin, p_SrcEnd);
            return static_cast<SizeType>(std::distance(p_SrcBegin, p_SrcEnd));
        }
        const SizeType tail = static_cast<SizeType>(std::distance(p_Pos, p_End));
        const SizeType count = static_cast<SizeType>(std::distance(p_SrcBegin, p_SrcEnd));
        if (tail > count)
        {
            const Iterator overflowSrcBegin = p_End - count;
            const Iterator overflowSrcEnd = p_End;
            const Iterator overflowDstBegin = p_End;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            const Iterator inBetweenSrcBegin = p_Pos;
            const Iterator inBetweenSrcEnd = p_Pos + (tail - count);
            const Iterator inBetweenDstEnd = p_End;
            Memory::BackwardMove(inBetweenDstEnd, inBetweenSrcBegin, inBetweenSrcEnd);

            Memory::ForwardCopy(p_Pos, p_SrcBegin, p_SrcEnd);
        }
        else if (tail < count)
        {
            const Iterator overflowSrcBegin = p_Pos;
            const Iterator overflowSrcEnd = p_End;
            const Iterator overflowDstBegin = p_Pos + count;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            const It inBetweenSrcBegin = p_SrcBegin + tail;
            const It inBetweenSrcEnd = p_SrcEnd;
            const Iterator inBetweenDstEnd = p_End;
            CopyConstructFromRange(inBetweenDstEnd, inBetweenSrcBegin, inBetweenSrcEnd);

            Memory::ForwardCopy(p_Pos, p_SrcBegin, p_SrcBegin + tail);
        }
        else
        {
            const Iterator overflowSrcBegin = p_Pos;
            const Iterator overflowSrcEnd = p_End;
            const Iterator overflowDstBegin = p_End;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            Memory::ForwardCopy(p_Pos, p_SrcBegin, p_SrcEnd);
        }
        return count;
    }

    static constexpr SizeType GrowthFactor(const SizeType p_Size)
    {
        return 1 + p_Size + p_Size / 2;
    }

    static constexpr void RemoveOrdered(const Iterator p_End, const Iterator p_Pos)
    {
        // Copy/move the elements after the erased one
        Memory::ForwardMove(p_Pos, p_Pos + 1, p_End);
        // And destroy the last element
        if constexpr (!std::is_trivially_destructible_v<ValueType>)
            Memory::DestructFromIterator(p_End - 1);
    }

    static constexpr SizeType RemoveOrdered(const Iterator p_End, const Iterator p_RemBegin, const Iterator p_RemEnd)
    {
        TKIT_ASSERT(p_RemBegin <= p_RemEnd, "[TOOLKIT][CONTAINER] Begin iterator is greater than end iterator");
        // Copy/move the elements after the erased ones
        Memory::ForwardMove(p_RemBegin, p_RemEnd, p_End);

        const SizeType count = static_cast<SizeType>(std::distance(p_RemBegin, p_RemEnd));
        // And destroy the last elements
        if constexpr (!std::is_trivially_destructible_v<ValueType>)
            Memory::DestructRange(p_End - count, p_End);
        return count;
    }

    static constexpr void RemoveUnordered(const Iterator p_End, const Iterator p_Pos)
    {
        const Iterator last = p_End - 1;
        *p_Pos = std::move(*last);
        if constexpr (!std::is_trivially_destructible_v<ValueType>)
            Memory::DestructFromIterator(last);
    }
};

} // namespace TKit::Container
