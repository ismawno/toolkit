#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"

namespace TKit
{
thread_local usize t_Victim;
static usize cheapRand(const usize p_Workers)
{
    thread_local usize seed = 0x9e3779b9u ^ ITaskManager::GetThreadIndex();
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;

    return static_cast<usize>((u64(seed) * u64(p_Workers)) >> 32);
}
static void shuffleVictim(const usize p_WorkerIndex, const usize p_Workers)
{
    usize victim = cheapRand(p_Workers);
    while (victim == p_WorkerIndex)
        victim = cheapRand(p_Workers);
    t_Victim = victim;
}

void ThreadPool::drainTasks(const usize p_WorkerIndex, const usize p_Workers)
{
    Worker &myself = m_Workers[p_WorkerIndex];

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
        shuffleVictim(p_WorkerIndex, p_Workers);
}

bool ThreadPool::trySteal(const usize p_Victim)
{
    Worker &victim = m_Workers[p_Victim];
    if (const auto stolen = victim.Queue.PopFront())
    {
        victim.TaskCount.fetch_sub(1, std::memory_order_relaxed);
        ITask *task = *stolen;
        (*task)();
        return true;
    }
    return false;
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

        t_ThreadIndex = p_ThreadIndex;
        const usize workerIndex = p_ThreadIndex - 1;

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
    for (usize i = 0; i < p_WorkerCount; ++i)
        m_Workers.Append(worker, i + 1);

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

static void assignTask(const usize p_WorkerIndex, ThreadPool::Worker &p_Worker, ITask *p_Task)
{
    if (p_WorkerIndex == ThreadPool::GetWorkerIndex())
        p_Worker.Queue.PushBack(p_Task);
    else
        p_Worker.Inbox.Push(p_Task);

    p_Worker.TaskCount.fetch_add(1, std::memory_order_relaxed);
    p_Worker.Epochs.fetch_add(1, std::memory_order_release);
    p_Worker.Epochs.notify_one();
}

usize ThreadPool::SubmitTask(ITask *p_Task, usize p_SubmissionIndex)
{
    const usize wcount = m_Workers.GetSize();
    u32 maxCount = 0;
    for (;;)
    {
        for (usize i = p_SubmissionIndex; i < wcount; ++i)
        {
            const u32 count = m_Workers[i].TaskCount.load(std::memory_order_relaxed);
            if (count <= maxCount)
            {
                assignTask(i, m_Workers[i], p_Task);
                return (i + 1) % wcount;
            }
        }
        p_SubmissionIndex = 0;
        ++maxCount;
    }
}

void ThreadPool::WaitUntilFinished(const ITask &p_Task)
{
    const usize nworkers = m_Workers.GetSize();
    if (t_ThreadIndex == 0)
    {
        usize index = 0;
        while (!p_Task.IsFinished(std::memory_order_acquire))
        {
            trySteal(index);
            index = (index + 1) % nworkers;
            std::this_thread::yield();
        }
    }
    else
    {
        const usize workerIndex = GetWorkerIndex();
        while (!p_Task.IsFinished(std::memory_order_acquire))
        {
            drainTasks(workerIndex, nworkers);
            std::this_thread::yield();
        }
    }
}

} // namespace TKit
