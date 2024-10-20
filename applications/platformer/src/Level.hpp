#pragma once

#include "Enemy.hpp"
#include "Gem.hpp"
#include "Player.hpp"
#include "Tile.hpp"
#include <array>
#include <cerlib.hpp>

class Level : public cer::Asset
{
  public:
    struct Args
    {
        int* score = nullptr;
    };

    explicit Level(std::string_view name, std::string_view contents, Args args);

    std::string_view name() const;

    void update(cer::GameTime time);

    void draw();

    TileCollision collision_at(int x, int y) const;

    const Player* player() const;

    uint32_t width() const;

    uint32_t height() const;

    cer::Rectangle bounds(int x, int y) const;

    bool is_exit_reached() const;

    double time_remaining() const;

    void start_new_life();

  private:
    Tile load_tile(char type, int x, int y);

    Tile load_start_tile(int x, int y);

    Tile load_exit_tile(int x, int y);

    Tile load_enemy_tile(int x, int y, std::string_view sprite_set);

    Tile load_gem_tile(int x, int y, bool is_super_gem);

    void update_gems(cer::GameTime time);

    void update_enemies(cer::GameTime time);

    void on_gem_collected(Gem& gem);

    void on_player_killed(const Enemy* killed_by);

    void on_exit_reached();

    void draw_tiles();

    static constexpr int          entity_layer      = 2;
    static constexpr cer::Vector2 invalid_position  = cer::Vector2{-1};
    static constexpr int          points_per_second = 5;

    std::string               m_name;
    int                       m_width  = 0;
    int                       m_height = 0;
    cer::List<Tile>           m_tiles;
    std::array<cer::Image, 3> m_layers;
    Player                    m_player;
    cer::List<Gem>            m_gems;
    cer::List<Enemy>          m_enemies;
    cer::Vector2              m_start;
    cer::Vector2              m_exit            = invalid_position;
    int*                      m_score           = nullptr;
    bool                      m_is_exit_reached = false;
    double                    m_time_remaining  = 0.0;
    cer::Sound                m_exit_reached_sound;
};
