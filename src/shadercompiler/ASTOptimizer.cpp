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
        auto keepGoing = false;

        keepGoing |= remove_unused_functions(ast);
        keepGoing |= remove_unused_structs(ast);

        for (const auto& child : ast.decls())
            if (const auto func = asa<FunctionDecl>(child.get()))
                if (func->is_shader())
                    keepGoing |= optimize_block(func->body());

        if (!keepGoing)
            break;
    }

    // Remove unused parameters
    AST::DeclsType& decls = ast.decls();

    const auto it =
        std::ranges::remove_if(decls, [&ast](const auto& decl) {
            return isa<ShaderParamDecl>(decl.get()) && !ast.is_symbol_accessed_anywhere(*decl);
        }).begin();

    decls.erase(it, decls.end());
}

bool ASTOptimizer::remove_unused_functions(AST& ast) const
{
    auto& decls = ast.decls();

    const auto it = std::ranges::remove_if(decls, [&ast](const auto& decl) {
                        const auto func = asa<FunctionDecl>(decl.get());
                        if (!func)
                        {
                            return false;
                        }

                        if (!func->body())
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

bool ASTOptimizer::remove_unused_structs(AST& ast) const
{
    auto& decls = ast.decls();

    const auto it = std::ranges::remove_if(decls, [&ast](const auto& decl) {
                        const auto strct = asa<StructDecl>(decl.get());
                        if (!strct)
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

bool ASTOptimizer::optimize_block(CodeBlock* block)
{
    const auto it = std::ranges::find_if(m_code_block_name_gens,
                                         [block](const auto& pair) { return pair.first == block; });

    if (it == m_code_block_name_gens.cend())
        m_code_block_name_gens.emplace(block, TempVarNameGen(block));

    return remove_unused_variables(block);
}

bool ASTOptimizer::remove_unused_variables(CodeBlock* block)
{
    SmallVector<VarStmt*, 4> var_stmts;

    for (const auto& stmt : block->stmts())
    {
        if (const auto& var_stmt = asa<VarStmt>(stmt.get()))
            var_stmts.push_back(var_stmt);
    }

    SmallVector<gsl::not_null<VarStmt*>, 4> var_stmts_to_remove;

    for (const auto& var_stmt : var_stmts)
    {
        if (!block->accesses_symbol(var_stmt->variable(), false))
            var_stmts_to_remove.emplace_back(var_stmt);
    }

    const bool has_removed_any = !var_stmts_to_remove.empty();

    for (const auto& lbe : var_stmts_to_remove)
        block->remove_stmt(*lbe);

    return has_removed_any;
}
} // namespace cer::shadercompiler
