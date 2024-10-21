// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Math.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <snitch/snitch.hpp>

TEST_CASE("Math", "[math]")
{
    SECTION("remap")
    {
        REQUIRE(cer::remap(0.0f, 1.0f, 0.0f, 100.0f, 0.0f) == 0.0f);
        REQUIRE(cer::remap(0.0f, 1.0f, 0.0f, 100.0f, 0.5f) == 50.0f);
        REQUIRE(cer::remap(0.0f, 1.0f, 0.0f, 100.0f, 1.0f) == 100.0f);
    }

    SECTION("min")
    {
        REQUIRE(cer::min(0.0f, 0.0f) == 0.0f);
        REQUIRE(cer::min(0.0f, 1.0f) == 0.0f);
        REQUIRE(cer::min(1.0f, 0.0f) == 0.0f);
        REQUIRE(cer::min(-1.0f, 0.0f) == -1.0f);
    }

    SECTION("max")
    {
        REQUIRE(cer::max(0.0f, 0.0f) == 0.0f);
        REQUIRE(cer::max(0.0f, 1.0f) == 1.0f);
        REQUIRE(cer::max(1.0f, 0.0f) == 1.0f);
        REQUIRE(cer::max(-1.0f, 0.0f) == 0.0f);
    }

    SECTION("abs")
    {
        REQUIRE(cer::abs(0.0f) == 0.0f);
        REQUIRE(cer::abs(1.0f) == 1.0f);
        REQUIRE(cer::abs(-1.0f) == 1.0f);
    }

    SECTION("radians")
    {
        REQUIRE(cer::radians(0.0f) == 0.0f);
        REQUIRE(cer::radians(90.0f) == float(cer::half_pi));
        REQUIRE(cer::radians(180.0f) == float(cer::pi));
        REQUIRE(cer::radians(360.0f) == float(cer::two_pi));
    }

    SECTION("degrees")
    {
        REQUIRE(cer::degrees(0.0f) == 0.0f);
        REQUIRE(cer::degrees(cer::half_pi) == 90.0f);
        REQUIRE(cer::degrees(cer::pi) == 180.0f);
        REQUIRE(cer::degrees(cer::two_pi) == 360.0f);
    }

    SECTION("is_zero")
    {
        REQUIRE(cer::is_zero(0.0f));
        REQUIRE_FALSE(cer::is_zero(1.0f));
        REQUIRE_FALSE(cer::is_zero(-1.0f));
    }

    SECTION("are_numbers_within_epsilon")
    {
        REQUIRE(cer::equal_within_epsilon(0.0f, 0.0f));
        REQUIRE(cer::equal_within_epsilon(1.0f, 1.0f));
        REQUIRE_FALSE(cer::equal_within_epsilon(0.0f, 0.1f));
    }

    SECTION("are_numbers_equal_within")
    {
        REQUIRE(cer::equal_within(1.5f, 1.6f, 0.105f));
        REQUIRE(cer::equal_within(-1.5f, -1.7f, 0.21f));
        REQUIRE_FALSE(cer::equal_within(-1.5f, -1.7f, 0.19f));
    }

    SECTION("distance")
    {
        REQUIRE(cer::distance(0.0f, 0.0f) == 0.0f);
        REQUIRE(cer::distance(0.0f, 1.0f) == 1.0f);
        REQUIRE(cer::distance(-1.0f, 1.0f) == 2.0f);
    }

    SECTION("lerp")
    {
        REQUIRE(cer::lerp(0.0f, 0.0f, 0.0f) == 0.0f);
        REQUIRE(cer::lerp(0.0f, 1.0f, 0.0f) == 0.0f);
        REQUIRE(cer::lerp(0.0f, 1.0f, 0.5f) == 0.5f);
        REQUIRE(cer::lerp(0.0f, 1.0f, 1.0f) == 1.0f);
        REQUIRE(cer::lerp(0.0f, 100.0f, 0.0f) == 0.0f);
        REQUIRE(cer::lerp(0.0f, 100.0f, 0.5f) == 50.0f);
        REQUIRE(cer::lerp(0.0f, 100.0f, 1.0f) == 100.0f);
    }

    SECTION("inverse_lerp")
    {
        REQUIRE(cer::inverse_lerp(100.0f, 300.0f, 200.0f) == 0.5f);
    }
}
