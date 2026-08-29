#include "tkit/core/pch.hpp"
#include "tkit/container/ecs.hpp"

namespace TKit
{
void ComponentInfo::CopyConstructFromRange(std::byte *dstBegin, const std::byte *srcBegin,
                                           const std::byte *srcEnd) const
{
    if (CopyCtor)
    {
        std::byte *dst = dstBegin;
        for (const std::byte *src = srcBegin; src != srcEnd; src += Size, dst += Size)
            CopyCtor(dst, src);
    }
    else
        ForwardCopy(dstBegin, srcBegin, srcEnd);
}

void ComponentInfo::CopyAssignFromRange(std::byte *dstBegin, std::byte *dstEnd, const std::byte *srcBegin,
                                        const std::byte *srcEnd) const
{
    const usize dstSize = usize(std::distance(dstBegin, dstEnd));
    const usize srcSize = usize(std::distance(srcBegin, srcEnd));

    const auto destroyLeftovers = [&] {
        if (Destroy && srcSize < dstSize)
        {
            for (const std::byte *dst = dstBegin + srcSize; dst != dstEnd; dst += Size)
                Destroy(dst);
            return true;
        }
        return false;
    };

    if (!CopyCtor && !CopyAssigner)
    {
        ForwardCopy(dstBegin, srcBegin, srcEnd);
        destroyLeftovers();
    }
    else
    {
        if (CopyAssigner)
        {
            std::byte *dst = dstBegin;
            const std::byte *src = srcBegin;
            for (; src != srcEnd && dst != dstEnd; src += Size, dst += Size)
                CopyAssigner(dst, src);
        }
        else
        {
            const usize minSize = dstSize < srcSize ? dstSize : srcSize;
            ForwardCopy(dstBegin, srcBegin, srcBegin + minSize);
        }
        if (!destroyLeftovers())
        {
            if (CopyCtor)
            {
                std::byte *dst = dstEnd;
                const std::byte *src = srcBegin + dstSize;
                for (; src != srcEnd; src += Size, dst += Size)
                    CopyCtor(dst, src);
            }
            else
                ForwardCopy(dstEnd, srcBegin + dstSize, srcEnd);
        }
    }
}
void ComponentColumn::Append(void *component)
{
    resizeIfNeeded();
    const usize row = m_RowCount++;
    void *dst = Get(row);
    m_Info.MoveConstruct(dst, component);
}

void ComponentColumn::Pop()
{
    const usize lidx = m_RowCount - 1;
    void *component = Get(lidx);
    if (m_Info.Destroy)
        m_Info.Destroy(component);
    m_RowCount = lidx;
}
void ComponentColumn::Remove(const usize row)
{
    const usize lidx = m_RowCount - 1;
    void *component = Get(row);
    void *last = Get(lidx);
    m_Info.MoveAssign(component, last);
    if (m_Info.Destroy)
        m_Info.Destroy(last);
    m_RowCount = lidx;
}

void ComponentColumn::resize(const usize capacity)
{
    TKIT_ASSERT(capacity > m_RowCapacity, "[TOOLKIT][ECS] New capacity must be bigger than old capacity");
    m_RowCapacity = capacity;
    void *ndata = TKit::AllocateAligned(m_Info.Size * m_RowCapacity, m_Info.Alignment);
    if (m_Data)
    {
        for (u32 r = 0; r < m_RowCount; ++r)
        {
            void *src = Get(r);
            void *dst = scast<std::byte *>(ndata) + r * m_Info.Size;
            m_Info.MoveConstruct(dst, src);
        }
        TKit::DeallocateAligned(m_Data);
    }
    m_Data = ndata;
}

void Archetype::AddColumn(const ComponentInfo &cinfo)
{
    TKIT_ASSERT(m_Entities.IsEmpty(), "[TOOLKIT][ECS] Can only add columns to an archetype if it is empty");
    TKIT_ASSERT(m_Id == TKIT_USZ_MAX,
                "[TOOLKIT][ECS] Can only add columns to an archetype if it has not been finalized");

    m_ColumnByComponent[cinfo.Id] = m_Columns.GetSize();
    m_Columns.Append(cinfo);
    for (u32 i = 0; i < m_ComponentIdSet.GetSize(); ++i)
        if (m_ComponentIdSet[i] > cinfo.Id)
        {
            m_ComponentIdSet.Insert(m_ComponentIdSet.begin() + i, cinfo.Id);
            return;
        }
    m_ComponentIdSet.Append(cinfo.Id);
}

usz Archetype::ComputeArchetypeIdToAdd(const ComponentId cid) const
{
    StackArray<ComponentId> extSet{};
    extSet.Reserve(m_ComponentIdSet.GetSize() + 1);
    extSet = m_ComponentIdSet;

    for (u32 i = 0; i < extSet.GetSize(); ++i)
        if (extSet[i] > cid)
        {
            extSet.Insert(extSet.begin() + i, cid);
            return CreateIdFromComponents(extSet);
        }
    extSet.Append(cid);
    return CreateIdFromComponents(extSet);
}
usz Archetype::ComputeArchetypeIdToRemove(const ComponentId cid) const
{
    StackArray<ComponentId> extSet{};
    extSet.Reserve(m_ComponentIdSet.GetSize() - 1);
    for (const ComponentId c : m_ComponentIdSet)
        if (c != cid)
            extSet.Append(c);
    return CreateIdFromComponents(extSet);
}

Entity Archetype::RemoveRowWithTransfer(const ComponentId cid, Archetype *dst, const usize srcRow)
{
    TKIT_ASSERT(
        m_ColumnByComponent.Contains(cid),
        "[TOOLKIT][ECS] When removing a row, the associated component must be contained in the source archetype");

    TKIT_ASSERT(dst->m_Columns.GetSize() == m_Columns.GetSize() - 1,
                "[TOOLKIT][ECS] When removing a row for a destination archetype, the "
                "destination archetype must exactly have one less column than the current archetype, but "
                "destination has {} "
                "columns and current has {}",
                dst->m_Columns.GetSize(), m_Columns.GetSize());

    for (ComponentColumn &dstCol : dst->m_Columns)
    {
        ComponentColumn &srcCol = getColumnForTransfer(dstCol);
        TKIT_ASSERT(cid != srcCol.GetInfo().Id, "[TOOLKIT][ECS] When transferring rows, the component that "
                                                "triggered such transfer must not be part of the transfer");
        transferComponent(srcRow, dstCol, srcCol);
    }

    m_RemoveEdges[cid] = dst;
    const usize idx = m_ColumnByComponent[cid];
    m_Columns[idx].Remove(srcRow);

    m_RemoveEdges[cid] = dst;
    return transferEntity(srcRow, dst, this);
}

Entity Archetype::RemoveRow(const usize row)
{
    for (ComponentColumn &col : m_Columns)
        col.Remove(row);
    return removeEntity(row);
}

bool Archetype::HasComponents(const Span<const ComponentId> ids) const
{
    usize i = 0;
    usize j = 0;
    const usize ni = m_ComponentIdSet.GetSize();
    const usize nj = ids.GetSize();
    while (i < ni && j < nj)
    {
        if (m_ComponentIdSet[i] == ids[j])
        {
            ++i;
            ++j;
        }
        else if (m_ComponentIdSet[i] < ids[j])
            ++i;
        else
            return false;
    }
    return j == nj;
}

ComponentColumn &Archetype::getColumnForTransfer(const ComponentColumn &other)
{
    const ComponentId cid = other.GetInfo().Id;
    TKIT_ASSERT(m_ColumnByComponent.Contains(cid), "[TOOLKIT][ECS] When transfering a row, all components "
                                                   "must be registered in both archetypes");

    const usize cidx = m_ColumnByComponent[cid];
    return m_Columns[cidx];
}

void Archetype::transferComponent(const usize srcRow, ComponentColumn &dst, ComponentColumn &src)
{
    void *srcComp = src.Get(srcRow);
    dst.Append(srcComp);
    src.Remove(srcRow);
}

Entity Archetype::transferEntity(const usize srcRow, Archetype *dst, Archetype *src)
{
    dst->m_Entities.Append(src->m_Entities[srcRow]);
    return src->removeEntity(srcRow);
}
Entity Archetype::removeEntity(const usize row)
{
    m_Entities.RemoveUnordered(m_Entities.begin() + row);
    return row != GetRowCount() ? m_Entities[row] : NullEntity;
}

Registry::~Registry()
{
    cleanup();
}

void Registry::DestroyEntity(const Entity e)
{
    const EntityRecord &record = m_Entities[e];
    if (record.Archetype)
    {
        const Entity shuffled = record.Archetype->RemoveRow(record.Row);
        if (shuffled != NullEntity)
            m_Entities[shuffled].Row = record.Row;
    }
    m_Entities.Remove(e);
}

void Registry::cleanup()
{
    for (Archetype *arch : m_Archetypes)
        destroyArchetype(arch);
    for (const KeyValuePair<const usz, CachedQuery> &pair : m_Queries)
    {
        const CachedQuery &q = pair.Value;
        q.Destroy(q.Query);
    }
}

Archetype *Registry::createArchetype(const usz archId)
{
    TierAllocator *tier = GetTier();
    Archetype *arch = tier->Create<Archetype>();
    m_ArchetypeById[archId] = arch;
    return m_Archetypes.Append(arch);
}

void Registry::destroyArchetype(const Archetype *arch)
{
    TierAllocator *tier = GetTier();
    tier->Destroy(arch);
}

} // namespace TKit
