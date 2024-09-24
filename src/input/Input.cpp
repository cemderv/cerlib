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

bool cer::is_key_down(Key key)
{
    return details::InputImpl::instance().is_key_down(key);
}

bool cer::is_key_up(Key key)
{
    return !is_key_down(key);
}

bool cer::was_key_just_pressed(Key key)
{
    return details::InputImpl::instance().was_key_just_pressed(key);
}

bool cer::was_key_just_released(Key key)
{
    return details::InputImpl::instance().was_key_just_released(key);
}

bool cer::is_mouse_button_down(MouseButton button)
{
    return details::InputImpl::instance().is_mouse_button_down(button);
}

bool cer::is_mouse_button_up(MouseButton button)
{
    return !is_mouse_button_down(button);
}

cer::Vector2 cer::current_mouse_position()
{
#ifdef __EMSCRIPTEN__
    int X{}, Y{};
    SDL_GetMouseState(&X, &Y);
    return {float(X), float(Y)};
#else
    float x{};
    float y{};
    SDL_GetMouseState(&x, &y);
    return {x, y};
#endif
}

cer::Vector2 cer::current_mouse_position_delta()
{
    return details::InputImpl::instance().mouse_position_delta();
}

cer::Vector2 cer::current_mouse_wheel_delta()
{
    return details::InputImpl::instance().mouse_wheel_delta();
}
