// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Vector2.hpp>
#include <cerlib/Vector3.hpp>
#include <snitch/snitch.hpp>

using namespace cer;

TEST_CASE("Vector3", "[math]")
{
    SECTION("construction")
    {
        REQUIRE(Vector3() == Vector3(0, 0, 0));
        REQUIRE(Vector3() == Vector3(0));

        {
            constexpr Vector3 v{1, 2, 3};
            REQUIRE(v.x == 1.0f);
            REQUIRE(v.y == 2.0f);
            REQUIRE(v.z == 3.0f);
        }
    }

    SECTION("clamp")
    {
        REQUIRE(clamp(Vector3(), {}, {}) == Vector3());
        REQUIRE(clamp(Vector3(1, 2, 3), {1.5f, 2, 3}, {2, 3, 4}) == Vector3(1.5f, 2, 3));
    }

    SECTION("is_zero")
    {
        REQUIRE(is_zero(Vector3()));
        REQUIRE_FALSE(is_zero(Vector3(1, 0, 0)));
        REQUIRE_FALSE(is_zero(Vector3(0, 1, 0)));
        REQUIRE_FALSE(is_zero(Vector3(0, 0, 1)));
    }

    SECTION("are_equal_within")
    {
        REQUIRE(are_equal_within(Vector3(), Vector3()));
        REQUIRE(are_equal_within({0.1f, 0.2f, 0.3f}, {0.1f, 0.2f, 0.3f}));
        REQUIRE(are_equal_within({0.3f, 0.2f, 0.1f}, {0.3f, 0.2f, 0.1f}));

        REQUIRE_FALSE(are_equal_within({0.1f, 0.2f, 0.3f}, {0.3f, 0.2f, 0.1f}));

        REQUIRE(are_equal_within(Vector3(), Vector3(std::numeric_limits<float>::epsilon())));

        REQUIRE_FALSE(
            are_equal_within({}, Vector3(std::numeric_limits<float>::epsilon() + 0.001f)));

        REQUIRE(are_equal_within({1, 2, 3}, {1.2f, 2.3f, 3.4f}, 0.5f));
    }

    SECTION("normalize")
    {
        REQUIRE(is_zero(normalize(Vector3())));
        REQUIRE(normalize(Vector3(1)) == Vector3(std::sqrt(3.0f) / 3.0f));
        REQUIRE(normalize(Vector3(2)) == Vector3(std::sqrt(3.0f) / 3.0f));
        REQUIRE(normalize(Vector3(1, 2, 3)) == Vector3(0.26726124f, 0.5345225f, 0.8017837f));
    }

    SECTION("dot")
    {
        REQUIRE(is_zero(dot(Vector3(), {})));
        REQUIRE(is_zero(dot(Vector3(), {1, 0, 0})));
        REQUIRE(is_zero(dot(Vector3(), {0, 1, 0})));
        REQUIRE(is_zero(dot(Vector3(), {0.5f, 0.5f, 0.5f})));
        REQUIRE(dot(Vector3(0.5f, 0.5f, 0.5f), {1, 0, 0}) == 0.5f);
        REQUIRE(dot(Vector3(0.5f, 0.5f, 0.5f), {0, 1, 0}) == 0.5f);
    }

    SECTION("length")
    {
        REQUIRE(length(Vector3()) == 0.0f);
        REQUIRE(length(Vector3(1, 0, 0)) == 1.0f);
        REQUIRE(length(Vector3(0, 1, 0)) == 1.0f);
        REQUIRE(length(Vector3(1, 1, 0)) == std::sqrt(2.0f));
        REQUIRE(length(Vector3(0.45f, 0, 0)) == 0.45f);
    }

    SECTION("length_squared")
    {
        REQUIRE(length_squared(Vector3()) == 0.0f);
        REQUIRE(length_squared(Vector3(1, 0, 0)) == 1.0f);
        REQUIRE(length_squared(Vector3(0, 1, 0)) == 1.0f);
        REQUIRE(length_squared(Vector3(1, 1, 1)) == 3.0f);
        REQUIRE(length_squared(Vector3(0.45f, 0, 0)) == 0.45f * 0.45f);
    }

    SECTION("min")
    {
        REQUIRE(min(Vector3(), Vector3()) == Vector3());
        REQUIRE(min(Vector3(), {1, 2, 3}) == Vector3());
        REQUIRE(min({1, 2, 3}, {1.5f, 2, 3}) == Vector3(1, 2, 3));
        REQUIRE(min({1, 2, 3}, {1.5f, 1.25f, 1.75f}) == Vector3(1, 1.25f, 1.75f));
        REQUIRE(min({-0.5f, 0.5f, -0.5f}, {0.5f, -3.5f, 0}) == Vector3(-0.5f, -3.5f, -0.5f));
    }

    SECTION("max")
    {
        REQUIRE(max(Vector3(), Vector3()) == Vector3());
        REQUIRE(max(Vector3(), {1, 2, 3}) == Vector3(1, 2, 3));
        REQUIRE(max({1, 2, 3}, {1.5f, 2, 3.5f}) == Vector3(1.5f, 2, 3.5f));
        REQUIRE(max({1, 2, 3}, {1.5f, 1.25f, 1.75f}) == Vector3(1.5f, 2, 3));
        REQUIRE(max({-0.5f, 0.5f, -0.5f}, {0.5f, -2, 0}) == Vector3(0.5f, 0.5f, 0));
    }

    SECTION("operator+")
    {
        REQUIRE(Vector3() + Vector3() == Vector3());
        REQUIRE(Vector3(1, 2, 3) + Vector3(0.5f, 0.25f, 0.75f) == Vector3(1.5f, 2.25f, 3.75f));
        REQUIRE(Vector3(-0.5f, -0.25f, -0.75f) + Vector3(-0.5f, 3.5f, 1.65f) ==
                Vector3(-1, 3.25f, 0.9f));
    }

    SECTION("operator-")
    {
        REQUIRE(Vector3() - Vector3() == Vector3());
        REQUIRE(Vector3(1, 2, 3) - Vector3(0.5f, 0.25f, 0.75f) == Vector3(0.5f, 1.75f, 2.25f));
        REQUIRE(Vector3(-0.5f, -0.25f, -0.75f) - Vector3(-0.5f, 3.5f, 1.65f) ==
                Vector3(0, -3.75f, -2.4f));
    }

    SECTION("operator*")
    {
        REQUIRE(Vector3() * Vector3() == Vector3());
        REQUIRE(Vector3(1, 2, 3) * Vector3(0.5f, 0.25f, 0.75f) == Vector3(0.5f, 0.5f, 2.25f));
        REQUIRE(Vector3(-0.5f, -0.25f, -0.75) * Vector3(-0.5f, 3.5f, 1.65f) ==
                Vector3(0.25f, -0.875f, -1.2375f));
        REQUIRE(Vector3() * 1.0f == Vector3());
        REQUIRE(Vector3(1, 2, 3) * 2.0f == Vector3(2, 4, 6));
        REQUIRE(1.0f * Vector3() == Vector3());
        REQUIRE(2.0f * Vector3(1, 2, 3) == Vector3(2, 4, 6));
    }

    SECTION("operator/")
    {
        REQUIRE(Vector3() / Vector3(0.001f) == Vector3());
        REQUIRE(Vector3(1, 2, 3) / Vector3(0.5f, 0.25f, 0.75f) == Vector3(2, 8, 4));
        REQUIRE(Vector3(-0.5f, -0.25f, -0.75f) / Vector3(-0.5f, 3.5f, 1.65f) ==
                Vector3(1, -0.071428575f, -0.45454547f));
        REQUIRE(Vector3() / 1.0f == Vector3());
        REQUIRE(Vector3(1, 2, 3) / 2.0f == Vector3(0.5f, 1, 1.5f));
    }

    SECTION("operator+=")
    {
        auto v = Vector3();
        v += Vector3();
        REQUIRE(v == Vector3());

        v = Vector3(1, 2, 3);
        v += Vector3(0.5f, 0.25f, 0.75f);
        REQUIRE(v == Vector3(1.5f, 2.25f, 3.75f));

        v = Vector3(-0.5f, -0.25f, -0.75f);
        v += Vector3(-0.5f, 3.5f, 1.65f);
        REQUIRE(v == Vector3(-1, 3.25f, 0.9f));
    }

    SECTION("operator-=")
    {
        auto v = Vector3();
        v += Vector3();
        REQUIRE(v == Vector3());

        v = Vector3(1, 2, 3);
        v -= Vector3(0.5f, 0.25f, 0.75f);
        REQUIRE(v == Vector3(0.5f, 1.75f, 2.25f));

        v = Vector3(-0.5f, -0.25f, -0.75f);
        v -= Vector3(-0.5f, 3.5f, 1.65f);
        REQUIRE(v == Vector3(0, -3.75f, -2.4f));
    }

    SECTION("operator*=")
    {
        auto v = Vector3();
        v *= Vector3();
        REQUIRE(v == Vector3());

        v = Vector3(1, 2, 3);
        v *= Vector3(0.5f, 0.25f, 0.75f);
        REQUIRE(v == Vector3(0.5f, 0.5f, 2.25f));

        v = Vector3(-0.5f, -0.25f, -0.75f);
        v *= Vector3(-0.5f, 3.5f, 1.65f);
        REQUIRE(v == Vector3(0.25f, -0.875f, -1.2375f));

        v = Vector3();
        v *= 1.0f;
        REQUIRE(v == Vector3());

        v = Vector3(1, 2, 3);
        v *= 2.0f;
        REQUIRE(v == Vector3(2, 4, 6));
    }

    SECTION("operator/=")
    {
        auto v = Vector3();
        v /= Vector3(0.001f);
        REQUIRE(v == Vector3());

        v = Vector3(1, 2, 3);
        v /= Vector3(0.5f, 0.25f, 0.75f);
        REQUIRE(v == Vector3(2.0f, 8.0f, 4.0f));

        v = Vector3(-0.5f, -0.25f, -0.75f);
        v /= Vector3(-0.5f, 3.5f, 1.65f);
        REQUIRE(v == Vector3(1.0f, -0.071428575f, -0.45454547f));

        v = Vector3();
        v /= 1.0f;
        REQUIRE(v == Vector3());

        v = Vector3(1, 2, 3);
        v /= 2.0f;
        REQUIRE(v == Vector3(0.5f, 1, 1.5f));
    }

    SECTION("unary operator-")
    {
        REQUIRE(-Vector3() == Vector3());
        REQUIRE(-Vector3(1, 2, 3) == Vector3(-1, -2, -3));
        REQUIRE(-Vector3(-1.0, -2, -3) == Vector3(1, 2, 3));
    }
}
