#include "tkit/filesystem/path.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

using namespace TKit;
using namespace TKit::Alias;

namespace fs = std::filesystem;

static ArenaAllocator s_Arena{10_mib};
static StackAllocator s_Stack{16_kib};
static ArenaAllocator s_TierArena{64_kib};
static TierAllocator s_Tier{{.MaxAllocation = 16_kib}};

template <typename T> using StaticAlloc256 = StaticAllocation<T, 257>;

// static, arena and stack paths cannot grow in place, so every path is created with this capacity to leave room for
// in-place modifications
static constexpr usize s_Capacity = 128;

static const char *s_Paths[] = {"",
                                "/",
                                "a",
                                "a/",
                                "a//",
                                "/a",
                                "/a/",
                                "a/b",
                                "a/b/",
                                "a//b",
                                "/a/b/c.txt",
                                "foo.bar.baz",
                                ".bashrc",
                                ".",
                                "..",
                                "./",
                                "../",
                                "a/.",
                                "a/..",
                                "a/../",
                                "a/b/..",
                                "a/./b/../c",
                                "/..",
                                "/../a",
                                "../a/..",
                                "../../a/b",
                                "a/b/../../..",
                                "foo/.///bar/../",
                                "foo/./bar/..",
                                "/usr/local/lib/libtoolkit.a",
                                "dir.with.dots/file",
                                "dir.with.dots/",
                                "file.",
                                "..x",
                                "x..",
                                "/a/b/./c/../../d"};

// Separators are compared as '/', as standard libraries disagree on which one they return on windows (msvc keeps
// the original one, libstdc++ often uses the preferred one)
static std::string str(std::string result)
{
#ifdef TKIT_OS_WINDOWS
    for (char &c : result)
        if (c == '\\')
            c = '/';
#endif
    return result;
}
static std::string str(const StringView view)
{
    return str(std::string{view.GetData(), view.GetSize()});
}

// Creates a path with room for s_Capacity characters, using the given allocator (if any)
template <template <typename> typename A, typename... Args> Path<A<char>> makePath(const char *raw, Args... args)
{
    using P = Path<A<char>>;
    String<A<char>> str{args...};
    if constexpr (P::Type != Array_Static)
        str.Reserve(s_Capacity);
    P path{std::move(str)};
    path = raw;
    return path;
}

template <template <typename> typename A, typename... Args>
static bool usesAllocator(const Path<A<char>> &path, Args... args)
{
    if constexpr (sizeof...(Args) == 0)
        return true;
    else
        return ((path.GetString().GetAllocator() == args) && ...);
}

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> static void TestDecomposition(Args... args)
{
    for (const char *raw : s_Paths)
    {
        INFO("path: '" << raw << "'");
        const auto path = makePath<A>(raw, args...);
        const fs::path expected{raw};

        CHECK(str(path.GetView()) == str(expected.string()));
        CHECK(str(path.GetRootName()) == str(expected.root_name().string()));
        CHECK(str(path.GetRootDirectory()) == str(expected.root_directory().string()));
        CHECK(str(path.GetRootPath()) == str(expected.root_path().string()));
        CHECK(str(path.GetRelativePath()) == str(expected.relative_path().string()));
        CHECK(str(path.GetParentPath()) == str(expected.parent_path().string()));
        CHECK(str(path.GetFilename()) == str(expected.filename().string()));
        CHECK(str(path.GetStem()) == str(expected.stem().string()));
        CHECK(str(path.GetExtension()) == str(expected.extension().string()));

        CHECK(path.HasRootName() == expected.has_root_name());
        CHECK(path.HasRootDirectory() == expected.has_root_directory());
        CHECK(path.HasRootPath() == expected.has_root_path());
        CHECK(path.HasRelativePath() == expected.has_relative_path());
        CHECK(path.HasParentPath() == expected.has_parent_path());
        CHECK(path.HasFilename() == expected.has_filename());
        CHECK(path.HasStem() == expected.has_stem());
        CHECK(path.HasExtension() == expected.has_extension());
        CHECK(path.IsAbsolute() == expected.is_absolute());
        CHECK(path.IsRelative() == expected.is_relative());

        // decomposed paths share the allocator of the original one
        {
            const auto parent = path.GetParentPath();
            if (!parent.IsEmpty())
                CHECK(usesAllocator<A>(parent, args...));
        }

        CHECK(str(path.LexicallyNormal()) == str(expected.lexically_normal().string()));

        auto eit = expected.begin();
        for (const StringView element : path)
        {
            CHECK(eit != expected.end());
            CHECK(str(element) == str(eit->string()));
            ++eit;
        }
        CHECK(eit == expected.end());
    }
}

template <template <typename> typename A, typename... Args> static void TestBinaryOperations(Args... args)
{
    using P = Path<A<char>>;
    for (const char *lraw : s_Paths)
        for (const char *rraw : s_Paths)
        {
            INFO("lhs: '" << lraw << "', rhs: '" << rraw << "'");
            const P lhs = makePath<A>(lraw, args...);
            const P rhs = makePath<A>(rraw, args...);
            const fs::path elhs{lraw};
            const fs::path erhs{rraw};

            CHECK(str(lhs / rhs) == str((elhs / erhs).string()));
            CHECK(str(lhs.LexicallyRelative(rhs)) == str(elhs.lexically_relative(erhs).string()));
            CHECK(str(lhs.LexicallyProximate(rhs)) == str(elhs.lexically_proximate(erhs).string()));

            const i32 cmp = lhs.Compare(rhs);
            const int ecmp = elhs.compare(erhs);
            CHECK((cmp < 0) == (ecmp < 0));
            CHECK((cmp > 0) == (ecmp > 0));
            CHECK((lhs == rhs) == (elhs == erhs));
            if (lhs == rhs)
                CHECK(std::hash<P>{}(lhs) == std::hash<P>{}(rhs));

            // copies keep the capacity (and allocator) of the original
            {
                P appended = lhs;
                appended /= rhs;
                CHECK(str(appended) == str((elhs / erhs).string()));
            }
            {
                P concatenated = lhs;
                concatenated += rhs;
                // not using fs::path::operator+= here: libstdc++ 13 corrupts the path's component list on some
                // inputs
                CHECK(str(concatenated) == str(elhs.string() + erhs.string()));
            }
            {
                P replaced = lhs;
                replaced.ReplaceFilename(rhs);
                fs::path ereplaced = elhs;
                ereplaced.replace_filename(erhs);
                CHECK(str(replaced) == str(ereplaced.string()));
            }
            {
                P extension = lhs;
                extension.ReplaceExtension(rhs);
                // fs::path::replace_extension() goes through the same buggy operator+=, so the expected value is
                // built by hand
                std::string eextension =
                    elhs.string().substr(0, elhs.string().size() - elhs.extension().string().size());
                if (!erhs.empty() && erhs.string()[0] != '.')
                    eextension += '.';
                eextension += erhs.string();
                CHECK(str(extension) == str(eextension));
            }
        }
}

template <template <typename> typename A, typename... Args> static void TestModifiers(Args... args)
{
    auto path = makePath<A>("a/b/file.txt", args...);
    path.RemoveFilename();
    REQUIRE(path == "a/b/");
    path.ReplaceFilename("other.cpp");
    REQUIRE(str(path) == "a/b/other.cpp");
    path.ReplaceExtension("hpp");
    REQUIRE(str(path) == "a/b/other.hpp");
    path.ReplaceExtension(".cpp");
    REQUIRE(str(path) == "a/b/other.cpp");
    path.ReplaceExtension();
    REQUIRE(str(path) == "a/b/other");
    path += ".x";
    REQUIRE(str(path) == "a/b/other.x");
    path += 'y';
    REQUIRE(str(path) == "a/b/other.xy");
    path /= "c";
    REQUIRE(str(path) == "a/b/other.xy/c");
    path /= "/abs";
    REQUIRE(str(path) == "/abs");
    path.MakePreferred();
    REQUIRE(str(path) == "/abs");
    path.Clear();
    REQUIRE(path.IsEmpty());
    REQUIRE(path.CString()[0] == '\0');
    path = "again";
    REQUIRE(str(path) == "again");
    REQUIRE(usesAllocator<A>(path, args...));
}

template <template <typename> typename A, typename... Args> static void TestAliasing(Args... args)
{
    // appending, assigning and concatenating a path to itself
    auto path = makePath<A>("x/y", args...);
    path /= path;
    REQUIRE(str(path) == "x/y/x/y");
    path = path.GetView();
    REQUIRE(str(path) == "x/y/x/y");
    path += path;
    REQUIRE(str(path) == "x/y/x/yx/y/x/y");
    path = StringView{path.GetData() + 2, 3};
    REQUIRE(str(path) == "y/x");
}

template <template <typename> typename A, typename... Args> static void TestEmptyStates(Args... args)
{
    using P = Path<A<char>>;
    const P empty{};
    REQUIRE(empty.IsEmpty());
    REQUIRE(empty.GetSize() == 0);
    REQUIRE(empty.CString() != nullptr);
    REQUIRE(empty.CString()[0] == '\0');
    REQUIRE(empty.GetFilename().IsEmpty());
    REQUIRE(empty.GetParentPath().IsEmpty());
    REQUIRE(empty.LexicallyNormal().IsEmpty());
    REQUIRE(empty.begin() == empty.end());
    REQUIRE_FALSE(empty.HasFilename());
    REQUIRE(empty.IsRelative());

    const P fromNull{scast<const char *>(nullptr), args...};
    REQUIRE(fromNull.IsEmpty());
    REQUIRE(fromNull == empty);

    const P fromEmpty{"", args...};
    REQUIRE(fromEmpty.IsEmpty());
    REQUIRE(fromEmpty == empty);
}

template <template <typename> typename A, typename... Args> static void TestConversionsAndComparisons(Args... args)
{
    using P = Path<A<char>>;
    const DynamicPath dyn{"some/dir/file.txt"};
    const P path{dyn, args...};
    REQUIRE(path == dyn);
    REQUIRE(dyn == path);
    REQUIRE(str(path) == "some/dir/file.txt");
    REQUIRE(usesAllocator<A>(path, args...));

    const P fromStd{std::string{"a/b"}, args...};
    REQUIRE(fromStd == "a/b");
    REQUIRE(fromStd == "a//b");
    REQUIRE(fromStd != "a/c");
    REQUIRE(fromStd == StringView{"a/b/", 3});

    const P fromView{std::string_view{"a/c"}, args...};
    REQUIRE(fromStd < fromView);
    REQUIRE(fromView > fromStd);
    REQUIRE(fromStd <= fromStd);
    REQUIRE(fromStd >= fromStd);

    auto assigned = makePath<A>("", args...);
    assigned = "x/y";
    REQUIRE(str(assigned) == "x/y");
    assigned = std::string{"z"};
    REQUIRE(str(assigned) == "z");
    assigned = std::string_view{"w"};
    REQUIRE(str(assigned) == "w");
    assigned = dyn;
    REQUIRE(str(assigned) == "some/dir/file.txt");
    REQUIRE(usesAllocator<A>(assigned, args...));

    const P joined = "prefix" / path;
    REQUIRE(str(joined) == "prefix/some/dir/file.txt");
    REQUIRE(usesAllocator<A>(joined, args...));

    const P joinedDyn = path / dyn;
    REQUIRE(str(joinedDyn) == "some/dir/file.txt/some/dir/file.txt");

    REQUIRE(str(StringView{path.GetGenericString()}) == "some/dir/file.txt");
    REQUIRE(fmt::format("{}", path) == "some/dir/file.txt");
    REQUIRE(std::hash<P>{}(P{"a/b", args...}) == std::hash<P>{}(P{"a//b", args...}));
}

// Paths with repeated leading separators are handled differently by every standard library (libstdc++, libc++ and
// msvc disagree on their root directory, parent path and normal form), so they are checked against fixed values
template <template <typename> typename A, typename... Args> static void TestRepeatedSeparators(Args... args)
{
    {
        const auto path = makePath<A>("//", args...);
        CHECK(str(path.GetRootName()).empty());
        CHECK(str(path.GetRootDirectory()) == "/");
        CHECK(str(path.GetRootPath()) == "/");
        CHECK(str(path.GetRelativePath()).empty());
        CHECK(str(path.GetParentPath()) == "//");
        CHECK(str(path.GetFilename()).empty());
        CHECK(path.IsAbsolute() == Detail::IsAbsolutePath(StringView{"/"}));
        CHECK(str(path.LexicallyNormal()) == "/");

        auto it = path.begin();
        REQUIRE(it != path.end());
        CHECK(str(*it) == "/");
        CHECK(++it == path.end());
    }
    {
        const auto path = makePath<A>("///a", args...);
        CHECK(str(path.GetRootDirectory()) == "/");
        CHECK(str(path.GetRootPath()) == "/");
        CHECK(str(path.GetRelativePath()) == "a");
        CHECK(str(path.GetParentPath()) == "/");
        CHECK(str(path.GetFilename()) == "a");
        CHECK(str(path.LexicallyNormal()) == "/a");
        CHECK(path == "/a");

        auto it = path.begin();
        REQUIRE(it != path.end());
        CHECK(str(*it) == "/");
        REQUIRE(++it != path.end());
        CHECK(str(*it) == "a");
        CHECK(++it == path.end());
    }
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

// the arena is reset after every run so that it does not run out, and the stack must be empty after every run
#define PATH_TEST_ALL(Fn)                                                                                              \
    Fn<ArenaAllocation>(&s_Arena);                                                                                     \
    s_Arena.Reset();                                                                                                   \
    Fn<DynamicAllocation>();                                                                                           \
    Fn<StackAllocation>(&s_Stack);                                                                                     \
    REQUIRE(s_Stack.IsEmpty());                                                                                        \
    Fn<StaticAlloc256>();                                                                                              \
    Fn<TierAllocation>(&s_Tier)

TEST_CASE("Path: repeated separators", "[path]")
{
    PATH_TEST_ALL(TestRepeatedSeparators);
}

TEST_CASE("Path: decomposition matches std::filesystem", "[path]")
{
    PATH_TEST_ALL(TestDecomposition);
}

TEST_CASE("Path: binary operations match std::filesystem", "[path]")
{
    PATH_TEST_ALL(TestBinaryOperations);
}

TEST_CASE("Path: modifiers", "[path]")
{
    PATH_TEST_ALL(TestModifiers);
}

TEST_CASE("Path: aliasing", "[path]")
{
    PATH_TEST_ALL(TestAliasing);
}

TEST_CASE("Path: default and empty states", "[path]")
{
    PATH_TEST_ALL(TestEmptyStates);
}

TEST_CASE("Path: conversions and comparisons", "[path]")
{
    PATH_TEST_ALL(TestConversionsAndComparisons);
}

TEST_CASE("Path: tier paths grow beyond their initial size", "[path]")
{
    TierPath path{"base", &s_Tier};
    for (u32 i = 0; i < 64; ++i)
        path /= "segment";
    REQUIRE(path.GetString().GetAllocator() == &s_Tier);
    REQUIRE(path.GetSize() == 4 + 64 * 8);
    REQUIRE(path.GetFilename().GetString() == "segment");
}

TEST_CASE("Path: stack paths keep a LIFO allocation order", "[path]")
{
    {
        const StackPath path{"/usr/local/lib/libtoolkit.a", &s_Stack};
        const StackPath parent = path.GetParentPath();
        REQUIRE(str(parent) == "/usr/local/lib");

        const StackPath joined = parent / "other.a";
        REQUIRE(str(joined) == "/usr/local/lib/other.a");

        // the source must outlive the result (a temporary would be freed out of order)
        const StackPath source{"a/./b/../c", &s_Stack};
        const StackPath normal = source.LexicallyNormal();
        REQUIRE(str(normal) == "a/c");

        // shrinking in place never reallocates
        StackPath shrinking{"a/b/c.txt", &s_Stack};
        shrinking.RemoveFilename();
        shrinking.ReplaceFilename("d");
        REQUIRE(str(shrinking) == "a/b/d");
    }
    REQUIRE(s_Stack.IsEmpty());
}
