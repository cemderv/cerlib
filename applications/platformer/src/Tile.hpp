#pragma once

#include <cerlib.hpp>

enum class TileCollision
{
    Passable   = 0,
    Impassable = 1,
    Platform   = 2,
};

struct Tile
{
    static constexpr float        width  = 40.0f;
    static constexpr float        height = 32.0f;
    static constexpr cer::Vector2 size   = cer::Vector2{width, height};

    cer::Image    image;
    TileCollision collision = TileCollision::Passable;
};
