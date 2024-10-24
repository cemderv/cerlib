#pragma once

#include <cerlib.hpp>

using namespace cer;

class Level;

class Gem
{
  public:
    Gem(Level* level, Vector2 position, bool is_super_gem);

    void update(GameTime time);

    void draw() const;

    void on_collected();

    Vector2 position() const;

    Circle bounding_circle() const;

    int score_value() const;

  private:
    Level*  m_level = nullptr;
    Image   m_texture;
    Vector2 m_origin;
    Sound   m_collected_sound;
    Vector2 m_base_position;
    float   m_bounce       = 0.0f;
    bool    m_is_super_gem = false;
};