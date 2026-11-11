#include "tkit/filesystem/file.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string>

using namespace TKit;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{64_kib};
static StackAllocator s_Stack{64_kib};
static TierAllocator s_Tier{{.MaxAllocation = 128_kib}};

template <typename T> using StaticAlloc1024 = StaticAllocation<T, 1025>;

namespace
{
// the filesystem functions build their internal paths with the pushed tier allocator (FileSystemPath), so one is kept
// pushed for as long as the directory lives
struct File_TempDirectory
{
    File_TempDirectory()
    {
        PushTier(&s_Tier);
        const auto tmp = GetTempDirectoryPath();
        REQUIRE(tmp);
        Root = *tmp / "tkit-file-test";
        REQUIRE(RemoveAll(Root));
        REQUIRE(CreateDirectory(Root).GetValue());
    }
    ~File_TempDirectory()
    {
        REQUIRE(RemoveAll(Root));
        PopTier();
    }
    DynamicPath Root;
};
} // namespace

static std::string readAll(File &file)
{
    std::string contents{};
    char buffer[64];
    for (;;)
    {
        const auto read = file.Read(buffer, sizeof(buffer));
        REQUIRE(read);
        if (read.GetValue() == 0)
            break;
        contents.append(buffer, read.GetValue());
    }
    return contents;
}

TEST_CASE("File: open flags", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "file.txt";

    REQUIRE(File::Open(path, FileOpenFlag_Read).GetError() == FileSystemError_NotFound);
    REQUIRE(File::Open(path, FileOpenFlag_Write).GetError() == FileSystemError_NotFound);
    REQUIRE_FALSE(PathExists(path));

    {
        auto file = File::Open(path, FileOpenFlag_Write | FileOpenFlag_Create);
        REQUIRE(file);
        REQUIRE(file->IsOpen());
        REQUIRE(file->Write("hello"));
    }
    REQUIRE(GetFileSize(path).GetValue() == 5);

    // exclusive creation fails on existing files
    REQUIRE(File::Open(path, FileOpenFlag_Write | FileOpenFlag_Create | FileOpenFlag_Exclusive).GetError() ==
            FileSystemError_AlreadyExists);

    // opening for writing without truncating keeps the contents
    {
        auto file = File::Open(path, FileOpenFlag_Write);
        REQUIRE(file);
        REQUIRE(file->Write("J"));
    }
    REQUIRE(ReadFile(path).GetValue() == "Jello");

    // append
    {
        auto file = File::Open(path, FileOpenFlag_AppendOrCreate);
        REQUIRE(file);
        REQUIRE(file->Seek(0));
        REQUIRE(file->Write(", world"));
    }
    REQUIRE(ReadFile(path).GetValue() == "Jello, world");

    // truncate
    {
        auto file = File::Open(path, FileOpenFlag_Overwrite);
        REQUIRE(file);
        REQUIRE(file->GetSize().GetValue() == 0);
    }
    REQUIRE(GetFileSize(path).GetValue() == 0);

    // directories cannot be opened for writing
    REQUIRE_FALSE(File::Open(tmp.Root, FileOpenFlag_Write));
}

TEST_CASE("File: read, write and seek", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "data.bin";

    auto file = File::Open(path, FileOpenFlag_Read | FileOpenFlag_Write | FileOpenFlag_Create);
    REQUIRE(file);

    // binary data, including zeros and newlines that must not be translated
    const u8 data[] = {0, 1, 2, '\n', '\r', '\n', 255, 0, 42};
    REQUIRE(file->Write(data, sizeof(data)));
    REQUIRE(file->GetPosition().GetValue() == sizeof(data));
    REQUIRE(file->GetSize().GetValue() == sizeof(data));

    REQUIRE(file->Seek(0).GetValue() == 0);
    u8 read[sizeof(data)]{};
    REQUIRE(file->Read(read, sizeof(read)).GetValue() == sizeof(data));
    REQUIRE(std::memcmp(read, data, sizeof(data)) == 0);

    // reading at the end returns 0 bytes, reading past it returns the remaining ones
    REQUIRE(file->Read(read, sizeof(read)).GetValue() == 0);
    REQUIRE(file->Seek(-2, SeekOrigin_End).GetValue() == sizeof(data) - 2);
    REQUIRE(file->Read(read, sizeof(read)).GetValue() == 2);
    REQUIRE(read[0] == 0);
    REQUIRE(read[1] == 42);

    REQUIRE(file->Seek(3).GetValue() == 3);
    REQUIRE(file->Seek(2, SeekOrigin_Current).GetValue() == 5);
    REQUIRE(file->Write("xy"));
    REQUIRE(file->Seek(0));
    const std::string contents = readAll(*file);
    REQUIRE(contents.size() == sizeof(data));
    REQUIRE(contents[5] == 'x');
    REQUIRE(contents[6] == 'y');

    // writing past the end leaves a zero filled gap
    REQUIRE(file->Seek(20));
    REQUIRE(file->Write("z"));
    REQUIRE(file->GetSize().GetValue() == 21);

    REQUIRE(file->Flush());
}

TEST_CASE("File: resize", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "resize.bin";
    REQUIRE(WriteFile(path, "0123456789"));

    auto file = File::Open(path, FileOpenFlag_Read | FileOpenFlag_Write);
    REQUIRE(file);
    REQUIRE(file->Seek(8));

    REQUIRE(file->Resize(4));
    REQUIRE(file->GetSize().GetValue() == 4);
    // the position is not modified
    REQUIRE(file->GetPosition().GetValue() == 8);

    REQUIRE(file->Resize(6));
    REQUIRE(file->Seek(0));
    const std::string contents = readAll(*file);
    REQUIRE(contents == std::string{"0123\0\0", 6});
}

TEST_CASE("File: move and close", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "move.txt";

    File file{};
    REQUIRE_FALSE(file.IsOpen());
    {
        auto opened = File::Open(path, FileOpenFlag_Overwrite);
        REQUIRE(opened);
        file = std::move(*opened);
        REQUIRE_FALSE(opened->IsOpen());
    }
    REQUIRE(file.IsOpen());
    REQUIRE(file.Write("moved"));

    File other{std::move(file)};
    REQUIRE_FALSE(file.IsOpen());
    REQUIRE(other.IsOpen());
    other.Close();
    REQUIRE_FALSE(other.IsOpen());
    other.Close();

    REQUIRE(ReadFile(path).GetValue() == "moved");
}

TEST_CASE("File: WriteFile and ReadFile", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "contents.txt";

    REQUIRE(WriteFile(path, "first line\n"));
    REQUIRE(WriteFile(path, "second line\n", FileOpenFlag_AppendOrCreate));
    REQUIRE(ReadFile(path).GetValue() == "first line\nsecond line\n");

    REQUIRE(WriteFile(path, "replaced"));
    REQUIRE(ReadFile(path).GetValue() == "replaced");

    const u32 numbers[] = {1, 2, 3, 0xFFFFFFFF};
    REQUIRE(WriteFile(path.CString(), numbers, sizeof(numbers)));
    REQUIRE(GetFileSize(path).GetValue() == sizeof(numbers));

    REQUIRE(WriteFile(path, ""));
    const auto empty = ReadFile(path);
    REQUIRE(empty);
    REQUIRE(empty->IsEmpty());

    REQUIRE(ReadFile(tmp.Root / "missing").GetError() == FileSystemError_NotFound);
    REQUIRE(WriteFile(tmp.Root / "missing-dir" / "file", "x").GetError() == FileSystemError_NotFound);
}

// ---------------------------------------------------------------------------
// ReadFile into every string type
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> static void TestReadFileInto(Args... args)
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "text.txt";

    std::string expected{};
    for (u32 i = 0; i < 40; ++i)
        expected += "line " + std::to_string(i) + "\n";
    REQUIRE(WriteFile(path, StringView{expected}));

    {
        String<A<char>> contents{};
        if constexpr (String<A<char>>::Type == Array_Arena || String<A<char>>::Type == Array_Stack)
            contents.Reserve(1024);
        REQUIRE(ReadFile(path, contents));
        REQUIRE(std::string{contents.CString()} == expected);
        REQUIRE(contents.GetSize() == expected.size());
        if constexpr (sizeof...(Args) != 0)
            REQUIRE(((contents.GetAllocator() == args) && ...));
    }
    {
        // reading the same file twice into the same string reuses it
        String<A<char>> contents{};
        if constexpr (String<A<char>>::Type == Array_Arena || String<A<char>>::Type == Array_Stack)
            contents.Reserve(1024);
        REQUIRE(ReadFile(path, contents));
        REQUIRE(WriteFile(path, "short"));
        REQUIRE(ReadFile(path, contents));
        REQUIRE(contents == "short");
    }
}

// The allocator is pushed around each run (paths take it from there), and still passed so tests can check it is used.
// The arena is reset after every run so that it does not run out, and the stack must be empty after every run
#define FILE_TEST_ALL(Fn)                                                                                              \
    PushArena(&s_Arena);                                                                                               \
    Fn<ArenaAllocation>(&s_Arena);                                                                                     \
    PopArena();                                                                                                        \
    s_Arena.Reset();                                                                                                   \
    Fn<DynamicAllocation>();                                                                                           \
    PushStack(&s_Stack);                                                                                               \
    Fn<StackAllocation>(&s_Stack);                                                                                     \
    PopStack();                                                                                                        \
    REQUIRE(s_Stack.IsEmpty());                                                                                        \
    Fn<StaticAlloc1024>();                                                                                             \
    PushTier(&s_Tier);                                                                                                 \
    Fn<TierAllocation>(&s_Tier);                                                                                       \
    PopTier()

TEST_CASE("File: ReadFile into every string type", "[file]")
{
    FILE_TEST_ALL(TestReadFileInto);
}

TEST_CASE("File: ReadFile into a string that is too small", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "big.txt";
    REQUIRE(WriteFile(path, "this does not fit in 8 characters"));
    REQUIRE(ReadFile<StaticString8>(path).GetError() == FileSystemError_TooLarge);
    REQUIRE(ReadFile<StaticString64>(path).GetValue() == "this does not fit in 8 characters");
}

TEST_CASE("File: ReadFile with the pushed allocator", "[file]")
{
    const File_TempDirectory tmp{};
    const DynamicPath path = tmp.Root / "pushed.txt";
    REQUIRE(WriteFile(path, "pushed"));

    PushTier(&s_Tier);
    {
        const auto contents = ReadFile<TierString>(path);
        REQUIRE(contents);
        REQUIRE(contents.GetValue() == "pushed");
        REQUIRE(contents->GetAllocator() == &s_Tier);
    }
    PopTier();
}
