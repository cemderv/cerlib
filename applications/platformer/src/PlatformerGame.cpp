#include "PlatformerGame.hpp"

// When the time remaining is less than the warning time, it blinks on the hud
constexpr auto warning_time = 10.0f;

PlatformerGame::PlatformerGame()
{
    // The original game is written for a fixed resolution of 800x480 pixels.
    constexpr cer::Vector2 resolution{800, 480};

    m_window =
        cer::Window{"Platformer Game", 0, {}, {}, uint32_t(resolution.x), uint32_t(resolution.y)};

    m_window.set_resizable(false);
    m_window.set_clear_color(cer::green);

    if (m_window.pixel_ratio() != 1.0f)
    {
        m_canvas = cer::Image{uint32_t(resolution.x),
                              uint32_t(resolution.y),
                              cer::ImageFormat::R8G8B8A8_UNorm,
                              m_window};
    }

    cer::register_custom_asset_loader_for_type<Level>(
        [](std::string_view name, const cer::AssetData& data, const std::any& extra_info) {
            return std::make_shared<Level>(name,
                                           data.as_string_view(),
                                           std::any_cast<Level::Args>(extra_info));
        });
}

void PlatformerGame::load_content()
{
    m_hud_font = cer::Font::built_in();

    m_win_overlay  = cer::Image{"overlays/you_win.png"};
    m_lose_overlay = cer::Image{"overlays/you_lose.png"};
    m_died_overlay = cer::Image{"overlays/you_died.png"};

    cer::SoundChannel music_channel = cer::play_sound_in_background(cer::Sound{"sounds/music.mp3"});

    music_channel.set_looping(true);

    load_next_level();
}

bool PlatformerGame::update(const cer::GameTime& time)
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

    cer::GameTime corrected_time{time};

    corrected_time.elapsed_time = target_elapsed_time;
    m_time_accumulator -= target_elapsed_time;

    if (was_key_just_pressed(cer::Key::Escape))
    {
        return false;
    }

    if (was_key_just_pressed(cer::Key::Space))
    {
        if (!m_level->player()->is_alive())
        {
            m_total_score = 0;
            m_level_index = -1;
            load_next_level();
        }
        else if (cer::is_zero(m_level->time_remaining()))
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

void PlatformerGame::draw(const cer::Window& window)
{
    if (m_canvas)
    {
        set_blend_state(cer::non_premultiplied);
        m_canvas.set_canvas_clear_color(cer::red);
        set_canvas(m_canvas);
    }

    m_level->draw();
    draw_hud();

    if (m_canvas)
    {
        cer::set_blend_state(cer::opaque);
        cer::set_canvas({});

        cer::draw_sprite({
            .image    = m_canvas,
            .dst_rect = {0, 0, cer::current_canvas_size()},
        });
    }
}

void PlatformerGame::draw_hud()
{
    constexpr uint32_t text_size = 20;

    const auto draw_shadowed_string =
        [&](std::string_view text, cer::Vector2 position, cer::Color color) {
            draw_string(text, m_hud_font, text_size, position + cer::Vector2{1, 1}, cer::black);
            draw_string(text, m_hud_font, text_size, position, color);
        };

    const cer::Vector2 canvas_size  = cer::current_canvas_size();
    const cer::Vector2 hud_location = cer::Vector2{10};

    // Draw time remaining. Uses modulo division to cause blinking when the
    // player is running out of time.
    const double time_remaining = m_level->time_remaining();

    const std::string minutes_string = std::to_string(int(time_remaining / 60.0));
    std::string       seconds_string = std::to_string(int(std::fmod(time_remaining, 60)));

    if (seconds_string.size() == 1)
    {
        seconds_string.insert(0, "0");
    }

    const std::string time_string = cer_fmt::format("Time: {}:{}", minutes_string, seconds_string);

    cer::Color time_color;

    if (time_remaining > warning_time || m_level->is_exit_reached() ||
        (int(time_remaining) % 2) == 0)
    {
        time_color = cer::yellow;
    }
    else
    {
        time_color = cer::red;
    }

    draw_shadowed_string(time_string, hud_location, time_color);

    // Draw score
    const float       time_height  = m_hud_font.measure(time_string, text_size).y;
    const std::string score_string = cer_fmt::format("Score: {}", m_total_score);

    draw_shadowed_string(score_string,
                         hud_location + cer::Vector2{0, time_height * 1.2f},
                         cer::yellow);

    // Determine the status overlay message to show.
    cer::Image status;

    if (cer::is_zero(time_remaining))
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

    m_level = cer::load_custom_asset_of_type<Level>(cer_fmt::format("levels/{}.txt", m_level_index),
                                                    Level::Args{
                                                        .score = &m_total_score,
                                                    });
}

void PlatformerGame::reload_current_level()
{
    --m_level_index;
    load_next_level();
}
