#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/topology.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/utils/limits.hpp"

#ifdef TKIT_HWLOC_INSTALLED
#    include <hwloc.h>
#endif

#ifdef TKIT_OS_WINDOWS
#    include <windows.h>
#else
#    include <pthread.h>
#    include <sched.h>
#endif

namespace TKit::Topology
{
static thread_local usize t_ThreadIndex = 0;
usize GetThreadIndex()
{
    return t_ThreadIndex;
}
void SetThreadIndex(const usize threadIndex)
{
    t_ThreadIndex = threadIndex;
}

void SetThreadName(const u32 threadIndex, const char *name)
{
#ifdef TKIT_OS_WINDOWS
    const HANDLE thread = GetCurrentThread();
    std::ostringstream oss;
    if (name)
        oss << name;
    else
        oss << "tkit-worker-" << threadIndex;

    const auto str = oss.str();
    const std::wstring wname(str.begin(), str.end());

    SetThreadDescription(thread, wname.c_str());
#else
    const std::string sname = name ? name : ("tkit-worker-" + std::to_string(threadIndex));
#endif
#ifdef TKIT_OS_LINUX
    const pthread_t current = pthread_self();
    pthread_setname_np(current, sname.c_str());
#elif defined(TKIT_OS_APPLE)
    pthread_setname_np(sname.c_str());
#endif
}

#ifdef TKIT_HWLOC_INSTALLED
constexpr u32 Unknown = TKIT_U32_MAX;

struct Handle
{
    hwloc_topology_t Topology = nullptr;
};

static DynamicArray<u32> s_BuildOrder{};

struct KindInfo
{
    enum CoreType
    {
        Unk = Unknown,
        IntelCore = 0,
        IntelAtom = 1
    };
    u32 Rank = Unknown;
    u32 Efficiency = Unknown;
    CoreType CType = Unk;
};

static KindInfo getKindInfo(const hwloc_topology_t topology, const u32 PuIndex)
{
    const i32 err = hwloc_cpukinds_get_nr(topology, 0);
    KindInfo kinfo{};

    if (err == -1)
        return kinfo;

    const u32 nr = static_cast<u32>(err);
    for (u32 i = 0; i < nr; ++i)
    {
        const hwloc_bitmap_t set = hwloc_bitmap_alloc();
        i32 eff = -1;
        u32 ninfos = 0;
        struct hwloc_info_s *infos = nullptr;

        if (hwloc_cpukinds_get_info(topology, i, set, &eff, &ninfos, &infos, 0) == 0 &&
            hwloc_bitmap_isset(set, PuIndex))
        {
            kinfo.Rank = i;
            kinfo.Efficiency = eff == -1 ? Unknown : static_cast<u32>(eff);
            for (u32 j = 0; j < ninfos; j++)
            {
                if (strcmp(infos[j].name, "CoreType") != 0)
                    continue;
                if (strcmp(infos[j].value, "IntelCore") == 0)
                    kinfo.CType = KindInfo::IntelCore;
                else if (strcmp(infos[j].value, "IntelAtom") == 0)
                    kinfo.CType = KindInfo::IntelAtom;
            }

            hwloc_bitmap_free(set);
            return kinfo;
        }
        hwloc_bitmap_free(set);
    }

    return kinfo;
}

#    ifdef TKIT_ENABLE_DEBUG_LOGS
static std::string toString(const u32 value)
{
    if (value == Unknown)
        return "Unknown";
    return std::to_string(value);
}
static std::string toString(const KindInfo::CoreType cType)
{
    switch (cType)
    {
    case KindInfo::IntelCore:
        return "IntelCore";
    case KindInfo::IntelAtom:
        return "IntelAtom";
    case KindInfo::Unk:
        return "Unknown";
    }
    return "Unknown";
}
#    endif

struct PuInfo
{
    u32 Pu = Unknown;
    u32 Numa = Unknown;
    u32 Core = Unknown;
    u32 SmtRank = Unknown;
    KindInfo KInfo{};
};

static hwloc_obj_t ancestor(const hwloc_topology_t topology, const hwloc_obj_t object, const hwloc_obj_type_t type)
{
    return hwloc_get_ancestor_obj_by_type(topology, type, object);
}

static DynamicArray<u32> buildOrder(const hwloc_topology_t topology)
{
    TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY] Building affinity order...");

    DynamicArray<u32> order{};

    const u32 nbpus = static_cast<u32>(hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU));
    TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY] Found {} PUs", nbpus);
    if (nbpus == 0)
        return order;

    DynamicArray<PuInfo> infos{};
    infos.Reserve(nbpus);

    for (u32 i = 0; i < nbpus; ++i)
    {
        const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
        if (!pu)
        {
            TKIT_LOG_WARNING("[TOOLKIT][TOPOLOGY]    PU {} is NULL", i);
            continue;
        }

        PuInfo p{};
        p.Pu = pu->os_index;
        if (const hwloc_obj_t numa = ancestor(topology, pu, HWLOC_OBJ_NUMANODE))
            p.Numa = numa->os_index;
        if (const hwloc_obj_t core = ancestor(topology, pu, HWLOC_OBJ_CORE))
            p.Core = core->os_index;

        u32 rank = 0;
        if (const hwloc_obj_t core = ancestor(topology, pu, HWLOC_OBJ_CORE))
        {
            const auto next = [topology, core](const hwloc_obj_t child = nullptr) {
                return hwloc_get_next_obj_inside_cpuset_by_type(topology, core->cpuset, HWLOC_OBJ_PU, child);
            };
            for (hwloc_obj_t child = next(); child; child = next(child))
            {
                if (child->os_index == pu->os_index)
                    break;
                ++rank;
            }
        }
        p.SmtRank = rank;
        p.KInfo = getKindInfo(topology, p.Pu);

        // TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    PU {} SMT rank: {}", i, toString(p.SmtRank));
        // TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    PU {} Kind rank: {}", i, toString(p.KInfo.Rank));
        // TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    PU {} Efficiency rank: {}", i, toString(p.KInfo.Efficiency));
        // TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    PU {} Core type: {}", i, toString(p.KInfo.CType));

        infos.Append(p);
    }

    std::sort(infos.begin(), infos.end(), [](const PuInfo &node1, const PuInfo &node2) {
        const u32 e1 = node1.KInfo.Efficiency;
        const u32 e2 = node2.KInfo.Efficiency;

        if (e1 != Unknown && e2 != Unknown && e1 != e2)
            return e1 > e2;

        if (node1.SmtRank != node2.SmtRank)
            return node1.SmtRank < node2.SmtRank;

        if (node1.KInfo.CType != node2.KInfo.CType)
            return node1.KInfo.CType < node2.KInfo.CType;

        const u32 r1 = node1.KInfo.Rank;
        const u32 r2 = node2.KInfo.Rank;

        if (r1 != Unknown && r2 != Unknown && r1 != r2)
            return r1 > r2;

        if (node1.Core != node2.Core)
            return node1.Core < node2.Core;

        return node1.Pu < node2.Pu;
    });

    TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY] Gathered all PUs. Sorting by desirability...");
    for (u32 i = 0; i < infos.GetSize(); ++i)
    {
        const PuInfo &p = infos[i];
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY] Pu reserved to thread with index {}:", i);
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    Pu: {}", toString(p.Pu));
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    Core: {}", toString(p.Core));
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    Numa: {}", toString(p.Numa));
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    SMT rank: {}", toString(p.SmtRank));
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    Kind score: {}", toString(p.KInfo.Rank));
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    Efficiency score: {}", toString(p.KInfo.Efficiency));
        TKIT_LOG_DEBUG("[TOOLKIT][TOPOLOGY]    Core type: {}", toString(p.KInfo.CType));
        order.Append(p.Pu);
    }

    return order;
}

static void bindCurrentThread(const hwloc_topology_t topology, const u32 puIndex)
{
    const hwloc_obj_t pu = hwloc_get_pu_obj_by_os_index(topology, puIndex);
    if (!pu)
    {
        TKIT_LOG_WARNING("[TOOLKIT][TOPOLOGY] Failed to bind PU index {}: PU was NULL", puIndex);
        return;
    }

    const hwloc_cpuset_t set = hwloc_bitmap_dup(pu->cpuset);
    hwloc_bitmap_singlify(set);

    TKIT_LOG_WARNING_IF_NOT_RETURNS(hwloc_set_cpubind(topology, set, HWLOC_CPUBIND_THREAD), 0,
                                    "[TOOLKIT][TOPOLOGY] CPU Bind to Pu index {} failed", puIndex);
    hwloc_bitmap_free(set);
}

void BuildAffinityOrder(const Handle *handle)
{
    if (s_BuildOrder.IsEmpty())
    {
        s_BuildOrder = buildOrder(handle->Topology);
        return;
    }
}

void PinThread(const Handle *handle, const u32 threadIndex)
{
    if (s_BuildOrder.IsEmpty())
        return;
    const u32 index = threadIndex % s_BuildOrder.GetSize();
    const u32 pindex = s_BuildOrder[index];
    bindCurrentThread(handle->Topology, pindex);
}

const Handle *Initialize()
{
    Handle *handle = new Handle;
    hwloc_topology_init(&handle->Topology);
    hwloc_topology_set_flags(handle->Topology, HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM);
    hwloc_topology_load(handle->Topology);
    return handle;
}

void Terminate(const Handle *handle)
{
    hwloc_topology_destroy(handle->Topology);
    delete handle;
}
#else
struct Handle
{
};
void BuildAffinityOrder(const Handle *)
{
}

void PinThread(const Handle *, const u32)
{
}

const Handle *Initialize()
{
    TKIT_LOG_WARNING(
        "[TOOLKIT][TOPOLOGY] The library HWLOC, required to pin threads to optimal cpu cores, has not been found. "
        "Thread affinity will be disabled and threads will be scheduled by default, which may be non-optimal.");
    return nullptr;
}

void Terminate(const Handle *)
{
}
#endif
} // namespace TKit::Topology
