// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/AST.hpp"
#include "shadercompiler/Lexer.hpp"
#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
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

    NON_COPYABLE_NON_MOVABLE(Parser);

    ~Parser() noexcept = default;

    AST::DeclsType parse(std::span<const Token> tokens);

  private:
    std::unique_ptr<Decl> parse_decl_at_global_scope();

    std::unique_ptr<Stmt> parse_stmt();

    std::unique_ptr<Expr> parse_expr(std::unique_ptr<Expr> lhs            = {},
                                     int                   min_precedence = 0,
                                     std::string_view      name           = std::string_view());

    std::unique_ptr<Expr> parse_primary_expr();

    std::unique_ptr<ShaderParamDecl> parse_shader_param(const SourceLocation& location,
                                                        const Type&           return_type,
                                                        std::string_view      name);

    std::unique_ptr<FunctionDecl> parse_function(std::string_view      name,
                                                 const SourceLocation& name_location,
                                                 const Type&           return_type);

    std::unique_ptr<StructDecl> parse_struct();

    std::unique_ptr<StructFieldDecl> parse_struct_field_decl();

    std::unique_ptr<FunctionParamDecl> parse_function_param_decl();

    std::unique_ptr<CompoundStmt> parse_compound_stmt(std::unique_ptr<Expr>* parsed_lhs);

    std::unique_ptr<AssignmentStmt> parse_assignment_stmt(std::unique_ptr<Expr> lhs);

    std::unique_ptr<ReturnStmt> parse_return_stmt();

    std::unique_ptr<ForStmt> parse_for_stmt();

    std::unique_ptr<IfStmt> parse_if_stmt(bool is_if);

    std::unique_ptr<VarStmt> parse_var_stmt();

    std::unique_ptr<RangeExpr> parse_range_expr();

    std::unique_ptr<IntLiteralExpr> parse_int_literal_expr();

    std::unique_ptr<BoolLiteralExpr> parse_bool_literal_expr();

    std::unique_ptr<FloatLiteralExpr> parse_float_literal_expr();

    std::unique_ptr<UnaryOpExpr> parse_unary_op_expr();

    std::unique_ptr<StructCtorArg> parse_struct_ctor_arg();

    std::unique_ptr<SymAccessExpr> parse_sym_access_expr();

    std::unique_ptr<StructCtorCall> parse_struct_ctor_call(std::unique_ptr<Expr> callee);

    std::unique_ptr<FunctionCallExpr> parse_function_call(std::unique_ptr<Expr> callee);

    std::unique_ptr<ScientificIntLiteralExpr> parse_scientific_int_literal_expr();

    std::unique_ptr<HexadecimalIntLiteralExpr> parse_hexadecimal_int_literal_expr();

    std::unique_ptr<ParenExpr> parse_paren_expr();

    std::unique_ptr<TernaryExpr> parse_ternary_expr(std::unique_ptr<Expr> condition_expr);

    std::unique_ptr<CodeBlock> parse_code_block();

    const Type& parse_type();

    const Token& next_tk() const;

    void advance();

    void expect_identifier() const;

    std::string_view consume_identifier();

    bool consume_keyword(std::string_view str, bool must_exist);

    bool consume(TokenType type, bool must_exist, std::string_view msg = std::string_view());

    bool is_keyword(std::string_view str) const;

    bool is_at_end() const;

    void verify_not_eof(const SourceLocation& start_location) const;

    class TokenPusher final
    {
      public:
        using TokenIterator = std::span<const Token>::iterator;
        using StackType     = SmallVector<TokenIterator, 4>;

        explicit TokenPusher(StackType& stack, TokenIterator tk);

        NON_COPYABLE_NON_MOVABLE(TokenPusher);

        ~TokenPusher() noexcept;

        TokenIterator initial_tk() const;

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
