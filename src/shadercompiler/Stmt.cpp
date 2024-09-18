// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Stmt.hpp"

#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/SemaContext.hpp"

#include <cassert>

namespace cer::shadercompiler
{
Stmt::~Stmt() noexcept = default;

const SourceLocation& Stmt::location() const
{
    return m_location;
}

void Stmt::verify(SemaContext& context, Scope& scope)
{
    if (!m_is_verified)
    {
        on_verify(context, scope);
        m_is_verified = true;
    }
}

Stmt::Stmt(const SourceLocation& location)
    : m_location(location)
{
}

VarStmt::VarStmt(const SourceLocation& location, std::unique_ptr<VarDecl> variable)
    : Stmt(location)
    , m_variable(std::move(variable))
{
}

VarStmt::~VarStmt() noexcept = default;

void VarStmt::on_verify(SemaContext& context, Scope& scope)
{
    m_variable->verify(context, scope);
}

std::string_view VarStmt::name() const
{
    return m_variable->name();
}

const VarDecl& VarStmt::variable() const
{
    return *m_variable;
}

std::unique_ptr<VarDecl> VarStmt::steal_variable()
{
    return std::move(m_variable);
}

bool VarStmt::accesses_symbol(const Decl& symbol, bool transitive) const
{
    return m_variable->expr().accesses_symbol(symbol, true);
}

ReturnStmt::ReturnStmt(const SourceLocation& location, std::unique_ptr<Expr> expr)
    : Stmt(location)
    , m_expr(std::move(expr))
{
    assert(m_expr);
}

ReturnStmt::~ReturnStmt() noexcept = default;

void ReturnStmt::on_verify(SemaContext& context, Scope& scope)
{
    m_expr->verify(context, scope);
}

const Expr& ReturnStmt::expr() const
{
    return *m_expr;
}

CompoundStmt::CompoundStmt(const SourceLocation& location,
                           CompoundStmtKind      kind,
                           std::unique_ptr<Expr> lhs,
                           std::unique_ptr<Expr> rhs)
    : Stmt(location)
    , m_kind(kind)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{
    assert(m_lhs);
    assert(m_rhs);
}

CompoundStmt::~CompoundStmt() noexcept = default;

void CompoundStmt::on_verify(SemaContext& context, Scope& scope)
{
    m_lhs->verify(context, scope);
    m_rhs->verify(context, scope);
}

CompoundStmtKind CompoundStmt::kind() const
{
    return m_kind;
}

const Expr& CompoundStmt::lhs() const
{
    return *m_lhs;
}

const Expr& CompoundStmt::rhs() const
{
    return *m_rhs;
}

bool CompoundStmt::accesses_symbol(const Decl& symbol, bool transitive) const
{
    return m_lhs->accesses_symbol(symbol, transitive) || m_rhs->accesses_symbol(symbol, transitive);
}

bool ReturnStmt::accesses_symbol(const Decl& symbol, bool transitive) const
{
    return m_expr->accesses_symbol(symbol, transitive);
}

ForStmt::ForStmt(const SourceLocation&                location,
                 std::unique_ptr<ForLoopVariableDecl> loop_variable,
                 std::unique_ptr<RangeExpr>           range,
                 std::unique_ptr<CodeBlock>           body)
    : Stmt(location)
    , m_loop_variable(std::move(loop_variable))
    , m_range(std::move(range))
    , m_body(std::move(body))
{
    assert(m_range);
    assert(m_body);
}

ForStmt::~ForStmt() noexcept = default;

void ForStmt::on_verify(SemaContext& context, Scope& scope)
{
    const std::string_view loop_variable_name = m_loop_variable->name();

    if (scope.contains_symbol_here_or_up(loop_variable_name))
    {
        throw Error{location(), "symbol named '{}' already exists", loop_variable_name};
    }

    m_range->verify(context, scope);

    m_loop_variable->set_type(m_range->type());
    m_loop_variable->verify(context, scope);

    m_body->verify(context, scope, {});

    scope.remove_symbol(*m_loop_variable);
}

const ForLoopVariableDecl& ForStmt::loop_variable() const
{
    return *m_loop_variable;
}

const RangeExpr& ForStmt::range() const
{
    return *m_range;
}

const CodeBlock& ForStmt::body() const
{
    return *m_body;
}

bool ForStmt::accesses_symbol(const Decl& symbol, bool transitive) const
{
    if (m_range->accesses_symbol(symbol, transitive))
    {
        return true;
    }

    return m_body->accesses_symbol(symbol, transitive);
}

IfStmt::IfStmt(const SourceLocation&      location,
               std::unique_ptr<Expr>      condition_expr,
               std::unique_ptr<CodeBlock> body,
               std::unique_ptr<IfStmt>    next)
    : Stmt(location)
    , m_condition_expr(std::move(condition_expr))
    , m_body(std::move(body))
    , m_next(std::move(next))
{
    assert(m_body);
}

IfStmt::~IfStmt() noexcept = default;

void IfStmt::on_verify(SemaContext& context, Scope& scope)
{
    if (m_condition_expr != nullptr)
    {
        m_condition_expr->verify(context, scope);

        if (&m_condition_expr->type() != &BoolType::instance())
        {
            throw Error{m_condition_expr->location(),
                        "condition must evaluate to type {}",
                        BoolType::instance().type_name()};
        }
    }

    m_body->verify(context, scope, {});

    if (m_next)
    {
        m_next->verify(context, scope);
    }
}

const Expr* IfStmt::condition_expr() const
{
    return m_condition_expr.get();
}

const CodeBlock& IfStmt::body() const
{
    return *m_body;
}

const IfStmt* IfStmt::next() const
{
    return m_next.get();
}

bool IfStmt::accesses_symbol(const Decl& symbol, bool transitive) const
{
    if (m_condition_expr != nullptr && m_condition_expr->accesses_symbol(symbol, transitive))
    {
        return true;
    }

    if (m_body->accesses_symbol(symbol, transitive))
    {
        return true;
    }

    if (m_next != nullptr && m_next->accesses_symbol(symbol, transitive))
    {
        return true;
    }

    return false;
}

AssignmentStmt::AssignmentStmt(const SourceLocation& location,
                               std::unique_ptr<Expr> lhs,
                               std::unique_ptr<Expr> rhs)
    : Stmt(location)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{
    assert(m_lhs);
    assert(m_rhs);
}

AssignmentStmt::~AssignmentStmt() noexcept = default;

void AssignmentStmt::on_verify(SemaContext& context, Scope& scope)
{
    m_lhs->verify(context, scope);
    m_rhs->verify(context, scope);

    SemaContext::verify_type_assignment(m_lhs->type(), *m_rhs, false);
    SemaContext::verify_symbol_assignment(*m_lhs);
}

const Expr& AssignmentStmt::lhs() const
{
    return *m_lhs;
}

const Expr& AssignmentStmt::rhs() const
{
    return *m_rhs;
}

bool AssignmentStmt::accesses_symbol(const Decl& symbol, bool transitive) const
{
    return m_lhs->accesses_symbol(symbol, transitive) || m_rhs->accesses_symbol(symbol, transitive);
}
} // namespace cer::shadercompiler
