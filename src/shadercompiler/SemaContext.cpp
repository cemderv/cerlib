// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/SemaContext.hpp"

#include "shadercompiler/Casting.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Lexer.hpp"
#include "shadercompiler/Naming.hpp"
#include "shadercompiler/SourceLocation.hpp"
#include "shadercompiler/Type.hpp"

namespace cer::shadercompiler
{
SemaContext::SemaContext(const AST&            ast,
                         const BuiltInSymbols& built_in_symbols,
                         const BinOpTable&     bin_op_table)
    : m_ast(ast)
    , m_built_in_symbols(built_in_symbols)
    , m_bin_op_table(bin_op_table)
    , m_allow_forbidden_identifier_prefix(false)
{
}

const AST& SemaContext::ast() const
{
    return m_ast;
}

const BuiltInSymbols& SemaContext::built_in_symbols() const
{
    return m_built_in_symbols;
}

const BinOpTable& SemaContext::bin_op_table() const
{
    return m_bin_op_table;
}

bool SemaContext::can_assign(const Type& target_type,
                             const Expr& rhs,
                             bool        is_implicit_cast_allowed)
{
    const Type& rhs_type = rhs.type();

    if (is_implicit_cast_allowed)
    {
        // Can assign int literal to float?
        if (&target_type == &FloatType::instance() && &rhs_type == &IntType::instance())
        {
            if (isa<IntLiteralExpr>(&rhs))
            {
                return true;
            }

            if (const UnaryOpExpr* const unary_op = asa<UnaryOpExpr>(&rhs))
            {
                if (isa<IntLiteralExpr>(&unary_op->expr()))
                {
                    return true;
                }
            }

            if (const BinOpExpr* const bin_op = asa<BinOpExpr>(&rhs))
            {
                if (isa<IntLiteralExpr>(bin_op->lhs()) && isa<IntLiteralExpr>(bin_op->rhs()))
                {
                    return true;
                }
            }
        }
    }

    if (&target_type != &rhs_type)
    {
        return false;
    }

    return true;
}

void SemaContext::verify_type_assignment(const Type& target_type,
                                         const Expr& rhs,
                                         bool        is_implicit_cast_allowed)
{
    if (!can_assign(target_type, rhs, is_implicit_cast_allowed))
    {
        throw Error{rhs.location(),
                    "cannot assign type '{}' to '{}' and no implicit conversion exists",
                    rhs.type().type_name(),
                    target_type.type_name()};
    }
}

void SemaContext::verify_symbol_assignment(const Expr& lhs)
{
    const Decl* symbol = lhs.symbol();

    if (symbol == nullptr)
    {
        throw Error{lhs.location(), "cannot assign a value to an unnamed value"};
    }

    if (const BinOpExpr* const bin_op = asa<BinOpExpr>(&lhs))
    {
        symbol = bin_op->lhs().symbol();
    }
    else if (const SubscriptExpr* const subscript = asa<SubscriptExpr>(&lhs))
    {
        throw Error{subscript->location(),
                    "assignment to subscript expressions is not supported yet"};
    }

    if (const VarDecl* const var = asa<VarDecl>(symbol))
    {
        if (var->is_const())
        {
            throw Error{lhs.location(),
                        "cannot assign to immutable variable '{}'; consider marking it as '{}' "
                        "instead of '{}'",
                        var->name(),
                        keyword::var,
                        keyword::const_};
        }
    }
}

void SemaContext::verify_symbol_name(const SourceLocation& location, std::string_view name) const
{
    if (!m_allow_forbidden_identifier_prefix)
    {
        if (naming::is_identifier_forbidden(name))
        {
            throw Error{location,
                        "prefix '{}' is reserved and cannot be used for identifiers",
                        naming::forbidden_identifier_prefix};
        }
    }
}

void SemaContext::set_allow_forbidden_identifier_prefix(bool value)
{
    m_allow_forbidden_identifier_prefix = value;
}
} // namespace cer::shadercompiler
