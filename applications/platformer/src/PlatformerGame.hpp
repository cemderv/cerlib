#pragma once

#include "Level.hpp"
#include <cerlib.hpp>

using namespace cer;

class PlatformerGame final : public Game
{
  public:
    PlatformerGame();

  private:
    void load_content() override;

    bool update(const GameTime& time) override;

    void draw(const Window& window) override;

    void draw_hud();

    void load_next_level();

    void reload_current_level();

    Window m_window;
    Image  m_canvas;

    double m_time_accumulator = 0.0;

    // Global content
    Font  m_hud_font;
    Image m_win_overlay;
    Image m_lose_overlay;
    Image m_died_overlay;

    // Meta-level game state.
    static constexpr int number_of_levels = 6;

    int              m_level_index = -1;
    SharedPtr<Level> m_level;

    int m_total_score = 0;
};