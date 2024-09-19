// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/CodeBlock.hpp"

#include "shadercompiler/Casting.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/Stmt.hpp"

namespace cer::shadercompiler
{
CodeBlock::CodeBlock(const SourceLocation& location, StmtsType stmts)
    : m_location(location)
    , m_stmts(std::move(stmts))
{
}

CodeBlock::~CodeBlock() noexcept = default;

void CodeBlock::verify(SemaContext&                                context,
                       Scope&                                      scope,
                       std::span<const gsl::not_null<const Decl*>> extra_symbols) const
{
    Scope& child_scope = scope.push_child();

    for (const gsl::not_null<const Decl*>& symbol : extra_symbols)
    {
        child_scope.add_symbol(*symbol);
    }

    for (const std::unique_ptr<Stmt>& stmt : m_stmts)
    {
        stmt->verify(context, child_scope);
    }

    scope.pop_child();
}

SmallVector<gsl::not_null<VarStmt*>, 8> CodeBlock::variables() const
{
    SmallVector<gsl::not_null<VarStmt*>, 8> vars;
    vars.reserve(m_stmts.size());

    for (const std::unique_ptr<Stmt>& stmt : m_stmts)
    {
        if (VarStmt* var = asa<VarStmt>(stmt.get()))
        {
            vars.emplace_back(var);
        }
    }

    return vars;
}

void CodeBlock::remove_stmt(const Stmt& stmt)
{
    const auto it = std::ranges::find_if(m_stmts, [&stmt](const std::unique_ptr<Stmt>& e) {
        return e.get() == &stmt;
    });

    if (it != m_stmts.end())
    {
        m_stmts.erase(it);
    }
}

bool CodeBlock::accesses_symbol(const Decl& symbol, bool transitive) const
{
    return std::ranges::any_of(m_stmts, [&symbol, transitive](const std::unique_ptr<Stmt>& e) {
        return e->accesses_symbol(symbol, transitive);
    });
}

const SourceLocation& CodeBlock::location() const
{
    return m_location;
}

const CodeBlock::StmtsType& CodeBlock::stmts() const
{
    return m_stmts;
}
} // namespace cer::shadercompiler
