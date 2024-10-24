// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
namespace details
{
class LuaScriptImpl;
}

struct LuaCode
{
    constexpr explicit LuaCode(std::string_view code)
        : code(code)
    {
    }

    std::string_view code;
};

class LuaScript final
{
    CERLIB_DECLARE_OBJECT(LuaScript);

  public:
    explicit LuaScript(std::string_view asset_name);

    explicit LuaScript(std::string_view name, LuaCode code);

    auto name() const -> std::string_view;

    auto code() const -> std::string_view;
};
} // namespace cer

constexpr auto operator""_lua(const char* code, size_t length) -> cer::LuaCode
{
    return cer::LuaCode{std::string_view{code, length}};
}
