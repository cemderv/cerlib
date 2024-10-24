#pragma once

#include "Animation.hpp"
#include <cerlib.hpp>

using namespace cer;

enum class FaceDirection
{
    Left  = -1,
    Right = 1,
};

class Level;

class Enemy
{
  public:
    Enemy(Level* level, Vector2 position, std::string sprite_set);

    void update(GameTime time);

    void draw() const;

    Rectangle bounding_rect() const;

  private:
    Level*    m_level = nullptr;
    Vector2   m_position;
    Rectangle m_local_bounds;

    // Animations
    Animation       m_run_animation;
    Animation       m_idle_animation;
    AnimationPlayer m_sprite;

    FaceDirection m_direction = FaceDirection::Left;
    float         m_wait_time = 0.0f;
};
