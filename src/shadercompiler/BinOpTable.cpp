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
    const Type* int_t    = &IntType::instance();
    const Type* bool_t   = &BoolType::instance();
    const Type* float_t  = &FloatType::instance();
    const Type* vector_t = &Vector2Type::instance();
    const Type* color_t  = &Vector4Type::instance();
    const Type* matrix_t = &MatrixType::instance();

    m_entries = {
        Entry{BinOpKind::Add, int_t, int_t, int_t},
        Entry{BinOpKind::Subtract, int_t, int_t, int_t},
        Entry{BinOpKind::Multiply, int_t, int_t, int_t},
        Entry{BinOpKind::Divide, int_t, int_t, int_t},
        Entry{BinOpKind::LessThan, int_t, int_t, bool_t},
        Entry{BinOpKind::LessThanOrEqual, int_t, int_t, bool_t},
        Entry{BinOpKind::GreaterThan, int_t, int_t, bool_t},
        Entry{BinOpKind::GreaterThanOrEqual, int_t, int_t, bool_t},
        Entry{BinOpKind::Equal, int_t, int_t, bool_t},
        Entry{BinOpKind::NotEqual, int_t, int_t, bool_t},

        Entry{BinOpKind::BitwiseAnd, int_t, int_t, int_t},
        Entry{BinOpKind::BitwiseOr, int_t, int_t, int_t},
        Entry{BinOpKind::BitwiseXor, int_t, int_t, int_t},
        Entry{BinOpKind::LeftShift, int_t, int_t, int_t},
        Entry{BinOpKind::RightShift, int_t, int_t, int_t},

        Entry{BinOpKind::Add, float_t, float_t, float_t},
        Entry{BinOpKind::Subtract, float_t, float_t, float_t},
        Entry{BinOpKind::Multiply, float_t, float_t, float_t},
        Entry{BinOpKind::Divide, float_t, float_t, float_t},
        Entry{BinOpKind::LessThan, float_t, float_t, bool_t},
        Entry{BinOpKind::LessThanOrEqual, float_t, float_t, bool_t},
        Entry{BinOpKind::GreaterThan, float_t, float_t, bool_t},
        Entry{BinOpKind::GreaterThanOrEqual, float_t, float_t, bool_t},
        Entry{BinOpKind::Equal, float_t, float_t, bool_t},
        Entry{BinOpKind::NotEqual, float_t, float_t, bool_t},

        Entry{BinOpKind::Add, float_t, int_t, float_t},
        Entry{BinOpKind::Add, int_t, float_t, float_t},
        Entry{BinOpKind::Subtract, float_t, int_t, float_t},
        Entry{BinOpKind::Subtract, int_t, float_t, float_t},
        Entry{BinOpKind::Multiply, float_t, int_t, float_t},
        Entry{BinOpKind::Multiply, int_t, float_t, float_t},
        Entry{BinOpKind::Divide, float_t, int_t, float_t},
        Entry{BinOpKind::Divide, int_t, float_t, float_t},

        Entry{BinOpKind::Add, vector_t, vector_t, vector_t},
        Entry{BinOpKind::Subtract, vector_t, vector_t, vector_t},
        Entry{BinOpKind::Multiply, vector_t, vector_t, vector_t},
        Entry{BinOpKind::Multiply, vector_t, float_t, vector_t},
        Entry{BinOpKind::Multiply, float_t, vector_t, vector_t},
        Entry{BinOpKind::Divide, vector_t, vector_t, vector_t},
        Entry{BinOpKind::Divide, vector_t, float_t, vector_t},

        Entry{BinOpKind::Add, color_t, color_t, color_t},
        Entry{BinOpKind::Subtract, color_t, color_t, color_t},
        Entry{BinOpKind::Multiply, color_t, color_t, color_t},
        Entry{BinOpKind::Multiply, color_t, float_t, color_t},
        Entry{BinOpKind::Multiply, float_t, color_t, color_t},
        Entry{BinOpKind::Divide, color_t, color_t, color_t},
        Entry{BinOpKind::Divide, color_t, float_t, color_t},

        Entry{BinOpKind::Multiply, matrix_t, matrix_t, matrix_t},
        Entry{BinOpKind::Multiply, matrix_t, vector_t, vector_t},
        Entry{BinOpKind::Multiply, vector_t, matrix_t, vector_t},

        Entry{BinOpKind::LogicalAnd, bool_t, bool_t, bool_t},
        Entry{BinOpKind::LogicalOr, bool_t, bool_t, bool_t},
        Entry{BinOpKind::Equal, bool_t, bool_t, bool_t},
        Entry{BinOpKind::NotEqual, bool_t, bool_t, bool_t},
    };
}

const Type* BinOpTable::bin_op_result_type(BinOpKind   op_kind,
                                           const Type& lhs,
                                           const Type& rhs) const
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
