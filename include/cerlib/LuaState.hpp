// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Game.hpp>
#include <cerlib/List.hpp>
#include <cerlib/LuaScript.hpp>
#include <cerlib/Window.hpp>
#include <cerlib/details/ObjectMacros.hpp>
#include <optional>
#include <string>
#include <variant>

namespace cer
{
class LuaScript;

namespace details
{
class LuaStateImpl;
}

enum class LuaLibraries
{
    All,
};

using LuaValue = std::variant<double, bool, std::string, GameTime, Window>;

class LuaState final
{
    CERLIB_DECLARE_OBJECT(LuaState);

  public:
    explicit LuaState(LuaLibraries libraries_to_include, const List<LuaScript>& scripts = {});

    auto variable(std::string_view name) const -> std::optional<LuaValue>;

    template <typename T>
    auto variable_as(std::string_view name) const -> std::optional<T>;

    void set_variable(std::string_view name, const std::optional<LuaValue>& value);

    void run_code(std::string_view code);

    void run_script(const LuaScript& script);
};

template <typename T>
auto LuaState::variable_as(std::string_view name) const -> std::optional<T>
{
    auto val = variable(name);

    if (!val.has_value())
    {
        return std::nullopt;
    }

    return std::make_optional(std::get<T>(std::move(*val)));
}
} // namespace cer
