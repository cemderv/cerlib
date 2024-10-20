// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Expr.hpp"

#include "BinOpTable.hpp"
#include "cerlib/Color.hpp"
#include "cerlib/Vector2.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/SemaContext.hpp"
#include "shadercompiler/Type.hpp"
#include <cassert>
#include <cerlib/InternalError.hpp>
#include <optional>
#include <unordered_set>
#include <utility>

namespace cer::shadercompiler
{
static auto bin_op_kind_display_string(BinOpKind kind) -> std::string_view
{
    switch (kind)
    {
        case BinOpKind::Add: return "+";
        case BinOpKind::Subtract: return "-";
        case BinOpKind::Multiply: return "*";
        case BinOpKind::Divide: return "/";
        case BinOpKind::LogicalAnd: return "&&";
        case BinOpKind::LogicalOr: return "||";
        case BinOpKind::LessThan: return "<";
        case BinOpKind::LessThanOrEqual: return "<=";
        case BinOpKind::GreaterThan: return ">";
        case BinOpKind::GreaterThanOrEqual: return ">=";
        case BinOpKind::MemberAccess: return ".";
        case BinOpKind::BitwiseXor: return "^";
        case BinOpKind::BitwiseAnd: return "&";
        case BinOpKind::Equal: return "==";
        case BinOpKind::NotEqual: return "!=";
        case BinOpKind::RightShift: return ">>";
        case BinOpKind::BitwiseOr: return "|";
        case BinOpKind::LeftShift: return "<<";
    }

    throw std::invalid_argument{"Invalid BinOpKind"};
}

Expr::~Expr() noexcept = default;

void Expr::verify(SemaContext& context, Scope& scope)
{
    if (!m_is_verified)
    {
        on_verify(context, scope);

        // Every expression must have a type after verification.
        assert(m_type != nullptr);

        m_is_verified = true;
    }
}

auto Expr::evaluate_constant_value([[maybe_unused]] SemaContext& context,
                                   [[maybe_unused]] Scope&       scope) const -> std::any
{
    return {};
}

auto Expr::is_literal() const -> bool
{
    return false;
}

auto Expr::accesses_symbol(const Decl& symbol, [[maybe_unused]] bool transitive) const -> bool
{
    return m_symbol == &symbol;
}

auto Expr::location() const -> const SourceLocation&
{
    return m_location;
}

auto Expr::type() const -> const Type&
{
    return *m_type;
}

auto Expr::symbol() const -> const Decl*
{
    return m_symbol;
}

Expr::Expr(const SourceLocation& location)
    : m_location(location)
    , m_is_verified(false)
{
}

auto Expr::is_verified() const -> bool
{
    return m_is_verified;
}

void Expr::set_symbol(const Decl* symbol)
{
    m_symbol = symbol;
}

void Expr::set_type(const Type& type)
{
    m_type = &type;
}

void IntLiteralExpr::on_verify([[maybe_unused]] SemaContext& context, [[maybe_unused]] Scope& scope)
{
}

IntLiteralExpr::IntLiteralExpr(const SourceLocation& location, int32_t value)
    : Expr(location)
    , m_value(value)
{
    set_type(IntType::instance());
}

auto IntLiteralExpr::value() const -> int32_t
{
    return m_value;
}

auto IntLiteralExpr::evaluate_constant_value([[maybe_unused]] SemaContext& context,
                                             [[maybe_unused]] Scope&       scope) const -> std::any
{
    return m_value;
}

auto IntLiteralExpr::is_literal() const -> bool
{
    return true;
}

void BoolLiteralExpr::on_verify([[maybe_unused]] SemaContext& context,
                                [[maybe_unused]] Scope&       scope)
{
}

BoolLiteralExpr::BoolLiteralExpr(const SourceLocation& location, bool value)
    : Expr(location)
    , m_value(value)
{
    set_type(BoolType::instance());
}

auto BoolLiteralExpr::value() const -> bool
{
    return m_value;
}

auto BoolLiteralExpr::evaluate_constant_value([[maybe_unused]] SemaContext& context,
                                              [[maybe_unused]] Scope&       scope) const -> std::any
{
    return m_value;
}

auto BoolLiteralExpr::is_literal() const -> bool
{
    return true;
}

void FloatLiteralExpr::on_verify([[maybe_unused]] SemaContext& context,
                                 [[maybe_unused]] Scope&       scope)
{
}

auto FloatLiteralExpr::string_value() const -> std::string_view
{
    return m_string_value;
}

FloatLiteralExpr::FloatLiteralExpr(const SourceLocation& location,
                                   std::string_view      string_value,
                                   double                value)
    : Expr(location)
    , m_string_value(string_value)
    , m_value(value)
{
    set_type(FloatType::instance());
}

auto FloatLiteralExpr::value() const -> double
{
    return m_value;
}

auto FloatLiteralExpr::evaluate_constant_value([[maybe_unused]] SemaContext& context,
                                               [[maybe_unused]] Scope& scope) const -> std::any
{
    return m_value;
}

auto FloatLiteralExpr::is_literal() const -> bool
{
    return true;
}

void BinOpExpr::on_verify(SemaContext& context, Scope& scope)
{
    m_lhs->verify(context, scope);

    if (is(BinOpKind::MemberAccess))
    {
        if (auto* sym_access = asa<SymAccessExpr>(m_rhs.get()))
        {
            sym_access->m_ancestor_expr = m_lhs.get();
        }
    }

    m_rhs->verify(context, scope);

    if (m_bin_op_kind == BinOpKind::MemberAccess)
    {
        // The member we have accessed dictates our type.
        set_type(m_rhs->type());
        set_symbol(m_rhs->symbol());
    }
    else
    {
        // The binary operation dictates our type.
        const auto& bin_op_table = context.bin_op_table();

        const auto* result_type =
            bin_op_table.bin_op_result_type(m_bin_op_kind, m_lhs->type(), m_rhs->type());

        if (result_type == nullptr)
        {
            throw Error{location(),
                        "operator '{}' is not defined between types '{}' and '{}'",
                        bin_op_kind_display_string(m_bin_op_kind),
                        m_lhs->type().type_name(),
                        m_rhs->type().type_name()};
        }

        set_type(*result_type);
    }
}

auto BinOpExpr::evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any
{
    const auto lhs = m_lhs->evaluate_constant_value(context, scope);
    const auto rhs = m_rhs->evaluate_constant_value(context, scope);

    if (lhs.has_value() && rhs.has_value())
    {
        if (lhs.type() != rhs.type())
        {
            return {};
        }

        if (const auto *lhs_int = std::any_cast<int32_t>(&lhs),
            *rhs_int            = std::any_cast<int32_t>(&rhs);
            lhs_int != nullptr && rhs_int != nullptr)
        {
            switch (m_bin_op_kind)
            {
                case BinOpKind::Add: return *lhs_int + *rhs_int;
                case BinOpKind::Subtract: return *lhs_int - *rhs_int;
                case BinOpKind::Multiply: return *lhs_int * *rhs_int;
                case BinOpKind::Divide: return *lhs_int / *rhs_int;
                case BinOpKind::LessThan: return *lhs_int < *rhs_int;
                case BinOpKind::LessThanOrEqual: return *lhs_int <= *rhs_int;
                case BinOpKind::GreaterThan: return *lhs_int > *rhs_int;
                case BinOpKind::GreaterThanOrEqual: return *lhs_int >= *rhs_int;
                case BinOpKind::BitwiseXor: return *lhs_int ^ *rhs_int;
                case BinOpKind::BitwiseAnd: return *lhs_int & *rhs_int;
                case BinOpKind::Equal: return *lhs_int == *rhs_int;
                case BinOpKind::NotEqual: return *lhs_int != *rhs_int;
                case BinOpKind::RightShift: return *lhs_int >> *rhs_int;
                case BinOpKind::BitwiseOr: return *lhs_int | *rhs_int;
                case BinOpKind::LeftShift: return *lhs_int << *rhs_int;
                default: return {};
            }
        }

        if (const auto *lhs_float = std::any_cast<float>(&lhs),
            *rhs_float            = std::any_cast<float>(&rhs);
            lhs_float != nullptr && rhs_float != nullptr)
        {
            switch (m_bin_op_kind)
            {
                case BinOpKind::Add: return *lhs_float + *rhs_float;
                case BinOpKind::Subtract: return *lhs_float - *rhs_float;
                case BinOpKind::Multiply: return *lhs_float * *rhs_float;
                case BinOpKind::Divide: return *lhs_float / *rhs_float;
                case BinOpKind::LessThan: return *lhs_float < *rhs_float;
                case BinOpKind::LessThanOrEqual: return *lhs_float <= *rhs_float;
                case BinOpKind::GreaterThan: return *lhs_float > *rhs_float;
                case BinOpKind::GreaterThanOrEqual: return *lhs_float >= *rhs_float;
                case BinOpKind::Equal: return *lhs_float == *rhs_float;
                case BinOpKind::NotEqual: return *lhs_float != *rhs_float;
                default: return {};
            }
        }

        if (const auto *lhs_vector2 = std::any_cast<Vector2>(&lhs),
            *rhs_vector2            = std::any_cast<Vector2>(&rhs);
            lhs_vector2 != nullptr && rhs_vector2 != nullptr)
        {
            switch (m_bin_op_kind)
            {
                case BinOpKind::Add: return *lhs_vector2 + *rhs_vector2;
                case BinOpKind::Subtract: return *lhs_vector2 - *rhs_vector2;
                case BinOpKind::Multiply: return *lhs_vector2 * *rhs_vector2;
                case BinOpKind::Divide: return *lhs_vector2 / *rhs_vector2;
                case BinOpKind::Equal: return *lhs_vector2 == *rhs_vector2;
                case BinOpKind::NotEqual: return *lhs_vector2 != *rhs_vector2;
                default: return {};
            }
        }
    }

    return {};
}

auto BinOpExpr::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return m_lhs->accesses_symbol(symbol, transitive) || m_rhs->accesses_symbol(symbol, transitive);
}

BinOpExpr::BinOpExpr(const SourceLocation& location,
                     BinOpKind             kind,
                     std::unique_ptr<Expr> lhs,
                     std::unique_ptr<Expr> rhs)
    : Expr(location)
    , m_bin_op_kind(kind)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{
}

auto BinOpExpr::bin_op_kind() const -> BinOpKind
{
    return m_bin_op_kind;
}

auto BinOpExpr::lhs() const -> const Expr&
{
    return *m_lhs;
}

auto BinOpExpr::rhs() const -> const Expr&
{
    return *m_rhs;
}

auto BinOpExpr::is(BinOpKind kind) const -> bool
{
    return m_bin_op_kind == kind;
}

void StructCtorCall::on_verify(SemaContext& context, Scope& scope)
{
    m_callee->verify(context, scope);
    set_symbol(m_callee->symbol());

    const auto* ctor = asa<FunctionDecl>(symbol());

    if (ctor == nullptr || !ctor->is_struct_ctor())
    {
        throw Error{location(), "call does not represent a struct initialization"};
    }

    const auto* strct = asa<StructDecl>(&ctor->type());

    assert(strct != nullptr);

    if (!m_args.empty())
    {
        std::unordered_set<std::string_view> already_initialized_fields;

        for (const auto& arg : m_args)
        {
            const auto field_name = arg->name();

            if (already_initialized_fields.contains(field_name))
            {
                throw Error{arg->location(),
                            "duplicate initialization of struct field '{}'",
                            field_name};
            }

            const auto* field = strct->find_field(field_name);

            if (field == nullptr)
            {
                throw Error{arg->location(),
                            "struct '{}' does not have any field named '{}'",
                            strct->name(),
                            field_name};
            }

            arg->verify(context, scope);

            SemaContext::verify_type_assignment(field->type(), *arg, false);

            already_initialized_fields.insert(field_name);
        }

        for (const auto& field : strct->get_fields())
        {
            if (const auto it = already_initialized_fields.find(field->name());
                it == already_initialized_fields.cend())
            {
                throw Error{location(),
                            "missing initializer for '{}.{}'",
                            strct->name(),
                            field->name()};
            }
        }
    }

    set_type(symbol()->type());
}

StructCtorCall::StructCtorCall(const SourceLocation&                     location,
                               std::unique_ptr<Expr>                     callee,
                               UniquePtrList<StructCtorArg, 4> args)
    : Expr(location)
    , m_callee(std::move(callee))
    , m_args(std::move(args))
{
}

auto StructCtorCall::callee() const -> const Expr&
{
    return *m_callee;
}

auto StructCtorCall::args() const -> std::span<const std::unique_ptr<StructCtorArg>>
{
    return m_args;
}

auto StructCtorCall::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    if (m_callee->accesses_symbol(symbol, transitive))
    {
        return true;
    }

    if (transitive)
    {
        if (const auto* sym_access = asa<SymAccessExpr>(m_callee.get()))
        {
            if (const auto* func = asa<FunctionDecl>(sym_access->symbol()))
            {
                if (const auto* strct = asa<StructDecl>(&func->type()); strct == &symbol)
                {
                    return true;
                }

                if (func->body() != nullptr && func->body()->accesses_symbol(symbol, transitive))
                {
                    return true;
                }
            }
        }
    }

    return std::ranges::any_of(m_args,
                               [&symbol, transitive](const std::unique_ptr<StructCtorArg>& expr) {
                                   return expr->accesses_symbol(symbol, transitive);
                               });
}

void SubscriptExpr::on_verify(SemaContext& context, Scope& scope)
{
    m_expr->verify(context, scope);

    set_symbol(m_expr->symbol());
    assert(symbol());

    m_index_expr->verify(context, scope);

    const auto& index_type = m_index_expr->type();

    if (&index_type != &IntType::instance())
    {
        throw Error{m_index_expr->location(),
                    "'{}' cannot be used to index into an array; expected '{}'",
                    index_type.type_name(),
                    IntType::instance().type_name()};
    }

    const auto* array_type = asa<ArrayType>(&symbol()->type());

    if (array_type == nullptr)
    {
        throw Error{m_index_expr->location(),
                    "cannot index into non-array type '{}'",
                    symbol()->type().type_name()};
    }

    const auto array_size = array_type->size();

    auto constant_index = std::optional<uint32_t>{};
    {
        const auto constant_value = m_index_expr->evaluate_constant_value(context, scope);

        if (constant_value.has_value())
        {
            if (const auto* int_index = std::any_cast<int32_t>(&constant_value))
            {
                if (*int_index < 0)
                {
                    throw Error{location(),
                                "negative index is not allowed (specified index = {})",
                                *int_index};
                }

                constant_index = narrow_cast<uint32_t>(*int_index);
            }
            else if (const auto* uint_index = std::any_cast<uint32_t>(&constant_value))
            {
                constant_index = *uint_index;
            }
        }
    }

    if (constant_index)
    {
        if (*constant_index >= array_size)
        {
            throw Error{location(),
                        "index (= {}) exceeds the array's size (= {})",
                        *constant_index,
                        array_size};
        }
    }

    set_type(array_type->element_type());
}

auto SubscriptExpr::expr() const -> const Expr&
{
    return *m_expr;
}

auto SubscriptExpr::index_expr() const -> const Expr&
{
    return *m_index_expr;
}

auto SubscriptExpr::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return m_expr->accesses_symbol(symbol, transitive) ||
           m_index_expr->accesses_symbol(symbol, transitive);
}

SubscriptExpr::SubscriptExpr(const SourceLocation& location,
                             std::unique_ptr<Expr> expr,
                             std::unique_ptr<Expr> index_expr)
    : Expr(location)
    , m_expr(std::move(expr))
    , m_index_expr(std::move(index_expr))
{
}

SymAccessExpr::SymAccessExpr(const SourceLocation& location, Decl& symbol)
    : Expr(location)
    , m_ancestor_expr()
{
    m_name = symbol.name();
    set_symbol(&symbol);
    set_type(symbol.type());
}

void SymAccessExpr::on_verify(SemaContext& context, Scope& scope)
{
    const auto& built_ins = context.built_in_symbols();

    if (m_ancestor_expr != nullptr)
    {
        // This is a member access. Search the symbol within the type (i.e. a
        // member).
        const auto& ancestor_type = m_ancestor_expr->type();
        const auto* member_symbol = ancestor_type.find_member_symbol(context, m_name);

        if (member_symbol == nullptr)
        {
            throw Error{location(),
                        "type '{}' has no member named '{}'",
                        ancestor_type.type_name(),
                        m_name};
        }

        set_symbol(member_symbol);
    }
    else if (scope.context() == ScopeContext::FunctionCall)
    {
        // We're looking up a symbol that represents a function call.
        // Because we support overloading, we have to look for the correct function
        // depending on the currently passed argument types.
        const auto& args                      = scope.function_call_args();
        auto        was_function_found_at_all = false;
        auto        all_functions_that_match  = List<const FunctionDecl*, 8>{};

        for (const auto& symbol : scope.find_symbols(m_name, true))
        {
            if (const auto* function = asa<FunctionDecl>(&symbol.get()))
            {
                const auto accepts_implicitly_cast_arguments =
                    built_ins.accepts_implicitly_cast_arguments(*function);

                was_function_found_at_all = true;

                const auto params = function->parameters();

                if (params.size() != args.size())
                {
                    continue;
                }

                auto do_param_types_match = true;

                for (size_t i = 0; i < args.size(); ++i)
                {
                    if (!SemaContext::can_assign(params[i]->type(),
                                                 args[i],
                                                 accepts_implicitly_cast_arguments))
                    {
                        do_param_types_match = false;
                        break;
                    }
                }

                if (do_param_types_match)
                {
                    // We've got a match.
                    all_functions_that_match.push_back(function);
                }
            }
        }

        const auto build_call_string = [&] {
            auto str = std::string{m_name};
            str += '(';
            for (const auto& arg_ref : args)
            {
                const auto& arg = arg_ref.get();
                str += arg.type().type_name();

                if (&arg != &args.back().get())
                {
                    str += ", ";
                }
            }
            str += ')';
            return str;
        };

        if (all_functions_that_match.empty())
        {
            if (was_function_found_at_all)
            {
                throw Error{location(),
                            "no matching overload for function call '{}'",
                            build_call_string()};
            }

            throw Error{location(), "function '{}(...)' not found", m_name};
        }

        if (all_functions_that_match.size() > 1)
        {
            throw Error{location(), "ambiguous call for '{}'", build_call_string()};
        }

        assert(all_functions_that_match.size() == 1);
        set_symbol(all_functions_that_match.front());
    }
    else
    {
        set_symbol(scope.find_symbol(m_name));
    }

    if (symbol() == nullptr)
    {
        // See if there's a similarly named symbol. If so, suggest it in the error
        // message.
        if (const auto similar_symbol = scope.find_symbol_with_similar_name(m_name);
            m_name.size() > 2 && similar_symbol != nullptr)
        {
            throw Error{location(),
                        "symbol '{}' not found; did you mean '{}'?",
                        m_name,
                        similar_symbol->name()};
        }

        throw Error{location(), "symbol '{}' not found", m_name};
    }

    set_type(symbol()->type());
}

auto SymAccessExpr::evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any
{
    if (const auto* variable = asa<VarDecl>(symbol()))
    {
        return variable->expr().evaluate_constant_value(context, scope);
    }

    return {};
}

auto SymAccessExpr::accesses_symbol(const Decl& symbol, bool /*transitive*/) const -> bool
{
    return this->symbol() == &symbol;
}

SymAccessExpr::SymAccessExpr(const SourceLocation& location, std::string_view name)
    : Expr(location)
    , m_name(name)
    , m_ancestor_expr(nullptr)
{
}

auto SymAccessExpr::name() const -> std::string_view
{
    return symbol() != nullptr ? symbol()->name() : m_name;
}

void FunctionCallExpr::on_verify(SemaContext& context, Scope& scope)
{
    auto args = RefList<const Expr, 4>{};
    args.reserve(m_args.size());

    for (const auto& arg : m_args)
    {
        arg->verify(context, scope);
        args.emplace_back(*arg);
    }

    scope.push_context(ScopeContext::FunctionCall);
    scope.set_function_call_args(std::move(args));

    m_callee->verify(context, scope);
    set_symbol(m_callee->symbol());

    scope.set_function_call_args({});
    scope.pop_context();

    set_type(symbol()->type());

    if (const auto* called_function = asa<FunctionDecl>(m_callee->symbol()))
    {
        if (called_function->is_shader())
        {
            throw Error{location(), "cannot call a shader entry point"};
        }
    }
}

auto FunctionCallExpr::args() const -> std::span<const std::unique_ptr<Expr>>
{
    return m_args;
}

auto FunctionCallExpr::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    if (m_callee->accesses_symbol(symbol, transitive))
    {
        return true;
    }

    if (transitive)
    {
        if (const auto* sym_access = asa<SymAccessExpr>(m_callee.get()))
        {
            if (const auto* func = asa<FunctionDecl>(sym_access->symbol()); func != nullptr)
            {
                if (const auto* strct = asa<StructDecl>(&func->type()); strct == &symbol)
                {
                    return true;
                }

                if (func->body() != nullptr && func->body()->accesses_symbol(symbol, transitive))
                {
                    return true;
                }
            }
        }
    }

    return std::ranges::any_of(m_args, [&symbol, transitive](const auto& expr) {
        return expr->accesses_symbol(symbol, transitive);
    });
}

auto FunctionCallExpr::evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any
{
    assert(is_verified());

    const auto& built_ins = context.built_in_symbols();
    const auto& symbol    = *this->callee().symbol();

    const auto get_arg_constant_values = [this, &context, &scope] {
        auto arg_values = List<std::any, 4>{};
        arg_values.reserve(m_args.size());

        for (const auto& arg : m_args)
        {
            auto value = arg->evaluate_constant_value(context, scope);
            if (!value.has_value())
            {
                arg_values.clear();
                break;
            }

            arg_values.push_back(std::move(value));
        }

        return arg_values;
    };

    const auto expect_and_get_float = [](const std::any& value) {
        if (const auto* f = std::any_cast<float>(&value))
        {
            return *f;
        }

        if (const auto* i = std::any_cast<int32_t>(&value))
        {
            return float(*i);
        }

        if (const auto* ui = std::any_cast<uint32_t>(&value))
        {
            return float(*ui);
        }

        CER_THROW_INTERNAL_ERROR_STR("expected float argument");
    };

    const auto expect_and_get_vector2 = [](const std::any& value) {
        if (const auto* v = std::any_cast<Vector2>(&value))
        {
            return *v;
        }

        CER_THROW_INTERNAL_ERROR("expected argument of type '{}'",
                                 Vector2Type::instance().type_name());
    };

    const auto expect_and_get_vector3 = [](const std::any& value) {
        if (const auto* v = std::any_cast<Vector3>(&value))
        {
            return *v;
        }

        CER_THROW_INTERNAL_ERROR("expected argument of type '{}'",
                                 Vector3Type::instance().type_name());
    };

    if (built_ins.is_float_ctor(symbol))
    {
        const auto values = get_arg_constant_values();

        if (values.empty())
        {
            return {};
        }

        return expect_and_get_float(values.at(0));
    }

    if (built_ins.is_int_ctor(symbol))
    {
        CER_THROW_NOT_IMPLEMENTED("implicit conversion to int");
    }

    if (built_ins.is_uint_ctor(symbol))
    {
        CER_THROW_NOT_IMPLEMENTED("implicit conversion to unsigned int");
    }

    if (built_ins.is_vector2_ctor(symbol))
    {
        const auto values = get_arg_constant_values();

        if (values.empty())
        {
            return {};
        }

        if (&symbol == built_ins.vector2_ctor_x_y.get())
        {
            const auto x = expect_and_get_float(values.at(0));
            const auto y = expect_and_get_float(values.at(1));

            return Vector2{x, y};
        }

        if (&symbol == built_ins.vector2_ctor_xy.get())
        {
            return Vector2{expect_and_get_float(values.at(0))};
        }

        CER_THROW_INTERNAL_ERROR_STR("unknown Vector constructor call");
    }

    if (built_ins.is_vector4_ctor(symbol))
    {
        const auto values = get_arg_constant_values();

        if (values.empty())
        {
            return {};
        }

        if (&symbol == built_ins.vector4_ctor_x_y_z_w.get())
        {
            const auto x = expect_and_get_float(values.at(0));
            const auto y = expect_and_get_float(values.at(1));
            const auto z = expect_and_get_float(values.at(2));
            const auto w = expect_and_get_float(values.at(3));

            return Vector4{x, y, z, w};
        }

        if (&symbol == built_ins.vector4_ctor_xy_zw.get())
        {
            const auto xy = expect_and_get_vector2(values.at(0));
            const auto zw = expect_and_get_vector2(values.at(1));

            return Vector4{xy, zw};
        }

        if (&symbol == built_ins.vector4_ctor_xy_z_w.get())
        {
            const auto xy = expect_and_get_vector2(values.at(0));
            const auto z  = expect_and_get_float(values.at(1));
            const auto w  = expect_and_get_float(values.at(2));

            return Vector4{xy, z, w};
        }

        if (&symbol == built_ins.vector4_ctor_xyz_w.get())
        {
            const auto xyz = expect_and_get_vector3(values.at(0));
            const auto w   = expect_and_get_float(values.at(1));

            return Vector4{xyz, w};
        }

        CER_THROW_INTERNAL_ERROR_STR("unknown Vector3 ctor call");
    }

    return {};
}

auto FunctionCallExpr::callee() const -> const Expr&
{
    return *m_callee;
}

FunctionCallExpr::FunctionCallExpr(const SourceLocation&            location,
                                   std::unique_ptr<Expr>            callee,
                                   UniquePtrList<Expr, 4> args)
    : Expr(location)
    , m_callee(std::move(callee))
    , m_args(std::move(args))
{
}

ScientificIntLiteralExpr::ScientificIntLiteralExpr(const SourceLocation& location,
                                                   std::string_view      value)
    : Expr(location)
    , m_value(value)
{
    set_type(FloatType::instance());
}

void ScientificIntLiteralExpr::on_verify([[maybe_unused]] SemaContext& context,
                                         [[maybe_unused]] Scope&       scope)
{
}

auto ScientificIntLiteralExpr::value() const -> std::string_view
{
    return m_value;
}

void HexadecimalIntLiteralExpr::on_verify([[maybe_unused]] SemaContext& context,
                                          [[maybe_unused]] Scope&       scope)
{
    set_type(IntType::instance());
}

HexadecimalIntLiteralExpr::HexadecimalIntLiteralExpr(const SourceLocation& location,
                                                     std::string_view      value)
    : Expr(location)
    , m_value(value)
{
}

auto HexadecimalIntLiteralExpr::value() const -> std::string_view
{
    return m_value;
}

RangeExpr::RangeExpr(const SourceLocation& location,
                     std::unique_ptr<Expr> start,
                     std::unique_ptr<Expr> end)
    : Expr(location)
    , m_start(std::move(start))
    , m_end(std::move(end))
{
}

void RangeExpr::on_verify(SemaContext& context, Scope& scope)
{
    m_start->verify(context, scope);
    m_end->verify(context, scope);

    if (&m_start->type() != &m_end->type())
    {
        throw Error{location(),
                    "type mismatch between range start and end ({} to {})",
                    m_start->type().type_name(),
                    m_end->type().type_name()};
    }

    set_type(m_start->type());
}

auto RangeExpr::start() const -> const Expr&
{
    return *m_start;
}

auto RangeExpr::end() const -> const Expr&
{
    return *m_end;
}

auto RangeExpr::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return m_start->accesses_symbol(symbol, transitive) ||
           m_end->accesses_symbol(symbol, transitive);
}

UnaryOpExpr::UnaryOpExpr(const SourceLocation& location,
                         UnaryOpKind           kind,
                         std::unique_ptr<Expr> expr)
    : Expr(location)
    , m_kind(kind)
    , m_expr(std::move(expr))
{
}

void UnaryOpExpr::on_verify(SemaContext& context, Scope& scope)
{
    m_expr->verify(context, scope);

    set_type(m_expr->type());
    set_symbol(m_expr->symbol());
}

auto UnaryOpExpr::unary_op_kind() const -> UnaryOpKind
{
    return m_kind;
}

auto UnaryOpExpr::expr() const -> const Expr&
{
    return *m_expr;
}

auto UnaryOpExpr::evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any
{
    const std::any value = m_expr->evaluate_constant_value(context, scope);

    if (!value.has_value())
    {
        return {};
    }

    if (const auto* i = std::any_cast<int32_t>(&value))
    {
        if (m_kind == UnaryOpKind::Negate)
        {
            return -*i;
        }
    }
    else if (const auto* f = std::any_cast<float>(&value))
    {
        if (m_kind == UnaryOpKind::Negate)
        {
            return -*f;
        }
    }
    else if (const auto* b = std::any_cast<bool>(&value))
    {
        if (m_kind == UnaryOpKind::LogicalNot)
        {
            return !*b;
        }
    }

    return {};
}

auto UnaryOpExpr::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return m_expr->accesses_symbol(symbol, transitive);
}

StructCtorArg::StructCtorArg(const SourceLocation& location,
                             std::string_view      name,
                             std::unique_ptr<Expr> expr)
    : Expr(location)
    , m_name(name)
    , m_expr(std::move(expr))
{
}

void StructCtorArg::on_verify(SemaContext& context, Scope& scope)
{
    m_expr->verify(context, scope);
    set_type(m_expr->type());
}

auto StructCtorArg::name() const -> std::string_view
{
    return m_name;
}

auto StructCtorArg::expr() const -> const Expr&
{
    return *m_expr;
}

auto StructCtorArg::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return m_expr->accesses_symbol(symbol, transitive);
}

ParenExpr::ParenExpr(const SourceLocation& location, std::unique_ptr<Expr> expr)
    : Expr(location)
    , m_expr(std::move(expr))
{
}

void ParenExpr::on_verify(SemaContext& context, Scope& scope)
{
    m_expr->verify(context, scope);
    set_type(m_expr->type());
    set_symbol(m_expr->symbol());
}

auto ParenExpr::expr() const -> const Expr&
{
    return *m_expr;
}

auto ParenExpr::evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any
{
    return m_expr->evaluate_constant_value(context, scope);
}

auto ParenExpr::accesses_symbol(const Decl& symbol, bool transitive) const -> bool
{
    return m_expr->accesses_symbol(symbol, transitive);
}

TernaryExpr::TernaryExpr(const SourceLocation& location,
                         std::unique_ptr<Expr> condition_expr,
                         std::unique_ptr<Expr> true_expr,
                         std::unique_ptr<Expr> false_expr)
    : Expr(location)
    , m_condition_expr(std::move(condition_expr))
    , m_true_expr(std::move(true_expr))
    , m_false_expr(std::move(false_expr))
{
}

void TernaryExpr::on_verify(SemaContext& context, Scope& scope)
{
    m_condition_expr->verify(context, scope);
    m_true_expr->verify(context, scope);
    m_false_expr->verify(context, scope);

    if (&m_true_expr->type() != &m_false_expr->type())
    {
        throw Error{location(),
                    "type mismatch between true-expression ('{}') and false-expression "
                    "('{}'); both expressions must be of "
                    "the same type",
                    m_true_expr->type().type_name(),
                    m_false_expr->type().type_name()};
    }

    set_type(m_true_expr->type());
}

auto TernaryExpr::condition_expr() const -> const Expr&
{
    return *m_condition_expr;
}

auto TernaryExpr::true_expr() const -> const Expr&
{
    return *m_true_expr;
}

auto TernaryExpr::false_expr() const -> const Expr&
{
    return *m_false_expr;
}

auto TernaryExpr::evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any
{
    const auto condition_value = m_condition_expr->evaluate_constant_value(context, scope);

    if (!condition_value.has_value())
    {
        return {};
    }

    auto true_value = m_true_expr->evaluate_constant_value(context, scope);

    if (!true_value.has_value())
    {
        return {};
    }

    auto false_value = m_false_expr->evaluate_constant_value(context, scope);

    if (!false_value.has_value())
    {
        return {};
    }

    if (const auto* bool_value = std::any_cast<bool>(&condition_value))
    {
        assert(true_value.type() == false_value.type());

        if (*bool_value)
        {
            return true_value;
        }

        return false_value;
    }

    return {};
}

bool TernaryExpr::accesses_symbol(const Decl& symbol, bool transitive) const
{
    return m_condition_expr->accesses_symbol(symbol, transitive) ||
           m_true_expr->accesses_symbol(symbol, transitive) ||
           m_false_expr->accesses_symbol(symbol, transitive);
}
} // namespace cer::shadercompiler
