// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Input.hpp>

#include "InputImpl.hpp"

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

auto cer::is_key_down(Key key) -> bool
{
    return details::InputImpl::instance().is_key_down(key);
}

auto cer::is_key_up(Key key) -> bool
{
    return !is_key_down(key);
}

auto cer::was_key_just_pressed(Key key) -> bool
{
    return details::InputImpl::instance().was_key_just_pressed(key);
}

auto cer::was_key_just_released(Key key) -> bool
{
    return details::InputImpl::instance().was_key_just_released(key);
}

auto cer::is_mouse_button_down(MouseButton button) -> bool
{
    return details::InputImpl::instance().is_mouse_button_down(button);
}

auto cer::is_mouse_button_up(MouseButton button) -> bool
{
    return !is_mouse_button_down(button);
}

auto cer::current_mouse_position() -> cer::Vector2
{
#ifdef __EMSCRIPTEN__
    int X{}, Y{};
    SDL_GetMouseState(&X, &Y);
    return {float(X), float(Y)};
#else
    auto x = 0.0f;
    auto y = 0.0f;
    SDL_GetMouseState(&x, &y);
    return {x, y};
#endif
}

auto cer::current_mouse_position_delta() -> cer::Vector2
{
    return details::InputImpl::instance().mouse_position_delta();
}

auto cer::current_mouse_wheel_delta() -> cer::Vector2
{
    return details::InputImpl::instance().mouse_wheel_delta();
}
