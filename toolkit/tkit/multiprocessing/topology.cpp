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

static KindInfo getKindInfo(const hwloc_topology_t p_Topology, const u32 PuIndex) noexcept
{
    const i32 err = hwloc_cpukinds_get_nr(p_Topology, 0);
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

        if (hwloc_cpukinds_get_info(p_Topology, i, set, &eff, &ninfos, &infos, 0) == 0 &&
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

#    ifdef TKIT_ENABLE_INFO_LOGS
static std::string toString(const u32 p_Value) noexcept
{
    if (p_Value == Unknown)
        return "Unknown";
    return std::to_string(p_Value);
}
static std::string toString(const KindInfo::CoreType p_CType) noexcept
{
    switch (p_CType)
    {
    case KindInfo::IntelCore:
        return "IntelCore";
    case KindInfo::IntelAtom:
        return "IntelAtom";
    default:
        return "Unknown";
    }
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
        p.KInfo = getKindInfo(p_Topology, p.Pu);

        // TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    PU {} SMT rank: {}", i, toString(p.SmtRank));
        // TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    PU {} Kind rank: {}", i, toString(p.KInfo.Rank));
        // TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    PU {} Efficiency rank: {}", i, toString(p.KInfo.Efficiency));
        // TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    PU {} Core type: {}", i, toString(p.KInfo.CType));

        infos.Append(p);
    }

    std::sort(infos.begin(), infos.end(), [](const PuInfo &p_Node1, const PuInfo &p_Node2) {
        if (p_Node1.SmtRank != p_Node2.SmtRank)
            return p_Node1.SmtRank < p_Node2.SmtRank;

        const u32 e1 = p_Node1.KInfo.Efficiency;
        const u32 e2 = p_Node2.KInfo.Efficiency;

        if (e1 != Unknown && e2 != Unknown && e1 != e2)
            return e1 > e2;

        if (p_Node1.KInfo.CType != p_Node2.KInfo.CType)
            return p_Node1.KInfo.CType < p_Node2.KInfo.CType;

        const u32 r1 = p_Node1.KInfo.Rank;
        const u32 r2 = p_Node2.KInfo.Rank;

        if (r1 != Unknown && r2 != Unknown && r1 != r2)
            return r1 > r2;

        if (p_Node1.Core != p_Node2.Core)
            return p_Node1.Core < p_Node2.Core;

        return p_Node1.Pu < p_Node2.Pu;
    });

    TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] Gathered all PUs. Sorting by desirability...");
    for (u32 i = 0; i < infos.GetSize(); ++i)
    {
        const PuInfo &p = infos[i];
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY] Pu reserved to thread with index {}:", i);
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    Pu: {}", toString(p.Pu));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    Core: {}", toString(p.Core));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    Numa: {}", toString(p.Numa));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    SMT rank: {}", toString(p.SmtRank));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    Kind score: {}", toString(p.KInfo.Rank));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    Efficiency score: {}", toString(p.KInfo.Efficiency));
        TKIT_LOG_INFO("[TOOLKIT][TOPOLOGY]    Core type: {}", toString(p.KInfo.CType));
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

    TKIT_ASSERT_RETURNS(hwloc_set_cpubind(p_Topology, set, HWLOC_CPUBIND_THREAD), 0,
                        "[TOOLKIT][TOPOLOGY] CPU Bind to Pu index {} failed", p_PuIndex);
    hwloc_bitmap_free(set);
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
    hwloc_topology_set_flags(handle->Topology, HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM);
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
void BuildAffinityOrder(const Handle *) noexcept
{
}

void PinThread(const Handle *) noexcept
{
}

const Handle *Initialize() noexcept
{
    TKIT_LOG_WARNING(
        "[TOOLKIT][TOPOLOGY] The library HWLOC, required to pin threads to optimal cpu cores, has not been found. "
        "Thread affinity will be disabled and threads will be scheduled by default, which may be non-optimal.");
    return nullptr;
}

void Terminate(const Handle *) noexcept
{
}
#endif
} // namespace TKit::Topology
