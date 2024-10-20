// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/NonCopyable.hpp"
#include "util/small_vector.hpp"
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

    auto symbols() const -> std::span<const gsl::not_null<const Decl*>>;

    void add_symbol(const Decl& symbol);

    void remove_symbol(std::string_view name);

    void remove_symbol(const Decl& symbol);

    auto find_symbol(std::string_view name, bool fall_back_to_parent = true) const -> const Decl*;

    auto find_symbol_with_similar_name(std::string_view name, bool fall_back_to_parent = true) const
        -> const Decl*;

    auto find_symbols(std::string_view name, bool fall_back_to_parent = true) const
        -> gch::small_vector<gsl::not_null<const Decl*>, 4>;

    auto contains_symbol_only_here(std::string_view name) const -> bool;

    auto contains_symbol_here_or_up(std::string_view name) const -> bool;

    auto types() const -> std::span<const gsl::not_null<const Type*>>;

    void add_type(const Type& type);

    void remove_type(std::string_view name);

    void remove_type(const Type& type);

    auto find_type(std::string_view name, bool fall_back_to_parent = true) const -> const Type*;

    auto contains_type_only_here(std::string_view name) const -> bool;

    auto contains_type_here_or_up(std::string_view name) const -> bool;

    auto parent() const -> Scope*;

    auto children() const -> std::span<const std::unique_ptr<Scope>>;

    auto push_child() -> Scope&;

    void pop_child();

    auto context() const -> ScopeContext;

    void push_context(ScopeContext value);

    void pop_context();

    auto current_function() const -> const FunctionDecl*;

    void set_current_function(const FunctionDecl* value);

    auto function_call_args() const -> const gch::small_vector<gsl::not_null<const Expr*>, 4>&;

    void set_function_call_args(gch::small_vector<gsl::not_null<const Expr*>, 4> args);

  private:
    gch::small_vector<gsl::not_null<const Decl*>, 8> m_symbols;
    gch::small_vector<gsl::not_null<const Type*>, 8> m_types;
    Scope*                                           m_parent{};
    gch::small_vector<std::unique_ptr<Scope>, 4>     m_children;
    gch::small_vector<ScopeContext, 4>               m_context_stack;
    const FunctionDecl*                              m_current_function{};
    gch::small_vector<gsl::not_null<const Expr*>, 4> m_function_call_args;
};
} // namespace cer::shadercompiler
