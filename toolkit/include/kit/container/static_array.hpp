#pragma once

#include "kit/container/iarray.hpp"
#include <array>

namespace KIT
{

// An STL-like array with a fixed size. It can be used as a replacement for std::array in
// the sense that it offers a bit more control and functionality, but it is not meant to be a drop-in replacement, as
// the extra functionality comes with some overhead.
template <typename T, usize N>
    requires(N > 0)
class StaticArray : public IArray<T, StaticArray<T, N>>
{
  public:
    using BaseArray = IArray<T, StaticArray<T, N>>;

    StaticArray() noexcept = default;

    template <typename... Args> StaticArray(const usize p_Size, Args &&...p_Args) noexcept : BaseArray(p_Size)
    {
        this->Constructor(std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It> StaticArray(It p_Begin, It p_End) noexcept
    {
        this->Constructor(p_Begin, p_End);
    }

    StaticArray(const StaticArray &p_Other) noexcept : BaseArray(p_Other.size())
    {
        this->Constructor(p_Other);
    }

    explicit(false) StaticArray(std::initializer_list<T> p_List) noexcept : BaseArray(p_List.size())
    {
        this->Constructor(p_List);
    }

    // This constructor WONT include the case M == N (ie, copy constructor), because the native copty constructor will
    // always be a perfect match
    template <usize M> explicit(false) StaticArray(const StaticArray<T, M> &p_Other) noexcept
    {
        this->Constructor(p_Other.begin(), p_Other.end());
    }

    ~StaticArray() noexcept
    {
        this->clear();
    }

    StaticArray &operator=(const StaticArray &p_Other) noexcept
    {
        this->CopyAssignment(p_Other);
        return *this;
    }
    StaticArray &operator=(std::initializer_list<T> p_List) noexcept
    {
        this->CopyAssignment(p_List);
        return *this;
    }

    // Same goes for assignment. It wont include M == N, and use the default assignment operator
    template <usize M> StaticArray &operator=(const StaticArray<T, M> &p_Other) noexcept
    {
        if constexpr (M == N)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > N)
        {
            KIT_ASSERT(p_Other.size() <= N, "Size is bigger than capacity");
        }
        this->clear();
        for (usize i = 0; i < p_Other.size(); ++i)
            this->push_back(p_Other[i]);
        return *this;
    }

    constexpr usize capacity() const noexcept
    {
        return N;
    }

    const T *data() const noexcept
    {
        return reinterpret_cast<const T *>(m_Data.data());
    }
    T *data() noexcept
    {
        return reinterpret_cast<T *>(m_Data.data());
    }

  private:
    struct alignas(T) Element
    {
        std::byte m_Data[sizeof(T)];
    };

    static_assert(sizeof(Element) == sizeof(T), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(T), "Element alignment is not equal to T alignment");

    std::array<Element, N> m_Data;
};
} // namespace KIT