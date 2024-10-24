// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
#include <cerlib/String.hpp>
#include <span>

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

    forbid_copy(Scope);

    Scope(Scope&& rhs) noexcept;

    auto operator=(Scope&& rhs) noexcept -> Scope&;

    ~Scope() noexcept;

    auto symbols() const -> std::span<const std::reference_wrapper<const Decl>>;

    void add_symbol(const Decl& symbol);

    void remove_symbol(std::string_view name);

    void remove_symbol(const Decl& symbol);

    auto find_symbol(std::string_view name, bool fall_back_to_parent = true) const -> const Decl*;

    auto find_symbol_with_similar_name(std::string_view name, bool fall_back_to_parent = true) const
        -> const Decl*;

    auto find_symbols(std::string_view name, bool fall_back_to_parent = true) const
        -> RefList<const Decl, 4>;

    auto contains_symbol_only_here(std::string_view name) const -> bool;

    auto contains_symbol_here_or_up(std::string_view name) const -> bool;

    auto types() const -> std::span<const std::reference_wrapper<const Type>>;

    void add_type(const Type& type);

    void remove_type(std::string_view name);

    void remove_type(const Type& type);

    auto find_type(std::string_view name, bool fall_back_to_parent = true) const -> const Type*;

    auto contains_type_only_here(std::string_view name) const -> bool;

    auto contains_type_here_or_up(std::string_view name) const -> bool;

    auto parent() const -> Scope*;

    auto children() const -> std::span<const UniquePtr<Scope>>;

    auto push_child() -> Scope&;

    void pop_child();

    auto context() const -> ScopeContext;

    void push_context(ScopeContext value);

    void pop_context();

    auto current_function() const -> const FunctionDecl*;

    void set_current_function(const FunctionDecl* value);

    auto function_call_args() const -> const RefList<const Expr, 4>&;

    void set_function_call_args(RefList<const Expr, 4> args);

  private:
    RefList<const Decl, 8>  m_symbols;
    RefList<const Type, 8>  m_types;
    Scope*                  m_parent{};
    UniquePtrList<Scope, 4> m_children;
    List<ScopeContext, 4>   m_context_stack;
    const FunctionDecl*     m_current_function{};
    RefList<const Expr, 4>  m_function_call_args;
};
} // namespace cer::shadercompiler
