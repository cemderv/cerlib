// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "lua/LuaStateImpl.hpp"
#include "cerlib/Defer.hpp"
#include "cerlib/Drawing.hpp"
#include "cerlib/LuaScript.hpp"
#include "cerlib/Util.hpp"
#include <stdexcept>

namespace cer::details
{
static void open_cerlib_api(sol::state& l);

LuaStateImpl::LuaStateImpl([[maybe_unused]] LuaLibraries libraries_to_open,
                           const List<LuaScript>&        scripts_to_run)
{
    // m_lua_state.open_libraries();

    m_lua_state.open_libraries(sol::lib::base);
    m_lua_state.open_libraries(sol::lib::coroutine);
    m_lua_state.open_libraries(sol::lib::string);
    m_lua_state.open_libraries(sol::lib::math);
    m_lua_state.open_libraries(sol::lib::table);

#ifndef NDEBUG
    m_lua_state.open_libraries(sol::lib::debug);
#endif

    open_cerlib_api(m_lua_state);

    for (size_t i = 0; const auto& script : scripts_to_run)
    {
        if (!script)
        {
            throw std::invalid_argument{fmt::format("No script specified at index {}", i)};
        }

        run_script(script);
        ++i;
    }
}

auto LuaStateImpl::variable(std::string_view name) const -> std::optional<LuaValue>
{
    const auto str = std::string{name};
    const auto l   = m_lua_state.lua_state();

    lua_getglobal(l, str.c_str());

    defer
    {
        lua_pop(l, 1);
    };

    if (lua_isnumber(l, -1) != 0)
    {
        const auto num = lua_tonumber(l, -1);
        return LuaValue{double(num)};
    }

    if (lua_isboolean(l, -1) != 0)
    {
        const auto val = lua_toboolean(l, -1);
        return LuaValue{val != 0};
    }

    if (lua_isstring(l, -1) != 0)
    {
        const auto str = lua_tostring(l, -1);
        return LuaValue{std::string{str}};
    }

    return std::nullopt;
}

void LuaStateImpl::set_variable(std::string_view name, const std::optional<LuaValue>& value)
{
    if (value.has_value())
    {
        std::visit(
            util::VariantSwitch{
                [this, name](const auto& val) {
                    m_lua_state[name] = val;
                },
            },
            *value);
    }
    else
    {
        m_lua_state[name] = sol::lua_nil;
    }
}

void LuaStateImpl::run_code(std::string_view code)
{
    m_lua_state.script(code);
}

void LuaStateImpl::run_script(const LuaScript& script)
{
    if (!script)
    {
        throw std::invalid_argument("No script specified.");
    }

    m_lua_state.script(script.code());
}

void open_cerlib_api(sol::state& l)
{
    l.new_usertype<Vector2>("Vector2",
                            sol::call_constructor,
                            sol::factories(
                                [] {
                                    return Vector2{};
                                },
                                [](float x, float y) {
                                    return Vector2{x, y};
                                },
                                [](float xy) {
                                    return Vector2{xy};
                                }),
                            "x",
                            &Vector2::x,
                            "y",
                            &Vector2::y);

    l.new_usertype<Vector3>("Vector3", "x", &Vector3::x, "y", &Vector3::y, "z", &Vector3::z);

    l.new_usertype<Vector4>("Vector4",
                            "x",
                            &Vector4::x,
                            "y",
                            &Vector4::y,
                            "z",
                            &Vector4::z,
                            "w",
                            &Vector4::w);

    l.new_usertype<Color>("Color", "r", &Color::r, "g", &Color::g, "b", &Color::b, "a", &Color::a);

    l["color_white"] = white;

    l.new_enum("SpriteFlip",
               "None",
               SpriteFlip::None,
               "Vertically",
               SpriteFlip::Vertically,
               "Horizontally",
               SpriteFlip::Horizontally,
               "Both",
               SpriteFlip::Both);

    l.new_usertype<Sprite>("Sprite",
                           "image",
                           &Sprite::image,
                           "dst_rect",
                           &Sprite::dst_rect,
                           "src_rect",
                           &Sprite::src_rect,
                           "color",
                           &Sprite::color,
                           "rotation",
                           &Sprite::rotation,
                           "origin",
                           &Sprite::origin,
                           "scale",
                           &Sprite::scale,
                           "flip",
                           &Sprite::flip);

    l.new_usertype<Image>("Image",
                          sol::call_constructor,
                          sol::factories([](std::string_view asset_name) {
                              return Image{asset_name};
                          }),
                          "size",
                          &Image::size);

    l.new_usertype<Window>("Window", "size_px", &Window::size_px);

    l.new_usertype<GameTime>("GameTime",
                             "elapsed_time",
                             &GameTime::elapsed_time,
                             "total_time",
                             &GameTime::total_time);

    l.set_function("draw_sprite", [](const Sprite& sprite) {
        draw_sprite(sprite);
    });

    l.set_function(
        "draw_sprite_simple",
        [](const Image& image, const Vector2& position, const std::optional<Color>& color) {
            draw_sprite(image, position, color.value_or(white));
        });
}
} // namespace cer::details
