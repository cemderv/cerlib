// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/ShaderGenerator.hpp"

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
#include <format>
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

AccessedParams ShaderGenerator::params_accessed_by_function(const FunctionDecl& function) const
{
    AccessedParams params;
    params.scalars.reserve(8);
    params.resources.reserve(4);

    const CodeBlock* body = function.body();

    for (const std::unique_ptr<Decl>& decl : m_ast->decls())
    {
        const ShaderParamDecl* param = asa<ShaderParamDecl>(decl.get());

        if (param == nullptr)
        {
            continue;
        }

        if (const Type& type = param->type(); type.can_be_in_constant_buffer())
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

ShaderGenerationResult ShaderGenerator::generate(const SemaContext& context,
                                                 const AST&         ast,
                                                 std::string_view   entry_point_name,
                                                 bool               minify)
{
    CERLIB_UNUSED(minify);

    assert(m_ast == nullptr);
    assert(ast.is_verified());

    m_ast = std::addressof(ast);

    const auto _ = gsl::finally([&] { m_ast = nullptr; });

    const std::string shader_name = filesystem::filename_without_extension(ast.filename());

    const SmallVector<const Decl*, 8> children_to_generate =
        gather_ast_decls_to_generate(ast, entry_point_name, context);

    if (children_to_generate.empty())
    {
        throw Error(SourceLocation(), "failed to gather children to generate");
    }

    const FunctionDecl* entry_point = asa<FunctionDecl>(children_to_generate.back());
    assert(entry_point);
    assert(entry_point->is_shader());

    m_currently_generated_shader_function = entry_point;

    std::string code = do_generation(context, *entry_point, children_to_generate);

    m_currently_generated_shader_function = nullptr;

    while (!code.empty() && code.back() == '\n')
    {
        code.pop_back();
    }

    while (!code.empty() && code.front() == '\n')
    {
        code.erase(0, 1);
    }

    if (!code.empty())
    {
        code += '\n';
    }

    const AccessedParams accessed_params = params_accessed_by_function(*entry_point);

    SmallVector<const ShaderParamDecl*, 8> parameters;
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

    for (const std::unique_ptr<Stmt>& stmt : code_block.stmts())
    {
        generate_stmt(w, *stmt, context);
        w << WNewline;
    }

    m_temp_var_name_gen_stack.pop_back();
}

void ShaderGenerator::generate_expr(Writer& w, const Expr& expr, const SemaContext& context)
{
    if (const ParenExpr* e = asa<ParenExpr>(&expr))
    {
        w << '(';
        generate_expr(w, e->expr(), context);
        w << ')';
    }
    else if (const IntLiteralExpr* int_lit = asa<IntLiteralExpr>(&expr))
    {
        w << int_lit->value();
    }
    else if (const FloatLiteralExpr* float_lit = asa<FloatLiteralExpr>(&expr))
    {
        w << float_lit->string_value();
        if (m_needs_float_literal_suffix)
        {
            w << 'f';
        }
    }
    else if (const BoolLiteralExpr* bool_lit = asa<BoolLiteralExpr>(&expr))
    {
        w << bool_lit->value();
    }
    else if (const ScientificIntLiteralExpr* scientific_lit = asa<ScientificIntLiteralExpr>(&expr))
    {
        w << scientific_lit->value();
    }
    else if (const HexadecimalIntLiteralExpr* hexadecimal_lit =
                 asa<HexadecimalIntLiteralExpr>(&expr))
    {
        w << hexadecimal_lit->value();
    }
    else if (const SymAccessExpr* sym_access = asa<SymAccessExpr>(&expr))
    {
        generate_sym_access_expr(w, *sym_access, context);
    }
    else if (const TernaryExpr* ternary = asa<TernaryExpr>(&expr))
    {
        generate_ternary_expr(w, *ternary, context);
    }
    else if (const UnaryOpExpr* unary_op = asa<UnaryOpExpr>(&expr))
    {
        w << '-';
        generate_expr(w, unary_op->expr(), context);
    }
    else if (const FunctionCallExpr* function_call = asa<FunctionCallExpr>(&expr))
    {
        generate_function_call_expr(w, *function_call, context);
    }
    else if (const StructCtorCall* struct_ctor_call = asa<StructCtorCall>(&expr))
    {
        generate_struct_ctor_call(w, *struct_ctor_call, context);
    }
    else if (const BinOpExpr* bin_op = asa<BinOpExpr>(&expr))
    {
        generate_bin_op_expr(w, *bin_op, context);
    }
    else if (const SubscriptExpr* sub = asa<SubscriptExpr>(&expr))
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
    if (const BinOpExpr* bin_op = asa<BinOpExpr>(&expr))
    {
        prepare_expr(w, bin_op->lhs(), context);
        prepare_expr(w, bin_op->rhs(), context);
    }
    else if (const UnaryOpExpr* unary_op = asa<UnaryOpExpr>(&expr))
    {
        prepare_expr(w, unary_op->expr(), context);
    }
    else if (const TernaryExpr* ternary_op = asa<TernaryExpr>(&expr))
    {
        prepare_expr(w, ternary_op->condition_expr(), context);
        prepare_expr(w, ternary_op->true_expr(), context);
        prepare_expr(w, ternary_op->false_expr(), context);
    }
    else if (const SubscriptExpr* subscript = asa<SubscriptExpr>(&expr))
    {
        prepare_expr(w, subscript->expr(), context);
        prepare_expr(w, subscript->index_expr(), context);
    }
    else if (const StructCtorCall* struct_ctor_call = asa<StructCtorCall>(&expr))
    {
        prepare_expr(w, struct_ctor_call->callee(), context);

        for (const std::unique_ptr<StructCtorArg>& arg : struct_ctor_call->args())
        {
            prepare_expr(w, *arg, context);
        }
    }
    else if (const FunctionCallExpr* func_call = asa<FunctionCallExpr>(&expr))
    {
        prepare_expr(w, func_call->callee(), context);

        for (const std::unique_ptr<Expr>& arg : func_call->args())
        {
            prepare_expr(w, *arg, context);
        }
    }
    else if (const StructCtorArg* struct_ctor_arg = asa<StructCtorArg>(&expr))
    {
        prepare_expr(w, struct_ctor_arg->expr(), context);
    }
}

void ShaderGenerator::generate_stmt(Writer& w, const Stmt& stmt, const SemaContext& context)
{
    if (const VarStmt* var_stmt = asa<VarStmt>(&stmt))
    {
        generate_var_stmt(w, *var_stmt, context);
    }
    else if (const IfStmt* if_stmt = asa<IfStmt>(&stmt))
    {
        generate_if_stmt(w, *if_stmt, context);
    }
    else if (const ReturnStmt* return_stmt = asa<ReturnStmt>(&stmt))
    {
        generate_return_stmt(w, *return_stmt, context);
    }
    else if (const ForStmt* for_stmt = asa<ForStmt>(&stmt))
    {
        generate_for_stmt(w, *for_stmt, context);
    }
    else if (const CompoundStmt* compound_stmt = asa<CompoundStmt>(&stmt))
    {
        generate_compound_stmt(w, *compound_stmt, context);
    }
    else if (const AssignmentStmt* assignment_stmt = asa<AssignmentStmt>(&stmt))
    {
        generate_assignment_stmt(w, *assignment_stmt, context);
    }
    else
    {
        w << "<< NotImplementedStmt() >> ";
    }
}

void ShaderGenerator::generate_struct_decl(Writer&            w,
                                           const StructDecl&  strct,
                                           const SemaContext& context)
{
    CERLIB_UNUSED(context);

    w << "struct " << strct.name() << " ";
    w.open_brace();

    for (const std::unique_ptr<StructFieldDecl>& field : strct.get_fields())
    {
        w << translate_type(field->type(), TypeNameContext::StructField) << " ";
        w << field->name() << ';' << WNewline;
    }

    w.close_brace(true);
}

void ShaderGenerator::generate_if_stmt(Writer& w, const IfStmt& if_stmt, const SemaContext& context)
{
    {
        const IfStmt* stmt = &if_stmt;

        while (stmt != nullptr)
        {
            if (stmt->condition_expr() != nullptr)
            {
                prepare_expr(w, *stmt->condition_expr(), context);
            }

            stmt = stmt->next();
        }
    }

    const IfStmt* stmt = &if_stmt;

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
    const std::string_view var_name = for_stmt.loop_variable().name();
    const RangeExpr&       range    = for_stmt.range();
    const Type&            type     = range.type();

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
    if (const FunctionDecl* function = asa<FunctionDecl>(&decl))
    {
        generate_function_decl(w, *function, context);
    }
    else if (const StructDecl* strct = asa<StructDecl>(&decl))
    {
        generate_struct_decl(w, *strct, context);
    }
    else if (const VarDecl* var = asa<VarDecl>(&decl))
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
    const Expr* lhs_expr = &bin_op.lhs();
    const Expr* rhs_expr = &bin_op.rhs();

    if (m_is_swapping_matrix_vector_multiplications)
    {
        const Type& lhs_type = lhs_expr->type();
        const Type& rhs_type = rhs_expr->type();

        if ((lhs_type.is_matrix_type() && rhs_type.is_matrix_type()) ||
            (lhs_type.is_matrix_type() && rhs_type.is_vector_type()) ||
            (lhs_type.is_vector_type() && rhs_type.is_matrix_type()))
        {
            std::swap(lhs_expr, rhs_expr);
        }
    }

    generate_expr(w, *lhs_expr, context);

    const bool space_between_operands = bin_op.bin_op_kind() != BinOpKind::MemberAccess;

    if (space_between_operands)
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

    if (space_between_operands)
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

    const Decl*            symbol = expr.symbol();
    const std::string_view name   = expr.name();

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
        for (const auto& [builtInType, builtInTypeName] : m_built_in_type_dictionary)
        {
            if (builtInType->type_name() == name)
            {
                w << builtInTypeName;
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

template <typename T>
void remove_duplicates_but_keep_order(T& container)
{
    size_t i{};
    while (i != container.size())
    {
        size_t j = i;
        ++j;
        while (j != container.size())
        {
            if (container.at(j) == container.at(i))
            {
                container.erase(container.begin() + j);
            }
            else
            {
                ++j;
            }
        }
        ++i;
    }
}

std::string ShaderGenerator::translate_type(const Type& type, TypeNameContext context) const
{
    if (context == TypeNameContext::FunctionParam)
    {
        if (const StructDecl* strct = asa<StructDecl>(&type))
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

std::string ShaderGenerator::translate_array_type(const ArrayType& type,
                                                  std::string_view variable_name) const
{
    return std::format("{} {}[{}]",
                       translate_type(type.element_type(), TypeNameContext::Normal),
                       variable_name,
                       type.size());
}

SmallVector<const Decl*, 8> ShaderGenerator::gather_ast_decls_to_generate(
    const AST& ast, std::string_view entry_point, const SemaContext& context) const
{
    const SmallVector<std::unique_ptr<Decl>, 8>& decls = ast.decls();

    const auto main_function = [&] {
        const auto it = std::ranges::find_if(decls, [entry_point](const auto& decl) {
            const FunctionDecl* func = asa<FunctionDecl>(decl.get());
            return func != nullptr && func->is_shader() && func->name() == entry_point;
        });

        return it != ast.decls().cend() ? asa<FunctionDecl>(it->get()) : nullptr;
    }();

    if (main_function == nullptr)
    {
        throw Error{SourceLocation(ast.filename(), 0, 0, 0),
                    "no suitable entry point named '{}' found",
                    entry_point};
    }

    // See what the main function depends on.
    SmallVector<const Decl*, 16> accessed_symbols;

    for (const std::unique_ptr<Decl>& decl : ast.decls())
    {
        if (const FunctionDecl* func = asa<FunctionDecl>(decl.get());
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
        const auto it = std::ranges::remove_if(accessed_symbols, [&](const Decl* symbol) {
                            return !ast.is_top_level_symbol(context, *symbol);
                        }).begin();

        accessed_symbols.erase(it, accessed_symbols.end());
    }

    remove_duplicates_but_keep_order(accessed_symbols);

    SmallVector<const Decl*, 8> decls_to_generate;
    decls_to_generate.reserve(accessed_symbols.size() + 1);

    for (const Decl* symbol : accessed_symbols)
    {
        decls_to_generate.push_back(symbol);
    }

    // The entry point / shader function is always last.
    decls_to_generate.push_back(main_function);

    return decls_to_generate;
}
} // namespace cer::shadercompiler
