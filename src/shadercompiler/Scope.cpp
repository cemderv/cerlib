// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Scope.hpp"

#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Type.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <ranges>
#include <utility>

namespace cer::shadercompiler
{
Scope::Scope()
{
    m_context_stack.push_back(ScopeContext::Normal);

    add_type(IntType::instance());
    add_type(FloatType::instance());
    add_type(BoolType::instance());
    add_type(Vector2Type::instance());
    add_type(Vector4Type::instance());
    add_type(MatrixType::instance());
    add_type(ImageType::instance());
}

Scope::Scope(Scope&& rhs) noexcept = default;

Scope& Scope::operator=(Scope&& rhs) noexcept = default; // NOLINT

Scope::~Scope() noexcept = default;

auto Scope::symbols() const -> std::span<const gsl::not_null<const Decl*>>
{
    return m_symbols;
}

void Scope::add_symbol(const Decl& symbol)
{
    assert(std::ranges::find_if(m_symbols, [&symbol](const auto& e) {
               return e.get() == &symbol;
           }) == m_symbols.cend());

    m_symbols.emplace_back(&symbol);
}

void Scope::remove_symbol(std::string_view name)
{
    assert(!name.empty());

    m_symbols.erase(std::ranges::find_if(std::as_const(m_symbols), [name](const auto& e) {
        return e->name() == name;
    }));
}

void Scope::remove_symbol(const Decl& symbol)
{
    const auto it = std::ranges::find_if(std::as_const(m_symbols), [&symbol](const auto& e) {
        return e.get() == &symbol;
    });

    assert(it != m_symbols.cend());
    m_symbols.erase(it);
}

auto Scope::find_symbol(std::string_view name, bool fall_back_to_parent) const -> const Decl*
{
    assert(!name.empty());

    const Decl* decl = nullptr;

    for (const auto& symbol : std::views::reverse(m_symbols))
    {
        if (const auto* e = symbol.get(); e->name() == name)
        {
            decl = e;
            break;
        }
    }

    if (decl != nullptr)
    {
        return decl;
    }

    if (fall_back_to_parent)
    {
        return m_parent != nullptr ? m_parent->find_symbol(name) : nullptr;
    }

    return nullptr;
}

static auto get_levenstein_distance(std::string_view s1, std::string_view s2) -> size_t
{
    const auto s1_len = s1.size();
    const auto s2_len = s2.size();

    auto distances = gch::small_vector<size_t, 4>(s2_len + 1);
    std::iota(distances.begin(), distances.end(), size_t(0));

    for (size_t i = 0; i < s1_len; ++i)
    {
        auto previous_distance = size_t(0);

        for (size_t j = 0; j < s2_len; ++j)
        {
            distances.at(j + 1) = std::min({std::exchange(previous_distance, distances.at(j + 1)) +
                                                (s1.at(i) == s2.at(j) ? 0 : 1),
                                            distances.at(j) + 1,
                                            distances.at(j + 1) + 1});
        }
    }

    return distances.at(s2_len);
}

auto Scope::find_symbol_with_similar_name(std::string_view name, bool fall_back_to_parent) const
    -> const Decl*
{
    constexpr double threshold = 0.5;

    assert(!name.empty());

    auto symbol_with_min_distance = static_cast<const Decl*>(nullptr);
    auto min_distance             = std::numeric_limits<double>::max();

    for (const gsl::not_null<const Decl*>& symbol : std::ranges::reverse_view(m_symbols))
    {
        const auto s1 = symbol->name();
        const auto s2 = name;

        if (s1 == s2)
        {
            continue;
        }

        const auto len = std::max(s1.length(), s2.length());
        const auto d   = double(get_levenstein_distance(s1, s2)) / double(len);

        if (d <= threshold && d < min_distance)
        {
            symbol_with_min_distance = symbol;
            min_distance             = d;
        }
    }

    if (symbol_with_min_distance != nullptr)
    {
        return symbol_with_min_distance;
    }

    if (fall_back_to_parent)
    {
        return m_parent != nullptr ? m_parent->find_symbol(name) : nullptr;
    }

    return nullptr;
}

auto Scope::find_symbols(std::string_view name, bool fall_back_to_parent) const
    -> gch::small_vector<gsl::not_null<const Decl*>, 4>
{
    assert(!name.empty());

    auto found_symbols = gch::small_vector<gsl::not_null<const Decl*>, 4>{};

    for (const auto& sym : m_symbols)
    {
        if (sym->name() == name)
        {
            found_symbols.push_back(sym);
        }
    }

    if (fall_back_to_parent && m_parent != nullptr)
    {
        const auto syms = m_parent->find_symbols(name, true);
        found_symbols.insert(found_symbols.begin(), syms.begin(), syms.cend());
    }

    return found_symbols;
}

auto Scope::contains_symbol_only_here(std::string_view name) const -> bool
{
    return find_symbol(name, false) != nullptr;
}

auto Scope::contains_symbol_here_or_up(std::string_view name) const -> bool
{
    return find_symbol(name, true) != nullptr;
}

auto Scope::types() const -> std::span<const gsl::not_null<const Type*>>
{
    return m_types;
}

void Scope::add_type(const Type& type)
{
    assert(std::ranges::find_if(m_types, [&type](const auto& e) {
               return e.get() == &type;
           }) == m_types.cend());

    m_types.emplace_back(&type);
}

void Scope::remove_type(std::string_view name)
{
    assert(!name.empty());

    if (const auto it = std::ranges::find_if(std::as_const(m_types),
                                             [name](const auto& e) {
                                                 return e->type_name() == name;
                                             });
        it != m_types.cend())
    {
        m_types.erase(it);
    }
}

void Scope::remove_type(const Type& type)
{
    if (const auto it = std::ranges::find_if(std::as_const(m_types),
                                             [&type](const auto& e) {
                                                 return e.get() == &type;
                                             });
        it != m_types.cend())
    {
        m_types.erase(it);
    }
}

auto Scope::find_type(std::string_view name, bool fall_back_to_parent) const -> const Type*
{
    assert(!name.empty());

    if (const auto it = std::ranges::find_if(m_types,
                                             [name](const auto& e) {
                                                 return e->type_name() == name;
                                             });
        it != m_types.cend())
    {
        return *it;
    }

    if (fall_back_to_parent && m_parent != nullptr)
    {
        return m_parent->find_type(name);
    }

    return nullptr;
}

auto Scope::contains_type_only_here(std::string_view name) const -> bool
{
    return find_type(name, false) != nullptr;
}

auto Scope::contains_type_here_or_up(std::string_view name) const -> bool
{
    return find_type(name, true) != nullptr;
}

auto Scope::parent() const -> Scope*
{
    return m_parent;
}

auto Scope::children() const -> std::span<const std::unique_ptr<Scope>>
{
    return m_children;
}

auto Scope::push_child() -> Scope&
{
    m_children.push_back(std::make_unique<Scope>());
    m_children.back()->m_parent = this;

    return *m_children.back();
}

void Scope::pop_child()
{
    m_children.pop_back();
}

auto Scope::context() const -> ScopeContext
{
    return m_context_stack.back();
}

void Scope::push_context(ScopeContext value)
{
    m_context_stack.push_back(value);
}

void Scope::pop_context()
{
    m_context_stack.pop_back();
}

auto Scope::current_function() const -> const FunctionDecl*
{
    return m_current_function;
}

void Scope::set_current_function(const FunctionDecl* value)
{
    m_current_function = value;
}

auto Scope::function_call_args() const -> const gch::small_vector<gsl::not_null<const Expr*>, 4>&
{
    return m_function_call_args;
}

void Scope::set_function_call_args(gch::small_vector<gsl::not_null<const Expr*>, 4> args)
{
    m_function_call_args = std::move(args);
}
} // namespace cer::shadercompiler
