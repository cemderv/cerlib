// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "BinOpTable.hpp"

#include "shadercompiler/Expr.hpp"

#include <algorithm>

namespace cer::shadercompiler
{
BinOpTable::BinOpTable()
{
    const auto* int_t    = &IntType::instance();
    const auto* bool_t   = &BoolType::instance();
    const auto* float_t  = &FloatType::instance();
    const auto* vector_t = &Vector2Type::instance();
    const auto* color_t  = &Vector4Type::instance();
    const auto* matrix_t = &MatrixType::instance();

    m_entries = {
        {BinOpKind::Add, int_t, int_t, int_t},
        {BinOpKind::Subtract, int_t, int_t, int_t},
        {BinOpKind::Multiply, int_t, int_t, int_t},
        {BinOpKind::Divide, int_t, int_t, int_t},
        {BinOpKind::LessThan, int_t, int_t, bool_t},
        {BinOpKind::LessThanOrEqual, int_t, int_t, bool_t},
        {BinOpKind::GreaterThan, int_t, int_t, bool_t},
        {BinOpKind::GreaterThanOrEqual, int_t, int_t, bool_t},
        {BinOpKind::Equal, int_t, int_t, bool_t},
        {BinOpKind::NotEqual, int_t, int_t, bool_t},

        {BinOpKind::BitwiseAnd, int_t, int_t, int_t},
        {BinOpKind::BitwiseOr, int_t, int_t, int_t},
        {BinOpKind::BitwiseXor, int_t, int_t, int_t},
        {BinOpKind::LeftShift, int_t, int_t, int_t},
        {BinOpKind::RightShift, int_t, int_t, int_t},

        {BinOpKind::Add, float_t, float_t, float_t},
        {BinOpKind::Subtract, float_t, float_t, float_t},
        {BinOpKind::Multiply, float_t, float_t, float_t},
        {BinOpKind::Divide, float_t, float_t, float_t},
        {BinOpKind::LessThan, float_t, float_t, bool_t},
        {BinOpKind::LessThanOrEqual, float_t, float_t, bool_t},
        {BinOpKind::GreaterThan, float_t, float_t, bool_t},
        {BinOpKind::GreaterThanOrEqual, float_t, float_t, bool_t},
        {BinOpKind::Equal, float_t, float_t, bool_t},
        {BinOpKind::NotEqual, float_t, float_t, bool_t},

        {BinOpKind::Add, float_t, int_t, float_t},
        {BinOpKind::Add, int_t, float_t, float_t},
        {BinOpKind::Subtract, float_t, int_t, float_t},
        {BinOpKind::Subtract, int_t, float_t, float_t},
        {BinOpKind::Multiply, float_t, int_t, float_t},
        {BinOpKind::Multiply, int_t, float_t, float_t},
        {BinOpKind::Divide, float_t, int_t, float_t},
        {BinOpKind::Divide, int_t, float_t, float_t},

        {BinOpKind::Add, vector_t, vector_t, vector_t},
        {BinOpKind::Subtract, vector_t, vector_t, vector_t},
        {BinOpKind::Multiply, vector_t, vector_t, vector_t},
        {BinOpKind::Multiply, vector_t, float_t, vector_t},
        {BinOpKind::Multiply, float_t, vector_t, vector_t},
        {BinOpKind::Divide, vector_t, vector_t, vector_t},
        {BinOpKind::Divide, vector_t, float_t, vector_t},

        {BinOpKind::Add, color_t, color_t, color_t},
        {BinOpKind::Subtract, color_t, color_t, color_t},
        {BinOpKind::Multiply, color_t, color_t, color_t},
        {BinOpKind::Multiply, color_t, float_t, color_t},
        {BinOpKind::Multiply, float_t, color_t, color_t},
        {BinOpKind::Divide, color_t, color_t, color_t},
        {BinOpKind::Divide, color_t, float_t, color_t},

        {BinOpKind::Multiply, matrix_t, matrix_t, matrix_t},
        {BinOpKind::Multiply, matrix_t, vector_t, vector_t},
        {BinOpKind::Multiply, vector_t, matrix_t, vector_t},

        {BinOpKind::LogicalAnd, bool_t, bool_t, bool_t},
        {BinOpKind::LogicalOr, bool_t, bool_t, bool_t},
        {BinOpKind::Equal, bool_t, bool_t, bool_t},
        {BinOpKind::NotEqual, bool_t, bool_t, bool_t},
    };
}

auto BinOpTable::bin_op_result_type(BinOpKind op_kind, const Type& lhs, const Type& rhs) const
    -> const Type*
{
    if (const auto it = std::ranges::find_if(m_entries,
                                             [&](const auto& e) {
                                                 return e.op_kind == op_kind &&
                                                        e.lhs.get() == &lhs && e.rhs.get() == &rhs;
                                             });
        it != m_entries.cend())
    {
        return it->result;
    }

    return nullptr;
}

BinOpTable::Entry::Entry(BinOpKind                  op_kind,
                         gsl::not_null<const Type*> lhs,
                         gsl::not_null<const Type*> rhs,
                         gsl::not_null<const Type*> result)
    : op_kind(op_kind)
    , lhs(lhs)
    , rhs(rhs)
    , result(result)
{
}
} // namespace cer::shadercompiler
