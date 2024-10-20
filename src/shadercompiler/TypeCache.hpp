// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
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

    forbid_copy(TypeCache);

    TypeCache(TypeCache&&) noexcept;

    auto operator=(TypeCache&&) noexcept -> TypeCache&;

    ~TypeCache() noexcept;

    auto create_array_type(const SourceLocation& location,
                           std::string_view      element_type_name,
                           std::unique_ptr<Expr> size_expr) -> ArrayType&;

    auto create_unresolved_type(const SourceLocation& location, std::string_view name)
        -> UnresolvedType&;

    void clear();

  private:
    small_vector_of_uniques<ArrayType, 32>      m_array_types;
    small_vector_of_uniques<UnresolvedType, 32> m_unresolved_types;
};
} // namespace cer::shadercompiler
