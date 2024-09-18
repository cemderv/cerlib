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

auto AST::verify(SemaContext& context, Scope& global_scope) -> void
{
    if (m_is_verified)
        return;

    for (const auto& decl : m_decls)
        decl->verify(context, global_scope);

    m_is_verified = true;
}

auto AST::is_top_level_symbol(const SemaContext& context, const Decl& symbol) const -> bool
{
    if (isa<StructFieldDecl>(&symbol))
        return false;

    if (std::ranges::any_of(m_decls, [&symbol](const auto& e) { return e.get() == &symbol; }))
        return true;

    return context.built_in_symbols().contains(symbol);
}

auto AST::filename() const -> std::string_view
{
    return m_filename;
}

auto AST::decls() -> DeclsType&
{
    return m_decls;
}

auto AST::decls() const -> const DeclsType&
{
    return m_decls;
}

auto AST::has_parameters() const -> bool
{
    return std::ranges::any_of(m_decls,
                               [](const auto& decl) { return isa<ShaderParamDecl>(decl.get()); });
}

auto AST::is_symbol_accessed_anywhere(const Decl& symbol) const -> bool
{
    return std::ranges::any_of(m_decls, [&symbol](const std::unique_ptr<Decl>& decl) {
        FunctionDecl* function = asa<FunctionDecl>(decl.get());
        return function != nullptr ? function->accesses_symbol(symbol, true) : false;
    });
}

auto AST::user_specified_defines() const -> const StringViewUnorderedSet*
{
    return m_user_specified_defines;
}

auto AST::is_verified() const -> bool
{
    return m_is_verified;
}
} // namespace cer::shadercompiler
