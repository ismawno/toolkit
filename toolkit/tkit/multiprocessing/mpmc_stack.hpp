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
        constexpr Node(Args &&...args) : Value(std::forward<Args>(args)...)
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
     * @param args The arguments to pass to the constructor of `T`.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    Node *CreateNode(Args &&...args)
    {
        Node *node = t_FreeList.Head;
        if (!node)
            node = m_FreeHead.exchange(nullptr, std::memory_order_acquire);

        if (node)
        {
            t_FreeList.Head = node->Next;
            node->Value = T{std::forward<Args>(args)...};
            return node;
        }
        return new Node{std::forward<Args>(args)...};
    }

    /**
     * @brief Push a new element into the stack.
     *
     * The element is constructed in place using the provided arguments.
     * This method may be accessed concurrently by any thread.
     *
     * @param args The arguments to pass to the constructor of `T`.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void Push(Args &&...args)
    {
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        Node *head = CreateNode(std::forward<Args>(args)...);
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
     * @param head The head of the range.
     * @param tail The tail of the range.
     */
    void Push(Node *head, Node *tail)
    {
        TKIT_ASSERT(head && tail, "[TKIT][MULTIPROC] The head and tail must not be null when pushing");
        Node *oldHead = m_Head.load(std::memory_order_relaxed);
        do
        {
            tail->Next = oldHead;
        } while (!m_Head.compare_exchange_weak(oldHead, head, std::memory_order_release, std::memory_order_relaxed));
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
    void Reclaim(Node *head, Node *tail = nullptr)
    {
        TKIT_ASSERT(head, "[TKIT][MULTIPROC] The head must not be null when reclaiming");
        Node *freeList = m_FreeHead.exchange(nullptr, std::memory_order_relaxed);
        if (!tail)
        {
            tail = head;
            while (tail->Next)
                tail = tail->Next;
        }
        TKIT_ASSERT(
            tail && !tail->Next,
            "[TKIT][MULTIPROC] The tail should have resolved to a non null value and its next pointer should be null");

        tail->Next = freeList;
        m_FreeHead.store(head, std::memory_order_release);
    }

    /**
     * @brief Destroy claimed left-over nodes.
     *
     * This method may be accessed concurrently by any thread.
     */
    static void DestroyNodes(const Node *node)
    {
        while (node)
        {
            const Node *next = node->Next;
            DestroyNode(node);
            node = next;
        }
    }

    /**
     * @brief Destroy a claimed left-over node.
     *
     * This method may be accessed concurrently by any thread.
     */
    static void DestroyNode(const Node *node)
    {
        delete node;
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
