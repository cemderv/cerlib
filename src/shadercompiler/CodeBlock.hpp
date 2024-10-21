// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
#include <span>

namespace cer::shadercompiler
{
class SemaContext;
class Stmt;
class Scope;
class Decl;
class VarStmt;
class Expr;
class TempVarNameGen;

class CodeBlock final
{
  public:
    using StmtsType = UniquePtrList<Stmt, 16>;

    explicit CodeBlock(const SourceLocation& location, StmtsType stmts);

    forbid_copy_and_move(CodeBlock);

    ~CodeBlock() noexcept;

    void verify(SemaContext&                                        context,
                Scope&                                              scope,
                std::span<const std::reference_wrapper<const Decl>> extra_symbols) const;

    auto variables() const -> RefList<VarStmt, 8>;

    auto location() const -> const SourceLocation&;

    auto stmts() const -> const StmtsType&;

    void remove_stmt(const Stmt& stmt);

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool;

  private:
    SourceLocation m_location;
    StmtsType      m_stmts;
};
} // namespace cer::shadercompiler
