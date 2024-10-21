// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include <cerlib/CopyMoveMacros.hpp>

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

class Stmt
{
  protected:
    explicit Stmt(const SourceLocation& location);

    virtual void on_verify(SemaContext& context, Scope& scope) = 0;

  public:
    forbid_copy_and_move(Stmt);

    virtual ~Stmt() noexcept;

    auto location() const -> const SourceLocation&;

    void verify(SemaContext& context, Scope& scope);

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

class CompoundStmt final : public Stmt
{
  public:
    CompoundStmt(const SourceLocation& location,
                 CompoundStmtKind      kind,
                 std::unique_ptr<Expr> lhs,
                 std::unique_ptr<Expr> rhs);

    forbid_copy_and_move(CompoundStmt);

    ~CompoundStmt() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto kind() const -> CompoundStmtKind;

    auto lhs() const -> const Expr&;

    auto rhs() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    CompoundStmtKind      m_kind;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class AssignmentStmt final : public Stmt
{
  public:
    AssignmentStmt(const SourceLocation& location,
                   std::unique_ptr<Expr> lhs,
                   std::unique_ptr<Expr> rhs);

    forbid_copy_and_move(AssignmentStmt);

    ~AssignmentStmt() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto lhs() const -> const Expr&;

    auto rhs() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class ReturnStmt final : public Stmt
{
  public:
    explicit ReturnStmt(const SourceLocation& location, std::unique_ptr<Expr> expr);

    forbid_copy_and_move(ReturnStmt);

    ~ReturnStmt() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto expr() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_expr;
};

class ForStmt final : public Stmt
{
  public:
    ForStmt(const SourceLocation&                location,
            std::unique_ptr<ForLoopVariableDecl> loop_variable,
            std::unique_ptr<RangeExpr>           range,
            std::unique_ptr<CodeBlock>           body);

    forbid_copy_and_move(ForStmt);

    ~ForStmt() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto loop_variable() const -> const ForLoopVariableDecl&;

    auto range() const -> const RangeExpr&;

    auto body() const -> const CodeBlock&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<ForLoopVariableDecl> m_loop_variable;
    std::unique_ptr<RangeExpr>           m_range;
    std::unique_ptr<CodeBlock>           m_body;
};

class IfStmt final : public Stmt
{
  public:
    IfStmt(const SourceLocation&      location,
           std::unique_ptr<Expr>      condition_expr,
           std::unique_ptr<CodeBlock> body,
           std::unique_ptr<IfStmt>    next);

    forbid_copy_and_move(IfStmt);

    ~IfStmt() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto condition_expr() const -> const Expr*;

    auto body() const -> const CodeBlock&;

    auto next() const -> const IfStmt*;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr>      m_condition_expr;
    std::unique_ptr<CodeBlock> m_body;
    std::unique_ptr<IfStmt>    m_next;
};

class VarStmt final : public Stmt
{
  public:
    explicit VarStmt(const SourceLocation& location, std::unique_ptr<VarDecl> variable);

    forbid_copy_and_move(VarStmt);

    ~VarStmt() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto name() const -> std::string_view;

    auto variable() const -> const VarDecl&;

    auto steal_variable() -> std::unique_ptr<VarDecl>;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<VarDecl> m_variable;
};
} // namespace cer::shadercompiler
