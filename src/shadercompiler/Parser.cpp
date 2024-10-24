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
#include <cassert>
#include <cerlib/Option.hpp>

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
    BinOpInfo{
        .ttype       = TokenType::Dot,
        .precedence  = 11,
        .bin_op_kind = BinOpKind::MemberAccess,
    },
    BinOpInfo{
        .ttype       = TokenType::Asterisk,
        .precedence  = 10,
        .bin_op_kind = BinOpKind::Multiply,
    },
    BinOpInfo{
        .ttype       = TokenType::ForwardSlash,
        .precedence  = 9,
        .bin_op_kind = BinOpKind::Divide,
    },
    BinOpInfo{
        .ttype       = TokenType::Plus,
        .precedence  = 8,
        .bin_op_kind = BinOpKind::Add,
    },
    BinOpInfo{
        .ttype       = TokenType::Hyphen,
        .precedence  = 8,
        .bin_op_kind = BinOpKind::Subtract,
    },
    BinOpInfo{
        .ttype       = TokenType::LeftShift,
        .precedence  = 7,
        .bin_op_kind = BinOpKind::LeftShift,
    },
    BinOpInfo{
        .ttype       = TokenType::RightShift,
        .precedence  = 7,
        .bin_op_kind = BinOpKind::RightShift,
    },
    BinOpInfo{
        .ttype       = TokenType::LeftAngleBracket,
        .precedence  = 7,
        .bin_op_kind = BinOpKind::LessThan,
    },
    BinOpInfo{
        .ttype       = TokenType::LessThanOrEqual,
        .precedence  = 7,
        .bin_op_kind = BinOpKind::LessThanOrEqual,
    },
    BinOpInfo{
        .ttype       = TokenType::RightAngleBracket,
        .precedence  = 7,
        .bin_op_kind = BinOpKind::GreaterThan,
    },
    BinOpInfo{
        .ttype       = TokenType::GreaterThanOrEqual,
        .precedence  = 7,
        .bin_op_kind = BinOpKind::GreaterThanOrEqual,
    },
    BinOpInfo{
        .ttype       = TokenType::LogicalEqual,
        .precedence  = 6,
        .bin_op_kind = BinOpKind::Equal,
    },
    BinOpInfo{
        .ttype       = TokenType::LogicalNotEqual,
        .precedence  = 6,
        .bin_op_kind = BinOpKind::NotEqual,
    },
    BinOpInfo{
        .ttype       = TokenType::Ampersand,
        .precedence  = 5,
        .bin_op_kind = BinOpKind::BitwiseAnd,
    },
    BinOpInfo{
        .ttype       = TokenType::Hat,
        .precedence  = 4,
        .bin_op_kind = BinOpKind::BitwiseXor,
    },
    BinOpInfo{
        .ttype       = TokenType::Bar,
        .precedence  = 3,
        .bin_op_kind = BinOpKind::BitwiseOr,
    },
    BinOpInfo{
        .ttype       = TokenType::LogicalAnd,
        .precedence  = 2,
        .bin_op_kind = BinOpKind::LogicalAnd,
    },
    BinOpInfo{
        .ttype       = TokenType::LogicalOr,
        .precedence  = 1,
        .bin_op_kind = BinOpKind::LogicalOr,
    },
};

static auto get_bin_op_precedence(TokenType type) -> Option<int>
{
    const auto it = std::ranges::find_if(bin_op_precedence_table, [type](const auto& op) {
        return op.ttype == type;
    });

    return it != bin_op_precedence_table.end() ? std::make_optional(it->precedence) : std::nullopt;
}

static auto get_token_type_to_bin_op_kind(TokenType type) -> Option<BinOpKind>
{
    const auto it = std::ranges::find_if(bin_op_precedence_table, [type](const auto& op) {
        return op.ttype == type;
    });

    return it != bin_op_precedence_table.end() ? std::make_optional(it->bin_op_kind) : std::nullopt;
}

Parser::Parser(TypeCache& type_cache)
    : m_type_cache(type_cache)
{
}

auto Parser::parse(std::span<const Token> tokens) -> AST::DeclsType
{
    if (tokens.empty())
    {
        throw std::invalid_argument{"No tokens specified."};
    }

    m_tokens = tokens;
    m_tk     = m_tokens.begin();

    auto decls = AST::DeclsType{};

    while (!is_at_end())
    {
        PUSH_TK;

        auto decl = parse_decl_at_global_scope();

        if (decl == nullptr)
        {
            throw Error{m_tk_stack.back()->location, "invalid declaration at global scope"};
        }

        const auto [is_decl_allowed_at_global_scope, is_mutable_variable] = [&decl] {
            if (isa<ShaderParamDecl>(*decl) || isa<StructDecl>(*decl) || isa<FunctionDecl>(*decl))
            {
                return std::pair{true, false};
            }

            if (const auto* var = asa<VarDecl>(decl.get()))
            {
                if (var->is_const())
                {
                    return std::pair{true, false};
                }

                return std::pair{false, true};
            }

            return std::pair{false, false};
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

auto Parser::parse_decl_at_global_scope() -> UniquePtr<Decl>
{
    // struct <StructDecl>
    // var|const <VarStmt>
    // <type> <name>() <body>

    if (consume_keyword(keyword::struct_, false))
    {
        return parse_struct();
    }

    if (is_keyword(keyword::var) || is_keyword(keyword::const_))
    {
        return parse_var_stmt()->steal_variable();
    }

    // variable or constant
    const auto& type          = parse_type();
    const auto& name_location = m_tk->location;
    const auto  name          = consume_identifier();

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

auto Parser::parse_stmt() -> UniquePtr<Stmt>
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

    auto            lhs  = UniquePtr<Expr>{};
    UniquePtr<Stmt> stmt = parse_compound_stmt(&lhs);

    if (!stmt && !is_at_end() && m_tk->is(TokenType::Equal))
    {
        stmt = parse_assignment_stmt(std::move(lhs));
    }

    return stmt;
}

auto Parser::parse_expr(UniquePtr<Expr> lhs, int min_precedence, std::string_view name)
    -> UniquePtr<Expr>
{
    // Shunting-yard algorithm

    const auto fail = [this, &name] {
        if (!name.empty())
        {
            throw Error{m_tk->location, "expected a {}", name};
        }

        return UniquePtr<Expr>{};
    };

    if (lhs == nullptr)
    {
        lhs = parse_primary_expr();

        if (lhs == nullptr)
        {
            return fail();
        }
    }

    auto lookahead               = m_tk->type;
    auto lookahead_location      = m_tk->location;
    auto lookahead_op_precedence = get_bin_op_precedence(lookahead);

    while (lookahead_op_precedence && *lookahead_op_precedence >= min_precedence)
    {
        const auto op            = *get_token_type_to_bin_op_kind(lookahead);
        const auto op_precedence = *lookahead_op_precedence;

        advance();

        auto rhs = parse_primary_expr();

        if (rhs == nullptr)
        {
            return fail();
        }

        lookahead               = m_tk->type;
        lookahead_location      = m_tk->location;
        lookahead_op_precedence = get_bin_op_precedence(lookahead);

        while (lookahead_op_precedence && *lookahead_op_precedence > op_precedence)
        {
            rhs = parse_expr(std::move(rhs), op_precedence + 1, name);

            if (rhs == nullptr)
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

auto Parser::parse_primary_expr() -> UniquePtr<Expr>
{
    auto expr = UniquePtr<Expr>{};

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

    if (expr != nullptr)
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

            auto index_expr = parse_expr();

            if (index_expr == nullptr)
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

auto Parser::parse_shader_param(const SourceLocation& location,
                                const Type&           return_type,
                                std::string_view      name) -> UniquePtr<ShaderParamDecl>
{
    auto default_value_expr = UniquePtr<Expr>{};

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

auto Parser::parse_function(std::string_view      name,
                            const SourceLocation& name_location,
                            const Type&           return_type) -> UniquePtr<FunctionDecl>
{
    PUSH_TK;

    consume(TokenType::LeftParen, true);

    auto params = UniquePtrList<FunctionParamDecl, 4>{};

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

    auto body = parse_code_block();

    return std::make_unique<FunctionDecl>(name_location,
                                          name,
                                          std::move(params),
                                          return_type,
                                          std::move(body),
                                          false);
}

auto Parser::parse_struct() -> UniquePtr<StructDecl>
{
    // Assume 'struct' is already consumed

    PUSH_TK;

    const auto name = consume_identifier();

    consume(TokenType::LeftBrace, true);

    auto fields = UniquePtrList<StructFieldDecl, 8>{};

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

auto Parser::parse_struct_field_decl() -> UniquePtr<StructFieldDecl>
{
    const auto& type = parse_type();

    PUSH_TK;

    const auto name = consume_identifier();

    consume(TokenType::Semicolon, true);

    return std::make_unique<StructFieldDecl>(tk_pusher_.initial_tk()->location, name, type);
}

auto Parser::parse_function_param_decl() -> UniquePtr<FunctionParamDecl>
{
    const auto& type = parse_type();

    PUSH_TK;

    const auto name = consume_identifier();

    return std::make_unique<FunctionParamDecl>(tk_pusher_.initial_tk()->location, name, type);
}

auto Parser::parse_compound_stmt(UniquePtr<Expr>* parsed_lhs) -> UniquePtr<CompoundStmt>
{
    PUSH_TK;

    auto lhs = parse_expr();

    if (lhs == nullptr)
    {
        return nullptr;
    }

    *parsed_lhs = std::move(lhs);

    const auto kind = [&] {
        const auto mod = m_tk->value;

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

        return CompoundStmtKind(-1);
    }();

    if (kind == CompoundStmtKind(-1))
    {
        return nullptr;
    }

    advance();

    auto rhs = parse_expr();

    if (rhs == nullptr)
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

auto Parser::parse_assignment_stmt(UniquePtr<Expr> lhs) -> UniquePtr<AssignmentStmt>
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

    assert(lhs != nullptr);

    if (!consume(TokenType::Equal, false))
    {
        return nullptr;
    }

    auto rhs = parse_expr();

    if (rhs != nullptr)
    {
        throw Error{(m_tk - 1)->location,
                    "expected a right-hand-side expression for the assignment"};
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<AssignmentStmt>(tk_pusher_.initial_tk()->location,
                                            std::move(lhs),
                                            std::move(rhs));
}

auto Parser::parse_return_stmt() -> UniquePtr<ReturnStmt>
{
    // Assume 'return' is already consumed.

    PUSH_TK;

    auto expr = parse_expr();

    if (expr == nullptr)
    {
        return nullptr;
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<ReturnStmt>(tk_pusher_.initial_tk()->location, std::move(expr));
}

auto Parser::parse_for_stmt() -> UniquePtr<ForStmt>
{
    // Assume 'for' is consumed.

    PUSH_TK;

    consume(TokenType::LeftParen, true);

    const auto loop_var_location = m_tk->location;
    const auto loop_var_name     = consume_identifier();

    auto loop_var = std::make_unique<ForLoopVariableDecl>(loop_var_location, loop_var_name);

    consume_keyword(keyword::in, true);

    auto range = parse_range_expr();

    if (range == nullptr)
    {
        throw Error{m_tk->location, "expected a range expression"};
    }

    consume(TokenType::RightParen, true);

    auto body = parse_code_block();

    return std::make_unique<ForStmt>(tk_pusher_.initial_tk()->location,
                                     std::move(loop_var),
                                     std::move(range),
                                     std::move(body));
}

auto Parser::parse_if_stmt(bool is_if) -> UniquePtr<IfStmt>
{
    // Assume 'if' is consumed.

    PUSH_TK;

    auto condition = UniquePtr<Expr>{};

    if (is_if)
    {
        consume(TokenType::LeftParen, true);

        condition = parse_expr();

        if (condition == nullptr)
        {
            throw Error{m_tk->location, "expected a condition expression"};
        }

        consume(TokenType::RightParen, true);
    }

    auto body = parse_code_block();
    auto next = UniquePtr<IfStmt>{};

    if (consume_keyword(keyword::else_, false))
    {
        next = parse_if_stmt(consume_keyword(keyword::if_, false));

        if (next == nullptr)
        {
            throw Error{m_tk->location, "expected a consecutive if-statement"};
        }
    }

    return std::make_unique<IfStmt>(tk_pusher_.initial_tk()->location,
                                    std::move(condition),
                                    std::move(body),
                                    std::move(next));
}

auto Parser::parse_var_stmt() -> UniquePtr<VarStmt>
{
    const auto is_var   = is_keyword(keyword::var);
    const auto is_const = is_keyword(keyword::const_);

    if (!is_var && !is_const)
    {
        return nullptr;
    }

    advance();

    const auto name_location = m_tk->location;
    const auto name          = consume_identifier();

    consume(TokenType::Equal, true);

    auto expr = parse_expr();

    if (expr == nullptr)
    {
        throw Error{m_tk->location, "expected a variable statement expression"};
    }

    consume(TokenType::Semicolon, true);

    return std::make_unique<VarStmt>(
        name_location,
        std::make_unique<VarDecl>(name_location, name, std::move(expr), is_const));
}

auto Parser::parse_range_expr() -> UniquePtr<RangeExpr>
{
    PUSH_TK;

    auto start = parse_expr();

    if (start == nullptr)
    {
        return nullptr;
    }

    consume(TokenType::DotDot, true);

    auto end = parse_expr();

    if (end == nullptr)
    {
        throw Error{m_tk->location, "expected a range-end expression"};
    }

    return std::make_unique<RangeExpr>(tk_pusher_.initial_tk()->location,
                                       std::move(start),
                                       std::move(end));
}

auto Parser::parse_int_literal_expr() -> UniquePtr<IntLiteralExpr>
{
    if (m_tk->is(TokenType::IntLiteral))
    {
        const auto location = m_tk->location;
        auto       value    = 0;

        try
        {
            value = std::stoi(String{m_tk->value});
        }
        catch (const std::exception& ex)
        {
            throw Error{location, "failed to parse integer literal (reason: {})", ex.what()};
        }

        advance();

        return std::make_unique<IntLiteralExpr>(location, value);
    }

    return nullptr;
}

auto Parser::parse_bool_literal_expr() -> UniquePtr<BoolLiteralExpr>
{
    if (is_keyword(keyword::true_) || is_keyword(keyword::false_))
    {
        const auto value    = m_tk->value == keyword::true_;
        const auto location = m_tk->location;

        advance();

        return std::make_unique<BoolLiteralExpr>(location, value);
    }

    return nullptr;
}

auto Parser::parse_float_literal_expr() -> UniquePtr<FloatLiteralExpr>
{
    if (m_tk->is(TokenType::FloatLiteral))
    {
        const auto location     = m_tk->location;
        auto       string_value = std::string_view{};
        auto       value        = 0.0;

        try
        {
            string_value = m_tk->value;
            value        = std::stod(String{string_value});
        }
        catch (const std::exception& ex)
        {
            throw Error{location, "failed to parse float literal (reason: {})", ex.what()};
        }

        advance();

        return std::make_unique<FloatLiteralExpr>(location, string_value, value);
    }

    return nullptr;
}

auto Parser::parse_unary_op_expr() -> UniquePtr<UnaryOpExpr>
{
    PUSH_TK;

    auto op_kind = Option<UnaryOpKind>{};

    if (m_tk->is(TokenType::ExclamationMark))
    {
        op_kind = UnaryOpKind::LogicalNot;
    }
    else if (m_tk->is(TokenType::Hyphen))
    {
        op_kind = UnaryOpKind::Negate;
    }

    if (!op_kind.has_value())
    {
        return nullptr;
    }

    advance();

    auto expr = parse_primary_expr();

    if (expr == nullptr)
    {
        throw Error{m_tk->location, "expected an expression for the unary operation"};
    }

    return std::make_unique<UnaryOpExpr>(tk_pusher_.initial_tk()->location,
                                         *op_kind,
                                         std::move(expr));
}

auto Parser::parse_struct_ctor_arg() -> UniquePtr<StructCtorArg>
{
    PUSH_TK;
    const auto name = consume_identifier();

    consume(TokenType::Equal, true);

    auto expr = parse_expr();

    if (expr == nullptr)
    {
        throw Error{m_tk->location, "expected an expression for struct field '{}'", name};
    }

    return std::make_unique<StructCtorArg>(tk_pusher_.initial_tk()->location,
                                           name,
                                           std::move(expr));
}

auto Parser::parse_sym_access_expr() -> UniquePtr<SymAccessExpr>
{
    if (m_tk->is(TokenType::Identifier))
    {
        const auto name     = m_tk->value;
        const auto location = m_tk->location;

        advance();

        return std::make_unique<SymAccessExpr>(location, name);
    }

    return nullptr;
}

auto Parser::parse_struct_ctor_call(UniquePtr<Expr> callee) -> UniquePtr<StructCtorCall>
{
    PUSH_TK;
    consume(TokenType::LeftBrace, true);

    auto args = UniquePtrList<StructCtorArg, 4>{};

    while (!is_at_end() && !m_tk->is(TokenType::RightBrace))
    {
        auto arg = parse_struct_ctor_arg();

        if (arg == nullptr)
        {
            throw Error{m_tk->location,
                        "expected a struct field argument in the form "
                        "of '<name> = <expression>'"};
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

auto Parser::parse_function_call(UniquePtr<Expr> callee) -> UniquePtr<FunctionCallExpr>
{
    PUSH_TK;
    consume(TokenType::LeftParen, true);

    auto args = UniquePtrList<Expr, 4>{};

    while (!is_at_end() && !m_tk->is(TokenType::RightParen))
    {
        auto arg = parse_expr();

        if (arg == nullptr)
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

auto Parser::parse_scientific_int_literal_expr() -> UniquePtr<ScientificIntLiteralExpr>
{
    if (m_tk->is(TokenType::ScientificNumber))
    {
        const auto location = m_tk->location;
        const auto value    = m_tk->value;

        advance();

        return std::make_unique<ScientificIntLiteralExpr>(location, value);
    }

    return nullptr;
}

auto Parser::parse_hexadecimal_int_literal_expr() -> UniquePtr<HexadecimalIntLiteralExpr>
{
    if (m_tk->is(TokenType::HexNumber))
    {
        const auto location = m_tk->location;
        const auto value    = m_tk->value;

        advance();

        return std::make_unique<HexadecimalIntLiteralExpr>(location, value);
    }

    return nullptr;
}

auto Parser::parse_paren_expr() -> UniquePtr<ParenExpr>
{
    PUSH_TK;

    if (!consume(TokenType::LeftParen, false))
    {
        return nullptr;
    }

    auto expr = parse_expr();

    if (expr == nullptr)
    {
        throw Error{m_tk->location, "expected an expression inside parentheses"};
    }

    consume(TokenType::RightParen, true);

    return std::make_unique<ParenExpr>(tk_pusher_.initial_tk()->location, std::move(expr));
}

auto Parser::parse_ternary_expr(UniquePtr<Expr> condition_expr) -> UniquePtr<TernaryExpr>
{
    assert(condition_expr);

    if (!consume(TokenType::QuestionMark, false))
    {
        return nullptr;
    }

    auto true_expr = parse_expr({}, 0, "true-expression");

    consume(TokenType::Colon, true);

    auto false_expr = parse_expr({}, 0, "false-expression");

    return std::make_unique<TernaryExpr>(condition_expr->location(),
                                         std::move(condition_expr),
                                         std::move(true_expr),
                                         std::move(false_expr));
}

auto Parser::parse_code_block() -> UniquePtr<CodeBlock>
{
    const auto location = m_tk->location;

    consume(TokenType::LeftBrace, true, "expected a code block");

    auto stmts = CodeBlock::StmtsType{};

    while (!is_at_end() && !m_tk->is(TokenType::RightBrace))
    {
        auto stmt = parse_stmt();

        if (stmt == nullptr)
        {
            throw Error{m_tk->location, "expected a statement"};
        }

        stmts.push_back(std::move(stmt));
    }

    consume(TokenType::RightBrace, true);

    return std::make_unique<CodeBlock>(location, std::move(stmts));
}

auto Parser::parse_type() -> const Type&
{
    const auto location       = m_tk->location;
    const auto base_type_name = consume_identifier();

    if (consume(TokenType::LeftBracket, false))
    {
        // Array type
        auto size_expr = parse_expr();

        if (size_expr == nullptr)
        {
            throw Error{m_tk->location, "expected a size expression for the array type"};
        }

        consume(TokenType::RightBracket, true, "expected a ']' that ends the array type");

        return m_type_cache.create_array_type(location, base_type_name, std::move(size_expr));
    }

    return m_type_cache.create_unresolved_type(location, base_type_name);
}

auto Parser::next_tk() const -> const Token&
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
    {
        throw Error(m_tk->location, "expected an identifier");
    }
}

auto Parser::consume_identifier() -> std::string_view
{
    expect_identifier();
    const auto tk = m_tk;
    advance();

    return tk->value;
}

auto Parser::consume_keyword(std::string_view str, bool must_exist) -> bool
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

auto Parser::consume(TokenType type, bool must_exist, std::string_view msg) -> bool
{
    if (!m_tk->is(type))
    {
        if (must_exist)
        {
            const auto error_location =
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
    {
        m_stack.pop_back();
    }
}

auto Parser::TokenPusher::initial_tk() const -> TokenIterator
{
    return m_initial_tk;
}

void Parser::TokenPusher::pop()
{
    m_stack.pop_back();
    m_is_active = false;
}
} // namespace cer::shadercompiler
