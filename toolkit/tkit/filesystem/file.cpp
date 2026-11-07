#include "tkit/core/pch.hpp"
#include "tkit/filesystem/file.hpp"

#ifdef TKIT_OS_WINDOWS
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#else
#    include <cerrno>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

namespace TKit
{
File::~File()
{
    Close();
}

File::File(File &&other) : m_Handle(other.m_Handle), m_IsOpen(other.m_IsOpen)
{
    other.m_Handle = 0;
    other.m_IsOpen = false;
}

File &File::operator=(File &&other)
{
    if (this != &other)
    {
        Close();
        m_Handle = other.m_Handle;
        m_IsOpen = other.m_IsOpen;
        other.m_Handle = 0;
        other.m_IsOpen = false;
    }
    return *this;
}

static FileSystemResult<> lastError()
{
    return FileSystemResult<>::Error(Detail::LastError());
}
template <typename T> static FileSystemResult<T> lastError()
{
    return FileSystemResult<T>::Error(Detail::LastError());
}

#ifdef TKIT_OS_WINDOWS
// ============================================================================================================
// Windows
// ============================================================================================================

static HANDLE toHandle(const uptr handle)
{
    return rcast<HANDLE>(handle);
}

FileSystemResult<File> File::open(const char *path, const FileOpenFlags flags)
{
    const int wsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    if (wsize <= 0)
        return FileSystemResult<File>::Error(FileSystemError_InvalidArgument);
    DynamicArray<wchar_t> wpath{};
    wpath.Resize(usize(wsize));
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wpath.GetData(), wsize);

    DWORD access = 0;
    if (flags & FileOpenFlag_Read)
        access |= GENERIC_READ;
    // FILE_APPEND_DATA without FILE_WRITE_DATA makes every write go to the end of the file
    if (flags & FileOpenFlag_Append)
        access |= FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE;
    else if (flags & FileOpenFlag_Write)
        access |= GENERIC_WRITE;

    DWORD disposition = OPEN_EXISTING;
    if (flags & FileOpenFlag_Create)
    {
        if (flags & FileOpenFlag_Exclusive)
            disposition = CREATE_NEW;
        else if (flags & FileOpenFlag_Truncate)
            disposition = CREATE_ALWAYS;
        else
            disposition = OPEN_ALWAYS;
    }
    else if (flags & FileOpenFlag_Truncate)
        disposition = TRUNCATE_EXISTING;

    const HANDLE handle = CreateFileW(wpath.GetData(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return lastError<File>();

    File file{};
    file.m_Handle = rcast<uptr>(handle);
    file.m_IsOpen = true;
    return FileSystemResult<File>::Ok(std::move(file));
}

void File::Close()
{
    if (!m_IsOpen)
        return;
    CloseHandle(toHandle(m_Handle));
    m_Handle = 0;
    m_IsOpen = false;
}

FileSystemResult<usz> File::Read(void *data, const usz size)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    std::byte *dst = scast<std::byte *>(data);
    usz total = 0;
    while (total < size)
    {
        const usz remaining = size - total;
        const DWORD chunk = remaining > 0x40000000 ? DWORD(0x40000000) : DWORD(remaining);
        DWORD bytes = 0;
        if (!::ReadFile(toHandle(m_Handle), dst + total, chunk, &bytes, nullptr))
            return lastError<usz>();
        if (bytes == 0)
            break;
        total += bytes;
    }
    return FileSystemResult<usz>::Ok(total);
}

FileSystemResult<> File::Write(const void *data, const usz size)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    const std::byte *src = scast<const std::byte *>(data);
    usz total = 0;
    while (total < size)
    {
        const usz remaining = size - total;
        const DWORD chunk = remaining > 0x40000000 ? DWORD(0x40000000) : DWORD(remaining);
        DWORD bytes = 0;
        if (!::WriteFile(toHandle(m_Handle), src + total, chunk, &bytes, nullptr))
            return lastError();
        total += bytes;
    }
    return FileSystemResult<>::Ok();
}

FileSystemResult<u64> File::Seek(const i64 offset, const SeekOrigin origin)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    const DWORD method =
        origin == SeekOrigin_Begin ? FILE_BEGIN : (origin == SeekOrigin_Current ? FILE_CURRENT : FILE_END);
    LARGE_INTEGER distance;
    distance.QuadPart = LONGLONG(offset);
    LARGE_INTEGER position;
    if (!SetFilePointerEx(toHandle(m_Handle), distance, &position, method))
        return lastError<u64>();
    return FileSystemResult<u64>::Ok(u64(position.QuadPart));
}

FileSystemResult<u64> File::GetPosition()
{
    return Seek(0, SeekOrigin_Current);
}

FileSystemResult<u64> File::GetSize() const
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    LARGE_INTEGER size;
    if (!GetFileSizeEx(toHandle(m_Handle), &size))
        return lastError<u64>();
    return FileSystemResult<u64>::Ok(u64(size.QuadPart));
}

FileSystemResult<> File::Resize(const u64 size)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    FILE_END_OF_FILE_INFO info;
    info.EndOfFile.QuadPart = LONGLONG(size);
    if (!SetFileInformationByHandle(toHandle(m_Handle), FileEndOfFileInfo, &info, sizeof(info)))
        return lastError();
    return FileSystemResult<>::Ok();
}

FileSystemResult<> File::Flush()
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    if (!FlushFileBuffers(toHandle(m_Handle)))
        return lastError();
    return FileSystemResult<>::Ok();
}

#else
// ============================================================================================================
// POSIX
// ============================================================================================================

static int toDescriptor(const uptr handle)
{
    return int(handle);
}

FileSystemResult<File> File::open(const char *path, const FileOpenFlags flags)
{
    const bool read = flags & FileOpenFlag_Read;
    const bool write = flags & (FileOpenFlag_Write | FileOpenFlag_Append);

    int oflags = O_CLOEXEC;
    if (read && write)
        oflags |= O_RDWR;
    else if (write)
        oflags |= O_WRONLY;
    else
        oflags |= O_RDONLY;

    if (flags & FileOpenFlag_Append)
        oflags |= O_APPEND;
    if (flags & FileOpenFlag_Create)
        oflags |= O_CREAT;
    if (flags & FileOpenFlag_Truncate)
        oflags |= O_TRUNC;
    if (flags & FileOpenFlag_Exclusive)
        oflags |= O_EXCL;

    int fd;
    do
        fd = ::open(path, oflags, 0666);
    while (fd < 0 && errno == EINTR);
    if (fd < 0)
        return lastError<File>();

    File file{};
    file.m_Handle = uptr(fd);
    file.m_IsOpen = true;
    return FileSystemResult<File>::Ok(std::move(file));
}

void File::Close()
{
    if (!m_IsOpen)
        return;
    ::close(toDescriptor(m_Handle));
    m_Handle = 0;
    m_IsOpen = false;
}

FileSystemResult<usz> File::Read(void *data, const usz size)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    std::byte *dst = scast<std::byte *>(data);
    usz total = 0;
    while (total < size)
    {
        const ssize_t bytes = ::read(toDescriptor(m_Handle), dst + total, size - total);
        if (bytes < 0)
        {
            if (errno == EINTR)
                continue;
            return lastError<usz>();
        }
        if (bytes == 0)
            break;
        total += usz(bytes);
    }
    return FileSystemResult<usz>::Ok(total);
}

FileSystemResult<> File::Write(const void *data, const usz size)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    const std::byte *src = scast<const std::byte *>(data);
    usz total = 0;
    while (total < size)
    {
        const ssize_t bytes = ::write(toDescriptor(m_Handle), src + total, size - total);
        if (bytes < 0)
        {
            if (errno == EINTR)
                continue;
            return lastError();
        }
        total += usz(bytes);
    }
    return FileSystemResult<>::Ok();
}

FileSystemResult<u64> File::Seek(const i64 offset, const SeekOrigin origin)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    const int whence = origin == SeekOrigin_Begin ? SEEK_SET : (origin == SeekOrigin_Current ? SEEK_CUR : SEEK_END);
    const off_t position = ::lseek(toDescriptor(m_Handle), off_t(offset), whence);
    if (position < 0)
        return lastError<u64>();
    return FileSystemResult<u64>::Ok(u64(position));
}

FileSystemResult<u64> File::GetPosition()
{
    return Seek(0, SeekOrigin_Current);
}

FileSystemResult<u64> File::GetSize() const
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    struct stat st;
    if (::fstat(toDescriptor(m_Handle), &st) != 0)
        return lastError<u64>();
    return FileSystemResult<u64>::Ok(u64(st.st_size));
}

FileSystemResult<> File::Resize(const u64 size)
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    if (::ftruncate(toDescriptor(m_Handle), off_t(size)) != 0)
        return lastError();
    return FileSystemResult<>::Ok();
}

FileSystemResult<> File::Flush()
{
    TKIT_ASSERT(m_IsOpen, "[TOOLKIT][FILE] File is not open");
    if (::fsync(toDescriptor(m_Handle)) != 0)
        return lastError();
    return FileSystemResult<>::Ok();
}
#endif
} // namespace TKit
