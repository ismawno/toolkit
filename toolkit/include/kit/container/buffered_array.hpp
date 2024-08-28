#pragma once

#include "kit/container/array.hpp"

namespace KIT
{

// An STL-like buffered array with a fixed size. It manages a buffer of data without owning/handling the data itself,
// meaning it is not responsible for its allocation/deallocation, but provides a nice API to work with it
template <typename T> class BufferedArray : public IArray<T, BufferedArray<T>>
{
  public:
    using BaseArray = IArray<T, BufferedArray<T>>;

    BufferedArray() noexcept = default;

    BufferedArray(T *p_Data, const usize p_Capacity) noexcept : m_Data(p_Data), m_Capacity(p_Capacity)
    {
    }

    template <typename... Args>
    BufferedArray(T *p_Data, const usize p_Capacity, const usize p_Size, Args &&...p_Args) noexcept
        : BaseArray(p_Size), m_Data(p_Data), m_Capacity(p_Capacity)
    {
        this->Constructor(std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It>
    BufferedArray(T *p_Data, const usize p_Capacity, It p_Begin, It p_End) noexcept
        : m_Data(p_Data), m_Capacity(p_Capacity)
    {
        this->Constructor(p_Begin, p_End);
    }

    BufferedArray(T *p_Data, const BufferedArray &p_Other) noexcept
        : BaseArray(p_Other.size()), m_Data(p_Data), m_Capacity(p_Other.m_Capacity)
    {
        this->Constructor(p_Other);
    }

    BufferedArray(BufferedArray &&p_Other) noexcept
        : BaseArray(p_Other.size()), m_Data(p_Other.m_Data), m_Capacity(p_Other.m_Capacity)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Capacity = 0;
    }

    explicit(false) BufferedArray(T *p_Data, const usize p_Capacity, std::initializer_list<T> p_List) noexcept
        : BaseArray(p_List.size()), m_Data(p_Data), m_Capacity(p_Capacity)
    {
        this->Constructor(p_List);
    }

    ~BufferedArray() noexcept
    {
        this->clear();
    }

    BufferedArray &operator=(const BufferedArray &p_Other) noexcept
    {
        KIT_ASSERT(m_Data, "BufferedArray has not been provided with a buffer");
        this->CopyAssignment(p_Other);
        return *this;
    }
    BufferedArray &operator=(BufferedArray &&p_Other) noexcept
    {
        // This can cause a leak if m_Data is set and is heap allocated
        m_Data = p_Other.m_Data;
        m_Capacity = p_Other.m_Capacity;
        p_Other.m_Data = nullptr;
        p_Other.m_Capacity = 0;
        return *this;
    }

    BufferedArray &operator=(std::initializer_list<T> p_List) noexcept
    {
        KIT_ASSERT(m_Data, "BufferedArray has not been provided with a buffer");
        this->CopyAssignment(p_List);
        return *this;
    }

    usize capacity() const noexcept
    {
        return m_Capacity;
    }

    const T *data() const noexcept
    {
        return m_Data;
    }
    T *data() noexcept
    {
        return m_Data;
    }

  private:
    T *m_Data = nullptr;
    usize m_Capacity = 0;
};
} // namespace KIT