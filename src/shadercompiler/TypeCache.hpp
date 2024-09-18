// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <gsl/pointers>
#include <memory>

namespace cer::shadercompiler
{
class Expr;
class SourceLocation;
class ArrayType;
class UnresolvedType;

class CERLIB_API_INTERNAL TypeCache final
{
  public:
    TypeCache();

    NON_COPYABLE(TypeCache);

    TypeCache(TypeCache&&) noexcept;

    auto operator=(TypeCache&&) noexcept -> TypeCache&;

    ~TypeCache() noexcept;

    gsl::not_null<ArrayType*> create_array_type(const SourceLocation& location,
                                                std::string_view      element_type_name,
                                                std::unique_ptr<Expr> size_expr);

    gsl::not_null<UnresolvedType*> create_unresolved_type(const SourceLocation& location,
                                                          std::string_view      name);

    void clear();

  private:
    SmallVector<std::unique_ptr<ArrayType>, 32>      m_array_types;
    SmallVector<std::unique_ptr<UnresolvedType>, 32> m_unresolved_types;
};
} // namespace cer::shadercompiler
