// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/GLSLShaderGenerator.hpp"

#include "cerlib/Util.hpp"
#include "contentmanagement/FileSystem.hpp"
#include "shadercompiler/AST.hpp"
#include "shadercompiler/BuiltInSymbols.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Naming.hpp"
#include "shadercompiler/SemaContext.hpp"
#include "shadercompiler/Stmt.hpp"
#include "shadercompiler/Type.hpp"
#include "shadercompiler/Writer.hpp"
#include <cassert>

using namespace std::string_literals;

namespace cer::shadercompiler
{
static constexpr auto fragment_shader_output_variable_name = "OutColor";

GLSLShaderGenerator::GLSLShaderGenerator(bool is_gles)
    : m_is_gles(is_gles)
{
    m_is_swapping_matrix_vector_multiplications = true;

    m_v2f_prefix = fmt::format("{}v2f_", naming::forbidden_identifier_prefix);

    m_built_in_type_dictionary = {
        {IntType::instance(), "int"s},
        {BoolType::instance(), "bool"s},
        {FloatType::instance(), "float"s},
        {Vector2Type::instance(), "vec2"s},
        {Vector3Type::instance(), "vec3"s},
        {Vector4Type::instance(), "vec4"s},
        {MatrixType::instance(), "mat4"s},
    };

    m_needs_float_literal_suffix = false;
}

auto GLSLShaderGenerator::do_generation(const SemaContext&          context,
                                        const FunctionDecl&         entry_point,
                                        const List<const Decl*, 8>& decls_to_generate) -> String
{
    const auto shader_name = filesystem::filename_without_extension(m_ast->filename());

    auto w = Writer{};

    if (m_is_gles)
    {
        w << "#version 300 es" << WNewline;
    }
    else
    {
        w << "#version 140" << WNewline;
    }

    w << "precision highp float;" << WNewline;
    w << "precision highp sampler2D;" << WNewline;

    w << WNewline;

    // Emit uniforms that are always available/implicit, depending on shader domain.
    w << "uniform sampler2D SpriteImage;" << WNewline;
    w << WNewline;

    // Emit the uniform buffer for the shader parameters.
    if (const auto accessed_params = params_accessed_by_function(entry_point))
    {
        emit_uniform_buffer_for_user_params(w, entry_point, accessed_params);
        w << WNewline;
    }

    for (const auto* decl : decls_to_generate)
    {
        if (isa<ShaderParamDecl>(decl))
        {
            // Skip params, we generate the uniform buffer manually.
            continue;
        }

        const auto writer_size = w.buffer_length();

        generate_decl(w, *decl, context);

        if (w.buffer_length() > writer_size)
        {
            // Something was written
            w << WNewline << WNewline;
        }
    }

    w << WNewline;

    return w.take_buffer();
}

void GLSLShaderGenerator::generate_var_stmt(Writer&            w,
                                            const VarStmt&     var_stmt,
                                            const SemaContext& context)
{
    const auto& var = var_stmt.variable();

    if (var.is_system_value())
    {
        return;
    }

    prepare_expr(w, var_stmt.variable().expr(), context);

    w << translate_type(var.type(), TypeNameContext::Normal) << ' ' << var_stmt.name() << " = ";
    generate_expr(w, var_stmt.variable().expr(), context);
    w << ';';
}

void GLSLShaderGenerator::generate_function_decl(Writer&             w,
                                                 const FunctionDecl& function,
                                                 const SemaContext&  context)
{
    if (function.body() == nullptr)
    {
        return;
    }

    m_currently_generated_function = &function;

    m_call_stack.push_back(&function);

    if (function.is_shader())
    {
        // Keep this in sync with SpriteBatchVS.vert output!
        w << "in vec4 " << m_v2f_prefix << "Color;" << WNewline;
        w << "in vec2 " << m_v2f_prefix << "UV;" << WNewline;

        w << WNewline;

        // Fragment shader outputs
        w << "out vec4 " << naming::forbidden_identifier_prefix
          << fragment_shader_output_variable_name << ";" << WNewline;

        w << WNewline;

        // Shader body
        w << "void main() ";
        w.open_brace();

        generate_code_block(w, *function.body(), context);

        w.close_brace();
    }
    else
    {
        w << translate_type(function.type(), TypeNameContext::FunctionReturnType) << ' '
          << function.name() << '(';

        for (const auto& param : function.parameters())
        {
            w << translate_type(param->type(), TypeNameContext::FunctionParam) << ' '
              << param->name();

            if (param != function.parameters().back())
            {
                w << ", ";
            }
        }

        w << ") ";

        w.open_brace();
        generate_code_block(w, *function.body(), context);
        w.close_brace();
    }

    m_call_stack.pop_back();
}

void GLSLShaderGenerator::prepare_expr(Writer& w, const Expr& expr, const SemaContext& context)
{
    if (const auto* struct_ctor_call = asa<StructCtorCall>(&expr))
    {
        auto tmp_name = m_temp_var_name_gen_stack.back().next();

        prepare_expr(w, struct_ctor_call->callee(), context);

        for (const auto& arg : struct_ctor_call->args())
        {
            prepare_expr(w, *arg, context);
        }

        generate_expr(w, struct_ctor_call->callee(), context);

        w << ' ' << tmp_name << ';' << WNewline;

        for (const auto& arg : struct_ctor_call->args())
        {
            w << tmp_name << '.' << arg->name() << " = ";
            generate_expr(w, arg->expr(), context);
            w << ';' << WNewline;
        }

        m_temporary_vars.emplace(&expr, std::move(tmp_name));
    }
}

void GLSLShaderGenerator::generate_return_stmt(Writer&            w,
                                               const ReturnStmt&  stmt,
                                               const SemaContext& context)
{
    assert(!m_call_stack.empty());

    if (const auto& current_function = *m_call_stack.back(); current_function.is_shader())
    {
        prepare_expr(w, stmt.expr(), context);

        w << naming::forbidden_identifier_prefix << fragment_shader_output_variable_name << " = ";
        generate_expr(w, stmt.expr(), context);
        w << ';';
    }
    else
    {
        prepare_expr(w, stmt.expr(), context);
        w << "return ";
        generate_expr(w, stmt.expr(), context);
        w << ';';
    }
}

void GLSLShaderGenerator::generate_global_var_decl(Writer&            w,
                                                   const VarDecl&     decl,
                                                   const SemaContext& context)
{
    prepare_expr(w, decl.expr(), context);
    w << "const " << translate_type(decl.type(), TypeNameContext::Normal) << ' ' << decl.name();
    w << " = ";
    generate_expr(w, decl.expr(), context);
    w << ';';
}

void GLSLShaderGenerator::generate_function_call_expr(Writer&                 w,
                                                      const FunctionCallExpr& function_call,
                                                      const SemaContext&      context)
{
    const auto& callee = function_call.callee();
    const auto  args   = function_call.args();

    prepare_expr(w, callee, context);

    for (const auto& arg : args)
    {
        prepare_expr(w, *arg, context);
    }

    generate_expr(w, callee, context);

    w << '(';

    for (const auto& arg : args)
    {
        generate_expr(w, *arg, context);

        if (arg != args.back())
        {
            w << ", ";
        }
    }

    w << ')';
}

void GLSLShaderGenerator::generate_sym_access_expr(Writer&              w,
                                                   const SymAccessExpr& expr,
                                                   const SemaContext&   context)
{
    const auto& built_ins = context.built_in_symbols();
    const auto& symbol    = *expr.symbol();
    const auto  name      = expr.name();

    if (const auto param = asa<ShaderParamDecl>(&symbol);
        param != nullptr && param->type().can_be_in_constant_buffer())
    {
        w << name;
    }
    else if (&symbol == built_ins.sprite_image.get())
    {
        w << "SpriteImage";
    }
    else if (&symbol == built_ins.sprite_color.get())
    {
        w << m_v2f_prefix << "Color";
    }
    else if (&symbol == built_ins.sprite_uv.get())
    {
        w << m_v2f_prefix << "UV";
    }
    else if (built_ins.is_lerp_function(symbol))
    {
        w << "mix";
    }
    else if (built_ins.is_image_sampling_function(symbol))
    {
        w << "texture";
    }
    else if (built_ins.is_atan2_function(symbol))
    {
        // atan2 is not available in GLSL, but it's just atan with two arguments.
        w << "atan";
    }
    else if (built_ins.is_some_intrinsic_function(symbol))
    {
        // Our intrinsic functions are PascalCase, whereas in GLSL they're camelBack.
        w << util::to_lower_case(name);
    }
    else if (built_ins.is_vector_field_access(symbol))
    {
        w << name;
    }
    else
    {
        ShaderGenerator::generate_sym_access_expr(w, expr, context);
    }
}

void GLSLShaderGenerator::emit_uniform_buffer_for_user_params(
    Writer& w, [[maybe_unused]] const FunctionDecl& shader, const AccessedParams& params) const
{
    if (!params.scalars.empty())
    {
#if 0 // We don't support UBOs for now
      w << "layout(std140) uniform " << UboName << ' ';
      w.OpenBrace();

      for (const auto& param : params.Constants)
      {
        const auto name = GetUniformFieldName(shader, param->GetName());
        const auto& type = param->GetType();

        if (const auto& arrayType = asa<shaderc::ArrayType>(&type))
        {
          w << TranslateArrayType(*arrayType, name);
        }
        else
        {
          w << TranslateType(type, TypeNameContext::StructField) << ' ' << name;
        }

        w << ';' << WNewline;
      }

      w.CloseBrace(true);
      w << WNewline;
#else
        for (const auto& param_ref : params.scalars)
        {
            const auto& param = param_ref.get();
            const auto  name  = param.name();
            const auto& type  = param.type();

            w << "uniform ";

            if (const auto* array_type = asa<ArrayType>(&type))
            {
                w << translate_array_type(*array_type, name);
            }
            else
            {
                w << translate_type(type, TypeNameContext::StructField) << ' ' << name;
            }

            w << ';' << WNewline;
        }
#endif
    }

    // Image parameters
    {
        // int i = 0;
        for (const auto param_ref : params.resources)
        {
            const auto& param = param_ref.get();

            // Not always supported. Have to check support first before using
            // layout(binding=...). w << "layout(binding = " << i << ") ";

            w << "uniform ";

            if (&param.type() == &ImageType::instance())
            {
                w << "sampler2D";
            }
            else
            {
                throw std::runtime_error{"Image type not implemented."};
            }

            w << " " << param.name() << ";" << WNewline;
            // ++i;
        }
    }
}
} // namespace cer::shadercompiler
