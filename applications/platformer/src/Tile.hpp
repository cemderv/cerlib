#pragma once

#include <cerlib.hpp>

using namespace cer;

enum class TileCollision
{
    Passable   = 0,
    Impassable = 1,
    Platform   = 2,
};

struct Tile
{
    static constexpr auto width  = 40.0f;
    static constexpr auto height = 32.0f;
    static constexpr auto size   = Vector2{width, height};

    Image         image;
    TileCollision collision = TileCollision::Passable;
};
