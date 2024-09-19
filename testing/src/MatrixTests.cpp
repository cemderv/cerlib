// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <algorithm>
#include <cerlib/Matrix.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Vector4.hpp>
#include <snitch/snitch.hpp>

using namespace cer;

TEST_CASE("Matrix", "[math]")
{
    SECTION("Construction")
    {
        REQUIRE(Matrix() == Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    }

    SECTION("transpose")
    {
        REQUIRE(transpose(Matrix()) == Matrix());
        REQUIRE(transpose(Matrix(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)) ==
                Matrix(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16));
    }

    SECTION("translate")
    {
        REQUIRE(translate({0, 0}) == Matrix());
        REQUIRE(translate({1, 2}).m41 == 1);
        REQUIRE(translate({1, 2}).m42 == 2);
        REQUIRE(translate({-1, 2}) == Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -1, 2, 0, 1));
    }

    SECTION("scale")
    {
        REQUIRE(scale({1, 1}) == Matrix());
        REQUIRE(scale({0, 0}) == Matrix(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
        REQUIRE(scale({1, 2}) == Matrix(1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    }

    SECTION("rotate")
    {
        const float rad = cer::radians(45.0f);
        const float s   = cer::sin(rad);
        const float c   = cer::cos(rad);

        REQUIRE(rotate(radians(0.0f)) == Matrix());
        REQUIRE(rotate(rad) == Matrix(c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
        REQUIRE(rotate(-rad) == Matrix(c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    }

    SECTION("operator*")
    {
        REQUIRE(Matrix() * Matrix() == Matrix());

        const Matrix a = translate({1, 2});
        const Matrix b = rotate(radians(45.0f));
        const Matrix c = translate({3, 4});

        // ab != ba:
        REQUIRE(a * b != b * a);

        // (ab)c = a(bc):
        REQUIRE(are_equal_within((a * b) * c, a * (b * c), 0.000001f));

        // I * a = a && a * I = a:
        REQUIRE(Matrix() * a == a);
        REQUIRE(a * Matrix() == a);

        // 0 * a = 0 && a * 0 = 0:
        REQUIRE(Matrix(0.0f) * a == Matrix(0.0f));
        REQUIRE(a * Matrix(0.0f) == Matrix(0.0f));
    }
}
