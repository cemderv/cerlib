// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Color.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Vector3.hpp>
#include <cerlib/Vector4.hpp>
#include <snitch/snitch.hpp>

using cer::Color;

TEST_CASE("Color", "[math]")
{
    SECTION("Construction")
    {
        REQUIRE(Color{} == Color{0, 0, 0, 0});

        {
            constexpr auto c = Color{1, 2, 3, 4};
            REQUIRE(c.r == 1.0f);
            REQUIRE(c.g == 2.0f);
            REQUIRE(c.b == 3.0f);
            REQUIRE(c.a == 4.0f);
        }

        REQUIRE(Color{1, 2, 3, 1} == Color{1, 2, 3, 1});
    }

    SECTION("Built-in colors")
    {
        REQUIRE(cer::white == Color{1, 1, 1, 1});
        REQUIRE(cer::black == Color{0, 0, 0, 1});
        REQUIRE(cer::red == Color{1, 0, 0, 1});
        REQUIRE(cer::green == Color{0, 0.5f, 0, 1});
        REQUIRE(cer::blue == Color{0, 0, 1, 1});
        REQUIRE(cer::cornflowerblue == Color{100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1});
    }

    SECTION("Properties")
    {
        REQUIRE(Color{1, 2, 3, 1}.to_vector3() == cer::Vector3{1, 2, 3});
        REQUIRE(Color{1, 2, 3, 1}.to_vector4() == cer::Vector4{1, 2, 3, 1});
    }
}
