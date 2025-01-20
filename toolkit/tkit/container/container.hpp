#pragma once

#include "tkit/container/alias.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/core/logging.hpp"

namespace TKit::Detail
{
template <typename Traits> struct Container
{
    using value_type = typename Traits::value_type;
    using size_type = typename Traits::size_type;
    using difference_type = typename Traits::difference_type;
    using pointer = typename Traits::pointer;
    using const_pointer = typename Traits::const_pointer;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * @brief Insert a new element at the specified position. The element is copied or moved into the array.
     *
     * @param p_End The end iterator of the array.
     * @param p_Pos The position to insert the element at.
     * @param p_Value The value to insert.
     */
    template <typename T> static void Insert(const iterator p_End, const iterator p_Pos, T &&p_Value) noexcept
    {
        if (p_Pos == p_End) [[unlikely]]
        {
            Memory::ConstructFromIterator(p_Pos, std::forward<T>(p_Value));
            return;
        }

        if constexpr (!std::is_trivially_constructible_v<value_type>)
        { // Current p_End pointer is uninitialized, so it must be handled manually
            Memory::ConstructFromIterator(p_End, std::move(*(p_End - 1)));

            if (const iterator shiftedEnd = p_End - 1; p_Pos < shiftedEnd)
                std::move_backward(p_Pos, shiftedEnd, p_End);
        }
        else
            std::move_backward(p_Pos, p_End, p_End + 1);

        if constexpr (!std::is_trivially_destructible_v<value_type>)
            Memory::DestructFromIterator(p_Pos);
        Memory::ConstructFromIterator(p_Pos, std::forward<T>(p_Value));
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_End The end iterator of the array.
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     * @return The amount of elements inserted.
     */
    template <typename It>
    static size_type Insert(const iterator p_End, iterator p_Pos, It p_SrcBegin, It p_SrcEnd) noexcept
    {
        TKIT_ASSERT(p_SrcBegin <= p_SrcEnd, "[TOOLKIT] Begin iterator is greater than end iterator");
        if (p_Pos == p_End)
        {
            for (auto it = p_SrcBegin; it != p_SrcEnd; ++it)
                Memory::ConstructFromIterator(p_Pos++, *it);
            return static_cast<size_type>(std::distance(p_SrcBegin, p_SrcEnd));
        }

        // This method is a bit verbose, but it has many edge cases to handle:
        // If the amount of elements to insert is small (ie, less than the trailing end), then the out of bound elements
        // must be copy-moved to the end of the array.

        // If the amount of elements to insert is bigger than the trailing end, then the trailing end must be copy-moved
        // to the end of the array, and the out of bound inserted elements must be copied as well to the remaining, left
        // portion of the out of bounds array.

        // The remaining elements inside the original array must then be shifted, in case the amount of elements to
        // insert was small

        const size_type trail = static_cast<size_type>(std::distance(p_Pos, p_End));
        const size_type count = static_cast<size_type>(std::distance(p_SrcBegin, p_SrcEnd));
        const size_type outOfBounds = count < trail ? count : trail;

        // Current p_End + outOfBounds pointers are uninitialized, so they must be handled manually
        for (size_type i = 0; i < count; ++i)
        {
            const size_type idx1 = count - i - 1;
            if (i < outOfBounds)
            {
                const size_type idx2 = outOfBounds - i - 1;
                Memory::ConstructFromIterator(p_End + idx1, std::move(*(p_Pos + idx2)));
            }
            else
                Memory::ConstructFromIterator(p_End + idx1, *(--p_SrcEnd));
        }

        if (const iterator shiftedEnd = p_End - count; p_Pos < shiftedEnd)
            std::move_backward(p_Pos, shiftedEnd, p_End);
        for (size_type i = 0; i < outOfBounds; ++i)
        {
            if constexpr (!std::is_trivially_destructible_v<value_type>)
                Memory::DestructFromIterator(p_Pos + i);
            Memory::ConstructFromIterator(p_Pos + i, *(p_SrcBegin++));
        }

        return count;
    }
    static void Erase(const iterator p_End, const iterator p_Pos) noexcept
    {
        // Copy/move the elements after the erased one
        std::move(p_Pos + 1, p_End, p_Pos);
        // And destroy the last element
        if constexpr (!std::is_trivially_destructible_v<value_type>)
            Memory::DestructFromIterator(p_End - 1);
    }

    static size_type Erase(const iterator p_End, const iterator p_RemBegin, const iterator p_RemEnd) noexcept
    {
        TKIT_ASSERT(p_RemBegin <= p_RemEnd, "[TOOLKIT] Begin iterator is greater than end iterator");
        // Copy the elements after the erased ones
        std::move(p_RemEnd, p_End, p_RemBegin);

        const size_type count = static_cast<size_type>(std::distance(p_RemBegin, p_RemEnd));
        // And destroy the last elements
        if constexpr (!std::is_trivially_destructible_v<value_type>)
            Memory::DestructRange(p_End - count, p_End);
        return count;
    }
};

} // namespace TKit::Detail