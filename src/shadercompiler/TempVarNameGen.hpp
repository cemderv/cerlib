// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <string>

namespace cer::shadercompiler
{
class CodeBlock;

class TempVarNameGen final
{
  public:
    explicit TempVarNameGen(const CodeBlock* block = nullptr);

    std::string next(std::string_view hint = {});

  private:
    std::string m_prefix;
    int         m_counter;
};
} // namespace cer::shadercompiler
