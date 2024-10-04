// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/ShaderGenerator.hpp"

namespace cer::shadercompiler
{
class AccessedParams;

class GLSLShaderGenerator final : public ShaderGenerator
{
  public:
    static constexpr std::string_view ubo_name{"cer_Params"};

    explicit GLSLShaderGenerator(bool is_gles);

    auto do_generation(const SemaContext&                    context,
                       const FunctionDecl&                   entry_point,
                       const inplace_vector<const Decl*, 8>& decls_to_generate)
        -> std::string override;

  private:
    void generate_var_stmt(Writer& w, const VarStmt& var_stmt, const SemaContext& context) override;

    void generate_function_decl(Writer&             w,
                                const FunctionDecl& function,
                                const SemaContext&  context) override;

    void prepare_expr(Writer& w, const Expr& expr, const SemaContext& context) override;

    void generate_return_stmt(Writer&            w,
                              const ReturnStmt&  stmt,
                              const SemaContext& context) override;

    void generate_global_var_decl(Writer&            w,
                                  const VarDecl&     decl,
                                  const SemaContext& context) override;

    void generate_function_call_expr(Writer&                 w,
                                     const FunctionCallExpr& function_call,
                                     const SemaContext&      context) override;

    void generate_sym_access_expr(Writer&              w,
                                  const SymAccessExpr& expr,
                                  const SemaContext&   context) override;

    void emit_uniform_buffer_for_user_params(Writer&               w,
                                             const FunctionDecl&   shader,
                                             const AccessedParams& params) const;

    bool                m_is_gles = false;
    std::string         m_v2f_prefix;
    const FunctionDecl* m_currently_generated_function = nullptr;
};
} // namespace cer::shadercompiler
