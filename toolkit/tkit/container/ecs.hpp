#pragma once

#ifndef TKIT_ENABLE_ECS
#    error                                                                                                             \
        "[TOOLKIT][ECS] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_ECS"
#endif

#include "tkit/container/tier_array.hpp"
#include "tkit/container/hash_map.hpp"
#include "tkit/container/span.hpp"
#include "tkit/memory/memory.hpp"

namespace TKit
{
using ComponentId = usize;

inline ComponentId NextComponentId()
{
    static ComponentId counter = 0;
    return counter++;
}

template <typename T> ComponentId GetComponentId()
{
    static const ComponentId id = NextComponentId();
    return id;
}
template <typename... T> void RegisterComponentIds()
{
    (GetComponentId<T>(), ...);
}

struct ComponentInfo
{
    ComponentId Id;
    usize Size;
    usize Alignment;

    void (*Destroy)(const void *);
    // void (*CopyCtor)(void *dst, const void *src);
    void (*MoveCtor)(void *dst, void *src);
    void (*MoveAssign)(void *dst, void *src);

#ifdef TKIT_ENABLE_ENSURE
    bool Registered = false;
#endif

    template <typename T> static ComponentInfo Create()
    {
        ComponentInfo info;
        info.Id = GetComponentId<T>();
        info.Size = sizeof(T);
        info.Alignment = alignof(T);

        info.Destroy = [](const void *ptr) { Destruct(scast<const T *>(ptr)); };
        // info.CopyCtor = [](const void *dst, void *src) { Construct(scast<T *>(dst), *scast<const T *>(src)); };
        info.MoveCtor = [](void *dst, void *src) { Construct(scast<T *>(dst), std::move(*scast<T *>(src))); };
        info.MoveAssign = [](void *dst, void *src) { *scast<T *>(dst) = std::move(*scast<T *>(src)); };
#ifdef TKIT_ENABLE_ENSURE
        info.Registered = true;
#endif

        return info;
    }
};

class ComponentColumn
{
  public:
    static constexpr usize StartingCapacity = 16;

    ComponentColumn(const ComponentInfo &info) : m_Info(info)
    {
        m_Data = AllocateAligned(StartingCapacity * info.Size, info.Alignment);
    }

    template <typename T, typename... Args> T *Append(Args &&...args)
    {
        const usize row = m_RowCount;
        ++m_RowCount;
        return Construct(Get<T>(row), std::forward<Args>(args)...);
    }

    void Append(void *component);

    void Pop();
    void Remove(usize row);

    void *Get(const usize row)
    {
        TKIT_ASSERT(row < m_RowCapacity,
                    "[TOOLKIT][ECS] The row index ({}) exceeds component data buffer's capacity ({})", row,
                    m_RowCapacity);
        TKIT_ASSERT(row < m_RowCount, "[TOOLKIT][ECS] The row index ({}) exceeds component data buffer's size ({})",
                    row, m_RowCount);
        return scast<std::byte *>(m_Data) + row * m_Info.Size;
    }

    template <typename T> T *Get(const usize row)
    {
        TKIT_ASSERT(sizeof(T) == m_Info.Size,
                    "[TOOLKIT][ECS] Size mismatch between sizeof(T) = {} and element size = {}", sizeof(T),
                    m_Info.Size);
        return scast<T *>(Get(row));
    }

    usize GetRowCount() const
    {
        return m_RowCount;
    }

    const ComponentInfo &GetInfo() const
    {
        return m_Info;
    }

  private:
    void *m_Data;

    usize m_RowCount = 0;
    usize m_RowCapacity{StartingCapacity};
    ComponentInfo m_Info;
};

struct Entity
{
    usize Index;
#ifdef TKIT_ENABLE_ENSURE
    usize Generation = 0;
#endif
};

#ifdef TKIT_ENABLE_ENSURE
constexpr Entity NullEntity = {TKIT_USIZE_MAX, TKIT_USIZE_MAX};
#else
constexpr Entity NullEntity = {TKIT_USIZE_MAX};
#endif

constexpr bool operator==(const Entity &lhs, const Entity &rhs)
{
#ifdef TKIT_ENABLE_ENSURE
    return lhs.Index == rhs.Index && lhs.Generation == rhs.Generation;
#else
    return lhs.Index == rhs.Index;
#endif
}

class Archetype;
struct EntityRecord
{
    Archetype *Archetype = nullptr;
    usize Row = TKIT_USIZE_MAX;
#ifdef TKIT_ENABLE_ENSURE
    usize Generation = 0;
    bool Alive = true;
#endif
};

class Archetype
{
  public:
    template <typename T> struct TransferResult
    {
        T *Added;
        Entity ShuffledEntity;
    };

    void AddColumn(const ComponentInfo &cinfo);

    void FinalizeColumns(const usz id)
    {
        m_Id = id;
    }
    void FinalizeColumns()
    {
        FinalizeColumns(CreateIdFromComponents(m_ComponentIdSet));
    }

    usz ComputeArchetypeIdToAdd(ComponentId cid) const;
    usz ComputeArchetypeIdToRemove(ComponentId cid) const;

    template <typename T, typename... Args> T *AddRow(const Entity e, Args &&...args)
    {
        TKIT_ASSERT(m_Columns.GetSize() == 1,
                    "[TOOLKIT][ECS] If adding a row from a single component, the column count of the archetype must be "
                    "exactly one, but is {}",
                    m_Columns.GetSize());
        TKIT_ASSERT(
            m_ColumnByComponent.Contains(GetComponentId<T>()),
            "[TOOLKIT][ECS] When adding a row with one component, the archetype must have that component as a row");

        m_Entities.Append(e);
        return emplaceAtColumn<T>(0, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    TransferResult<T> AddRowWithTransfer(Archetype *src, const usize srcRow, Args &&...args)
    {
        const ComponentId cid = GetComponentId<T>();
        TKIT_ASSERT(m_ColumnByComponent.Contains(cid), "[TOOLKIT][ECS] When adding a row, all source components "
                                                       "must be registered in the destination archetype");

        TKIT_ASSERT(src->m_Columns.GetSize() == m_Columns.GetSize() - 1,
                    "[TOOLKIT][ECS] When adding a row from a source archetype, the "
                    "source archetype must exactly have one less column than the destination archetype, but source has "
                    "{} column(s) and destination has {} column(s)",
                    src->m_Columns.GetSize(), m_Columns.GetSize());

        for (ComponentColumn &srcCol : src->m_Columns)
        {
            ComponentColumn &dstCol = getColumnForTransfer(srcCol);
            TKIT_ASSERT(cid != dstCol.GetInfo().Id, "[TOOLKIT][ECS] When transferring rows, the component that "
                                                    "triggered such transfer must not be part of the transfer");
            transferComponent(srcRow, dstCol, srcCol);
        }

        src->m_AddEdges[cid] = this;
        const Entity e = transferEntity(srcRow, this, src);

        const usize idx = m_ColumnByComponent[cid];
        return {emplaceAtColumn<T>(idx, std::forward<Args>(args)...), e};
    }

    Entity RemoveRowWithTransfer(ComponentId cid, Archetype *dst, usize srcRow);

    Archetype *GetArchetypeToAdd(const ComponentId cid) const
    {
        const auto it = m_AddEdges.Find(cid);
        if (it == m_AddEdges.end())
            return nullptr;
        return it->Value;
    }

    Archetype *GetArchetypeToRemove(const ComponentId cid) const
    {
        const auto it = m_RemoveEdges.Find(cid);
        if (it == m_RemoveEdges.end())
            return nullptr;
        return it->Value;
    }

    ComponentColumn *GetColumn(const ComponentId cid)
    {
        const auto it = m_ColumnByComponent.Find(cid);
        if (it == m_ColumnByComponent.end())
            return nullptr;
        return &m_Columns[it->Value];
    }

    const TierArray<ComponentId> &GetComponentIds() const
    {
        return m_ComponentIdSet;
    }

    usize GetRowCount() const
    {
        return m_Entities.GetSize();
    }

    static usz CreateIdFromComponents(const Span<const ComponentId> ids)
    {
        return HashRange(ids.begin(), ids.end());
    }
    static usz CreateIdFromComponent(const ComponentId id)
    {
        return Hash(id);
    }

  private:
    template <typename T, typename... Args> T *emplaceAtColumn(const usize idx, Args &&...args)
    {
        TKIT_ASSERT(m_Columns[idx].GetRowCount() == GetRowCount() - 1,
                    "[TOOLKIT][ECS] The row count of all the columns must match the archetype's row count. Column row "
                    "count is {} when it should be {}",
                    m_Columns[idx].GetRowCount(), GetRowCount() - 1);
        return m_Columns[idx].Append<T>(std::forward<Args>(args)...);
    }
    void transferRow(Archetype *source, usize srcRow);

    ComponentColumn &getColumnForTransfer(const ComponentColumn &other);
    void transferComponent(usize srcRow, ComponentColumn &dst, ComponentColumn &src);
    static Entity transferEntity(usize srcRow, Archetype *dst, Archetype *src);

    usz m_Id = TKIT_USZ_MAX;
    TierArray<ComponentId> m_ComponentIdSet{};
    TierArray<ComponentColumn> m_Columns{};
    TierArray<Entity> m_Entities{};

    TierHashMap<ComponentId, usize> m_ColumnByComponent{};
    TierHashMap<ComponentId, Archetype *> m_AddEdges{};
    TierHashMap<ComponentId, Archetype *> m_RemoveEdges{};
};

class Registry
{
    TKIT_NON_COPYABLE(Registry)
  public:
    Registry() = default;
    ~Registry();

    Entity CreateEntity();

    template <typename T> void RegisterComponent()
    {
        const ComponentId cid = GetComponentId<T>();
        if (cid >= m_Components.GetSize())
            m_Components.Resize(cid + 1);

        TKIT_ENSURE(!m_Components[cid].Registered, "[TOOLKIT][ECS] Cannot register an already registered component");
        m_Components[cid] = ComponentInfo::Create<T>();
    }

    template <typename... T> void RegisterComponents()
    {
        (RegisterComponent<T>(), ...);
    }

    template <typename T> T *GetComponent(const Entity e) const
    {
        TKIT_ENSURE(IsAlive(e), "[TOOLKIT][ECS] The entity with index {} is not alive and cannot be queried", e.Index);
        const EntityRecord &r = m_Entities[e.Index];
        Archetype *arch = r.Archetype;
        if (!arch)
            return nullptr;

        const ComponentId cid = GetComponentId<T>();

        ComponentColumn *column = arch->GetColumn(cid);
        if (!column)
            return nullptr;

        TKIT_ASSERT(r.Row < arch->GetRowCount(),
                    "[TOOLKIT][ECS] Found entity having a row ({}) greater or equal to its archetype's row count ({})",
                    r.Row, arch->GetRowCount());

        return column->Get<T>(r.Row);
    }

    template <typename T, typename... Args> T *AddComponent(const Entity e, Args &&...args)
    {
        const ComponentId cid = GetComponentId<T>();

        TKIT_ENSURE(IsAlive(e), "[TOOLKIT][ECS] The entity with index {} is not alive and cannot be queried", e.Index);
        TKIT_ASSERT(!GetComponent<T>(e), "[TOOLKIT][ECS] The entity with index {} already has a component with id {}",
                    e.Index, cid);
        EntityRecord &r = m_Entities[e.Index];
        Archetype *arch = r.Archetype;

        if (!arch)
        {
            TKIT_ASSERT(r.Row == TKIT_USIZE_MAX,
                        "[TOOLKIT][ECS] If an entity has no archetype, its row number must be uninitialized");

            const usz archId = Archetype::CreateIdFromComponent(cid);
            const auto it = m_ArchetypeById.Find(archId);
            if (it == m_ArchetypeById.end())
            {
                arch = createArchetype();
                arch->AddColumn(m_Components[cid]);
                arch->FinalizeColumns(archId);
            }

            r.Archetype = arch;
            r.Row = arch->GetRowCount();
            return arch->AddRow<T>(e, std::forward<Args>(args)...);
        }

        Archetype *src = arch;
        Archetype *dst = src->GetArchetypeToAdd(cid);

        if (!dst)
        {
            const usz archId = src->ComputeArchetypeIdToAdd(cid);
            const auto it = m_ArchetypeById.Find(archId);
            if (it == m_ArchetypeById.end())
            {
                dst = createArchetype();

                for (const ComponentId c : src->GetComponentIds())
                    dst->AddColumn(m_Components[c]);
                dst->AddColumn(m_Components[cid]);

                dst->FinalizeColumns(archId);
            }
        }

        const usize nrow = dst->GetRowCount();
        const Archetype::TransferResult<T> result = dst->AddRowWithTransfer<T>(src, r.Row, std::forward<Args>(args)...);
        if (result.ShuffledEntity != NullEntity)
        {
            TKIT_ASSERT(result.ShuffledEntity != e,
                        "[TOOLKIT][ECS] Target and shuffled entity cannot possibly be the same");
            m_Entities[result.ShuffledEntity.Index].Row = r.Row;
        }

        r.Row = nrow;
        r.Archetype = dst;
        return result.Added;
    }

    template <typename T> void RemoveComponent(const Entity e)
    {
        TKIT_ENSURE(IsAlive(e), "[TOOLKIT][ECS] The entity with index {} is not alive and cannot be queried", e.Index);
        EntityRecord &r = m_Entities[e.Index];
        Archetype *src = r.Archetype;
        TKIT_ASSERT(src, "[TOOLKIT][ECS] To remove a component, target entity must have a valid archetype");

        const ComponentId cid = GetComponentId<T>();
        Archetype *dst = src->GetArchetypeToRemove(cid);
        if (!dst)
        {
            const usz archId = src->ComputeArchetypeIdToRemove(cid);
            TKIT_ASSERT(m_ArchetypeById.Contains(archId),
                        "[TOOLKIT][ECS] When removing an entity, the archetype id to remove must be already present, "
                        "as it is not possible to add multiple components simultaneously");
            dst = m_ArchetypeById[archId];
        }

        const usize nrow = dst->GetRowCount();
        const Entity shuffled = src->RemoveRowWithTransfer(cid, dst, r.Row);

        if (shuffled != NullEntity)
        {
            TKIT_ASSERT(shuffled != e, "[TOOLKIT][ECS] Target and shuffled entity cannot possibly be the same");
            m_Entities[shuffled.Index].Row = r.Row;
        }
        r.Row = nrow;
        r.Archetype = dst;
    }

#ifdef TKIT_ENABLE_ENSURE
    bool IsAlive(const Entity e) const
    {
        return e != NullEntity && m_Entities[e.Index].Generation == e.Generation;
    }
#endif

  private:
    Archetype *createArchetype();
    void destroyArchetype(const Archetype *arch);
    void removeArchetype(const Archetype *arch);

    TierArray<EntityRecord> m_Entities{};
    TierArray<Entity> m_FreeEntities{};
    TierArray<ComponentInfo> m_Components{};
    TierArray<Archetype *> m_Archetypes{};

    TierHashMap<usz, Archetype *> m_ArchetypeById{};
};

} // namespace TKit
