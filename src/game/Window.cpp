// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Window.hpp"

#include "GameImpl.hpp"
#include "WindowImpl.hpp"
#include "util/Util.hpp"

#ifdef CERLIB_HAVE_OPENGL
#include "graphics/opengl/OpenGLWindow.hpp"
#endif

#include <cassert>

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Window)

Window::Window(std::string_view        title,
               uint32_t                id,
               std::optional<int32_t>  position_x,
               std::optional<int32_t>  position_y,
               std::optional<uint32_t> width,
               std::optional<uint32_t> height,
               bool                    allow_high_dpi)
    : m_impl(nullptr)
{
    details::WindowImpl* impl{};

#ifdef CERLIB_HAVE_OPENGL
    impl = std::make_unique<details::OpenGLWindow>(title,
                                                   id,
                                                   position_x,
                                                   position_y,
                                                   width,
                                                   height,
                                                   allow_high_dpi)
               .release();
#else
    CER_THROW_RUNTIME_ERROR_STR("OpenGL is not available on this system.");
#endif

    assert(impl);

    set_impl(*this, impl);
}

uint32_t Window::id() const
{
    DECLARE_THIS_IMPL;
    return impl->id();
}

void Window::set_id(uint32_t value)
{
    DECLARE_THIS_IMPL;
    impl->set_id(value);
}

void Window::set_size(uint32_t width, uint32_t height)
{
    DECLARE_THIS_IMPL;
    impl->set_size(width, height);
}

float Window::width() const
{
    return size().x;
}

float Window::height() const
{
    return size().y;
}

Vector2 Window::size() const
{
    DECLARE_THIS_IMPL;
    return impl->size();
}

float Window::width_px() const
{
    return size_px().x;
}

float Window::height_px() const
{
    return size_px().y;
}

Vector2 Window::size_px() const
{
    DECLARE_THIS_IMPL;
    return impl->size_px();
}

float Window::pixel_ratio() const
{
    DECLARE_THIS_IMPL;
    return impl->pixel_ratio();
}

std::string_view Window::title() const
{
    DECLARE_THIS_IMPL;
    return impl->title();
}

void Window::set_title(std::string_view value)
{
    DECLARE_THIS_IMPL;
    impl->set_title(value);
}

void Window::set_visible(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_visible(value);
}

void Window::set_always_on_top(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_always_on_top(value);
}

void Window::set_bordered(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_bordered(value);
}

void Window::set_full_screen(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_full_screen(value);
}

void Window::set_resizable(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_resizable(value);
}

void Window::minimize()
{
    DECLARE_THIS_IMPL;
    impl->minimize();
}

void Window::maximize()
{
    DECLARE_THIS_IMPL;
    impl->maximize();
}

void Window::show()
{
    DECLARE_THIS_IMPL;
    impl->show();
}

void Window::hide()
{
    DECLARE_THIS_IMPL;
    impl->hide();
}

void Window::set_minimum_size(uint32_t width, uint32_t height)
{
    DECLARE_THIS_IMPL;
    impl->set_minimum_size(width, height);
}

void Window::set_maximum_size(uint32_t width, uint32_t height)
{
    DECLARE_THIS_IMPL;
    impl->set_maximum_size(width, height);
}

void Window::set_mouse_grab(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_mouse_grab(value);
}

void Window::set_position(int32_t x, int32_t y)
{
    DECLARE_THIS_IMPL;
    impl->set_position(x, y);
}

void Window::set_resize_callback(const ResizeCallback& value)
{
    DECLARE_THIS_IMPL;
    impl->set_resize_callback(value);
}

uint32_t Window::display_index() const
{
    DECLARE_THIS_IMPL;
    return impl->display_index();
}

uint32_t Window::sync_interval() const
{
    DECLARE_THIS_IMPL;
    return impl->sync_interval();
}

void Window::set_sync_interval(uint32_t value)
{
    DECLARE_THIS_IMPL;
    impl->set_sync_interval(value);
}

void Window::set_clear_color(std::optional<Color> value)
{
    DECLARE_THIS_IMPL;
    impl->set_clear_color(value);
}

std::optional<Color> Window::clear_color() const
{
    DECLARE_THIS_IMPL;
    return impl->clear_color();
}

void Window::show_message_box(MessageBoxType   type,
                              std::string_view title,
                              std::string_view message,
                              Window           parent_window)
{
    impl_t::show_message_box(type, title, message, parent_window);
}
} // namespace cer