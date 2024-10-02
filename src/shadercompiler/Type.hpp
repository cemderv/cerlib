// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include <gsl/pointers>
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
    NON_COPYABLE_NON_MOVABLE(Type);

    virtual ~Type() noexcept = default;

    [[nodiscard]] virtual const Type& resolve(SemaContext& context, Scope& scope) const = 0;

    virtual std::string_view type_name() const = 0;

    virtual const Type* member_type(std::string_view name) const;

    virtual Decl* find_member_symbol(const SemaContext& context, std::string_view name) const;

    bool can_be_in_constant_buffer() const;

    virtual bool can_be_shader_parameter() const;

    bool is_unresolved() const;

    bool is_array() const;

    virtual bool is_scalar_type() const;

    virtual bool is_vector_type() const;

    virtual bool is_matrix_type() const;

    virtual bool is_image_type() const;

    bool is_user_defined_struct() const;

    const SourceLocation& location() const;

  private:
    SourceLocation m_Location;
};

class IntType final : public Type
{
  public:
    IntType();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    bool is_scalar_type() const override;
};

class BoolType final : public Type
{
  public:
    BoolType();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;
};

class FloatType final : public Type
{
  public:
    FloatType();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    bool is_scalar_type() const override;
};

class Vector2Type final : public Type
{
  public:
    Vector2Type();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    Decl* find_member_symbol(const SemaContext& context, std::string_view name) const override;

    bool is_vector_type() const override;
};

class Vector3Type final : public Type
{
  public:
    Vector3Type();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    Decl* find_member_symbol(const SemaContext& context, std::string_view name) const override;

    bool is_vector_type() const override;
};

class Vector4Type final : public Type
{
  public:
    Vector4Type();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    Decl* find_member_symbol(const SemaContext& context, std::string_view name) const override;
};

class MatrixType final : public Type
{
  public:
    MatrixType();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    bool is_matrix_type() const override;
};

class ImageType final : public Type
{
  public:
    ImageType();

    static const Type& instance();

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

    bool is_image_type() const override;
};

class ArrayType final : public Type
{
  public:
    static constexpr uint32_t max_size = 255;

    explicit ArrayType(const SourceLocation& location,
                       gsl::not_null<Type*>  element_type,
                       std::unique_ptr<Expr> size_expr);

    ~ArrayType() noexcept override;

    const Type& element_type() const;

    const Expr& size_expr() const;

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    uint32_t size() const;

    std::string_view type_name() const override;

    bool can_be_shader_parameter() const override;

  private:
    mutable gsl::not_null<const Type*> m_element_type;
    std::unique_ptr<Expr>              m_size_expr;
    mutable uint32_t                   m_size{};
    mutable std::string                m_name;
};

class UnresolvedType final : public Type
{
  public:
    explicit UnresolvedType(const SourceLocation& location, std::string_view name);

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::string_view type_name() const override;

  private:
    std::string_view m_name;
};
} // namespace cer::shadercompiler
