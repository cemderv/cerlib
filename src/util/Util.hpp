// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

// Contains general purpose loose functions.

#pragma once

#include "InternalError.hpp"
#include "util/Util.hpp"

#include <cstdint>
#include <string>

#define CERLIB_UNUSED(x) (void)x

#define CERLIB_IMPLEMENT_OBJECT_FUNCS(class_name)                                                  \
    class_name::class_name(impl_t* impl)                                                           \
        : m_impl(impl)                                                                             \
    {                                                                                              \
        if (m_impl != nullptr)                                                                     \
        {                                                                                          \
            m_impl->add_ref();                                                                     \
        }                                                                                          \
    }                                                                                              \
    class_name::class_name(const class_name& copy_from)                                            \
        : m_impl(copy_from.m_impl)                                                                 \
    {                                                                                              \
        if (m_impl != nullptr)                                                                     \
        {                                                                                          \
            m_impl->add_ref();                                                                     \
        }                                                                                          \
    }                                                                                              \
    class_name& class_name::operator=(const class_name& copy_from)                                 \
    {                                                                                              \
        if (std::addressof(copy_from) != this)                                                     \
        {                                                                                          \
            if (m_impl != nullptr)                                                                 \
            {                                                                                      \
                m_impl->release();                                                                 \
            }                                                                                      \
                                                                                                   \
            m_impl = copy_from.m_impl;                                                             \
                                                                                                   \
            if (m_impl != nullptr)                                                                 \
            {                                                                                      \
                m_impl->add_ref();                                                                 \
            }                                                                                      \
        }                                                                                          \
                                                                                                   \
        return *this;                                                                              \
    }                                                                                              \
    class_name::class_name(class_name&& move_from) noexcept                                        \
        : m_impl(move_from.m_impl)                                                                 \
    {                                                                                              \
        move_from.m_impl = nullptr;                                                                \
    }                                                                                              \
    class_name& class_name::operator=(class_name&& move_from) noexcept                             \
    {                                                                                              \
        if (std::addressof(move_from) != this)                                                     \
        {                                                                                          \
            if (m_impl != nullptr)                                                                 \
            {                                                                                      \
                m_impl->release();                                                                 \
            }                                                                                      \
                                                                                                   \
            m_impl           = move_from.m_impl;                                                   \
            move_from.m_impl = nullptr;                                                            \
        }                                                                                          \
                                                                                                   \
        return *this;                                                                              \
    }                                                                                              \
    class_name::~class_name() noexcept                                                             \
    {                                                                                              \
        if (m_impl != nullptr)                                                                     \
        {                                                                                          \
            m_impl->release();                                                                     \
        }                                                                                          \
    }

#define VERIFY_CERLIB_OBJECT(T)                                                                    \
    static_assert(sizeof(T) == sizeof(uintptr_t),                                                  \
                  "The type must be a cerlib object without any extra fields.")

#define CERLIB_IMPLEMENT_OBJECT_NO_CTOR(class_name)                                                \
    VERIFY_CERLIB_OBJECT(class_name);                                                              \
    CERLIB_IMPLEMENT_OBJECT_FUNCS(class_name)

#define CERLIB_IMPLEMENT_OBJECT(class_name)                                                        \
    CERLIB_IMPLEMENT_OBJECT_NO_CTOR(class_name);                                                   \
    class_name::class_name()                                                                       \
        : m_impl(nullptr)                                                                          \
    {                                                                                              \
    }

#define CERLIB_IMPLEMENT_DERIVED_OBJECT(base_class_name, class_name)                               \
    class_name::class_name()                                                                       \
    {                                                                                              \
    }                                                                                              \
    class_name::class_name(base_class_name::impl_t* impl)                                          \
        : base_class_name(impl)                                                                    \
    {                                                                                              \
    }

#define VERIFY_IMPL_ACCESS                                                                         \
    if (!impl)                                                                                     \
    {                                                                                              \
        CER_THROW_LOGIC_ERROR("Attempting to access an empty {}", type_name);                      \
    }

// clang-format off
#define DECLARE_THIS_IMPL                                                                \
  const auto impl = this->impl();                                                        \
  VERIFY_IMPL_ACCESS

#define DECLARE_THIS_IMPL_OR_RETURN                                                      \
  const auto impl = this->impl();                                                        \
  if (!impl)                                                                             \
    return;

#define DECLARE_THIS_IMPL_OR_RETURN_VALUE(value)                                         \
  const auto impl = this->impl();                                                        \
  if (!impl)                                                                             \
    return value;

// clang-format on

namespace cer::details
{
template <typename T>
struct ObjectLayout
{
    VERIFY_CERLIB_OBJECT(T);

    typename T::impl_t* impl{};
};

template <typename T, typename TImpl = typename T::Impl>
static void set_impl(T& obj, TImpl* impl)
{
    static_assert(sizeof(T) == sizeof(uintptr_t),
                  "Invalid type of object specified; must consist of a single impl pointer.");

    ObjectLayout<T>& s = reinterpret_cast<ObjectLayout<T>&>(obj);

    if (s.impl != nullptr)
    {
        s.impl->release();
    }

    s.impl = impl;

    if (s.impl)
    {
        s.impl->add_ref();
    }
}

template <typename Iterator, typename T>
static Iterator binary_find(Iterator begin, Iterator end, T value)
{
    const auto it = std::lower_bound(begin, end, value);

    if (it != end && !(value < *it))
    {
        end = it;
    }

    return end;
}
} // namespace cer::details
