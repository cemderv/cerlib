// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <gsl/pointers>
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

class CERLIB_API_INTERNAL CodeBlock final
{
  public:
    using StmtsType = SmallVector<std::unique_ptr<Stmt>, 16>;

    explicit CodeBlock(const SourceLocation& location, StmtsType stmts);

    NON_COPYABLE_NON_MOVABLE(CodeBlock);

    ~CodeBlock() noexcept;

    void verify(SemaContext&                                context,
                Scope&                                      scope,
                std::span<const gsl::not_null<const Decl*>> extra_symbols) const;

    SmallVector<gsl::not_null<VarStmt*>, 8> variables() const;

    const SourceLocation& location() const;

    const StmtsType& stmts() const;

    void remove_stmt(const Stmt& stmt);

    bool accesses_symbol(const Decl& symbol, bool transitive) const;

  private:
    SourceLocation m_location;
    StmtsType      m_stmts;
};
} // namespace cer::shadercompiler
