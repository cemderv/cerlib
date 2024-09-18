#pragma once

#include <cerlib.hpp>

class Level;

class Gem
{
  public:
    Gem(Level* level, cer::Vector2 position, bool is_super_gem);

    void update(cer::GameTime time);

    void draw() const;

    void on_collected();

    cer::Vector2 position() const;

    cer::Circle bounding_circle() const;

    int score_value() const;

  private:
    Level*       m_level = nullptr;
    cer::Image   m_texture;
    cer::Vector2 m_origin;
    cer::Sound   m_collected_sound;
    cer::Vector2 m_base_position;
    float        m_bounce       = 0.0f;
    bool         m_is_super_gem = false;
};