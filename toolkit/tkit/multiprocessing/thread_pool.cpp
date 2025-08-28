#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"

namespace TKit
{
thread_local u32 s_Victim;
static u32 cheapRand(const u32 p_Workers) noexcept
{
    thread_local u32 seed = 0x9e3779b9u ^ ITaskManager::GetThreadIndex();
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;

    return static_cast<u32>((u64(seed) * u64(p_Workers)) >> 32);
}
static void shuffleVictim(const u32 p_WorkerIndex, const u32 p_Workers) noexcept
{
    u32 victim = cheapRand(p_Workers);
    while (victim == p_WorkerIndex)
        victim = cheapRand(p_Workers);
    s_Victim = victim;
}

void ThreadPool::drainTasks(const u32 p_WorkerIndex, const u32 p_Workers) noexcept
{
    Worker &myself = m_Workers[p_WorkerIndex];

    using Node = MpmcStack<ITask *>::Node;
    const Node *freshTasks = myself.Inbox.Claim();

    if (freshTasks)
    {
        while (freshTasks->Next)
        {
            ITask *task = freshTasks->Value;
            myself.Queue.PushBack(task);

            const Node *next = freshTasks->Next;
            myself.Inbox.DestroyNode(freshTasks);
            freshTasks = next;
        }
        ITask *task = freshTasks->Value;
        (*task)();
        myself.Inbox.DestroyNode(freshTasks);
        myself.TaskCount.fetch_sub(1, std::memory_order_relaxed);
    }

    while (const auto t = myself.Queue.PopBack())
    {
        ITask *task = *t;
        (*task)();
        myself.TaskCount.fetch_sub(1, std::memory_order_relaxed);
    }

    const u32 victim = s_Victim;
    if (const auto stolen = m_Workers[victim].Queue.PopFront())
    {
        m_Workers[victim].TaskCount.fetch_sub(1, std::memory_order_relaxed);
        ITask *task = *stolen;
        (*task)();
    }
    else
        shuffleVictim(p_WorkerIndex, p_Workers);
}

ThreadPool::ThreadPool(const usize p_WorkerCount) : ITaskManager(p_WorkerCount)
{
    TKIT_ASSERT(p_WorkerCount > 1, "[TOOLKIT][MULTIPROC] At least 2 workers are required to create a thread pool");
    m_Handle = Topology::Initialize();
    Topology::BuildAffinityOrder(m_Handle);
    Topology::PinThread(m_Handle, 0);
    Topology::SetThreadName(0, "tkit-main");

    const auto worker = [this](const usize p_ThreadIndex) {
        Topology::PinThread(m_Handle, p_ThreadIndex);
        Topology::SetThreadName(p_ThreadIndex);

        s_ThreadIndex = p_ThreadIndex;
        const u32 workerIndex = p_ThreadIndex - 1;

        m_ReadySignal.wait(false, std::memory_order_acquire);

        Worker &myself = m_Workers[workerIndex];
        const u32 nworkers = m_Workers.GetSize();

        shuffleVictim(workerIndex, nworkers);
        u64 epoch = 0;
        for (;;)
        {
            myself.Epochs.wait(epoch, std::memory_order_acquire);
            epoch = myself.Epochs.load(std::memory_order_relaxed);

            drainTasks(workerIndex, nworkers);

            if (myself.TerminateSignal.test(std::memory_order_relaxed))
                break;
        }
    };
    for (usize i = 0; i < p_WorkerCount; ++i)
        m_Workers.Append(worker, i + 1);

    m_ReadySignal.test_and_set(std::memory_order_release);
    m_ReadySignal.notify_all();
}

ThreadPool::~ThreadPool() noexcept
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

static void assignTask(const u32 p_WorkerIndex, ThreadPool::Worker &p_Worker, ITask *p_Task) noexcept
{
    if (p_WorkerIndex == ThreadPool::GetWorkerIndex())
        p_Worker.Queue.PushBack(p_Task);
    else
        p_Worker.Inbox.Push(p_Task);

    p_Worker.TaskCount.fetch_add(1, std::memory_order_relaxed);
    p_Worker.Epochs.fetch_add(1, std::memory_order_release);
    p_Worker.Epochs.notify_one();
}

void ThreadPool::SubmitTask(ITask *p_Task) noexcept
{
    u32 minCount = Limits<u32>::max();
    u32 index = 0;
    const u32 size = m_Workers.GetSize();
    for (u32 i = 0; i < size; ++i)
    {
        Worker &worker = m_Workers[i];
        const u32 count = worker.TaskCount.load(std::memory_order_relaxed);
        if (count == 0)
        {
            assignTask(i, worker, p_Task);
            return;
        }
        if (minCount > count)
        {
            minCount = count;
            index = i;
        }
    }
    assignTask(index, m_Workers[index], p_Task);
}
void ThreadPool::SubmitTasks(const Span<ITask *const> p_Tasks) noexcept
{
    u32 stride = 0;
    const u32 size = p_Tasks.GetSize();

    const u32 wcount = m_Workers.GetSize();
    constexpr u32 workers = TKIT_THREAD_POOL_MAX_WORKERS;

    Array<u32, workers> counts{};
    for (u32 i = 0; i < wcount; ++i)
        counts[i] = m_Workers[i].TaskCount.load(std::memory_order_relaxed);

    ITask *task = p_Tasks[stride];
    u32 minCount = Limits<u32>::max();

    for (u32 i = 0; i < wcount; ++i)
    {
        Worker &worker = m_Workers[i];
        u32 &count = counts[i];
        if (count == 0)
        {
            assignTask(i, worker, task);
            ++count;
            if (++stride == size)
                return;
            task = p_Tasks[stride];
        }
        if (minCount > count)
            minCount = count;
    }

    for (;;)
    {
        for (u32 i = 0; i < wcount; ++i)
        {
            Worker &worker = m_Workers[i];
            u32 &count = counts[i];
            if (count <= minCount)
            {
                assignTask(i, worker, task);
                ++count;
                if (++stride == size)
                    return;
                task = p_Tasks[stride];
            }
        }
        ++minCount;
    }
}

void ThreadPool::WaitUntilFinished(ITask *p_Task) noexcept
{
    if (s_ThreadIndex == 0)
    {
        p_Task->WaitUntilFinished();
        return;
    }

    const u32 workerIndex = GetWorkerIndex();
    const u32 nworkers = m_Workers.GetSize();

    while (!p_Task->IsFinished(std::memory_order_acquire))
        drainTasks(workerIndex, nworkers);
}

usize ThreadPool::GetWorkerIndex() noexcept
{
    return s_ThreadIndex - 1;
}

} // namespace TKit
