// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/SourceLocation.hpp"
#include "shadercompiler/Type.hpp"
#include "util/NonCopyable.hpp"
#include "util/small_vector.hpp"
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

    auto is_verified() const -> bool;

    auto location() const -> const SourceLocation&;

    auto name() const -> std::string_view;

    auto type() const -> const Type&;

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
    using FieldList = gch::small_vector<std::unique_ptr<StructFieldDecl>, 8>;

    explicit StructDecl(const SourceLocation& location,
                        std::string_view      name,
                        FieldList             fields,
                        bool                  is_built_in);

    NON_COPYABLE_NON_MOVABLE(StructDecl);

    ~StructDecl() noexcept override;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto type_name() const -> std::string_view override;

    auto resolve(SemaContext& context, Scope& scope) const -> const Type& override;

    auto get_fields() const -> std::span<const std::unique_ptr<StructFieldDecl>>;

    auto find_field(std::string_view name) const -> StructFieldDecl*;

    auto has_field(std::string_view name) const -> bool;

    auto find_member_symbol(const SemaContext& context, std::string_view name) const
        -> Decl* override;

    auto ctor() const -> FunctionDecl*;

    auto is_built_in() const -> bool;

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

    auto kind() const -> FunctionParamKind;

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
    explicit FunctionDecl(const SourceLocation&                                    location,
                          std::string_view                                         name,
                          gch::small_vector<std::unique_ptr<FunctionParamDecl>, 4> parameters,
                          const Type&                                              return_type,
                          std::unique_ptr<CodeBlock>                               body,
                          bool is_struct_ctor = false);

    NON_COPYABLE_NON_MOVABLE(FunctionDecl);

    ~FunctionDecl() noexcept override;

    auto parameters() const -> std::span<const std::unique_ptr<FunctionParamDecl>>;

    auto accesses_symbol(const Decl& symbol, bool transitive) const -> bool;

    auto body() -> CodeBlock*;

    auto body() const -> const CodeBlock*;

    auto kind() const -> FunctionKind;

    auto is(FunctionKind kind) const -> bool;

    auto is_normal_function() const -> bool;

    auto is_shader() const -> bool;

    void on_verify(SemaContext& context, Scope& scope) override;

    auto find_parameter(std::string_view name) const -> FunctionParamDecl*;

    auto is_struct_ctor() const -> bool;

  private:
    FunctionKind                                             m_kind;
    gch::small_vector<std::unique_ptr<FunctionParamDecl>, 4> m_parameters;
    std::unique_ptr<CodeBlock>                               m_body;
    bool                                                     m_is_struct_ctor;
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

    auto is_array() const -> bool;

    auto array_size() const -> uint16_t;

    auto default_value_expr() const -> const Expr*;

    auto default_value() const -> const std::any&;

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

    auto is_const() const -> bool;

    auto is_system_value() const -> bool;

    auto expr() const -> const Expr&;

  private:
    bool                  m_is_const;
    std::unique_ptr<Expr> m_expr;
    bool                  m_is_system_value{};
};
} // namespace cer::shadercompiler
