// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "cerlib/LuaScript.hpp"
#include "util/Object.hpp"

namespace cer::details
{
class LuaScriptImpl final : public Object, public Asset
{
  public:
    explicit LuaScriptImpl(std::string name, std::string code);

    auto name() const -> std::string_view;

    auto code() const -> std::string_view;

  private:
    std::string m_name;
    std::string m_code;
};
} // namespace cer::details
