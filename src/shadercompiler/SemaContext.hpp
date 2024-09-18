// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/BinOpTable.hpp"
#include "shadercompiler/BuiltInSymbols.hpp"
#include "util/NonCopyable.hpp"

namespace cer::shadercompiler
{
class AST;
class Expr;
class Type;
class SourceLocation;

class SemaContext final
{
  public:
    explicit SemaContext(const AST&            ast,
                         const BuiltInSymbols& built_in_symbols,
                         const BinOpTable&     bin_op_table);

    NON_COPYABLE_NON_MOVABLE(SemaContext);

    ~SemaContext() noexcept = default;

    const AST& ast() const;

    const BuiltInSymbols& built_in_symbols() const;

    const BinOpTable& bin_op_table() const;

    static bool can_assign(const Type& target_type, const Expr& rhs, bool is_implicit_cast_allowed);

    static void verify_type_assignment(const Type& target_type,
                                       const Expr& rhs,
                                       bool        is_implicit_cast_allowed);

    /**
     * Verifies the mutation of a symbol (i.e. prevents assignment to immutable
     * variables).
     *
     * The left-hand-side of the mutation.
     */
    static void verify_symbol_assignment(const Expr& lhs);

    void verify_symbol_name(const SourceLocation& location, std::string_view name) const;

    void set_allow_forbidden_identifier_prefix(bool value);

  private:
    const AST&            m_ast;
    const BuiltInSymbols& m_built_in_symbols;
    const BinOpTable&     m_bin_op_table;
    bool                  m_allow_forbidden_identifier_prefix;
};
} // namespace cer::shadercompiler
