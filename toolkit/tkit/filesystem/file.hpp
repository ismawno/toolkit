#pragma once

#include "tkit/filesystem/filesystem.hpp"

namespace TKit
{
using FileOpenFlags = u8;
enum FileOpenFlagBit : FileOpenFlags
{
    FileOpenFlag_Read = 1 << 0,
    FileOpenFlag_Write = 1 << 1,
    // every write goes to the end of the file. implies write access
    FileOpenFlag_Append = 1 << 2,
    // create the file if it does not exist
    FileOpenFlag_Create = 1 << 3,
    // discard the contents of the file when opening it. requires write access
    FileOpenFlag_Truncate = 1 << 4,
    // together with FileOpenFlag_Create, fail if the file already exists
    FileOpenFlag_Exclusive = 1 << 5,

    // common combinations
    FileOpenFlag_Overwrite = FileOpenFlag_Write | FileOpenFlag_Create | FileOpenFlag_Truncate,
    FileOpenFlag_AppendOrCreate = FileOpenFlag_Append | FileOpenFlag_Create,
};

enum SeekOrigin : u8
{
    SeekOrigin_Begin,
    SeekOrigin_Current,
    SeekOrigin_End
};

/**
 * @brief An open file, read and written in binary mode (no newline translation on any platform).
 *
 * It is created through `Open()`, which reports errors through results. It is movable but not copyable, and it closes
 * the file when destroyed.
 */
class File
{
    TKIT_NON_COPYABLE(File)
  public:
    File() = default;
    ~File();

    File(File &&other);
    File &operator=(File &&other);

    template <PathLike P>
    static FileSystemResult<File> Open(const P &path, const FileOpenFlags flags = FileOpenFlag_Read)
    {
        return open(Detail::ToCString(path), flags);
    }

    void Close();

    bool IsOpen() const
    {
        return m_IsOpen;
    }

    /**
     * @brief Read up to `size` bytes into `data`.
     *
     * @return The number of bytes read. It is only smaller than `size` if the end of the file was reached.
     */
    FileSystemResult<usz> Read(void *data, usz size);

    /**
     * @brief Write all `size` bytes of `data`.
     */
    FileSystemResult<> Write(const void *data, usz size);
    FileSystemResult<> Write(const StringView str)
    {
        return Write(str.GetData(), str.GetSize());
    }

    /**
     * @brief Move the file position.
     *
     * @return The new position, from the beginning of the file.
     */
    FileSystemResult<u64> Seek(i64 offset, SeekOrigin origin = SeekOrigin_Begin);
    FileSystemResult<u64> GetPosition();
    FileSystemResult<u64> GetSize() const;

    /**
     * @brief Truncate or zero-extend the file. The file position is not modified.
     */
    FileSystemResult<> Resize(u64 size);

    /**
     * @brief Make sure everything written so far reaches the disk.
     */
    FileSystemResult<> Flush();

  private:
    static FileSystemResult<File> open(const char *path, FileOpenFlags flags);

    // a file descriptor on posix, a HANDLE on windows
    uptr m_Handle = 0;
    bool m_IsOpen = false;
};

/**
 * @brief Read a whole file into `contents`, which keeps its allocator.
 */
template <typename AllocState, PathLike P> FileSystemResult<> ReadFile(const P &path, String<AllocState> &contents)
{
    auto file = File::Open(path, FileOpenFlag_Read);
    TKIT_RETURN_ON_ERROR(file);

    const auto size = file->GetSize();
    TKIT_RETURN_ON_ERROR(size);

    const u64 fsize = size.GetValue();
    if (fsize >= u64(TKIT_USIZE_MAX) ||
        (String<AllocState>::Type == Array_Static && fsize >= u64(contents.GetCapacity())))
        return FileSystemError_TooLarge;

    contents.Resize(usize(fsize));
    const auto read = file->Read(contents.GetData(), usz(fsize));
    TKIT_RETURN_ON_ERROR(read);

    // the file may have shrunk in the meantime
    if (read.GetValue() < usz(fsize))
        contents.Resize(usize(read.GetValue()));
    return FileSystemResult<>::Ok();
}

/**
 * @brief Read a whole file into a new string. Allocators that need one use the currently pushed allocator.
 */
template <typename StringType = DynamicString, PathLike P> FileSystemResult<StringType> ReadFile(const P &path)
{
    StringType contents{};
    const auto result = ReadFile(path, contents);
    TKIT_RETURN_ON_ERROR(result);
    return FileSystemResult<StringType>::Ok(std::move(contents));
}

/**
 * @brief Write `size` bytes to a file. By default, the file is created if needed and its previous contents are
 * discarded. Use `FileOpenFlag_AppendOrCreate` to append instead.
 */
// the size is a template so that flags (an enum) are never mistaken for it: WriteFile(path, "text", flags) must pick
// the StringView overload
template <PathLike P, std::integral Size>
FileSystemResult<> WriteFile(const P &path, const void *data, const Size size,
                             const FileOpenFlags flags = FileOpenFlag_Overwrite)
{
    auto file = File::Open(path, flags);
    TKIT_RETURN_ON_ERROR(file);
    return file->Write(data, usz(size));
}
template <PathLike P>
FileSystemResult<> WriteFile(const P &path, const StringView contents,
                             const FileOpenFlags flags = FileOpenFlag_Overwrite)
{
    return WriteFile(path, contents.GetData(), contents.GetSize(), flags);
}
} // namespace TKit
