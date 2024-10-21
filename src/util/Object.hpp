// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/CopyMoveMacros.hpp"

#ifdef CERLIB_ATOMIC_REFCOUNTING
#include <atomic>
#endif

#include <cstddef>
#include <cstdint>

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
        throw std::logic_error{"Attempting to access an empty object"};                            \
    }
#else
#define VERIFY_IMPL_ACCESS
#endif

#define DECLARE_THIS_IMPL                                                                          \
    const auto impl = this -> impl();                                                              \
    VERIFY_IMPL_ACCESS

#define DECLARE_THIS_IMPL_OR_RETURN                                                                \
    const auto impl = this -> impl();                                                              \
    if (!impl)                                                                                     \
        return;

#define DECLARE_THIS_IMPL_OR_RETURN_VALUE(value)                                                   \
    const auto impl = this -> impl();                                                              \
    if (!impl)                                                                                     \
        return value;


namespace cer::details
{
class Object
{
  public:
    explicit Object() = default;

    forbid_copy_and_move(Object);

    virtual ~Object() noexcept = default;

    void add_ref();

    auto release() -> uint64_t;

    auto ref_count() const -> uint64_t;

  private:
#ifdef CERLIB_ATOMIC_REFCOUNTING
    std::atomic<uint64_t> m_ref_count;
#else
    uint64_t m_ref_count{};
#endif
};

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
} // namespace cer::details
