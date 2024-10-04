// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

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
} // namespace cer::shadercompiler
