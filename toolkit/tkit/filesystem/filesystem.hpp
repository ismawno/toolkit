#pragma once

#include "tkit/filesystem/path.hpp"
#include "tkit/utils/result.hpp"
#include "tkit/utils/non_copyable.hpp"
#include <chrono>

// <windows.h> defines these as macros (mapping them to their A/W variants), which would silently rename the
// functions below. Same deal as min/max in limits.hpp
#ifdef TKIT_OS_WINDOWS
#    undef CreateDirectory
#    undef CreateHardLink
#    undef CopyFile
#endif

#ifndef TKIT_FILESYSTEM_DEFAULT_PATH
#    define TKIT_FILESYSTEM_DEFAULT_PATH TKit::TierPath
#endif
#ifndef TKIT_FILESYSTEM_DEFAULT_ARRAY
#    define TKIT_FILESYSTEM_DEFAULT_ARRAY TKit::TierArray
#endif

namespace TKit
{
using FileSystemPath = TKIT_FILESYSTEM_DEFAULT_PATH;
template <typename T> using FileSystemArray = TKIT_FILESYSTEM_DEFAULT_ARRAY<T>;

enum FileSystemError : u8
{
    FileSystemError_Unknown,
    FileSystemError_NotFound,
    FileSystemError_AlreadyExists,
    FileSystemError_PermissionDenied,
    FileSystemError_NotADirectory,
    FileSystemError_IsADirectory,
    FileSystemError_DirectoryNotEmpty,
    FileSystemError_InvalidArgument,
    FileSystemError_NameTooLong,
    FileSystemError_TooManySymlinks,
    FileSystemError_ReadOnly,
    FileSystemError_NoSpace,
    FileSystemError_CrossDevice,
    FileSystemError_Busy,
    FileSystemError_NotSupported,
    FileSystemError_IO,
    FileSystemError_TooLarge
};

const char *ToString(FileSystemError error);

template <typename T = void> using FileSystemResult = Result<T, FileSystemError>;

enum FileType : u8
{
    FileType_None,
    FileType_NotFound,
    FileType_Regular,
    FileType_Directory,
    FileType_Symlink,
    FileType_Block,
    FileType_Character,
    FileType_Fifo,
    FileType_Socket,
    FileType_Unknown
};

using FilePermissions = u16;
enum FilePermissionBit : FilePermissions
{
    FilePermission_None = 0,
    FilePermission_OwnerRead = 0400,
    FilePermission_OwnerWrite = 0200,
    FilePermission_OwnerExecute = 0100,
    FilePermission_OwnerAll = 0700,
    FilePermission_GroupRead = 040,
    FilePermission_GroupWrite = 020,
    FilePermission_GroupExecute = 010,
    FilePermission_GroupAll = 070,
    FilePermission_OthersRead = 04,
    FilePermission_OthersWrite = 02,
    FilePermission_OthersExecute = 01,
    FilePermission_OthersAll = 07,
    FilePermission_All = 0777,
    FilePermission_SetUid = 04000,
    FilePermission_SetGid = 02000,
    FilePermission_StickyBit = 01000,
    FilePermission_Mask = 07777,
    FilePermission_Unknown = 0xFFFF
};

using PermissionOptionFlags = u8;
enum PermissionOptionFlagBit : PermissionOptionFlags
{
    PermissionOptionFlag_Replace = 1 << 0,
    PermissionOptionFlag_Add = 1 << 1,
    PermissionOptionFlag_Remove = 1 << 2,
    PermissionOptionFlag_NoFollow = 1 << 3,
};

using CopyOptionFlags = u16;
enum CopyOptionFlagBit : CopyOptionFlags
{
    CopyOptionFlag_None = 0,
    // existing files
    CopyOptionFlag_SkipExisting = 1 << 0,
    CopyOptionFlag_OverwriteExisting = 1 << 1,
    CopyOptionFlag_UpdateExisting = 1 << 2,
    // sub-directories
    CopyOptionFlag_Recursive = 1 << 3,
    // symlinks
    CopyOptionFlag_CopySymlinks = 1 << 4,
    CopyOptionFlag_SkipSymlinks = 1 << 5,
    // form of copying
    CopyOptionFlag_DirectoriesOnly = 1 << 6,
    CopyOptionFlag_CreateSymlinks = 1 << 7,
    CopyOptionFlag_CreateHardLinks = 1 << 8,
};

using DirectoryOptionFlags = u8;
enum DirectoryOptionFlagBit : DirectoryOptionFlags
{
    DirectoryOptionFlag_None = 0,
    DirectoryOptionFlag_FollowDirectorySymlink = 1 << 0,
    DirectoryOptionFlag_SkipPermissionDenied = 1 << 1,
};

struct FileStatus
{
    FileType Type = FileType_None;
    FilePermissions Permissions = FilePermission_Unknown;

    constexpr bool Exists() const
    {
        return Type != FileType_None && Type != FileType_NotFound;
    }
    constexpr bool IsRegularFile() const
    {
        return Type == FileType_Regular;
    }
    constexpr bool IsDirectory() const
    {
        return Type == FileType_Directory;
    }
    constexpr bool IsSymlink() const
    {
        return Type == FileType_Symlink;
    }
    constexpr bool IsBlockFile() const
    {
        return Type == FileType_Block;
    }
    constexpr bool IsCharacterFile() const
    {
        return Type == FileType_Character;
    }
    constexpr bool IsFifo() const
    {
        return Type == FileType_Fifo;
    }
    constexpr bool IsSocket() const
    {
        return Type == FileType_Socket;
    }
    constexpr bool IsOther() const
    {
        return Exists() && !IsRegularFile() && !IsDirectory() && !IsSymlink();
    }
};

struct SpaceInfo
{
    u64 Capacity;
    u64 Free;
    u64 Available;
};

using FileTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
} // namespace TKit

namespace TKit::Detail
{
constexpr const char *ToCString(const char *path)
{
    return path ? path : "";
}
template <typename AllocState> constexpr const char *ToCString(const Path<AllocState> &path)
{
    return path.CString();
}
template <typename AllocState> constexpr const char *ToCString(const String<AllocState> &str)
{
    return str.IsEmpty() ? "" : str.CString();
}
inline const char *ToCString(const std::string &str)
{
    return str.c_str();
}
} // namespace TKit::Detail

namespace TKit
{
/**
 * @brief Anything that can be turned into a null terminated path string: paths, toolkit strings, `std::string` and
 * C strings. `StringView` is not, as it is not guaranteed to be null terminated.
 */
template <typename T>
concept PathLike = requires(const T &path) {
    { Detail::ToCString(path) } -> std::same_as<const char *>;
};
} // namespace TKit

// OS specific implementations. These are not meant to be called directly. Use the public API below
namespace TKit::Detail
{
FileSystemResult<FileStatus> GetStatus(const char *path, bool followSymlinks);
FileSystemResult<bool> IsEmpty(const char *path);
FileSystemResult<bool> Equivalent(const char *path1, const char *path2);
FileSystemResult<u64> GetFileSize(const char *path);
FileSystemResult<u64> GetHardLinkCount(const char *path);
FileSystemResult<FileTime> GetLastWriteTime(const char *path);
FileSystemResult<> SetLastWriteTime(const char *path, FileTime time);
FileSystemResult<> SetPermissions(const char *path, FilePermissions permissions, PermissionOptionFlags flags);

FileSystemResult<bool> CreateDirectory(const char *path);
FileSystemResult<bool> CreateDirectory(const char *path, const char *existing);
FileSystemResult<bool> CreateDirectories(const char *path);
FileSystemResult<> CreateSymlink(const char *target, const char *link);
FileSystemResult<> CreateDirectorySymlink(const char *target, const char *link);
FileSystemResult<> CreateHardLink(const char *target, const char *link);
FileSystemResult<FileSystemPath> ReadSymlink(const char *path);

FileSystemResult<bool> CopyFile(const char *from, const char *to, CopyOptionFlags flags);
FileSystemResult<> CopySymlink(const char *from, const char *to);
FileSystemResult<> Copy(const char *from, const char *to, CopyOptionFlags flags);
FileSystemResult<bool> Remove(const char *path);
FileSystemResult<u64> RemoveAll(const char *path);
FileSystemResult<> Rename(const char *from, const char *to);
FileSystemResult<> ResizeFile(const char *path, u64 size);
FileSystemResult<SpaceInfo> GetSpace(const char *path);

FileSystemResult<FileSystemPath> GetCurrentPath();
FileSystemResult<> SetCurrentPath(const char *path);
FileSystemResult<FileSystemPath> GetTempDirectoryPath();
FileSystemResult<FileSystemPath> Absolute(const char *path);
FileSystemResult<FileSystemPath> Canonical(const char *path);
FileSystemResult<FileSystemPath> WeaklyCanonical(const char *path);
FileSystemResult<FileSystemPath> Relative(const char *path, const char *base);
FileSystemResult<FileSystemPath> Proximate(const char *path, const char *base);
// The error of the last failed OS call in this thread (errno or GetLastError())
FileSystemError LastError();

/**
 * @brief A thin, non-recursive wrapper over the OS directory listing facilities. "." and ".." are skipped.
 */
class DirectoryStream
{
    TKIT_NON_COPYABLE(DirectoryStream)
  public:
    DirectoryStream() = default;
    ~DirectoryStream();

    DirectoryStream(DirectoryStream &&other);
    DirectoryStream &operator=(DirectoryStream &&other);

    FileSystemResult<> Open(const char *path);
    void Close();

    /**
     * @brief Advance to the next entry.
     *
     * @return `true` if a new entry is available, `false` if the stream is exhausted.
     */
    FileSystemResult<bool> Next();

    StringView GetName() const;

    /**
     * @brief Get the type of the current entry, without following symlinks.
     */
    FileType GetType() const;

    bool IsOpen() const
    {
        return m_Handle != nullptr;
    }

  private:
    void *m_Handle = nullptr;
};

template <typename AllocState> struct StaticCapacity;
template <usize Capacity> struct StaticCapacity<StaticAllocation<char, Capacity>>
{
    static constexpr usize Value = Capacity;
};

// Converts a result coming from the OS layer into a path of the requested allocation type, sharing the allocator of
// `like` if it is provided.
template <typename AllocState>
FileSystemResult<Path<AllocState>> ConvertPath(FileSystemResult<FileSystemPath> &&result,
                                               [[maybe_unused]] const Path<AllocState> *like = nullptr)
{
    using PathType = Path<AllocState>;
    if (!result)
        return FileSystemResult<PathType>::Error(result.GetError());

    if constexpr (std::is_same_v<PathType, FileSystemPath>)
        return std::move(result);
    else
    {
        const StringView view = result.GetValue().GetView();
        if constexpr (PathType::Type == Array_Static)
        {
            if (view.GetSize() >= StaticCapacity<AllocState>::Value)
                return FileSystemResult<PathType>::Error(FileSystemError_NameTooLong);
            return FileSystemResult<PathType>::Ok(view);
        }
        else if constexpr (PathType::Type == Array_Dynamic)
            return FileSystemResult<PathType>::Ok(view);
        else
        {
            using AllocatorPtr = decltype(PathType{}.GetString().GetAllocator());
            const AllocatorPtr allocator = like ? like->GetString().GetAllocator() : nullptr;
            return FileSystemResult<PathType>::Ok(view, allocator);
        }
    }
}
} // namespace TKit::Detail

namespace TKit
{
// === Status ===

/**
 * @brief Get the status of a file, following symlinks.
 *
 * A path that does not exist is not an error: the returned status will have a `FileType_NotFound` type.
 */
template <PathLike P> FileSystemResult<FileStatus> GetStatus(const P &path)
{
    return Detail::GetStatus(Detail::ToCString(path), true);
}
/**
 * @brief Get the status of a file, without following symlinks.
 *
 * A path that does not exist is not an error: the returned status will have a `FileType_NotFound` type.
 */
template <PathLike P> FileSystemResult<FileStatus> GetSymlinkStatus(const P &path)
{
    return Detail::GetStatus(Detail::ToCString(path), false);
}

// These return false on error. Use GetStatus() if errors must be told apart from a false result.

template <PathLike P> bool PathExists(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->Exists();
}
template <PathLike P> bool IsRegularFile(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsRegularFile();
}
template <PathLike P> bool IsDirectory(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsDirectory();
}
template <PathLike P> bool IsSymlink(const P &path)
{
    const auto result = GetSymlinkStatus(path);
    return result && result->IsSymlink();
}
template <PathLike P> bool IsBlockFile(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsBlockFile();
}
template <PathLike P> bool IsCharacterFile(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsCharacterFile();
}
template <PathLike P> bool IsFifo(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsFifo();
}
template <PathLike P> bool IsSocket(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsSocket();
}
template <PathLike P> bool IsOther(const P &path)
{
    const auto result = GetStatus(path);
    return result && result->IsOther();
}

/**
 * @brief Check if a file is empty, or if a directory has no entries.
 */
template <PathLike P> FileSystemResult<bool> IsEmpty(const P &path)
{
    return Detail::IsEmpty(Detail::ToCString(path));
}

/**
 * @brief Check if two paths resolve to the same file.
 */
template <PathLike P1, PathLike P2> FileSystemResult<bool> Equivalent(const P1 &path1, const P2 &path2)
{
    return Detail::Equivalent(Detail::ToCString(path1), Detail::ToCString(path2));
}

template <PathLike P> FileSystemResult<u64> GetFileSize(const P &path)
{
    return Detail::GetFileSize(Detail::ToCString(path));
}
template <PathLike P> FileSystemResult<u64> GetHardLinkCount(const P &path)
{
    return Detail::GetHardLinkCount(Detail::ToCString(path));
}

template <PathLike P> FileSystemResult<FileTime> GetLastWriteTime(const P &path)
{
    return Detail::GetLastWriteTime(Detail::ToCString(path));
}
template <PathLike P> FileSystemResult<> SetLastWriteTime(const P &path, const FileTime time)
{
    return Detail::SetLastWriteTime(Detail::ToCString(path), time);
}

/**
 * @brief Change the permissions of a file. On windows, only the write bits are meaningful (they toggle the read-only
 * attribute).
 *
 * @param flags One of `PermissionOptionFlag_Replace`, `PermissionOptionFlag_Add` or `PermissionOptionFlag_Remove`,
 * optionally combined with `PermissionOptionFlag_NoFollow`.
 */
template <PathLike P>
FileSystemResult<> SetPermissions(const P &path, const FilePermissions permissions,
                                  const PermissionOptionFlags flags = PermissionOptionFlag_Replace)
{
    return Detail::SetPermissions(Detail::ToCString(path), permissions, flags);
}

// === Creation ===

/**
 * @brief Create a directory. Its parent must exist.
 *
 * @return `true` if the directory was created, `false` if it already existed.
 */
template <PathLike P> FileSystemResult<bool> CreateDirectory(const P &path)
{
    return Detail::CreateDirectory(Detail::ToCString(path));
}
/**
 * @brief Create a directory, copying the attributes of `existing`.
 *
 * @return `true` if the directory was created, `false` if it already existed.
 */
template <PathLike P1, PathLike P2> FileSystemResult<bool> CreateDirectory(const P1 &path, const P2 &existing)
{
    return Detail::CreateDirectory(Detail::ToCString(path), Detail::ToCString(existing));
}
/**
 * @brief Create a directory and all of its missing parents.
 *
 * @return `true` if a directory was created, `false` if it already existed.
 */
template <PathLike P> FileSystemResult<bool> CreateDirectories(const P &path)
{
    return Detail::CreateDirectories(Detail::ToCString(path));
}

template <PathLike P1, PathLike P2> FileSystemResult<> CreateSymlink(const P1 &target, const P2 &link)
{
    return Detail::CreateSymlink(Detail::ToCString(target), Detail::ToCString(link));
}
template <PathLike P1, PathLike P2> FileSystemResult<> CreateDirectorySymlink(const P1 &target, const P2 &link)
{
    return Detail::CreateDirectorySymlink(Detail::ToCString(target), Detail::ToCString(link));
}
template <PathLike P1, PathLike P2> FileSystemResult<> CreateHardLink(const P1 &target, const P2 &link)
{
    return Detail::CreateHardLink(Detail::ToCString(target), Detail::ToCString(link));
}

// === Copy, move, remove ===

/**
 * @brief Copy a single regular file.
 *
 * @param flags At most one of `CopyOptionFlag_SkipExisting`, `CopyOptionFlag_OverwriteExisting` or
 * `CopyOptionFlag_UpdateExisting`. Without any of them, an existing destination is an error.
 * @return `true` if the file was copied, `false` if it was skipped.
 */
template <PathLike P1, PathLike P2>
FileSystemResult<bool> CopyFile(const P1 &from, const P2 &to, const CopyOptionFlags flags = CopyOptionFlag_None)
{
    return Detail::CopyFile(Detail::ToCString(from), Detail::ToCString(to), flags);
}

/**
 * @brief Copy a symlink itself (not the file it points to).
 */
template <PathLike P1, PathLike P2> FileSystemResult<> CopySymlink(const P1 &from, const P2 &to)
{
    return Detail::CopySymlink(Detail::ToCString(from), Detail::ToCString(to));
}

/**
 * @brief Copy files, directories and symlinks, following `std::filesystem::copy()` semantics.
 */
template <PathLike P1, PathLike P2>
FileSystemResult<> Copy(const P1 &from, const P2 &to, const CopyOptionFlags flags = CopyOptionFlag_None)
{
    return Detail::Copy(Detail::ToCString(from), Detail::ToCString(to), flags);
}

/**
 * @brief Remove a file or an empty directory. Symlinks are removed, not followed.
 *
 * @return `true` if the file was removed, `false` if it did not exist.
 */
template <PathLike P> FileSystemResult<bool> Remove(const P &path)
{
    return Detail::Remove(Detail::ToCString(path));
}

/**
 * @brief Remove a file or a directory with all of its contents. Symlinks are removed, not followed.
 *
 * @return The number of files and directories removed.
 */
template <PathLike P> FileSystemResult<u64> RemoveAll(const P &path)
{
    return Detail::RemoveAll(Detail::ToCString(path));
}

template <PathLike P1, PathLike P2> FileSystemResult<> Rename(const P1 &from, const P2 &to)
{
    return Detail::Rename(Detail::ToCString(from), Detail::ToCString(to));
}

template <PathLike P> FileSystemResult<> ResizeFile(const P &path, const u64 size)
{
    return Detail::ResizeFile(Detail::ToCString(path), size);
}

template <PathLike P> FileSystemResult<SpaceInfo> GetSpace(const P &path)
{
    return Detail::GetSpace(Detail::ToCString(path));
}

// === Paths ===

template <typename PathType = FileSystemPath> FileSystemResult<PathType> GetCurrentPath()
{
    return Detail::ConvertPath<typename PathType::AllocStateType>(Detail::GetCurrentPath());
}
template <PathLike P> FileSystemResult<> SetCurrentPath(const P &path)
{
    return Detail::SetCurrentPath(Detail::ToCString(path));
}

template <typename PathType = FileSystemPath> FileSystemResult<PathType> GetTempDirectoryPath()
{
    return Detail::ConvertPath<typename PathType::AllocStateType>(Detail::GetTempDirectoryPath());
}

template <typename AllocState> FileSystemResult<Path<AllocState>> ReadSymlink(const Path<AllocState> &path)
{
    return Detail::ConvertPath(Detail::ReadSymlink(path.CString()), &path);
}

/**
 * @brief Make a path absolute by prepending the current path (if needed). The file does not need to exist.
 */
template <typename AllocState> FileSystemResult<Path<AllocState>> Absolute(const Path<AllocState> &path)
{
    return Detail::ConvertPath(Detail::Absolute(path.CString()), &path);
}

/**
 * @brief Get the absolute path with no symlinks, "." or ".." elements. The file must exist.
 */
template <typename AllocState> FileSystemResult<Path<AllocState>> Canonical(const Path<AllocState> &path)
{
    return Detail::ConvertPath(Detail::Canonical(path.CString()), &path);
}

/**
 * @brief Same as `Canonical()`, but the trailing elements that do not exist are lexically normalized and appended
 * instead.
 */
template <typename AllocState> FileSystemResult<Path<AllocState>> WeaklyCanonical(const Path<AllocState> &path)
{
    return Detail::ConvertPath(Detail::WeaklyCanonical(path.CString()), &path);
}

/**
 * @brief Get `path` relative to `base`, after resolving both with `WeaklyCanonical()`. The result is empty if no
 * relative path exists.
 */
template <typename AllocState, PathLike P>
FileSystemResult<Path<AllocState>> Relative(const Path<AllocState> &path, const P &base)
{
    return Detail::ConvertPath(Detail::Relative(path.CString(), Detail::ToCString(base)), &path);
}
/**
 * @brief Get `path` relative to the current path.
 */
template <typename AllocState> FileSystemResult<Path<AllocState>> Relative(const Path<AllocState> &path)
{
    return Detail::ConvertPath(Detail::Relative(path.CString(), "."), &path);
}

/**
 * @brief Same as `Relative()`, but returns `WeaklyCanonical(path)` if no relative path exists.
 */
template <typename AllocState, PathLike P>
FileSystemResult<Path<AllocState>> Proximate(const Path<AllocState> &path, const P &base)
{
    return Detail::ConvertPath(Detail::Proximate(path.CString(), Detail::ToCString(base)), &path);
}
template <typename AllocState> FileSystemResult<Path<AllocState>> Proximate(const Path<AllocState> &path)
{
    return Detail::ConvertPath(Detail::Proximate(path.CString(), "."), &path);
}

// === Directory iteration ===

template <typename AllocState> class DirectoryIterator;
template <typename AllocState> class RecursiveDirectoryIterator;

template <typename AllocState> class DirectoryEntry
{
  public:
    constexpr const Path<AllocState> &GetPath() const
    {
        return m_Path;
    }
    constexpr operator const Path<AllocState> &() const
    {
        return m_Path;
    }

    /**
     * @brief Get the type of the entry, without following symlinks. Retrieved when listing the directory, so it does
     * not touch the filesystem.
     */
    constexpr FileType GetSymlinkType() const
    {
        return m_Type;
    }
    /**
     * @brief Get the type of the entry, following symlinks. Only touches the filesystem if the entry is a symlink.
     */
    FileType GetType() const
    {
        if (m_Type != FileType_Symlink)
            return m_Type;
        const auto result = TKit::GetStatus(m_Path);
        return result ? result->Type : FileType_Unknown;
    }

    bool Exists() const
    {
        const FileType type = GetType();
        return type != FileType_None && type != FileType_NotFound && type != FileType_Unknown;
    }
    bool IsRegularFile() const
    {
        return GetType() == FileType_Regular;
    }
    bool IsDirectory() const
    {
        return GetType() == FileType_Directory;
    }
    constexpr bool IsSymlink() const
    {
        return m_Type == FileType_Symlink;
    }
    bool IsBlockFile() const
    {
        return GetType() == FileType_Block;
    }
    bool IsCharacterFile() const
    {
        return GetType() == FileType_Character;
    }
    bool IsFifo() const
    {
        return GetType() == FileType_Fifo;
    }
    bool IsSocket() const
    {
        return GetType() == FileType_Socket;
    }

    FileSystemResult<u64> GetFileSize() const
    {
        return TKit::GetFileSize(m_Path);
    }
    FileSystemResult<FileTime> GetLastWriteTime() const
    {
        return TKit::GetLastWriteTime(m_Path);
    }

  private:
    Path<AllocState> m_Path{};
    FileType m_Type = FileType_None;

    template <typename A> friend class DirectoryIterator;
    template <typename A> friend class RecursiveDirectoryIterator;
};

struct DirectoryIteratorSentinel
{
};

namespace Detail
{
// Paths of the same type are copied so that they share their allocator. Anything else uses the default one
template <typename AllocState, PathLike P> Path<AllocState> MakeRootPath(const P &path)
{
    if constexpr (std::is_same_v<P, Path<AllocState>>)
        return path;
    else
        return Path<AllocState>{StringView{ToCString(path)}};
}

// What range-based for loops actually iterate with. Directory iterators own OS handles and are not copyable, so the
// loop works with a cursor pointing to them instead
template <typename Iterator> class DirectoryCursor
{
  public:
    constexpr DirectoryCursor(Iterator *iterator) : m_Iterator(iterator)
    {
    }

    const auto &operator*() const
    {
        return m_Iterator->GetEntry();
    }
    const auto *operator->() const
    {
        return &m_Iterator->GetEntry();
    }
    DirectoryCursor &operator++()
    {
        if (!m_Iterator->Advance())
            m_Iterator->Close();
        return *this;
    }
    bool operator==(DirectoryIteratorSentinel) const
    {
        return m_Iterator->IsEnd();
    }
    bool operator!=(DirectoryIteratorSentinel) const
    {
        return !m_Iterator->IsEnd();
    }

  private:
    Iterator *m_Iterator;
};
} // namespace Detail

/**
 * @brief Iterate over the entries of a directory, without recursing into sub-directories. "." and ".." are skipped.
 *
 * Unlike `std::filesystem::directory_iterator`, it is created through `Open()` (as errors are reported through
 * results) and it is not copyable. If `Open()` receives a `Path<AllocState>`, entries share its allocator. It can be
 * used in range-based for loops:
 *
 * @code
 * auto it = DirectoryIterator<TierAllocation<char>>::Open(path);
 * if (it)
 *     for (const auto &entry : *it)
 *         ...
 * @endcode
 *
 * Errors happening while iterating in a range-based for loop end the iteration. Use `Advance()` manually to catch
 * them.
 */
template <typename AllocState> class DirectoryIterator
{
    TKIT_NON_COPYABLE(DirectoryIterator)
  public:
    using EntryType = DirectoryEntry<AllocState>;

    DirectoryIterator() = default;
    DirectoryIterator(DirectoryIterator &&) = default;
    DirectoryIterator &operator=(DirectoryIterator &&) = default;

    template <PathLike P>
    static FileSystemResult<DirectoryIterator> Open(const P &path,
                                                    const DirectoryOptionFlags flags = DirectoryOptionFlag_None)
    {
        DirectoryIterator it{};
        it.m_Directory = Detail::MakeRootPath<AllocState>(path);
        const auto result = it.m_Stream.Open(it.m_Directory.CString());
        if (!result)
        {
            if (result.GetError() == FileSystemError_PermissionDenied &&
                (flags & DirectoryOptionFlag_SkipPermissionDenied))
                return FileSystemResult<DirectoryIterator>::Ok(std::move(it));
            return FileSystemResult<DirectoryIterator>::Error(result.GetError());
        }

        const auto advance = it.Advance();
        if (!advance)
            return FileSystemResult<DirectoryIterator>::Error(advance.GetError());
        return FileSystemResult<DirectoryIterator>::Ok(std::move(it));
    }

    /**
     * @brief Move to the next entry. Once the directory is exhausted, `IsEnd()` returns true.
     */
    FileSystemResult<> Advance()
    {
        TKIT_ASSERT(m_Stream.IsOpen(), "[TOOLKIT][FILESYSTEM] Cannot advance an end directory iterator");
        const auto next = m_Stream.Next();
        if (!next)
            return next.GetError();
        if (!next.GetValue())
        {
            Close();
            return FileSystemResult<>::Ok();
        }
        // the previous entry is released before the new one is allocated (keeps LIFO order for stack paths)
        m_Entry.m_Path = Path<AllocState>{};
        m_Entry.m_Path = m_Directory.Join(m_Stream.GetName());
        m_Entry.m_Type = m_Stream.GetType();
        return FileSystemResult<>::Ok();
    }

    void Close()
    {
        m_Stream.Close();
        m_Entry.m_Path = Path<AllocState>{};
    }

    const EntryType &GetEntry() const
    {
        TKIT_ASSERT(!IsEnd(), "[TOOLKIT][FILESYSTEM] Cannot access the entry of an end directory iterator");
        return m_Entry;
    }
    const EntryType &operator*() const
    {
        return GetEntry();
    }
    const EntryType *operator->() const
    {
        return &GetEntry();
    }

    bool IsEnd() const
    {
        return !m_Stream.IsOpen();
    }

    Detail::DirectoryCursor<DirectoryIterator> begin()
    {
        return Detail::DirectoryCursor<DirectoryIterator>{this};
    }
    DirectoryIteratorSentinel end() const
    {
        return DirectoryIteratorSentinel{};
    }

  private:
    // declared before the entry so that it is destroyed last (keeps LIFO order for stack paths)
    Path<AllocState> m_Directory{};
    EntryType m_Entry{};
    Detail::DirectoryStream m_Stream{};
};

/**
 * @brief Iterate over the entries of a directory and all of its sub-directories (pre-order). "." and ".." are skipped.
 *
 * Same usage as `DirectoryIterator`. Directory symlinks are not followed unless
 * `DirectoryOptionFlag_FollowDirectorySymlink` is provided.
 */
template <typename AllocState> class RecursiveDirectoryIterator
{
    TKIT_NON_COPYABLE(RecursiveDirectoryIterator)
  public:
    using EntryType = DirectoryEntry<AllocState>;

    RecursiveDirectoryIterator() = default;
    RecursiveDirectoryIterator(RecursiveDirectoryIterator &&) = default;
    RecursiveDirectoryIterator &operator=(RecursiveDirectoryIterator &&) = default;

    ~RecursiveDirectoryIterator()
    {
        // stack paths must be released in reverse order
        m_Entry.m_Path = Path<AllocState>{};
        while (!m_Levels.IsEmpty())
            m_Levels.Pop();
    }

    template <PathLike P>
    static FileSystemResult<RecursiveDirectoryIterator> Open(
        const P &path, const DirectoryOptionFlags flags = DirectoryOptionFlag_None)
    {
        RecursiveDirectoryIterator it{};
        it.m_Flags = flags;

        Level &level = it.m_Levels.Append();
        level.Directory = Detail::MakeRootPath<AllocState>(path);
        const auto result = level.Stream.Open(level.Directory.CString());
        if (!result)
        {
            it.m_Levels.Pop();
            if (result.GetError() == FileSystemError_PermissionDenied &&
                (flags & DirectoryOptionFlag_SkipPermissionDenied))
                return FileSystemResult<RecursiveDirectoryIterator>::Ok(std::move(it));
            return FileSystemResult<RecursiveDirectoryIterator>::Error(result.GetError());
        }

        const auto advance = it.advance(false);
        if (!advance)
            return FileSystemResult<RecursiveDirectoryIterator>::Error(advance.GetError());
        return FileSystemResult<RecursiveDirectoryIterator>::Ok(std::move(it));
    }

    /**
     * @brief Move to the next entry, recursing into the current one if it is a directory (and recursion was not
     * disabled for it).
     */
    FileSystemResult<> Advance()
    {
        TKIT_ASSERT(!IsEnd(), "[TOOLKIT][FILESYSTEM] Cannot advance an end directory iterator");
        const bool recurse = m_Pending && shouldRecurse();
        return advance(recurse);
    }

    /**
     * @brief Stop iterating the current directory and move to the next entry of its parent.
     */
    FileSystemResult<> Pop()
    {
        TKIT_ASSERT(!IsEnd(), "[TOOLKIT][FILESYSTEM] Cannot pop an end directory iterator");
        m_Entry.m_Path = Path<AllocState>{};
        m_Levels.Pop();
        if (m_Levels.IsEmpty())
            return FileSystemResult<>::Ok();
        return advance(false);
    }

    /**
     * @brief Prevent the next call to `Advance()` from recursing into the current entry.
     */
    void DisableRecursionPending()
    {
        m_Pending = false;
    }
    bool IsRecursionPending() const
    {
        return m_Pending;
    }

    /**
     * @brief Get how deep the current entry is. Entries of the root directory have depth 0.
     */
    usize GetDepth() const
    {
        return m_Levels.GetSize() - 1;
    }
    DirectoryOptionFlags GetFlags() const
    {
        return m_Flags;
    }

    void Close()
    {
        m_Entry.m_Path = Path<AllocState>{};
        while (!m_Levels.IsEmpty())
            m_Levels.Pop();
    }

    const EntryType &GetEntry() const
    {
        TKIT_ASSERT(!IsEnd(), "[TOOLKIT][FILESYSTEM] Cannot access the entry of an end directory iterator");
        return m_Entry;
    }
    const EntryType &operator*() const
    {
        return GetEntry();
    }
    const EntryType *operator->() const
    {
        return &GetEntry();
    }

    bool IsEnd() const
    {
        return m_Levels.IsEmpty();
    }

    Detail::DirectoryCursor<RecursiveDirectoryIterator> begin()
    {
        return Detail::DirectoryCursor<RecursiveDirectoryIterator>{this};
    }
    DirectoryIteratorSentinel end() const
    {
        return DirectoryIteratorSentinel{};
    }

  private:
    struct Level
    {
        Path<AllocState> Directory{};
        Detail::DirectoryStream Stream{};
    };

    bool shouldRecurse() const
    {
        const FileType type = m_Entry.GetSymlinkType();
        if (type == FileType_Directory)
            return true;
        if (type == FileType_Symlink && (m_Flags & DirectoryOptionFlag_FollowDirectorySymlink))
            return m_Entry.IsDirectory();
        return false;
    }

    FileSystemResult<> advance(const bool recurse)
    {
        if (recurse)
        {
            Level level{};
            // the entry path is moved into the new level (not copied) so that stack paths keep a LIFO order
            level.Directory = std::move(m_Entry.m_Path);
            const auto result = level.Stream.Open(level.Directory.CString());
            if (result)
                m_Levels.Append(std::move(level));
            else if (result.GetError() != FileSystemError_PermissionDenied ||
                     !(m_Flags & DirectoryOptionFlag_SkipPermissionDenied))
                return result;
        }
        else
            m_Entry.m_Path = Path<AllocState>{};

        m_Pending = true;
        while (!m_Levels.IsEmpty())
        {
            Level &level = m_Levels.GetBack();
            const auto next = level.Stream.Next();
            if (!next)
                return next.GetError();
            if (next.GetValue())
            {
                m_Entry.m_Path = level.Directory.Join(level.Stream.GetName());
                m_Entry.m_Type = level.Stream.GetType();
                return FileSystemResult<>::Ok();
            }
            m_Levels.Pop();
        }
        return FileSystemResult<>::Ok();
    }

    // the levels own their own directory paths, which are allocated before the current entry
    FileSystemArray<Level> m_Levels{};
    EntryType m_Entry{};
    DirectoryOptionFlags m_Flags = DirectoryOptionFlag_None;
    bool m_Pending = true;
};

template <typename AllocState>
DirectoryIterator<AllocState> IterateDirectories(const Path<AllocState> &path,
                                                 const DirectoryOptionFlags flags = DirectoryOptionFlag_None)
{
    return DirectoryIterator<AllocState>::Open(path, flags);
}
template <typename AllocState>
RecursiveDirectoryIterator<AllocState> IterateDirectoriesRecursive(
    const Path<AllocState> &path, const DirectoryOptionFlags flags = DirectoryOptionFlag_None)
{
    return RecursiveDirectoryIterator<AllocState>::Open(path, flags);
}

} // namespace TKit
