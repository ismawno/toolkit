#pragma once

#include "kit/container/iarray.hpp"

namespace KIT
{
/**
 * @brief An STL-like array view with a fixed size. It manages a buffer of data without owning/handling the data itself,
 * meaning it is not responsible for its allocation/deallocation, but provides a nice API to work with it.
 *
 * @tparam T The type of the elements in the array.
 */
template <typename T> class ArrayView : public IArray<T, ArrayView<T>>
{
  public:
    using BaseArray = IArray<T, ArrayView<T>>;

    ArrayView() noexcept = default;

    ArrayView(T *p_Data, const usize p_Capacity) noexcept : m_Data(p_Data), m_Capacity(p_Capacity)
    {
    }

    template <typename... Args>
    ArrayView(T *p_Data, const usize p_Capacity, const usize p_Size, Args &&...p_Args) noexcept
        : BaseArray(p_Size), m_Data(p_Data), m_Capacity(p_Capacity)
    {
        this->Constructor(std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It>
    ArrayView(T *p_Data, const usize p_Capacity, It p_Begin, It p_End) noexcept : m_Data(p_Data), m_Capacity(p_Capacity)
    {
        this->Constructor(p_Begin, p_End);
    }

    ArrayView(T *p_Data, const ArrayView &p_Other) noexcept
        : BaseArray(p_Other.size()), m_Data(p_Data), m_Capacity(p_Other.m_Capacity)
    {
        this->Constructor(p_Other);
    }

    ArrayView(ArrayView &&p_Other) noexcept
        : BaseArray(p_Other.size()), m_Data(p_Other.m_Data), m_Capacity(p_Other.m_Capacity)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Capacity = 0;
    }

    explicit(false) ArrayView(T *p_Data, const usize p_Capacity, std::initializer_list<T> p_List) noexcept
        : BaseArray(p_List.size()), m_Data(p_Data), m_Capacity(p_Capacity)
    {
        this->Constructor(p_List);
    }

    ~ArrayView() noexcept
    {
        this->clear();
    }

    ArrayView &operator=(const ArrayView &p_Other) noexcept
    {
        KIT_ASSERT(m_Data, "ArrayView has not been provided with a buffer");
        this->CopyAssignment(p_Other);
        return *this;
    }
    ArrayView &operator=(ArrayView &&p_Other) noexcept
    {
        // This can cause a leak if m_Data is set and is heap allocated
        m_Data = p_Other.m_Data;
        m_Capacity = p_Other.m_Capacity;
        p_Other.m_Data = nullptr;
        p_Other.m_Capacity = 0;
        return *this;
    }

    ArrayView &operator=(std::initializer_list<T> p_List) noexcept
    {
        KIT_ASSERT(m_Data, "ArrayView has not been provided with a buffer");
        this->CopyAssignment(p_List);
        return *this;
    }

    /**
     * @brief Get the capacity of the underlying buffer.
     *
     */
    usize capacity() const noexcept
    {
        return m_Capacity;
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    const T *data() const noexcept
    {
        return m_Data;
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    T *data() noexcept
    {
        return m_Data;
    }

  private:
    T *m_Data = nullptr;
    usize m_Capacity = 0;
};
} // namespace KIT