// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "Object.hpp"

#include <cassert>

namespace cer::details
{
void Object::add_ref()
{
    ++m_ref_count;
}

auto Object::release() -> uint64_t
{
#ifdef CERLIB_ATOMIC_REFCOUNTING
    assert(m_ref_count.load() > 0);
#else
    assert(m_ref_count > 0);
#endif

    const auto new_ref_count = --m_ref_count;

    if (new_ref_count == 0)
    {
        delete this;
    }

    return new_ref_count;
}

auto Object::ref_count() const -> uint64_t
{
#ifdef CERLIB_ATOMIC_REFCOUNTING
    return m_ref_count.load();
#else
    return m_ref_count;
#endif
}
} // namespace cer::details
