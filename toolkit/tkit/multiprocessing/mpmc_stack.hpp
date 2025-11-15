#pragma once

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/non_copyable.hpp"
#include "tkit/utils/debug.hpp"
#include <atomic>
#include <concepts>

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
 * Any thread can allocate nodes fromt the stack, but only one thread can deallocate nodes at a time.
 *
 * @tparam T The type of the elements in the stack.
 */
template <typename T> class MpmcStack
{
    TKIT_NON_COPYABLE(MpmcStack)
  public:
    struct Node
    {
        template <typename... Args>
            requires std::constructible_from<T, Args...>
        constexpr Node(Args &&...p_Args) : Value(std::forward<Args>(p_Args)...)
        {
        }

        T Value;
        Node *Next = nullptr;
    };

    constexpr MpmcStack() = default;
    ~MpmcStack()
    {
        DestroyNodes(m_Head.load(std::memory_order_relaxed));
        DestroyNodes(m_FreeHead.load(std::memory_order_relaxed));
    }

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
    Node *CreateNode(Args &&...p_Args)
    {
        Node *node = t_FreeList.Head;
        if (!node)
            node = m_FreeHead.exchange(nullptr, std::memory_order_acquire);

        if (node)
        {
            t_FreeList.Head = node->Next;
            node->Value = T{std::forward<Args>(p_Args)...};
            return node;
        }
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
    void Push(Args &&...p_Args)
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
    void Push(Node *p_Head, Node *p_Tail)
    {
        TKIT_ASSERT(p_Head && p_Tail, "[TKIT][MULTIPROC] The head and tail must not be null when pushing");
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        do
        {
            p_Tail->Next = oldHead;
        } while (!m_Head.compare_exchange_weak(oldHead, p_Head, std::memory_order_release, std::memory_order_relaxed));
    }

    /**
     * @brief Acquire the whole stack, allowing the consumer to read its contents and flushing the whole stack at the
     * same time.
     *
     * This method may be accessed concurrently by any thread.
     *
     */
    Node *Acquire()
    {
        return m_Head.exchange(nullptr, std::memory_order_acquire);
    }

    /**
     * @brief Reclaim left-over nodes.
     *
     * This method may only be accessed by one thread at a time.
     */
    void Reclaim(Node *p_Head, Node *p_Tail = nullptr)
    {
        TKIT_ASSERT(p_Head, "[TKIT][MULTIPROC] The head must not be null when reclaiming");
        Node *freeList = m_FreeHead.exchange(nullptr, std::memory_order_relaxed);
        if (!p_Tail)
        {
            p_Tail = p_Head;
            while (p_Tail->Next)
                p_Tail = p_Tail->Next;
        }
        TKIT_ASSERT(
            p_Tail && !p_Tail->Next,
            "[TKIT][MULTIPROC] The tail should have resolved to a non null value and its next pointer should be null");

        p_Tail->Next = freeList;
        m_FreeHead.store(p_Head, std::memory_order_release);
    }

    /**
     * @brief Destroy claimed left-over nodes.
     *
     * This method may be accessed concurrently by any thread.
     */
    static void DestroyNodes(const Node *p_Node)
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
    static void DestroyNode(const Node *p_Node)
    {
        delete p_Node;
    }

  private:
    class FreeList
    {
        TKIT_NON_COPYABLE(FreeList)
      public:
        FreeList() = default;
        ~FreeList()
        {
            DestroyNodes(Head);
        }

        Node *Head = nullptr;
    };

    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<Node *> m_Head{nullptr};
    alignas(TKIT_CACHE_LINE_SIZE) std::atomic<Node *> m_FreeHead{nullptr};

    static inline thread_local FreeList t_FreeList{};
};
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
