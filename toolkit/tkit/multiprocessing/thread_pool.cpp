#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"

namespace TKit
{
ThreadPool::ThreadPool(const usize p_ThreadCount) : ITaskManager(p_ThreadCount)
{
    m_Handle = Topology::Initialize();
    Topology::BuildAffinityOrder(m_Handle);
    Topology::PinThread(m_Handle, 0);
    Topology::SetThreadName(0, "tkit-main");

    const auto worker = [this](const usize p_ThreadIndex) {
        Topology::PinThread(m_Handle, p_ThreadIndex);
        Topology::SetThreadName(p_ThreadIndex);

        s_ThreadIndex = p_ThreadIndex;
        const u32 workerIndex = p_ThreadIndex - 1;

        Worker &myself = m_Workers[workerIndex];

        u32 seed = 0x9e3779b9u ^ p_ThreadIndex;
        const u32 nworkers = m_Workers.GetSize();

        const auto rand = [&seed, nworkers]() {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;

            return static_cast<u32>((u64(seed) * u64(nworkers)) >> 32);
        };

        const auto chooseVictim = [workerIndex, &rand]() {
            u32 victim = rand();
            while (victim == workerIndex)
                victim = rand();
            return victim;
        };

        u32 victim = chooseVictim();
        u64 epoch = 0;
        for (;;)
        {
            myself.Epochs.wait(epoch, std::memory_order_acquire);
            epoch = myself.Epochs.load(std::memory_order_relaxed);

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

            if (const auto stolen = m_Workers[victim].Queue.PopFront())
            {
                m_Workers[victim].TaskCount.fetch_sub(1, std::memory_order_relaxed);
                ITask *task = *stolen;
                (*task)();
            }
            else
                victim = chooseVictim();

            if (myself.TerminateSignal.test(std::memory_order_relaxed))
                break;
        }
        myself.TerminateConfirmation.test_and_set(std::memory_order_relaxed);
    };
    for (usize i = 0; i < p_ThreadCount; ++i)
        m_Workers.Append(worker, i + 1);
}

ThreadPool::~ThreadPool() noexcept
{
    bool allFinished = false;
    while (!allFinished)
    {
        allFinished = true;
        for (Worker &worker : m_Workers)
        {
            if (worker.TerminateConfirmation.test(std::memory_order_relaxed))
            {
                if (worker.Thread.joinable())
                    worker.Thread.join();
                continue;
            }
            allFinished = false;
            worker.TerminateSignal.test_and_set(std::memory_order_relaxed);
            worker.Epochs.fetch_add(1, std::memory_order_release);
            worker.Epochs.notify_all();
        }
        if (allFinished)
            break;
    }
    Topology::Terminate(m_Handle);
}

static void assignTask(const u32 p_WorkerIndex, ThreadPool::Worker &p_Worker, ITask *p_Task) noexcept
{
    if (p_WorkerIndex + 1 == ITaskManager::GetThreadIndex())
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

} // namespace TKit
