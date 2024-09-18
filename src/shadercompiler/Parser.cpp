// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Parser.hpp"

#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Error.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Lexer.hpp"
#include "shadercompiler/Stmt.hpp"
#include "shadercompiler/Type.hpp"
#include "shadercompiler/TypeCache.hpp"
#include "util/InternalError.hpp"

#include <cassert>
#include <optional>

#define PUSH_TK auto tk_pusher_ = TokenPusher(m_tk_stack, m_tk)
#define POP_TK  tk_pusher_.pop()

namespace cer::shadercompiler
{
struct BinOpInfo
{
    TokenType ttype;
    int       precedence;
    BinOpKind bin_op_kind;
};

static constexpr auto bin_op_precedence_table = std::array{
    BinOpInfo{TokenType::Dot, 11, BinOpKind::MemberAccess},
    BinOpInfo{TokenType::Asterisk, 10, BinOpKind::Multiply},
    BinOpInfo{TokenType::ForwardSlash, 9, BinOpKind::Divide},
    BinOpInfo{TokenType::Plus, 8, BinOpKind::Add},
    BinOpInfo{TokenType::Hyphen, 8, BinOpKind::Subtract},
    BinOpInfo{TokenType::LeftShift, 7, BinOpKind::LeftShift},
    BinOpInfo{TokenType::RightShift, 7, BinOpKind::RightShift},
    BinOpInfo{TokenType::LeftAngleBracket, 7, BinOpKind::LessThan},
    BinOpInfo{TokenType::LessThanOrEqual, 7, BinOpKind::LessThanOrEqual},
    BinOpInfo{TokenType::RightAngleBracket, 7, BinOpKind::GreaterThan},
    BinOpInfo{TokenType::GreaterThanOrEqual, 7, BinOpKind::GreaterThanOrEqual},
    BinOpInfo{TokenType::LogicalEqual, 6, BinOpKind::Equal},
    BinOpInfo{TokenType::LogicalNotEqual, 6, BinOpKind::NotEqual},
    BinOpInfo{TokenType::Ampersand, 5, BinOpKind::BitwiseAnd},
    BinOpInfo{TokenType::Hat, 4, BinOpKind::BitwiseXor},
    BinOpInfo{TokenType::Bar, 3, BinOpKind::BitwiseOr},
    BinOpInfo{TokenType::LogicalAnd, 2, BinOpKind::LogicalAnd},
    BinOpInfo{TokenType::LogicalOr, 1, BinOpKind::LogicalOr},
};

static std::optional<int> get_bin_op_precedence(TokenType type)
{
    const auto it = std::ranges::find_if(bin_op_precedence_table,
                                         [type](const BinOpInfo& tk) { return tk.ttype == type; });

    return it != bin_op_precedence_table.end() ? std::make_optional(it->precedence) : std::nullopt;
}

static std::optional<BinOpKind> get_token_type_to_bin_op_kind(TokenType type)
{
    const auto it = std::ranges::find_if(bin_op_precedence_table,
                                         [type](const BinOpInfo& tk) { return tk.ttype == type; });

    return it != bin_op_precedence_table.end() ? std::make_optional(it->bin_op_kind) : std::nullopt;
}

Parser::Parser(TypeCache& type_cache)
    : m_type_cache(type_cache)
{
}

AST::DeclsType Parser::parse(std::span<const Token> tokens)
{
    if (tokens.empty())
    {
        CER_THROW_INVALID_ARG_STR("No tokens specified.");
    }

    m_tokens = tokens;
    m_tk     = m_tokens.begin();

    AST::DeclsType decls;

    while (!is_at_end())
    {
        PUSH_TK;

        std::unique_ptr<Decl> decl = parse_decl_at_global_scope();

        if (!decl)
        {
            throw Error{m_tk_stack.back()->location, "invalid declaration at global scope"};
        }

        const auto [is_decl_allowed_at_global_scope,
                    is_mutable_variable] = [&decl]() -> std::pair<bool, bool> {
            if (isa<ShaderParamDecl>(*decl) || isa<StructDecl>(*decl) || isa<FunctionDecl>(*decl))
            {
                return {true, false};
            }

            if (const auto var = asa<VarDecl>(decl.get()))
            {
                if (var->is_const())
                {
                    return {true, false};
                }

                return {false, true};
            }

            return {false, false};
        }();

        if (!is_decl_allowed_at_global_scope)
        {
            if (is_mutable_variable)
            {
                throw Error{m_tk->location,
                            "invalid declaration '{}' at global scope; let-bindings at global "
                            "scope must be const",
                            decl->name()};
            }

            throw Error{m_tk->location, "invalid declaration '{}' at global scope", decl->name()};
        }

        assert(decl != nullptr && "Somehow could not build a global decl");

        decls.push_back(std::move(decl));
    }

    return decls;
}

std::unique_ptr<Decl> Parser::parse_decl_at_global_scope()
{
    // struct <StructDecl>
    // var|const <VarStmt>
    // fn <name>() -> <type> <body>

    if (consume_keyword(keyword::struct_, false))
    {
        return parse_struct();
    }

    if (is_keyword(keyword::var) || is_keyword(keyword::const_))
    {
        const std::unique_ptr<VarStmt> stmt = parse_var_stmt();
        return stmt->steal_variable();
    }

    // variable or constant
    const Type&            type          = parse_type();
    const SourceLocation   name_location = m_tk->location;
    const std::string_view name          = consume_identifier();

    if (m_tk->is(TokenType::LeftParen))
    {
        return parse_function(name, name_location, type);
    }

    if (m_tk->is(TokenType::Equal) || m_tk->is(TokenType::Colon) || m_tk->is(TokenType::Semicolon))
    {
        return parse_shader_param(name_location, type, name);
    }

    return nullptr;
}

std::unique_ptr<Stmt> Parser::parse_stmt()
{
    if (is_keyword(keyword::var) || is_keyword(keyword::const_))
    {
        return parse_var_stmt();
    }

    if (consume_keyword(keyword::return_, false))
    {
        return parse_return_stmt();
    }

    if (consume_keyword(keyword::if_, false))
    {
        return parse_if_stmt(true);
    }

    if (consume_keyword(keyword::for_, false))
    {
        return parse_for_stmt();
    }

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Stmt> stmt = parse_compound_stmt(&lhs);

    if (!stmt && !is_at_end() && m_tk->is(TokenType::Equal))
    {
        stmt = parse_assignment_stmt(std::move(lhs));
    }

    return stmt;
}

std::unique_ptr<Expr> Parser::parse_expr(std::unique_ptr<Expr> lhs,
                                         int                   min_precedence,
                                         std::string_view      name)
{
    // Shunting-yard algorithm

    const auto fail = [&]() -> std::unique_ptr<Expr> {
        if (!name.empty())
        {
            throw Error{m_tk->location, "expected a {}", name};
        }

        return nullptr;
    };

    if (!lhs)
    {
        lhs = parse_primary_expr();

        if (!lhs)
        {
            return fail();
        }
    }

    TokenType          lookahead               = m_tk->type;
    SourceLocation     lookahead_location      = m_tk->location;
    std::optional<int> lookahead_op_precedence = get_bin_op_precedence(lookahead);

    while (lookahead_op_precedence && *lookahead_op_precedence >= min_precedence)
    {
        const BinOpKind op            = *get_token_type_to_bin_op_kind(lookahead);
        const int       op_precedence = *lookahead_op_precedence;

        advance();

        std::unique_ptr<Expr> rhs = parse_primary_expr();

        if (!rhs)
        {
            return fail();
        }

        lookahead               = m_tk->type;
        lookahead_location      = m_tk->location;
        lookahead_op_precedence = get_bin_op_precedence(lookahead);

        while (lookahead_op_precedence && *lookahead_op_precedence > op_precedence)
        {
            rhs = parse_expr(std::move(rhs), op_precedence + 1, name);

            if (!rhs)
            {
                return fail();
            }

            lookahead               = m_tk->type;
            lookahead_location      = m_tk->location;
            lookahead_op_precedence = get_bin_op_precedence(lookahead);
        }

        lhs = std::make_unique<BinOpExpr>(lookahead_location, op, std::move(lhs), std::move(rhs));
    }

    if (m_tk->is(TokenType::QuestionMark))
    {
        lhs = parse_ternary_expr(std::move(lhs));
    }

    return lhs;
}

std::unique_ptr<Expr> Parser::parse_primary_expr()
{
    std::unique_ptr<Expr> expr;

    if (auto paren_expr = parse_paren_expr())
    {
        expr = std::move(paren_expr);
    }
    else if (auto int_lit = parse_int_literal_expr())
    {
        expr = std::move(int_lit);
    }
    else if (auto scint_lit = parse_scientific_int_literal_expr())
    {
        expr = std::move(scint_lit);
    }
    else if (auto hexint_lit = parse_hexadecimal_int_literal_expr())
    {
        expr = std::move(hexint_lit);
    }
    else if (auto float_lit = parse_float_literal_expr())
    {
        expr = std::move(float_lit);
    }
    else if (auto bool_lit = parse_bool_literal_expr())
    {
        expr = std::move(bool_lit);
    }
    else if (auto sym_access = parse_sym_access_expr())
    {
        expr = std::move(sym_access);
    }
    else
    {
        expr = parse_unary_op_expr();
    }

    if (expr)
    {
        // Got the first part. See what follows.
        if (m_tk->is(TokenType::LeftParen))
        {
            // function call
            expr = parse_function_call(std::move(expr));
        }
        else if (m_tk->is(TokenType::LeftBrace))
        {
            // struct ctor call
            expr = parse_struct_ctor_call(std::move(expr));
        }
        else if (m_tk->is(TokenType::LeftBracket))
        {
            // subscript expression
            advance();

            std::unique_ptr<Expr> index_expr = parse_expr();

            if (!index_expr)
            {
                return nullptr;
            }

            consume(TokenType::RightBracket, true);

            expr = std::make_unique<SubscriptExpr>(index_expr->location(),
                                                   std::move(expr),
                                                   std::move(index_expr));
        }
    }

    return expr;
}

std::unique_ptr<ShaderParamDecl> Parser::parse_shader_param(const SourceLocation& location,
                                                            const Type&           return_type,
                                                            std::string_view      name)
{
    std::unique_ptr<Expr> default_value_expr;

    if (m_tk->is(TokenType::Equal))
    {
        advance();
        default_value_expr = parse_expr({}, 0, "default parameter value expression");
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<ShaderParamDecl>(location,
                                             name,
                                             return_type,
                                             std::move(default_value_expr));
}

std::unique_ptr<FunctionDecl> Parser::parse_function(std::string_view      name,
                                                     const SourceLocation& name_location,
                                                     const Type&           return_type)
{
    PUSH_TK;

    consume(TokenType::LeftParen, true);

    SmallVector<std::unique_ptr<FunctionParamDecl>, 4> params;

    while (!is_at_end() && !m_tk->is(TokenType::RightParen))
    {
        params.push_back(parse_function_param_decl());

        if (!m_tk->is(TokenType::Comma))
        {
            break;
        }

        advance();
    }

    consume(TokenType::RightParen, true);

    std::unique_ptr<CodeBlock> body = parse_code_block();

    return std::make_unique<FunctionDecl>(name_location,
                                          name,
                                          std::move(params),
                                          return_type,
                                          std::move(body),
                                          false);
}

std::unique_ptr<StructDecl> Parser::parse_struct()
{
    // Assume 'struct' is already consumed

    PUSH_TK;
    const std::string_view name = consume_identifier();

    consume(TokenType::LeftBrace, true);

    SmallVector<std::unique_ptr<StructFieldDecl>, 8> fields;

    while (!is_at_end() && !m_tk->is(TokenType::RightBrace))
    {
        fields.push_back(parse_struct_field_decl());
    }

    consume(TokenType::RightBrace, true);

    return std::make_unique<StructDecl>(tk_pusher_.initial_tk()->location,
                                        name,
                                        std::move(fields),
                                        false);
}

std::unique_ptr<StructFieldDecl> Parser::parse_struct_field_decl()
{
    const Type& type = parse_type();
    PUSH_TK;
    const std::string_view name = consume_identifier();

    consume(TokenType::Semicolon, true);

    return std::make_unique<StructFieldDecl>(tk_pusher_.initial_tk()->location, name, type);
}

std::unique_ptr<FunctionParamDecl> Parser::parse_function_param_decl()
{
    const Type& type = parse_type();
    PUSH_TK;
    const std::string_view name = consume_identifier();

    return std::make_unique<FunctionParamDecl>(tk_pusher_.initial_tk()->location, name, type);
}

std::unique_ptr<CompoundStmt> Parser::parse_compound_stmt(std::unique_ptr<Expr>* parsed_lhs)
{
    PUSH_TK;

    std::unique_ptr<Expr> lhs = parse_expr();
    if (!lhs)
    {
        return nullptr;
    }

    *parsed_lhs = std::move(lhs);

    CompoundStmtKind kind = [&] {
        const std::string_view mod = m_tk->value;

        if (mod == "*=")
        {
            return CompoundStmtKind::Mul;
        }
        if (mod == "/=")
        {
            return CompoundStmtKind::Div;
        }
        if (mod == "+=")
        {
            return CompoundStmtKind::Add;
        }
        if (mod == "-=")
        {
            return CompoundStmtKind::Sub;
        }

        return static_cast<CompoundStmtKind>(-1);
    }();

    if (kind == static_cast<CompoundStmtKind>(-1))
    {
        return nullptr;
    }

    advance();

    auto rhs = parse_expr();

    if (!rhs)
    {
        return nullptr;
    }

    consume(TokenType::Semicolon, true);

    lhs = std::move(*parsed_lhs);
    parsed_lhs->reset();

    return std::make_unique<CompoundStmt>(tk_pusher_.initial_tk()->location,
                                          kind,
                                          std::move(lhs),
                                          std::move(rhs));
}

std::unique_ptr<AssignmentStmt> Parser::parse_assignment_stmt(std::unique_ptr<Expr> lhs)
{
    PUSH_TK;

    if (lhs == nullptr)
    {
        lhs = parse_expr();

        if (lhs == nullptr)
        {
            return nullptr;
        }
    }

    assert(lhs);

    if (!consume(TokenType::Equal, false))
    {
        return nullptr;
    }

    std::unique_ptr<Expr> rhs = parse_expr();

    if (!rhs)
    {
        throw Error{(m_tk - 1)->location,
                    "expected a right-hand-side expression for the assignment"};
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<AssignmentStmt>(tk_pusher_.initial_tk()->location,
                                            std::move(lhs),
                                            std::move(rhs));
}

std::unique_ptr<ReturnStmt> Parser::parse_return_stmt()
{
    // Assume 'return' is already consumed.

    PUSH_TK;

    std::unique_ptr<Expr> expr = parse_expr();

    if (expr == nullptr)
    {
        return nullptr;
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<ReturnStmt>(tk_pusher_.initial_tk()->location, std::move(expr));
}

std::unique_ptr<ForStmt> Parser::parse_for_stmt()
{
    // Assume 'for' is consumed.

    PUSH_TK;

    consume(TokenType::LeftParen, true);

    const SourceLocation   loop_var_location = m_tk->location;
    const std::string_view loop_var_name     = consume_identifier();

    std::unique_ptr loop_var =
        std::make_unique<ForLoopVariableDecl>(loop_var_location, loop_var_name);

    consume_keyword(keyword::in, true);

    std::unique_ptr<RangeExpr> range = parse_range_expr();

    if (!range)
    {
        throw Error{m_tk->location, "expected a range expression"};
    }

    consume(TokenType::RightParen, true);

    std::unique_ptr<CodeBlock> body = parse_code_block();

    return std::make_unique<ForStmt>(tk_pusher_.initial_tk()->location,
                                     std::move(loop_var),
                                     std::move(range),
                                     std::move(body));
}

std::unique_ptr<IfStmt> Parser::parse_if_stmt(bool is_if)
{
    // Assume 'if' is consumed.

    PUSH_TK;

    std::unique_ptr<Expr> condition;

    if (is_if)
    {
        consume(TokenType::LeftParen, true);

        condition = parse_expr();

        if (!condition)
        {
            throw Error{m_tk->location, "expected a condition expression"};
        }

        consume(TokenType::RightParen, true);
    }

    std::unique_ptr<CodeBlock> body = parse_code_block();
    std::unique_ptr<IfStmt>    next;

    if (consume_keyword(keyword::else_, false))
    {
        next = parse_if_stmt(consume_keyword(keyword::if_, false));
        if (!next)
        {
            throw Error{m_tk->location, "expected a consecutive if-statement"};
        }
    }

    return std::make_unique<IfStmt>(tk_pusher_.initial_tk()->location,
                                    std::move(condition),
                                    std::move(body),
                                    std::move(next));
}

std::unique_ptr<VarStmt> Parser::parse_var_stmt()
{
    const bool is_var   = is_keyword(keyword::var);
    const bool is_const = is_keyword(keyword::const_);

    if (!is_var && !is_const)
    {
        return nullptr;
    }

    advance();

    const SourceLocation   name_location = m_tk->location;
    const std::string_view name          = consume_identifier();

    consume(TokenType::Equal, true);

    std::unique_ptr<Expr> expr = parse_expr();

    if (!expr)
    {
        throw Error{m_tk->location, "expected a variable statement expression"};
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<VarStmt>(
        name_location,
        std::make_unique<VarDecl>(name_location, name, std::move(expr), is_const));
}

std::unique_ptr<RangeExpr> Parser::parse_range_expr()
{
    PUSH_TK;

    std::unique_ptr<Expr> start = parse_expr();

    if (!start)
    {
        return nullptr;
    }

    consume(TokenType::DotDot, true);

    std::unique_ptr<Expr> end = parse_expr();

    if (!end)
    {
        throw Error{m_tk->location, "expected a range-end expression"};
    }

    return std::make_unique<RangeExpr>(tk_pusher_.initial_tk()->location,
                                       std::move(start),
                                       std::move(end));
}

std::unique_ptr<IntLiteralExpr> Parser::parse_int_literal_expr()
{
    if (m_tk->is(TokenType::IntLiteral))
    {
        const SourceLocation location = m_tk->location;
        int32_t              value    = 0;

        try
        {
            value = std::stoi(std::string{m_tk->value});
        }
        catch (const std::exception&)
        {
            throw Error{location, "failed to parse integer literal"};
        }

        advance();

        return std::make_unique<IntLiteralExpr>(location, value);
    }

    return nullptr;
}

std::unique_ptr<BoolLiteralExpr> Parser::parse_bool_literal_expr()
{
    if (is_keyword(keyword::true_) || is_keyword(keyword::false_))
    {
        const bool           value    = m_tk->value == keyword::true_;
        const SourceLocation location = m_tk->location;
        advance();

        return std::make_unique<BoolLiteralExpr>(location, value);
    }

    return nullptr;
}

auto Parser::parse_float_literal_expr() -> std::unique_ptr<FloatLiteralExpr>
{
    if (m_tk->is(TokenType::FloatLiteral))
    {
        const SourceLocation location = m_tk->location;
        std::string_view     string_value;
        double               value = 0.0;

        try
        {
            string_value = m_tk->value;
            value        = std::stod(std::string{string_value});
        }
        catch (const std::exception&)
        {
            throw Error{location, "failed to parse float literal"};
        }

        advance();

        return std::make_unique<FloatLiteralExpr>(location, string_value, value);
    }

    return nullptr;
}

std::unique_ptr<UnaryOpExpr> Parser::parse_unary_op_expr()
{
    PUSH_TK;

    std::optional<UnaryOpKind> op_kind;

    if (m_tk->is(TokenType::ExclamationMark))
    {
        op_kind = UnaryOpKind::LogicalNot;
    }
    else if (m_tk->is(TokenType::Hyphen))
    {
        op_kind = UnaryOpKind::Negate;
    }

    if (!op_kind)
    {
        return nullptr;
    }

    advance();

    auto expr = parse_primary_expr();

    if (!expr)
    {
        throw Error{m_tk->location, "expected an expression for the unary operation"};
    }

    return std::make_unique<UnaryOpExpr>(tk_pusher_.initial_tk()->location,
                                         *op_kind,
                                         std::move(expr));
}

std::unique_ptr<StructCtorArg> Parser::parse_struct_ctor_arg()
{
    PUSH_TK;
    const std::string_view name = consume_identifier();

    consume(TokenType::Equal, true);

    std::unique_ptr<Expr> expr = parse_expr();

    if (!expr)
    {
        throw Error{m_tk->location, "expected an expression for struct field '{}'", name};
    }

    return std::make_unique<StructCtorArg>(tk_pusher_.initial_tk()->location,
                                           name,
                                           std::move(expr));
}

std::unique_ptr<SymAccessExpr> Parser::parse_sym_access_expr()
{
    if (m_tk->is(TokenType::Identifier))
    {
        const std::string_view name     = m_tk->value;
        const SourceLocation   location = m_tk->location;
        advance();

        return std::make_unique<SymAccessExpr>(location, name);
    }

    return nullptr;
}

std::unique_ptr<StructCtorCall> Parser::parse_struct_ctor_call(std::unique_ptr<Expr> callee)
{
    PUSH_TK;
    consume(TokenType::LeftBrace, true);

    SmallVector<std::unique_ptr<StructCtorArg>, 4> args;

    while (!is_at_end() && !m_tk->is(TokenType::RightBrace))
    {
        std::unique_ptr<StructCtorArg> arg = parse_struct_ctor_arg();

        if (!arg)
        {
            throw Error{m_tk->location,
                        "expected a struct field argument in the form "
                        "of '<name>: <expression>'"};
        }

        args.push_back(std::move(arg));

        if (m_tk->is(TokenType::Comma))
        {
            advance();
        }
    }

    consume(TokenType::RightBrace, true, "expected a struct field initializer or '}'");

    return std::make_unique<StructCtorCall>(tk_pusher_.initial_tk()->location,
                                            std::move(callee),
                                            std::move(args));
}

std::unique_ptr<FunctionCallExpr> Parser::parse_function_call(std::unique_ptr<Expr> callee)
{
    PUSH_TK;
    consume(TokenType::LeftParen, true);

    SmallVector<std::unique_ptr<Expr>, 4> args;

    while (!is_at_end() && !m_tk->is(TokenType::RightParen))
    {
        std::unique_ptr<Expr> arg = parse_expr();

        if (!arg)
        {
            throw Error{m_tk->location, "expected a function call argument"};
        }

        args.push_back(std::move(arg));

        if (!m_tk->is(TokenType::Comma))
        {
            break;
        }

        advance();
    }

    consume(TokenType::RightParen, true, "expected a function call argument or ')'");

    return std::make_unique<FunctionCallExpr>(tk_pusher_.initial_tk()->location,
                                              std::move(callee),
                                              std::move(args));
}

std::unique_ptr<ScientificIntLiteralExpr> Parser::parse_scientific_int_literal_expr()
{
    if (m_tk->is(TokenType::ScientificNumber))
    {
        const SourceLocation   location = m_tk->location;
        const std::string_view value    = m_tk->value;
        advance();

        return std::make_unique<ScientificIntLiteralExpr>(location, value);
    }

    return nullptr;
}

std::unique_ptr<HexadecimalIntLiteralExpr> Parser::parse_hexadecimal_int_literal_expr()
{
    if (m_tk->is(TokenType::HexNumber))
    {
        const SourceLocation   location = m_tk->location;
        const std::string_view value    = m_tk->value;
        advance();

        return std::make_unique<HexadecimalIntLiteralExpr>(location, value);
    }

    return nullptr;
}

std::unique_ptr<ParenExpr> Parser::parse_paren_expr()
{
    PUSH_TK;

    if (!consume(TokenType::LeftParen, false))
    {
        return nullptr;
    }

    std::unique_ptr<Expr> expr = parse_expr();

    if (!expr)
    {
        throw Error{m_tk->location, "expected an expression inside parentheses"};
    }

    consume(TokenType::RightParen, true);

    return std::make_unique<ParenExpr>(tk_pusher_.initial_tk()->location, std::move(expr));
}

std::unique_ptr<TernaryExpr> Parser::parse_ternary_expr(std::unique_ptr<Expr> condition_expr)
{
    assert(condition_expr);

    if (!consume(TokenType::QuestionMark, false))
    {
        return nullptr;
    }

    std::unique_ptr<Expr> true_expr = parse_expr({}, 0, "true-expression");

    consume(TokenType::Colon, true);

    std::unique_ptr<Expr> false_expr = parse_expr({}, 0, "false-expression");

    return std::make_unique<TernaryExpr>(condition_expr->location(),
                                         std::move(condition_expr),
                                         std::move(true_expr),
                                         std::move(false_expr));
}

std::unique_ptr<CodeBlock> Parser::parse_code_block()
{
    const SourceLocation location = m_tk->location;

    consume(TokenType::LeftBrace, true, "expected a code block");

    CodeBlock::StmtsType stmts;

    while (!is_at_end() && !m_tk->is(TokenType::RightBrace))
    {
        std::unique_ptr<Stmt> stmt = parse_stmt();

        if (stmt == nullptr)
        {
            throw Error{m_tk->location, "expected a statement"};
        }

        stmts.push_back(std::move(stmt));
    }

    consume(TokenType::RightBrace, true);

    return std::make_unique<CodeBlock>(location, std::move(stmts));
}

const Type& Parser::parse_type()
{
    const SourceLocation   location       = m_tk->location;
    const std::string_view base_type_name = consume_identifier();

    if (consume(TokenType::LeftBracket, false))
    {
        // Array type
        std::unique_ptr<Expr> size_expr = parse_expr();

        if (!size_expr)
        {
            throw Error{m_tk->location, "expected a size expression for the array type"};
        }

        consume(TokenType::RightBracket, true, "expected a ']' that ends the array type");

        return *m_type_cache.create_array_type(location, base_type_name, std::move(size_expr));
    }

    return *m_type_cache.create_unresolved_type(location, base_type_name);
}

const Token& Parser::next_tk() const
{
    assert(m_tk + 1 < m_tokens.end());
    return *(m_tk + 1);
}

void Parser::advance()
{
    ++m_tk;
}

void Parser::expect_identifier() const
{
    if (!m_tk->is(TokenType::Identifier))
        throw Error(m_tk->location, "expected an identifier");
}

std::string_view Parser::consume_identifier()
{
    expect_identifier();
    const auto tk = m_tk;
    advance();
    return tk->value;
}

bool Parser::consume_keyword(std::string_view str, bool must_exist)
{
    if (m_tk->is(TokenType::Keyword) && m_tk->value == str)
    {
        advance();
        return true;
    }

    if (must_exist)
    {
        throw Error{m_tk->location, "expected keyword '{}'", str};
    }

    return false;
}

bool Parser::consume(TokenType type, bool must_exist, std::string_view msg)
{
    if (!m_tk->is(type))
    {
        if (must_exist)
        {
            const SourceLocation error_location =
                m_tk->is(TokenType::EndOfFile) ? m_tk_stack.back()->location : m_tk->location;

            if (msg.empty())
            {
                throw Error{error_location, "expected '{}'", token_type_to_string(type)};
            }

            throw Error{error_location, "{}", msg};
        }

        return false;
    }

    advance();

    return true;
}

auto Parser::is_keyword(std::string_view str) const -> bool
{
    return m_tk->is(TokenType::Keyword) && m_tk->value == str;
}

auto Parser::is_at_end() const -> bool
{
    return m_tk->is(TokenType::EndOfFile) || m_tk >= m_tokens.end();
}

auto Parser::verify_not_eof(const SourceLocation& start_location) const -> void
{
    if (is_at_end())
    {
        throw Error{start_location, "end-of-file reached unexpectedly"};
    }
}

Parser::TokenPusher::TokenPusher(StackType& stack, TokenIterator tk)
    : m_stack(stack)
    , m_initial_tk(tk)
{
    m_stack.push_back(tk);
}

Parser::TokenPusher::~TokenPusher() noexcept
{
    if (m_is_active)
        m_stack.pop_back();
}

Parser::TokenPusher::TokenIterator Parser::TokenPusher::initial_tk() const
{
    return m_initial_tk;
}

void Parser::TokenPusher::pop()
{
    m_stack.pop_back();
    m_is_active = false;
}
} // namespace cer::shadercompiler
