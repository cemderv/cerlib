// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/NonCopyable.hpp"
#include "util/inplace_vector.hpp"
#include <gsl/pointers>
#include <memory>

namespace cer::shadercompiler
{
class Expr;
class SourceLocation;
class ArrayType;
class UnresolvedType;

class TypeCache final
{
  public:
    TypeCache();

    NON_COPYABLE(TypeCache);

    TypeCache(TypeCache&&) noexcept;

    auto operator=(TypeCache&&) noexcept -> TypeCache&;

    ~TypeCache() noexcept;

    auto create_array_type(const SourceLocation& location,
                           std::string_view      element_type_name,
                           std::unique_ptr<Expr> size_expr) -> gsl::not_null<ArrayType*>;

    auto create_unresolved_type(const SourceLocation& location, std::string_view name)
        -> gsl::not_null<UnresolvedType*>;

    void clear();

  private:
    inplace_vector<std::unique_ptr<ArrayType>, 32>      m_array_types;
    inplace_vector<std::unique_ptr<UnresolvedType>, 32> m_unresolved_types;
};
} // namespace cer::shadercompiler
