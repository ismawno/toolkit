#pragma once

#include "tkit/container/container.hpp"
#include <atomic>

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)

namespace TKit
{
/**
 * @brief A multiple-producer multiple-consumer stack.
 *
 * Any producer thread may push to the head of the stack. The consumer threads may acquire the whole stack in a single
 * atomic operation, flushing the whole stack at once.
 *
 * @tparam T The type of the elements in the stack.
 */
template <typename T> class MpmcStack
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

    MpmcStack() noexcept = default;

    /**
     * @brief Create a stack node by emplacing an element of type T on creation.
     *
     * The element is constructed in place using the provided arguments.
     * This method may be accessed consurrently by any thread.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static Node *CreateNode(Args &&...p_Args) noexcept
    {
        return new Node{std::forward<Args>(p_Args)...};
    }

    /**
     * @brief Push a new element into the stack.
     *
     * The element is constructed in place using the provided arguments.
     * This method may be accessed concurrently by any thread.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     */
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

    /**
     * @brief Push a range of elements into the stack.
     *
     * The element is constructed in place using the provided arguments.
     * This method may be accessed concurrently by any thread.
     *
     * @param p_Head The head of the range.
     * @param p_Tail The tail of the range.
     */
    void Push(Node *p_Head, Node *p_Tail) noexcept
    {
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        do
        {
            p_Tail->Next = oldHead;
        } while (!m_Head.compare_exchange_weak(oldHead, p_Head, std::memory_order_release, std::memory_order_relaxed));
    }

    /**
     * @brief Claim the whole stack, allowing the consumer to read its contents and flushing the whole stack at the same
     * time.
     *
     * This method may be accessed concurrently by any thread.
     *
     */
    Node *Claim() noexcept
    {
        return m_Head.exchange(nullptr, std::memory_order_acquire);
    }

    /**
     * @brief Destroy claimed left-over nodes.
     *
     * This method may be accessed concurrently by any thread.
     */
    static void DestroyNodes(const Node *p_Node) noexcept
    {
        while (p_Node)
        {
            const Node *next = p_Node->Next;
            DestroyNode(p_Node);
            p_Node = next;
        }
    }

    /**
     * @brief Destroy a claimed left-over node.
     *
     * This method may be accessed concurrently by any thread.
     */
    static void DestroyNode(const Node *p_Node) noexcept
    {
        delete p_Node;
    }

  private:
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<Node *> m_Head{nullptr};
};
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
