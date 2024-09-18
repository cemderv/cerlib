// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Decl.hpp"

#include "shadercompiler/AST.hpp"
#include "shadercompiler/BuiltInSymbols.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Naming.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/SemaContext.hpp"
#include "shadercompiler/Stmt.hpp"
#include "shadercompiler/Type.hpp"
#include "util/Util.hpp"
#include <algorithm>
#include <cassert>
#include <gsl/util>
#include <utility>

namespace cer::shadercompiler
{
Decl::~Decl() noexcept = default;

void Decl::verify(SemaContext& context, Scope& scope)
{
    assert(!m_name.empty());

    if (!m_is_verified)
    {
        on_verify(context, scope);
        assert(m_type);
        m_is_verified = true;
    }
}

bool Decl::is_verified() const
{
    return m_is_verified;
}

Decl::Decl(const SourceLocation& location, std::string_view name)
    : m_location(location)
    , m_is_verified(false)
    , m_name(name)
    , m_type(nullptr)
{
    assert(!m_name.empty());
}

const SourceLocation& Decl::location() const
{
    return m_location;
}

std::string_view Decl::name() const
{
    return m_name;
}

const Type& Decl::type() const
{
    return *m_type;
}

StructFieldDecl::StructFieldDecl(const SourceLocation& location,
                                 std::string_view      name,
                                 const Type&           type)
    : Decl(location, name)
{
    set_type(type);
}

StructFieldDecl::~StructFieldDecl() noexcept = default;

void StructFieldDecl::on_verify(SemaContext& context, Scope& scope)
{
    if (!name().starts_with(naming::forbidden_identifier_prefix))
        context.verify_symbol_name(location(), name());

    const Type& type = this->type().resolve(context, scope);

    set_type(type);

    if (type.is_array() || type.is_image_type() || type.is_user_defined_struct())
    {
        throw Error{location(),
                    "invalid type for struct field; expected a scalar, "
                    "vector or matrix type"};
    }
}

StructDecl::StructDecl(const SourceLocation& location,
                       std::string_view      name,
                       FieldList             fields,
                       bool                  is_built_in)
    : Decl(location, name)
    , Type(location)
    , m_fields(std::move(fields))
    , m_ctor(nullptr)
    , m_is_built_in(is_built_in)
{
}

StructDecl::~StructDecl() noexcept = default;

void StructDecl::on_verify(SemaContext& context, Scope& scope)
{
    context.verify_symbol_name(Decl::location(), name());

    if (scope.contains_type_only_here(name()))
    {
        throw Error(Decl::location(), "type '{}' is already defined");
    }

    for (const std::unique_ptr<StructFieldDecl>& field : m_fields)
    {
        if (const StructFieldDecl* other_field = find_field(field->name());
            other_field != nullptr && other_field != field.get())
        {
            throw Error(field->location(), "duplicate struct field '{}'", field->name());
        }

        field->verify(context, scope);
    }

    scope.add_type(*this);

    m_ctor = std::make_unique<FunctionDecl>(Decl::location(),
                                            name(),
                                            SmallVector<std::unique_ptr<FunctionParamDecl>, 4>(),
                                            *this,
                                            nullptr,
                                            /*is_struct_ctor:*/ true);

    scope.add_symbol(*m_ctor);

    set_type(*this);
}

StructFieldDecl* StructDecl::find_field(std::string_view name) const
{
    const auto it =
        std::ranges::find_if(m_fields, [name](const auto& field) { return field->name() == name; });

    return it != m_fields.cend() ? it->get() : nullptr;
}

bool StructDecl::has_field(std::string_view name) const
{
    return std::ranges::find_if(m_fields, [name](const auto& e) { return e->name() == name; }) !=
           m_fields.cend();
}

std::string_view StructDecl::type_name() const
{
    return name();
}

const Type& StructDecl::resolve(SemaContext& context, Scope& scope) const
{
    CERLIB_UNUSED(scope);
    return *this;
}

std::span<const std::unique_ptr<StructFieldDecl>> StructDecl::get_fields() const
{
    return m_fields;
}

Decl* StructDecl::find_member_symbol(const SemaContext& context, std::string_view name) const
{
    return find_field(name);
}

FunctionDecl* StructDecl::ctor() const
{
    return m_ctor.get();
}

bool StructDecl::is_built_in() const
{
    return m_is_built_in;
}

FunctionDecl::FunctionDecl(const SourceLocation&                              location,
                           std::string_view                                   name,
                           SmallVector<std::unique_ptr<FunctionParamDecl>, 4> parameters,
                           const Type&                                        return_type,
                           std::unique_ptr<CodeBlock>                         body,
                           bool                                               is_struct_ctor)
    : Decl(location, name)
    , m_kind(FunctionKind::Normal)
    , m_parameters(std::move(parameters))
    , m_body(std::move(body))
    , m_is_struct_ctor(is_struct_ctor)
{
    set_type(return_type);
}

FunctionDecl::~FunctionDecl() noexcept = default;

void FunctionDecl::on_verify(SemaContext& context, Scope& scope)
{
    scope.set_current_function(this);

    if (name() == naming::shader_entry_point)
    {
        m_kind = FunctionKind::Shader;
    }

    context.verify_symbol_name(location(), name());

    const bool is_built_in = m_body == nullptr;

    if (!is_built_in && scope.contains_symbol_only_here(name()))
    {
        throw Error{location(), "symbol '{}' is already defined", name()};
    }

    for (const std::unique_ptr<FunctionParamDecl>& param : m_parameters)
    {
        if (!is_built_in)
        {
            scope.add_symbol(*param);
        }

        param->verify(context, scope);
    }

    const Type& return_type = type().resolve(context, scope);

    set_type(return_type);

    // Verify that the function does not return a type that is never allowed to be
    // returned from functions.
    if (return_type.is_array() || return_type.is_image_type())
    {
        throw Error{location(),
                    "invalid function return type; expected a scalar, vector, matrix or "
                    "struct type"};
    }

    std::vector<gsl::not_null<const Decl*>> extra_symbols;

    if (is_shader())
    {
        // Add extra symbols here if the function is a shader.
        const BuiltInSymbols& built_ins = context.built_in_symbols();

        extra_symbols.emplace_back(built_ins.sprite_image.get());
        extra_symbols.emplace_back(built_ins.sprite_color.get());
        extra_symbols.emplace_back(built_ins.sprite_uv.get());
    }

    if (!is_built_in)
    {
        assert(m_body);

        m_body->verify(context, scope, extra_symbols);

        if (m_body->stmts().empty())
        {
            throw Error{location(), "function (= {}) must contain at least one statement", name()};
        }

        for (const std::unique_ptr<FunctionParamDecl>& param : m_parameters)
        {
            scope.remove_symbol(*param);
        }
    }

    scope.add_symbol(*this);

    if (is_shader())
    {
        // Shaders must have exactly one return statement, which is the last
        // statement.
        if (!isa<ReturnStmt>(m_body->stmts().back().get()))
        {
            throw Error{location(),
                        "shader (= {}) must return exactly one value, at the end",
                        name()};
        }

        // Check parameters. Only vertex shaders can have an empty parameter list.
        // This allows the user to draw primitives without any vertex buffer (and
        // thus no input layout) set.
        if (!is_shader())
        {
            if (m_parameters.empty())
            {
                throw Error{location(),
                            "non-vertex shader (= {}) must have at least one "
                            "parameter: the stage input",
                            name()};
            }
        }

        // Verify shader stage input parameter (if any)
        if (!m_parameters.empty())
        {
            const std::unique_ptr<FunctionParamDecl>& param = m_parameters.front();

            if (m_parameters.size() > 1)
            {
                throw Error{location(), "a shader function must not have more than one parameter"};
            }

            if (param->name() != naming::shader_stage_input_param)
            {
                throw Error{location(),
                            "the parameter of a shader function must be named '{}'",
                            naming::shader_stage_input_param};
            }

            if (!param->type().is_user_defined_struct())
            {
                throw Error{location(),
                            "the input vertex must be of a user-defined structure type"};
            }

            param->m_kind = FunctionParamKind::ShaderStageInput;
        }
    }

    if (is_shader())
    {
        if (&type() != &Vector4Type::instance())
        {
            throw Error{location(),
                        "a pixel shader must return a value of type {} or a structure",
                        Vector4Type::instance().type_name()};
        }
    }

    if (m_body)
    {
        // Check actual returned type with the function's declared return type.
        const ReturnStmt* return_stmt = asa<ReturnStmt>(m_body->stmts().back().get());

        if (return_stmt == nullptr)
        {
            throw Error(m_body->stmts().back()->location(), "expected a return statement");
        }

        SemaContext::verify_type_assignment(type(), return_stmt->expr(), false);
    }
}

FunctionParamDecl* FunctionDecl::find_parameter(std::string_view name) const
{
    if (const auto it =
            std::ranges::find_if(m_parameters, [name](const auto& e) { return e->name() == name; });
        it != m_parameters.cend())
    {
        return it->get();
    }

    return nullptr;
}

std::span<const std::unique_ptr<FunctionParamDecl>> FunctionDecl::parameters() const
{
    return m_parameters;
}

bool FunctionDecl::accesses_symbol(const Decl& symbol, bool transitive) const
{
    if (const StructDecl* strct = asa<StructDecl>(&symbol))
    {
        if (&type() == strct)
        {
            return true;
        }

        if (std::ranges::any_of(m_parameters,
                                [strct](const auto& param) { return &param->type() == strct; }))
        {
            return true;
        }
    }

    return m_body && m_body->accesses_symbol(symbol, transitive);
}

CodeBlock* FunctionDecl::body()
{
    return m_body.get();
}

const CodeBlock* FunctionDecl::body() const
{
    return m_body.get();
}

FunctionKind FunctionDecl::kind() const
{
    return m_kind;
}

bool FunctionDecl::is(FunctionKind kind) const
{
    return this->kind() == kind;
}

bool FunctionDecl::is_normal_function() const
{
    return is(FunctionKind::Normal);
}

bool FunctionDecl::is_shader() const
{
    return is(FunctionKind::Shader);
}

bool FunctionDecl::is_struct_ctor() const
{
    return m_is_struct_ctor;
}

FunctionParamDecl::FunctionParamDecl(const SourceLocation& location,
                                     std::string_view      name,
                                     const Type&           type)
    : FunctionParamDecl(location, name, FunctionParamKind::Normal, type)
{
}

FunctionParamDecl::FunctionParamDecl(const SourceLocation& location,
                                     std::string_view      name,
                                     FunctionParamKind     kind,
                                     const Type&           type)
    : Decl(location, name)
    , m_kind(kind)
{
    set_type(type);
}

FunctionParamDecl::~FunctionParamDecl() noexcept = default;

void FunctionParamDecl::on_verify(SemaContext& context, Scope& scope)
{
    const Type& type = this->type().resolve(context, scope);

    set_type(type);

    if (const FunctionDecl* function = scope.current_function();
        function != nullptr && function->body())
    {
        if (type.is_array() || type.is_image_type())
        {
            throw Error{location(),
                        "invalid type for function parameter; expected a scalar, vector, "
                        "matrix or struct type"};
        }
    }
}

FunctionParamKind FunctionParamDecl::kind() const
{
    return m_kind;
}

ForLoopVariableDecl::ForLoopVariableDecl(const SourceLocation& location, std::string_view name)
    : Decl(location, name)
{
}

void ForLoopVariableDecl::on_verify(SemaContext& context, Scope& scope)
{
    scope.add_symbol(*this);
}

ShaderParamDecl::ShaderParamDecl(const SourceLocation& location,
                                 std::string_view      name,
                                 const Type&           type,
                                 std::unique_ptr<Expr> default_value_expr)
    : Decl(location, name)
    , m_default_value_expr(std::move(default_value_expr))
{
    set_type(type);
}

ShaderParamDecl::~ShaderParamDecl() noexcept = default;

void ShaderParamDecl::on_verify(SemaContext& context, Scope& scope)
{
    const Type& type = this->type().resolve(context, scope);

    set_type(type);

    if (!type.can_be_shader_parameter())
    {
        throw Error{location(), "type '{}' cannot be used as a shader parameter", type.type_name()};
    }

    if (m_default_value_expr)
    {
        m_default_value_expr->verify(context, scope);

        const std::any constant_value =
            m_default_value_expr->evaluate_constant_value(context, scope);

        if (!constant_value.has_value())
        {
            throw Error{m_default_value_expr->location(),
                        "the default value of a shader parameter must be a constant "
                        "expression"};
        }

        m_default_value = constant_value;

        if (type.is_image_type())
        {
            // Images are assigned integer values (that correspond to the respective
            // image slot).
            SemaContext::verify_type_assignment(IntType::instance(), *m_default_value_expr, true);
        }
        else
        {
            SemaContext::verify_type_assignment(type, *m_default_value_expr, false);
        }
    }

    scope.add_symbol(*this);
}

bool ShaderParamDecl::is_array() const
{
    assert(is_verified());
    return isa<ArrayType>(type());
}

uint16_t ShaderParamDecl::array_size() const
{
    assert(is_verified());

    const gsl::not_null array_type{asa<ArrayType>(&type())};

    return gsl::narrow_cast<uint16_t>(array_type->size());
}

const Expr* ShaderParamDecl::default_value_expr() const
{
    return m_default_value_expr.get();
}

const std::any& ShaderParamDecl::default_value() const
{
    return m_default_value;
}

VarDecl::VarDecl(const SourceLocation& location,
                 std::string_view      name,
                 std::unique_ptr<Expr> expr,
                 bool                  is_const)
    : Decl(location, name)
    , m_is_const(is_const)
    , m_expr(std::move(expr))
{
}

VarDecl::VarDecl(std::string_view name, const Type& type)
    : Decl(SourceLocation::std, name)
    , m_is_const(true)
    , m_is_system_value(true)
{
    // A valid type must be known beforehand
    assert(!type.is_unresolved());
    set_type(type);
}

VarDecl::~VarDecl() noexcept = default;

void VarDecl::on_verify(SemaContext& context, Scope& scope)
{
    if (m_is_system_value)
    {
        assert(!type().is_unresolved());
    }
    else
    {
        context.verify_symbol_name(location(), name());

        m_expr->verify(context, scope);
        set_type(m_expr->type());
    }

    scope.add_symbol(*this);
}

bool VarDecl::is_const() const
{
    return m_is_const;
}

bool VarDecl::is_system_value() const
{
    return m_is_system_value;
}

const Expr& VarDecl::expr() const
{
    return *m_expr;
}
} // namespace cer::shadercompiler
