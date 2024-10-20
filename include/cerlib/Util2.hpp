// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

// Contains general purpose loose functions.

#pragma once

#include <cerlib/Util2.hpp>
#include <cstdint>
#include <string>

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b)       CONCAT_INNER(a, b)
#define UNIQUE_NAME(base)  CONCAT(base, __COUNTER__)

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
        CER_THROW_LOGIC_ERROR_STR("Attempting to access an empty object");                         \
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

namespace cer
{
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

template <typename T>
void remove_duplicates_but_keep_order(T& container)
{
    auto i = size_t(0);

    while (i != container.size())
    {
        auto j = i;
        ++j;

        while (j != container.size())
        {
            if (container.at(j) == container.at(i))
            {
                container.erase(container.begin() + j);
            }
            else
            {
                ++j;
            }
        }

        ++i;
    }
}

template <typename... T>
struct VariantSwitch : T...
{
    using T::operator()...;
};

template <typename... T>
VariantSwitch(T...) -> VariantSwitch<T...>;

namespace details
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
} // namespace details
} // namespace cer
