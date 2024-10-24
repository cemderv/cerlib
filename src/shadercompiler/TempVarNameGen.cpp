// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/TempVarNameGen.hpp"
#include "cerlib/Formatters.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Naming.hpp"
#include "shadercompiler/Stmt.hpp"

namespace cer::shadercompiler
{
TempVarNameGen::TempVarNameGen(const CodeBlock* block)
    : m_prefix(naming::forbidden_identifier_prefix)
    , m_counter(0)
{
    m_prefix += "var";

    if (block != nullptr)
    {
        for (const auto& stmt : block->stmts())
        {
            const auto* lbe = asa<VarStmt>(stmt.get());

            if (lbe == nullptr)
            {
                continue;
            }

            const auto name = lbe->name();

            if (!name.starts_with(m_prefix))
            {
                continue;
            }

            if (const auto num = std::stoi(String{name.substr(m_prefix.size())}); num >= m_counter)
            {
                m_counter = num + 1;
            }
        }
    }
}

auto TempVarNameGen::next(std::string_view hint) -> String
{
    auto str = hint.empty() ? cer_fmt::format("{}{}", m_prefix, m_counter)
                            : cer_fmt::format("{}{}_{}", m_prefix, m_counter, hint);

    ++m_counter;

    return str;
}
} // namespace cer::shadercompiler
