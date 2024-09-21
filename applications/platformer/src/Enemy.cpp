#include "Enemy.hpp"

#include "Level.hpp"

static constexpr float max_wait_time = 0.5f;
static constexpr float move_speed    = 64.0f;

Enemy::Enemy(Level* level, cer::Vector2 position, std::string sprite_set)
    : m_level(level)
    , m_position(position)
{
    // Load animations.
    sprite_set       = cer_fmt::format("sprites/{}/", sprite_set);
    m_run_animation  = {sprite_set + "run.png", 0.1f, true};
    m_idle_animation = {sprite_set + "idle.png", 0.15f, true};

    m_sprite.play_animation(m_idle_animation);

    // Calculate bounds within texture size.
    const auto width  = static_cast<int>(m_idle_animation.frame_width() * 0.35);
    const auto left   = (m_idle_animation.frame_width() - width) / 2;
    const auto height = static_cast<int>(m_idle_animation.frame_width() * 0.7);
    const auto top    = m_idle_animation.frame_height() - height;

    m_local_bounds = {static_cast<float>(left),
                      static_cast<float>(top),
                      static_cast<float>(width),
                      static_cast<float>(height)};
}

void Enemy::update(cer::GameTime time)
{
    const auto elapsed = static_cast<float>(time.elapsed_time);
    const auto dir     = static_cast<int>(m_direction);

    // Calculate tile position based on the side we are walking towards.
    const float pos_x  = m_position.x + (m_local_bounds.width / 2.0f * static_cast<float>(dir));
    const auto  tile_x = static_cast<int>(floor(pos_x / Tile::width)) - dir;
    const auto  tile_y = static_cast<int>(floor(m_position.y / Tile::height));

    if (m_wait_time > 0)
    {
        // Wait for some amount of time.
        m_wait_time = cer::max(0.0f, m_wait_time - elapsed);

        if (m_wait_time <= 0.0f)
        {
            // Then turn around.
            m_direction = static_cast<FaceDirection>(-dir);
        }
    }
    else
    {
        // If we are about to run into a wall or off a cliff, start waiting.
        if (m_level->collision_at(tile_x + dir, tile_y - 1) == TileCollision::Impassable ||
            m_level->collision_at(tile_x + dir, tile_y) == TileCollision::Passable)
        {
            m_wait_time = max_wait_time;
        }
        else
        {
            // Move in the current direction.
            m_position += cer::Vector2{static_cast<float>(dir) * move_speed * elapsed, 0.0f};
        }
    }

    m_sprite.update(time);

    // Stop running when the game is paused or before turning around.
    if (!m_level->player()->is_alive() || m_level->is_exit_reached() ||
        cer::is_zero(m_level->time_remaining()) || m_wait_time > 0)
    {
        m_sprite.play_animation(m_idle_animation);
    }
    else
    {
        m_sprite.play_animation(m_run_animation);
    }
}

void Enemy::draw() const
{
    // Draw facing the way the enemy is moving.
    const cer::SpriteFlip flip =
        static_cast<int>(m_direction) > 0 ? cer::SpriteFlip::Horizontally : cer::SpriteFlip::None;

    m_sprite.draw(m_position, flip);
}

cer::Rectangle Enemy::bounding_rect() const
{
    const cer::Vector2 sprite_origin = m_sprite.origin();
    const float        left          = round(m_position.x - sprite_origin.x) + m_local_bounds.x;
    const float        top           = round(m_position.y - sprite_origin.y) + m_local_bounds.y;

    return {left, top, m_local_bounds.width, m_local_bounds.height};
}