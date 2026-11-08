#include "tkit/core/pch.hpp"
#include "tkit/filesystem/filesystem.hpp"
#include "tkit/utils/defer.hpp"

#ifdef TKIT_OS_WINDOWS
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#    include <winioctl.h>
// see filesystem.hpp
#    undef CreateDirectory
#    undef CreateHardLink
#    undef CopyFile
#else
#    include <cerrno>
#    include <cstdlib>
#    include <dirent.h>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <sys/statvfs.h>
#    include <unistd.h>
#    ifdef TKIT_OS_LINUX
#        include <sys/sendfile.h>
#    endif
#endif

namespace TKit
{
const char *ToString(const FileSystemError error)
{
    switch (error)
    {
    case FileSystemError_Unknown:
        return "Unknown error";
    case FileSystemError_NotFound:
        return "No such file or directory";
    case FileSystemError_AlreadyExists:
        return "File already exists";
    case FileSystemError_PermissionDenied:
        return "Permission denied";
    case FileSystemError_IsNotDirectory:
        return "Not a directory";
    case FileSystemError_IsDirectory:
        return "Is a directory";
    case FileSystemError_DirectoryNotEmpty:
        return "Directory not empty";
    case FileSystemError_InvalidArgument:
        return "Invalid argument";
    case FileSystemError_NameTooLong:
        return "File name too long";
    case FileSystemError_TooManySymlinks:
        return "Too many levels of symbolic links";
    case FileSystemError_ReadOnly:
        return "Read-only file system";
    case FileSystemError_NoSpace:
        return "No space left on device";
    case FileSystemError_CrossDevice:
        return "Invalid cross-device link";
    case FileSystemError_Busy:
        return "Device or resource busy";
    case FileSystemError_NotSupported:
        return "Operation not supported";
    case FileSystemError_IO:
        return "Input/output error";
    case FileSystemError_TooLarge:
        return "File too large";
    }
    return "Unknown error";
}
} // namespace TKit

namespace TKit::Detail
{
template <typename T> TKIT_NO_DISCARD static FileSystemResult<T> error(const FileSystemError err)
{
    return FileSystemResult<T>::Error(err);
}
template <typename T> TKIT_NO_DISCARD static FileSystemResult<std::remove_cvref_t<T>> ok(T &&value)
{
    return FileSystemResult<std::remove_cvref_t<T>>::Ok(std::forward<T>(value));
}
TKIT_NO_DISCARD static FileSystemResult<> ok()
{
    return FileSystemResult<>::Ok();
}

#ifdef TKIT_OS_WINDOWS
// ============================================================================================================
// Windows
// ============================================================================================================

#    ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#        define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#    endif

// Seconds between 1601-01-01 (FILETIME epoch) and 1970-01-01 (unix epoch), in 100ns intervals
static constexpr u64 s_EpochDifference = 116444736000000000ULL;

static FileSystemError fromNative(const DWORD error)
{
    switch (error)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
    case ERROR_BAD_NETPATH:
    case ERROR_BAD_NET_NAME:
        return FileSystemError_NotFound;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        return FileSystemError_AlreadyExists;
    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
        return FileSystemError_PermissionDenied;
    case ERROR_DIRECTORY:
        return FileSystemError_IsNotDirectory;
    case ERROR_DIR_NOT_EMPTY:
        return FileSystemError_DirectoryNotEmpty;
    case ERROR_INVALID_PARAMETER:
    case ERROR_INVALID_NAME:
    case ERROR_NOT_A_REPARSE_POINT:
        return FileSystemError_InvalidArgument;
    case ERROR_FILENAME_EXCED_RANGE:
        return FileSystemError_NameTooLong;
    case ERROR_CANT_RESOLVE_FILENAME:
        return FileSystemError_TooManySymlinks;
    case ERROR_WRITE_PROTECT:
        return FileSystemError_ReadOnly;
    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL:
        return FileSystemError_NoSpace;
    case ERROR_NOT_SAME_DEVICE:
        return FileSystemError_CrossDevice;
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:
    case ERROR_BUSY:
        return FileSystemError_Busy;
    case ERROR_NOT_SUPPORTED:
    case ERROR_CALL_NOT_IMPLEMENTED:
        return FileSystemError_NotSupported;
    default:
        return FileSystemError_Unknown;
    }
}
template <typename T = void> TKIT_NO_DISCARD static FileSystemResult<T> lastError()
{
    return FileSystemResult<T>::Error(fromNative(GetLastError()));
}
FileSystemError LastError()
{
    return fromNative(::GetLastError());
}

static bool isNotFound(const DWORD error)
{
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_INVALID_DRIVE ||
           error == ERROR_BAD_NETPATH || error == ERROR_BAD_NET_NAME || error == ERROR_INVALID_NAME;
}

using WideString = FileSystemArray<wchar_t>;

// UTF-8 -> UTF-16, null terminated. Returns an empty array on failure
static WideString widen(const char *str)
{
    WideString wide{};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);
    if (size <= 0)
        return wide;
    wide.Resize(usize(size));
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, wide.GetData(), size);
    return wide;
}

// UTF-16 -> UTF-8
static FileSystemPath narrow(const wchar_t *str, const usize length)
{
    DynamicString narrow{};
    if (length == 0)
        return FileSystemPath{std::move(narrow)};
    const int size = WideCharToMultiByte(CP_UTF8, 0, str, int(length), nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return FileSystemPath{std::move(narrow)};
    narrow.Resize(usize(size));
    WideCharToMultiByte(CP_UTF8, 0, str, int(length), narrow.GetData(), size, nullptr, nullptr);
    return FileSystemPath{std::move(narrow)};
}

#    define TKIT_WIDEN(name, path)                                                                                     \
        const WideString name = widen(path);                                                                           \
        if (name.IsEmpty())                                                                                            \
            return FileSystemError_InvalidArgument;

struct HandleGuard
{
    HANDLE Handle;
    ~HandleGuard()
    {
        if (Handle != INVALID_HANDLE_VALUE)
            CloseHandle(Handle);
    }
};

static HANDLE openHandle(const wchar_t *path, const DWORD access, const bool followSymlinks)
{
    const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | (followSymlinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT);
    return CreateFileW(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                       flags, nullptr);
}

static bool isSymlinkTag(const DWORD tag)
{
    return tag == IO_REPARSE_TAG_SYMLINK || tag == IO_REPARSE_TAG_MOUNT_POINT;
}

static DWORD getReparseTag(const wchar_t *path)
{
    WIN32_FIND_DATAW data;
    const HANDLE handle = FindFirstFileW(path, &data);
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    FindClose(handle);
    return (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? data.dwReserved0 : 0;
}

static FilePermissions permissionsFromAttributes(const DWORD attributes)
{
    return (attributes & FILE_ATTRIBUTE_READONLY) ? FilePermissions(0555) : FilePermissions(0777);
}

static FileTime fromFileTime(const FILETIME &time)
{
    const u64 ticks = (u64(time.dwHighDateTime) << 32) | u64(time.dwLowDateTime);
    return FileTime{std::chrono::nanoseconds{i64(ticks - s_EpochDifference) * 100}};
}

static FILETIME toFileTime(const FileTime time)
{
    const u64 ticks = u64(time.time_since_epoch().count() / 100) + s_EpochDifference;
    FILETIME ft;
    ft.dwLowDateTime = DWORD(ticks & 0xFFFFFFFF);
    ft.dwHighDateTime = DWORD(ticks >> 32);
    return ft;
}

TKIT_NO_DISCARD static FileSystemResult<BY_HANDLE_FILE_INFORMATION> getInformation(const char *path)
{
    TKIT_WIDEN(wpath, path)
    const HandleGuard guard{openHandle(wpath.GetData(), 0, true)};
    if (guard.Handle == INVALID_HANDLE_VALUE)
        return lastError<BY_HANDLE_FILE_INFORMATION>();

    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(guard.Handle, &info))
        return lastError<BY_HANDLE_FILE_INFORMATION>();
    return FileSystemResult<BY_HANDLE_FILE_INFORMATION>::Ok(info);
}

FileSystemResult<FileStatus> GetStatus(const char *path, const bool followSymlinks)
{
    TKIT_WIDEN(wpath, path)
    DWORD attributes = GetFileAttributesW(wpath.GetData());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        const DWORD err = GetLastError();
        if (isNotFound(err))
            return ok(FileStatus{FileType_NotFound, FilePermission_Unknown});
        return error<FileStatus>(fromNative(err));
    }

    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        if (!followSymlinks)
        {
            if (isSymlinkTag(getReparseTag(wpath.GetData())))
                return ok(FileStatus{FileType_Symlink, permissionsFromAttributes(attributes)});
        }
        else
        {
            const HandleGuard guard{openHandle(wpath.GetData(), 0, true)};
            if (guard.Handle == INVALID_HANDLE_VALUE)
            {
                const DWORD err = GetLastError();
                if (isNotFound(err))
                    return ok(FileStatus{FileType_NotFound, FilePermission_Unknown});
                return error<FileStatus>(fromNative(err));
            }
            BY_HANDLE_FILE_INFORMATION info;
            if (!GetFileInformationByHandle(guard.Handle, &info))
                return lastError<FileStatus>();
            attributes = info.dwFileAttributes;
        }
    }

    const FileType type = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? FileType_Directory : FileType_Regular;
    return ok(FileStatus{type, permissionsFromAttributes(attributes)});
}

FileSystemResult<bool> Equivalent(const char *path1, const char *path2)
{
    const auto info1 = getInformation(path1);
    TKIT_RETURN_ON_ERROR(info1);
    const auto info2 = getInformation(path2);
    TKIT_RETURN_ON_ERROR(info2);
    return ok(info1->dwVolumeSerialNumber == info2->dwVolumeSerialNumber &&
              info1->nFileIndexHigh == info2->nFileIndexHigh && info1->nFileIndexLow == info2->nFileIndexLow);
}

FileSystemResult<u64> GetFileSize(const char *path)
{
    const auto info = getInformation(path);
    TKIT_RETURN_ON_ERROR(info);
    if (info->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return error<u64>(FileSystemError_IsDirectory);
    return ok((u64(info->nFileSizeHigh) << 32) | u64(info->nFileSizeLow));
}

FileSystemResult<u64> GetHardLinkCount(const char *path)
{
    const auto info = getInformation(path);
    TKIT_RETURN_ON_ERROR(info);
    return ok(u64(info->nNumberOfLinks));
}

FileSystemResult<FileTime> GetLastWriteTime(const char *path)
{
    const auto info = getInformation(path);
    TKIT_RETURN_ON_ERROR(info);
    return ok(fromFileTime(info->ftLastWriteTime));
}

FileSystemResult<> SetLastWriteTime(const char *path, const FileTime time)
{
    TKIT_WIDEN(wpath, path)
    const HandleGuard guard{openHandle(wpath.GetData(), FILE_WRITE_ATTRIBUTES, true)};
    if (guard.Handle == INVALID_HANDLE_VALUE)
        return lastError();
    const FILETIME ft = toFileTime(time);
    if (!SetFileTime(guard.Handle, nullptr, nullptr, &ft))
        return lastError();
    return ok();
}

FileSystemResult<> SetPermissions(const char *path, const FilePermissions permissions,
                                  const PermissionOptionFlags flags)
{
    TKIT_WIDEN(wpath, path)
    const DWORD attributes = GetFileAttributesW(wpath.GetData());
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return lastError();

    const FilePermissions current = permissionsFromAttributes(attributes);
    FilePermissions target = permissions;
    if (flags & PermissionOptionFlag_Add)
        target = current | permissions;
    else if (flags & PermissionOptionFlag_Remove)
        target = current & ~permissions;

    const bool readOnly =
        !(target & (FilePermission_OwnerWrite | FilePermission_GroupWrite | FilePermission_OthersWrite));
    const DWORD newAttributes =
        readOnly ? (attributes | FILE_ATTRIBUTE_READONLY) : (attributes & ~DWORD(FILE_ATTRIBUTE_READONLY));
    if (newAttributes != attributes && !SetFileAttributesW(wpath.GetData(), newAttributes))
        return lastError();
    return ok();
}

FileSystemResult<bool> CreateDirectory(const char *path)
{
    TKIT_WIDEN(wpath, path)
    if (CreateDirectoryW(wpath.GetData(), nullptr))
        return ok(true);

    const DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
    {
        const DWORD attributes = GetFileAttributesW(wpath.GetData());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
            return ok(false);
    }
    return error<bool>(fromNative(err));
}

FileSystemResult<bool> CreateDirectory(const char *path, const char *existing)
{
    TKIT_WIDEN(wpath, path)
    TKIT_WIDEN(wexisting, existing)
    if (CreateDirectoryExW(wexisting.GetData(), wpath.GetData(), nullptr))
        return ok(true);

    const DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
    {
        const DWORD attributes = GetFileAttributesW(wpath.GetData());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
            return ok(false);
    }
    return error<bool>(fromNative(err));
}

TKIT_NO_DISCARD static FileSystemResult<> createSymlink(const char *target, const char *link, const DWORD flags)
{
    TKIT_WIDEN(wtarget, target)
    TKIT_WIDEN(wlink, link)
    if (CreateSymbolicLinkW(wlink.GetData(), wtarget.GetData(), flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
        return ok();
    // older versions of windows do not know about the unprivileged flag
    if (GetLastError() == ERROR_INVALID_PARAMETER && CreateSymbolicLinkW(wlink.GetData(), wtarget.GetData(), flags))
        return ok();
    return lastError();
}

FileSystemResult<> CreateSymlink(const char *target, const char *link)
{
    return createSymlink(target, link, 0);
}
FileSystemResult<> CreateDirectorySymlink(const char *target, const char *link)
{
    return createSymlink(target, link, SYMBOLIC_LINK_FLAG_DIRECTORY);
}

FileSystemResult<> CreateHardLink(const char *target, const char *link)
{
    TKIT_WIDEN(wtarget, target)
    TKIT_WIDEN(wlink, link)
    if (!CreateHardLinkW(wlink.GetData(), wtarget.GetData(), nullptr))
        return lastError();
    return ok();
}

// REPARSE_DATA_BUFFER lives in the DDK headers, so it is replicated here
struct ReparseBuffer
{
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLink;
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPoint;
    };
};

FileSystemResult<FileSystemPath> ReadSymlink(const char *path)
{
    TKIT_WIDEN(wpath, path)
    const HandleGuard guard{openHandle(wpath.GetData(), 0, false)};
    if (guard.Handle == INVALID_HANDLE_VALUE)
        return lastError<FileSystemPath>();

    FileSystemArray<std::byte> buffer{};
    buffer.Resize(usize(MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
    DWORD bytes = 0;
    if (!DeviceIoControl(guard.Handle, FSCTL_GET_REPARSE_POINT, nullptr, 0, buffer.GetData(), buffer.GetSize(), &bytes,
                         nullptr))
        return lastError<FileSystemPath>();

    const ReparseBuffer *reparse = rcast<const ReparseBuffer *>(buffer.GetData());
    const WCHAR *names;
    USHORT printOffset, printLength, subOffset, subLength;
    if (reparse->ReparseTag == IO_REPARSE_TAG_SYMLINK)
    {
        names = reparse->SymbolicLink.PathBuffer;
        printOffset = reparse->SymbolicLink.PrintNameOffset;
        printLength = reparse->SymbolicLink.PrintNameLength;
        subOffset = reparse->SymbolicLink.SubstituteNameOffset;
        subLength = reparse->SymbolicLink.SubstituteNameLength;
    }
    else if (reparse->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
    {
        names = reparse->MountPoint.PathBuffer;
        printOffset = reparse->MountPoint.PrintNameOffset;
        printLength = reparse->MountPoint.PrintNameLength;
        subOffset = reparse->MountPoint.SubstituteNameOffset;
        subLength = reparse->MountPoint.SubstituteNameLength;
    }
    else
        return error<FileSystemPath>(FileSystemError_InvalidArgument);

    // offsets and lengths are in bytes
    if (printLength != 0)
        return ok(narrow(names + printOffset / sizeof(WCHAR), printLength / sizeof(WCHAR)));

    const WCHAR *name = names + subOffset / sizeof(WCHAR);
    usize length = subLength / sizeof(WCHAR);
    // strip the NT "\??\" prefix
    if (length >= 4 && name[0] == L'\\' && name[1] == L'?' && name[2] == L'?' && name[3] == L'\\')
    {
        name += 4;
        length -= 4;
    }
    return ok(narrow(name, length));
}

TKIT_NO_DISCARD static FileSystemResult<> copyContents(const char *from, const char *to)
{
    TKIT_WIDEN(wfrom, from)
    TKIT_WIDEN(wto, to)
    if (!CopyFileW(wfrom.GetData(), wto.GetData(), FALSE))
        return lastError();
    return ok();
}

FileSystemResult<bool> Remove(const char *path)
{
    TKIT_WIDEN(wpath, path)
    const DWORD attributes = GetFileAttributesW(wpath.GetData());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        const DWORD err = GetLastError();
        if (isNotFound(err))
            return ok(false);
        return error<bool>(fromNative(err));
    }

    if (attributes & FILE_ATTRIBUTE_READONLY)
        SetFileAttributesW(wpath.GetData(), attributes & ~DWORD(FILE_ATTRIBUTE_READONLY));

    // directory symlinks and junctions also carry the directory attribute, and are removed (not followed) by
    // RemoveDirectoryW
    const BOOL removed =
        (attributes & FILE_ATTRIBUTE_DIRECTORY) ? RemoveDirectoryW(wpath.GetData()) : DeleteFileW(wpath.GetData());
    if (!removed)
    {
        const DWORD err = GetLastError();
        if (attributes & FILE_ATTRIBUTE_READONLY)
            SetFileAttributesW(wpath.GetData(), attributes);
        return error<bool>(fromNative(err));
    }
    return ok(true);
}

FileSystemResult<> Rename(const char *from, const char *to)
{
    TKIT_WIDEN(wfrom, from)
    TKIT_WIDEN(wto, to)
    if (!MoveFileExW(wfrom.GetData(), wto.GetData(), MOVEFILE_REPLACE_EXISTING))
        return lastError();
    return ok();
}

FileSystemResult<> ResizeFile(const char *path, const u64 size)
{
    TKIT_WIDEN(wpath, path)
    const HandleGuard guard{openHandle(wpath.GetData(), GENERIC_WRITE, true)};
    if (guard.Handle == INVALID_HANDLE_VALUE)
        return lastError();

    LARGE_INTEGER offset;
    offset.QuadPart = LONGLONG(size);
    if (!SetFilePointerEx(guard.Handle, offset, nullptr, FILE_BEGIN) || !SetEndOfFile(guard.Handle))
        return lastError();
    return ok();
}

FileSystemResult<SpaceInfo> GetSpace(const char *path)
{
    TKIT_WIDEN(wpath, path)
    ULARGE_INTEGER available, capacity, free;
    if (!GetDiskFreeSpaceExW(wpath.GetData(), &available, &capacity, &free))
        return lastError<SpaceInfo>();
    return ok(SpaceInfo{u64(capacity.QuadPart), u64(free.QuadPart), u64(available.QuadPart)});
}

FileSystemResult<FileSystemPath> GetCurrentPath()
{
    const DWORD size = GetCurrentDirectoryW(0, nullptr);
    if (size == 0)
        return lastError<FileSystemPath>();

    WideString buffer{};
    buffer.Resize(usize(size));
    const DWORD length = GetCurrentDirectoryW(size, buffer.GetData());
    if (length == 0 || length >= size)
        return lastError<FileSystemPath>();
    return ok(narrow(buffer.GetData(), usize(length)));
}

FileSystemResult<> SetCurrentPath(const char *path)
{
    TKIT_WIDEN(wpath, path)
    if (!SetCurrentDirectoryW(wpath.GetData()))
        return lastError();
    return ok();
}

FileSystemResult<FileSystemPath> GetTempDirectoryPath()
{
    WideString buffer{};
    buffer.Resize(usize(MAX_PATH + 1));
    DWORD length = GetTempPathW(DWORD(buffer.GetSize()), buffer.GetData());
    if (length == 0)
        return lastError<FileSystemPath>();
    if (length > buffer.GetSize())
    {
        buffer.Resize(usize(length));
        length = GetTempPathW(DWORD(buffer.GetSize()), buffer.GetData());
        if (length == 0)
            return lastError<FileSystemPath>();
    }

    // GetTempPathW returns a trailing separator
    if (length > 1 && (buffer[length - 1] == L'\\' || buffer[length - 1] == L'/') && buffer[length - 2] != L':')
        --length;
    return ok(narrow(buffer.GetData(), usize(length)));
}

FileSystemResult<FileSystemPath> Absolute(const char *path)
{
    if (path[0] == '\0')
        return ok(FileSystemPath{});

    TKIT_WIDEN(wpath, path)
    const DWORD size = GetFullPathNameW(wpath.GetData(), 0, nullptr, nullptr);
    if (size == 0)
        return lastError<FileSystemPath>();

    WideString buffer{};
    buffer.Resize(usize(size));
    const DWORD length = GetFullPathNameW(wpath.GetData(), size, buffer.GetData(), nullptr);
    if (length == 0 || length >= size)
        return lastError<FileSystemPath>();
    return ok(narrow(buffer.GetData(), usize(length)));
}

FileSystemResult<FileSystemPath> Canonical(const char *path)
{
    TKIT_WIDEN(wpath, path)
    const HandleGuard guard{openHandle(wpath.GetData(), 0, true)};
    if (guard.Handle == INVALID_HANDLE_VALUE)
        return lastError<FileSystemPath>();

    constexpr DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
    const DWORD size = GetFinalPathNameByHandleW(guard.Handle, nullptr, 0, flags);
    if (size == 0)
        return lastError<FileSystemPath>();

    WideString buffer{};
    buffer.Resize(usize(size));
    const DWORD length = GetFinalPathNameByHandleW(guard.Handle, buffer.GetData(), size, flags);
    if (length == 0 || length >= size)
        return lastError<FileSystemPath>();

    // strip the "\\?\" (or "\\?\UNC\") prefix
    const wchar_t *name = buffer.GetData();
    usize nlength = usize(length);
    if (nlength >= 8 && wcsncmp(name, L"\\\\?\\UNC\\", 8) == 0)
    {
        // keep two leading separators: "\\?\UNC\server" -> "\\server"
        name += 6;
        nlength -= 6;
        buffer[6] = L'\\';
    }
    else if (nlength >= 4 && wcsncmp(name, L"\\\\?\\", 4) == 0)
    {
        name += 4;
        nlength -= 4;
    }
    return ok(narrow(name, nlength));
}

struct StreamHandle
{
    HANDLE Find = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW Data;
    bool First = true;
    DynamicString Name{};
    FileType Type = FileType_None;
};

DirectoryStream::~DirectoryStream()
{
    Close();
}

void DirectoryStream::Close()
{
    if (!m_Handle)
        return;
    StreamHandle *handle = scast<StreamHandle *>(m_Handle);
    if (handle->Find != INVALID_HANDLE_VALUE)
        FindClose(handle->Find);
    delete handle;
    m_Handle = nullptr;
}

FileSystemResult<> DirectoryStream::Open(const char *path)
{
    Close();

    DynamicString pattern{path};
    const usize size = pattern.GetSize();
    if (size == 0)
        return FileSystemError_NotFound;
    if (!IsPathSeparator(pattern[size - 1]) && pattern[size - 1] != ':')
        pattern += '\\';
    pattern += '*';

    TKIT_WIDEN(wpattern, pattern.CString())
    StreamHandle *handle = new StreamHandle{};
    handle->Find = FindFirstFileExW(wpattern.GetData(), FindExInfoBasic, &handle->Data, FindExSearchNameMatch, nullptr,
                                    FIND_FIRST_EX_LARGE_FETCH);
    if (handle->Find == INVALID_HANDLE_VALUE)
    {
        const DWORD err = GetLastError();
        delete handle;
        return fromNative(err);
    }
    m_Handle = handle;
    return ok();
}

FileSystemResult<bool> DirectoryStream::Next()
{
    TKIT_ASSERT(m_Handle, "[TOOLKIT][FILESYSTEM] Directory stream is not open");
    StreamHandle *handle = scast<StreamHandle *>(m_Handle);
    for (;;)
    {
        if (handle->First)
            handle->First = false;
        else if (!FindNextFileW(handle->Find, &handle->Data))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_FILES)
                return ok(false);
            return error<bool>(fromNative(err));
        }

        const wchar_t *name = handle->Data.cFileName;
        if (name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0')))
            continue;

        FileSystemPath narrowed = narrow(name, usize(wcslen(name)));
        handle->Name = narrowed.GetString();

        const DWORD attributes = handle->Data.dwFileAttributes;
        if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) && isSymlinkTag(handle->Data.dwReserved0))
            handle->Type = FileType_Symlink;
        else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
            handle->Type = FileType_Directory;
        else
            handle->Type = FileType_Regular;
        return ok(true);
    }
}

StringView DirectoryStream::GetName() const
{
    TKIT_ASSERT(m_Handle, "[TOOLKIT][FILESYSTEM] Directory stream is not open");
    const StreamHandle *handle = scast<const StreamHandle *>(m_Handle);
    return StringView{handle->Name.GetData(), handle->Name.GetSize()};
}

FileType DirectoryStream::GetType() const
{
    TKIT_ASSERT(m_Handle, "[TOOLKIT][FILESYSTEM] Directory stream is not open");
    return scast<const StreamHandle *>(m_Handle)->Type;
}

#    undef TKIT_WIDEN

#else
// ============================================================================================================
// POSIX
// ============================================================================================================

static constexpr bool isDotOrDotDot(const char *name)
{
    return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

static FileSystemError fromNative(const int err)
{
    if (err == ENOENT)
        return FileSystemError_NotFound;
    if (err == EEXIST)
        return FileSystemError_AlreadyExists;
    if (err == EACCES || err == EPERM)
        return FileSystemError_PermissionDenied;
    if (err == ENOTDIR)
        return FileSystemError_IsNotDirectory;
    if (err == EISDIR)
        return FileSystemError_IsDirectory;
    if (err == ENOTEMPTY)
        return FileSystemError_DirectoryNotEmpty;
    if (err == EINVAL)
        return FileSystemError_InvalidArgument;
    if (err == ENAMETOOLONG)
        return FileSystemError_NameTooLong;
    if (err == ELOOP)
        return FileSystemError_TooManySymlinks;
    if (err == EROFS)
        return FileSystemError_ReadOnly;
    if (err == ENOSPC || err == EDQUOT)
        return FileSystemError_NoSpace;
    if (err == EXDEV)
        return FileSystemError_CrossDevice;
    if (err == EBUSY || err == ETXTBSY)
        return FileSystemError_Busy;
    if (err == ENOTSUP || err == EOPNOTSUPP || err == ENOSYS)
        return FileSystemError_NotSupported;
    if (err == EIO)
        return FileSystemError_IO;
    return FileSystemError_Unknown;
}
template <typename T = void> TKIT_NO_DISCARD static FileSystemResult<T> lastError()
{
    return FileSystemResult<T>::Error(fromNative(errno));
}
FileSystemError LastError()
{
    return fromNative(errno);
}

static FileType toFileType(const mode_t mode)
{
    if (S_ISREG(mode))
        return FileType_Regular;
    if (S_ISDIR(mode))
        return FileType_Directory;
    if (S_ISLNK(mode))
        return FileType_Symlink;
    if (S_ISBLK(mode))
        return FileType_Block;
    if (S_ISCHR(mode))
        return FileType_Character;
    if (S_ISFIFO(mode))
        return FileType_Fifo;
    if (S_ISSOCK(mode))
        return FileType_Socket;
    return FileType_Unknown;
}

TKIT_NO_DISCARD static FileSystemResult<struct stat> getStat(const char *path, const bool followSymlinks = true)
{
    struct stat st;
    if ((followSymlinks ? ::stat(path, &st) : ::lstat(path, &st)) != 0)
        return lastError<struct stat>();
    return FileSystemResult<struct stat>::Ok(st);
}

FileSystemResult<FileStatus> GetStatus(const char *path, const bool followSymlinks)
{
    struct stat st;
    if ((followSymlinks ? ::stat(path, &st) : ::lstat(path, &st)) != 0)
    {
        if (errno == ENOENT || errno == ENOTDIR)
            return ok(FileStatus{FileType_NotFound, FilePermission_Unknown});
        return lastError<FileStatus>();
    }
    return ok(FileStatus{toFileType(st.st_mode), FilePermissions(st.st_mode & FilePermission_Mask)});
}

FileSystemResult<bool> Equivalent(const char *path1, const char *path2)
{
    const auto st1 = getStat(path1);
    TKIT_RETURN_ON_ERROR(st1);
    const auto st2 = getStat(path2);
    TKIT_RETURN_ON_ERROR(st2);
    return ok(st1->st_dev == st2->st_dev && st1->st_ino == st2->st_ino);
}

FileSystemResult<u64> GetFileSize(const char *path)
{
    const auto st = getStat(path);
    TKIT_RETURN_ON_ERROR(st);
    if (S_ISDIR(st->st_mode))
        return error<u64>(FileSystemError_IsDirectory);
    if (!S_ISREG(st->st_mode))
        return error<u64>(FileSystemError_NotSupported);
    return ok(u64(st->st_size));
}

FileSystemResult<u64> GetHardLinkCount(const char *path)
{
    const auto st = getStat(path);
    TKIT_RETURN_ON_ERROR(st);
    return ok(u64(st->st_nlink));
}

FileSystemResult<FileTime> GetLastWriteTime(const char *path)
{
    const auto st = getStat(path);
    TKIT_RETURN_ON_ERROR(st);
#    ifdef TKIT_OS_APPLE
    const timespec &time = st->st_mtimespec;
#    else
    const timespec &time = st->st_mtim;
#    endif
    return ok(FileTime{std::chrono::seconds{time.tv_sec} + std::chrono::nanoseconds{time.tv_nsec}});
}

FileSystemResult<> SetLastWriteTime(const char *path, const FileTime time)
{
    const i64 nanoseconds = time.time_since_epoch().count();
    i64 seconds = nanoseconds / 1000000000;
    i64 remainder = nanoseconds % 1000000000;
    if (remainder < 0)
    {
        --seconds;
        remainder += 1000000000;
    }

    timespec times[2];
    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = time_t(seconds);
    times[1].tv_nsec = long(remainder);
    if (::utimensat(AT_FDCWD, path, times, 0) != 0)
        return lastError();
    return ok();
}

FileSystemResult<> SetPermissions(const char *path, const FilePermissions permissions,
                                  const PermissionOptionFlags flags)
{
    TKIT_ASSERT(((flags & PermissionOptionFlag_Replace) != 0) + ((flags & PermissionOptionFlag_Add) != 0) +
                        ((flags & PermissionOptionFlag_Remove) != 0) ==
                    1,
                "[TOOLKIT][FILESYSTEM] Exactly one of Replace, Add or Remove must be specified");

    const bool follow = !(flags & PermissionOptionFlag_NoFollow);
    mode_t mode = mode_t(permissions & FilePermission_Mask);
    if (flags & (PermissionOptionFlag_Add | PermissionOptionFlag_Remove))
    {
        const auto st = getStat(path, follow);
        TKIT_RETURN_ON_ERROR(st);
        const mode_t current = st->st_mode & FilePermission_Mask;
        mode = (flags & PermissionOptionFlag_Add) ? (current | mode) : (current & ~mode);
    }

    if (::fchmodat(AT_FDCWD, path, mode, follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0)
        return lastError();
    return ok();
}

TKIT_NO_DISCARD static FileSystemResult<bool> createDirectory(const char *path, const mode_t mode)
{
    if (::mkdir(path, mode) == 0)
        return ok(true);

    const int err = errno;
    if (err == EEXIST)
    {
        struct stat st;
        if (::stat(path, &st) == 0 && S_ISDIR(st.st_mode))
            return ok(false);
    }
    return error<bool>(fromNative(err));
}

FileSystemResult<bool> CreateDirectory(const char *path)
{
    return createDirectory(path, 0777);
}

FileSystemResult<bool> CreateDirectory(const char *path, const char *existing)
{
    const auto st = getStat(existing);
    TKIT_RETURN_ON_ERROR(st);
    if (!S_ISDIR(st->st_mode))
        return error<bool>(FileSystemError_IsNotDirectory);
    return createDirectory(path, st->st_mode & FilePermission_Mask);
}

FileSystemResult<> CreateSymlink(const char *target, const char *link)
{
    if (::symlink(target, link) != 0)
        return lastError();
    return ok();
}

FileSystemResult<> CreateDirectorySymlink(const char *target, const char *link)
{
    return CreateSymlink(target, link);
}

FileSystemResult<> CreateHardLink(const char *target, const char *link)
{
    if (::link(target, link) != 0)
        return lastError();
    return ok();
}

FileSystemResult<FileSystemPath> ReadSymlink(const char *path)
{
    DynamicString buffer{};
    usize capacity = 256;
    for (;;)
    {
        buffer.Resize(capacity);
        const isz length = ::readlink(path, buffer.GetData(), capacity);
        if (length < 0)
            return lastError<FileSystemPath>();
        if (usize(length) < capacity)
        {
            buffer.Resize(usize(length));
            return ok(FileSystemPath{std::move(buffer)});
        }
        capacity *= 2;
    }
}

TKIT_NO_DISCARD static FileSystemResult<> copyContents(const char *from, const char *to)
{
    const int in = ::open(from, O_RDONLY | O_CLOEXEC);
    if (in < 0)
        return lastError();

    TKIT_DEFER(::close(in));

    struct stat st;
    if (::fstat(in, &st) != 0)
    {
        const int err = errno;
        return fromNative(err);
    }

    const mode_t mode = st.st_mode & FilePermission_Mask;
    const int out = ::open(to, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode);
    if (out < 0)
    {
        const int err = errno;
        return fromNative(err);
    }

    int err = 0;
    bool fallback = true;
#    ifdef TKIT_OS_LINUX
    fallback = false;
    off_t remaining = st.st_size;
    while (remaining > 0)
    {
        const ssize_t sent = ::sendfile(out, in, nullptr, usz(remaining));
        if (sent < 0)
        {
            if (errno == EINTR)
                continue;
            // the file system does not support sendfile(). nothing has been sent at this point
            if (errno == EINVAL || errno == ENOSYS)
                fallback = true;
            else
                err = errno;
            break;
        }
        if (sent == 0)
            break;
        remaining -= sent;
    }
#    endif

    if (fallback && err == 0)
    {
        char buffer[1 << 15];
        for (;;)
        {
            const ssize_t bytes = ::read(in, buffer, sizeof(buffer));
            if (bytes < 0)
            {
                if (errno == EINTR)
                    continue;
                err = errno;
                break;
            }
            if (bytes == 0)
                break;

            ssize_t written = 0;
            while (written < bytes)
            {
                const ssize_t w = ::write(out, buffer + written, usz(bytes - written));
                if (w < 0)
                {
                    if (errno == EINTR)
                        continue;
                    err = errno;
                    break;
                }
                written += w;
            }
            if (err != 0)
                break;
        }
    }

    // the destination may have existed with other permissions
    if (err == 0 && ::fchmod(out, mode) != 0)
        err = errno;

    if (::close(out) != 0 && err == 0)
        err = errno;
    if (err != 0)
        return fromNative(err);
    return ok();
}

FileSystemResult<bool> Remove(const char *path)
{
    if (::remove(path) == 0)
        return ok(true);
    if (errno == ENOENT)
        return ok(false);
    return lastError<bool>();
}

FileSystemResult<> Rename(const char *from, const char *to)
{
    if (::rename(from, to) != 0)
        return lastError();
    return ok();
}

FileSystemResult<> ResizeFile(const char *path, const u64 size)
{
    if (::truncate(path, off_t(size)) != 0)
        return lastError();
    return ok();
}

FileSystemResult<SpaceInfo> GetSpace(const char *path)
{
    struct statvfs st;
    if (::statvfs(path, &st) != 0)
        return lastError<SpaceInfo>();
    const u64 fragment = u64(st.f_frsize);
    return ok(SpaceInfo{u64(st.f_blocks) * fragment, u64(st.f_bfree) * fragment, u64(st.f_bavail) * fragment});
}

FileSystemResult<FileSystemPath> GetCurrentPath()
{
    DynamicString buffer{};
    usize capacity = 256;
    for (;;)
    {
        // the string always holds one extra character for the null terminator
        buffer.Resize(capacity);
        if (::getcwd(buffer.GetData(), capacity + 1))
        {
            buffer.Resize(usize(std::char_traits<char>::length(buffer.GetData())));
            return ok(FileSystemPath{std::move(buffer)});
        }
        if (errno != ERANGE)
            return lastError<FileSystemPath>();
        capacity *= 2;
    }
}

FileSystemResult<> SetCurrentPath(const char *path)
{
    if (::chdir(path) != 0)
        return lastError();
    return ok();
}

FileSystemResult<FileSystemPath> GetTempDirectoryPath()
{
    const char *path = nullptr;
    for (const char *var : {"TMPDIR", "TMP", "TEMP", "TEMPDIR"})
    {
        path = std::getenv(var);
        if (path && path[0] != '\0')
            break;
        path = nullptr;
    }
    if (!path)
        path = "/tmp";

    const auto st = getStat(path);
    TKIT_RETURN_ON_ERROR(st);
    if (!S_ISDIR(st->st_mode))
        return error<FileSystemPath>(FileSystemError_IsNotDirectory);
    return ok(FileSystemPath{path});
}

FileSystemResult<FileSystemPath> Absolute(const char *path)
{
    if (path[0] == '\0')
        return ok(FileSystemPath{});
    if (path[0] == '/')
        return ok(FileSystemPath{path});

    auto current = GetCurrentPath();
    TKIT_RETURN_ON_ERROR(current);
    *current /= path;
    return current;
}

FileSystemResult<FileSystemPath> Canonical(const char *path)
{
    char *resolved = ::realpath(path, nullptr);
    if (!resolved)
        return lastError<FileSystemPath>();
    FileSystemPath result{resolved};
    std::free(resolved);
    return ok(std::move(result));
}

struct StreamHandle
{
    DIR *Directory = nullptr;
    const char *Name = nullptr;
    usize NameSize = 0;
    FileType Type = FileType_None;
};

DirectoryStream::~DirectoryStream()
{
    Close();
}

void DirectoryStream::Close()
{
    if (!m_Handle)
        return;
    StreamHandle *handle = scast<StreamHandle *>(m_Handle);
    ::closedir(handle->Directory);
    delete handle;
    m_Handle = nullptr;
}

FileSystemResult<> DirectoryStream::Open(const char *path)
{
    Close();
    DIR *directory = ::opendir(path);
    if (!directory)
        return lastError();

    StreamHandle *handle = new StreamHandle{};
    handle->Directory = directory;
    m_Handle = handle;
    return ok();
}

FileSystemResult<bool> DirectoryStream::Next()
{
    TKIT_ASSERT(m_Handle, "[TOOLKIT][FILESYSTEM] Directory stream is not open");
    StreamHandle *handle = scast<StreamHandle *>(m_Handle);
    for (;;)
    {
        errno = 0;
        const dirent *entry = ::readdir(handle->Directory);
        if (!entry)
        {
            if (errno != 0)
                return lastError<bool>();
            return ok(false);
        }
        if (isDotOrDotDot(entry->d_name))
            continue;

        handle->Name = entry->d_name;
        handle->NameSize = usize(std::char_traits<char>::length(entry->d_name));

        handle->Type = FileType_Unknown;
#    if defined(_DIRENT_HAVE_D_TYPE) || defined(TKIT_OS_APPLE)
        switch (entry->d_type)
        {
        case DT_REG:
            handle->Type = FileType_Regular;
            break;
        case DT_DIR:
            handle->Type = FileType_Directory;
            break;
        case DT_LNK:
            handle->Type = FileType_Symlink;
            break;
        case DT_BLK:
            handle->Type = FileType_Block;
            break;
        case DT_CHR:
            handle->Type = FileType_Character;
            break;
        case DT_FIFO:
            handle->Type = FileType_Fifo;
            break;
        case DT_SOCK:
            handle->Type = FileType_Socket;
            break;
        default:
            break;
        }
#    endif
        // some file systems do not report the type
        if (handle->Type == FileType_Unknown)
        {
            struct stat st;
            if (::fstatat(::dirfd(handle->Directory), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0)
                handle->Type = toFileType(st.st_mode);
        }
        return ok(true);
    }
}

StringView DirectoryStream::GetName() const
{
    TKIT_ASSERT(m_Handle, "[TOOLKIT][FILESYSTEM] Directory stream is not open");
    const StreamHandle *handle = scast<const StreamHandle *>(m_Handle);
    return StringView{handle->Name, handle->NameSize};
}

FileType DirectoryStream::GetType() const
{
    TKIT_ASSERT(m_Handle, "[TOOLKIT][FILESYSTEM] Directory stream is not open");
    return scast<const StreamHandle *>(m_Handle)->Type;
}
#endif

// ============================================================================================================
// OS agnostic
// ============================================================================================================

DirectoryStream::DirectoryStream(DirectoryStream &&other) : m_Handle(other.m_Handle)
{
    other.m_Handle = nullptr;
}

DirectoryStream &DirectoryStream::operator=(DirectoryStream &&other)
{
    if (this != &other)
    {
        Close();
        m_Handle = other.m_Handle;
        other.m_Handle = nullptr;
    }
    return *this;
}

FileSystemResult<bool> IsEmpty(const char *path)
{
    const auto status = GetStatus(path, true);
    TKIT_RETURN_ON_ERROR(status);
    if (!status->Exists())
        return error<bool>(FileSystemError_NotFound);

    if (status->IsDirectory())
    {
        DirectoryStream stream{};
        const auto result = stream.Open(path);
        TKIT_RETURN_ON_ERROR(result);
        const auto next = stream.Next();
        TKIT_RETURN_ON_ERROR(next);
        return ok(!next.GetValue());
    }

    const auto size = GetFileSize(path);
    TKIT_RETURN_ON_ERROR(size);
    return ok(size.GetValue() == 0);
}

FileSystemResult<bool> CreateDirectories(const char *path)
{
    DynamicString buffer{path};
    const usize size = buffer.GetSize();
    if (size == 0)
        return error<bool>(FileSystemError_InvalidArgument);

    const auto status = GetStatus(path, true);
    TKIT_RETURN_ON_ERROR(status);
    if (status->IsDirectory())
        return ok(false);
    if (status->Exists())
        return error<bool>(FileSystemError_AlreadyExists);

    // create every prefix ending at an element boundary, temporarily null-terminating the buffer there
    char *data = buffer.GetData();
    bool created = false;
    for (usize i = Detail::GetRelativePathStart({data, size}); i <= size; ++i)
    {
        if (i != size && !IsPathSeparator(data[i]))
            continue;
        if (i == 0 || IsPathSeparator(data[i - 1]))
            continue;

        const char saved = data[i];
        data[i] = '\0';
        const auto result = CreateDirectory(data);
        data[i] = saved;
        TKIT_RETURN_ON_ERROR(result);
        created |= result.GetValue();
    }
    return ok(created);
}

FileSystemResult<bool> CopyFile(const char *from, const char *to, const CopyOptionFlags flags)
{
    const auto fromStatus = GetStatus(from, true);
    TKIT_RETURN_ON_ERROR(fromStatus);
    if (!fromStatus->Exists())
        return error<bool>(FileSystemError_NotFound);
    if (!fromStatus->IsRegularFile())
        return error<bool>(fromStatus->IsDirectory() ? FileSystemError_IsDirectory : FileSystemError_NotSupported);

    const auto toStatus = GetStatus(to, true);
    TKIT_RETURN_ON_ERROR(toStatus);
    if (toStatus->Exists())
    {
        if (!toStatus->IsRegularFile())
            return error<bool>(toStatus->IsDirectory() ? FileSystemError_IsDirectory : FileSystemError_NotSupported);

        const auto equivalent = Equivalent(from, to);
        TKIT_RETURN_ON_ERROR(equivalent);
        if (equivalent.GetValue())
            return error<bool>(FileSystemError_AlreadyExists);

        if (flags & CopyOptionFlag_SkipExisting)
            return ok(false);
        if (flags & CopyOptionFlag_UpdateExisting)
        {
            const auto fromTime = GetLastWriteTime(from);
            TKIT_RETURN_ON_ERROR(fromTime);
            const auto toTime = GetLastWriteTime(to);
            TKIT_RETURN_ON_ERROR(toTime);
            if (fromTime.GetValue() <= toTime.GetValue())
                return ok(false);
        }
        else if (!(flags & CopyOptionFlag_OverwriteExisting))
            return error<bool>(FileSystemError_AlreadyExists);
    }

    const auto result = copyContents(from, to);
    TKIT_RETURN_ON_ERROR(result);
    return ok(true);
}

FileSystemResult<> CopySymlink(const char *from, const char *to)
{
    const auto target = ReadSymlink(from);
    TKIT_RETURN_ON_ERROR(target);

    const auto status = GetStatus(from, true);
    TKIT_RETURN_ON_ERROR(status);
    if (status->IsDirectory())
        return CreateDirectorySymlink(target->CString(), to);
    return CreateSymlink(target->CString(), to);
}

// internal flag to mark that Copy() is being called from within a recursive copy
static constexpr CopyOptionFlags s_InRecursiveCopy = 1 << 15;

FileSystemResult<> Copy(const char *from, const char *to, const CopyOptionFlags flags)
{
    const bool symlinkStatus =
        flags & (CopyOptionFlag_CopySymlinks | CopyOptionFlag_SkipSymlinks | CopyOptionFlag_CreateSymlinks);
    const auto fromStatus = GetStatus(from, !symlinkStatus);
    TKIT_RETURN_ON_ERROR(fromStatus);
    if (!fromStatus->Exists())
        return FileSystemError_NotFound;

    const bool toSymlinkStatus = flags & (CopyOptionFlag_SkipSymlinks | CopyOptionFlag_CreateSymlinks);
    const auto toStatus = GetStatus(to, !toSymlinkStatus);
    TKIT_RETURN_ON_ERROR(toStatus);

    if (toStatus->Exists())
    {
        const auto equivalent = Equivalent(from, to);
        TKIT_RETURN_ON_ERROR(equivalent);
        if (equivalent.GetValue())
            return FileSystemError_AlreadyExists;
    }
    if (fromStatus->IsOther() || toStatus->IsOther())
        return FileSystemError_NotSupported;
    if (fromStatus->IsDirectory() && toStatus->IsRegularFile())
        return FileSystemError_IsDirectory;

    if (fromStatus->IsSymlink())
    {
        if (flags & CopyOptionFlag_SkipSymlinks)
            return ok();
        if (!toStatus->Exists() && (flags & CopyOptionFlag_CopySymlinks))
            return CopySymlink(from, to);
        return FileSystemError_InvalidArgument;
    }

    if (fromStatus->IsRegularFile())
    {
        if (flags & CopyOptionFlag_DirectoriesOnly)
            return ok();
        if (flags & CopyOptionFlag_CreateSymlinks)
            return CreateSymlink(from, to);
        if (flags & CopyOptionFlag_CreateHardLinks)
            return CreateHardLink(from, to);

        FileSystemResult<bool> result = FileSystemError_Unknown;
        if (toStatus->IsDirectory())
        {
            const FileSystemPath source{from};
            const FileSystemPath destination = FileSystemPath{to} / source.GetFilename();
            result = CopyFile(from, destination.CString(), flags);
        }
        else
            result = CopyFile(from, to, flags);

        TKIT_RETURN_ON_ERROR(result);
        return ok();
    }

    if (fromStatus->IsDirectory())
    {
        if (flags & CopyOptionFlag_CreateSymlinks)
            return FileSystemError_IsDirectory;

        // with no flags at all, only the top level directory is copied (its sub-directories are visited with the
        // internal recursive flag, so they will not match this condition)
        const bool recurse = (flags & CopyOptionFlag_Recursive) || flags == CopyOptionFlag_None;
        if (!recurse)
            return ok();

        if (!toStatus->Exists())
        {
            const auto created = CreateDirectory(to, from);
            TKIT_RETURN_ON_ERROR(created);
        }

        DirectoryStream stream{};
        const auto opened = stream.Open(from);
        TKIT_RETURN_ON_ERROR(opened);

        const FileSystemPath source{from};
        const FileSystemPath destination{to};
        for (;;)
        {
            const auto next = stream.Next();
            TKIT_RETURN_ON_ERROR(next);
            if (!next.GetValue())
                break;

            const FileSystemPath src = source / stream.GetName();
            const FileSystemPath dst = destination / stream.GetName();
            const auto result = Copy(src.CString(), dst.CString(), flags | s_InRecursiveCopy);
            TKIT_RETURN_ON_ERROR(result);
        }
    }
    return ok();
}

FileSystemResult<u64> RemoveAll(const char *path)
{
    const auto status = GetStatus(path, false);
    TKIT_RETURN_ON_ERROR(status);
    if (!status->Exists())
        return ok(u64(0));

    u64 count = 0;
    if (status->IsDirectory())
    {
        DirectoryStream stream{};
        const auto opened = stream.Open(path);
        TKIT_RETURN_ON_ERROR(opened);

        const FileSystemPath directory{path};
        for (;;)
        {
            const auto next = stream.Next();
            TKIT_RETURN_ON_ERROR(next);
            if (!next.GetValue())
                break;

            const FileSystemPath child = directory / stream.GetName();
            const auto removed = RemoveAll(child.CString());
            TKIT_RETURN_ON_ERROR(removed);
            count += removed.GetValue();
        }
    }

    const auto removed = Remove(path);
    TKIT_RETURN_ON_ERROR(removed);
    return ok(count + (removed.GetValue() ? 1 : 0));
}

FileSystemResult<FileSystemPath> WeaklyCanonical(const char *path)
{
    const FileSystemPath source{path};
    const PathIterator end = source.end();

    // find the longest leading sequence of elements that exists
    FileSystemPath head{};
    PathIterator it = source.begin();
    for (; it != end; ++it)
    {
        const FileSystemPath candidate = head / *it;
        const auto status = GetStatus(candidate.CString(), true);
        TKIT_RETURN_ON_ERROR(status);
        if (!status->Exists())
            break;
        head = candidate;
    }

    FileSystemPath result{};
    if (!head.IsEmpty())
    {
        auto canonical = Canonical(head.CString());
        TKIT_RETURN_ON_ERROR(canonical);
        result = std::move(canonical.GetValue());
    }
    for (; it != end; ++it)
        result /= *it;

    return ok(result.LexicallyNormal());
}

FileSystemResult<FileSystemPath> Relative(const char *path, const char *base)
{
    const auto canonicalPath = WeaklyCanonical(path);
    TKIT_RETURN_ON_ERROR(canonicalPath);
    const auto canonicalBase = WeaklyCanonical(base);
    TKIT_RETURN_ON_ERROR(canonicalBase);
    return ok(canonicalPath->LexicallyRelative(canonicalBase.GetValue()));
}

FileSystemResult<FileSystemPath> Proximate(const char *path, const char *base)
{
    const auto canonicalPath = WeaklyCanonical(path);
    TKIT_RETURN_ON_ERROR(canonicalPath);
    const auto canonicalBase = WeaklyCanonical(base);
    TKIT_RETURN_ON_ERROR(canonicalBase);
    return ok(canonicalPath->LexicallyProximate(canonicalBase.GetValue()));
}
} // namespace TKit::Detail
