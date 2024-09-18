// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/TempVarNameGen.hpp"

#include <unordered_map>

namespace cer::shadercompiler
{
class AST;
class Stmt;
class CodeBlock;
class FunctionCallExpr;

class ASTOptimizer
{
  public:
    auto optimize(AST& ast) -> void;

  private:
    bool remove_unused_functions(AST& ast) const;

    bool remove_unused_structs(AST& ast) const;

    bool optimize_block(CodeBlock* block);

    static bool remove_unused_variables(CodeBlock* block);

    std::unordered_map<const CodeBlock*, TempVarNameGen> m_code_block_name_gens{};
};
} // namespace cer::shadercompiler
