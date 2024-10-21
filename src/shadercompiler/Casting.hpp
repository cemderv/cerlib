// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <exception>
#include <type_traits>

namespace cer::shadercompiler
{
template <typename To, typename From>
static auto isa(const From* f) -> bool
{
    return dynamic_cast<const To*>(f) != nullptr;
}

template <typename To, typename From>
static auto isa(const From& f) -> bool
    requires(!std::is_pointer_v<From>)
{
    return dynamic_cast<const To*>(&f) != nullptr;
}

template <typename U, typename T>
static auto asa(T* obj) -> U*
{
    return isa<U>(obj) ? static_cast<U*>(obj) : nullptr;
}

template <typename U, typename T>
static auto asa(const T* obj) -> const U*
{
    return isa<U>(obj) ? static_cast<const U*>(obj) : nullptr;
}

struct CastingError final : std::exception
{
    auto what() const noexcept -> const char* override
    {
        return "asserted cast failed; object contains an unexpected type";
    }
};

template <typename U, typename T>
static auto asa_or_error(T& obj) -> U&
{
    auto* result = asa<U>(&obj);

    if (result == nullptr)
    {
        throw CastingError{};
    }

    return *result;
}

template <typename U, typename T>
static auto asa_or_error(const T& obj) -> const U&
{
    const auto* result = asa<U>(&obj);

    if (result == nullptr)
    {
        throw CastingError{};
    }

    return *result;
}

template <typename U, typename T>
static auto asa_or_error(const std::reference_wrapper<T>& obj) -> U&
{
    auto* result = asa<U>(&obj.get());

    if (result == nullptr)
    {
        throw CastingError{};
    }

    return *result;
}

template <typename U, typename T>
static auto asa_or_error(const std::reference_wrapper<const T>& obj) -> const U&
{
    const auto* result = asa<U>(&obj.get());

    if (result == nullptr)
    {
        throw CastingError{};
    }

    return *result;
}
} // namespace cer::shadercompiler
