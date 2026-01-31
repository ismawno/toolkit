#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"

namespace TKit
{
thread_local usize t_Victim;
static usize cheapRand(const usize workers)
{
    thread_local usize seed = 0x9e3779b9u ^ Topology::GetThreadIndex();
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;

    return static_cast<usize>((u64(seed) * u64(workers)) >> 32);
}
static void shuffleVictim(const usize workerIndex, const usize workers)
{
    usize victim = cheapRand(workers);
    while (victim == workerIndex)
        victim = cheapRand(workers);
    t_Victim = victim;
}

void ThreadPool::drainTasks(const usize workerIndex, const usize workers)
{
    Worker &myself = m_Workers[workerIndex];

    using Node = MpmcStack<ITask *>::Node;
    Node *taskTail = myself.Inbox.Acquire();

    if (taskTail)
    {
        Node *taskHead = taskTail;
        while (taskTail->Next)
        {
            ITask *task = taskTail->Value;
            myself.Queue.PushBack(task);
            taskTail = taskTail->Next;
        }
        ITask *task = taskTail->Value;
        (*task)();

        myself.Inbox.Reclaim(taskHead, taskTail);
        myself.TaskCount.fetch_sub(1, std::memory_order_relaxed);
    }

    while (const auto t = myself.Queue.PopBack())
    {
        ITask *task = *t;
        (*task)();
        myself.TaskCount.fetch_sub(1, std::memory_order_relaxed);
    }
    if (!trySteal(t_Victim))
        shuffleVictim(workerIndex, workers);
}

bool ThreadPool::trySteal(const usize victim)
{
    Worker &wvictim = m_Workers[victim];
    if (const auto stolen = wvictim.Queue.PopFront())
    {
        wvictim.TaskCount.fetch_sub(1, std::memory_order_relaxed);
        ITask *task = *stolen;
        (*task)();
        return true;
    }
    return false;
}

ThreadPool::ThreadPool(const usize workerCount, const usize maxTasksPerQueue)
    : ThreadPool(TKit::GetArena(), workerCount, maxTasksPerQueue)
{
}

ThreadPool::ThreadPool(ArenaAllocator *allocator, const usize workerCount, const usize maxTasksPerQueue)
    : ITaskManager(workerCount), m_Workers{allocator, workerCount}
{
    TKIT_ASSERT(allocator, "[TOOLKIT][MULTIPROC] An arena allocator must be provided, but passed value was null");
    TKIT_ASSERT(workerCount > 1, "[TOOLKIT][MULTIPROC] At least 2 workers are required to create a thread pool");
    m_Handle = Topology::Initialize();
    Topology::SetThreadIndex(0);
    Topology::BuildAffinityOrder(m_Handle);
    Topology::PinThread(m_Handle, 0);
    Topology::SetThreadName(0, "tkit-main");

    const auto worker = [this](const usize threadIndex) {
        Topology::SetThreadIndex(threadIndex);
        Topology::PinThread(m_Handle, threadIndex);
        Topology::SetThreadName(threadIndex);

        const usize workerIndex = threadIndex - 1;

        m_ReadySignal.wait(false, std::memory_order_acquire);

        Worker &myself = m_Workers[workerIndex];
        const usize nworkers = m_Workers.GetSize();

        shuffleVictim(workerIndex, nworkers);
        u32 epoch = 0;
        for (;;)
        {
            myself.Epochs.wait(epoch, std::memory_order_acquire);
            epoch = myself.Epochs.load(std::memory_order_relaxed);

            drainTasks(workerIndex, nworkers);

            if (myself.TerminateSignal.test(std::memory_order_relaxed))
                break;
        }
    };
    for (usize i = 0; i < workerCount; ++i)
        m_Workers.Append(allocator, maxTasksPerQueue, worker, i + 1);

    m_ReadySignal.test_and_set(std::memory_order_release);
    m_ReadySignal.notify_all();
}

ThreadPool::~ThreadPool()
{
    m_ReadySignal.notify_all();
    for (Worker &worker : m_Workers)
    {
        worker.TerminateSignal.test_and_set(std::memory_order_relaxed);
        worker.Epochs.fetch_add(1, std::memory_order_release);
        worker.Epochs.notify_all();
        worker.Thread.join();
    }
    Topology::Terminate(m_Handle);
}

static void assignTask(const usize workerIndex, ThreadPool::Worker &worker, ITask *task)
{
    if (workerIndex == ThreadPool::GetWorkerIndex())
        worker.Queue.PushBack(task);
    else
        worker.Inbox.Push(task);

    worker.TaskCount.fetch_add(1, std::memory_order_relaxed);
    worker.Epochs.fetch_add(1, std::memory_order_release);
    worker.Epochs.notify_one();
}

usize ThreadPool::SubmitTask(ITask *task, usize submissionIndex)
{
    const usize wcount = m_Workers.GetSize();
    u32 maxCount = 0;
    for (;;)
    {
        for (usize i = submissionIndex; i < wcount; ++i)
        {
            const u32 count = m_Workers[i].TaskCount.load(std::memory_order_relaxed);
            if (count <= maxCount)
            {
                assignTask(i, m_Workers[i], task);
                return (i + 1) % wcount;
            }
        }
        submissionIndex = 0;
        ++maxCount;
    }
}

void ThreadPool::WaitUntilFinished(const ITask &task)
{
    const usize nworkers = m_Workers.GetSize();
    if (Topology::GetThreadIndex() == 0)
    {
        usize index = cheapRand(nworkers);
        while (!task.IsFinished(std::memory_order_acquire))
        {
            trySteal(index);
            index = (index + 1) % nworkers;
            std::this_thread::yield();
        }
    }
    else
    {
        const usize workerIndex = GetWorkerIndex();
        while (!task.IsFinished(std::memory_order_acquire))
        {
            drainTasks(workerIndex, nworkers);
            std::this_thread::yield();
        }
    }
}

} // namespace TKit
