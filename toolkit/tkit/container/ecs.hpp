#pragma once

#ifndef TKIT_ENABLE_ECS
#    error                                                                                                             \
        "[TOOLKIT][ECS] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_ECS"
#endif

#include "tkit/container/tier_array.hpp"
#include "tkit/container/hash_map.hpp"
#include "tkit/container/span.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/utils.hpp"

namespace TKit
{
using ComponentId = usize;

inline ComponentId NextComponentId()
{
    static ComponentId counter = 0;
    return counter++;
}

template <typename C> ComponentId GetComponentId()
{
    static const ComponentId id = NextComponentId();
    return id;
}
template <typename... C> void RegisterComponentIds()
{
    (GetComponentId<C>(), ...);
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

    template <typename C> static ComponentInfo Create()
    {
        ComponentInfo info;
        info.Id = GetComponentId<C>();
        info.Size = sizeof(C);
        info.Alignment = alignof(C);

        info.Destroy = [](const void *ptr) { Destruct(scast<const C *>(ptr)); };
        // info.CopyCtor = [](const void *dst, void *src) { Construct(scast<C *>(dst), *scast<const C *>(src)); };
        info.MoveCtor = [](void *dst, void *src) { Construct(scast<C *>(dst), std::move(*scast<C *>(src))); };
        info.MoveAssign = [](void *dst, void *src) { *scast<C *>(dst) = std::move(*scast<C *>(src)); };
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

    template <typename C, typename... Args> C *Append(Args &&...args)
    {
        resizeIfNeeded();
        const usize row = m_RowCount;
        ++m_RowCount;
        return Construct(Get<C>(row), std::forward<Args>(args)...);
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

    template <typename C> C *Get(const usize row)
    {
        TKIT_ASSERT(sizeof(C) == m_Info.Size,
                    "[TOOLKIT][ECS] Size mismatch between sizeof(C) = {} and element size = {}", sizeof(C),
                    m_Info.Size);
        return scast<C *>(Get(row));
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
    void resizeIfNeeded();

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
    template <typename C> struct TransferResult
    {
        C *Added;
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

    template <typename C, typename... Args> C *AddRow(const Entity e, Args &&...args)
    {
        TKIT_ASSERT(m_Columns.GetSize() == 1,
                    "[TOOLKIT][ECS] If adding a row from a single component, the column count of the archetype must be "
                    "exactly one, but is {}",
                    m_Columns.GetSize());
        TKIT_ASSERT(
            m_ColumnByComponent.Contains(GetComponentId<C>()),
            "[TOOLKIT][ECS] When adding a row with one component, the archetype must have that component as a row");

        m_Entities.Append(e);
        return emplaceAtColumn<C>(0, std::forward<Args>(args)...);
    }

    template <typename C, typename... Args>
    TransferResult<C> AddRowWithTransfer(Archetype *src, const usize srcRow, Args &&...args)
    {
        const ComponentId cid = GetComponentId<C>();
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
        return {emplaceAtColumn<C>(idx, std::forward<Args>(args)...), e};
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

    ComponentColumn *QueryColumn(const ComponentId cid)
    {
        const auto it = m_ColumnByComponent.Find(cid);
        if (it == m_ColumnByComponent.end())
            return nullptr;
        return &m_Columns[it->Value];
    }
    ComponentColumn &GetColumn(const ComponentId cid)
    {
        TKIT_ASSERT(m_ColumnByComponent.Contains(cid), "[TOOLKIT][ECS] Archetype does not contain component with id {}",
                    cid);
        return m_Columns[m_ColumnByComponent[cid]];
    }

    const TierArray<ComponentId> &GetComponentIds() const
    {
        return m_ComponentIdSet;
    }

    usize GetRowCount() const
    {
        return m_Entities.GetSize();
    }
    Entity GetEntity(const usize row) const
    {
        return m_Entities[row];
    }

    bool HasComponents(const Span<const ComponentId> ids) const;

    static usz CreateIdFromComponents(const Span<const ComponentId> ids)
    {
        return HashRange(ids.begin(), ids.end());
    }
    template <typename... C> static usz CreateIdFromComponents()
    {
        return Hash(GetComponentId<C>()...);
    }
    static usz CreateIdFromComponent(const ComponentId id)
    {
        return Hash(id);
    }

  private:
    template <typename C, typename... Args> C *emplaceAtColumn(const usize idx, Args &&...args)
    {
        TKIT_ASSERT(m_Columns[idx].GetRowCount() == GetRowCount() - 1,
                    "[TOOLKIT][ECS] The row count of all the columns must match the archetype's row count. Column row "
                    "count is {} when it should be {}",
                    m_Columns[idx].GetRowCount(), GetRowCount() - 1);
        return m_Columns[idx].Append<C>(std::forward<Args>(args)...);
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

template <usize N>
    requires(N != 0)
using ArchetypeQueryColumns = FixedArray<ComponentColumn *, N>;

template <usize N>
    requires(N != 0)
constexpr usize GetRowCount(const ArchetypeQueryColumns<N> &info)
{
#ifdef TKIT_ENABLE_ENSURE
    for (usize i = 1; i < N; ++i)
    {
        TKIT_ENSURE(info[0]->GetRowCount() == info[i]->GetRowCount(),
                    "[TOOLKIT][ECS] All archetype columns must have matching row counts, but found column 0 has {} "
                    "rows while column {} has {}",
                    info[0]->GetRowCount(), i, info[i]->GetRowCount());
    }
#endif
    return info[0]->GetRowCount();
}

template <typename... Cs>
    requires(!HasDuplicateTypes<Cs...>() && sizeof...(Cs) != 0)
class ComponentQuery
{
    static constexpr usize Count = sizeof...(Cs);

  public:
    struct ArchetypeQueryInterface;

    class RowView
    {
        static constexpr usize Count = sizeof...(Cs);

      public:
        RowView(const ArchetypeQueryInterface *archInfo, const usize row) : m_ArchInfo(archInfo), m_Row(row)
        {
        }

        template <typename C>
            requires(IsTypeContained<C, Cs...>())
        C &GetComponent() const;

        Entity GetEntity() const;

        const RowView &operator*() const
        {
            return *this;
        }
        RowView &operator*()
        {
            return *this;
        }

        RowView &operator++()
        {
            ++m_Row;
            return *this;
        }
        RowView operator++(int)
        {
            const RowView cpy = *this;
            ++(*this);
            return cpy;
        }

        friend bool operator==(const RowView &lhs, const RowView &rhs)
        {
            return lhs.m_Row == rhs.m_Row && lhs.m_ArchInfo == rhs.m_ArchInfo;
        }
        friend bool operator!=(const RowView &lhs, const RowView &rhs)
        {
            return lhs.m_Row != rhs.m_Row || lhs.m_ArchInfo != rhs.m_ArchInfo;
        }

      private:
        const ArchetypeQueryInterface *m_ArchInfo;
        usize m_Row;
    };

    struct ArchetypeQueryInterface
    {
        RowView begin() const
        {
            return RowView{this, 0};
        }
        RowView end() const
        {
            return RowView{this, Columns.GetSize()};
        }

        Archetype *Archetype;
        ArchetypeQueryColumns<Count> Columns;
    };

    class RowIterator
    {
      public:
        RowIterator(const TierArray<ArchetypeQueryInterface> *archInfo, const usize archetype)
            : m_ArchInfo(archInfo), m_Archetype(archetype)
        {
        }

        RowView operator*() const
        {
            return RowView{&m_ArchInfo->At(m_Archetype), m_Row};
        }

        RowIterator &operator++()
        {
            const ArchetypeQueryColumns<Count> &columns = m_ArchInfo->At(m_Archetype).Columns;
            const usize rcount = GetRowCount(columns);
            if (++m_Row < rcount)
                return *this;

            m_Row = 0;
            if (++m_Archetype < m_ArchInfo->GetSize())
                return *this;

            m_Archetype = TKIT_USIZE_MAX;
            return *this;
        }

        RowIterator operator++(int)
        {
            const RowIterator cpy = *this;
            ++(*this);
            return cpy;
        }

        friend bool operator==(const RowIterator &lhs, const RowIterator &rhs)
        {
            return lhs.m_Archetype == rhs.m_Archetype && lhs.m_Row == rhs.m_Row && lhs.m_ArchInfo == rhs.m_ArchInfo;
        }
        friend bool operator!=(const RowIterator &lhs, const RowIterator &rhs)
        {
            return lhs.m_Archetype != rhs.m_Archetype || lhs.m_Row != rhs.m_Row || lhs.m_ArchInfo != rhs.m_ArchInfo;
        }

      private:
        const TierArray<ArchetypeQueryInterface> *m_ArchInfo;
        usize m_Archetype;
        usize m_Row = 0;
    };

    template <typename F> void Each(F &&func) const
    {
        for (usize i = 0; i < m_ArchetypeInfo.GetSize(); ++i)
            eachForArchetype(std::forward<F>(func), i, std::make_integer_sequence<usize, Count>{});
    }

    RowIterator begin() const
    {
        return RowIterator{&m_ArchetypeInfo, 0};
    }
    RowIterator end() const
    {
        return RowIterator{&m_ArchetypeInfo, TKIT_USIZE_MAX};
    }

    const TierArray<ArchetypeQueryInterface> &Archetypes() const
    {
        return m_ArchetypeInfo;
    }

  private:
    template <typename F, usize... I>
    void eachForArchetype(F &&func, const usize idx, const std::integer_sequence<usize, I...>) const
    {
        const ArchetypeQueryInterface &interface = m_ArchetypeInfo[idx];
        const ArchetypeQueryColumns<Count> &columns = interface.Columns;
        const usize rcount = GetRowCount(columns);
        for (usize r = 0; r < rcount; ++r)
            if constexpr (std::is_invocable_v<F, Entity, Cs &...>)
                std::forward<F>(func)(interface.Archetype->GetEntity(r), *columns[I]->template Get<Cs>(r)...);
            else
                std::forward<F>(func)(*columns[I]->template Get<Cs>(r)...);
    }

    void addArchetype(Archetype *arch)
    {
        ArchetypeQueryInterface &interface = m_ArchetypeInfo.Append();
        interface.Archetype = arch;
        interface.Columns = ArchetypeQueryColumns<Count>{&arch->GetColumn(GetComponentId<Cs>())...};
    }

    TierArray<ArchetypeQueryInterface> m_ArchetypeInfo{};

    FixedArray<ComponentId, Count> m_ComponentIds{};
    usize m_CheckedArchetypes;
    friend class Registry;
};

template <typename... Cs>
    requires(!HasDuplicateTypes<Cs...>() && sizeof...(Cs) != 0)
template <typename C>
    requires(IsTypeContained<C, Cs...>())
C &ComponentQuery<Cs...>::RowView::GetComponent() const
{
    constexpr usize idx = GetTypeIndex<C, Cs...>();
    return *m_ArchInfo->Columns.At(idx)->template Get<C>(m_Row);
}

template <typename... Cs>
    requires(!HasDuplicateTypes<Cs...>() && sizeof...(Cs) != 0)
Entity ComponentQuery<Cs...>::RowView::GetEntity() const
{
    return m_ArchInfo->Archetype->GetEntity(m_Row);
}

class Registry
{
    TKIT_NON_COPYABLE(Registry)
  public:
    Registry() = default;
    ~Registry();

    Entity CreateEntity();

    template <typename C> void RegisterComponent()
    {
        const ComponentId cid = GetComponentId<C>();
        if (cid >= m_Components.GetSize())
            m_Components.Resize(cid + 1);

        TKIT_ENSURE(!m_Components[cid].Registered, "[TOOLKIT][ECS] Cannot register an already registered component");
        m_Components[cid] = ComponentInfo::Create<C>();
    }

    template <typename... C> void RegisterComponents()
    {
        (RegisterComponent<C>(), ...);
    }

    template <typename C> C *GetComponent(const Entity e) const
    {
        TKIT_ENSURE(IsAlive(e), "[TOOLKIT][ECS] The entity with index {} is not alive and cannot be queried", e.Index);
        const EntityRecord &r = m_Entities[e.Index];
        Archetype *arch = r.Archetype;
        if (!arch)
            return nullptr;

        const ComponentId cid = GetComponentId<C>();

        ComponentColumn *column = arch->QueryColumn(cid);
        if (!column)
            return nullptr;

        TKIT_ASSERT(r.Row < arch->GetRowCount(),
                    "[TOOLKIT][ECS] Found entity having a row ({}) greater or equal to its archetype's row count ({})",
                    r.Row, arch->GetRowCount());

        return column->Get<C>(r.Row);
    }
    template <typename C> bool HasComponent(const Entity e) const
    {
        return GetComponent<C>(e) != nullptr;
    }

    template <typename C, typename... Args> C *AddComponent(const Entity e, Args &&...args)
    {
        const ComponentId cid = GetComponentId<C>();

        TKIT_ENSURE(IsAlive(e), "[TOOLKIT][ECS] The entity with index {} is not alive and cannot be queried", e.Index);
        TKIT_ASSERT(!GetComponent<C>(e), "[TOOLKIT][ECS] The entity with index {} already has a component with id {}",
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
                arch = createArchetype(archId);
                arch->AddColumn(m_Components[cid]);
                arch->FinalizeColumns(archId);
            }
            else
                arch = it->Value;

            r.Archetype = arch;
            r.Row = arch->GetRowCount();
            return arch->AddRow<C>(e, std::forward<Args>(args)...);
        }

        Archetype *src = arch;
        Archetype *dst = src->GetArchetypeToAdd(cid);

        if (!dst)
        {
            const usz archId = src->ComputeArchetypeIdToAdd(cid);
            const auto it = m_ArchetypeById.Find(archId);
            if (it == m_ArchetypeById.end())
            {
                dst = createArchetype(archId);

                for (const ComponentId c : src->GetComponentIds())
                    dst->AddColumn(m_Components[c]);
                dst->AddColumn(m_Components[cid]);

                dst->FinalizeColumns(archId);
            }
            else
                dst = it->Value;
        }

        const usize nrow = dst->GetRowCount();
        const Archetype::TransferResult<C> result = dst->AddRowWithTransfer<C>(src, r.Row, std::forward<Args>(args)...);
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

    template <typename C> void RemoveComponent(const Entity e)
    {
        TKIT_ENSURE(IsAlive(e), "[TOOLKIT][ECS] The entity with index {} is not alive and cannot be queried", e.Index);
        EntityRecord &r = m_Entities[e.Index];
        Archetype *src = r.Archetype;
        TKIT_ASSERT(src, "[TOOLKIT][ECS] To remove a component, target entity must have a valid archetype");

        const ComponentId cid = GetComponentId<C>();
        Archetype *dst = src->GetArchetypeToRemove(cid);
        if (!dst)
        {
            const usz archId = src->ComputeArchetypeIdToRemove(cid);
            const auto it = m_ArchetypeById.Find(archId);
            if (it == m_ArchetypeById.end())
            {
                dst = createArchetype(archId);

                for (const ComponentId c : src->GetComponentIds())
                    if (c != cid)
                        dst->AddColumn(m_Components[c]);
                dst->FinalizeColumns(archId);
            }
            else
                dst = it->Value;
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

    template <typename... Cs>
        requires(!HasDuplicateTypes<Cs...>() && sizeof...(Cs) != 0)
    const ComponentQuery<Cs...> &Query()
    {
        const usz id = Archetype::CreateIdFromComponents<Cs...>();
        const auto it = m_Queries.Find(id);
        if (it == m_Queries.end())
        {
            TierAllocator *tier = GetTier();
            ComponentQuery<Cs...> *q = tier->Create<ComponentQuery<Cs...>>();

            TKit::FixedArray<ComponentId, sizeof...(Cs)> cids{GetComponentId<Cs>()...};
            std::sort(cids.begin(), cids.end());

            q->m_ComponentIds = cids;
            q->m_CheckedArchetypes = m_Archetypes.GetSize();
            for (Archetype *arch : m_Archetypes)
                if (arch->HasComponents(cids))
                    q->addArchetype(arch);

            m_Queries[id] = {q, [](const void *p) { GetTier()->Destroy(scast<const ComponentQuery<Cs...> *>(p)); }};
            return *q;
        }

        ComponentQuery<Cs...> *q = scast<ComponentQuery<Cs...> *>(it->Value.Query);

        for (u32 i = q->m_CheckedArchetypes; i < m_Archetypes.GetSize(); ++i)
            if (m_Archetypes[i]->HasComponents(q->m_ComponentIds))
                q->addArchetype(m_Archetypes[i]);

        q->m_CheckedArchetypes = m_Archetypes.GetSize();
        return *q;
    }

#ifdef TKIT_ENABLE_ENSURE
    bool IsAlive(const Entity e) const
    {
        return e != NullEntity && m_Entities[e.Index].Generation == e.Generation;
    }
#endif

  private:
    Archetype *createArchetype(usz archId);
    void destroyArchetype(const Archetype *arch);
    void removeArchetype(const Archetype *arch);

    TierArray<EntityRecord> m_Entities{};
    TierArray<Entity> m_FreeEntities{};
    TierArray<ComponentInfo> m_Components{};
    TierArray<Archetype *> m_Archetypes{};

    TierHashMap<usz, Archetype *> m_ArchetypeById{};
    struct CachedQuery
    {
        void *Query;
        void (*Destroy)(const void *);
    };
    TierHashMap<usz, CachedQuery> m_Queries{};
};

} // namespace TKit
