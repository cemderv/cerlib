// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/TempVarNameGen.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <gsl/pointers>
#include <optional>
#include <string>
#include <unordered_map>

namespace cer::shadercompiler
{
class SemaContext;
class AST;
class Writer;
class Stmt;
class Decl;
class FunctionDecl;
class VarDecl;
class CodeBlock;
class Type;
class VarStmt;
class ReturnStmt;
class Expr;
class IfStmt;
class BinOpExpr;
class FunctionCallExpr;
class StructCtorCall;
class StructDecl;
class ForStmt;
class CompoundStmt;
class SubscriptExpr;
class AssignmentStmt;
class ArrayType;
class SymAccessExpr;
class ShaderParamDecl;
class AccessedParams;
class TernaryExpr;

class ShaderGenerationResult
{
  public:
    using ParameterList = SmallVector<const ShaderParamDecl*, 8>;

    explicit ShaderGenerationResult(std::string                        glsl_code,
                                    gsl::not_null<const FunctionDecl*> entry_point,
                                    ParameterList                      parameters);

    std::string                        glsl_code;
    gsl::not_null<const FunctionDecl*> entry_point;
    ParameterList                      parameters;
};

class ShaderGenerator
{
  protected:
    ShaderGenerator();

  public:
    NON_COPYABLE_NON_MOVABLE(ShaderGenerator);

    virtual ~ShaderGenerator() noexcept;

    ShaderGenerationResult generate(const SemaContext& context,
                                    const AST&         ast,
                                    std::string_view   entry_point,
                                    bool               minify);

  protected:
    enum class TypeNameContext
    {
        Normal,
        FunctionParam,
        FunctionParamNoConstRef,
        FunctionReturnType,
        StructField,
    };

    AccessedParams params_accessed_by_function(const FunctionDecl& function) const;

    virtual std::string do_generation(const SemaContext&                 context,
                                      const FunctionDecl&                entry_point,
                                      const SmallVector<const Decl*, 8>& decls_to_generate) = 0;

    virtual void generate_stmt(Writer& w, const Stmt& stmt, const SemaContext& context);

    virtual void generate_decl(Writer& w, const Decl& decl, const SemaContext& context);

    virtual void generate_global_var_decl(Writer&            w,
                                          const VarDecl&     decl,
                                          const SemaContext& context) = 0;

    virtual void generate_var_stmt(Writer& w, const VarStmt& var_stmt, const SemaContext& context);

    virtual void generate_function_decl(Writer&             w,
                                        const FunctionDecl& function,
                                        const SemaContext&  context) = 0;

    virtual void generate_code_block(Writer&            w,
                                     const CodeBlock&   code_block,
                                     const SemaContext& context);

    virtual void generate_expr(Writer& w, const Expr& expr, const SemaContext& context);

    virtual void prepare_expr(Writer& w, const Expr& expr, const SemaContext& context);

    virtual void generate_struct_decl(Writer&            w,
                                      const StructDecl&  strct,
                                      const SemaContext& context);

    virtual void generate_if_stmt(Writer& w, const IfStmt& if_stmt, const SemaContext& context);

    virtual void generate_for_stmt(Writer& w, const ForStmt& for_stmt, const SemaContext& context);

    virtual void generate_return_stmt(Writer&            w,
                                      const ReturnStmt&  stmt,
                                      const SemaContext& context) = 0;

    virtual void generate_bin_op_expr(Writer&            w,
                                      const BinOpExpr&   bin_op,
                                      const SemaContext& context);

    virtual void generate_function_call_expr(Writer&                 w,
                                             const FunctionCallExpr& function_call,
                                             const SemaContext&      context) = 0;

    virtual void generate_struct_ctor_call(Writer&               w,
                                           const StructCtorCall& struct_ctor_call,
                                           const SemaContext&    context);

    virtual void generate_subscript_expr(Writer&              w,
                                         const SubscriptExpr& subscript_expr,
                                         const SemaContext&   context);

    virtual void generate_compound_stmt(Writer&             w,
                                        const CompoundStmt& stmt,
                                        const SemaContext&  context);

    virtual void generate_assignment_stmt(Writer&               w,
                                          const AssignmentStmt& stmt,
                                          const SemaContext&    context);

    virtual void generate_sym_access_expr(Writer&              w,
                                          const SymAccessExpr& expr,
                                          const SemaContext&   context);

    virtual void generate_ternary_expr(Writer&            w,
                                       const TernaryExpr& expr,
                                       const SemaContext& context);

    virtual std::string translate_type(const Type& type, TypeNameContext context) const;

    virtual std::string translate_array_type(const ArrayType& type,
                                             std::string_view variable_name) const;

    SmallVector<const Decl*, 8> gather_ast_decls_to_generate(const AST&         ast,
                                                             std::string_view   entry_point,
                                                             const SemaContext& context) const;

    bool     m_is_swapping_matrix_vector_multiplications{};
    uint32_t m_uniform_buffer_alignment{};
    std::unordered_map<gsl::not_null<const Type*>, std::string> m_built_in_type_dictionary;

    const AST*                                   m_ast{};
    const FunctionDecl*                          m_currently_generated_shader_function{};
    SmallVector<const FunctionDecl*, 8>          m_call_stack;
    SmallVector<TempVarNameGen, 4>               m_temp_var_name_gen_stack;
    std::unordered_map<const Expr*, std::string> m_temporary_vars;
    std::optional<std::string>                   m_current_sym_access_override;
    bool                                         m_needs_float_literal_suffix{};
};
} // namespace cer::shadercompiler
