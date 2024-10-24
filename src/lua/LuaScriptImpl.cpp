// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "lua/LuaScriptImpl.hpp"

namespace cer::details
{
LuaScriptImpl::LuaScriptImpl(std::string name, std::string code)
    : m_name(std::move(name))
    , m_code(std::move(code))
{
}

auto LuaScriptImpl::name() const -> std::string_view
{
    return m_name;
}

auto LuaScriptImpl::code() const -> std::string_view
{
    return m_code;
}
} // namespace cer::details
