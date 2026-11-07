#pragma once

#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/span.hpp"
#include "tkit/utils/hash.hpp"
#include "tkit/container/arena_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/tier_array.hpp"
#include <iterator>

namespace TKit
{
#ifdef TKIT_OS_WINDOWS
constexpr char PathSeparator = '\\';
#else
constexpr char PathSeparator = '/';
#endif

constexpr bool IsPathSeparator(const char c)
{
#ifdef TKIT_OS_WINDOWS
    return c == '\\' || c == '/';
#else
    return c == '/';
#endif
}
} // namespace TKit

namespace TKit::Detail
{
// All of these work on a raw (not necessarily null terminated) path and return positions/sizes within it. They follow
// the std::filesystem decomposition rules: root-name, root-directory, relative-path (filenames separated by
// directory separators).

constexpr usize GetRootNameSize([[maybe_unused]] const TKit::StringView path)
{
#ifdef TKIT_OS_WINDOWS
    // drive letter: "C:"
    if (path.GetSize() >= 2 && path[1] == ':' &&
        ((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')))
        return 2;
    // network name: "\\server"
    if (path.GetSize() > 2 && IsPathSeparator(path[0]) && IsPathSeparator(path[1]) && !IsPathSeparator(path[2]))
    {
        usize i = 3;
        while (i < path.GetSize() && !IsPathSeparator(path[i]))
            ++i;
        return i;
    }
#endif
    return 0;
}

constexpr bool HasRootDirectory(const TKit::StringView path)
{
    const usize rn = GetRootNameSize(path);
    return rn < path.GetSize() && IsPathSeparator(path[rn]);
}

constexpr bool IsAbsolutePath(const TKit::StringView path)
{
#ifdef TKIT_OS_WINDOWS
    return GetRootNameSize(path) != 0 && HasRootDirectory(path);
#else
    return !path.IsEmpty() && path[0] == '/';
#endif
}

constexpr usize GetRelativePathStart(const TKit::StringView path)
{
    usize i = GetRootNameSize(path);
    while (i < path.GetSize() && IsPathSeparator(path[i]))
        ++i;
    return i;
}

// Returns size if the path has no filename
constexpr usize GetFilenameStart(const TKit::StringView path)
{
    const usize rel = GetRelativePathStart(path);
    if (rel == path.GetSize())
        return path.GetSize();
    usize i = path.GetSize();
    while (i > rel && !IsPathSeparator(path[i - 1]))
        --i;
    return i;
}

constexpr usize GetParentPathEnd(const TKit::StringView path)
{
    const usize rel = GetRelativePathStart(path);
    if (rel == path.GetSize())
        return path.GetSize();
    usize end = GetFilenameStart(path);
    while (end > rel && IsPathSeparator(path[end - 1]))
        --end;
    return end;
}

// Returns size if the path has no extension
constexpr usize GetExtensionStart(const TKit::StringView path)
{
    const usize fstart = GetFilenameStart(path);
    const usize flen = path.GetSize() - fstart;
    if (flen == 0 || (flen == 1 && path[fstart] == '.') ||
        (flen == 2 && path[fstart] == '.' && path[fstart + 1] == '.'))
        return path.GetSize();

    // a leading dot does not start an extension (".bashrc")
    for (usize i = path.GetSize(); i > fstart + 1; --i)
        if (path[i - 1] == '.')
            return i - 1;
    return path.GetSize();
}

constexpr bool IsSeparatorElement(const StringView element)
{
    return element.GetSize() == 1 && IsPathSeparator(element[0]);
}

constexpr bool IsDotElement(const StringView element)
{
    return element.GetSize() == 1 && element[0] == '.';
}

constexpr bool IsDotDotElement(const StringView element)
{
    return element.GetSize() == 2 && element[0] == '.' && element[1] == '.';
}

constexpr i32 ComparePathElements(const StringView lhs, const StringView rhs)
{
    // root directories may be spelled with different separators on windows
    if (IsSeparatorElement(lhs) && IsSeparatorElement(rhs))
        return 0;
    return lhs.Compare(rhs);
}
} // namespace TKit::Detail

namespace TKit
{
/**
 * @brief A forward iterator over the elements of a path, following `std::filesystem::path::iterator` semantics: the
 * root name (if any), the root directory (if any), every filename, and an empty element if the path ends with a
 * directory separator.
 *
 * Elements are yielded as `StringView`s pointing into the iterated path, so the path must outlive the iterator.
 */
class PathIterator
{
    enum ElementKind : u8
    {
        Element_RootName,
        Element_RootDirectory,
        Element_FileOrDirectoryName,
        Element_Trailing,
        Element_End
    };

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = StringView;
    using difference_type = isz;
    using pointer = const StringView *;
    using reference = StringView;

    constexpr PathIterator() = default;

    static constexpr PathIterator Begin(const TKit::StringView path)
    {
        PathIterator it{path};

        if (path.IsEmpty())
            return it;

        const usize rn = Detail::GetRootNameSize(path);
        if (rn != 0)
        {
            it.m_Pos = 0;
            it.m_Length = rn;
            it.m_Kind = Element_RootName;
        }
        else if (IsPathSeparator(path[0]))
        {
            it.m_Pos = 0;
            it.m_Length = 1;
            it.m_Kind = Element_RootDirectory;
        }
        else
            it.setFileOrDirectoryName(0);
        return it;
    }
    static constexpr PathIterator End(const TKit::StringView path)
    {
        return PathIterator{path};
    }

    constexpr StringView operator*() const
    {
        TKIT_ASSERT(m_Kind != Element_End, "[TOOLKIT][PATH] Cannot dereference an end path iterator");
        return StringView{m_Path.GetData() + m_Pos, m_Length};
    }

    constexpr PathIterator &operator++()
    {
        TKIT_ASSERT(m_Kind != Element_End, "[TOOLKIT][PATH] Cannot increment an end path iterator");
        switch (m_Kind)
        {
        case Element_RootName: {
            const usize pos = m_Pos + m_Length;
            if (pos < m_Path.GetSize() && IsPathSeparator(m_Path[pos]))
            {
                m_Pos = pos;
                m_Length = 1;
                m_Kind = Element_RootDirectory;
            }
            else
                setFileOrDirectoryName(pos);
            break;
        }
        case Element_RootDirectory:
            setFileOrDirectoryName(skipSeparators(m_Pos));
            break;
        case Element_FileOrDirectoryName: {
            const usize pos = m_Pos + m_Length;
            if (pos == m_Path.GetSize())
                setEnd();
            else
            {
                const usize next = skipSeparators(pos);
                if (next == m_Path.GetSize())
                {
                    m_Pos = m_Path.GetSize();
                    m_Length = 0;
                    m_Kind = Element_Trailing;
                }
                else
                    setFileOrDirectoryName(next);
            }
            break;
        }
        default:
            setEnd();
            break;
        }
        return *this;
    }
    constexpr PathIterator operator++(int)
    {
        const PathIterator it = *this;
        ++(*this);
        return it;
    }

    constexpr bool operator==(const PathIterator &other) const
    {
        return m_Path == other.m_Path && m_Pos == other.m_Pos && m_Kind == other.m_Kind;
    }
    constexpr bool operator!=(const PathIterator &other) const
    {
        return !(*this == other);
    }

  private:
    constexpr PathIterator(const TKit::StringView path) : m_Path(path), m_Pos(m_Path.GetSize())
    {
    }

    constexpr usize skipSeparators(usize pos) const
    {
        while (pos < m_Path.GetSize() && IsPathSeparator(m_Path[pos]))
            ++pos;
        return pos;
    }
    constexpr void setEnd()
    {
        m_Pos = m_Path.GetSize();
        m_Length = 0;
        m_Kind = Element_End;
    }
    constexpr void setFileOrDirectoryName(const usize pos)
    {
        if (pos >= m_Path.GetSize())
        {
            setEnd();
            return;
        }
        usize end = pos;
        while (end < m_Path.GetSize() && !IsPathSeparator(m_Path[end]))
            ++end;
        m_Pos = pos;
        m_Length = end - pos;
        m_Kind = Element_FileOrDirectoryName;
    }

    TKit::StringView m_Path{};
    usize m_Pos = 0;
    usize m_Length = 0;
    ElementKind m_Kind = Element_End;
};

/**
 * @brief A filesystem path, meant as a `std::filesystem::path` equivalent built on top of toolkit strings.
 *
 * The path is stored in its native format, exactly as given. On windows, both '/' and '\\' are accepted as directory
 * separators, and root names ("C:", "\\\\server") are recognized.
 *
 * Operations that produce a new path (decomposition, lexical operations, `operator/`) allocate the new path with the
 * same allocator as this one. Operations that modify the path in place (`operator/=`, `operator+=`,
 * `ReplaceExtension()`, etc.) are subject to the same capacity rules as the underlying string: static, arena and stack
 * paths cannot grow beyond the capacity they were created with.
 *
 * @tparam AllocState The allocation state of the underlying string.
 */
template <typename AllocState> class Path
{
  public:
    using StringType = String<AllocState>;
    using AllocStateType = AllocState;
    using Iterator = PathIterator;

    static constexpr ArrayType Type = StringType::Type;
    static constexpr char Separator = PathSeparator;

    constexpr Path() = default;

    template <typename... Args>
    constexpr Path(const char *path, Args &&...args) : m_Path(path ? path : "", std::forward<Args>(args)...)
    {
    }
    template <typename... Args>
    constexpr Path(const StringView path, Args &&...args)
        : m_Path(path.GetData(), path.GetSize(), std::forward<Args>(args)...)
    {
    }
    template <typename... Args>
    constexpr Path(const std::string_view path, Args &&...args)
        : m_Path(path.data(), usize(path.size()), std::forward<Args>(args)...)
    {
    }
    template <typename... Args>
    constexpr Path(const std::string &path, Args &&...args)
        : m_Path(path.c_str(), usize(path.size()), std::forward<Args>(args)...)
    {
    }

    constexpr Path(const StringType &path) : m_Path(path)
    {
    }
    constexpr Path(StringType &&path) : m_Path(std::move(path))
    {
    }

    template <typename OtherAlloc, typename... Args>
        requires(!std::is_same_v<OtherAlloc, AllocState>)
    constexpr Path(const Path<OtherAlloc> &other, Args &&...args) : Path(other.GetView(), std::forward<Args>(args)...)
    {
    }

    constexpr Path(const Path &) = default;
    constexpr Path(Path &&) = default;

    constexpr Path &operator=(const Path &) = default;
    constexpr Path &operator=(Path &&) = default;

    constexpr Path &operator=(const StringView path)
    {
        rebuild(0, {path});
        return *this;
    }
    constexpr Path &operator=(const char *path)
    {
        return *this = StringView{path};
    }
    constexpr Path &operator=(const std::string_view path)
    {
        return *this = StringView{path};
    }
    constexpr Path &operator=(const std::string &path)
    {
        return *this = StringView{path};
    }
    template <typename OtherAlloc>
        requires(!std::is_same_v<OtherAlloc, AllocState>)
    constexpr Path &operator=(const Path<OtherAlloc> &other)
    {
        return *this = other.GetView();
    }

    // === Concatenation ===

    /**
     * @brief Append a path with a directory separator, following `std::filesystem::path::operator/=` semantics.
     *
     * If `path` is absolute (or has a different root name), it replaces this path entirely.
     */
    constexpr Path &Append(const StringView path)
    {
        const AppendPlan plan = planAppend(path);
        if (plan.Replace)
            rebuild(0, {path});
        else if (plan.Separator)
            rebuild(plan.Keep, {StringView{&PathSeparator, 1}, plan.Tail});
        else
            rebuild(plan.Keep, {plan.Tail});
        return *this;
    }

    /**
     * @brief Concatenate a string to the path without adding any directory separator, following
     * `std::filesystem::path::operator+=` semantics.
     */
    constexpr Path &Concat(const StringView str)
    {
        rebuild(GetSize(), {str});
        return *this;
    }

    constexpr Path &operator/=(const char *path)
    {
        return Append(StringView{path});
    }
    constexpr Path &operator/=(const StringView path)
    {
        return Append(path);
    }
    template <typename OtherAlloc> constexpr Path &operator/=(const Path<OtherAlloc> &path)
    {
        return Append(path.GetView());
    }

    constexpr Path &operator+=(const char *str)
    {
        return Concat(StringView{str});
    }
    constexpr Path &operator+=(const StringView str)
    {
        return Concat(str);
    }
    constexpr Path &operator+=(const char c)
    {
        return Concat(StringView{&c, 1});
    }
    template <typename OtherAlloc> constexpr Path &operator+=(const Path<OtherAlloc> &path)
    {
        return Concat(path.GetView());
    }

    /**
     * @brief Create a new path that results from appending `path` to this one with a directory separator. The new path
     * is allocated with the exact size it needs.
     */
    constexpr Path Join(const StringView path) const
    {
        const AppendPlan plan = planAppend(path);
        if (plan.Replace)
            return subPath(path.GetData(), path.GetSize());
        if (plan.Separator)
            return build(plan.Keep, {StringView{&PathSeparator, 1}, plan.Tail});
        return build(plan.Keep, {plan.Tail});
    }

    friend constexpr Path operator/(const Path &lhs, const char *rhs)
    {
        return lhs.Join(StringView{rhs});
    }
    friend constexpr Path operator/(const Path &lhs, const StringView rhs)
    {
        return lhs.Join(rhs);
    }
    /**
     * @brief The result uses the allocator of `rhs`, and is allocated once (so that it works with stack paths).
     */
    friend constexpr Path operator/(const char *lhs, const Path &rhs)
    {
        const StringView base{lhs};
        Path result{rhs.createString()};
        if constexpr (Type != Array_Static)
            result.Reserve(base.GetSize() + 1 + rhs.GetSize());

        result = base;
        result.Append(rhs.GetView());
        return result;
    }
    template <typename OtherAlloc> friend constexpr Path operator/(const Path &lhs, const Path<OtherAlloc> &rhs)
    {
        return lhs.Join(rhs.GetView());
    }

    // === Modifiers ===

    constexpr void Clear()
    {
        if (!IsEmpty())
            m_Path.Resize(0);
    }

    /**
     * @brief Convert all directory separators to the preferred one. Does nothing outside windows.
     */
    constexpr Path &MakePreferred()
    {
#ifdef TKIT_OS_WINDOWS
        m_Path.Replace('/', '\\');
#endif
        return *this;
    }

    constexpr Path &RemoveFilename()
    {
        rebuild(filenameStart(), {});
        return *this;
    }

    constexpr Path &ReplaceFilename(const StringView filename)
    {
        RemoveFilename();
        return Append(filename);
    }

    /**
     * @brief Replace the extension of the path. A dot is prepended if `extension` is not empty and does not start
     * with one. An empty `extension` removes it.
     */
    constexpr Path &ReplaceExtension(const StringView extension = {})
    {
        const usize keep = extensionStart();
        if (extension.GetSize() != 0 && extension[0] != '.')
            rebuild(keep, {StringView{".", 1}, extension});
        else
            rebuild(keep, {extension});
        return *this;
    }

    constexpr void Reserve(const usize capacity)
        requires(Type != Array_Static)
    {
        m_Path.Reserve(capacity);
    }

    // === Decomposition ===

    constexpr Path GetRootName() const
    {
        return subPath(GetData(), rootNameSize());
    }
    constexpr Path GetRootDirectory() const
    {
        const usize rn = rootNameSize();
        return HasRootDirectory() ? subPath(GetData() + rn, 1) : subPath(nullptr, 0);
    }
    constexpr Path GetRootPath() const
    {
        return subPath(GetData(), rootNameSize() + (HasRootDirectory() ? 1 : 0));
    }
    constexpr Path GetRelativePath() const
    {
        const usize start = relativePathStart();
        return subPath(GetData() + start, GetSize() - start);
    }
    constexpr Path GetParentPath() const
    {
        const usize end = parentPathEnd();
        // if only the root is left, return the root path so that "///a" yields "/"
        if (end != GetSize() && end <= relativePathStart())
            return GetRootPath();
        return subPath(GetData(), end);
    }
    constexpr Path GetFilename() const
    {
        const usize start = filenameStart();
        return subPath(GetData() + start, GetSize() - start);
    }
    constexpr Path GetStem() const
    {
        const usize start = filenameStart();
        return subPath(GetData() + start, extensionStart() - start);
    }
    constexpr Path GetExtension() const
    {
        const usize start = extensionStart();
        return subPath(GetData() + start, GetSize() - start);
    }

    // === Queries ===

    constexpr bool IsEmpty() const
    {
        return m_Path.IsEmpty();
    }
    constexpr bool HasRootName() const
    {
        return rootNameSize() != 0;
    }
    constexpr bool HasRootDirectory() const
    {
        return Detail::HasRootDirectory(GetView());
    }
    constexpr bool HasRootPath() const
    {
        return HasRootName() || HasRootDirectory();
    }
    constexpr bool HasRelativePath() const
    {
        return relativePathStart() != GetSize();
    }
    constexpr bool HasParentPath() const
    {
        return parentPathEnd() != 0;
    }
    constexpr bool HasFilename() const
    {
        return filenameStart() != GetSize();
    }
    constexpr bool HasStem() const
    {
        return extensionStart() != filenameStart();
    }
    constexpr bool HasExtension() const
    {
        return extensionStart() != GetSize();
    }
    constexpr bool IsAbsolute() const
    {
        return Detail::IsAbsolutePath(GetView());
    }
    constexpr bool IsRelative() const
    {
        return !IsAbsolute();
    }

    // === Lexical operations ===

    /**
     * @brief Get the normal form of the path, following `std::filesystem::path::lexically_normal()` semantics.
     *
     * Redundant separators, "." elements and "name/.." pairs are removed, and separators are converted to the
     * preferred one. An empty result becomes ".".
     */
    constexpr Path LexicallyNormal() const
    {
        const usize size = GetSize();
        if (size == 0)
            return subPath(nullptr, 0);

        const char *src = GetData();
        StringType str = createString();
        str.Resize(size);
        char *dst = str.GetData();
        usize w = 0;

        const usize rn = rootNameSize();
        for (usize i = 0; i < rn; ++i)
            dst[w++] = IsPathSeparator(src[i]) ? PathSeparator : src[i];

        const bool rootDir = rn < size && IsPathSeparator(src[rn]);
        if (rootDir)
            dst[w++] = PathSeparator;

        const usize base = w;
        const auto lastElementStart = [&]() {
            usize start = w;
            while (start > base && dst[start - 1] != PathSeparator)
                --start;
            return start;
        };
        const auto lastIsDotDot = [&]() {
            const usize start = lastElementStart();
            return w - start == 2 && dst[start] == '.' && dst[start + 1] == '.';
        };

        bool trailing = false;
        usize i = rn;
        while (i < size)
        {
            while (i < size && IsPathSeparator(src[i]))
                ++i;
            if (i == size)
                break;

            usize j = i;
            while (j < size && !IsPathSeparator(src[j]))
                ++j;

            const StringView element{src + i, j - i};
            trailing = false;
            if (Detail::IsDotElement(element))
                trailing = true;
            else if (Detail::IsDotDotElement(element))
            {
                if (w > base && !lastIsDotDot())
                {
                    w = lastElementStart();
                    if (w > base)
                        --w;
                    trailing = true;
                }
                else if (!rootDir)
                {
                    if (w > base)
                        dst[w++] = PathSeparator;
                    dst[w++] = '.';
                    dst[w++] = '.';
                }
            }
            else
            {
                if (w > base)
                    dst[w++] = PathSeparator;
                for (usize k = i; k < j; ++k)
                    dst[w++] = src[k];
            }
            i = j;
        }

        const usize relStart = relativePathStart();
        if (relStart < size && IsPathSeparator(src[size - 1]))
            trailing = true;

        if (w > base && trailing && !lastIsDotDot())
            dst[w++] = PathSeparator;
        if (w == 0)
            dst[w++] = '.';

        TKIT_ASSERT(w <= size, "[TOOLKIT][PATH] Normal form ({}) cannot be larger than the original path ({})", w,
                    size);
        str.Resize(w);
        return Path{std::move(str)};
    }

    /**
     * @brief Get this path relative to `base`, following `std::filesystem::path::lexically_relative()` semantics.
     *
     * No filesystem access is performed. An empty path is returned if no relative path exists.
     */
    constexpr Path LexicallyRelative(const StringView base) const
    {
        const char *data = GetData();

        const StringView rootName{data, rootNameSize()};
        const StringView bRootName{base.GetData(), Detail::GetRootNameSize(base)};
        if (rootName.Compare(bRootName) != 0 || IsAbsolute() != Detail::IsAbsolutePath(base) ||
            (!HasRootDirectory() && Detail::HasRootDirectory(base)))
            return subPath(nullptr, 0);

        PathIterator it = begin();
        const PathIterator itEnd = end();

        PathIterator bit = PathIterator::Begin(base);
        const PathIterator bitEnd = PathIterator::End(base);

        while (it != itEnd && bit != bitEnd && Detail::ComparePathElements(*it, *bit) == 0)
        {
            ++it;
            ++bit;
        }
        if (it == itEnd && bit == bitEnd)
            return subPath(".", 1);

        i32 count = 0;
        for (; bit != bitEnd; ++bit)
        {
            const StringView element = *bit;
            if (element.GetSize() == 0 || Detail::IsDotElement(element))
                continue;
            if (Detail::IsDotDotElement(element))
                --count;
            else
                ++count;
        }
        if (count < 0)
            return subPath(nullptr, 0);
        if (count == 0 && (it == itEnd || (*it).GetSize() == 0))
            return subPath(".", 1);

        // two passes: one to compute the size, the other to write
        const auto emit = [&](char *out) {
            usize w = 0;
            bool hasFilename = false;
            const auto put = [&](const StringView element) {
                if (hasFilename)
                {
                    if (out)
                        out[w] = PathSeparator;
                    ++w;
                }
                if (out)
                    std::char_traits<char>::copy(out + w, element.GetData(), element.GetSize());
                w += element.GetSize();
                hasFilename = element.GetSize() != 0;
            };
            for (i32 i = 0; i < count; ++i)
                put(StringView{"..", 2});
            for (PathIterator rit = it; rit != itEnd; ++rit)
                put(*rit);
            return w;
        };

        StringType str = createString();
        str.Resize(emit(nullptr));
        emit(str.GetData());
        return Path{std::move(str)};
    }
    template <typename OtherAlloc> constexpr Path LexicallyRelative(const Path<OtherAlloc> &base) const
    {
        return LexicallyRelative(base.GetView());
    }

    /**
     * @brief Same as `LexicallyRelative()`, but returns this path if the relative one is empty.
     */
    constexpr Path LexicallyProximate(const StringView base) const
    {
        Path relative = LexicallyRelative(base);
        if (relative.IsEmpty())
            return *this;
        return relative;
    }
    template <typename OtherAlloc> constexpr Path LexicallyProximate(const Path<OtherAlloc> &base) const
    {
        return LexicallyProximate(base.GetView());
    }

    // === Comparison ===

    /**
     * @brief Compare two paths following `std::filesystem::path::compare()` semantics: root names first, then rooted
     * paths go after unrooted ones, and then the relative paths are compared element-wise (so that "a/b" and "a//b"
     * compare equal).
     */
    constexpr i32 Compare(const StringView other) const
    {
        const char *data = GetData();
        const usize size = GetSize();

        const char *odata = other.GetData();
        const usize osize = other.GetSize();

        const StringView rootName = StringView{data, rootNameSize()};
        const StringView otherRootName = StringView{odata, Detail::GetRootNameSize(other)};

        const i32 rootNames = rootName.Compare(otherRootName);
        if (rootNames != 0)
            return rootNames;

        const bool rootDir = HasRootDirectory();
        const bool orootDir = Detail::HasRootDirectory(other);
        if (rootDir != orootDir)
            return rootDir ? 1 : -1;

        const usize rel = relativePathStart();
        const usize orel = Detail::GetRelativePathStart(other);

        PathIterator it = PathIterator::Begin({data + rel, size - rel});
        const PathIterator itEnd = PathIterator::End({data + rel, size - rel});

        PathIterator oit = PathIterator::Begin({odata + orel, osize - orel});
        const PathIterator oitEnd = PathIterator::End({odata + orel, osize - orel});

        for (; it != itEnd && oit != oitEnd; ++it, ++oit)
        {
            const i32 result = (*it).Compare(*oit);
            if (result != 0)
                return result;
        }
        if (it == itEnd && oit == oitEnd)
            return 0;
        return it == itEnd ? -1 : 1;
    }
    template <typename OtherAlloc> constexpr i32 Compare(const Path<OtherAlloc> &other) const
    {
        return Compare(other.GetView());
    }

    template <typename OtherAlloc> constexpr bool operator==(const Path<OtherAlloc> &other) const
    {
        return Compare(other) == 0;
    }
    template <typename OtherAlloc> constexpr bool operator!=(const Path<OtherAlloc> &other) const
    {
        return Compare(other) != 0;
    }
    template <typename OtherAlloc> constexpr bool operator<(const Path<OtherAlloc> &other) const
    {
        return Compare(other) < 0;
    }
    template <typename OtherAlloc> constexpr bool operator>(const Path<OtherAlloc> &other) const
    {
        return Compare(other) > 0;
    }
    template <typename OtherAlloc> constexpr bool operator<=(const Path<OtherAlloc> &other) const
    {
        return Compare(other) <= 0;
    }
    template <typename OtherAlloc> constexpr bool operator>=(const Path<OtherAlloc> &other) const
    {
        return Compare(other) >= 0;
    }

    constexpr bool operator==(const char *other) const
    {
        return Compare(StringView{other}) == 0;
    }
    constexpr bool operator!=(const char *other) const
    {
        return Compare(StringView{other}) != 0;
    }
    constexpr bool operator==(const StringView other) const
    {
        return Compare(other) == 0;
    }
    constexpr bool operator!=(const StringView other) const
    {
        return Compare(other) != 0;
    }

    // === Access ===

    constexpr const StringType &GetString() const
    {
        return m_Path;
    }
    constexpr const char *GetData() const
    {
        return m_Path.GetData();
    }
    /**
     * @brief Get a null terminated string. Never returns a null pointer, even if the path was never allocated.
     */
    constexpr const char *CString() const
    {
        return IsEmpty() ? "" : m_Path.CString();
    }
    constexpr usize GetSize() const
    {
        return m_Path.GetSize();
    }
    constexpr StringView GetView() const
    {
        return StringView{GetData(), GetSize()};
    }
    constexpr operator StringView() const
    {
        return GetView();
    }

    /**
     * @brief Get the path with '/' as the directory separator. Equivalent to the native string outside windows.
     */
    constexpr StringType GetGenericString() const
    {
        StringType str = createString();
        if (IsEmpty())
            return str;
        str.Resize(GetSize());
        std::char_traits<char>::copy(str.GetData(), GetData(), GetSize());
#ifdef TKIT_OS_WINDOWS
        str.Replace('\\', '/');
#endif
        return str;
    }

    constexpr PathIterator begin() const
    {
        return PathIterator::Begin(GetView());
    }
    constexpr PathIterator end() const
    {
        return PathIterator::End(GetView());
    }

    friend std::ostream &operator<<(std::ostream &os, const Path &path)
    {
        return os.write(path.GetData(), path.GetSize());
    }

  private:
    struct AppendPlan
    {
        usize Keep;
        StringView Tail;
        bool Replace;
        bool Separator;
    };

    constexpr usize rootNameSize() const
    {
        return Detail::GetRootNameSize(GetView());
    }
    constexpr usize relativePathStart() const
    {
        return Detail::GetRelativePathStart(GetView());
    }
    constexpr usize filenameStart() const
    {
        return Detail::GetFilenameStart(GetView());
    }
    constexpr usize parentPathEnd() const
    {
        return Detail::GetParentPathEnd(GetView());
    }
    constexpr usize extensionStart() const
    {
        return Detail::GetExtensionStart(GetView());
    }

    constexpr AppendPlan planAppend(const StringView path) const
    {
        const char *data = path.GetData();
        const usize size = path.GetSize();

        const usize rn = rootNameSize();
        const usize prn = Detail::GetRootNameSize(path);

        AppendPlan plan{};
        if (Detail::IsAbsolutePath(path) || (prn != 0 && StringView{data, prn}.Compare(StringView{GetData(), rn}) != 0))
        {
            plan.Replace = true;
            return plan;
        }

        plan.Tail = StringView{data + prn, size - prn};
        if (Detail::HasRootDirectory(path))
            plan.Keep = rootNameSize();
        else
        {
            plan.Keep = GetSize();
            plan.Separator = HasFilename();
        }
        return plan;
    }

    // Creates an empty string that shares this path's allocator
    constexpr StringType createString() const
    {
        if constexpr (Type == Array_Static || Type == Array_Dynamic)
            return StringType{};
        else
            return StringType(m_Path.GetAllocator());
    }

    constexpr Path subPath(const char *data, const usize size) const
    {
        StringType str = createString();
        if (size != 0)
        {
            str.Resize(size);
            std::char_traits<char>::copy(str.GetData(), data, size);
        }
        return Path{std::move(str)};
    }

    // Creates a new path with the first `keep` characters of this one followed by `pieces`
    constexpr Path build(const usize keep, const std::initializer_list<StringView> pieces) const
    {
        usize size = keep;
        for (const StringView &piece : pieces)
            size += piece.GetSize();

        StringType str = createString();
        if (size == 0)
            return Path{std::move(str)};

        str.Resize(size);
        char *dst = str.GetData();
        std::char_traits<char>::copy(dst, GetData(), keep);
        usize offset = keep;
        for (const StringView &piece : pieces)
        {
            std::char_traits<char>::copy(dst + offset, piece.GetData(), piece.GetSize());
            offset += piece.GetSize();
        }
        return Path{std::move(str)};
    }

    constexpr bool aliases(const StringView piece) const
    {
        if (piece.GetSize() == 0 || !GetData())
            return false;
        const char *bg = GetData();
        const char *ed = bg + m_Path.GetCapacity();
        return piece.GetData() >= bg && piece.GetData() < ed;
    }

    // Truncates the path to `keep` characters and appends `pieces`, in place
    constexpr void rebuild(const usize keep, const std::initializer_list<StringView> pieces)
    {
        TKIT_ASSERT(keep <= GetSize(), "[TOOLKIT][PATH] Cannot keep {} characters of a path of size {}", keep,
                    GetSize());
        for (const StringView &piece : pieces)
            if (aliases(piece))
            {
                // the pieces point into our own buffer, which may be overwritten or reallocated. build the result
                // separately and copy it back. copy assignment is used (instead of move) so that stack paths keep a
                // LIFO allocation order
                const Path path = build(keep, pieces);
                m_Path = path.m_Path;
                return;
            }

        usize size = keep;
        for (const StringView &piece : pieces)
            size += piece.GetSize();

        if (size == 0)
        {
            Clear();
            return;
        }

        m_Path.Resize(size);
        char *dst = m_Path.GetData();
        usize offset = keep;
        for (const StringView &piece : pieces)
        {
            std::char_traits<char>::copy(dst + offset, piece.GetData(), piece.GetSize());
            offset += piece.GetSize();
        }
    }

    StringType m_Path{};

    template <typename OtherAlloc> friend class Path;
};

using DynamicPath = Path<DynamicAllocation<char>>;

template <usize Capacity> using StaticPath = Path<StaticAllocation<char, Capacity + 1>>;
using StaticPath64 = StaticPath<64>;
using StaticPath128 = StaticPath<128>;
using StaticPath256 = StaticPath<256>;
using StaticPath512 = StaticPath<512>;
using StaticPath1024 = StaticPath<1024>;

using ArenaPath = Path<ArenaAllocation<char>>;
using StackPath = Path<StackAllocation<char>>;
using TierPath = Path<TierAllocation<char>>;
} // namespace TKit

template <typename AllocState> struct fmt::formatter<TKit::Path<AllocState>> : fmt::formatter<fmt::string_view>
{
    auto format(const TKit::Path<AllocState> &path, fmt::format_context &ctx) const
    {
        return fmt::formatter<fmt::string_view>::format(fmt::string_view(path.GetData(), path.GetSize()), ctx);
    }
};

template <typename AllocState> struct std::hash<TKit::Path<AllocState>>
{
    std::size_t operator()(const TKit::Path<AllocState> &path) const noexcept
    {
        // hashed element-wise to stay consistent with Path::operator==
        TKit::usz seed = TKIT_HASH_SEED;
        for (const TKit::StringView element : path)
        {
            if (TKit::Detail::IsSeparatorElement(element))
                TKit::HashCombine(seed, TKit::StringView{"/", 1});
            else
                TKit::HashCombine(seed, element);
        }
        return seed;
    }
};
