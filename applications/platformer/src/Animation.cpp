#include "Animation.hpp"

Animation::Animation(std::string_view name, float frame_time, bool is_looping)
    : image(name)
    , frame_time(frame_time)
    , is_looping(is_looping)
{
}

uint32_t Animation::frame_count() const
{
    return image.width() / frame_width();
}

uint32_t Animation::frame_width() const
{
    // Assume square frames.
    return image.height();
}

uint32_t Animation::frame_height() const
{
    return image.height();
}

Vector2 AnimationPlayer::origin() const
{
    return {
        float(m_animation.frame_width()) / 2,
        float(m_animation.frame_height()),
    };
}

void AnimationPlayer::update(GameTime time)
{
    m_time += float(time.elapsed_time);

    while (m_time > m_animation.frame_time)
    {
        m_time -= m_animation.frame_time;

        if (m_animation.is_looping)
        {
            m_frame_index = (m_frame_index + 1) % m_animation.frame_count();
        }
        else
        {
            m_frame_index = min(m_frame_index + 1, m_animation.frame_count() - 1);
        }
    }
}

void AnimationPlayer::draw(Vector2 position, SpriteFlip flip) const
{
    const float texture_height = m_animation.image.heightf();

    Rectangle source{float(m_frame_index) * texture_height, 0, texture_height, texture_height};

    draw_sprite({
        .image    = m_animation.image,
        .dst_rect = {position, texture_height, texture_height},
        .src_rect = source,
        .origin   = origin(),
        .flip     = flip,
    });
}

void AnimationPlayer::play_animation(const Animation& animation)
{
    if (m_animation.image == animation.image)
    {
        return;
    }

    m_animation   = animation;
    m_frame_index = 0;
    m_time        = 0.0f;
}