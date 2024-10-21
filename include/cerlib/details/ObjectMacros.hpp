// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string_view>

// clang-format off

/**
 * Declares that a class is a cerlib-specific object.
 * cerlib objects have automatic memory management via shared reference counting with
 * support for C++ move semantics. They can therefore be passed around very efficiently.
 * This behavior is similar to that of classic shared pointer types, such as
 * `std::shared_ptr`.
 *
 * Every object stores at most a single pointer, which points to an instance of its
 * implementation in the free store (heap).
 *
 * @attention The reference counting mechanism for cerlib objects is **not** thread-safe.
 */
#define CERLIB_DECLARE_OBJECT(class_name)                                                          \
  public:                                                                                          \
    using impl_t = details::class_name## Impl;                                                      \
    class_name();                                                                                  \
    explicit class_name(impl_t* impl);                                                             \
    class_name(const class_name& copyFrom);                                                        \
    auto operator=(const class_name& copyFrom)->class_name&;                                       \
    class_name(class_name&& moveFrom) noexcept;                                                    \
    auto operator=(class_name&& moveFrom) noexcept -> class_name&;                                 \
    ~class_name() noexcept;                                                                        \
    explicit operator bool() const                                                                 \
    {                                                                                              \
        return m_impl != nullptr;                                                                  \
    }                                                                                              \
    auto operator==(const class_name& other) const->bool                                           \
    {                                                                                              \
        return m_impl == other.m_impl;                                                             \
    }                                                                                              \
    auto operator!=(const class_name& other) const->bool                                           \
    {                                                                                              \
        return m_impl != other.m_impl;                                                             \
    }                                                                                              \
    auto operator<(const class_name& other) const->bool                                            \
    {                                                                                              \
        return m_impl < other.m_impl;                                                              \
    }                                                                                              \
    auto impl() const -> impl_t*                                                                   \
    {                                                                                              \
        return m_impl;                                                                             \
    }                                                                                              \
                                                                                                   \
  private:                                                                                         \
    impl_t* m_impl

/**
 * Declares that a cerlib-object class derives from another cerlib-object class.
 */
#define CERLIB_DECLARE_DERIVED_OBJECT(base_class_name, class_name)                                 \
  public:                                                                                          \
    class_name();                                                                                  \
    explicit class_name(base_class_name::impl_t* impl);                                            \
    auto operator==(const class_name& other) const->bool                                           \
    {                                                                                              \
        return impl() == other.impl();                                                             \
    }                                                                                              \
    auto operator!=(const class_name& other) const->bool                                           \
    {                                                                                              \
        return impl() != other.impl();                                                             \
    }                                                                                              \
    auto operator<(const class_name& other) const->bool                                            \
    {                                                                                              \
        return impl() < other.impl();                                                              \
    }

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
    auto class_name::operator=(const class_name& copy_from) -> class_name&                         \
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
    auto class_name::operator=(class_name&& move_from) noexcept -> class_name&                     \
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

#ifndef NDEBUG
#define VERIFY_IMPL_ACCESS                                                                         \
    if (!impl)                                                                                     \
    {                                                                                              \
        throw std::logic_error{"Attempting to access an empty object"};                         \
    }
#else
#define VERIFY_IMPL_ACCESS
#endif

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
