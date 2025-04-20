#include "tkit/utils/result.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;

TEST_CASE("Ok and Error static constructors, basic accessors", "[Result]")
{
    SECTION("Ok with u32")
    {
        auto r = Result<u32>::Ok(42);
        REQUIRE(r.IsOk());
        REQUIRE(static_cast<bool>(r));
        REQUIRE(r.GetValue() == 42);
        REQUIRE(*r == 42);
        REQUIRE(&r.GetValue() == r.operator->());
    }
    SECTION("Error with const char*")
    {
        auto e = Result<u32>::Error("failure");
        REQUIRE(!e.IsOk());
        REQUIRE(!static_cast<bool>(e));
        REQUIRE(std::string(e.GetError()) == "failure");
    }
}

TEST_CASE("Pointer-like operators", "[Result]")
{
    auto r = Result<std::string>::Ok("hello");
    // operator->
    REQUIRE(r->size() == 5);
    // operator*
    REQUIRE((*r) == "hello");
}

TEST_CASE("Copy construction and copy assignment", "[Result]")
{
    // copy Ok<Result<std::string>>
    auto r1 = Result<std::string>::Ok("orig");
    auto r2 = r1; // copy ctor
    r2.GetValue()[0] = 'O';
    REQUIRE(r1.GetValue() == "orig");
    REQUIRE(r2.GetValue() == "Orig");

    // copy assign from Ok to Ok
    auto r3 = Result<std::string>::Ok("abc");
    r3 = r1;
    REQUIRE(r3.GetValue() == "orig");

    // copy Error<Result<u32, std::string>>
    auto e1 = Result<u32, std::string>::Error(std::string("err"));
    auto e2 = e1; // copy ctor
    REQUIRE(e2.GetError() == "err");

    // copy assign Ok -> Error
    auto mix = Result<u32, std::string>::Ok(7);
    mix = e1;
    REQUIRE(!mix.IsOk());
    REQUIRE(mix.GetError() == "err");
}

TEST_CASE("Move construction and move assignment", "[Result]")
{
    // move Ok<Result<std::string>>
    auto r1 = Result<std::string>::Ok("move");
    auto r2 = std::move(r1);
    REQUIRE(r2.GetValue() == "move");

    // move assign Error
    auto e1 = Result<u32, std::string>::Error(std::string("foo"));
    auto e2 = Result<u32, std::string>::Ok(0);
    e2 = std::move(e1);
    REQUIRE(!e2.IsOk());
    REQUIRE(e2.GetError() == "foo");
}

struct RTrack
{
    static inline u32 ctor = 0, dtor = 0;
    RTrack()
    {
        ++ctor;
    }
    RTrack(const RTrack &)
    {
        ++ctor;
    }
    ~RTrack()
    {
        ++dtor;
    }
};

TEST_CASE("Destruction cleans up union members without leak", "[Result]")
{

    // Ok path
    {
        RTrack::ctor = RTrack::dtor = 0;
        auto r = Result<RTrack>::Ok();
        REQUIRE(RTrack::ctor == 1);
        // r goes out of scope → destructor should call ~RTrack()
    }
    REQUIRE(RTrack::ctor == RTrack::dtor);

    // Error path
    {
        RTrack::ctor = RTrack::dtor = 0;
        auto e = Result<u32, RTrack>::Error();
        REQUIRE(RTrack::ctor == 1);
    }
    REQUIRE(RTrack::ctor == RTrack::dtor);
}
