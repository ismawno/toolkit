#pragma once

#include "tkit/utils/alias.hpp"
#include <algorithm>

namespace TKit::Detail
{
template <typename... T> struct TupleStorage;
template <typename T0, typename... T> struct TupleStorage<T0, T...>
{
    T0 Head;
    TupleStorage<T...> Tail;

    constexpr TupleStorage() = default;
    template <typename U0, typename... U>
        requires(std::convertible_to<U0, T0> && ... && std::convertible_to<U, T>)
    constexpr TupleStorage(U0 &&arg0, U &&...args) : Head(std::forward<U0>(arg0)), Tail(std::forward<U>(args)...)
    {
    }

    template <typename U> constexpr const U &Get() const
    {
        if constexpr (std::is_same_v<T0, U>)
            return Head;
        else
            return Tail.template Get<U>();
    }
    template <typename U> constexpr U &Get()
    {
        if constexpr (std::is_same_v<T0, U>)
            return Head;
        else
            return Tail.template Get<U>();
    }

    template <usize I> constexpr const auto &Get() const
    {
        if constexpr (I == 0)
            return Head;
        else
            return Tail.template Get<I - 1>();
    }
    template <usize I> constexpr auto &Get()
    {
        if constexpr (I == 0)
            return Head;
        else
            return Tail.template Get<I - 1>();
    }
};

template <typename T> struct TupleStorage<T>
{
    T Head;
    constexpr TupleStorage() = default;
    template <std::convertible_to<T> U> constexpr TupleStorage(U &&arg) : Head(std::forward<U>(arg))
    {
    }

    template <typename U> constexpr const U &Get() const
    {
        static_assert(std::is_same_v<T, U>, "[TOOLKIT][TUPLE] Could not find type T in union");
        return Head;
    }
    template <typename U> constexpr U &Get()
    {
        static_assert(std::is_same_v<T, U>, "[TOOLKIT][TUPLE] Could not find type T in union");
        return Head;
    }

    template <usize I> constexpr const T &Get() const
    {
        static_assert(I == 0, "[TOOLKIT][TUPLE] Index I exceeds union size");
        return Head;
    }
    template <usize I> constexpr T &Get()
    {
        static_assert(I == 0, "[TOOLKIT][TUPLE] Index I exceeds union size");
        return Head;
    }
};

template <> struct TupleStorage<>
{
};
} // namespace TKit::Detail

namespace TKit
{
template <typename... T> class Tuple
{
  public:
    constexpr Tuple() = default;
    template <typename... U>
        requires(sizeof...(U) == sizeof...(T) && (std::convertible_to<U, T> && ... && true))
    constexpr Tuple(U &&...args) : m_Data(std::forward<U>(args)...)
    {
    }
    template <typename U> constexpr const U &Get() const
    {
        return m_Data.template Get<U>();
    }
    template <typename U> constexpr U &Get()
    {
        return m_Data.template Get<U>();
    }
    template <usize I> constexpr const auto &Get() const
    {
        return m_Data.template Get<I>();
    }
    template <usize I> constexpr auto &Get()
    {
        return m_Data.template Get<I>();
    }

  private:
    Detail::TupleStorage<T...> m_Data;
};

template <typename... T> constexpr Tuple<std::remove_cvref_t<T>...> CreateTuple(T &&...args)
{
    return Tuple<std::remove_cvref_t<T>...>{std::forward<T>(args)...};
}

namespace Detail
{
template <typename... T0, typename... T1, usize... I0, usize... I1>
constexpr Tuple<T0..., T1...> ConcatenateTwo(const Tuple<T0...> &t0, const Tuple<T1...> &t1,
                                             const std::integer_sequence<usize, I0...>,
                                             const std::integer_sequence<usize, I1...>)
{
    return Tuple<T0..., T1...>{t0.template Get<I0>()..., t1.template Get<I1>()...};
}
template <typename F, typename Tup, usize... Is>
constexpr auto ApplyImpl(F &&f, Tup &&t, const std::integer_sequence<usize, Is...>)
{
    return std::forward<F>(f)(Get<Is>(std::forward<Tup>(t))...);
}
} // namespace Detail

template <typename F, typename... T> constexpr auto Apply(F &&f, Tuple<T...> &t)
{
    return Detail::ApplyImpl(std::forward<F>(f), t, std::make_integer_sequence<usize, sizeof...(T)>{});
}

template <typename F, typename... T> constexpr auto Apply(F &&f, const Tuple<T...> &t)
{
    return Detail::ApplyImpl(std::forward<F>(f), t, std::make_integer_sequence<usize, sizeof...(T)>{});
}

template <typename F, typename... T> constexpr auto Apply(F &&f, Tuple<T...> &&t)
{
    return Detail::ApplyImpl(std::forward<F>(f), std::move(t), std::make_integer_sequence<usize, sizeof...(T)>{});
}

template <typename... T0, typename... T1>
constexpr auto ConcatenateTuples(const Tuple<T0...> &t0, const Tuple<T1...> &t1)
{
    return Detail::ConcatenateTwo(t0, t1, std::make_integer_sequence<usize, sizeof...(T0)>{},
                                  std::make_integer_sequence<usize, sizeof...(T1)>{});
}

template <typename T0, typename T1, typename... T>
constexpr auto ConcatenateTuples(T0 &&tuple0, T1 &&tuple1, T &&...rest)
{
    return ConcatenateTuples(ConcatenateTuples(std::forward<T0>(tuple0), std::forward<T1>(tuple1)),
                             std::forward<T>(rest)...);
}

template <usize I, typename... T> auto &Get(Tuple<T...> &t)
{
    return t.template Get<I>();
}

template <usize I, typename... T> const auto &Get(const Tuple<T...> &t)
{
    return t.template Get<I>();
}

template <usize I, typename... T> auto &&Get(Tuple<T...> &&t)
{
    return std::move(t).template Get<I>();
}

} // namespace TKit

template <typename... T> struct std::tuple_size<TKit::Tuple<T...>> : std::integral_constant<TKit::usize, sizeof...(T)>
{
};

template <TKit::usize I, typename T0, typename... T> struct std::tuple_element<I, TKit::Tuple<T0, T...>>
{
    using type = typename std::tuple_element<I - 1, TKit::Tuple<T...>>::type;
};

template <typename T0, typename... T> struct std::tuple_element<0, TKit::Tuple<T0, T...>>
{
    using type = T0;
};
