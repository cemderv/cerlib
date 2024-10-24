// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/String.hpp>

namespace cer::shadercompiler
{
class CodeBlock;

class TempVarNameGen final
{
  public:
    explicit TempVarNameGen(const CodeBlock* block = nullptr);

    auto next(std::string_view hint = {}) -> String;

  private:
    String m_prefix;
    int    m_counter;
};
} // namespace cer::shadercompiler
