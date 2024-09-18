// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Math.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <snitch/snitch.hpp>

using namespace cer;

TEST_CASE("Math", "[math]")
{
    SECTION("remap")
    {
        REQUIRE(remap(0.0f, 1.0f, 0.0f, 100.0f, 0.0f) == 0.0f);
        REQUIRE(remap(0.0f, 1.0f, 0.0f, 100.0f, 0.5f) == 50.0f);
        REQUIRE(remap(0.0f, 1.0f, 0.0f, 100.0f, 1.0f) == 100.0f);
    }

    SECTION("min")
    {
        REQUIRE(min(0.0f, 0.0f) == 0.0f);
        REQUIRE(min(0.0f, 1.0f) == 0.0f);
        REQUIRE(min(1.0f, 0.0f) == 0.0f);
        REQUIRE(min(-1.0f, 0.0f) == -1.0f);
    }

    SECTION("max")
    {
        REQUIRE(max(0.0f, 0.0f) == 0.0f);
        REQUIRE(max(0.0f, 1.0f) == 1.0f);
        REQUIRE(max(1.0f, 0.0f) == 1.0f);
        REQUIRE(max(-1.0f, 0.0f) == 0.0f);
    }

    SECTION("abs")
    {
        REQUIRE(abs(0.0f) == 0.0f);
        REQUIRE(abs(1.0f) == 1.0f);
        REQUIRE(abs(-1.0f) == 1.0f);
    }

    SECTION("radians")
    {
        REQUIRE(radians(0.0f) == 0.0f);
        REQUIRE(radians(90.0f) == float(half_pi));
        REQUIRE(radians(180.0f) == float(pi));
        REQUIRE(radians(360.0f) == float(two_pi));
    }

    SECTION("degrees")
    {
        REQUIRE(degrees(0.0f) == 0.0f);
        REQUIRE(degrees(half_pi) == 90.0f);
        REQUIRE(degrees(pi) == 180.0f);
        REQUIRE(degrees(two_pi) == 360.0f);
    }

    SECTION("is_zero")
    {
        REQUIRE(is_zero(0.0f));
        REQUIRE_FALSE(is_zero(1.0f));
        REQUIRE_FALSE(is_zero(-1.0f));
    }

    SECTION("are_numbers_within_epsilon")
    {
        REQUIRE(equal_within_epsilon(0.0f, 0.0f));
        REQUIRE(equal_within_epsilon(1.0f, 1.0f));
        REQUIRE_FALSE(equal_within_epsilon(0.0f, 0.1f));
    }

    SECTION("are_numbers_equal_within")
    {
        REQUIRE(equal_within(1.5f, 1.6f, 0.105f));
        REQUIRE(equal_within(-1.5f, -1.7f, 0.21f));
        REQUIRE_FALSE(equal_within(-1.5f, -1.7f, 0.19f));
    }

    SECTION("distance")
    {
        REQUIRE(distance(0.0f, 0.0f) == 0.0f);
        REQUIRE(distance(0.0f, 1.0f) == 1.0f);
        REQUIRE(distance(-1.0f, 1.0f) == 2.0f);
    }

    SECTION("lerp")
    {
        REQUIRE(lerp(0.0f, 0.0f, 0.0f) == 0.0f);
        REQUIRE(lerp(0.0f, 1.0f, 0.0f) == 0.0f);
        REQUIRE(lerp(0.0f, 1.0f, 0.5f) == 0.5f);
        REQUIRE(lerp(0.0f, 1.0f, 1.0f) == 1.0f);
        REQUIRE(lerp(0.0f, 100.0f, 0.0f) == 0.0f);
        REQUIRE(lerp(0.0f, 100.0f, 0.5f) == 50.0f);
        REQUIRE(lerp(0.0f, 100.0f, 1.0f) == 100.0f);
    }

    SECTION("inverse_lerp")
    {
        REQUIRE(inverse_lerp(100.0f, 300.0f, 200.0f) == 0.5f);
    }

    SECTION("mipmap_extent")
    {
        REQUIRE(mipmap_extent(0, 0) == 0u);
        REQUIRE(mipmap_extent(128, 0) == 128u);
        REQUIRE(mipmap_extent(128, 1) == 64u);
        REQUIRE(mipmap_extent(128, 2) == 32u);
        REQUIRE(mipmap_extent(128, 3) == 16u);
        REQUIRE(mipmap_extent(128, 4) == 8u);
        REQUIRE(mipmap_extent(128, 5) == 4u);
        REQUIRE(mipmap_extent(128, 6) == 2u);
        REQUIRE(mipmap_extent(128, 7) == 1u);
        REQUIRE(mipmap_extent(128, 8) == 1u);
    }

    SECTION("max_mipmap_count_for_extent")
    {
        REQUIRE(max_mipmap_count_for_extent(0) == 0u);
        REQUIRE(max_mipmap_count_for_extent(1) == 1u);
        REQUIRE(max_mipmap_count_for_extent(64) == 7u);
    }
}
