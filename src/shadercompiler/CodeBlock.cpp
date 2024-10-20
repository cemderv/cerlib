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

void CodeBlock::verify(SemaContext&                                        context,
                       Scope&                                              scope,
                       std::span<const std::reference_wrapper<const Decl>> extra_symbols) const
{
    Scope& child_scope = scope.push_child();

    for (const auto& symbol : extra_symbols)
    {
        child_scope.add_symbol(symbol.get());
    }

    for (const auto& stmt : m_stmts)
    {
        stmt->verify(context, child_scope);
    }

    scope.pop_child();
}

auto CodeBlock::variables() const -> small_vector_of_refs<VarStmt, 8>
{
    auto vars = small_vector_of_refs<VarStmt, 8>{};
    vars.reserve(m_stmts.size());

    for (const std::unique_ptr<Stmt>& stmt : m_stmts)
    {
        if (auto* var = asa<VarStmt>(stmt.get()))
        {
            vars.emplace_back(*var);
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

auto CodeBlock::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return std::ranges::any_of(m_stmts, [&symbol, transitive](const std::unique_ptr<Stmt>& e) {
        return e->accesses_symbol(symbol, transitive);
    });
}

auto CodeBlock::location() const -> const SourceLocation&
{
    return m_location;
}

auto CodeBlock::stmts() const -> const CodeBlock::StmtsType&
{
    return m_stmts;
}
} // namespace cer::shadercompiler
