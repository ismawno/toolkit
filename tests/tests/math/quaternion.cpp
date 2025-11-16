#include "tkit/math/quaternion.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

using namespace TKit;
using namespace TKit::Alias;
using namespace TKit::Math;

TEST_CASE("Quaternion basic construction and indexing", "[Quaternion]")
{
    const f32q q(1.0f, 2.0f, 3.0f, 4.0f);

    REQUIRE(q.Length == 4);
    REQUIRE(q[0] == 1.0f);
    REQUIRE(q[1] == 2.0f);
    REQUIRE(q[2] == 3.0f);
    REQUIRE(q[3] == 4.0f);

    const f32v3 euler(0.1f, 0.2f, 0.3f);
    const f32q qe(euler); // FromEulerAngles constructor
    const f32v3 back = ToEulerAngles(Normalize(qe));

    REQUIRE(back[0] == Catch::Approx(euler[0]));
    REQUIRE(back[1] == Catch::Approx(euler[1]));
    REQUIRE(back[2] == Catch::Approx(euler[2]));

    const f32v4 v4(0.5f, 1.0f, 2.0f, 3.0f);
    const f32q qv(v4);

    REQUIRE(qv[0] == 0.5f);
    REQUIRE(qv[1] == 1.0f);
    REQUIRE(qv[2] == 2.0f);
    REQUIRE(qv[3] == 3.0f);
}

TEST_CASE("Quaternion dot, norm, and normalization", "[Quaternion]")
{
    const f32q q1(1.0f, 2.0f, 3.0f, 4.0f);
    const f32q q2(2.0f, 1.0f, 0.0f, -1.0f);

    const f32 dot = Dot(q1, q2);
    REQUIRE(dot == Catch::Approx(1 * 2 + 2 * 1 + 3 * 0 + 4 * -1));

    const f32 norm = Norm(q1);
    REQUIRE(norm == Catch::Approx(std::sqrt(30.0f)));

    const f32q normed = Normalize(q1);
    REQUIRE(Norm(normed) == Catch::Approx(1.0f));
}

TEST_CASE("Quaternion conjugate and inverse", "[Quaternion]")
{
    const f32q q(1.0f, 1.0f, 2.0f, 3.0f);

    const f32q c = Conjugate(q);
    REQUIRE(c[0] == Catch::Approx(1.0f));
    REQUIRE(c[1] == Catch::Approx(-1.0f));
    REQUIRE(c[2] == Catch::Approx(-2.0f));
    REQUIRE(c[3] == Catch::Approx(-3.0f));

    const f32q inv = Inverse(q);
    const f32q identity = q * inv;

    REQUIRE(identity.w == Catch::Approx(1.0f).margin(0.00001f));
    REQUIRE(identity.x == Catch::Approx(0.0f).margin(0.00001f));
    REQUIRE(identity.y == Catch::Approx(0.0f).margin(0.00001f));
    REQUIRE(identity.z == Catch::Approx(0.0f).margin(0.00001f));
}

TEST_CASE("Quaternion arithmetic operations", "[Quaternion]")
{
    const f32q a(1.0f, 2.0f, 3.0f, 4.0f);
    const f32q b(4.0f, 3.0f, 2.0f, 1.0f);

    const f32q sum = a + b;
    const f32q diff = a - b;
    const f32q scaled = a * 2.0f;
    const f32q divided = b / 2.0f;

    REQUIRE(sum[0] == 5.0f);
    REQUIRE(sum[1] == 5.0f);
    REQUIRE(sum[2] == 5.0f);
    REQUIRE(sum[3] == 5.0f);

    REQUIRE(diff[0] == -3.0f);
    REQUIRE(diff[1] == -1.0f);
    REQUIRE(diff[2] == 1.0f);
    REQUIRE(diff[3] == 3.0f);

    REQUIRE(scaled[0] == 2.0f);
    REQUIRE(scaled[1] == 4.0f);
    REQUIRE(scaled[2] == 6.0f);
    REQUIRE(scaled[3] == 8.0f);

    REQUIRE(divided[0] == 2.0f);
    REQUIRE(divided[1] == 1.5f);
    REQUIRE(divided[2] == 1.0f);
    REQUIRE(divided[3] == 0.5f);
}

TEST_CASE("Quaternion multiplication", "[Quaternion]")
{
    const f32q q1(0.0f, 1.0f, 0.0f, 0.0f);
    const f32q q2(0.0f, 0.0f, 1.0f, 0.0f);

    const f32q q12 = q1 * q2;

    REQUIRE(q12.w == Catch::Approx(0.0f));
    REQUIRE(q12.x == Catch::Approx(0.0f));
    REQUIRE(q12.y == Catch::Approx(0.0f));
    REQUIRE(q12.z == Catch::Approx(1.0f));
}

TEST_CASE("Quaternion rotate vector", "[Quaternion]")
{
    const f32v3 axis(0.0f, 0.0f, 1.0f);
    const f32 angle = Pi<f32>() / 2.0f;

    const f32q q = f32q::FromAngleAxis(angle, axis);

    const f32v3 v(1.0f, 0.0f, 0.0f);
    const f32v3 r = (q * v);

    REQUIRE(r[0] == Catch::Approx(0.0f).margin(0.000001f));
    REQUIRE(r[1] == Catch::Approx(1.0f).margin(0.000001f));
    REQUIRE(r[2] == Catch::Approx(0.0f).margin(0.000001f));
}

TEST_CASE("Quaternion conversions Mat3 and Mat4", "[Quaternion][Matrix]")
{
    const f32v3 axis(0.0f, 1.0f, 0.0f);
    const f32 angle = Pi<f32>() / 3.0f;

    const f32q q = f32q::FromAngleAxis(angle, axis);

    const f32m3 m3 = ToMat3(q);
    const f32m4 m4 = ToMat4(q);

    // Round-trip conversion check
    const f32q q_from_m3 = f32q(m3);
    REQUIRE(q_from_m3.w == Catch::Approx(q.w));
    REQUIRE(q_from_m3.x == Catch::Approx(q.x));
    REQUIRE(q_from_m3.y == Catch::Approx(q.y));
    REQUIRE(q_from_m3.z == Catch::Approx(q.z));

    // Basic mat4 structure
    REQUIRE(m4[3][3] == Catch::Approx(1.0f));
    REQUIRE(m4[0][3] == Catch::Approx(0.0f));
}

TEST_CASE("Quaternion angle and axis extraction", "[Quaternion]")
{
    const f32v3 axis(1.0f, 0.0f, 0.0f);
    const f32 angle = Pi<f32>() * 0.5f;

    const f32q q = f32q::FromAngleAxis(angle, axis);

    const f32 a = GetAngle(q);
    const f32v3 ax = GetAxis(q);

    REQUIRE(a == Catch::Approx(angle));
    REQUIRE(ax[0] == Catch::Approx(axis[0]));
    REQUIRE(ax[1] == Catch::Approx(axis[1]));
    REQUIRE(ax[2] == Catch::Approx(axis[2]));
}
