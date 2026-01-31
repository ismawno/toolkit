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

    template <typename It> static constexpr void CopyConstructFromRange(T *dstBegin, const It srcBegin, const It srcEnd)
    {
        using U = decltype(*srcBegin);
        if constexpr (std::is_trivially_copyable_v<T> && std::is_same_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
            ForwardCopy(dstBegin, srcBegin, srcEnd);
        else
            ConstructRangeCopy(dstBegin, srcBegin, srcEnd);
    }
    template <typename It> static constexpr void MoveConstructFromRange(T *dstBegin, const It srcBegin, const It srcEnd)
    {
        using U = decltype(*srcBegin);
        if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_move_constructible_v<T> &&
                      std::is_same_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
            ForwardMove(dstBegin, srcBegin, srcEnd);
        else
            ConstructRangeMove(dstBegin, srcBegin, srcEnd);
    }

    template <typename It>
    static constexpr void CopyAssignFromRange(T *dstBegin, T *dstEnd, const It srcBegin, const It srcEnd)
    {
        const usize dstSize = static_cast<usize>(std::distance(dstBegin, dstEnd));
        const usize srcSize = static_cast<usize>(std::distance(srcBegin, srcEnd));

        using U = decltype(*srcBegin);
        if constexpr (std::is_trivially_copyable_v<T> && std::is_same_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
        {
            ForwardCopy(dstBegin, srcBegin, srcEnd);
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                if (srcSize < dstSize)
                    DestructRange(dstBegin + srcSize, dstEnd);
            }
        }
        else
        {
            const usize minSize = dstSize < srcSize ? dstSize : srcSize;
            ForwardCopy(dstBegin, srcBegin, srcBegin + minSize);

            if (srcSize > dstSize)
                ConstructRangeCopy(dstEnd, srcBegin + dstSize, srcEnd);
            else if (srcSize < dstSize)
                DestructRange(dstBegin + srcSize, dstEnd);
        }
    }
    template <typename It>
    static constexpr void MoveAssignFromRange(T *dstBegin, T *dstEnd, const It srcBegin, const It srcEnd)
    {
        const usize dstSize = static_cast<usize>(std::distance(dstBegin, dstEnd));
        const usize srcSize = static_cast<usize>(std::distance(srcBegin, srcEnd));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            ForwardMove(dstBegin, srcBegin, srcEnd);
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                if (srcSize < dstSize)
                    DestructRange(dstBegin + srcSize, dstEnd);
            }
        }
        else
        {
            const usize minSize = dstSize < srcSize ? dstSize : srcSize;
            ForwardMove(dstBegin, srcBegin, srcBegin + minSize);

            if (srcSize > dstSize)
                ConstructRangeMove(dstEnd, srcBegin + dstSize, srcEnd);
            else if (srcSize < dstSize)
                DestructRange(dstBegin + srcSize, dstEnd);
        }
    }

    template <typename U> static constexpr void Insert(T *end, T *pos, U &&value)
    {
        if (pos == end) [[unlikely]]
        {
            ConstructFromIterator(pos, std::forward<U>(value));
            return;
        }

        if constexpr (!std::is_trivially_constructible_v<T>)
        { // Current end pointer is uninitialized, so it must be handled manually
            ConstructFromIterator(end, std::move(*(end - 1)));

            if (T *shiftedEnd = end - 1; pos < shiftedEnd)
                BackwardMove(end, pos, shiftedEnd);
        }
        else
            BackwardMove(end + 1, pos, end);

        *pos = std::forward<U>(value);
    }

    template <typename It> static constexpr usize Insert(T *end, T *pos, const It srcBegin, const It srcEnd)
    {
        TKIT_ASSERT(srcBegin <= srcEnd, "[TOOLKIT][CONTAINER] Begin iterator is greater than end iterator");
        if (pos == end)
        {
            CopyConstructFromRange(pos, srcBegin, srcEnd);
            return static_cast<usize>(std::distance(srcBegin, srcEnd));
        }
        const usize tail = static_cast<usize>(std::distance(pos, end));
        const usize count = static_cast<usize>(std::distance(srcBegin, srcEnd));
        if (tail > count)
        {
            T *overflowSrcBegin = end - count;
            T *overflowSrcEnd = end;
            T *overflowDstBegin = end;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            T *inBetweenSrcBegin = pos;
            T *inBetweenSrcEnd = pos + (tail - count);
            T *inBetweenDstEnd = end;
            BackwardMove(inBetweenDstEnd, inBetweenSrcBegin, inBetweenSrcEnd);

            ForwardCopy(pos, srcBegin, srcEnd);
        }
        else if (tail < count)
        {
            T *overflowSrcBegin = pos;
            T *overflowSrcEnd = end;
            T *overflowDstBegin = pos + count;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            const It inBetweenSrcBegin = srcBegin + tail;
            const It inBetweenSrcEnd = srcEnd;
            T *inBetweenDstEnd = end;
            CopyConstructFromRange(inBetweenDstEnd, inBetweenSrcBegin, inBetweenSrcEnd);

            ForwardCopy(pos, srcBegin, srcBegin + tail);
        }
        else
        {
            T *overflowSrcBegin = pos;
            T *overflowSrcEnd = end;
            T *overflowDstBegin = end;
            MoveConstructFromRange(overflowDstBegin, overflowSrcBegin, overflowSrcEnd);

            ForwardCopy(pos, srcBegin, srcEnd);
        }
        return count;
    }

    static constexpr usize GrowthFactor(const usize size)
    {
        return 1 + size + size / 2;
    }

    static constexpr void RemoveOrdered(T *end, T *pos)
    {
        // Copy/move the elements after the erased one
        ForwardMove(pos, pos + 1, end);
        // And destroy the last element
        if constexpr (!std::is_trivially_destructible_v<T>)
            DestructFromIterator(end - 1);
    }

    static constexpr usize RemoveOrdered(T *end, T *remBegin, T *remEnd)
    {
        TKIT_ASSERT(remBegin <= remEnd, "[TOOLKIT][CONTAINER] Begin iterator is greater than end iterator");
        // Copy/move the elements after the erased ones
        ForwardMove(remBegin, remEnd, end);

        const usize count = static_cast<usize>(std::distance(remBegin, remEnd));
        // And destroy the last elements
        if constexpr (!std::is_trivially_destructible_v<T>)
            DestructRange(end - count, end);
        return count;
    }

    static constexpr void RemoveUnordered(T *end, T *pos)
    {
        T *last = end - 1;
        *pos = std::move(*last);
        if constexpr (!std::is_trivially_destructible_v<T>)
            DestructFromIterator(last);
    }
};

} // namespace TKit::Container
