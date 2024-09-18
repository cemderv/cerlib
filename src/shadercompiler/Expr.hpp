// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <any>
#include <memory>
#include <span>
#include <vector>

namespace cer::shadercompiler
{
class SemaContext;
class Type;
class Decl;
class Scope;
class BinOpExpr;

class CERLIB_API_INTERNAL Expr
{
  protected:
    explicit Expr(const SourceLocation& location);

    virtual void on_verify(SemaContext& context, Scope& scope) = 0;

    void set_type(const Type& type);

    void set_symbol(const Decl* symbol);

    bool is_verified() const;

  public:
    NON_COPYABLE_NON_MOVABLE(Expr);

    virtual ~Expr() noexcept;

    void verify(SemaContext& context, Scope& scope);

    const SourceLocation& location() const;

    const Type& type() const;

    const Decl* symbol() const;

    virtual std::any evaluate_constant_value(SemaContext& context, Scope& scope) const;

    virtual bool is_literal() const;

    virtual bool accesses_symbol(const Decl& symbol, bool transitive) const;

  private:
    SourceLocation m_location;
    bool           m_is_verified;
    const Type*    m_type{};
    const Decl*    m_symbol{};
};

class CERLIB_API_INTERNAL RangeExpr final : public Expr
{
  public:
    explicit RangeExpr(const SourceLocation& location,
                       std::unique_ptr<Expr> start,
                       std::unique_ptr<Expr> end);

    void on_verify(SemaContext& context, Scope& scope) override;

    const Expr& start() const;

    const Expr& end() const;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

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

class CERLIB_API_INTERNAL BinOpExpr final : public Expr
{
  public:
    explicit BinOpExpr(const SourceLocation& location,
                       BinOpKind             kind,
                       std::unique_ptr<Expr> lhs,
                       std::unique_ptr<Expr> rhs);

    void on_verify(SemaContext& context, Scope& scope) override;

    BinOpKind bin_op_kind() const;

    const Expr& lhs() const;

    const Expr& rhs() const;

    bool is(BinOpKind kind) const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    BinOpKind             m_bin_op_kind;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

class CERLIB_API_INTERNAL IntLiteralExpr final : public Expr
{
  public:
    explicit IntLiteralExpr(const SourceLocation& location, int32_t value);

    void on_verify(SemaContext& context, Scope& scope) override;

    int32_t value() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool is_literal() const override;

  private:
    int32_t m_value;
};

class CERLIB_API_INTERNAL BoolLiteralExpr final : public Expr
{
  public:
    explicit BoolLiteralExpr(const SourceLocation& location, bool value);

    void on_verify(SemaContext& context, Scope& scope) override;

    bool value() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool is_literal() const override;

  private:
    bool m_Value;
};

class CERLIB_API_INTERNAL FloatLiteralExpr final : public Expr
{
  public:
    explicit FloatLiteralExpr(const SourceLocation& location,
                              std::string_view      string_value,
                              double                value);

    void on_verify(SemaContext& context, Scope& scope) override;

    std::string_view string_value() const;

    double value() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool is_literal() const override;

  private:
    std::string_view m_string_value;
    double           m_value;
};

enum class UnaryOpKind
{
    Negate,
    LogicalNot,
};

class CERLIB_API_INTERNAL UnaryOpExpr final : public Expr
{
  public:
    explicit UnaryOpExpr(const SourceLocation& location,
                         UnaryOpKind           kind,
                         std::unique_ptr<Expr> expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    UnaryOpKind unary_op_kind() const;

    const Expr& expr() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    UnaryOpKind           m_kind;
    std::unique_ptr<Expr> m_expr;
};

class CERLIB_API_INTERNAL StructCtorArg final : public Expr
{
  public:
    explicit StructCtorArg(const SourceLocation& location,
                           std::string_view      name,
                           std::unique_ptr<Expr> expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    std::string_view name() const;

    const Expr& expr() const;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::string_view      m_name;
    std::unique_ptr<Expr> m_expr;
};

class CERLIB_API_INTERNAL SymAccessExpr final : public Expr
{
    friend class BinOpExpr;

  public:
    explicit SymAccessExpr(const SourceLocation& location, std::string_view name);

    explicit SymAccessExpr(const SourceLocation& location, Decl& symbol);

    std::string_view name() const;

    void on_verify(SemaContext& context, Scope& scope) override;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::string_view m_name;
    Expr*            m_ancestor_expr;
};

class CERLIB_API_INTERNAL StructCtorCall final : public Expr
{
  public:
    explicit StructCtorCall(const SourceLocation&                          location,
                            std::unique_ptr<Expr>                          callee,
                            SmallVector<std::unique_ptr<StructCtorArg>, 4> args);

    void on_verify(SemaContext& context, Scope& scope) override;

    const Expr& callee() const;

    std::span<const std::unique_ptr<StructCtorArg>> args() const;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::unique_ptr<Expr>                          m_callee;
    SmallVector<std::unique_ptr<StructCtorArg>, 4> m_args;
};

class CERLIB_API_INTERNAL FunctionCallExpr final : public Expr
{
  public:
    explicit FunctionCallExpr(const SourceLocation&                 location,
                              std::unique_ptr<Expr>                 callee,
                              SmallVector<std::unique_ptr<Expr>, 4> args);

    void on_verify(SemaContext& context, Scope& scope) override;

    const Expr& callee() const;

    std::span<const std::unique_ptr<Expr>> args() const;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

  private:
    std::unique_ptr<Expr>                 m_callee;
    SmallVector<std::unique_ptr<Expr>, 4> m_args;
};

class CERLIB_API_INTERNAL SubscriptExpr final : public Expr
{
  public:
    explicit SubscriptExpr(const SourceLocation& location,
                           std::unique_ptr<Expr> expr,
                           std::unique_ptr<Expr> index_expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    const Expr& expr() const;

    const Expr& index_expr() const;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::unique_ptr<Expr> m_expr;
    std::unique_ptr<Expr> m_index_expr;
};

class CERLIB_API_INTERNAL ScientificIntLiteralExpr final : public Expr
{
  public:
    explicit ScientificIntLiteralExpr(const SourceLocation& location, std::string_view value);

    void on_verify(SemaContext& context, Scope& scope) override;

    std::string_view value() const;

  private:
    std::string_view m_value;
};

class CERLIB_API_INTERNAL HexadecimalIntLiteralExpr final : public Expr
{
  public:
    explicit HexadecimalIntLiteralExpr(const SourceLocation& location, std::string_view value);

    void on_verify(SemaContext& context, Scope& scope) override;

    std::string_view value() const;

  private:
    std::string_view m_value;
};

class CERLIB_API_INTERNAL ParenExpr final : public Expr
{
  public:
    explicit ParenExpr(const SourceLocation& location, std::unique_ptr<Expr> expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    const Expr& expr() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::unique_ptr<Expr> m_expr;
};

class CERLIB_API_INTERNAL TernaryExpr final : public Expr
{
  public:
    explicit TernaryExpr(const SourceLocation& location,
                         std::unique_ptr<Expr> condition_expr,
                         std::unique_ptr<Expr> true_expr,
                         std::unique_ptr<Expr> false_expr);

    void on_verify(SemaContext& context, Scope& scope) override;

    const Expr& condition_expr() const;

    const Expr& true_expr() const;

    const Expr& false_expr() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::unique_ptr<Expr> m_condition_expr;
    std::unique_ptr<Expr> m_true_expr;
    std::unique_ptr<Expr> m_false_expr;
};

class CERLIB_API_INTERNAL ArrayExpr final : public Expr
{
  public:
    explicit ArrayExpr(const SourceLocation& location, std::vector<std::unique_ptr<Expr>> elements);

    void on_verify(SemaContext& context, Scope& scope) override;

    std::span<const std::unique_ptr<Expr>> elements() const;

    std::any evaluate_constant_value(SemaContext& context, Scope& scope) const override;

    bool accesses_symbol(const Decl& symbol, bool transitive) const override;

  private:
    std::vector<std::unique_ptr<Expr>> m_elements;
};
} // namespace cer::shadercompiler
