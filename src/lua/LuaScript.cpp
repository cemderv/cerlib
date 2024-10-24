// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/LuaScript.hpp"
#include "contentmanagement/ContentManager.hpp"
#include "game/GameImpl.hpp"
#include "lua/LuaScriptImpl.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(LuaScript);

LuaScript::LuaScript(std::string_view asset_name)
    : LuaScript()
{
    auto& content = details::GameImpl::instance().content_manager();
    *this         = content.load_lua_script(asset_name);
}

LuaScript::LuaScript(std::string_view name, LuaCode code)
    : LuaScript()
{
    auto impl = std::make_unique<details::LuaScriptImpl>(std::string{name}, std::string{code.code});

    set_impl(*this, impl.release());
}

auto LuaScript::name() const -> std::string_view
{
    DECLARE_THIS_IMPL_OR_RETURN_VALUE(std::string_view{});
    return impl->name();
}

auto LuaScript::code() const -> std::string_view
{
    DECLARE_THIS_IMPL_OR_RETURN_VALUE(std::string_view{});
    return impl->code();
}
} // namespace cer
