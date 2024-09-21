// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "RenderingTestHelper.hpp"
#include <cerlib/Drawing.hpp>
#include <cerlib/Game.hpp>
#include <cerlib/OStreamCompat.hpp>
#include <cerlib/Shader.hpp>
#include <format>
#include <snitch/snitch.hpp>

using namespace cer;

class MockGame final : public Game
{
  public:
    MockGame()
        : m_window("Unit Test Window", 0, {}, {}, 300, 300, false)
        , m_rendering_test_helper(640, 480, m_window)
    {
    }

    void load_content() override
    {
        m_logo             = Image(std::format("{}/cerlib-logo300.png", TEST_ASSETS_DIR));
        m_grayscale_shader = Shader::create_grayscale();
    }

    bool update(const GameTime& time) override
    {
        return !m_have_executed_tests;
    }

    void draw(const Window& window) override
    {
        SECTION("basic_sprite")
        {
            m_rendering_test_helper.test_render("basic_sprite", [&] {
                draw_sprite(m_logo, {50, 50});
                draw_sprite(m_logo, {250, 250});
            });
        }

        SECTION("grayscale shader")
        {
            m_rendering_test_helper.test_render("grayscale_shader", [&] {
                set_sprite_shader(m_grayscale_shader);
                draw_sprite(m_logo, {50, 50});
                set_sprite_shader({});
            });
        }

        m_have_executed_tests = true;
    }

  private:
    Window              m_window;
    Image               m_logo;
    Shader              m_grayscale_shader;
    RenderingTestHelper m_rendering_test_helper;
    bool                m_have_executed_tests = false;
};

TEST_CASE("SpriteRenderingTests", "[drawing]")
{
    std::ignore = cer::run_game<MockGame>();
}
