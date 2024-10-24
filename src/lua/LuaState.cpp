// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/LuaState.hpp"
#include "contentmanagement/ContentManager.hpp"
#include "lua/LuaStateImpl.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(LuaState);

LuaState::LuaState(LuaLibraries libraries_to_include, const List<LuaScript>& scripts)
    : LuaState()
{
    auto impl = std::make_unique<details::LuaStateImpl>(libraries_to_include, scripts);
    set_impl(*this, impl.release());
}

auto LuaState::variable(std::string_view name) const -> std::optional<LuaValue>
{
    DECLARE_THIS_IMPL_OR_RETURN_VALUE(std::nullopt);
    return impl->variable(name);
}

void LuaState::set_variable(std::string_view name, const std::optional<LuaValue>& value)
{
    DECLARE_THIS_IMPL_OR_RETURN;
    impl->set_variable(name, value);
}

void LuaState::run_code(std::string_view code)
{
    DECLARE_THIS_IMPL_OR_RETURN;
    impl->run_code(code);
}

void LuaState::run_script(const LuaScript& script)
{
    DECLARE_THIS_IMPL_OR_RETURN;
    impl->run_script(script);
}
} // namespace cer
