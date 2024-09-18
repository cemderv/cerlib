#pragma once

#include "Level.hpp"
#include <cerlib.hpp>

class PlatformerGame final : public cer::Game
{
  public:
    PlatformerGame();

  private:
    void load_content() override;

    bool update(cer::GameTime time) override;

    void draw(cer::Window window) override;

    void draw_hud();

    void load_next_level();

    void reload_current_level();

    cer::Window m_window;
    cer::Image  m_canvas;

    double m_time_accumulator = 0.0;

    // Global content
    cer::Font  m_hud_font;
    cer::Image m_win_overlay;
    cer::Image m_lose_overlay;
    cer::Image m_died_overlay;

    // Meta-level game state.
    static constexpr int number_of_levels = 6;

    int                    m_level_index = -1;
    std::shared_ptr<Level> m_level;

    int m_total_score = 0;
};