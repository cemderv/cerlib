// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <algorithm>
#include <cerlib/Math.hpp>
#include <cerlib/Matrix.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Vector4.hpp>
#include <snitch/snitch.hpp>

using cer::Matrix;

TEST_CASE("Matrix", "[math]")
{
    SECTION("Construction")
    {
        REQUIRE(Matrix{} == Matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    }

    SECTION("transpose")
    {
        REQUIRE(transpose(Matrix{}) == Matrix{});
        REQUIRE(transpose(Matrix{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}) ==
                Matrix{1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16});
    }

    SECTION("translate")
    {
        REQUIRE(cer::translate({0, 0}) == Matrix{});
        REQUIRE(cer::translate({1, 2}).m41 == 1);
        REQUIRE(cer::translate({1, 2}).m42 == 2);
        REQUIRE(cer::translate({-1, 2}) == Matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -1, 2, 0, 1});
    }

    SECTION("scale")
    {
        REQUIRE(cer::scale({1, 1}) == Matrix{});
        REQUIRE(cer::scale({0, 0}) == Matrix{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
        REQUIRE(cer::scale({1, 2}) == Matrix{1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    }

    SECTION("rotate")
    {
        const auto rad = cer::radians(45.0f);
        const auto s   = cer::sin(rad);
        const auto c   = cer::cos(rad);

        REQUIRE(cer::rotate(cer::radians(0.0f)) == Matrix{});
        REQUIRE(cer::rotate(rad) == Matrix{c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
        REQUIRE(cer::rotate(-rad) == Matrix{c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    }

    SECTION("operator*")
    {
        REQUIRE(Matrix{} * Matrix{} == Matrix{});

        const auto a = cer::translate({1, 2});
        const auto b = cer::rotate(cer::radians(45.0f));
        const auto c = cer::translate({3, 4});

        // ab != ba:
        REQUIRE(a * b != b * a);

        // (ab)c = a(bc):
        REQUIRE(are_equal_within((a * b) * c, a * (b * c), 0.000001f));

        // I * a = a && a * I = a:
        REQUIRE(Matrix{} * a == a);
        REQUIRE(a * Matrix{} == a);

        // 0 * a = 0 && a * 0 = 0:
        REQUIRE(Matrix{0.0f} * a == Matrix{0.0f});
        REQUIRE(a * Matrix{0.0f} == Matrix{0.0f});
    }
}
