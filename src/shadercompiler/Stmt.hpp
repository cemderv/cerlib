// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"

#include <memory>

namespace cer::shadercompiler
{
class SemaContext;
class Decl;
class Expr;
class Scope;
class CodeBlock;
class RangeExpr;
class VarDecl;
class TempVarNameGen;
class ForLoopVariableDecl;

class CERLIB_API_INTERNAL Stmt
{
  protected:
    explicit Stmt(const SourceLocation& location);

    virtual void on_verify(SemaContext& context, Scope& scope) = 0;

  public:
    NON_COPYABLE_NON_MOVABLE(Stmt);

    virtual ~Stmt() noexcept;

    auto location() const -> const SourceLocation&;

    auto verify(SemaContext& context, Scope& scope) -> void;

    virtual auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool = 0;

  private:
    SourceLocation m_location;
    bool           m_is_verified = false;
};

enum class CompoundStmtKind
{
    Add,
    Sub,
    Mul,
    Div,
};

class CERLIB_API_INTERNAL CompoundStmt final : public Stmt
{
  public:
    CompoundStmt(const SourceLocation& location,
                 CompoundStmtKind      kind,
                 std::unique_ptr<Expr> lhs,
                 std::unique_ptr<Expr> rhs);

    NON_COPYABLE_NON_MOVABLE(CompoundStmt);

    ~CompoundStmt() noexcept override;

    auto on_verify(SemaContext& context, Scope& scope) -> void override;

    auto kind() const -> CompoundStmtKind;

    auto lhs() const -> const Expr&;

    auto rhs() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    CompoundStmtKind      m_kind;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class CERLIB_API_INTERNAL AssignmentStmt final : public Stmt
{
  public:
    AssignmentStmt(const SourceLocation& location,
                   std::unique_ptr<Expr> lhs,
                   std::unique_ptr<Expr> rhs);

    NON_COPYABLE_NON_MOVABLE(AssignmentStmt);

    ~AssignmentStmt() noexcept override;

    auto on_verify(SemaContext& context, Scope& scope) -> void override;

    auto lhs() const -> const Expr&;

    auto rhs() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class CERLIB_API_INTERNAL ReturnStmt final : public Stmt
{
  public:
    explicit ReturnStmt(const SourceLocation& location, std::unique_ptr<Expr> expr);

    NON_COPYABLE_NON_MOVABLE(ReturnStmt);

    ~ReturnStmt() noexcept override;

    auto on_verify(SemaContext& context, Scope& scope) -> void override;

    auto expr() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_expr;
};

class CERLIB_API_INTERNAL ForStmt final : public Stmt
{
  public:
    ForStmt(const SourceLocation&                location,
            std::unique_ptr<ForLoopVariableDecl> loop_variable,
            std::unique_ptr<RangeExpr>           range,
            std::unique_ptr<CodeBlock>           body);

    NON_COPYABLE_NON_MOVABLE(ForStmt);

    ~ForStmt() noexcept override;

    auto on_verify(SemaContext& context, Scope& scope) -> void override;

    auto loop_variable() const -> const ForLoopVariableDecl&;

    auto range() const -> const RangeExpr&;

    auto body() const -> const CodeBlock&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<ForLoopVariableDecl> m_loop_variable;
    std::unique_ptr<RangeExpr>           m_range;
    std::unique_ptr<CodeBlock>           m_body;
};

class CERLIB_API_INTERNAL IfStmt final : public Stmt
{
  public:
    IfStmt(const SourceLocation&      location,
           std::unique_ptr<Expr>      condition_expr,
           std::unique_ptr<CodeBlock> body,
           std::unique_ptr<IfStmt>    next);

    NON_COPYABLE_NON_MOVABLE(IfStmt);

    ~IfStmt() noexcept override;

    auto on_verify(SemaContext& context, Scope& scope) -> void override;

    auto condition_expr() const -> const Expr*;

    auto body() const -> const CodeBlock&;

    auto next() const -> const IfStmt*;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr>      m_condition_expr;
    std::unique_ptr<CodeBlock> m_body;
    std::unique_ptr<IfStmt>    m_next;
};

class CERLIB_API_INTERNAL VarStmt final : public Stmt
{
  public:
    explicit VarStmt(const SourceLocation& location, std::unique_ptr<VarDecl> variable);

    NON_COPYABLE_NON_MOVABLE(VarStmt);

    ~VarStmt() noexcept override;

    auto on_verify(SemaContext& context, Scope& scope) -> void override;

    auto name() const -> std::string_view;

    auto variable() const -> const VarDecl&;

    auto steal_variable() -> std::unique_ptr<VarDecl>;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<VarDecl> m_variable;
};
} // namespace cer::shadercompiler
