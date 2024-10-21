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
