// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Formatters.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <snitch/snitch.hpp>

using namespace cer;

TEST_CASE("Formatting", "[misc]")
{
    SECTION("Color")
    {
        REQUIRE(std::format("{}", Color()) == "[0; 0; 0; 0]");
        REQUIRE(std::format("{}", Color(1, 2, 3, 4)) == "[1; 2; 3; 4]");
        REQUIRE(std::format("{}", Color(0.25f, 0.5f, 0.75f, 1.0f)) == "[0.25; 0.5; 0.75; 1]");
    }

    SECTION("Vector2")
    {
        REQUIRE(std::format("{}", Vector2()) == "[0; 0]");
        REQUIRE(std::format("{}", Vector2(1, 2)) == "[1; 2]");
        REQUIRE(std::format("{}", Vector2(0.25f, 0.5f)) == "[0.25; 0.5]");
        REQUIRE(std::format("{}", Vector2(-0.25f, -0.5f)) == "[-0.25; -0.5]");
    }

    SECTION("Vector3")
    {
        REQUIRE(std::format("{}", Vector3()) == "[0; 0; 0]");
        REQUIRE(std::format("{}", Vector3(1, 2, 3)) == "[1; 2; 3]");
        REQUIRE(std::format("{}", Vector3(0.25f, 0.5f, 0.75f)) == "[0.25; 0.5; 0.75]");
        REQUIRE(std::format("{}", Vector3(-0.25f, -0.5f, +0.75f)) == "[-0.25; -0.5; 0.75]");
    }

    SECTION("Vector4")
    {
        REQUIRE(std::format("{}", Vector4()) == "[0; 0; 0; 0]");
        REQUIRE(std::format("{}", Vector4(1, 2, 3, 4)) == "[1; 2; 3; 4]");
        REQUIRE(std::format("{}", Vector4(0.25f, 0.5f, 0.75f, 1.0f)) == "[0.25; 0.5; 0.75; 1]");
        REQUIRE(std::format("{}", Vector4(-0.25f, -0.5f, +0.75f, 12.34f)) ==
                "[-0.25; -0.5; 0.75; 12.34]");
    }

    SECTION("Matrix")
    {
        REQUIRE(std::format("{}", Matrix()) == R"([
  1; 0; 0; 0
  0; 1; 0; 0
  0; 0; 1; 0
  0; 0; 0; 1
])");

        REQUIRE(std::format("{}", Matrix(2)) == R"([
  2; 0; 0; 0
  0; 2; 0; 0
  0; 0; 2; 0
  0; 0; 0; 2
])");

        REQUIRE(std::format("{}", Matrix(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)) ==
                R"([
  1; 2; 3; 4
  5; 6; 7; 8
  9; 10; 11; 12
  13; 14; 15; 16
])");
    }

    SECTION("ImageFormat")
    {
        REQUIRE(std::format("{}", ImageFormat::R8_UNorm) == "R8_UNorm");
        REQUIRE(std::format("{}", ImageFormat::R8G8B8A8_UNorm) == "R8G8B8A8_UNorm");
        REQUIRE(std::format("{}", ImageFormat::R8G8B8A8_Srgb) == "R8G8B8A8_Srgb");
        REQUIRE(std::format("{}", ImageFormat::R32G32B32A32_Float) == "R32G32B32A32_Float");
    }
}
