#pragma once

#include "Animation.hpp"
#include <cerlib.hpp>

using namespace cer;

class Level;
class Enemy;

class Player
{
  public:
    Player() = default;

    Player(Level* level, Vector2 position);

    /**
     * Resets the player to life.
     *
     * @param position The position to come to life at.
     */
    void reset(Vector2 position);

    void update_input();

    void update(GameTime time);

    void draw() const;

    void on_killed(const Enemy* killed_by);

    void on_reached_exit();

    bool is_alive() const;

    void apply_physics(GameTime time);

    Rectangle bounding_rect() const;

    bool is_on_ground() const;

  private:
    float do_jump(float velocity_y, GameTime game_time);

    void handle_collisions();

    Level* m_level = nullptr;

    // Animations
    Animation       m_idle_animation;
    Animation       m_run_animation;
    Animation       m_jump_animation;
    Animation       m_celebrate_animation;
    Animation       m_die_animation;
    AnimationPlayer m_sprite;

    // Sounds
    Sound m_killed_sound;
    Sound m_jump_sound;
    Sound m_fall_sound;

    // Physics state
    Vector2 m_position;
    float   m_previous_bottom = 0.0f;
    Vector2 m_velocity;

    bool  m_is_alive         = true;
    bool  m_is_on_ground     = false;
    bool  m_has_reached_exit = false;
    float m_movement         = 0.0f;
    float m_last_movement    = 0.0f;

    // Jumping state
    bool  m_is_jumping  = false;
    bool  m_was_jumping = false;
    float m_jump_time   = 0.0f;

    Rectangle m_local_bounds;
};
