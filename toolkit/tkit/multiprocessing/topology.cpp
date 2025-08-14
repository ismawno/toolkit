#include "tkit/core/pch.hpp"
#include "tkit/multiprocessing/topology.hpp"
#include "tkit/container/dynamic_array.hpp"

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

void SetThreadName(const u32 p_ThreadIndex, const char *p_Name) noexcept
{
#ifdef TKIT_OS_WINDOWS
    const HANDLE thread = GetCurrentThread();
    std::ostringstream oss;
    if (p_Name)
        oss << p_Name;
    else
        oss << "tkit-worker-" << p_ThreadIndex;

    const auto str = oss.str();
    const std::wstring name(str.begin(), str.end());

    SetThreadDescription(thread, name.c_str());
#else
    const std::string name = p_Name ? p_Name : ("tkit-worker-" + std::to_string(p_ThreadIndex));
#endif
#ifdef TKIT_OS_LINUX
    const pthread_t current = pthread_self();
    pthread_setname_np(current, name.c_str());
#else
    pthread_setname_np(name.c_str());
#endif
}

#ifdef TKIT_HWLOC_INSTALLED
struct Handle
{
    hwloc_topology_t Topology = nullptr;
};

DynamicArray<u32> s_BuildOrder{};

static u32 kindRank(const hwloc_topology_t p_Topology, const u32 PuIndex) noexcept
{
    const i32 err = hwloc_cpukinds_get_nr(p_Topology, 0);
    if (err == -1)
        return Unknown;
    const u32 nr = static_cast<u32>(err);
    for (u32 i = 0; i < nr; ++i)
    {
        const hwloc_bitmap_t set = hwloc_bitmap_alloc();
        if (hwloc_cpukinds_get_info(p_Topology, i, set, nullptr, nullptr, nullptr, 0) == 0 &&
            hwloc_bitmap_isset(set, PuIndex))
        {
            hwloc_bitmap_free(set);
            return i;
        }
        hwloc_bitmap_free(set);
    }

    return Unknown;
}

#    ifdef TKIT_ENABLE_INFO_LOGS
static std::string toString(const u32 p_Value) noexcept
{
    if (p_Value == Unknown)
        return "Unknown";
    return std::to_string(p_Value);
}
#    endif

struct PuInfo
{
    u32 Pu = Unknown;
    u32 Numa = Unknown;
    u32 Core = Unknown;
    u32 SmtRank = Unknown;
    u32 KindRank = Unknown;
};

static hwloc_obj_t ancestor(const hwloc_topology_t p_Topology, const hwloc_obj_t p_Object,
                            const hwloc_obj_type_t p_Type)
{
    return hwloc_get_ancestor_obj_by_type(p_Topology, p_Type, p_Object);
}

static DynamicArray<u32> buildOrder(const hwloc_topology_t p_Topology)
{
    TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] Building affinity order...");

    DynamicArray<u32> order{};

    const u32 nbpus = static_cast<u32>(hwloc_get_nbobjs_by_type(p_Topology, HWLOC_OBJ_PU));
    TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] Found {} PUs", nbpus);
    if (nbpus == 0)
        return order;

    DynamicArray<PuInfo> infos{};
    infos.Reserve(nbpus);

    for (u32 i = 0; i < nbpus; ++i)
    {
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] Processing PU {}...", i);
        const hwloc_obj_t pu = hwloc_get_obj_by_type(p_Topology, HWLOC_OBJ_PU, i);
        if (!pu)
        {
            TKIT_LOG_WARNING("[TOOLKIT][TOPOLOGY]    PU {} is NULL", i);
            continue;
        }

        PuInfo p{};
        p.Pu = pu->os_index;
        if (const hwloc_obj_t numa = ancestor(p_Topology, pu, HWLOC_OBJ_NUMANODE))
            p.Numa = numa->os_index;
        if (const hwloc_obj_t core = ancestor(p_Topology, pu, HWLOC_OBJ_CORE))
            p.Core = core->os_index;

        u32 rank = 0;
        if (const hwloc_obj_t core = ancestor(p_Topology, pu, HWLOC_OBJ_CORE))
        {
            const auto next = [p_Topology, core](const hwloc_obj_t p_Child = nullptr) {
                return hwloc_get_next_obj_inside_cpuset_by_type(p_Topology, core->cpuset, HWLOC_OBJ_PU, p_Child);
            };
            for (hwloc_obj_t child = next(); child; child = next(child))
            {
                if (child->os_index == pu->os_index)
                    break;
                ++rank;
            }
        }
        p.SmtRank = rank;
        p.KindRank = kindRank(p_Topology, p.Pu);

        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    PU {} SMT Rank: {}", i, toString(p.SmtRank));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    PU {} Kind Rank: {}", i, toString(p.KindRank));

        infos.Append(p);
    }

    // Partition per node into buckets:
    // A: kind=0 & smt=0 (best single threads on P-cores)
    // B: kind=0 & smt>0 (P-core siblings)
    // C: kind>0 & smt=0 (E-cores primary)
    // D: kind>0 & smt>0 (E-cores siblings)
    // Fallback if kind unknown: F0 (smt=0), F1 (smt>0)

    std::sort(infos.begin(), infos.end(), [](const PuInfo &p_Node1, const PuInfo &p_Node2) {
        if (p_Node1.SmtRank != p_Node2.SmtRank)
            return p_Node1.SmtRank < p_Node2.SmtRank;
        if (p_Node1.KindRank != p_Node2.KindRank)
            return p_Node1.KindRank < p_Node2.KindRank;
        if (p_Node1.Core != p_Node2.Core)
            return p_Node1.Core < p_Node2.Core;
        return p_Node1.Pu < p_Node2.Pu;
    });

    TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] Gathered all PUs. Sorting by desirability...");
    for (const PuInfo &p : infos)
    {
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] PU - {}, Core - {}, Kind Rank - {}, SMT Rank - {}, Numa - {}",
                      toString(p.Pu), toString(p.Core), toString(p.KindRank), toString(p.SmtRank), toString(p.Numa));
        order.Append(p.Pu);
    }

    return order;
}

static void bindCurrentThread(const hwloc_topology_t p_Topology, const u32 p_PuIndex) noexcept
{
    const hwloc_obj_t pu = hwloc_get_pu_obj_by_os_index(p_Topology, p_PuIndex);
    if (!pu)
    {
        TKIT_LOG_WARNING("[TOOLKIT][TOPOLOGY] Failed to bind PU index {}: PU was NULL", p_PuIndex);
        return;
    }

    const hwloc_cpuset_t set = hwloc_bitmap_dup(pu->cpuset);
    hwloc_bitmap_singlify(set);

    const u32 rc = static_cast<u32>(hwloc_set_cpubind(p_Topology, set, HWLOC_CPUBIND_THREAD));
    hwloc_bitmap_free(set);
    TKIT_LOG_WARNING_IF(rc != 0, "[TOOLKIT][TOPOLOGY] CPU Bind failed...");
}

void BuildAffinityOrder(const Handle *p_Handle) noexcept
{
    if (s_BuildOrder.IsEmpty())
    {
        s_BuildOrder = buildOrder(p_Handle->Topology);
        return;
    }
    TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] A build order has already been created. Using that instead");
}

void PinThread(const Handle *p_Handle, const u32 p_ThreadIndex) noexcept
{
    if (s_BuildOrder.IsEmpty())
        return;
    const u32 index = p_ThreadIndex % s_BuildOrder.GetSize();
    const u32 pindex = s_BuildOrder[index];
    bindCurrentThread(p_Handle->Topology, pindex);
}

const Handle *Initialize() noexcept
{
    Handle *handle = new Handle;
    hwloc_topology_init(&handle->Topology);
    hwloc_topology_set_flags(handle->Topology,
                             HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED | HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM);
    hwloc_topology_load(handle->Topology);
    return handle;
}

void Terminate(const Handle *p_Handle) noexcept
{
    hwloc_topology_destroy(p_Handle->Topology);
    delete p_Handle;
}
#else
struct Handle
{
};
void BuildAffinityOrder(const Handle *p_Handle) noexcept
{
}

void PinThread(const Handle *p_Handle) noexcept
{
}

const Handle *Initialize() noexcept
{
    TKIT_LOG_WARNING(
        "[TOOLKIT][TOPOLOGY] The library HWLOC, required to pin threads to optimal cpu cores, has not been found. "
        "Thread affinity will be disabled and threads will be scheduled by default, which may be non-optimal.");
}

void Terminate(const Handle *p_Handle) noexcept
{
}
#endif
} // namespace TKit::Topology
