// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Vector4.hpp>
#include <snitch/snitch.hpp>

using cer::Vector2;
using cer::Vector3;
using cer::Vector4;

TEST_CASE("Vector4", "[math]")
{
    SECTION("construction")
    {
        REQUIRE(Vector4{} == Vector4{0, 0, 0, 0});
        REQUIRE(Vector4{} == Vector4{0});

        {
            constexpr auto v = Vector4{1, 2, 3, 4};
            REQUIRE(v.x == 1.0f);
            REQUIRE(v.y == 2.0f);
            REQUIRE(v.z == 3.0f);
            REQUIRE(v.w == 4.0f);
        }

        REQUIRE(Vector4{Vector2{1, 2}, Vector2{3, 4}} == Vector4{1, 2, 3, 4});
        REQUIRE(Vector4{Vector3{1, 2, 3}, 4} == Vector4{1, 2, 3, 4});
    }

    SECTION("clamp")
    {
        REQUIRE(clamp(Vector4{}, {}, {}) == Vector4{});
        REQUIRE(clamp(Vector4{1, 2, 3, 4}, {1.5f, 2, 3, 4}, {2, 3, 4, 4}) ==
                Vector4{1.5f, 2, 3, 4});
    }

    SECTION("is_zero")
    {
        REQUIRE(is_zero(Vector4{}));
        REQUIRE_FALSE(is_zero(Vector4{1, 0, 0, 0}));
        REQUIRE_FALSE(is_zero(Vector4{0, 1, 0, 0}));
        REQUIRE_FALSE(is_zero(Vector4{0, 0, 1, 0}));
        REQUIRE_FALSE(is_zero(Vector4{0, 0, 0, 1}));
    }

    SECTION("are_equal_within")
    {
        REQUIRE(are_equal_within(Vector4{}, Vector4{}));
        REQUIRE(are_equal_within(Vector4{0.1f, 0.2f, 0.3f, 0.4f}, Vector4{0.1f, 0.2f, 0.3f, 0.4f}));
        REQUIRE(are_equal_within(Vector4{0.4f, 0.3f, 0.2f, 0.1f}, Vector4{0.4f, 0.3f, 0.2f, 0.1f}));
        REQUIRE_FALSE(
            are_equal_within(Vector4{0.1f, 0.2f, 0.3f, 0.4f}, Vector4{0.4f, 0.3f, 0.2f, 0.1f}));
        REQUIRE(are_equal_within(Vector4{}, Vector4{std::numeric_limits<float>::epsilon()}));
        REQUIRE_FALSE(
            are_equal_within(Vector4{}, Vector4{std::numeric_limits<float>::epsilon() + 0.001f}));
        REQUIRE(are_equal_within(Vector4{1, 2, 3, 4}, Vector4{1.2f, 2.3f, 3.4f, 4.45f}, 0.5f));
    }

    SECTION("normalize")
    {
        REQUIRE(is_zero(normalize(Vector4{})));
        REQUIRE(normalize(Vector4{1}) == Vector4{0.5f});
        REQUIRE(normalize(Vector4{2}) == Vector4{0.5f});
        REQUIRE(normalize(Vector4{1, 2, 3, 4}) ==
                Vector4{0.18257418f, 0.36514837f, 0.5477225f, 0.73029673f});
    }

    SECTION("dot")
    {
        REQUIRE(cer::is_zero(cer::dot(Vector4{}, Vector4{})));
        REQUIRE(cer::dot(Vector4{}, {1, 0, 0, 0}) == 0.0f);
        REQUIRE(cer::dot(Vector4{}, {0, 1, 0, 0}) == 0.0f);
        REQUIRE(cer::dot(Vector4{}, {0, 0, 1, 0}) == 0.0f);
        REQUIRE(cer::dot(Vector4{}, {0, 0, 0, 1}) == 0.0f);
        REQUIRE(cer::dot(Vector4{}, {0.5f, 0.5f, 0.5f, 0.5f}) == 0.0f);
        REQUIRE(cer::dot(Vector4{0.5f, 0.5f, 0.5f, 0.5f}, {1, 0, 0, 0}) == 0.5f);
        REQUIRE(cer::dot(Vector4{0.5f, 0.5f, 0.5f, 0.5f}, {0, 1, 0, 0}) == 0.5f);
    }

    SECTION("length")
    {
        REQUIRE(cer::is_zero(cer::length(Vector4{})));
        REQUIRE(cer::length(Vector4{1, 0, 0, 0}) == 1.0f);
        REQUIRE(cer::length(Vector4{0, 1, 0, 0}) == 1.0f);
        REQUIRE(cer::length(Vector4{1, 1, 0, 0}) == std::sqrt(2.0f));
        REQUIRE(cer::length(Vector4{0.45f, 0, 0, 0}) == 0.45f);
    }

    SECTION("length_squared")
    {
        REQUIRE(cer::is_zero(cer::length_squared(Vector4{})));
        REQUIRE(length_squared(Vector4{1, 0, 0, 0}) == 1.0f);
        REQUIRE(length_squared(Vector4{0, 1, 0, 0}) == 1.0f);
        REQUIRE(length_squared(Vector4{1, 1, 1, 1}) == 4.0f);
        REQUIRE(length_squared(Vector4{0.45f, 0, 0, 0}) == 0.45f * 0.45f);
    }

    SECTION("min")
    {
        REQUIRE(cer::min(Vector4{}, Vector4{}) == Vector4{});
        REQUIRE(cer::min(Vector4{}, {1, 2, 3, 4}) == Vector4{});
        REQUIRE(cer::min({1, 2, 3, 4}, {1.5f, 2, 3, 4}) == Vector4{1, 2, 3, 4});
        REQUIRE(cer::min({1, 2, 3, 4}, {1.5f, 1.25f, 1.75f, 2}) == Vector4{1, 1.25f, 1.75f, 2});
        REQUIRE(cer::min({-0.5f, 0.5f, -0.5f, 0.2f}, {0.5f, -3.5f, 0, 0.3f}) ==
                Vector4{-0.5f, -3.5f, -0.5f, 0.2f});
    }

    SECTION("max")
    {
        REQUIRE(cer::max(Vector4{}, Vector4{}) == Vector4{});
        REQUIRE(cer::max(Vector4{}, {1, 2, 3, 4}) == Vector4{1, 2, 3, 4});
        REQUIRE(cer::max({1, 2, 3, 4}, {1.5f, 2, 3, 4}) == Vector4{1.5f, 2, 3, 4});
        REQUIRE(cer::max({1, 2, 3, 4}, {1.5f, 1.25f, 1.75f, 2}) == Vector4{1.5f, 2, 3, 4});
        REQUIRE(cer::max({-0.5f, 0.5f, -0.5f, 0.2f}, {0.5f, -3.5f, 0, 0.3f}) ==
                Vector4{0.5f, 0.5f, 0, 0.3f});
    }

    SECTION("operator+")
    {
        REQUIRE(Vector4{} + Vector4{} == Vector4{});
        REQUIRE(Vector4{1, 2, 3, 4} + Vector4{0.5f, 0.25f, 0.75f, 1} ==
                Vector4{1.5f, 2.25f, 3.75f, 5});
        REQUIRE(Vector4{-0.5f, -0.25f, -0.75f, -1} + Vector4{-0.5f, 3.5f, 1.65f, 2} ==
                Vector4{-1, 3.25f, 0.9f, 1});
    }

    SECTION("operator-")
    {
        REQUIRE(Vector4{} - Vector4{} == Vector4{});
        REQUIRE(Vector4{1, 2, 3, 4} - Vector4{0.5f, 0.25f, 0.75f, 1} ==
                Vector4{0.5f, 1.75f, 2.25f, 3});
        REQUIRE(Vector4{-0.5f, -0.25f, -0.75f, -1} - Vector4{-0.5f, 3.5f, 1.65f, 2} ==
                Vector4{0, -3.75f, -2.4f, -3});
    }

    SECTION("operator*")
    {
        REQUIRE(Vector4{} * Vector4{} == Vector4{});
        REQUIRE(Vector4{1, 2, 3, 4} * Vector4{0.5f, 0.25f, 0.75f, 1} ==
                Vector4{0.5f, 0.5f, 2.25f, 4});
        REQUIRE(Vector4{-0.5f, -0.25f, -0.75f, -1} * Vector4{-0.5f, 3.5f, 1.65f, 2} ==
                Vector4{0.25f, -0.875f, -1.2375f, -2});
        REQUIRE(Vector4{} * 1 == Vector4{});
        REQUIRE(Vector4{1, 2, 3, 4} * 2 == Vector4{2, 4, 6, 8});
        REQUIRE(1.0f * Vector4{} == Vector4{});
        REQUIRE(2.0f * Vector4{1, 2, 3, 4} == Vector4{2, 4, 6, 8});
    }

    SECTION("operator/")
    {
        REQUIRE(Vector4{} / Vector4{0.001f} == Vector4{});
        REQUIRE(Vector4{1, 2, 3, 4} / Vector4{0.5f, 0.25f, 0.75f, 1} == Vector4{2, 8, 4, 4});
        REQUIRE(Vector4{-0.5f, -0.25f, -0.75f, -1} / Vector4{-0.5f, 3.5f, 1.65f, 2} ==
                Vector4{1, -0.25f / 3.5f, -0.75f / 1.65f, -0.5f});
        REQUIRE(Vector4{} / 1 == Vector4{});
        REQUIRE(Vector4{1, 2, 3, 4} / 2 == Vector4{0.5f, 1, 1.5f, 2});
    }

    SECTION("operator+=")
    {
        auto v = Vector4{};
        v += Vector4{};
        REQUIRE(v == Vector4{});

        v = Vector4{1, 2, 3, 4};
        v += Vector4{0.5f, 0.25f, 0.75f, 1};
        REQUIRE(v == Vector4{1.5f, 2.25f, 3.75f, 5});

        v = Vector4{-0.5f, -0.25f, -0.75f, -1};
        v += Vector4{-0.5f, 3.5f, 1.65f, 2};
        REQUIRE(v == Vector4{-1, 3.25f, 0.9f, 1});
    }

    SECTION("operator-=")
    {
        auto v = Vector4{};
        v += Vector4{};
        REQUIRE(v == Vector4{});

        v = Vector4{1, 2, 3, 4};
        v -= Vector4{0.5f, 0.25f, 0.75f, 1};
        REQUIRE(v == Vector4{0.5f, 1.75f, 2.25f, 3});

        v = Vector4{-0.5f, -0.25f, -0.75f, -1};
        v -= Vector4{-0.5f, 3.5f, 1.65f, 2};
        REQUIRE(v == Vector4{0, -3.75f, -2.4f, -3});
    }

    SECTION("operator*=")
    {
        auto v = Vector4{};
        v *= Vector4{};
        REQUIRE(v == Vector4{});

        v = Vector4{1, 2, 3, 4};
        v *= Vector4{0.5f, 0.25f, 0.75f, 1};
        REQUIRE(v == Vector4{0.5f, 0.5f, 2.25f, 4});

        v = Vector4{-0.5f, -0.25f, -0.75f, -1};
        v *= Vector4{-0.5f, 3.5f, 1.65f, 2};
        REQUIRE(v == Vector4{0.25f, -0.875f, -1.2375f, -2});

        v = Vector4{};
        v *= 1.0f;
        REQUIRE(v == Vector4{});

        v = Vector4{1, 2, 3, 4};
        v *= 2.0f;
        REQUIRE(v == Vector4{2, 4, 6, 8});
    }

    SECTION("operator/=")
    {
        auto v = Vector4{};
        v /= Vector4{0.001f};
        REQUIRE(v == Vector4{});

        v = Vector4{1, 2, 3, 4};
        v /= Vector4{0.5f, 0.25f, 0.75f, 1};
        REQUIRE(v == Vector4{2, 8, 4, 4});

        v = Vector4{-0.5f, -0.25f, -0.75f, -1};
        v /= Vector4{-0.5f, 3.5f, 1.65f, 2};
        REQUIRE(v == Vector4{1, -0.25f / 3.5f, -0.75f / 1.65f, -0.5f});

        v = Vector4{};
        v /= 1.0f;
        REQUIRE(v == Vector4{});

        v = Vector4{1, 2, 3, 4};
        v /= 2.0f;
        REQUIRE(v == Vector4{0.5f, 1, 1.5f, 2});
    }

    SECTION("unary operator-")
    {
        REQUIRE(-Vector4{} == Vector4{});
        REQUIRE(-Vector4{1, 2, 3, 4} == Vector4{-1, -2, -3, -4});
        REQUIRE(-Vector4{-1.0, -2, -3, -4} == Vector4{1, 2, 3, 4});
    }
}
