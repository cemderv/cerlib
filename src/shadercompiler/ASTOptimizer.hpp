// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/TempVarNameGen.hpp"

#include <cerlib/HashMap.hpp>

namespace cer::shadercompiler
{
class AST;
class Stmt;
class CodeBlock;
class FunctionCallExpr;

class ASTOptimizer
{
  public:
    void optimize(AST& ast);

  private:
    auto remove_unused_functions(AST& ast) const -> bool;

    auto remove_unused_structs(AST& ast) const -> bool;

    auto optimize_block(CodeBlock* block) -> bool;

    static auto remove_unused_variables(CodeBlock* block) -> bool;

    HashMap<const CodeBlock*, TempVarNameGen> m_code_block_name_gens{};
};
} // namespace cer::shadercompiler
