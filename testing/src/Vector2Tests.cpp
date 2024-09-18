// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Vector2.hpp>
#include <snitch/snitch.hpp>

using namespace cer;

TEST_CASE("Vector2", "[math]")
{
    SECTION("construction")
    {
        REQUIRE(Vector2() == Vector2(0, 0));
        REQUIRE(Vector2() == Vector2(0));

        {
            constexpr Vector2 v{1, 2};
            REQUIRE(v.x == 1.0f);
            REQUIRE(v.y == 2.0f);
        }
    }

    SECTION("clamp")
    {
        REQUIRE(clamp(Vector2{}, {}, {}) == Vector2());
        REQUIRE(clamp(Vector2{1, 2}, {1.5f, 2}, {2, 3}) == Vector2(1.5f, 2));
    }

    SECTION("is_zero")
    {
        REQUIRE(is_zero(Vector2{}));
        REQUIRE_FALSE(is_zero(Vector2{1, 0}));
        REQUIRE_FALSE(is_zero(Vector2{0, 1}));
    }

    SECTION("are_equal_within")
    {
        REQUIRE(are_equal_within({}, {}));
        REQUIRE(are_equal_within({0.1f, 0.2f}, {0.1f, 0.2f}));
        REQUIRE(are_equal_within({0.2f, 0.1f}, {0.2f, 0.1f}));
        REQUIRE_FALSE(are_equal_within({0.1f, 0.2f}, {0.2f, 0.1f}));
        REQUIRE(are_equal_within({}, std::numeric_limits<Vector2>::epsilon()));
        REQUIRE_FALSE(
            are_equal_within(Vector2(), Vector2(std::numeric_limits<float>::epsilon() + 0.001f)));
        REQUIRE(are_equal_within(Vector2(1, 2), Vector2(1.2f, 2.3f), 0.5f));
    }

    SECTION("normalized")
    {
        REQUIRE(is_zero(normalize(Vector2())));
        REQUIRE(are_equal_within(normalize(Vector2(1)), Vector2(std::sqrt(0.5f))));
        REQUIRE(are_equal_within(normalize(Vector2(2)), Vector2(std::sqrt(0.5f))));
        REQUIRE(are_equal_within(normalize(Vector2(1, 2)), {0.4472136f, 0.8944272f}));
    }

    SECTION("dot")
    {
        REQUIRE(is_zero(dot(Vector2(), {})));
        REQUIRE(is_zero(dot(Vector2(), {1, 0})));
        REQUIRE(is_zero(dot(Vector2(), {0, 1})));
        REQUIRE(is_zero(dot(Vector2(), {0.5f, 0.5f})));
        REQUIRE(dot(Vector2(0.5f), {1, 0}) == 0.5f);
        REQUIRE(dot(Vector2(0.5f), {0, 1}) == 0.5f);
    }

    SECTION("length")
    {
        REQUIRE(length(Vector2()) == 0.0f);
        REQUIRE(length(Vector2(1, 0)) == 1.0f);
        REQUIRE(length(Vector2(0, 1)) == 1.0f);
        REQUIRE(length(Vector2(1, 1)) == std::sqrt(2.0f));
        REQUIRE(length(Vector2(0.45f, 0)) == 0.45f);
    }

    SECTION("length_squared")
    {
        REQUIRE(length_squared(Vector2()) == 0.0f);
        REQUIRE(length_squared(Vector2(1, 0)) == 1.0f);
        REQUIRE(length_squared(Vector2(0, 1)) == 1.0f);
        REQUIRE(length_squared(Vector2(1, 1)) == 2.0f);
        REQUIRE(length_squared(Vector2(0.45f, 0)) == 0.45f * 0.45f);
    }

    SECTION("min")
    {
        REQUIRE(min({}, {}) == Vector2());
        REQUIRE(min({}, {1, 2}) == Vector2());
        REQUIRE(min({1, 2}, {1.5f, 2}) == Vector2(1, 2));
        REQUIRE(min({1, 2}, {1.5f, 1.25f}) == Vector2(1, 1.25f));
        REQUIRE(min({-0.5f, 0.5f}, {0.5f, -2}) == Vector2(-0.5f, -2));
    }

    SECTION("max")
    {
        REQUIRE(max({}, {}) == Vector2());
        REQUIRE(max({}, {1, 2}) == Vector2(1, 2));
        REQUIRE(max({1, 2}, {1.5f, 2}) == Vector2(1.5f, 2));
        REQUIRE(max({1, 2}, {1.5f, 1.25f}) == Vector2(1.5f, 2));
        REQUIRE(max({-0.5f, 0.5f}, {0.5f, -2}) == Vector2(0.5f, 0.5f));
    }

    SECTION("operator+")
    {
        REQUIRE(Vector2() + Vector2() == Vector2());
        REQUIRE(Vector2(1, 2) + Vector2(0.5f, 0.25f) == Vector2(1.5f, 2.25f));
        REQUIRE(Vector2(-0.5f, -0.25f) + Vector2(-0.5f, 3.5f) == Vector2(-1, 3.25f));
    }

    SECTION("operator-")
    {
        REQUIRE(Vector2() - Vector2() == Vector2());
        REQUIRE(Vector2(1, 2) - Vector2(0.5f, 0.25f) == Vector2(0.5f, 1.75f));
        REQUIRE(Vector2(-0.5f, -0.25f) - Vector2(-0.5f, 3.5f) == Vector2(0, -3.75f));
    }

    SECTION("operator*")
    {
        REQUIRE(Vector2() * Vector2() == Vector2());
        REQUIRE(Vector2(1, 2) * Vector2(0.5f, 0.25f) == Vector2(0.5f, 0.5f));
        REQUIRE(Vector2(-0.5f, -0.25f) * Vector2(-0.5f, 3.5f) == Vector2(0.25f, -0.875f));
        REQUIRE(Vector2() * 1.0f == Vector2());
        REQUIRE(Vector2(1, 2) * 2.0f == Vector2(2, 4));
        REQUIRE(1.0f * Vector2() == Vector2());
        REQUIRE(2.0f * Vector2(1, 2) == Vector2(2, 4));
    }

    SECTION("operator/")
    {
        REQUIRE(Vector2() / Vector2(0.001f) == Vector2());
        REQUIRE(Vector2(1, 2) / Vector2(0.5f, 0.25f) == Vector2(2, 8));
        REQUIRE(Vector2(-0.5f, -0.25f) / Vector2(-0.5f, 3.5f) == Vector2(1, -0.071428575f));
        REQUIRE(Vector2() / 1.0f == Vector2());
        REQUIRE(Vector2(1, 2) / 2.0f == Vector2(0.5f, 1));
    }

    SECTION("operator+=")
    {
        auto v = Vector2();
        v += Vector2();
        REQUIRE(v == Vector2());

        v = Vector2(1, 2);
        v += Vector2(0.5f, 0.25f);
        REQUIRE(v == Vector2(1.5f, 2.25f));

        v = Vector2(-0.5f, -0.25f);
        v += Vector2(-0.5f, 3.5f);
        REQUIRE(v == Vector2(-1, 3.25f));
    }

    SECTION("operator-=")
    {
        auto v = Vector2();
        v += Vector2();
        REQUIRE(v == Vector2());

        v = Vector2(1, 2);
        v -= Vector2(0.5f, 0.25f);
        REQUIRE(v == Vector2(0.5f, 1.75f));

        v = Vector2(-0.5f, -0.25f);
        v -= Vector2(-0.5f, 3.5f);
        REQUIRE(v == Vector2(0, -3.75f));
    }

    SECTION("operator*=")
    {
        auto v = Vector2();
        v *= Vector2();
        REQUIRE(v == Vector2());

        v = Vector2(1, 2);
        v *= Vector2(0.5f, 0.25f);
        REQUIRE(v == Vector2(0.5f, 0.5f));

        v = Vector2(-0.5f, -0.25f);
        v *= Vector2(-0.5f, 3.5f);
        REQUIRE(v == Vector2(0.25f, -0.875f));

        v = Vector2();
        v *= 1.0f;
        REQUIRE(v == Vector2());

        v = Vector2(1, 2);
        v *= 2.0f;
        REQUIRE(v == Vector2(2, 4));
    }

    SECTION("operator/=")
    {
        auto v = Vector2();
        v /= Vector2(0.001f);
        REQUIRE(v == Vector2());

        v = Vector2(1, 2);
        v /= Vector2(0.5f, 0.25f);
        REQUIRE(v == Vector2(2, 8));

        v = Vector2(-0.5f, -0.25f);
        v /= Vector2(-0.5f, 3.5f);
        REQUIRE(v == Vector2(1, -0.071428575f));

        v = Vector2();
        v /= 1.0f;
        REQUIRE(v == Vector2());

        v = Vector2(1, 2);
        v /= 2.0f;
        REQUIRE(v == Vector2(0.5f, 1));
    }

    SECTION("unary operator-")
    {
        REQUIRE(-Vector2() == Vector2());
        REQUIRE(-Vector2(1, 2) == Vector2(-1, -2));
        REQUIRE(-Vector2(-1.0, -2) == Vector2(1, 2));
    }
}
