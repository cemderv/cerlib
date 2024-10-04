// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/ASTOptimizer.hpp"

#include "shadercompiler/AST.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Stmt.hpp"

namespace cer::shadercompiler
{
void ASTOptimizer::optimize(AST& ast)
{
    while (true)
    {
        bool keep_going = false;

        keep_going |= remove_unused_functions(ast);
        keep_going |= remove_unused_structs(ast);

        for (const auto& child : ast.decls())
        {
            if (const auto func = asa<FunctionDecl>(child.get()))
            {
                if (func->is_shader())
                {
                    keep_going |= optimize_block(func->body());
                }
            }
        }

        if (!keep_going)
        {
            break;
        }
    }

    // Remove unused parameters
    auto& decls = ast.decls();

    const auto it =
        std::ranges::remove_if(decls, [&ast](const std::unique_ptr<Decl>& decl) {
            return isa<ShaderParamDecl>(decl.get()) && !ast.is_symbol_accessed_anywhere(*decl);
        }).begin();

    decls.erase(it, decls.end());
}

auto ASTOptimizer::remove_unused_functions(AST& ast) const -> bool
{
    auto& decls = ast.decls();

    const auto it = std::ranges::remove_if(decls, [&ast](const std::unique_ptr<Decl>& decl) {
                        const auto* func = asa<FunctionDecl>(decl.get());
                        if (func == nullptr)
                        {
                            return false;
                        }

                        if (func->body() == nullptr)
                        {
                            // A built-in function; don't optimize it away.
                            return false;
                        }

                        if (func->is_shader())
                        {
                            // Don't optimize away shaders.
                            return false;
                        }

                        return !ast.is_symbol_accessed_anywhere(*func);
                    }).begin();

    if (it != decls.end())
    {
        decls.erase(it, decls.end());
        return true;
    }

    return false;
}

auto ASTOptimizer::remove_unused_structs(AST& ast) const -> bool
{
    auto& decls = ast.decls();

    const auto it = std::ranges::remove_if(decls, [&ast](const std::unique_ptr<Decl>& decl) {
                        const auto* strct = asa<StructDecl>(decl.get());
                        if (strct == nullptr)
                        {
                            return false;
                        }

                        if (strct->is_built_in())
                        {
                            return false;
                        }

                        return !ast.is_symbol_accessed_anywhere(*strct);
                    }).begin();

    if (it != decls.end())
    {
        decls.erase(it, decls.end());
        return true;
    }

    return false;
}

auto ASTOptimizer::optimize_block(CodeBlock* block) -> bool
{
    const auto it = std::ranges::find_if(m_code_block_name_gens, [block](const auto& pair) {
        return pair.first == block;
    });

    if (it == m_code_block_name_gens.cend())
    {
        m_code_block_name_gens.emplace(block, TempVarNameGen(block));
    }

    return remove_unused_variables(block);
}

auto ASTOptimizer::remove_unused_variables(CodeBlock* block) -> bool
{
    auto var_stmts = inplace_vector<VarStmt*, 4>{};

    for (const auto& stmt : block->stmts())
    {
        if (auto* var_stmt = asa<VarStmt>(stmt.get()))
        {
            var_stmts.push_back(var_stmt);
        }
    }

    inplace_vector<gsl::not_null<VarStmt*>, 4> var_stmts_to_remove;

    for (VarStmt* var_stmt : var_stmts)
    {
        if (!block->accesses_symbol(var_stmt->variable(), false))
        {
            var_stmts_to_remove.emplace_back(var_stmt);
        }
    }

    const bool has_removed_any = !var_stmts_to_remove.empty();

    for (const gsl::not_null<VarStmt*>& lbe : var_stmts_to_remove)
    {
        block->remove_stmt(*lbe);
    }

    return has_removed_any;
}
} // namespace cer::shadercompiler
