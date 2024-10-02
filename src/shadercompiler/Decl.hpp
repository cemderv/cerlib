// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include "shadercompiler/Type.hpp"
#include "util/InternalExport.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <any>
#include <span>
#include <string>

namespace cer::shadercompiler
{
class Expr;
class SemaContext;
class CodeBlock;
class FunctionDecl;
class StructDecl;
class ForStmt;

class Decl
{
  protected:
    explicit Decl(const SourceLocation& location, std::string_view name);

    virtual auto on_verify(SemaContext& context, Scope& scope) -> void = 0;

    auto set_type(const Type& type) -> void
    {
        m_type = &type;
    }

  public:
    NON_COPYABLE_NON_MOVABLE(Decl);

    virtual ~Decl() noexcept;

    void verify(SemaContext& context, Scope& scope);

    bool is_verified() const;

    const SourceLocation& location() const;

    std::string_view name() const;

    const Type& type() const;

  private:
    SourceLocation m_location;
    bool           m_is_verified;
    std::string    m_name;
    const Type*    m_type;
};

class StructFieldDecl final : public Decl
{
    friend class StructDecl;

  public:
    explicit StructFieldDecl(const SourceLocation& location,
                             std::string_view      name,
                             const Type&           type);

    NON_COPYABLE_NON_MOVABLE(StructFieldDecl);

    ~StructFieldDecl() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;
};

class StructDecl final : public Decl, public Type
{
  public:
    using FieldList = SmallVector<std::unique_ptr<StructFieldDecl>, 8>;

    explicit StructDecl(const SourceLocation& location,
                        std::string_view      name,
                        FieldList             fields,
                        bool                  is_built_in);

    NON_COPYABLE_NON_MOVABLE(StructDecl);

    ~StructDecl() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    std::string_view type_name() const override;

    const Type& resolve(SemaContext& context, Scope& scope) const override;

    std::span<const std::unique_ptr<StructFieldDecl>> get_fields() const;

    StructFieldDecl* find_field(std::string_view name) const;

    bool has_field(std::string_view name) const;

    Decl* find_member_symbol(const SemaContext& context, std::string_view name) const override;

    FunctionDecl* ctor() const;

    bool is_built_in() const;

  private:
    FieldList                     m_fields;
    std::unique_ptr<FunctionDecl> m_ctor;
    bool                          m_is_built_in{};
};

enum class FunctionParamKind
{
    Normal,
    ShaderStageInput,
};

class FunctionParamDecl final : public Decl
{
    friend class FunctionDecl;

  public:
    explicit FunctionParamDecl(const SourceLocation& location,
                               std::string_view      name,
                               const Type&           type);

    explicit FunctionParamDecl(const SourceLocation& location,
                               std::string_view      name,
                               FunctionParamKind     kind,
                               const Type&           type);

    NON_COPYABLE_NON_MOVABLE(FunctionParamDecl);

    ~FunctionParamDecl() noexcept override;

    FunctionParamKind kind() const;

    void on_verify(SemaContext& context, Scope& scope) override;

  private:
    FunctionParamKind m_kind;
};

enum class FunctionKind
{
    Normal,
    Shader,
};

class ForLoopVariableDecl final : public Decl
{
    friend ForStmt;

  public:
    explicit ForLoopVariableDecl(const SourceLocation& location, std::string_view name);

    NON_COPYABLE_NON_MOVABLE(ForLoopVariableDecl);

    void on_verify(SemaContext& context, Scope& scope) override;

  private:
    void set_var_type(const Type& type)
    {
        set_type(type);
    }
};

class FunctionDecl final : public Decl
{
  public:
    explicit FunctionDecl(const SourceLocation&                              location,
                          std::string_view                                   name,
                          SmallVector<std::unique_ptr<FunctionParamDecl>, 4> parameters,
                          const Type&                                        return_type,
                          std::unique_ptr<CodeBlock>                         body,
                          bool is_struct_ctor = false);

    NON_COPYABLE_NON_MOVABLE(FunctionDecl);

    ~FunctionDecl() noexcept override;

    std::span<const std::unique_ptr<FunctionParamDecl>> parameters() const;

    bool accesses_symbol(const Decl& symbol, bool transitive) const;

    CodeBlock* body();

    const CodeBlock* body() const;

    FunctionKind kind() const;

    bool is(FunctionKind kind) const;

    bool is_normal_function() const;

    bool is_shader() const;

    void on_verify(SemaContext& context, Scope& scope) override;

    FunctionParamDecl* find_parameter(std::string_view name) const;

    bool is_struct_ctor() const;

  private:
    FunctionKind                                       m_kind;
    SmallVector<std::unique_ptr<FunctionParamDecl>, 4> m_parameters;
    std::unique_ptr<CodeBlock>                         m_body;
    bool                                               m_is_struct_ctor;
};

/**
 * \brief Represents a global shader parameter declaration.
 */
class ShaderParamDecl final : public Decl
{
  public:
    explicit ShaderParamDecl(const SourceLocation& location,
                             std::string_view      name,
                             const Type&           type,
                             std::unique_ptr<Expr> default_value_expr);

    NON_COPYABLE_NON_MOVABLE(ShaderParamDecl);

    ~ShaderParamDecl() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    bool is_array() const;

    uint16_t array_size() const;

    const Expr* default_value_expr() const;

    const std::any& default_value() const;

  private:
    std::unique_ptr<Expr> m_default_value_expr;
    std::any              m_default_value;
};

class VarDecl final : public Decl
{
  public:
    explicit VarDecl(const SourceLocation& location,
                     std::string_view      name,
                     std::unique_ptr<Expr> expr,
                     bool                  is_const);

    // Overload for system values
    explicit VarDecl(std::string_view name, const Type& type);

    NON_COPYABLE_NON_MOVABLE(VarDecl);

    ~VarDecl() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    bool is_const() const;

    bool is_system_value() const;

    const Expr& expr() const;

  private:
    bool                  m_is_const;
    std::unique_ptr<Expr> m_expr;
    bool                  m_is_system_value{};
};
} // namespace cer::shadercompiler
