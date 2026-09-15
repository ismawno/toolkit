#pragma once

#include "tkit/utils/storage.hpp"

namespace TKit
{
template <typename Storage, typename Signature> class Function;

template <typename Storage, typename Ret, typename... Args> class Function<Storage, Ret(Args...)>
{
    static constexpr StorageType Type = Storage::Type;

  public:
    constexpr Function() = default;
    constexpr Function(const std::nullptr_t)
    {
    }

    template <typename F>
        requires(!std::is_same_v<std::remove_cvref_t<F>, Function>)
    constexpr Function(F &&func)
    {
        if constexpr (Type == Storage_Dynamic)
            m_Storage = Storage{sizeof(F), alignof(F)};
        else if constexpr (Type != Storage_Static)
            m_Storage = Storage{sizeof(F)};

        m_Storage.template Construct<F>(std::forward<F>(func));
        m_Invoke = [](Storage &st, Args... args) { return st.template Get<F>()(args...); };
        m_Destroy = [](const Storage &st) { st.template Destruct<F>(); };
    }

    constexpr Function(Function &&other)
        requires(Type != Storage_Static)
        : m_Storage(std::move(other.m_Storage)), m_Invoke{other.m_Invoke}, m_Destroy{other.m_Destroy}
    {
        other.m_Invoke = nullptr;
        other.m_Destroy = nullptr;
    }

    constexpr ~Function()
    {
        if (m_Destroy)
            m_Destroy(m_Storage);
    }

    constexpr Function &operator=(Function &&other)
        requires(Type != Storage_Static)
    {
        if (&other == this)
            return *this;

        if (m_Destroy)
            m_Destroy(m_Storage);

        m_Storage = std::move(other.m_Storage);
        m_Invoke = other.m_Invoke;
        m_Destroy = other.m_Destroy;
        other.m_Invoke = nullptr;
        other.m_Destroy = nullptr;
        return *this;
    }

    template <typename F> constexpr Function &operator=(F &&func)
    {
        if (m_Destroy)
            m_Destroy(m_Storage);
        if constexpr (Type == Storage_Dynamic)
            m_Storage = Storage{sizeof(F), alignof(F)};
        else if constexpr (Type != Storage_Static)
            m_Storage = Storage{sizeof(F)};

        m_Storage.template Construct<F>(std::forward<F>(func));
        m_Invoke = [](Storage &st, Args... args) { return st.template Get<F>()(args...); };
        m_Destroy = [](const Storage &st) { st.template Destruct<F>(); };
        return *this;
    }

    constexpr Function &operator=(const std::nullptr_t)
    {
        m_Invoke = nullptr;
        m_Destroy = nullptr;
        return *this;
    }

    constexpr Ret operator()(Args... args)
    {
        return m_Invoke(m_Storage, args...);
    }

    constexpr operator bool() const
    {
        return m_Invoke;
    }

  private:
    Storage m_Storage;
    Ret (*m_Invoke)(Storage &, Args...) = nullptr;
    void (*m_Destroy)(const Storage &st) = nullptr;
};

template <typename Signature> using ArenaFunction = Function<ArenaStorage, Signature>;
template <typename Signature> using StackFunction = Function<StackStorage, Signature>;
template <typename Signature> using TierFunction = Function<TierStorage, Signature>;
template <typename Signature> using DynamicFunction = Function<DynamicStorage, Signature>;

template <usize Size, typename Signature> using StaticFunction = Function<StaticStorage<Size>, Signature>;

template <typename Signature> using StaticFunction4 = StaticFunction<4, Signature>;
template <typename Signature> using StaticFunction8 = StaticFunction<8, Signature>;
template <typename Signature> using StaticFunction16 = StaticFunction<16, Signature>;
template <typename Signature> using StaticFunction32 = StaticFunction<32, Signature>;
template <typename Signature> using StaticFunction64 = StaticFunction<64, Signature>;
template <typename Signature> using StaticFunction128 = StaticFunction<128, Signature>;
template <typename Signature> using StaticFunction196 = StaticFunction<196, Signature>;
template <typename Signature> using StaticFunction256 = StaticFunction<256, Signature>;
template <typename Signature> using StaticFunction384 = StaticFunction<384, Signature>;
template <typename Signature> using StaticFunction512 = StaticFunction<512, Signature>;
template <typename Signature> using StaticFunction768 = StaticFunction<768, Signature>;
template <typename Signature> using StaticFunction1024 = StaticFunction<1024, Signature>;
} // namespace TKit
