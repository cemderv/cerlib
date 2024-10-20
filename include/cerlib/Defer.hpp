// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/Util2.hpp>

namespace cer::details
{
template <typename Functor>
class DeferObject final
{
  public:
    DeferObject() = delete;

    explicit DeferObject(Functor action)
        : m_functor(action)
        , m_is_dismissed(true)
    {
    }

    forbid_copy(DeferObject);

    DeferObject(DeferObject&& other) noexcept
        : m_functor(std::move(other.m_functor))
        , m_is_dismissed(other.m_is_dismissed)
    {
        other.m_is_dismissed = true;
    }

    ~DeferObject()
    {
        if (m_is_dismissed)
        {
            m_functor();
        }
    }

    void dismiss()
    {
        m_is_dismissed = false;
    }

  private:
    Functor m_functor;
    bool    m_is_dismissed = false;
};

enum class DeferOperatorOverloadTag
{
};

template <typename Functor>
auto operator+(DeferOperatorOverloadTag, Functor&& functor) -> DeferObject<Functor>
{
    return DeferObject<Functor>{std::forward<Functor>(functor)};
}
} // namespace cer::details

// NOLINTBEGIN

#define defer_named(name)                                                                          \
    auto name = cer::details::DeferOperatorOverloadTag{} + [&]() // NOLINT(*-macro-parentheses)

#define defer defer_named(UNIQUE_NAME(SCOPEGUARD))

// NOLINTEND
