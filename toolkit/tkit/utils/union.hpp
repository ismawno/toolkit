#include "tkit/utils/storage.hpp"

namespace TKit::Detail
{
template <typename... T> struct UnionStorage;
template <typename T0, typename... T> struct UnionStorage<T0, T...>
{
    union {
        Storage<T0> Head;
        UnionStorage<T...> Tail;
    };

    template <typename U> constexpr const Storage<U> &GetStorage() const
    {
        if constexpr (std::is_same_v<T0, U>)
            return Head;
        else
            return Tail.template GetStorage<U>();
    }
    template <typename U> constexpr Storage<U> &GetStorage()
    {
        if constexpr (std::is_same_v<T0, U>)
            return Head;
        else
            return Tail.template GetStorage<U>();
    }

    template <usize I> constexpr const auto &GetStorage() const
    {
        if constexpr (I == 0)
            return Head;
        else
            return Tail.template GetStorage<I - 1>();
    }
    template <usize I> constexpr auto &GetStorage()
    {
        if constexpr (I == 0)
            return Head;
        else
            return Tail.template GetStorage<I - 1>();
    }
};

template <typename T> struct UnionStorage<T>
{
    Storage<T> Head;
    template <typename U> constexpr const Storage<U> &GetStorage() const
    {
        static_assert(std::is_same_v<T, U>, "[TOOLKIT][UNION] Could not find type T in union");
        return Head;
    }
    template <typename U> constexpr Storage<U> &GetStorage()
    {
        static_assert(std::is_same_v<T, U>, "[TOOLKIT][UNION] Could not find type T in union");
        return Head;
    }

    template <usize I> constexpr const Storage<T> &GetStorage() const
    {
        static_assert(I == 0, "[TOOLKIT][UNION] Index I exceeds union size");
        return Head;
    }
    template <usize I> constexpr Storage<T> &GetStorage()
    {
        static_assert(I == 0, "[TOOLKIT][UNION] Index I exceeds union size");
        return Head;
    }
};

template <> struct UnionStorage<>
{
};

} // namespace TKit::Detail

namespace TKit
{
template <typename... T> class Union
{
  public:
    template <typename U, typename... Args> constexpr U &Construct(Args &&...args)
    {
        return m_Data.template GetStorage<U>().Construct(std::forward<Args>(args)...);
    }
    template <typename U> constexpr U &Destruct()
    {
        if constexpr (!std::is_trivially_destructible_v<U>)
            return m_Data.template GetStorage<U>().Destruct();
    }

    template <typename U> constexpr const U &Get() const
    {
        return m_Data.template GetStorage<U>().Get();
    }
    template <typename U> constexpr U &Get()
    {
        return m_Data.template GetStorage<U>().Get();
    }
    template <usize I> constexpr const auto &Get() const
    {
        return m_Data.template GetStorage<I>().Get();
    }
    template <usize I> constexpr auto &Get()
    {
        return m_Data.template GetStorage<I>().Get();
    }

  private:
    Detail::UnionStorage<T...> m_Data;
};
} // namespace TKit
