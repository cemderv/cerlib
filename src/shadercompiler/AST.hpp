// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/StringViewUnorderedSet.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
#include <string>

namespace cer::shadercompiler
{
class SemaContext;
class Decl;
class Scope;
class SourceLocation;
class FunctionDecl;
class ShaderParamDecl;

class AccessedParams
{
  public:
    small_vector_of_refs<const ShaderParamDecl, 8> scalars{};
    small_vector_of_refs<const ShaderParamDecl, 8> resources{};

    explicit operator bool() const
    {
        return !scalars.empty() || !resources.empty();
    }
};

class AST final
{
  public:
    using DeclsType = small_vector_of_uniques<Decl, 8>;

    explicit AST(std::string_view              filename,
                 DeclsType                     decls,
                 const StringViewUnorderedSet* user_specified_defines);

    forbid_copy(AST);

    AST(AST&&) noexcept = default;

    auto operator=(AST&&) noexcept -> AST& = default;

    ~AST() noexcept;

    void verify(SemaContext& context, Scope& global_scope);

    auto filename() const -> std::string_view;

    auto decls() -> DeclsType&;

    auto decls() const -> const DeclsType&;

    auto is_top_level_symbol(const SemaContext& context, const Decl& symbol) const -> bool;

    auto has_parameters() const -> bool;

    auto is_symbol_accessed_anywhere(const Decl& symbol) const -> bool;

    auto user_specified_defines() const -> const StringViewUnorderedSet*;

    auto is_verified() const -> bool;

  private:
    std::string                      m_filename;
    small_vector_of_uniques<Decl, 8> m_decls;
    const StringViewUnorderedSet*    m_user_specified_defines;
    bool                             m_is_verified{};
};
} // namespace cer::shadercompiler
