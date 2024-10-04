// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/Parser.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/SemaContext.hpp"
#include "shadercompiler/Stmt.hpp"
#include "shadercompiler/TypeCache.hpp"
#include <snitch/snitch.hpp>

using cer::shadercompiler::ArrayType;
using cer::shadercompiler::asa;
using cer::shadercompiler::BinOpExpr;
using cer::shadercompiler::BinOpKind;
using cer::shadercompiler::Decl;
using cer::shadercompiler::Expr;
using cer::shadercompiler::FloatLiteralExpr;
using cer::shadercompiler::FloatType;
using cer::shadercompiler::FunctionCallExpr;
using cer::shadercompiler::FunctionDecl;
using cer::shadercompiler::IfStmt;
using cer::shadercompiler::IntLiteralExpr;
using cer::shadercompiler::isa;
using cer::shadercompiler::Parser;
using cer::shadercompiler::ReturnStmt;
using cer::shadercompiler::ShaderParamDecl;
using cer::shadercompiler::Stmt;
using cer::shadercompiler::StructCtorCall;
using cer::shadercompiler::StructDecl;
using cer::shadercompiler::SymAccessExpr;
using cer::shadercompiler::Token;
using cer::shadercompiler::TypeCache;
using cer::shadercompiler::VarStmt;
using cer::shadercompiler::Vector4Type;

static constexpr auto basic_expressions = std::string_view{R"(
float Test()
{
  const x = 1+2*3;
  const x = 1*2+3;
  const x = (1+2)*3;
  const x = !x+2;
  const x = x+!y+2;
  const x = x+!(y+2);
  const x = b * (5.0 + 2.0 / 4.0);
  const x = y[2];
  const x = y[2+3] + 4;

  const x = 1 < 2;
  const x = 1 > 2;
  const x = 1 <= 2;
  const x = 1 >= 2;
  const x = 1 - abs(2, 3) <= y;
}
)"};

static constexpr auto simple_if_stmt = std::string_view{R"(
float test()
{
  if (1 > 2)
  {
    return 3.0;
  }
}
)"};

static constexpr auto simple_if_stmt2 = std::string_view{R"(
float test()
{
  if (1.0 - abs(2.0) <= epsilon)
  {
    return 3.0;
  }
}
)"};

static constexpr auto simple_shader_code = std::string_view{R"(
float Value1;
int[32] Value2_;

struct InputVertex
{
  Vector4 position;
  Vector4 color;
}

struct OutputVertex
{
  Vector4 cer_position;
}

// Some shader
OutputVertex vs_main(InputVertex input)
{
  const a = 1.0 + 2.0;
  const b = a + 3.0;
  const c = b * (5.0 + 2.0 / Value1);

  return OutputVertex
  {
    cer_position = input.position * Vector4(1.0, a, c, Value1)
  };
}
)"};

TEST_CASE("Shader parser", "[shaderc]")
{
    SECTION("Simple expressions")
    {
        auto type_cache = TypeCache{};
        auto parser     = Parser{type_cache};

        std::vector<Token> tokens;
        do_lexing(basic_expressions, "SomeFile", true, tokens);

        const inplace_vector<std::unique_ptr<Decl>, 8> decls = parser.parse(tokens);

        REQUIRE(decls.size() == 1u);
        REQUIRE(isa<FunctionDecl>(*decls.front()));
        FunctionDecl* const func = asa<FunctionDecl>(decls.front().get());
        const inplace_vector<std::unique_ptr<Stmt>, 16>& stmts = func->body()->stmts();
        REQUIRE(stmts.size() == 14u);

        // 1 + 2 * 3
        {
            REQUIRE(isa<VarStmt>(*stmts[0]));
            const Expr& expr = asa<VarStmt>(stmts[0].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));
            const BinOpExpr* const bin_op_expr1 = asa<BinOpExpr>(&expr);
            REQUIRE(bin_op_expr1->bin_op_kind() == BinOpKind::Add);
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr1->lhs()));
            REQUIRE(isa<BinOpExpr>(bin_op_expr1->rhs()));

            const IntLiteralExpr* const one = asa<IntLiteralExpr>(&bin_op_expr1->lhs());
            REQUIRE(one->value() == 1);

            const BinOpExpr* const bin_op_expr2 = asa<BinOpExpr>(&bin_op_expr1->rhs());
            REQUIRE(bin_op_expr2->bin_op_kind() == BinOpKind::Multiply);

            REQUIRE(isa<IntLiteralExpr>(bin_op_expr2->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr2->rhs()));

            const IntLiteralExpr* const two   = asa<IntLiteralExpr>(&bin_op_expr2->lhs());
            const IntLiteralExpr* const three = asa<IntLiteralExpr>(&bin_op_expr2->rhs());

            REQUIRE(two->value() == 2);
            REQUIRE(three->value() == 3);
        }

        // 1 * 2 + 3
        {
            REQUIRE(isa<VarStmt>(*stmts[1]));
            const Expr& expr = asa<VarStmt>(stmts[1].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));
            const BinOpExpr* const bin_op_expr1 = asa<BinOpExpr>(&expr);
            REQUIRE(bin_op_expr1->bin_op_kind() == BinOpKind::Add);
            REQUIRE(isa<BinOpExpr>(bin_op_expr1->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr1->rhs()));

            const BinOpExpr* const bin_op_expr2 = asa<BinOpExpr>(&bin_op_expr1->lhs());
            REQUIRE(bin_op_expr2->bin_op_kind() == BinOpKind::Multiply);

            const IntLiteralExpr* const three = asa<IntLiteralExpr>(&bin_op_expr1->rhs());
            REQUIRE(three->value() == 3);

            REQUIRE(isa<IntLiteralExpr>(bin_op_expr2->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr2->rhs()));

            const IntLiteralExpr* const one = asa<IntLiteralExpr>(&bin_op_expr2->lhs());
            const IntLiteralExpr* const two = asa<IntLiteralExpr>(&bin_op_expr2->rhs());

            REQUIRE(one->value() == 1);
            REQUIRE(two->value() == 2);
        }

        // 1 < 2
        {
            REQUIRE(isa<VarStmt>(*stmts[9]));
            const Expr& expr = asa<VarStmt>(stmts[9].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));
            const BinOpExpr* const bin_op_expr = asa<BinOpExpr>(&expr);
            REQUIRE(bin_op_expr->bin_op_kind() == BinOpKind::LessThan);
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->rhs()));
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->lhs())->value() == 1);
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->rhs())->value() == 2);
        }

        // 1 > 2
        {
            REQUIRE(isa<VarStmt>(*stmts[10]));
            const Expr& expr = asa<VarStmt>(stmts[10].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));
            const BinOpExpr* const bin_op_expr = asa<BinOpExpr>(&expr);
            REQUIRE(bin_op_expr->bin_op_kind() == BinOpKind::GreaterThan);
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->rhs()));
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->lhs())->value() == 1);
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->rhs())->value() == 2);
        }

        // 1 <= 2
        {
            REQUIRE(isa<VarStmt>(*stmts[11]));
            const Expr& expr = asa<VarStmt>(stmts[11].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));
            const BinOpExpr* const bin_op_expr = asa<BinOpExpr>(&expr);
            REQUIRE(bin_op_expr->bin_op_kind() == BinOpKind::LessThanOrEqual);
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->rhs()));
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->lhs())->value() == 1);
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->rhs())->value() == 2);
        }

        // 1 >= 2
        {
            REQUIRE(isa<VarStmt>(*stmts[12]));
            const Expr& expr = asa<VarStmt>(stmts[12].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));
            const BinOpExpr* const bin_op_expr = asa<BinOpExpr>(&expr);
            REQUIRE(bin_op_expr->bin_op_kind() == BinOpKind::GreaterThanOrEqual);
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->lhs()));
            REQUIRE(isa<IntLiteralExpr>(bin_op_expr->rhs()));
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->lhs())->value() == 1);
            REQUIRE(asa<IntLiteralExpr>(&bin_op_expr->rhs())->value() == 2);
        }

        // 1 - abs(2, 3) <= y
        {
            REQUIRE(isa<VarStmt>(*stmts[13]));
            const Expr& expr = asa<VarStmt>(stmts[13].get())->variable().expr();

            REQUIRE(isa<BinOpExpr>(expr));

            const BinOpExpr* const bin_op_expr1 = asa<BinOpExpr>(&expr);

            REQUIRE(bin_op_expr1->bin_op_kind() == BinOpKind::LessThanOrEqual);

            REQUIRE(isa<BinOpExpr>(bin_op_expr1->lhs()));
            REQUIRE(isa<SymAccessExpr>(bin_op_expr1->rhs()));

            // verify lhs
            {
                const BinOpExpr* const lhs_bin_op_expr = asa<BinOpExpr>(&bin_op_expr1->lhs());

                REQUIRE(isa<IntLiteralExpr>(lhs_bin_op_expr->lhs()));
                REQUIRE(isa<FunctionCallExpr>(lhs_bin_op_expr->rhs()));

                REQUIRE(asa<IntLiteralExpr>(&lhs_bin_op_expr->lhs())->value() == 1);

                const FunctionCallExpr* const func_call =
                    asa<FunctionCallExpr>(&lhs_bin_op_expr->rhs());

                REQUIRE(func_call->args().size() == 2u);
                REQUIRE(isa<IntLiteralExpr>(func_call->args()[0].get()));
                REQUIRE(isa<IntLiteralExpr>(func_call->args()[1].get()));
                REQUIRE(asa<IntLiteralExpr>(func_call->args()[0].get())->value() == 2);
                REQUIRE(asa<IntLiteralExpr>(func_call->args()[1].get())->value() == 3);
            }

            // verify rhs
            {
                const SymAccessExpr* const rhs_sym_access_expr =
                    asa<SymAccessExpr>(&bin_op_expr1->rhs());

                REQUIRE(rhs_sym_access_expr->name() == "y");
            }
        }
    }

    SECTION("Simple if stmt")
    {
        auto type_cache = TypeCache();
        auto parser     = Parser(type_cache);

        auto tokens = std::vector<Token>();
        do_lexing(simple_if_stmt, "SomeFile", true, tokens);

        const auto decls = parser.parse(tokens);

        REQUIRE(decls.size() == 1u);
        REQUIRE(isa<FunctionDecl>(*decls[0].get()));
        const auto  func  = asa<FunctionDecl>(decls.front().get());
        const auto& stmts = func->body()->stmts();
        REQUIRE(stmts.size() == 1u);

        REQUIRE(isa<IfStmt>(*stmts[0]));
        const auto if_stmt = asa<IfStmt>(stmts[0].get());

        REQUIRE(if_stmt->condition_expr());
        REQUIRE(!if_stmt->next());

        // to check: if (1 > 2)
        REQUIRE(isa<BinOpExpr>(if_stmt->condition_expr()));
        const auto bin_op = asa<BinOpExpr>(if_stmt->condition_expr());

        REQUIRE(bin_op->bin_op_kind() == BinOpKind::GreaterThan);

        REQUIRE(isa<IntLiteralExpr>(bin_op->lhs()));
        REQUIRE(isa<IntLiteralExpr>(bin_op->rhs()));

        REQUIRE(asa<IntLiteralExpr>(&bin_op->lhs())->value() == 1);
        REQUIRE(asa<IntLiteralExpr>(&bin_op->rhs())->value() == 2);

        const auto& if_stmt_body_stmts = if_stmt->body().stmts();
        REQUIRE(if_stmt_body_stmts.size() == 1u);
        REQUIRE(isa<ReturnStmt>(if_stmt_body_stmts[0].get()));
        const auto return_stmt = asa<ReturnStmt>(if_stmt_body_stmts[0].get());

        REQUIRE(isa<FloatLiteralExpr>(&return_stmt->expr()));
        REQUIRE(asa<FloatLiteralExpr>(&return_stmt->expr())->value() == 3.0f);
    }

    SECTION("Simple if stmt 2")
    {
        auto type_cache = TypeCache();
        auto parser     = Parser(type_cache);

        auto tokens = std::vector<Token>();
        do_lexing(simple_if_stmt2, "SomeFile", true, tokens);

        const auto decls = parser.parse(tokens);

        REQUIRE(decls.size() == 1u);
        REQUIRE(isa<FunctionDecl>(decls[0].get()));
        const auto  func  = asa<FunctionDecl>(decls.front().get());
        const auto& stmts = func->body()->stmts();
        REQUIRE(stmts.size() == 1u);

        REQUIRE(isa<IfStmt>(*stmts[0]));
        const auto if_stmt = asa<IfStmt>(stmts[0].get());

        REQUIRE(if_stmt->condition_expr());
        REQUIRE(!if_stmt->next());

        // to check: if (1.0 - abs(2.0) <= epsilon)
        REQUIRE(isa<BinOpExpr>(if_stmt->condition_expr()));
        const auto binOp = asa<BinOpExpr>(if_stmt->condition_expr());

        REQUIRE(binOp->bin_op_kind() == BinOpKind::LessThanOrEqual);

        REQUIRE(isa<BinOpExpr>(binOp->lhs()));
        REQUIRE(isa<SymAccessExpr>(binOp->rhs()));

        // verify lhs
        {
            const auto lhs_bin_op_expr = asa<BinOpExpr>(&binOp->lhs());
            REQUIRE(isa<FloatLiteralExpr>(lhs_bin_op_expr->lhs()));
            REQUIRE(isa<FunctionCallExpr>(lhs_bin_op_expr->rhs()));

            REQUIRE(asa<FloatLiteralExpr>(&lhs_bin_op_expr->lhs())->value() == 1.0f);

            const auto func_call = asa<FunctionCallExpr>(&lhs_bin_op_expr->rhs());
            REQUIRE(func_call->args().size() == 1u);
            REQUIRE(isa<FloatLiteralExpr>(func_call->args()[0].get()));
            REQUIRE(asa<FloatLiteralExpr>(func_call->args()[0].get())->value() == 2.0f);
        }

        // verify rhs
        {
            const auto rhs_sym_access_expr = asa<SymAccessExpr>(&binOp->rhs());
            REQUIRE(rhs_sym_access_expr->name() == "epsilon");
        }

        const auto& if_stmt_body_stmts = if_stmt->body().stmts();
        REQUIRE(if_stmt_body_stmts.size() == 1u);
        REQUIRE(isa<ReturnStmt>(if_stmt_body_stmts[0].get()));
        const auto return_stmt = asa<ReturnStmt>(if_stmt_body_stmts[0].get());

        REQUIRE(isa<FloatLiteralExpr>(return_stmt->expr()));
        REQUIRE(asa<FloatLiteralExpr>(&return_stmt->expr())->value() == 3.0f);
    }

    SECTION("Simple function call")
    {
        // TODO
    }

    SECTION("Simple shader")
    {
        auto type_cache = TypeCache();
        auto parser     = Parser(type_cache);

        auto tokens = std::vector<Token>();
        do_lexing(simple_shader_code, "SomeFile", true, tokens);

        const auto decls = parser.parse(tokens);

        REQUIRE(decls.size() == 5u);
        REQUIRE(isa<ShaderParamDecl>(*decls[0]));
        REQUIRE(isa<ShaderParamDecl>(*decls[1]));
        REQUIRE(isa<StructDecl>(*decls[2]));
        REQUIRE(isa<StructDecl>(*decls[3]));
        REQUIRE(isa<FunctionDecl>(*decls[4]));

        {
            const auto param = asa<ShaderParamDecl>(decls[0].get());
            REQUIRE(param->name() == "Value1");
            REQUIRE(param->type().type_name() == FloatType::instance().type_name());
        }

        {
            const auto param = asa<ShaderParamDecl>(decls[1].get());
            REQUIRE(param->name() == "Value2_");
            REQUIRE(isa<ArrayType>(&param->type()));

            const auto array_type = asa<ArrayType>(&param->type());
            REQUIRE(isa<IntLiteralExpr>(array_type->size_expr()));

#if 0 // TODO: should move to separate SemaContext test
      const auto size = asa<IntLiteralExpr>(&arrayType->GetSizeExpr())->EvaluateConstantValue();
      REQUIRE(size.has_value());
      REQUIRE(std::any_cast<int32_t>(&size));
      REQUIRE(std::any_cast<int32_t>(*size) == 32);
#endif
        }

        {
            const auto strct = asa<StructDecl>(decls[2].get());
            REQUIRE(strct->name() == "InputVertex");

            const auto fields = strct->get_fields();
            REQUIRE(fields.size() == 2u);
            REQUIRE(fields[0]->name() == "position");
            REQUIRE(fields[0]->type().type_name() == Vector4Type::instance().type_name());
            REQUIRE(fields[1]->name() == "color");
            REQUIRE(fields[1]->type().type_name() == Vector4Type::instance().type_name());
        }

        {
            const auto strct = asa<StructDecl>(decls[3].get());
            REQUIRE(strct->name() == "OutputVertex");

            const auto fields = strct->get_fields();
            REQUIRE(fields.size() == 1u);
            REQUIRE(fields[0]->name() == "cer_position");
            REQUIRE(fields[0]->type().type_name() == Vector4Type::instance().type_name());
        }

        {
            const auto function = asa<FunctionDecl>(decls[4].get());
            REQUIRE(function->name() == "vs_main");

            const auto params = function->parameters();
            REQUIRE(params.size() == 1u);
            REQUIRE(params[0]->name() == "input");
            REQUIRE(params[0]->type().type_name() == "InputVertex");

            REQUIRE(function->type().type_name() == "OutputVertex");

            const auto& stmts = function->body()->stmts();
            REQUIRE(stmts.size() == 4u);
            REQUIRE(isa<VarStmt>(*stmts[0]));
            REQUIRE(isa<VarStmt>(*stmts[1]));
            REQUIRE(isa<VarStmt>(*stmts[2]));
            REQUIRE(isa<ReturnStmt>(*stmts[3]));

            const auto  return_stmt = asa<ReturnStmt>(stmts[3].get());
            const auto& expr        = return_stmt->expr();

            REQUIRE(isa<StructCtorCall>(expr));
            const auto call = asa<StructCtorCall>(&expr);

            REQUIRE(call->args().size() == 1u);
            const auto arg = call->args()[0].get();
            REQUIRE(arg->name() == "cer_position");

            REQUIRE(isa<BinOpExpr>(arg->expr()));
            const auto bin_op_expr = asa<BinOpExpr>(&arg->expr());

            REQUIRE(bin_op_expr->bin_op_kind() == BinOpKind::Multiply);
            REQUIRE(isa<BinOpExpr>(bin_op_expr->lhs()));
            REQUIRE(isa<FunctionCallExpr>(bin_op_expr->rhs()));

            {
                const auto bin_op_expr2 = asa<BinOpExpr>(&bin_op_expr->lhs());
                REQUIRE(bin_op_expr2->bin_op_kind() == BinOpKind::MemberAccess);
                const auto sym1 = asa<SymAccessExpr>(&bin_op_expr2->lhs());
                REQUIRE(sym1->name() == "input");
                const auto sym2 = asa<SymAccessExpr>(&bin_op_expr2->rhs());
                REQUIRE(sym2->name() == "position");
            }

            {
                const auto  func_call = asa<FunctionCallExpr>(&bin_op_expr->rhs());
                const auto& callee    = func_call->callee();
                REQUIRE(isa<SymAccessExpr>(callee));
                REQUIRE(asa<SymAccessExpr>(&callee)->name() == "Vector4");

                const auto args = func_call->args();
                REQUIRE(args.size() == 4u);
                REQUIRE(isa<FloatLiteralExpr>(*args[0]));
                REQUIRE(asa<FloatLiteralExpr>(args[0].get())->value() == 1.0f);

                REQUIRE(isa<SymAccessExpr>(*args[1]));
                REQUIRE(asa<SymAccessExpr>(args[1].get())->name() == "a");

                REQUIRE(isa<SymAccessExpr>(*args[2]));
                REQUIRE(asa<SymAccessExpr>(args[2].get())->name() == "c");

                REQUIRE(isa<SymAccessExpr>(*args[3]));
                REQUIRE(asa<SymAccessExpr>(args[3].get())->name() == "Value1");
            }
        }
    }

    SECTION("Handle erroneous inputs")
    {
        // TODO
    }
}
