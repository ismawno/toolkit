#pragma once

#include "tkit/container/container.hpp"

namespace TKit
{
template <typename T> class MpscStack
{
  public:
    struct Node
    {
        template <typename... Args>
            requires std::constructible_from<T, Args...>
        Node(Args &&...p_Args) : Value(std::forward<Args>(p_Args)...), Next(nullptr)
        {
        }

        T Value;
        Node *Next;
    };

    MpscStack() noexcept = default;

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    Node *CreateNode(Args &&...p_Args) noexcept
    {
        return new Node{std::forward<Args>(p_Args)...};
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void Push(Args &&...p_Args) noexcept
    {
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        Node *head = CreateNode(std::forward<Args>(p_Args)...);
        do
        {
            head->Next = oldHead;
        } while (!m_Head.compare_exchange_weak(oldHead, head, std::memory_order_release, std::memory_order_relaxed));
    }

    void Push(Node *p_Head, Node *p_Tail) noexcept
    {
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        do
        {
            p_Tail->Next = oldHead;
        } while (!m_Head.compare_exchange_weak(oldHead, p_Head, std::memory_order_release, std::memory_order_relaxed));
    }

    Node *Claim() noexcept
    {
        return m_Head.exchange(nullptr, std::memory_order_acquire);
    }

    void Recycle(const Node *p_Node) const noexcept
    {
        while (p_Node)
        {
            const Node *next = p_Node->Next;
            delete p_Node;
            p_Node = next;
        }
    }

  private:
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<Node *> m_Head{nullptr};
};
} // namespace TKit
