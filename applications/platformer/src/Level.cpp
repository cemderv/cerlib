#include "Level.hpp"

#include <sstream>
#include <stdexcept>

static Tile load_tile(std::string_view name, TileCollision collision)
{
    return {
        .image     = cer::load_image(cer_fmt::format("tiles/{}.png", name)),
        .collision = collision,
    };
}

static auto load_variety_tile(std::string_view base_name,
                              int              variation_count,
                              TileCollision    collision)
{
    const int index = cer::random_int(0, variation_count - 1);
    return load_tile(cer_fmt::format("{}{}", base_name, index), collision);
}

Level::Level(std::string_view name, std::string_view contents, Args args)
    : m_name(name)
    , m_score(args.score)
    , m_time_remaining(70.0)
{
    std::stringstream stream;
    stream << contents;

    cer::List<std::string> lines;
    std::string            line;

    while (getline(stream, line))
    {
        cer::util::trim_string(line, {{' ', '\r', '\n'}});
        lines.push_back(line);

        if (m_width != 0 && line.size() != m_width)
        {
            throw std::invalid_argument{
                "The length of a line is different from all preceding lines."};
        }

        m_width = int(line.size());
    }

    m_height = int(lines.size());

    // Allocate the tile grid.
    m_tiles.resize(size_t(m_width) * size_t(m_height));

    // Loop over every tile position to load each tile.
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            m_tiles.at(y * m_width + x) = load_tile(lines[y][x], x, y);
        }
    }

    // Load background layer textures. For now, all levels must
    // use the same backgrounds and only use the left-most part of them.
    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        // Choose a random segment if each background layer for level variety.
        const int         segment_index = cer::random_int(0, 2);
        const std::string image_name =
            cer_fmt::format("backgrounds/layer{}_{}.png", i, segment_index);

        m_layers[i] = cer::load_image(image_name);
    }

    m_exit_reached_sound = cer::load_sound("sounds/exit_reached.wav");
}

Tile Level::load_tile(char type, int x, int y)
{
    switch (type)
    {
        // Blank space
        case '.': return Tile{.collision = TileCollision::Passable};

        // Exit
        case 'X': return load_exit_tile(x, y);

        // Gem
        case 'G': return load_gem_tile(x, y, false);

        // Super gem
        case 'U': return load_gem_tile(x, y, true);

        // Floating platform
        case '-': return ::load_tile("platform", TileCollision::Platform);

        // Various enemies
        case 'A': return load_enemy_tile(x, y, "monster_a");
        case 'B': return load_enemy_tile(x, y, "monster_b");
        case 'C': return load_enemy_tile(x, y, "monster_c");
        case 'D': return load_enemy_tile(x, y, "monster_d");

        // Platform block
        case '~': return load_variety_tile("block_b", 2, TileCollision::Platform);

        // Passable block
        case ':': return load_variety_tile("block_b", 2, TileCollision::Passable);

        // Player 1 start point
        case '1': return load_start_tile(x, y);

        // Impassable block
        case '#': return load_variety_tile("block_a", 7, TileCollision::Impassable);

        // Unknown tile type character
        default: throw std::runtime_error{"Unsupported tile type character"};
    }
}

Tile Level::load_start_tile(int x, int y)
{
    m_start  = bounds(x, y).bottom_center();
    m_player = Player(this, m_start);

    return {.image = {}, .collision = TileCollision::Passable};
}

Tile Level::load_exit_tile(int x, int y)
{
    if (m_exit != invalid_position)
    {
        throw std::logic_error{"A level may only have one exit."};
    }

    m_exit = bounds(x, y).center();

    return ::load_tile("exit", TileCollision::Passable);
}

Tile Level::load_enemy_tile(int x, int y, std::string_view sprite_set)
{
    const auto position = bounds(x, y).bottom_center();
    m_enemies.emplace_back(this, position, std::string(sprite_set));

    return {.image = {}, .collision = TileCollision::Passable};
}

Tile Level::load_gem_tile(int x, int y, bool is_super_gem)
{
    m_gems.emplace_back(this, bounds(x, y).center(), is_super_gem);

    return {.image = {}, .collision = TileCollision::Passable};
}

void Level::update_gems(cer::GameTime time)
{
    const auto player_rect = m_player.bounding_rect();

    for (int i = 0; i < int(m_gems.size()); ++i)
    {
        Gem& gem = m_gems[i];
        gem.update(time);

        if (player_rect.intersects(gem.bounding_circle()))
        {
            on_gem_collected(gem);
            m_gems.erase(m_gems.begin() + i);
            --i;
        }
    }
}

void Level::update_enemies(cer::GameTime time)
{
    const cer::Rectangle player_rect = m_player.bounding_rect();

    for (Enemy& enemy : m_enemies)
    {
        enemy.update(time);

        if (enemy.bounding_rect().intersects(player_rect))
        {
            on_player_killed(&enemy);
        }
    }
}

void Level::on_gem_collected(Gem& gem)
{
    *m_score += gem.score_value();
    gem.on_collected();
}

void Level::on_player_killed(const Enemy* killed_by)
{
    m_player.on_killed(killed_by);
}

void Level::on_exit_reached()
{
    m_player.on_reached_exit();
    play_sound_fire_and_forget(m_exit_reached_sound);
    m_is_exit_reached = true;
}

void Level::start_new_life()
{
    m_player.reset(m_start);
}

void Level::draw_tiles()
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            // If there is a visible tile in that position
            if (auto image = m_tiles[y * m_width + x].image)
            {
                // Draw it in screen space.
                cer::draw_sprite(image, cer::Vector2{float(x), float(y)} * Tile::size);
            }
        }
    }
}

std::string_view Level::name() const
{
    return m_name;
}

void Level::update(cer::GameTime time)
{
    // Pause while the player is dead or time is expired.
    if (!m_player.is_alive() || cer::is_zero(m_time_remaining))
    {
        // Still want to perform physics on the player.
        m_player.update(time);
    }
    else if (m_is_exit_reached)
    {
        // Animate the time being converted into points.
        int seconds = int(cer::round(time.elapsed_time * 100.0f));
        seconds     = cer::min(seconds, int(cer::ceiling(m_time_remaining)));
        m_time_remaining -= seconds;
        *m_score += seconds * points_per_second;

        m_player.update(time);
    }
    else
    {
        m_time_remaining -= time.elapsed_time;
        m_player.update_input();
        m_player.update(time);
        update_gems(time);

        // Falling off the bottom of the level kills the player.
        if (m_player.bounding_rect().top() >= float(m_height) * Tile::height)
        {
            on_player_killed(nullptr);
        }

        update_enemies(time);

        // The player has reached the exit if they are standing on the ground and
        // his bounding rectangle contains the center of the exit tile. They can only
        // exit when they have collected all of the gems.
        if (m_player.is_alive() && m_player.is_on_ground() &&
            m_player.bounding_rect().contains(m_exit))
        {
            on_exit_reached();
        }
    }

    // Clamp the time remaining at zero.
    m_time_remaining = cer::max(0.0, m_time_remaining);
}

void Level::draw()
{
    const cer::Vector2   canvas_size     = cer::current_canvas_size();
    const cer::Rectangle background_rect = cer::Rectangle{0, 0, canvas_size};

    for (int i = 0; i <= entity_layer; ++i)
    {
        cer::draw_sprite({
            .image    = m_layers[i],
            .dst_rect = background_rect,
        });
    }

    draw_tiles();

    for (const Gem& gem : m_gems)
    {
        gem.draw();
    }

    m_player.draw();

    for (const Enemy& enemy : m_enemies)
    {
        enemy.draw();
    }

    for (int i = entity_layer + 1; i < m_layers.size(); ++i)
    {
        cer::draw_sprite({
            .image    = m_layers[i],
            .dst_rect = {0, 0, m_layers[i].size()},
        });
    }
}

cer::Rectangle Level::bounds(int x, int y) const
{
    return {float(x) * Tile::width, float(y) * Tile::height, Tile::width, Tile::height};
}

bool Level::is_exit_reached() const
{
    return m_is_exit_reached;
}

double Level::time_remaining() const
{
    return m_time_remaining;
}

TileCollision Level::collision_at(int x, int y) const
{
    // Prevent escaping past the level ends.
    if (x < 0 || x >= m_width)
    {
        return TileCollision::Impassable;
    }

    // Allow jumping past the level top and falling through the bottom.
    if (y < 0 || y >= m_height)
    {
        return TileCollision::Passable;
    }

    return m_tiles[y * m_width + x].collision;
}

const Player* Level::player() const
{
    return &m_player;
}

uint32_t Level::width() const
{
    return m_width;
}

uint32_t Level::height() const
{
    return m_height;
}
