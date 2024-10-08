// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "NonCopyable.hpp"
#include "util/InternalExport.hpp"

#ifdef CERLIB_ATOMIC_REFCOUNTING
#include <atomic>
#endif

#include <cstddef>
#include <cstdint>

namespace cer::details
{
class CERLIB_API_INTERNAL Object
{
  public:
    explicit Object();

    NON_COPYABLE_NON_MOVABLE(Object);

    virtual ~Object() noexcept;

    void add_ref();

    uint64_t release();

    uint64_t ref_count() const;

  private:
#ifdef CERLIB_ATOMIC_REFCOUNTING
    std::atomic<uint64_t> m_ref_count;
#else
    uint64_t m_ref_count{};
#endif
};
} // namespace cer::details
