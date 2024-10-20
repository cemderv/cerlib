// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/Type.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>

namespace cer::shadercompiler
{
enum class BinOpKind;

class BinOpTable final
{
  public:
    BinOpTable();

    forbid_copy(BinOpTable);

    BinOpTable(BinOpTable&&) noexcept = default;

    auto operator=(BinOpTable&&) noexcept -> BinOpTable& = default;

    ~BinOpTable() noexcept = default;

    auto bin_op_result_type(BinOpKind op_kind, const Type& lhs, const Type& rhs) const
        -> const Type*;

  private:
    struct Entry // NOLINT(*-pro-type-member-init)
    {
        Entry(BinOpKind op_kind, const Type& lhs, const Type& rhs, const Type& result);

        BinOpKind                          op_kind{};
        std::reference_wrapper<const Type> lhs;
        std::reference_wrapper<const Type> rhs;
        std::reference_wrapper<const Type> result;
    };

    small_vector<Entry, 128> m_entries;
};
} // namespace cer::shadercompiler
