// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Window.hpp"
#include "util/Object.hpp"

#include <cerlib/Option.hpp>
#include <cerlib/String.hpp>

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

namespace cer::details
{
class WindowImpl : public Object
{
  public:
    explicit WindowImpl(std::string_view title,
                        uint32_t         id,
                        Option<int32_t>  position_x,
                        Option<int32_t>  position_y,
                        Option<uint32_t> width,
                        Option<uint32_t> height,
                        bool             allow_high_dpi);

    forbid_copy_and_move(WindowImpl);

    ~WindowImpl() noexcept override;

    auto id() const -> uint32_t;

    void set_id(uint32_t value);

    auto size() const -> Vector2;

    auto size_px() const -> Vector2;

    auto pixel_ratio() const -> float;

    auto title() const -> std::string_view;

    void set_title(std::string_view value);

    void set_visible(bool value);

    void set_always_on_top(bool value);

    void set_bordered(bool value);

    void set_full_screen(bool value);

    void set_resizable(bool value);

    void minimize();

    void maximize();

    void show();

    void hide();

    void set_minimum_size(uint32_t width, uint32_t height);

    void set_maximum_size(uint32_t width, uint32_t height);

    void set_mouse_grab(bool value);

    void set_position(int32_t x, int32_t y);

    void set_size(uint32_t width, uint32_t height);

    void set_resize_callback(const Window::ResizeCallback& value);

    auto display_index() const -> uint32_t;

    auto sdl_window() const -> SDL_Window*;

    auto sync_interval() const -> uint32_t;

    void set_sync_interval(uint32_t value);

    void set_clear_color(Option<Color> value);

    auto clear_color() const -> Option<Color>;

    virtual void handle_resize_event() = 0;

    static void show_message_box(MessageBoxType   type,
                                 std::string_view title,
                                 std::string_view message,
                                 const Window&    parent_window);

    void activate_onscreen_keyboard();

    void deactivate_onscreen_keyboard();

  protected:
    void create_sdl_window(int additional_flags);

  private:
    String        m_initial_title;
    Option<int>   m_initial_position_x;
    Option<int>   m_initial_position_y;
    Option<int>   m_initial_width;
    Option<int>   m_initial_height;
    bool          m_allow_high_dpi;
    SDL_Window*   m_sdl_window;
    uint32_t      m_id;
    uint32_t      m_sync_interval;
    Option<Color> m_clear_color;

  protected:
    Window::ResizeCallback m_resize_callback; // NOLINT
};
} // namespace cer::details