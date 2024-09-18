// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include "util/StringViewUnorderedSet.hpp"
#include <gsl/pointers>
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
    SmallVector<gsl::not_null<const ShaderParamDecl*>, 8> scalars{};
    SmallVector<gsl::not_null<const ShaderParamDecl*>, 8> resources{};

    explicit operator bool() const
    {
        return !scalars.empty() || !resources.empty();
    }
};

class CERLIB_API_INTERNAL AST final
{
  public:
    using DeclsType = SmallVector<std::unique_ptr<Decl>, 8>;

    explicit AST(std::string_view              filename,
                 DeclsType                     decls,
                 const StringViewUnorderedSet* user_specified_defines);

    NON_COPYABLE(AST);

    AST(AST&&) noexcept = default;

    auto operator=(AST&&) noexcept -> AST& = default;

    ~AST() noexcept;

    void verify(SemaContext& context, Scope& global_scope);

    std::string_view filename() const;

    DeclsType& decls();

    const DeclsType& decls() const;

    bool is_top_level_symbol(const SemaContext& context, const Decl& symbol) const;

    bool has_parameters() const;

    bool is_symbol_accessed_anywhere(const Decl& symbol) const;

    const StringViewUnorderedSet* user_specified_defines() const;

    bool is_verified() const;

  private:
    std::string                           m_filename;
    SmallVector<std::unique_ptr<Decl>, 8> m_decls;
    const StringViewUnorderedSet*         m_user_specified_defines;
    bool                                  m_is_verified{};
};
} // namespace cer::shadercompiler
