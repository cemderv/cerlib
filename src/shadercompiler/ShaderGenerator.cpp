// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/ShaderGenerator.hpp"

#include "cerlib/Util.hpp"
#include "contentmanagement/FileSystem.hpp"
#include "shadercompiler/AST.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Stmt.hpp"
#include "shadercompiler/Type.hpp"
#include "shadercompiler/Writer.hpp"
#include "util/Util.hpp"
#include <cassert>
#include <gsl/util>

namespace cer::shadercompiler
{
ShaderGenerator::ShaderGenerator()
    : m_uniform_buffer_alignment(16)
    , m_needs_float_literal_suffix(true)
{
}

ShaderGenerator::~ShaderGenerator() noexcept = default;

ShaderGenerationResult::ShaderGenerationResult(std::string                        glsl_code,
                                               gsl::not_null<const FunctionDecl*> entry_point,
                                               ParameterList                      parameters)
    : glsl_code(std::move(glsl_code))
    , entry_point(entry_point)
    , parameters(std::move(parameters))
{
}

auto ShaderGenerator::params_accessed_by_function(const FunctionDecl& function) const
    -> AccessedParams
{
    AccessedParams params;
    params.scalars.reserve(8);
    params.resources.reserve(4);

    const auto* body = function.body();

    for (const auto& decl : m_ast->decls())
    {
        const auto* param = asa<ShaderParamDecl>(decl.get());

        if (param == nullptr)
        {
            continue;
        }

        if (const auto& type = param->type(); type.can_be_in_constant_buffer())
        {
            if (body->accesses_symbol(*param, true))
            {
                params.scalars.emplace_back(param);
            }
        }
        else if (type.is_image_type())
        {
            if (body->accesses_symbol(*param, true))
            {
                params.resources.emplace_back(param);
            }
        }
    }

    return params;
}

auto ShaderGenerator::generate(const SemaContext& context,
                               const AST&         ast,
                               std::string_view   entry_point_name,
                               bool               minify) -> ShaderGenerationResult
{
    CERLIB_UNUSED(minify);

    assert(m_ast == nullptr);
    assert(ast.is_verified());

    m_ast = std::addressof(ast);

    const auto _ = gsl::finally([&] {
        m_ast = nullptr;
    });

    const auto shader_name = filesystem::filename_without_extension(ast.filename());

    const auto children_to_generate = gather_ast_decls_to_generate(ast, entry_point_name, context);

    if (children_to_generate.empty())
    {
        throw Error{SourceLocation(), "failed to gather children to generate"};
    }

    const auto* entry_point = asa<FunctionDecl>(children_to_generate.back());

    assert(entry_point != nullptr);
    assert(entry_point->is_shader());

    m_currently_generated_shader_function = entry_point;

    auto code = do_generation(context, *entry_point, children_to_generate);

    m_currently_generated_shader_function = nullptr;

    util::trim_string(code, {{'\n'}});

    if (!code.empty())
    {
        code += '\n';
    }

    const auto accessed_params = params_accessed_by_function(*entry_point);

    auto parameters = gch::small_vector<const ShaderParamDecl*, 8>{};
    parameters.reserve(accessed_params.scalars.size() + accessed_params.resources.size());

    for (const gsl::not_null<const ShaderParamDecl*>& param : accessed_params.scalars)
    {
        parameters.push_back(param.get());
    }

    for (const gsl::not_null<const ShaderParamDecl*>& param : accessed_params.resources)
    {
        parameters.push_back(param.get());
    }

#if 0 // minify doesn't work atm
    if (minify && code.size() > 1)
    {
      auto len = code.size() - 1;
      for (size_t i = len; i > 0; --i)
      {
        if (code[i - 1] == ' ' && code[i] == ' ')
        {
          code.erase(i);
          --len;
        }
      }
    }
#endif

    return ShaderGenerationResult{std::move(code), entry_point, std::move(parameters)};
}

void ShaderGenerator::generate_code_block(Writer&            w,
                                          const CodeBlock&   code_block,
                                          const SemaContext& context)
{
    m_temp_var_name_gen_stack.emplace_back(&code_block);

    for (const auto& stmt : code_block.stmts())
    {
        generate_stmt(w, *stmt, context);
        w << WNewline;
    }

    m_temp_var_name_gen_stack.pop_back();
}

void ShaderGenerator::generate_expr(Writer& w, const Expr& expr, const SemaContext& context)
{
    if (const auto* e = asa<ParenExpr>(&expr))
    {
        w << '(';
        generate_expr(w, e->expr(), context);
        w << ')';
    }
    else if (const auto* int_lit = asa<IntLiteralExpr>(&expr))
    {
        w << int_lit->value();
    }
    else if (const auto* float_lit = asa<FloatLiteralExpr>(&expr))
    {
        w << float_lit->string_value();
        if (m_needs_float_literal_suffix)
        {
            w << 'f';
        }
    }
    else if (const auto* bool_lit = asa<BoolLiteralExpr>(&expr))
    {
        w << bool_lit->value();
    }
    else if (const auto* scientific_lit = asa<ScientificIntLiteralExpr>(&expr))
    {
        w << scientific_lit->value();
    }
    else if (const auto* hexadecimal_lit = asa<HexadecimalIntLiteralExpr>(&expr))
    {
        w << hexadecimal_lit->value();
    }
    else if (const auto* sym_access = asa<SymAccessExpr>(&expr))
    {
        generate_sym_access_expr(w, *sym_access, context);
    }
    else if (const auto* ternary = asa<TernaryExpr>(&expr))
    {
        generate_ternary_expr(w, *ternary, context);
    }
    else if (const auto* unary_op = asa<UnaryOpExpr>(&expr))
    {
        w << '-';
        generate_expr(w, unary_op->expr(), context);
    }
    else if (const auto* function_call = asa<FunctionCallExpr>(&expr))
    {
        generate_function_call_expr(w, *function_call, context);
    }
    else if (const auto* struct_ctor_call = asa<StructCtorCall>(&expr))
    {
        generate_struct_ctor_call(w, *struct_ctor_call, context);
    }
    else if (const auto* bin_op = asa<BinOpExpr>(&expr))
    {
        generate_bin_op_expr(w, *bin_op, context);
    }
    else if (const auto* sub = asa<SubscriptExpr>(&expr))
    {
        generate_subscript_expr(w, *sub, context);
    }
    else
    {
        w << "<< NotImplemented() >> ";
    }
}

void ShaderGenerator::prepare_expr(Writer& w, const Expr& expr, const SemaContext& context)
{
    if (const auto* bin_op = asa<BinOpExpr>(&expr))
    {
        prepare_expr(w, bin_op->lhs(), context);
        prepare_expr(w, bin_op->rhs(), context);
    }
    else if (const auto* unary_op = asa<UnaryOpExpr>(&expr))
    {
        prepare_expr(w, unary_op->expr(), context);
    }
    else if (const auto* ternary_op = asa<TernaryExpr>(&expr))
    {
        prepare_expr(w, ternary_op->condition_expr(), context);
        prepare_expr(w, ternary_op->true_expr(), context);
        prepare_expr(w, ternary_op->false_expr(), context);
    }
    else if (const auto* subscript = asa<SubscriptExpr>(&expr))
    {
        prepare_expr(w, subscript->expr(), context);
        prepare_expr(w, subscript->index_expr(), context);
    }
    else if (const auto* struct_ctor_call = asa<StructCtorCall>(&expr))
    {
        prepare_expr(w, struct_ctor_call->callee(), context);

        for (const auto& arg : struct_ctor_call->args())
        {
            prepare_expr(w, *arg, context);
        }
    }
    else if (const auto* func_call = asa<FunctionCallExpr>(&expr))
    {
        prepare_expr(w, func_call->callee(), context);

        for (const auto& arg : func_call->args())
        {
            prepare_expr(w, *arg, context);
        }
    }
    else if (const auto* struct_ctor_arg = asa<StructCtorArg>(&expr))
    {
        prepare_expr(w, struct_ctor_arg->expr(), context);
    }
}

void ShaderGenerator::generate_stmt(Writer& w, const Stmt& stmt, const SemaContext& context)
{
    if (const auto* var_stmt = asa<VarStmt>(&stmt))
    {
        generate_var_stmt(w, *var_stmt, context);
    }
    else if (const auto* if_stmt = asa<IfStmt>(&stmt))
    {
        generate_if_stmt(w, *if_stmt, context);
    }
    else if (const auto* return_stmt = asa<ReturnStmt>(&stmt))
    {
        generate_return_stmt(w, *return_stmt, context);
    }
    else if (const auto* for_stmt = asa<ForStmt>(&stmt))
    {
        generate_for_stmt(w, *for_stmt, context);
    }
    else if (const auto* compound_stmt = asa<CompoundStmt>(&stmt))
    {
        generate_compound_stmt(w, *compound_stmt, context);
    }
    else if (const auto* assignment_stmt = asa<AssignmentStmt>(&stmt))
    {
        generate_assignment_stmt(w, *assignment_stmt, context);
    }
    else
    {
        w << "<< not_implemented_stmt() >> ";
    }
}

void ShaderGenerator::generate_struct_decl(Writer&            w,
                                           const StructDecl&  strct,
                                           const SemaContext& context)
{
    CERLIB_UNUSED(context);

    w << "struct " << strct.name() << " ";
    w.open_brace();

    for (const auto& field : strct.get_fields())
    {
        w << translate_type(field->type(), TypeNameContext::StructField) << " ";
        w << field->name() << ';' << WNewline;
    }

    w.close_brace(true);
}

void ShaderGenerator::generate_if_stmt(Writer& w, const IfStmt& if_stmt, const SemaContext& context)
{
    {
        const auto* stmt = &if_stmt;

        while (stmt != nullptr)
        {
            if (stmt->condition_expr() != nullptr)
            {
                prepare_expr(w, *stmt->condition_expr(), context);
            }

            stmt = stmt->next();
        }
    }

    const auto* stmt = &if_stmt;

    while (stmt != nullptr)
    {
        if (stmt != &if_stmt)
        {
            w << " else ";
        }

        if (stmt->condition_expr() != nullptr)
        {
            w << "if (";
            generate_expr(w, *stmt->condition_expr(), context);
            w << ") ";
        }

        w.open_brace();
        generate_code_block(w, stmt->body(), context);
        w.close_brace();

        stmt = stmt->next();
    }
}

void ShaderGenerator::generate_for_stmt(Writer&            w,
                                        const ForStmt&     for_stmt,
                                        const SemaContext& context)
{
    const auto  var_name = for_stmt.loop_variable().name();
    const auto& range    = for_stmt.range();
    const auto& type     = range.type();

    prepare_expr(w, range.start(), context);
    prepare_expr(w, range.end(), context);

    w << "for ( " << translate_type(type, TypeNameContext::Normal) << ' ' << var_name << " = ";
    generate_expr(w, range.start(), context);
    w << "; " << var_name << " < ";
    generate_expr(w, range.end(), context);
    w << "; ++" << var_name << ") ";

    w.open_brace();
    generate_code_block(w, for_stmt.body(), context);
    w.close_brace();
}

void ShaderGenerator::generate_decl(Writer& w, const Decl& decl, const SemaContext& context)
{
    if (const auto* function = asa<FunctionDecl>(&decl))
    {
        generate_function_decl(w, *function, context);
    }
    else if (const auto* strct = asa<StructDecl>(&decl))
    {
        generate_struct_decl(w, *strct, context);
    }
    else if (const auto* var = asa<VarDecl>(&decl))
    {
        generate_global_var_decl(w, *var, context);
    }
    else
    {
        w << "<< generate_decl(" << decl.name() << ") >>";
    }
}

void ShaderGenerator::generate_var_stmt(Writer&            w,
                                        const VarStmt&     var_stmt,
                                        const SemaContext& context)
{
    CERLIB_UNUSED(var_stmt);
    CERLIB_UNUSED(context);

    w << "<< generate_var_stmt >>";
}

void ShaderGenerator::generate_bin_op_expr(Writer&            w,
                                           const BinOpExpr&   bin_op,
                                           const SemaContext& context)
{
    const auto* lhs_expr = &bin_op.lhs();
    const auto* rhs_expr = &bin_op.rhs();

    if (m_is_swapping_matrix_vector_multiplications)
    {
        const auto& lhs_type = lhs_expr->type();
        const auto& rhs_type = rhs_expr->type();

        if ((lhs_type.is_matrix_type() && rhs_type.is_matrix_type()) ||
            (lhs_type.is_matrix_type() && rhs_type.is_vector_type()) ||
            (lhs_type.is_vector_type() && rhs_type.is_matrix_type()))
        {
            std::swap(lhs_expr, rhs_expr);
        }
    }

    generate_expr(w, *lhs_expr, context);

    const auto need_space_between_operands = bin_op.bin_op_kind() != BinOpKind::MemberAccess;

    if (need_space_between_operands)
    {
        w << ' ';
    }

    switch (bin_op.bin_op_kind())
    {
        case BinOpKind::Add: w << "+"; break;
        case BinOpKind::Subtract: w << "-"; break;
        case BinOpKind::Multiply: w << "*"; break;
        case BinOpKind::Divide: w << "/"; break;
        case BinOpKind::LogicalAnd: w << "&&"; break;
        case BinOpKind::LogicalOr: w << "||"; break;
        case BinOpKind::LessThan: w << "<"; break;
        case BinOpKind::LessThanOrEqual: w << "<="; break;
        case BinOpKind::GreaterThan: w << ">"; break;
        case BinOpKind::GreaterThanOrEqual: w << ">="; break;
        case BinOpKind::MemberAccess: w << "."; break;
        case BinOpKind::BitwiseXor: w << "^"; break;
        case BinOpKind::BitwiseAnd: w << "&"; break;
        case BinOpKind::Equal: w << "=="; break;
        case BinOpKind::NotEqual: w << "!="; break;
        case BinOpKind::RightShift: w << ">>"; break;
        case BinOpKind::BitwiseOr: w << "|"; break;
        case BinOpKind::LeftShift: w << "<<"; break;
        default: throw Error(bin_op.location(), "Invalid binary operation kind");
    }

    if (need_space_between_operands)
    {
        w << ' ';
    }

    generate_expr(w, *rhs_expr, context);
}

void ShaderGenerator::generate_struct_ctor_call(Writer&               w,
                                                const StructCtorCall& struct_ctor_call,
                                                const SemaContext&    context)
{
    CERLIB_UNUSED(context);

    const auto it = m_temporary_vars.find(&struct_ctor_call);
    assert(it != m_temporary_vars.cend() && "Struct ctor call did not create a temporary variable");

    w << it->second;
}

void ShaderGenerator::generate_subscript_expr(Writer&              w,
                                              const SubscriptExpr& subscript_expr,
                                              const SemaContext&   context)
{
    generate_expr(w, subscript_expr.expr(), context);
    w << '[';
    generate_expr(w, subscript_expr.index_expr(), context);
    w << ']';
}

void ShaderGenerator::generate_compound_stmt(Writer&             w,
                                             const CompoundStmt& stmt,
                                             const SemaContext&  context)
{
    prepare_expr(w, stmt.lhs(), context);
    prepare_expr(w, stmt.rhs(), context);

    generate_expr(w, stmt.lhs(), context);

    switch (stmt.kind())
    {
        case CompoundStmtKind::Add: w << " += "; break;
        case CompoundStmtKind::Sub: w << " -= "; break;
        case CompoundStmtKind::Mul: w << " *= "; break;
        case CompoundStmtKind::Div: w << " /= "; break;
    }

    generate_expr(w, stmt.rhs(), context);
    w << ';';
}

void ShaderGenerator::generate_assignment_stmt(Writer&               w,
                                               const AssignmentStmt& stmt,
                                               const SemaContext&    context)
{
    prepare_expr(w, stmt.lhs(), context);
    prepare_expr(w, stmt.rhs(), context);

    generate_expr(w, stmt.lhs(), context);
    w << " = ";
    generate_expr(w, stmt.rhs(), context);
    w << ';';
}

void ShaderGenerator::generate_sym_access_expr(Writer&              w,
                                               const SymAccessExpr& expr,
                                               const SemaContext&   context)
{
    CERLIB_UNUSED(context);

    const auto* symbol = expr.symbol();
    const auto  name   = expr.name();

    if (isa<FunctionParamDecl>(symbol))
    {
        w << name;
    }
    else if (m_current_sym_access_override)
    {
        w << *m_current_sym_access_override;
    }
    else
    {
        for (const auto& [built_in_type, built_in_type_name] : m_built_in_type_dictionary)
        {
            if (built_in_type->type_name() == name)
            {
                w << built_in_type_name;
                return;
            }
        }

        w << name;
    }
}

void ShaderGenerator::generate_ternary_expr(Writer&            w,
                                            const TernaryExpr& expr,
                                            const SemaContext& context)
{
    w << "(";
    generate_expr(w, expr.condition_expr(), context);
    w << " ? ";
    generate_expr(w, expr.true_expr(), context);
    w << " : ";
    generate_expr(w, expr.false_expr(), context);
    w << ")";
}

auto ShaderGenerator::translate_type(const Type& type, TypeNameContext context) const -> std::string
{
    if (context == TypeNameContext::FunctionParam)
    {
        if (const auto* strct = asa<StructDecl>(&type))
        {
            return std::string{strct->name()};
        }
    }

    if (const auto it = m_built_in_type_dictionary.find(&type);
        it != m_built_in_type_dictionary.cend())
    {
        return it->second;
    }

    return std::string{type.type_name()};
}

auto ShaderGenerator::translate_array_type(const ArrayType& type,
                                           std::string_view variable_name) const -> std::string
{
    return cer_fmt::format("{} {}[{}]",
                           translate_type(type.element_type(), TypeNameContext::Normal),
                           variable_name,
                           type.size());
}

auto ShaderGenerator::gather_ast_decls_to_generate(const AST&         ast,
                                                   std::string_view   entry_point,
                                                   const SemaContext& context) const
    -> gch::small_vector<const Decl*, 8>
{
    const auto& decls = ast.decls();

    const auto main_function = [&] {
        const auto it = std::ranges::find_if(decls, [entry_point](const auto& decl) {
            const auto* func = asa<FunctionDecl>(decl.get());
            return func != nullptr && func->is_shader() && func->name() == entry_point;
        });

        return it != ast.decls().cend() ? asa<FunctionDecl>(it->get()) : nullptr;
    }();

    if (main_function == nullptr)
    {
        throw Error{SourceLocation{ast.filename(), 0, 0, 0},
                    "no suitable entry point named '{}' found",
                    entry_point};
    }

    // See what the main function depends on.
    auto accessed_symbols = gch::small_vector<const Decl*, 16>{};

    for (const auto& decl : ast.decls())
    {
        if (const auto* func = asa<FunctionDecl>(decl.get());
            func != nullptr && func->is_struct_ctor())
        {
            continue;
        }

        if (main_function->body()->accesses_symbol(*decl, true))
        {
            accessed_symbols.push_back(decl.get());
        }
    }

    // Remove non-top-level symbols
    {
        const auto it = std::ranges::remove_if(accessed_symbols, [&](const auto* symbol) {
                            return !ast.is_top_level_symbol(context, *symbol);
                        }).begin();

        accessed_symbols.erase(it, accessed_symbols.end());
    }

    remove_duplicates_but_keep_order(accessed_symbols);

    auto decls_to_generate = gch::small_vector<const Decl*, 8>{};
    decls_to_generate.reserve(accessed_symbols.size() + 1);

    for (const auto* symbol : accessed_symbols)
    {
        decls_to_generate.push_back(symbol);
    }

    // The entry point / shader function is always last.
    decls_to_generate.push_back(main_function);

    return decls_to_generate;
}
} // namespace cer::shadercompiler
