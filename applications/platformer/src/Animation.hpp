#pragma once

#include <cerlib.hpp>

using namespace cer;

class Animation
{
  public:
    Animation() = default;

    Animation(std::string_view name, float frame_time, bool is_looping);

    auto frame_count() const -> uint32_t;

    auto frame_width() const -> uint32_t;

    auto frame_height() const -> uint32_t;

    Image image;
    float frame_time = 0.0f;
    bool  is_looping = false;
};

class AnimationPlayer
{
  public:
    void play_animation(const Animation& animation);

    void update(GameTime time);

    void draw(Vector2 position, SpriteFlip flip) const;

    Vector2 origin() const;

  private:
    Animation m_animation;
    uint32_t  m_frame_index = 0;
    float     m_time        = 0.0f;
};
