// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/LuaState.hpp"
#include "lua/sol.hpp"
#include "util/Object.hpp"

namespace cer::details
{
class LuaStateImpl final : public Object
{
  public:
    LuaStateImpl(LuaLibraries libraries_to_open, const List<LuaScript>& scripts_to_run);

    auto variable(std::string_view name) const -> std::optional<LuaValue>;

    void set_variable(std::string_view name, const std::optional<LuaValue>& value);

    void run_code(std::string_view code);

    void run_script(const LuaScript& script);

  private:
    sol::state m_lua_state;
};
} // namespace cer::details
