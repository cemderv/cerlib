// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/AST.hpp"
#include "shadercompiler/Lexer.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
#include <span>

namespace cer::shadercompiler
{
enum class TokenType;
class TypeCache;
class Type;
class Decl;
class Stmt;
class Expr;
class ShaderParamDecl;
class FunctionDecl;
class StructDecl;
class StructFieldDecl;
class FunctionParamDecl;
class ForLoopVariableDecl;
class VarDecl;
class CompoundStmt;
class AssignmentStmt;
class ReturnStmt;
class ForStmt;
class IfStmt;
class VarStmt;
class RangeExpr;
class BinOpExpr;
class IntLiteralExpr;
class UIntLiteralExpr;
class BoolLiteralExpr;
class FloatLiteralExpr;
class UnaryOpExpr;
class StructCtorArg;
class SymAccessExpr;
class StructCtorCall;
class FunctionCallExpr;
class SubscriptExpr;
class ScientificIntLiteralExpr;
class HexadecimalIntLiteralExpr;
class ParenExpr;
class TernaryExpr;
class CodeBlock;

class Parser final
{
  public:
    explicit Parser(TypeCache& type_cache);

    forbid_copy_and_move(Parser);

    ~Parser() noexcept = default;

    auto parse(std::span<const Token> tokens) -> AST::DeclsType;

  private:
    auto parse_decl_at_global_scope() -> std::unique_ptr<Decl>;

    auto parse_stmt() -> std::unique_ptr<Stmt>;

    auto parse_expr(std::unique_ptr<Expr> lhs            = {},
                    int                   min_precedence = 0,
                    std::string_view      name = std::string_view()) -> std::unique_ptr<Expr>;

    auto parse_primary_expr() -> std::unique_ptr<Expr>;

    auto parse_shader_param(const SourceLocation& location,
                            const Type&           return_type,
                            std::string_view      name) -> std::unique_ptr<ShaderParamDecl>;

    auto parse_function(std::string_view      name,
                        const SourceLocation& name_location,
                        const Type&           return_type) -> std::unique_ptr<FunctionDecl>;

    auto parse_struct() -> std::unique_ptr<StructDecl>;

    auto parse_struct_field_decl() -> std::unique_ptr<StructFieldDecl>;

    auto parse_function_param_decl() -> std::unique_ptr<FunctionParamDecl>;

    auto parse_compound_stmt(std::unique_ptr<Expr>* parsed_lhs) -> std::unique_ptr<CompoundStmt>;

    auto parse_assignment_stmt(std::unique_ptr<Expr> lhs) -> std::unique_ptr<AssignmentStmt>;

    auto parse_return_stmt() -> std::unique_ptr<ReturnStmt>;

    auto parse_for_stmt() -> std::unique_ptr<ForStmt>;

    auto parse_if_stmt(bool is_if) -> std::unique_ptr<IfStmt>;

    auto parse_var_stmt() -> std::unique_ptr<VarStmt>;

    auto parse_range_expr() -> std::unique_ptr<RangeExpr>;

    auto parse_int_literal_expr() -> std::unique_ptr<IntLiteralExpr>;

    auto parse_bool_literal_expr() -> std::unique_ptr<BoolLiteralExpr>;

    auto parse_float_literal_expr() -> std::unique_ptr<FloatLiteralExpr>;

    auto parse_unary_op_expr() -> std::unique_ptr<UnaryOpExpr>;

    auto parse_struct_ctor_arg() -> std::unique_ptr<StructCtorArg>;

    auto parse_sym_access_expr() -> std::unique_ptr<SymAccessExpr>;

    auto parse_struct_ctor_call(std::unique_ptr<Expr> callee) -> std::unique_ptr<StructCtorCall>;

    auto parse_function_call(std::unique_ptr<Expr> callee) -> std::unique_ptr<FunctionCallExpr>;

    auto parse_scientific_int_literal_expr() -> std::unique_ptr<ScientificIntLiteralExpr>;

    auto parse_hexadecimal_int_literal_expr() -> std::unique_ptr<HexadecimalIntLiteralExpr>;

    auto parse_paren_expr() -> std::unique_ptr<ParenExpr>;

    auto parse_ternary_expr(std::unique_ptr<Expr> condition_expr) -> std::unique_ptr<TernaryExpr>;

    auto parse_code_block() -> std::unique_ptr<CodeBlock>;

    auto parse_type() -> const Type&;

    auto next_tk() const -> const Token&;

    void advance();

    void expect_identifier() const;

    auto consume_identifier() -> std::string_view;

    auto consume_keyword(std::string_view str, bool must_exist) -> bool;

    auto consume(TokenType type, bool must_exist, std::string_view msg = std::string_view())
        -> bool;

    auto is_keyword(std::string_view str) const -> bool;

    auto is_at_end() const -> bool;

    void verify_not_eof(const SourceLocation& start_location) const;

    class TokenPusher final
    {
      public:
        using TokenIterator = std::span<const Token>::iterator;
        using StackType     = small_vector<TokenIterator, 4>;

        explicit TokenPusher(StackType& stack, TokenIterator tk);

        forbid_copy_and_move(TokenPusher);

        ~TokenPusher() noexcept;

        auto initial_tk() const -> TokenIterator;

        void pop();

      private:
        StackType&    m_stack;
        TokenIterator m_initial_tk;
        bool          m_is_active = true;
    };

    TypeCache&                       m_type_cache;
    std::span<const Token>           m_tokens;
    std::span<const Token>::iterator m_tk;
    TokenPusher::StackType           m_tk_stack;
};
} // namespace cer::shadercompiler
