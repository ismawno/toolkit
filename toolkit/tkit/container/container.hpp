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

template <typename T> struct ArrayTools
{
    using ValueType = T;

    template <typename It>
    static constexpr void CopyConstructFromRange(T *p_DstBegin, const It p_SrcBegin, const It p_SrcEnd)
    {
        using U = decltype(*p_SrcBegin);
        if constexpr (std::is_trivially_copyable_v<T> && std::is_same_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
            Memory::ForwardCopy(p_DstBegin, p_SrcBegin, p_SrcEnd);
        else
            Memory::ConstructRangeCopy(p_DstBegin, p_SrcBegin, p_SrcEnd);
    }
    template <typename It>
    static constexpr void MoveConstructFromRange(T *p_DstBegin, const It p_SrcBegin, const It p_SrcEnd)
    {
        using U = decltype(*p_SrcBegin);
        if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_move_constructible_v<T> &&
                      std::is_same_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
            Memory::ForwardMove(p_DstBegin, p_SrcBegin, p_SrcEnd);
        else
            Memory::ConstructRangeMove(p_DstBegin, p_SrcBegin, p_SrcEnd);
    }

    template <typename It>
    static constexpr void CopyAssignFromRange(T *p_DstBegin, T *p_DstEnd, const It p_SrcBegin, const It p_SrcEnd)
    {
        const usize dstSize = static_cast<usize>(std::distance(p_DstBegin, p_DstEnd));
        const usize srcSize = static_cast<usize>(std::distance(p_SrcBegin, p_SrcEnd));

        using U = decltype(*p_SrcBegin);
        if constexpr (std::is_trivially_copyable_v<T> && std::is_same_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
        {
            Memory::ForwardCopy(p_DstBegin, p_SrcBegin, p_SrcEnd);
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                if (srcSize < dstSize)
                    Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
            }
        }
        else
        {
            const usize minSize = dstSize < srcSize ? dstSize : srcSize;
            Memory::ForwardCopy(p_DstBegin, p_SrcBegin, p_SrcBegin + minSize);

            if (srcSize > dstSize)
                Memory::ConstructRangeCopy(p_DstEnd, p_SrcBegin + dstSize, p_SrcEnd);
            else if (srcSize < dstSize)
                Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
        }
    }
    template <typename It>
    static constexpr void MoveAssignFromRange(T *p_DstBegin, T *p_DstEnd, const It p_SrcBegin, const It p_SrcEnd)
    {
        const usize dstSize = static_cast<usize>(std::distance(p_DstBegin, p_DstEnd));
        const usize srcSize = static_cast<usize>(std::distance(p_SrcBegin, p_SrcEnd));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            Memory::ForwardMove(p_DstBegin, p_SrcBegin, p_SrcEnd);
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                if (srcSize < dstSize)
                    Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
            }
        }
        else
        {
            const usize minSize = dstSize < srcSize ? dstSize : srcSize;
            Memory::ForwardMove(p_DstBegin, p_SrcBegin, p_SrcBegin + minSize);

            if (srcSize > dstSize)
                Memory::ConstructRangeMove(p_DstEnd, p_SrcBegin + dstSize, p_SrcEnd);
            else if (srcSize < dstSize)
                Memory::DestructRange(p_DstBegin + srcSize, p_DstEnd);
        }
    }

    template <typename U> static constexpr void Insert(T *p_End, T *p_Pos, U &&p_Value)
    {
        if (p_Pos == p_End) [[unlikely]]
        {
            Memory::ConstructFromIterator(p_Pos, std::forward<U>(p_Value));
            return;
        }

        if constexpr (!std::is_trivially_constructible_v<T>)
        { // Current p_End pointer is uninitialized, so it must be handled manually
            Memory::ConstructFromIterator(p_End, std::move(*(p_End - 1)));

            if (T *shiftedEnd = p_End - 1; p_Pos < shiftedEnd)
                Memory::BackwardMove(p_End, p_Pos, shiftedEnd);
        }
        else
            Memory::BackwardMove(p_End + 1, p_Pos, p_End);

        *p_Pos = std::forward<U>(p_Value);
    }

    template <typename It> static constexpr usize Insert(T *p_End, T *p_Pos, const It p_SrcBegin, const It p_SrcEnd)
    {
        TKIT_ASSERT(p_SrcBegin <= p_SrcEnd, "[TOOLKIT][CONTAINER] Begin iterator is greater than end iterator");
        if (p_Pos == p_End)
        {
            CopyConstructFromRange(p_Pos, p_SrcBegin, p_SrcEnd);
            return static_cast<usize>(std::distance(p_SrcBegin, p_SrcEnd));
        }
        const usize tail = static_cast<usize>(std::distance(p_Pos, p_End));
        const usize count = static_cast<usize>(std::distance(p_SrcBegin, p_SrcEnd));
        if (tail > count)
        {
            T *overflowSrcBegin = p_End - count;
            T *overflowSrcEnd = p_End;
            T *overflowDstBegin = p_End;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            T *inBetweenSrcBegin = p_Pos;
            T *inBetweenSrcEnd = p_Pos + (tail - count);
            T *inBetweenDstEnd = p_End;
            Memory::BackwardMove(inBetweenDstEnd, inBetweenSrcBegin, inBetweenSrcEnd);

            Memory::ForwardCopy(p_Pos, p_SrcBegin, p_SrcEnd);
        }
        else if (tail < count)
        {
            T *overflowSrcBegin = p_Pos;
            T *overflowSrcEnd = p_End;
            T *overflowDstBegin = p_Pos + count;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            const It inBetweenSrcBegin = p_SrcBegin + tail;
            const It inBetweenSrcEnd = p_SrcEnd;
            T *inBetweenDstEnd = p_End;
            CopyConstructFromRange(inBetweenDstEnd, inBetweenSrcBegin, inBetweenSrcEnd);

            Memory::ForwardCopy(p_Pos, p_SrcBegin, p_SrcBegin + tail);
        }
        else
        {
            T *overflowSrcBegin = p_Pos;
            T *overflowSrcEnd = p_End;
            T *overflowDstBegin = p_End;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            Memory::ForwardCopy(p_Pos, p_SrcBegin, p_SrcEnd);
        }
        return count;
    }

    static constexpr usize GrowthFactor(const usize p_Size)
    {
        return 1 + p_Size + p_Size / 2;
    }

    static constexpr void RemoveOrdered(T *p_End, T *p_Pos)
    {
        // Copy/move the elements after the erased one
        Memory::ForwardMove(p_Pos, p_Pos + 1, p_End);
        // And destroy the last element
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(p_End - 1);
    }

    static constexpr usize RemoveOrdered(T *p_End, T *p_RemBegin, T *p_RemEnd)
    {
        TKIT_ASSERT(p_RemBegin <= p_RemEnd, "[TOOLKIT][CONTAINER] Begin iterator is greater than end iterator");
        // Copy/move the elements after the erased ones
        Memory::ForwardMove(p_RemBegin, p_RemEnd, p_End);

        const usize count = static_cast<usize>(std::distance(p_RemBegin, p_RemEnd));
        // And destroy the last elements
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(p_End - count, p_End);
        return count;
    }

    static constexpr void RemoveUnordered(T *p_End, T *p_Pos)
    {
        T *last = p_End - 1;
        *p_Pos = std::move(*last);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(last);
    }
};

} // namespace TKit::Container
