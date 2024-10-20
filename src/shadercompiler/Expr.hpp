// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include <any>
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
#include <span>
#include <vector>

namespace cer::shadercompiler
{
class SemaContext;
class Type;
class Decl;
class Scope;
class BinOpExpr;

class Expr
{
  protected:
    explicit Expr(const SourceLocation& location);

    virtual void on_verify(SemaContext& context, Scope& scope) = 0;

    void set_type(const Type& type);

    void set_symbol(const Decl* symbol);

    auto is_verified() const -> bool;

  public:
    forbid_copy_and_move(Expr);

    virtual ~Expr() noexcept;

    void verify(SemaContext& context, Scope& scope);

    auto location() const -> const SourceLocation&;

    auto type() const -> const Type&;

    auto symbol() const -> const Decl*;

    virtual auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any;

    virtual auto is_literal() const -> bool;

    virtual auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool;

  private:
    SourceLocation m_location;
    bool           m_is_verified;
    const Type*    m_type{};
    const Decl*    m_symbol{};
};

class RangeExpr final : public Expr
{
  public:
    explicit RangeExpr(const SourceLocation& location,
                       std::unique_ptr<Expr> start,
                       std::unique_ptr<Expr> end);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto start() const -> const Expr&;

    auto end() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_start;
    std::unique_ptr<Expr> m_end; // exclusive
};

enum class BinOpKind
{
    Add,
    Subtract,
    Multiply,
    Divide,
    LogicalAnd,
    LogicalOr,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    MemberAccess,
    BitwiseXor,
    BitwiseAnd,
    Equal,
    NotEqual,
    RightShift,
    BitwiseOr,
    LeftShift,
};

class BinOpExpr final : public Expr
{
  public:
    explicit BinOpExpr(const SourceLocation& location,
                       BinOpKind             kind,
                       std::unique_ptr<Expr> lhs,
                       std::unique_ptr<Expr> rhs);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto bin_op_kind() const -> BinOpKind;

    auto lhs() const -> const Expr&;

    auto rhs() const -> const Expr&;

    auto is(BinOpKind kind) const -> bool;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    BinOpKind             m_bin_op_kind;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class IntLiteralExpr final : public Expr
{
  public:
    explicit IntLiteralExpr(const SourceLocation& location, int32_t value);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto value() const -> int32_t;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto is_literal() const -> bool override;

  private:
    int32_t m_value;
};

class BoolLiteralExpr final : public Expr
{
  public:
    explicit BoolLiteralExpr(const SourceLocation& location, bool value);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto value() const -> bool;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto is_literal() const -> bool override;

  private:
    bool m_value;
};

class FloatLiteralExpr final : public Expr
{
  public:
    explicit FloatLiteralExpr(const SourceLocation& location,
                              std::string_view      string_value,
                              double                value);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto string_value() const -> std::string_view;

    auto value() const -> double;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto is_literal() const -> bool override;

  private:
    std::string_view m_string_value;
    double           m_value;
};

enum class UnaryOpKind
{
    Negate,
    LogicalNot,
};

class UnaryOpExpr final : public Expr
{
  public:
    explicit UnaryOpExpr(const SourceLocation& location,
                         UnaryOpKind           kind,
                         std::unique_ptr<Expr> expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto unary_op_kind() const -> UnaryOpKind;

    auto expr() const -> const Expr&;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    UnaryOpKind           m_kind;
    std::unique_ptr<Expr> m_expr;
};

class StructCtorArg final : public Expr
{
  public:
    explicit StructCtorArg(const SourceLocation& location,
                           std::string_view      name,
                           std::unique_ptr<Expr> expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto name() const -> std::string_view;

    auto expr() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::string_view      m_name;
    std::unique_ptr<Expr> m_expr;
};

class SymAccessExpr final : public Expr
{
    friend class BinOpExpr;

  public:
    explicit SymAccessExpr(const SourceLocation& location, std::string_view name);

    explicit SymAccessExpr(const SourceLocation& location, Decl& symbol);

    auto name() const -> std::string_view;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::string_view m_name;
    Expr*            m_ancestor_expr;
};

class StructCtorCall final : public Expr
{
  public:
    explicit StructCtorCall(const SourceLocation&                     location,
                            std::unique_ptr<Expr>                     callee,
                            small_vector_of_uniques<StructCtorArg, 4> args);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto callee() const -> const Expr&;

    auto args() const -> std::span<const std::unique_ptr<StructCtorArg>>;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr>                     m_callee;
    small_vector_of_uniques<StructCtorArg, 4> m_args;
};

class FunctionCallExpr final : public Expr
{
  public:
    explicit FunctionCallExpr(const SourceLocation&            location,
                              std::unique_ptr<Expr>            callee,
                              small_vector_of_uniques<Expr, 4> args);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto callee() const -> const Expr&;

    auto args() const -> std::span<const std::unique_ptr<Expr>>;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

  private:
    std::unique_ptr<Expr>            m_callee;
    small_vector_of_uniques<Expr, 4> m_args;
};

class SubscriptExpr final : public Expr
{
  public:
    explicit SubscriptExpr(const SourceLocation& location,
                           std::unique_ptr<Expr> expr,
                           std::unique_ptr<Expr> index_expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto expr() const -> const Expr&;

    auto index_expr() const -> const Expr&;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_expr;
    std::unique_ptr<Expr> m_index_expr;
};

class ScientificIntLiteralExpr final : public Expr
{
  public:
    explicit ScientificIntLiteralExpr(const SourceLocation& location, std::string_view value);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto value() const -> std::string_view;

  private:
    std::string_view m_value;
};

class HexadecimalIntLiteralExpr final : public Expr
{
  public:
    explicit HexadecimalIntLiteralExpr(const SourceLocation& location, std::string_view value);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto value() const -> std::string_view;

  private:
    std::string_view m_value;
};

class ParenExpr final : public Expr
{
  public:
    explicit ParenExpr(const SourceLocation& location, std::unique_ptr<Expr> expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto expr() const -> const Expr&;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_expr;
};

class TernaryExpr final : public Expr
{
  public:
    explicit TernaryExpr(const SourceLocation& location,
                         std::unique_ptr<Expr> condition_expr,
                         std::unique_ptr<Expr> true_expr,
                         std::unique_ptr<Expr> false_expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto condition_expr() const -> const Expr&;

    auto true_expr() const -> const Expr&;

    auto false_expr() const -> const Expr&;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::unique_ptr<Expr> m_condition_expr;
    std::unique_ptr<Expr> m_true_expr;
    std::unique_ptr<Expr> m_false_expr;
};

class ArrayExpr final : public Expr
{
  public:
    explicit ArrayExpr(const SourceLocation& location, std::vector<std::unique_ptr<Expr>> elements);

    void on_verify(SemaContext& context, Scope& scope) override;

    auto elements() const -> std::span<const std::unique_ptr<Expr>>;

    auto evaluate_constant_value(SemaContext& context, Scope& scope) const -> std::any override;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool override;

  private:
    std::vector<std::unique_ptr<Expr>> m_elements;
};
} // namespace cer::shadercompiler
