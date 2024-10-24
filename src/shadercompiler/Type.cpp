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
#include <cassert>
#include <cerlib/Option.hpp>

namespace cer::shadercompiler
{
auto Type::member_type([[maybe_unused]] std::string_view name) const -> const Type*
{
    return nullptr;
}

auto Type::find_member_symbol([[maybe_unused]] const SemaContext& context,
                              [[maybe_unused]] std::string_view   name) const -> Decl*
{
    return nullptr;
}

auto Type::can_be_in_constant_buffer() const -> bool
{
    return !is_image_type() && !is_user_defined_struct();
}

auto Type::can_be_shader_parameter() const -> bool
{
    return true;
}

auto Type::is_unresolved() const -> bool
{
    return isa<UnresolvedType>(this);
}

auto Type::is_array() const -> bool
{
    return isa<ArrayType>(this);
}

auto Type::is_scalar_type() const -> bool
{
    return false;
}

auto Type::is_vector_type() const -> bool
{
    return false;
}

auto Type::is_matrix_type() const -> bool
{
    return false;
}

auto Type::is_image_type() const -> bool
{
    return false;
}

auto Type::is_user_defined_struct() const -> bool
{
    return isa<StructDecl>(this);
}

auto Type::location() const -> const SourceLocation&
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

auto IntType::instance() -> const Type&
{
    static auto s_instance = IntType{};
    return s_instance;
}

auto IntType::resolve([[maybe_unused]] SemaContext& context, [[maybe_unused]] Scope& scope) const
    -> const Type&
{
    return instance();
}

auto IntType::type_name() const -> std::string_view
{
    return "int";
}

auto IntType::is_scalar_type() const -> bool
{
    return true;
}

BoolType::BoolType()
    : Type(SourceLocation::std)
{
}

auto BoolType::instance() -> const Type&
{
    static auto s_instance = BoolType{};
    return s_instance;
}

auto BoolType::resolve([[maybe_unused]] SemaContext& context, [[maybe_unused]] Scope& scope) const
    -> const Type&
{
    return instance();
}

auto BoolType::type_name() const -> std::string_view
{
    return "bool";
}

FloatType::FloatType()
    : Type(SourceLocation::std)
{
}

auto FloatType::instance() -> const Type&
{
    static auto s_instance = FloatType{};
    return s_instance;
}

auto FloatType::resolve([[maybe_unused]] SemaContext& context, [[maybe_unused]] Scope& scope) const
    -> const Type&
{
    return instance();
}

auto FloatType::type_name() const -> std::string_view
{
    return "float";
}

auto FloatType::is_scalar_type() const -> bool
{
    return true;
}

Vector2Type::Vector2Type()
    : Type(SourceLocation::std)
{
}

auto Vector2Type::instance() -> const Type&
{
    static auto s_instance = Vector2Type{};
    return s_instance;
}

auto Vector2Type::resolve([[maybe_unused]] SemaContext& context,
                          [[maybe_unused]] Scope&       scope) const -> const Type&
{
    return instance();
}

auto Vector2Type::type_name() const -> std::string_view
{
    return "Vector2";
}

auto Vector2Type::find_member_symbol(const SemaContext& context, std::string_view name) const
    -> Decl*
{
    const auto& fields = context.built_in_symbols().vector2_fields;

    const auto it = std::ranges::find_if(fields, [name](const auto& e) {
        return e->name() == name;
    });

    return it != fields.cend() ? it->get() : nullptr;
}

auto Vector2Type::is_vector_type() const -> bool
{
    return true;
}

Vector3Type::Vector3Type()
    : Type(SourceLocation::std)
{
}

auto Vector3Type::instance() -> const Type&
{
    static auto s_instance = Vector3Type{};
    return s_instance;
}

auto Vector3Type::resolve([[maybe_unused]] SemaContext& context,
                          [[maybe_unused]] Scope&       scope) const -> const Type&
{
    return instance();
}

auto Vector3Type::type_name() const -> std::string_view
{
    return "Vector3";
}

auto Vector3Type::find_member_symbol(const SemaContext& context, std::string_view name) const
    -> Decl*
{
    const auto& fields = context.built_in_symbols().vector3_fields;

    const auto it = std::ranges::find_if(fields, [name](const auto& e) {
        return e->name() == name;
    });

    return it != fields.cend() ? it->get() : nullptr;
}

auto Vector3Type::is_vector_type() const -> bool
{
    return true;
}

Vector4Type::Vector4Type()
    : Type(SourceLocation::std)
{
}

auto Vector4Type::instance() -> const Type&
{
    static auto s_instance = Vector4Type{};
    return s_instance;
}

auto Vector4Type::resolve([[maybe_unused]] SemaContext& context,
                          [[maybe_unused]] Scope&       scope) const -> const Type&
{
    return instance();
}

auto Vector4Type::type_name() const -> std::string_view
{
    return "Vector4";
}

auto Vector4Type::find_member_symbol(const SemaContext& context, std::string_view name) const
    -> Decl*
{
    const auto& fields = context.built_in_symbols().vector4_fields;

    const auto it = std::ranges::find_if(fields, [name](const auto& e) {
        return e->name() == name;
    });

    return it != fields.cend() ? it->get() : nullptr;
}

MatrixType::MatrixType()
    : Type(SourceLocation::std)
{
}

auto MatrixType::instance() -> const Type&
{
    static MatrixType s_instance;
    return s_instance;
}

auto MatrixType::resolve([[maybe_unused]] SemaContext& context, [[maybe_unused]] Scope& scope) const
    -> const Type&
{
    return instance();
}

auto MatrixType::type_name() const -> std::string_view
{
    return "Matrix";
}

auto MatrixType::is_matrix_type() const -> bool
{
    return true;
}

ArrayType::ArrayType(const SourceLocation& location, Type& element_type, UniquePtr<Expr> size_expr)
    : Type(location)
    , m_element_type_ref(element_type)
    , m_size_expr(std::move(size_expr))
{
}

ArrayType::~ArrayType() noexcept = default;

auto ArrayType::element_type() const -> const Type&
{
    return m_element_type_ref.get();
}

auto ArrayType::size_expr() const -> const Expr&
{
    return *m_size_expr;
}

auto ArrayType::resolve(SemaContext& context, Scope& scope) const -> const Type&
{
    if (!m_element_type_ref.get().is_unresolved())
    {
        // We are already resolved
        return *this;
    }

    m_element_type_ref = m_element_type_ref.get().resolve(context, scope);

    const auto& element_type = m_element_type_ref.get();

    m_name = element_type.type_name();
    m_name += "[]";

    m_size_expr->verify(context, scope);

    const auto& size_type = m_size_expr->type();

    if (&size_type != &IntType::instance())
    {
        throw Error{m_size_expr->location(),
                    "values of type '{}' cannot be used as an array size; expected '{}'",
                    size_type.type_name(),
                    IntType::instance().type_name()};
    }

    const auto constant_value = m_size_expr->evaluate_constant_value(context, scope);

    if (!constant_value.has_value())
    {
        throw Error{location(), "expression does not evaluate to a constant integer value"};
    }

    auto size = Option<uint32_t>{};

    if (const auto* const int_size = std::any_cast<int32_t>(&constant_value))
    {
        if (*int_size < 0)
        {
            throw Error{location(),
                        "negative array sizes are not allowed (specified size = {})",
                        *int_size};
        }

        size = narrow_cast<uint32_t>(*int_size);
    }
    else if (const auto* uint_size = std::any_cast<uint32_t>(&constant_value))
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

auto ArrayType::size() const -> size_t
{
    assert(!m_element_type_ref.get().is_unresolved());
    return m_size;
}

auto ArrayType::type_name() const -> std::basic_string_view<char>
{
    return m_name;
}

auto ArrayType::can_be_shader_parameter() const -> bool
{
    const auto& element_type = m_element_type_ref.get();

    assert(!element_type.is_unresolved());

    // image arrays are not supported yet
    if (element_type.is_image_type())
    {
        return false;
    }

    // Probably never support user-defined structs in array parameters.
    if (element_type.is_user_defined_struct())
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

auto UnresolvedType::resolve([[maybe_unused]] SemaContext& context, Scope& scope) const
    -> const Type&
{
    const auto* resolved_type = scope.find_type(m_name);

    if (resolved_type == nullptr)
    {
        throw Error{location(), "undefined type '{}'", m_name};
    }

    return *resolved_type;
}

auto UnresolvedType::type_name() const -> std::string_view
{
    return m_name;
}

ImageType::ImageType()
    : Type(SourceLocation::std)
{
}

auto ImageType::instance() -> const Type&
{
    static ImageType s_instance;
    return s_instance;
}

auto ImageType::resolve([[maybe_unused]] SemaContext& context, [[maybe_unused]] Scope& scope) const
    -> const Type&
{
    return instance();
}

auto ImageType::type_name() const -> std::string_view
{
    return "Image";
}

auto ImageType::is_image_type() const -> bool
{
    return true;
}
} // namespace cer::shadercompiler
