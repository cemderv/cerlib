// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <gsl/pointers>
#include <span>
#include <string_view>

namespace cer::shadercompiler
{
class Decl;
class Type;
class Expr;
class FunctionDecl;

enum class ScopeContext
{
    Normal,
    FunctionCall,
};

class Scope final
{
  public:
    explicit Scope();

    NON_COPYABLE(Scope);

    Scope(Scope&& rhs) noexcept;

    auto operator=(Scope&& rhs) noexcept -> Scope&;

    ~Scope() noexcept;

    std::span<const gsl::not_null<const Decl*>> symbols() const;

    void add_symbol(const Decl& symbol);

    void remove_symbol(std::string_view name);

    void remove_symbol(const Decl& symbol);

    const Decl* find_symbol(std::string_view name, bool fall_back_to_parent = true) const;

    const Decl* find_symbol_with_similar_name(std::string_view name,
                                              bool             fall_back_to_parent = true) const;

    SmallVector<gsl::not_null<const Decl*>, 4> find_symbols(std::string_view name,
                                                            bool fall_back_to_parent = true) const;

    bool contains_symbol_only_here(std::string_view name) const;

    bool contains_symbol_here_or_up(std::string_view name) const;

    std::span<const gsl::not_null<const Type*>> types() const;

    void add_type(const Type& type);

    void remove_type(std::string_view name);

    void remove_type(const Type& type);

    const Type* find_type(std::string_view name, bool fall_back_to_parent = true) const;

    bool contains_type_only_here(std::string_view name) const;

    bool contains_type_here_or_up(std::string_view name) const;

    Scope* parent() const;

    std::span<const std::unique_ptr<Scope>> children() const;

    Scope& push_child();

    void pop_child();

    ScopeContext context() const;

    void push_context(ScopeContext value);

    void pop_context();

    const FunctionDecl* current_function() const;

    void set_current_function(const FunctionDecl* value);

    const SmallVector<gsl::not_null<const Expr*>, 4>& function_call_args() const;

    void set_function_call_args(SmallVector<gsl::not_null<const Expr*>, 4> args);

  private:
    SmallVector<gsl::not_null<const Decl*>, 8> m_symbols;
    SmallVector<gsl::not_null<const Type*>, 8> m_types;
    Scope*                                     m_parent{};
    SmallVector<std::unique_ptr<Scope>, 4>     m_children;
    SmallVector<ScopeContext, 4>               m_context_stack;
    const FunctionDecl*                        m_current_function{};
    SmallVector<gsl::not_null<const Expr*>, 4> m_function_call_args;
};
} // namespace cer::shadercompiler
