// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <type_traits>

namespace cer::shadercompiler
{
template <typename To, typename From>
static bool isa(const From* f)
{
    return dynamic_cast<const To*>(f) != nullptr;
}

template <typename To, typename From>
static bool isa(const From& f)
    requires(!std::is_pointer_v<From>)
{
    return dynamic_cast<const To*>(&f) != nullptr;
}

template <typename U, typename T>
static U* asa(T* obj)
{
    return isa<U>(obj) ? static_cast<U*>(obj) : nullptr;
}

template <typename U, typename T>
static const U* asa(const T* obj)
{
    return isa<U>(obj) ? static_cast<const U*>(obj) : nullptr;
}
} // namespace cer::shadercompiler
