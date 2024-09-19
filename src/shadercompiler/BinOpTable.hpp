// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/Type.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"

namespace cer::shadercompiler
{
enum class BinOpKind;

class BinOpTable final
{
  public:
    BinOpTable();

    NON_COPYABLE(BinOpTable);

    BinOpTable(BinOpTable&&) noexcept = default;

    BinOpTable& operator=(BinOpTable&&) noexcept = default;

    ~BinOpTable() noexcept = default;

    const Type* bin_op_result_type(BinOpKind op_kind, const Type& lhs, const Type& rhs) const;

  private:
    struct Entry // NOLINT(*-pro-type-member-init)
    {
        Entry(BinOpKind                  op_kind,
              gsl::not_null<const Type*> lhs,
              gsl::not_null<const Type*> rhs,
              gsl::not_null<const Type*> result);

        BinOpKind                  op_kind{};
        gsl::not_null<const Type*> lhs;
        gsl::not_null<const Type*> rhs;
        gsl::not_null<const Type*> result;
    };

    SmallVector<Entry, 128> m_entries;
};
} // namespace cer::shadercompiler
