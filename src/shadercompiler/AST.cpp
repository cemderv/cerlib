// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/AST.hpp"

#include "shadercompiler/Casting.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/SemaContext.hpp"

#include <algorithm>

namespace cer::shadercompiler
{
AST::AST(std::string_view              filename,
         DeclsType                     decls,
         const StringViewUnorderedSet* user_specified_defines)
    : m_filename(filename)
    , m_decls(std::move(decls))
    , m_user_specified_defines(user_specified_defines)
{
}

AST::~AST() noexcept = default;

void AST::verify(SemaContext& context, Scope& global_scope)
{
    if (m_is_verified)
    {
        return;
    }

    for (const auto& decl : m_decls)
    {
        decl->verify(context, global_scope);
    }

    m_is_verified = true;
}

bool AST::is_top_level_symbol(const SemaContext& context, const Decl& symbol) const
{
    if (isa<StructFieldDecl>(&symbol))
    {
        return false;
    }

    if (std::ranges::any_of(m_decls, [&symbol](const auto& e) { return e.get() == &symbol; }))
    {
        return true;
    }

    return context.built_in_symbols().contains(symbol);
}

std::string_view AST::filename() const
{
    return m_filename;
}

AST::DeclsType& AST::decls()
{
    return m_decls;
}

const AST::DeclsType& AST::decls() const
{
    return m_decls;
}

bool AST::has_parameters() const
{
    return std::ranges::any_of(m_decls,
                               [](const auto& decl) { return isa<ShaderParamDecl>(decl.get()); });
}

bool AST::is_symbol_accessed_anywhere(const Decl& symbol) const
{
    return std::ranges::any_of(m_decls, [&symbol](const std::unique_ptr<Decl>& decl) {
        auto* function = asa<FunctionDecl>(decl.get());
        return function != nullptr ? function->accesses_symbol(symbol, true) : false;
    });
}

const StringViewUnorderedSet* AST::user_specified_defines() const
{
    return m_user_specified_defines;
}

bool AST::is_verified() const
{
    return m_is_verified;
}
} // namespace cer::shadercompiler
