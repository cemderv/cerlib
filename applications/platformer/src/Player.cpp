#include "Player.hpp"

#include "Level.hpp"
#include "Tile.hpp"

// Constants for controling horizontal m_Movement
constexpr float move_acceleration  = 13000.0f;
constexpr float ground_drag_factor = 0.48f;
constexpr float air_drag_factor    = 0.58f;

// Constants for controlling vertical m_Movement
constexpr float max_jump_time        = 0.35f;
constexpr float jump_launch_velocity = -2700.0f;
constexpr float gravity_acceleration = 3400.0f;
constexpr float max_velocity_x       = 250.0f;
constexpr float max_fall_speed       = 550.0f;
constexpr float jump_control_power   = 0.14f;

Player::Player(Level* level, cer::Vector2 position)
    : m_level(level)
    , m_idle_animation("sprites/player/idle.png", 1.0f, true)
    , m_run_animation("sprites/player/run.png", 0.07f, true)
    , m_jump_animation("sprites/player/jump.png", 0.06f, false)
    , m_celebrate_animation("sprites/player/celebrate.png", 0.08f, false)
    , m_die_animation("sprites/player/die.png", 0.1f, false)
    , m_killed_sound("sounds/player_killed.wav")
    , m_jump_sound("sounds/player_jump.wav")
    , m_fall_sound("sounds/player_fall.wav")
    , m_position(position)
{
    // Calculate bounds within texture size.
    const auto  frame_width  = float(m_idle_animation.frame_width());
    const auto  frame_height = float(m_idle_animation.frame_height());
    const float width        = cer::round(frame_width * 0.4f);
    const float left         = cer::round((frame_width - width) / 2);
    const float height       = cer::round(frame_width * 0.8f);
    const float top          = cer::round(frame_height - height);

    m_local_bounds = {left, top, width, height};

    reset(position);
}

void Player::on_reached_exit()
{
    m_has_reached_exit = true;
    m_sprite.play_animation(m_celebrate_animation);
}

bool Player::is_alive() const
{
    return m_is_alive;
}

void Player::handle_collisions()
{
    // Get the player's bounding rectangle and find neighboring tiles.
    cer::Rectangle bounds = bounding_rect();

    const auto left_tile   = int(cer::floor(bounds.left() / Tile::width));
    const auto right_tile  = int(cer::ceiling(bounds.right() / Tile::width)) - 1;
    const auto top_tile    = int(cer::floor(bounds.top() / Tile::height));
    const auto bottom_tile = int(cer::ceiling(bounds.bottom() / Tile::height)) - 1;

    // Reset flag to search for ground collision.
    m_is_on_ground = false;

    // For each potentially colliding tile,
    for (int y = top_tile; y <= bottom_tile; ++y)
    {
        for (int x = left_tile; x <= right_tile; ++x)
        {
            // If this tile is collidable,
            const TileCollision collision = m_level->collision_at(x, y);

            if (collision != TileCollision::Passable)
            {
                // Determine collision depth (with direction) and magnitude.
                const cer::Rectangle tile_bounds = m_level->bounds(x, y);

                const std::optional<cer::Vector2> depth =
                    cer::Rectangle::intersection_depth(bounds, tile_bounds);

                if (depth)
                {
                    const cer::Vector2 abs_depth = cer::abs(*depth);

                    // Resolve the collision along the shallow axis.
                    if (abs_depth.y < abs_depth.x || collision == TileCollision::Platform)
                    {
                        // If we crossed the top of a tile, we are on the ground.
                        if (m_previous_bottom <= tile_bounds.top())
                        {
                            m_is_on_ground = true;
                        }

                        // Ignore platforms, unless we are on the ground.
                        if (collision == TileCollision::Impassable || m_is_on_ground)
                        {
                            // Resolve the collision along the Y axis.
                            m_position = {m_position.x, m_position.y + depth->y};

                            // Perform further collisions with the new bounds.
                            bounds = bounding_rect();
                        }
                    }
                    else if (collision == TileCollision::Impassable) // Ignore platforms.
                    {
                        // Resolve the collision along the X axis.
                        m_position = {m_position.x + depth->x, m_position.y};

                        // Perform further collisions with the new bounds.
                        bounds = bounding_rect();
                    }
                }
            }
        }
    }

    // Save the new bounds bottom.
    m_previous_bottom = bounds.bottom();
}

float Player::do_jump(float velocity_y, cer::GameTime game_time)
{
    // If the player wants to jump
    if (m_is_jumping)
    {
        // Begin or continue a jump
        if ((!m_was_jumping && m_is_on_ground) || m_jump_time > 0.0f)
        {
            if (m_jump_time == 0.0f)
            {
                play_sound_fire_and_forget(m_jump_sound);
            }

            m_jump_time += float(game_time.elapsed_time);
            m_sprite.play_animation(m_jump_animation);
        }

        // If we are in the ascent of the jump
        if (m_jump_time > 0.0f && m_jump_time <= max_jump_time)
        {
            // Fully override the vertical velocity with a power curve that gives players more
            // control over the top of the jump
            velocity_y = jump_launch_velocity *
                         (1.0f - cer::pow(m_jump_time / max_jump_time, jump_control_power));
        }
        else
        {
            // Reached the apex of the jump
            m_jump_time = 0.0f;
        }
    }
    else
    {
        // Continues not jumping or cancels a jump in progress
        m_jump_time = 0.0f;
    }

    m_was_jumping = m_is_jumping;

    return velocity_y;
}

void Player::apply_physics(cer::GameTime time)
{
    const float elapsed = float(time.elapsed_time);

    cer::Vector2 previous_position = m_position;

    // Base velocity is a combination of horizontal movement control and
    // acceleration downward due to gravity.
    m_velocity.x += m_movement * move_acceleration;
    m_velocity.y =
        cer::clamp(m_velocity.y + gravity_acceleration * elapsed, -max_fall_speed, max_fall_speed);

    m_velocity.y = do_jump(m_velocity.y, time);

    // Apply pseudo-drag horizontally.
    if (m_is_on_ground)
    {
        m_velocity.x *= ground_drag_factor;
    }
    else
    {
        m_velocity.x *= air_drag_factor;
    }

    // Prevent the player from running faster than his top speed.
    m_velocity.x = cer::clamp(m_velocity.x, -max_velocity_x, max_velocity_x);

    // Apply velocity.
    m_position += m_velocity * elapsed;
    m_position = round(m_position);

    // If the player is now colliding with the level, separate them.
    handle_collisions();

    // If the collision stopped us from moving, reset the velocity to zero.
    if (cer::equal_within_epsilon(m_position.x, previous_position.x))
    {
        m_velocity.x = 0;
    }

    if (cer::equal_within_epsilon(m_position.y, previous_position.y))
    {
        m_velocity.y = 0;
    }
}

cer::Rectangle Player::bounding_rect() const
{
    const cer::Vector2 sprite_origin = m_sprite.origin();
    const float        left = cer::round(m_position.x - sprite_origin.x) + m_local_bounds.x;
    const float        top  = cer::round(m_position.y - sprite_origin.y) + m_local_bounds.y;

    return {left, top, m_local_bounds.width, m_local_bounds.height};
}

bool Player::is_on_ground() const
{
    return m_is_on_ground;
}

void Player::update_input()
{
    // If any digital horizontal movement input is found, override the analog movement.
    if (is_key_down(cer::Key::Left) || is_key_down(cer::Key::A))
    {
        m_movement      = -1.0f;
        m_last_movement = m_movement;
    }
    else if (is_key_down(cer::Key::Right) || is_key_down(cer::Key::D))
    {
        m_movement      = 1.0f;
        m_last_movement = m_movement;
    }

    // Check if the player wants to jump.
    m_is_jumping =
        is_key_down(cer::Key::Space) || is_key_down(cer::Key::Up) || is_key_down(cer::Key::W);
}

void Player::update(cer::GameTime time)
{
    apply_physics(time);

    if (m_is_alive && m_is_on_ground && !m_has_reached_exit)
    {
        const bool is_moving = cer::abs(m_velocity.x) - 0.02f > 0;
        m_sprite.play_animation(is_moving ? m_run_animation : m_idle_animation);
    }

    // Clear input.
    m_movement   = 0.0f;
    m_is_jumping = false;

    m_sprite.update(time);
}

void Player::draw() const
{
    // Flip the sprite to face the way we are moving.
    const cer::SpriteFlip flip =
        m_last_movement > 0.0f ? cer::SpriteFlip::Horizontally : cer::SpriteFlip::None;

    m_sprite.draw(m_position, flip);
}

void Player::on_killed(const Enemy* killed_by)
{
    m_is_alive = false;

    if (killed_by != nullptr)
    {
        play_sound_fire_and_forget(m_killed_sound);
    }
    else
    {
        play_sound_fire_and_forget(m_fall_sound);
    }

    m_sprite.play_animation(m_die_animation);
}

void Player::reset(cer::Vector2 position)
{
    m_position = position - cer::Vector2{0, 10};
    m_velocity = {};
    m_is_alive = true;

    m_sprite.play_animation(m_idle_animation);
}
