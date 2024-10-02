// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cstddef>
#include <string_view>

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
    using impl_t = details::class_name##Impl;                                                      \
    class_name();                                                                                  \
    explicit class_name(impl_t* impl);                                                             \
    class_name(const class_name& copyFrom);                                                        \
    class_name& operator=(const class_name& copyFrom);                                             \
    class_name(class_name&& moveFrom) noexcept;                                                    \
    class_name& operator=(class_name&& moveFrom) noexcept;                                         \
    ~class_name() noexcept;                                                                        \
    explicit operator bool() const                                                                 \
    {                                                                                              \
        return m_impl != nullptr;                                                                  \
    }                                                                                              \
    bool operator==(const class_name& other) const                                                 \
    {                                                                                              \
        return m_impl == other.m_impl;                                                             \
    }                                                                                              \
    bool operator!=(const class_name& other) const                                                 \
    {                                                                                              \
        return m_impl != other.m_impl;                                                             \
    }                                                                                              \
    bool operator<(const class_name& other) const                                                  \
    {                                                                                              \
        return m_impl < other.m_impl;                                                              \
    }                                                                                              \
    impl_t* impl() const                                                                           \
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
    bool operator==(const class_name& other) const                                                 \
    {                                                                                              \
        return impl() == other.impl();                                                             \
    }                                                                                              \
    bool operator!=(const class_name& other) const                                                 \
    {                                                                                              \
        return impl() != other.impl();                                                             \
    }                                                                                              \
    bool operator<(const class_name& other) const                                                  \
    {                                                                                              \
        return impl() < other.impl();                                                              \
    }
