// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <string>

namespace cer::shadercompiler
{
class SemaContext;
class Expr;
class Scope;
class Decl;

class Type
{
  protected:
    explicit Type(const SourceLocation& location);

  public:
    using Ref      = std::reference_wrapper<Type>;
    using ConstRef = std::reference_wrapper<const Type>;

    forbid_copy_and_move(Type);

    virtual ~Type() noexcept = default;

    [[nodiscard]] virtual auto resolve(SemaContext& context, Scope& scope) const -> const Type& = 0;

    virtual auto type_name() const -> std::string_view = 0;

    virtual auto member_type(std::string_view name) const -> const Type*;

    virtual auto find_member_symbol(const SemaContext& context, std::string_view name) const
        -> Decl*;

    auto can_be_in_constant_buffer() const -> bool;

    virtual auto can_be_shader_parameter() const -> bool;

    auto is_unresolved() const -> bool;

    auto is_array() const -> bool;

    virtual auto is_scalar_type() const -> bool;

    virtual auto is_vector_type() const -> bool;

    virtual auto is_matrix_type() const -> bool;

    virtual auto is_image_type() const -> bool;

    auto is_user_defined_struct() const -> bool;

    auto location() const -> const SourceLocation&;

  private:
    SourceLocation m_Location;
};

class IntType final : public Type
{
  public:
    IntType();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::string_view override;

    auto is_scalar_type() const -> bool override;
};

class BoolType final : public Type
{
  public:
    BoolType();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::basic_string_view<char> override;
};

class FloatType final : public Type
{
  public:
    FloatType();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::basic_string_view<char> override;

    auto is_scalar_type() const -> bool override;
};

class Vector2Type final : public Type
{
  public:
    Vector2Type();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::string_view override;

    auto find_member_symbol(const SemaContext& context, std::string_view name) const
        -> Decl* override;

    auto is_vector_type() const -> bool override;
};

class Vector3Type final : public Type
{
  public:
    Vector3Type();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::basic_string_view<char> override;

    auto find_member_symbol(const SemaContext& context, std::string_view name) const
        -> Decl* override;

    auto is_vector_type() const -> bool override;
};

class Vector4Type final : public Type
{
  public:
    Vector4Type();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::string_view override;

    auto find_member_symbol(const SemaContext& context, std::string_view name) const
        -> Decl* override;
};

class MatrixType final : public Type
{
  public:
    MatrixType();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::string_view override;

    auto is_matrix_type() const -> bool override;
};

class ImageType final : public Type
{
  public:
    ImageType();

    static auto instance() -> const Type&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::string_view override;

    auto is_image_type() const -> bool override;
};

class ArrayType final : public Type
{
  public:
    static constexpr uint32_t max_size = 255;

    explicit ArrayType(const SourceLocation& location,
                       Type&                 element_type,
                       std::unique_ptr<Expr> size_expr);

    ~ArrayType() noexcept override;

    auto element_type() const -> const Type&;

    auto size_expr() const -> const Expr&;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto size() const -> size_t;

    auto type_name() const -> std::string_view override;

    auto can_be_shader_parameter() const -> bool override;

  private:
    mutable std::reference_wrapper<const Type> m_element_type_ref;
    std::unique_ptr<Expr>                      m_size_expr;
    mutable size_t                             m_size{};
    mutable std::string                        m_name;
};

class UnresolvedType final : public Type
{
  public:
    explicit UnresolvedType(const SourceLocation& location, std::string_view name);

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto type_name() const -> std::string_view override;

  private:
    std::string_view m_name;
};
} // namespace cer::shadercompiler
