// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/TypeCache.hpp"

#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Type.hpp"

namespace cer::shadercompiler
{
TypeCache::TypeCache() = default;

TypeCache::TypeCache(TypeCache&&) noexcept = default;

auto TypeCache::operator=(TypeCache&&) noexcept -> TypeCache& = default;

TypeCache::~TypeCache() noexcept = default;

auto TypeCache::create_array_type(const SourceLocation& location,
                                  std::string_view      element_type_name,
                                  std::unique_ptr<Expr> size_expr) -> gsl::not_null<ArrayType*>
{
    m_array_types.push_back(
        std::make_unique<ArrayType>(location,
                                    create_unresolved_type(location, element_type_name),
                                    std::move(size_expr)));

    return m_array_types.back().get();
}

auto TypeCache::create_unresolved_type(const SourceLocation& location, std::string_view name)
    -> gsl::not_null<UnresolvedType*>
{
    m_unresolved_types.push_back(std::make_unique<UnresolvedType>(location, name));
    return m_unresolved_types.back().get();
}

void TypeCache::clear()
{
    m_array_types.clear();
    m_unresolved_types.clear();
}
} // namespace cer::shadercompiler
