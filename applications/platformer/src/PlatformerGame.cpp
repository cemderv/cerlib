#include "PlatformerGame.hpp"

// When the time remaining is less than the warning time, it blinks on the hud
constexpr auto warning_time = 10.0f;

PlatformerGame::PlatformerGame()
{
    // The original game is written for a fixed resolution of 800x480 pixels.
    constexpr Vector2 resolution{800, 480};

    m_window = Window{"Platformer Game", 0, {}, {}, uint32_t(resolution.x), uint32_t(resolution.y)};

    m_window.set_resizable(false);
    m_window.set_clear_color(green);

    if (m_window.pixel_ratio() != 1.0f)
    {
        m_canvas = Image{uint32_t(resolution.x),
                         uint32_t(resolution.y),
                         ImageFormat::R8G8B8A8_UNorm,
                         m_window};
    }

    register_custom_asset_loader_for_type<Level>(
        [](std::string_view name, const AssetData& data, const std::any& extra_info) {
            return std::make_shared<Level>(name,
                                           data.as_string_view(),
                                           std::any_cast<Level::Args>(extra_info));
        });
}

void PlatformerGame::load_content()
{
    m_hud_font = Font::built_in();

    m_win_overlay  = Image{"overlays/you_win.png"};
    m_lose_overlay = Image{"overlays/you_lose.png"};
    m_died_overlay = Image{"overlays/you_died.png"};

    SoundChannel music_channel = play_sound_in_background(Sound{"sounds/music.mp3"});

    music_channel.set_looping(true);

    load_next_level();
}

bool PlatformerGame::update(const GameTime& time)
{
    if (time.elapsed_time == 0.0)
    {
        return true;
    }

    constexpr double target_elapsed_time = 1.0 / 60.0;

    m_time_accumulator += time.elapsed_time;

    if (m_time_accumulator < target_elapsed_time)
    {
        return true;
    }

    GameTime corrected_time{time};

    corrected_time.elapsed_time = target_elapsed_time;
    m_time_accumulator -= target_elapsed_time;

    if (was_key_just_pressed(Key::Escape))
    {
        return false;
    }

    if (was_key_just_pressed(Key::Space))
    {
        if (!m_level->player()->is_alive())
        {
            m_total_score = 0;
            m_level_index = -1;
            load_next_level();
        }
        else if (is_zero(m_level->time_remaining()))
        {
            if (m_level->is_exit_reached())
            {
                load_next_level();
            }
            else
            {
                m_total_score = 0;
                reload_current_level();
            }
        }
    }

    m_level->update(corrected_time);

    return true;
}

void PlatformerGame::draw(const Window& window)
{
    if (m_canvas)
    {
        set_blend_state(non_premultiplied);
        m_canvas.set_canvas_clear_color(red);
        set_canvas(m_canvas);
    }

    m_level->draw();
    draw_hud();

    if (m_canvas)
    {
        set_blend_state(opaque);
        set_canvas({});

        draw_sprite({
            .image    = m_canvas,
            .dst_rect = {0, 0, current_canvas_size()},
        });
    }
}

void PlatformerGame::draw_hud()
{
    constexpr uint32_t text_size = 20;

    const auto draw_shadowed_string = [&](std::string_view text, Vector2 position, Color color) {
        draw_string(text, m_hud_font, text_size, position + Vector2{1, 1}, black);
        draw_string(text, m_hud_font, text_size, position, color);
    };

    const Vector2 canvas_size  = current_canvas_size();
    const Vector2 hud_location = Vector2{10};

    // Draw time remaining. Uses modulo division to cause blinking when the
    // player is running out of time.
    const double time_remaining = m_level->time_remaining();

    const auto minutes_string = std::to_string(int(time_remaining / 60.0));
    auto       seconds_string = std::to_string(int(std::fmod(time_remaining, 60)));

    if (seconds_string.size() == 1)
    {
        seconds_string.insert(0, "0");
    }

    const auto time_string = cer_fmt::format("Time: {}:{}", minutes_string, seconds_string);

    Color time_color;

    if (time_remaining > warning_time || m_level->is_exit_reached() ||
        (int(time_remaining) % 2) == 0)
    {
        time_color = yellow;
    }
    else
    {
        time_color = red;
    }

    draw_shadowed_string(time_string, hud_location, time_color);

    // Draw score
    const float time_height  = m_hud_font.measure(time_string, text_size).y;
    const auto  score_string = cer_fmt::format("Score: {}", m_total_score);

    draw_shadowed_string(score_string, hud_location + Vector2{0, time_height * 1.2f}, yellow);

    // Determine the status overlay message to show.
    Image status;

    if (is_zero(time_remaining))
    {
        status = m_level->is_exit_reached() ? m_win_overlay : m_lose_overlay;
    }
    else if (!m_level->player()->is_alive())
    {
        status = m_died_overlay;
    }

    if (status)
    {
        // Draw status message.
        draw_sprite(status, (canvas_size - status.size()) / 2);
    }
}

void PlatformerGame::load_next_level()
{
    m_level_index = (m_level_index + 1) % number_of_levels;

    // Unload the current level first before loading the next level.
    m_level.reset();

    m_level = load_custom_asset_of_type<Level>(cer_fmt::format("levels/{}.txt", m_level_index),
                                               Level::Args{
                                                   .score = &m_total_score,
                                               });
}

void PlatformerGame::reload_current_level()
{
    --m_level_index;
    load_next_level();
}
