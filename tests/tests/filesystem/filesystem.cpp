#include "tkit/filesystem/filesystem.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

using namespace TKit;
using namespace TKit::Alias;

namespace fs = std::filesystem;

static TierAllocator s_Tier{{.MaxAllocation = 128_kib}};

namespace
{
struct FileSystem_TempDirectory
{
    FileSystem_TempDirectory()
    {
        PushTier(&s_Tier);
        const auto tmp = GetTempDirectoryPath();
        REQUIRE(tmp);
        Root = *tmp / "tkit-fs-test";
        REQUIRE(RemoveAll(Root));
        REQUIRE(CreateDirectory(Root).GetValue());
        PopTier();
    }
    ~FileSystem_TempDirectory()
    {
        PushTier(&s_Tier);
        REQUIRE(RemoveAll(Root));
        PopTier();
    }
    DynamicPath Root;
};

void writeFile(const DynamicPath &path, const std::string &contents)
{
    std::ofstream file{path.CString(), std::ios::binary};
    file << contents;
}

std::string readFile(const DynamicPath &path)
{
    std::ifstream file{path.CString(), std::ios::binary};
    return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}
} // namespace

TEST_CASE("Status and basic queries", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    const DynamicPath file = tmp.Root / "file.txt";
    writeFile(file, "hello");

    REQUIRE(PathExists(tmp.Root));
    REQUIRE(IsDirectory(tmp.Root));
    REQUIRE(PathExists(file));
    REQUIRE(IsRegularFile(file));
    REQUIRE_FALSE(IsDirectory(file));
    REQUIRE_FALSE(PathExists(tmp.Root / "nope"));
    REQUIRE(PathExists(file.CString()));
    REQUIRE(PathExists(std::string{file.CString()}));

    const auto missing = GetStatus(tmp.Root / "nope");
    REQUIRE(missing);
    REQUIRE(missing->Type == FileType_NotFound);

    REQUIRE(GetFileSize(file).GetValue() == 5);
    REQUIRE(GetFileSize(tmp.Root).GetError() == FileSystemError_IsDirectory);
    REQUIRE(GetFileSize(tmp.Root / "nope").GetError() == FileSystemError_NotFound);
    REQUIRE(GetHardLinkCount(file).GetValue() == 1);

    REQUIRE_FALSE(IsEmpty(file).GetValue());
    REQUIRE_FALSE(IsEmpty(tmp.Root).GetValue());
    const DynamicPath emptyDir = tmp.Root / "empty";
    REQUIRE(CreateDirectory(emptyDir).GetValue());
    REQUIRE(IsEmpty(emptyDir).GetValue());

    REQUIRE(Equivalent(file, tmp.Root / "." / "file.txt").GetValue());
    REQUIRE_FALSE(Equivalent(file, emptyDir).GetValue());

    const auto space = GetSpace(tmp.Root);
    REQUIRE(space);
    REQUIRE(space->Capacity >= space->Free);
    REQUIRE(space->Free >= space->Available);
    PopTier();
}

TEST_CASE("Times, sizes and permissions", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    const DynamicPath file = tmp.Root / "file.bin";
    writeFile(file, "0123456789");

    REQUIRE(ResizeFile(file, 4));
    REQUIRE(GetFileSize(file).GetValue() == 4);
    REQUIRE(readFile(file) == "0123");

    const FileTime time = FileTime{std::chrono::seconds{1000000000} + std::chrono::nanoseconds{123456789}};
    REQUIRE(SetLastWriteTime(file, time));
    REQUIRE(GetLastWriteTime(file).GetValue() == time);

    REQUIRE(SetPermissions(file, FilePermission_OwnerRead | FilePermission_OwnerWrite));
    REQUIRE(GetStatus(file)->Permissions == (FilePermission_OwnerRead | FilePermission_OwnerWrite));
    REQUIRE(SetPermissions(file, FilePermission_GroupRead, PermissionOptionFlag_Add));
    REQUIRE(GetStatus(file)->Permissions ==
            (FilePermission_OwnerRead | FilePermission_OwnerWrite | FilePermission_GroupRead));
    REQUIRE(SetPermissions(file, FilePermission_OwnerWrite, PermissionOptionFlag_Remove));
    REQUIRE(GetStatus(file)->Permissions == (FilePermission_OwnerRead | FilePermission_GroupRead));
    PopTier();
}

TEST_CASE("Directory creation and removal", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    const DynamicPath deep = tmp.Root / "a/b/c/d";

    REQUIRE(CreateDirectory(deep).GetError() == FileSystemError_NotFound);
    REQUIRE(CreateDirectories(deep).GetValue());
    REQUIRE(IsDirectory(deep));
    REQUIRE_FALSE(CreateDirectories(deep).GetValue());
    REQUIRE_FALSE(CreateDirectory(deep).GetValue());
    REQUIRE(CreateDirectories(tmp.Root / "a/b/e/").GetValue());

    writeFile(deep / "f1", "x");
    writeFile(tmp.Root / "a" / "f2", "y");
    REQUIRE(CreateDirectory(tmp.Root / "a" / "f2").GetError() == FileSystemError_AlreadyExists);

    REQUIRE(Remove(tmp.Root / "a").GetError() == FileSystemError_DirectoryNotEmpty);
    REQUIRE_FALSE(Remove(tmp.Root / "missing").GetValue());
    REQUIRE(Remove(deep / "f1").GetValue());

    // a, a/b, a/b/c, a/b/c/d, a/b/e, a/f2
    REQUIRE(RemoveAll(tmp.Root / "a").GetValue() == 6);
    REQUIRE_FALSE(PathExists(tmp.Root / "a"));
    REQUIRE(RemoveAll(tmp.Root / "a").GetValue() == 0);
    PopTier();
}

TEST_CASE("Symlinks and hard links", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    const DynamicPath file = tmp.Root / "target.txt";
    const DynamicPath dir = tmp.Root / "dir";
    writeFile(file, "content");
    REQUIRE(CreateDirectory(dir).GetValue());

    const DynamicPath link = tmp.Root / "link";
    REQUIRE(CreateSymlink("target.txt", link));
    REQUIRE(IsSymlink(link));
    REQUIRE(IsRegularFile(link));
    REQUIRE(GetSymlinkStatus(link)->Type == FileType_Symlink);
    REQUIRE(ReadSymlink(link).GetValue() == "target.txt");
    REQUIRE(Canonical(link).GetValue() == Canonical(file).GetValue());

    const DynamicPath dirLink = tmp.Root / "dirlink";
    REQUIRE(CreateDirectorySymlink(dir, dirLink));
    REQUIRE(IsDirectory(dirLink));
    REQUIRE(IsSymlink(dirLink));

    const DynamicPath hard = tmp.Root / "hard";
    REQUIRE(CreateHardLink(file, hard));
    REQUIRE(GetHardLinkCount(file).GetValue() == 2);
    REQUIRE(Equivalent(file, hard).GetValue());

    const DynamicPath linkCopy = tmp.Root / "link-copy";
    REQUIRE(CopySymlink(link, linkCopy));
    REQUIRE(ReadSymlink(linkCopy).GetValue() == "target.txt");

    // removing a symlink must not touch its target
    REQUIRE(Remove(link).GetValue());
    REQUIRE(PathExists(file));
    REQUIRE(RemoveAll(dirLink).GetValue() == 1);
    REQUIRE(IsDirectory(dir));

    const DynamicPath broken = tmp.Root / "broken";
    REQUIRE(CreateSymlink("does-not-exist", broken));
    REQUIRE_FALSE(PathExists(broken));
    REQUIRE(IsSymlink(broken));
    PopTier();
}

TEST_CASE("Copy and rename", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    const DynamicPath file = tmp.Root / "a.txt";
    writeFile(file, "first");

    const DynamicPath copy = tmp.Root / "b.txt";
    REQUIRE(CopyFile(file, copy).GetValue());
    REQUIRE(readFile(copy) == "first");
    REQUIRE(CopyFile(file, copy).GetError() == FileSystemError_AlreadyExists);
    REQUIRE_FALSE(CopyFile(file, copy, CopyOptionFlag_SkipExisting).GetValue());

    writeFile(file, "second");
    REQUIRE(CopyFile(file, copy, CopyOptionFlag_OverwriteExisting).GetValue());
    REQUIRE(readFile(copy) == "second");

    REQUIRE(SetLastWriteTime(copy, FileTime{std::chrono::seconds{2000000000}}));
    REQUIRE_FALSE(CopyFile(file, copy, CopyOptionFlag_UpdateExisting).GetValue());
    REQUIRE(SetLastWriteTime(copy, FileTime{std::chrono::seconds{1}}));
    REQUIRE(CopyFile(file, copy, CopyOptionFlag_UpdateExisting).GetValue());

    // a bigger file to go through several write loops
    std::string big(300000, 'x');
    for (usize i = 0; i < big.size(); ++i)
        big[i] = char('a' + i % 26);
    const DynamicPath bigFile = tmp.Root / "big";
    writeFile(bigFile, big);
    REQUIRE(CopyFile(bigFile, tmp.Root / "big-copy").GetValue());
    REQUIRE(readFile(tmp.Root / "big-copy") == big);

    // tree copy
    const DynamicPath tree = tmp.Root / "tree";
    REQUIRE(CreateDirectories(tree / "x/y"));
    writeFile(tree / "x/y/leaf", "leaf");
    writeFile(tree / "root", "root");
    REQUIRE(CreateSymlink("root", tree / "rootlink"));

    const DynamicPath treeCopy = tmp.Root / "tree-copy";
    REQUIRE(Copy(tree, treeCopy, CopyOptionFlag_Recursive | CopyOptionFlag_CopySymlinks));
    REQUIRE(readFile(treeCopy / "x/y/leaf") == "leaf");
    REQUIRE(readFile(treeCopy / "root") == "root");
    REQUIRE(IsSymlink(treeCopy / "rootlink"));

    const DynamicPath shallow = tmp.Root / "shallow";
    REQUIRE(Copy(tree, shallow, CopyOptionFlag_None));
    REQUIRE(PathExists(shallow / "root"));
    REQUIRE_FALSE(PathExists(shallow / "x"));

    const DynamicPath dirsOnly = tmp.Root / "dirs-only";
    REQUIRE(Copy(tree, dirsOnly, CopyOptionFlag_Recursive | CopyOptionFlag_DirectoriesOnly));
    REQUIRE(IsDirectory(dirsOnly / "x/y"));
    REQUIRE_FALSE(PathExists(dirsOnly / "root"));

    REQUIRE(Copy(file, tree));
    REQUIRE(readFile(tree / "a.txt") == "second");

    REQUIRE(Rename(copy, tmp.Root / "renamed.txt"));
    REQUIRE_FALSE(PathExists(copy));
    REQUIRE(readFile(tmp.Root / "renamed.txt") == "second");
    PopTier();
}

TEST_CASE("Current, absolute, canonical and relative paths", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    REQUIRE(CreateDirectories(tmp.Root / "x/y"));

    const auto previous = GetCurrentPath();
    REQUIRE(previous);
    REQUIRE(previous->GetString() == fs::current_path().string());

    REQUIRE(SetCurrentPath(tmp.Root));
    const DynamicPath canonicalRoot = Canonical(tmp.Root).GetValue();
    REQUIRE(GetCurrentPath().GetValue() == canonicalRoot);

    REQUIRE(Absolute(DynamicPath{"x/y"}).GetValue() == canonicalRoot / "x/y");
    REQUIRE(Absolute(DynamicPath{"/already"}).GetValue() == "/already");
    REQUIRE(Canonical(DynamicPath{"x/./y/../y"}).GetValue() == canonicalRoot / "x/y");
    REQUIRE(Canonical(DynamicPath{"x/nope"}).GetError() == FileSystemError_NotFound);
    REQUIRE(WeaklyCanonical(DynamicPath{"x/y/../nope/./z"}).GetValue() == canonicalRoot / "x/nope/z");
    REQUIRE(Relative(DynamicPath{"x/y"}).GetValue() == "x/y");
    REQUIRE(Relative(DynamicPath{"x"}, "x/y").GetValue() == "..");

    // compare against std
    for (const char *raw : {"x", "x/y/..", "nope/a/../b", "/", "x/y/"})
    {
        INFO(raw);
        REQUIRE(WeaklyCanonical(DynamicPath{raw}).GetValue().GetString() == fs::weakly_canonical(raw).string());
        REQUIRE(Relative(DynamicPath{raw}, "x").GetValue().GetString() == fs::relative(raw, "x").string());
        REQUIRE(Proximate(DynamicPath{raw}, "x").GetValue().GetString() == fs::proximate(raw, "x").string());
    }

    REQUIRE(SetCurrentPath(*previous));
    PopTier();
}

template <typename AllocState, typename... Args> static void testDirectoryIteration(Args... args)
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};
    REQUIRE(CreateDirectories(tmp.Root / "d1/d2"));
    writeFile(tmp.Root / "f1", "1");
    writeFile(tmp.Root / "d1/f2", "22");
    writeFile(tmp.Root / "d1/d2/f3", "333");
    REQUIRE(CreateDirectorySymlink("d1", tmp.Root / "link"));

    const Path<AllocState> root{tmp.Root.GetView(), args...};

    {
        std::set<std::string> names{};
        auto it = DirectoryIterator<AllocState>::Open(root);
        REQUIRE(it);
        for (const auto &entry : *it)
        {
            names.insert(std::string{entry.GetPath().GetFilename().CString()});
            if (entry.GetPath().GetFilename() == "f1")
            {
                REQUIRE(entry.IsRegularFile());
                REQUIRE(entry.GetFileSize().GetValue() == 1);
            }
            if (entry.GetPath().GetFilename() == "link")
            {
                REQUIRE(entry.IsSymlink());
                REQUIRE(entry.IsDirectory());
            }
        }
        REQUIRE(names == std::set<std::string>{"d1", "f1", "link"});
    }

    {
        std::set<std::string> relatives{};
        auto it = RecursiveDirectoryIterator<AllocState>::Open(root);
        REQUIRE(it);
        for (const auto &entry : *it)
            relatives.insert(std::string{entry.GetPath().LexicallyRelative(root).CString()});
        REQUIRE(relatives == std::set<std::string>{"d1", "d1/d2", "d1/d2/f3", "d1/f2", "f1", "link"});
    }

    {
        std::set<std::string> relatives{};
        auto it = RecursiveDirectoryIterator<AllocState>::Open(root, DirectoryOptionFlag_FollowDirectorySymlink);
        REQUIRE(it);
        u32 maxDepth = 0;
        while (!it->IsEnd())
        {
            relatives.insert(std::string{it->GetEntry().GetPath().LexicallyRelative(root).CString()});
            maxDepth = std::max(maxDepth, u32(it->GetDepth()));
            REQUIRE(it->Advance());
        }
        REQUIRE(maxDepth == 2);
        REQUIRE(relatives == std::set<std::string>{"d1", "d1/d2", "d1/d2/f3", "d1/f2", "f1", "link", "link/d2",
                                                   "link/d2/f3", "link/f2"});
    }

    {
        // disabling recursion
        std::set<std::string> relatives{};
        auto it = RecursiveDirectoryIterator<AllocState>::Open(root);
        REQUIRE(it);
        while (!it->IsEnd())
        {
            const auto &entry = it->GetEntry();
            relatives.insert(std::string{entry.GetPath().LexicallyRelative(root).CString()});
            if (entry.GetPath().GetFilename() == "d1")
                it->DisableRecursionPending();
            REQUIRE(it->Advance());
        }
        REQUIRE(relatives == std::set<std::string>{"d1", "f1", "link"});
    }

    REQUIRE(DirectoryIterator<AllocState>::Open(root / "nope").GetError() == FileSystemError_NotFound);
    REQUIRE(DirectoryIterator<AllocState>::Open(root / "f1").GetError() == FileSystemError_IsNotDirectory);
    PopTier();
}

TEST_CASE("Directory iteration (dynamic)", "[filesystem]")
{
    testDirectoryIteration<DynamicAllocation<char>>();
}
TEST_CASE("Directory iteration (static)", "[filesystem]")
{
    testDirectoryIteration<StaticAllocation<char, 513>>();
}
TEST_CASE("Directory iteration (stack)", "[filesystem]")
{
    StackAllocator stack{64_kib};
    testDirectoryIteration<StackAllocation<char>>(&stack);
    REQUIRE(stack.IsEmpty());
}
TEST_CASE("Directory iteration (tier)", "[filesystem]")
{
    testDirectoryIteration<TierAllocation<char>>(&s_Tier);
}

TEST_CASE("Path returning functions keep the allocation type", "[filesystem]")
{
    PushTier(&s_Tier);
    const FileSystem_TempDirectory tmp{};

    const auto current = GetCurrentPath<StaticPath1024>();
    REQUIRE(current);
    REQUIRE(current->GetString() == fs::current_path().string());

    const auto tooSmall = GetCurrentPath<StaticPath<1>>();
    REQUIRE(tooSmall.GetError() == FileSystemError_NameTooLong);

    StackAllocator stack{4_kib};
    {
        const StackPath path{tmp.Root.GetView(), &stack};
        const auto canonical = Canonical(path);
        REQUIRE(canonical);
        REQUIRE(canonical->GetString().GetAllocator() == &stack);
        REQUIRE(canonical.GetValue() == Canonical(tmp.Root).GetValue());
    }
    REQUIRE(stack.IsEmpty());
    const TierPath tpath{"some/relative"};
    const auto absolute = Absolute(tpath);
    REQUIRE(absolute);
    REQUIRE(absolute->GetString().GetAllocator() == &s_Tier);
    REQUIRE(absolute->IsAbsolute());
    PopTier();
}
