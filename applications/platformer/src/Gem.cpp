#include "Gem.hpp"

#include "Tile.hpp"

// Bounce control constants
constexpr float c_bounce_height = 0.18f;
constexpr float c_bounce_rate   = 3.0f;
constexpr float c_bounce_sync   = -0.75f;

Gem::Gem(Level* level, cer::Vector2 position, bool is_super_gem)
    : m_level(level)
    , m_base_position(position)
    , m_is_super_gem(is_super_gem)
{
    m_texture         = cer::load_image("sprites/gem.png");
    m_origin          = m_texture.size() / 2;
    m_collected_sound = is_super_gem ? cer::load_sound("sounds/super_gem_collected.wav")
                                     : cer::load_sound("sounds/gem_collected.wav");
}

void Gem::update(cer::GameTime time)
{
    const cer::Vector2 position      = this->position();
    const float        bounce_rate   = m_is_super_gem ? c_bounce_rate * 1.4f : c_bounce_rate;
    const float        bounce_height = m_is_super_gem ? c_bounce_height * 0.8f : c_bounce_height;

    // Bounce along a sine curve over time.
    // Include the X coordinate so that neighboring gems bounce in a nice wave pattern.
    const float t = time.total_time * bounce_rate + (position.x * c_bounce_sync);

    m_bounce = cer::sin(t) * bounce_height * m_texture.heightf();
}

void Gem::on_collected()
{
    play_sound_fire_and_forget(m_collected_sound);
}

void Gem::draw() const
{
    cer::draw_sprite({
        .image    = m_texture,
        .dst_rect = {position(), m_texture.size()},
        .color    = m_is_super_gem ? cer::cornflowerblue * 2.0f : cer::yellow,
        .origin   = m_origin,
    });
}

cer::Vector2 Gem::position() const
{
    return m_base_position + cer::Vector2{0, m_bounce};
}

cer::Circle Gem::bounding_circle() const
{
    return {position(), Tile::width / 3.0f};
}

int Gem::score_value() const
{
    return m_is_super_gem ? 100 : 30;
}
