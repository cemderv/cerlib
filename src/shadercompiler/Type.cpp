// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Type.hpp"

#include "shadercompiler/BuiltInSymbols.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/SemaContext.hpp"
#include "util/Util.hpp"

#include <cassert>
#include <gsl/util>
#include <optional>
#include <utility>

namespace cer::shadercompiler
{
const Type* Type::member_type(std::string_view name) const
{
    CERLIB_UNUSED(name);
    return nullptr;
}

Decl* Type::find_member_symbol(const SemaContext& context, std::string_view name) const
{
    CERLIB_UNUSED(name);
    return nullptr;
}

bool Type::can_be_in_constant_buffer() const
{
    return !is_image_type() && !is_user_defined_struct();
}

bool Type::can_be_shader_parameter() const
{
    return true;
}

bool Type::is_unresolved() const
{
    return isa<UnresolvedType>(this);
}

bool Type::is_array() const
{
    return isa<ArrayType>(this);
}

bool Type::is_scalar_type() const
{
    return false;
}

bool Type::is_vector_type() const
{
    return false;
}

bool Type::is_matrix_type() const
{
    return false;
}

bool Type::is_image_type() const
{
    return false;
}

bool Type::is_user_defined_struct() const
{
    return isa<StructDecl>(this);
}

const SourceLocation& Type::location() const
{
    return m_Location;
}

Type::Type(const SourceLocation& location)
    : m_Location(location)
{
}

IntType::IntType()
    : Type(SourceLocation::std)
{
}

const Type& IntType::instance()
{
    static IntType s_instance;
    return s_instance;
}

const Type& IntType::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view IntType::type_name() const
{
    return "int";
}

bool IntType::is_scalar_type() const
{
    return true;
}

BoolType::BoolType()
    : Type(SourceLocation::std)
{
}

const Type& BoolType::instance()
{
    static BoolType s_instance;
    return s_instance;
}

const Type& BoolType::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view BoolType::type_name() const
{
    return "bool";
}

FloatType::FloatType()
    : Type(SourceLocation::std)
{
}

const Type& FloatType::instance()
{
    static FloatType s_instance;
    return s_instance;
}

const Type& FloatType::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view FloatType::type_name() const
{
    return "float";
}

bool FloatType::is_scalar_type() const
{
    return true;
}

Vector2Type::Vector2Type()
    : Type(SourceLocation::std)
{
}

const Type& Vector2Type::instance()
{
    static Vector2Type s_instance;
    return s_instance;
}

const Type& Vector2Type::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view Vector2Type::type_name() const
{
    return "Vector2";
}

Decl* Vector2Type::find_member_symbol(const SemaContext& context, std::string_view name) const
{
    const std::vector<std::unique_ptr<Decl>>& fields = context.built_in_symbols().vector2_fields;

    const auto it =
        std::ranges::find_if(fields, [name](const auto& e) { return e->name() == name; });

    return it != fields.cend() ? it->get() : nullptr;
}

bool Vector2Type::is_vector_type() const
{
    return true;
}

Vector3Type::Vector3Type()
    : Type(SourceLocation::std)
{
}

const Type& Vector3Type::instance()
{
    static Vector3Type s_instance;
    return s_instance;
}

const Type& Vector3Type::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view Vector3Type::type_name() const
{
    return "Vector3";
}

Decl* Vector3Type::find_member_symbol(const SemaContext& context, std::string_view name) const
{
    const std::vector<std::unique_ptr<Decl>>& fields = context.built_in_symbols().vector3_fields;

    const auto it = std::ranges::find_if(fields, [name](const std::unique_ptr<Decl>& e) {
        return e->name() == name;
    });

    return it != fields.cend() ? it->get() : nullptr;
}

bool Vector3Type::is_vector_type() const
{
    return true;
}

Vector4Type::Vector4Type()
    : Type(SourceLocation::std)
{
}

const Type& Vector4Type::instance()
{
    static Vector4Type s_instance;
    return s_instance;
}

const Type& Vector4Type::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view Vector4Type::type_name() const
{
    return "Vector4";
}

Decl* Vector4Type::find_member_symbol(const SemaContext& context, std::string_view name) const
{
    const std::vector<std::unique_ptr<Decl>>& fields = context.built_in_symbols().vector4_fields;

    const auto it = std::ranges::find_if(fields, [name](const std::unique_ptr<Decl>& e) {
        return e->name() == name;
    });

    return it != fields.cend() ? it->get() : nullptr;
}

MatrixType::MatrixType()
    : Type(SourceLocation::std)
{
}

const Type& MatrixType::instance()
{
    static MatrixType s_instance;
    return s_instance;
}

const Type& MatrixType::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(context);
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view MatrixType::type_name() const
{
    return "Matrix";
}

bool MatrixType::is_matrix_type() const
{
    return true;
}

ArrayType::ArrayType(const SourceLocation& location,
                     gsl::not_null<Type*>  element_type,
                     std::unique_ptr<Expr> size_expr)
    : Type(location)
    , m_element_type(element_type)
    , m_size_expr(std::move(size_expr))
{
}

ArrayType::~ArrayType() noexcept = default;

const Type& ArrayType::element_type() const
{
    return *m_element_type;
}

const Expr& ArrayType::size_expr() const
{
    return *m_size_expr;
}

const Type& ArrayType::resolve(SemaContext& context, Scope& scope) const
{
    if (!m_element_type->is_unresolved())
    {
        // We are already resolved
        return *this;
    }

    m_element_type = &m_element_type->resolve(context, scope);

    m_name = m_element_type->type_name();
    m_name += "[]";

    m_size_expr->verify(context, scope);

    const Type& size_type = m_size_expr->type();

    if (&size_type != &IntType::instance())
    {
        throw Error{m_size_expr->location(),
                    "values of type '{}' cannot be used as an array size; expected '{}'",
                    size_type.type_name(),
                    IntType::instance().type_name()};
    }

    const std::any constant_value = m_size_expr->evaluate_constant_value(context, scope);

    if (!constant_value.has_value())
    {
        throw Error{location(), "expression does not evaluate to a constant integer value"};
    }

    std::optional<uint32_t> size;

    if (const int32_t* const int_size = std::any_cast<int32_t>(&constant_value))
    {
        if (*int_size < 0)
        {
            throw Error{location(),
                        "negative array sizes are not allowed (specified size = {})",
                        *int_size};
        }

        size = gsl::narrow_cast<uint32_t>(*int_size);
    }
    else if (const uint32_t* uint_size = std::any_cast<uint32_t>(&constant_value))
    {
        size = *uint_size;
    }
    else
    {
        throw Error{location(), "invalid size expression"};
    }

    if (*size == 0)
    {
        throw Error{location(), "zero array sizes are not allowed (specified size = {})", *size};
    }

    if (*size > max_size)
    {
        throw Error{location(),
                    "array size (= {}) exceeds the maximum allowed array size (= {})",
                    *size,
                    max_size};
    }

    m_size = *size;

    return *this;
}

uint32_t ArrayType::size() const
{
    assert(!m_element_type->is_unresolved());
    return m_size;
}

std::string_view ArrayType::type_name() const
{
    return m_name;
}

bool ArrayType::can_be_shader_parameter() const
{
    assert(!m_element_type->is_unresolved());

    // image arrays are not supported yet
    if (m_element_type->is_image_type())
    {
        return false;
    }

    // Probably never support user-defined structs in array parameters.
    if (m_element_type->is_user_defined_struct())
    {
        return false;
    }

    return true;
}

UnresolvedType::UnresolvedType(const SourceLocation& location, std::string_view name)
    : Type(location)
    , m_name(name)
{
}

const Type& UnresolvedType::resolve(SemaContext& context, Scope& scope) const
{
    const Type* resolved_type = scope.find_type(m_name);

    if (resolved_type == nullptr)
    {
        throw Error{location(), "undefined type '{}'", m_name};
    }

    return *resolved_type;
}

std::string_view UnresolvedType::type_name() const
{
    return m_name;
}

ImageType::ImageType()
    : Type(SourceLocation::std)
{
}

const Type& ImageType::instance()
{
    static ImageType s_instance;
    return s_instance;
}

const Type& ImageType::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(scope);
    return instance();
}

std::string_view ImageType::type_name() const
{
    return "Image";
}

bool ImageType::is_image_type() const
{
    return true;
}
} // namespace cer::shadercompiler
