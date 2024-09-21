// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/BuiltInSymbols.hpp"

#include "Casting.hpp"
#include "shadercompiler/CodeBlock.hpp"
#include "shadercompiler/Decl.hpp"
#include "shadercompiler/Naming.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/Type.hpp"

#include <cassert>
#include <format>

// NOLINTBEGIN

#define ADD_FUNC_FOR_FLOAT_TO_VECTOR4(name)                                                        \
    add_func(name##_Float, #name, {{"value", floatT}}, *floatT);                                   \
    add_func(name##_Vector2, #name, {{"value", vector2T}}, *vector2T);                             \
    add_func(name##_Vector3, #name, {{"value", vector3T}}, *vector3T);                             \
    add_func(name##_Vector4, #name, {{"value", vector4T}}, *vector4T)

#define ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(name, argname1, argname2)                           \
    add_func(name##_Float, #name, {{argname1, floatT}, {argname2, floatT}}, *floatT);              \
    add_func(name##_Vector2, #name, {{argname1, vector2T}, {argname2, vector2T}}, *vector2T);      \
    add_func(name##_Vector3, #name, {{argname1, vector3T}, {argname2, vector3T}}, *vector3T);      \
    add_func(name##_Vector4, #name, {{argname1, vector4T}, {argname2, vector4T}}, *vector4T)

#define ADD_FUNC_FOR_ALL_VECTORS(name)                                                             \
    add_func(name##_Vector2, #name, {{"value", vector2T}}, *vector2T);                             \
    add_func(name##_Vector3, #name, {{"value", vector3T}}, *vector3T);                             \
    add_func(name##_Vector4, #name, {{"value", vector4T}}, *vector4T)

#define ADD_FUNC_FOR_ALL_VECTORS_TWO_ARGS(name, argname1, argname2)                                \
    add_func(name##_Vector2, #name, {{argname1, vector2T}, {argname2, vector2T}}, *vector2T);      \
    add_func(name##_Vector3, #name, {{argname1, vector3T}, {argname2, vector3T}}, *vector3T);      \
    add_func(name##_Vector4, #name, {{argname1, vector4T}, {argname2, vector4T}}, *vector4T)

#define ADD_FUNC_FOR_FLOAT_TO_MATRIX(name)                                                         \
    add_func(name##_Float, #name, {{"value", floatT}}, *floatT);                                   \
    add_func(name##_Vector2, #name, {{"value", vector2T}}, *vector2T);                             \
    add_func(name##_Vector3, #name, {{"value", vector3T}}, *vector3T);                             \
    add_func(name##_Vector4, #name, {{"value", vector4T}}, *vector4T);                             \
    add_func(name##_Matrix4x4, #name, {{"value", matrixT}}, *matrixT)

#define ADD_FUNC_FOR_FLOAT_TO_MATRIX_BOOL(name)                                                    \
    add_func(name##_Float, #name, {{"value", floatT}}, *boolT);                                    \
    add_func(name##_Vector2, #name, {{"value", vector2T}}, *boolT);                                \
    add_func(name##_Vector3, #name, {{"value", vector3T}}, *boolT);                                \
    add_func(name##_Vector4, #name, {{"value", vector4T}}, *boolT);                                \
    add_func(name##_Matrix4x4, #name, {{"value", matrixT}}, *boolT)

// NOLINTEND

namespace cer::shadercompiler
{
static void add_struct_fields(
    SmallVector<gsl::not_null<Decl*>, 132>&                                        all,
    std::vector<std::unique_ptr<Decl>>&                                            var,
    std::initializer_list<std::pair<std::string_view, gsl::not_null<const Type*>>> fields)
{
    var.reserve(fields.size());

    for (const auto& [field_name, field_type] : fields)
    {
        var.push_back(
            std::make_unique<StructFieldDecl>(SourceLocation::std, field_name, *field_type));

        all.emplace_back(var.back().get());
    }
}

BuiltInSymbols::BuiltInSymbols()
{
    const Type* int_t    = &IntType::instance();
    const Type* floatT   = &FloatType::instance();
    const Type* vector2T = &Vector2Type::instance();
    const Type* vector3T = &Vector3Type::instance();
    const Type* vector4T = &Vector4Type::instance();
    const Type* matrixT  = &MatrixType::instance();
    const Type* image_t  = &ImageType::instance();
    const Type* boolT    = &BoolType::instance();

    add_func(float_ctor_int, floatT->type_name(), {{"value", int_t}}, *floatT);
    add_func(int_ctor_float, int_t->type_name(), {{"value", floatT}}, *int_t);
    add_func(int_ctor_uint, int_t->type_name(), {{"value", floatT}}, *int_t);

    // Vector2 ctors
    add_func(vector2_ctor_xy,
             vector2T->type_name(),
             {
                 {"xy", floatT},
             },
             *vector2T);

    add_func(vector2_ctor_x_y,
             vector2T->type_name(),
             {
                 {"x", floatT},
                 {"y", floatT},
             },
             *vector2T);

    // Vector3 ctors
    add_func(vector3_ctor_x_y_z,
             vector3T->type_name(),
             {
                 {"x", floatT},
                 {"y", floatT},
                 {"z", floatT},
             },
             *vector3T);

    add_func(vector3_ctor_xy_z,
             vector3T->type_name(),
             {
                 {"xy", vector2T},
                 {"z", floatT},
             },
             *vector3T);

    add_func(vector3_ctor_xyz,
             vector3T->type_name(),
             {
                 {"xyz", floatT},
             },
             *vector3T);

    // Vector4 ctors
    add_func(vector4_ctor_x_y_z_w,
             vector4T->type_name(),
             {
                 {"x", floatT},
                 {"y", floatT},
                 {"z", floatT},
                 {"w", floatT},
             },
             *vector4T);

    add_func(vector4_ctor_xy_zw,
             vector4T->type_name(),
             {
                 {"xy", vector2T},
                 {"zw", vector2T},
             },
             *vector4T);

    add_func(vector4_ctor_xy_z_w,
             vector4T->type_name(),
             {
                 {"xy", vector2T},
                 {"z", floatT},
                 {"w", floatT},
             },
             *vector4T);

    add_func(vector4_ctor_xyz_w,
             vector4T->type_name(),
             {
                 {"xyz", vector3T},
                 {"w", floatT},
             },
             *vector4T);

    add_func(vector4_ctor_xyzw,
             vector4T->type_name(),
             {
                 {"xyzw", floatT},
             },
             *vector4T);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(abs);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(acos);
    ADD_FUNC_FOR_FLOAT_TO_MATRIX_BOOL(all);
    ADD_FUNC_FOR_FLOAT_TO_MATRIX_BOOL(any);
    ADD_FUNC_FOR_FLOAT_TO_MATRIX(ceil);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(asin);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(atan);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(atan2, "y", "x");

    add_func(clamp_Float,
             "clamp",
             {{"value", floatT}, {"start", floatT}, {"end", floatT}},
             *floatT);

    add_func(clamp_Vector2,
             "clamp",
             {{"value", vector2T}, {"start", vector2T}, {"end", vector2T}},
             *vector2T);

    add_func(clamp_Vector3,
             "clamp",
             {{"value", vector3T}, {"start", vector3T}, {"end", vector3T}},
             *vector3T);

    add_func(clamp_Vector4,
             "clamp",
             {{"value", vector4T}, {"start", vector4T}, {"end", vector4T}},
             *vector4T);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(cos);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(degrees);

    add_func(determinant_matrix, "determinant", {{"value", matrixT}}, *floatT);

    add_func(distance_Vector2, "distance", {{"lhs", vector2T}, {"rhs", vector2T}}, *floatT);
    add_func(distance_Vector3, "distance", {{"lhs", vector3T}, {"rhs", vector3T}}, *floatT);
    add_func(distance_Vector4, "distance", {{"lhs", vector4T}, {"rhs", vector4T}}, *floatT);

    add_func(dot_Vector2, "dot", {{"lhs", vector2T}, {"rhs", vector2T}}, *floatT);
    add_func(dot_Vector3, "dot", {{"lhs", vector3T}, {"rhs", vector3T}}, *floatT);
    add_func(dot_Vector4, "dot", {{"lhs", vector4T}, {"rhs", vector4T}}, *floatT);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(exp);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(exp2);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(floor);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(fmod, "x", "y");

    add_func(length_Vector2, "length", {{"value", vector2T}}, *floatT);
    add_func(length_Vector3, "length", {{"value", vector3T}}, *floatT);
    add_func(length_Vector4, "length", {{"value", vector4T}}, *floatT);

    add_func(lerp_Float, "lerp", {{"start", floatT}, {"stop", floatT}, {"t", floatT}}, *floatT);
    add_func(lerp_Vector2,
             "lerp",
             {{"start", vector2T}, {"stop", vector2T}, {"t", floatT}},
             *vector2T);
    add_func(lerp_Vector3,
             "lerp",
             {{"start", vector3T}, {"stop", vector3T}, {"t", floatT}},
             *vector3T);
    add_func(lerp_Vector4,
             "lerp",
             {{"start", vector4T}, {"stop", vector4T}, {"t", floatT}},
             *vector4T);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(log);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(log2);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(max, "lhs", "rhs");
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(min, "lhs", "rhs");

    ADD_FUNC_FOR_ALL_VECTORS(normalize);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(pow, "x", "y");

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(radians);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(round);

    add_func(sample_image, "sample", {{"image", image_t}, {"coords", vector2T}}, *vector4T);

    add_func(sample_level_image,
             "sample_level",
             {{"image", image_t}, {"coords", vector2T}, {"level", floatT}},
             *vector4T);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(saturate);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(sign);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(sin);

    add_func(smoothstep_Float,
             "smoothstep",
             {{"min", floatT}, {"max", floatT}, {"value", floatT}},
             *floatT);

    add_func(smoothstep_Vector2,
             "smoothstep",
             {{"min", vector2T}, {"max", vector2T}, {"value", vector2T}},
             *vector2T);

    add_func(smoothstep_Vector3,
             "smoothstep",
             {{"min", vector3T}, {"max", vector3T}, {"value", vector3T}},
             *vector3T);

    add_func(smoothstep_Vector4,
             "smoothstep",
             {{"min", vector4T}, {"max", vector4T}, {"value", vector4T}},
             *vector4T);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(sqrt);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(tan);

    add_func(transpose_matrix, "transpose", {{"matrix", matrixT}}, *matrixT);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(trunc);

    add_struct_fields(m_all,
                      vector2_fields,
                      {
                          // XY accessors
                          std::make_pair("x", floatT),
                          std::make_pair("y", floatT),
                          std::make_pair("xx", vector2T),
                          std::make_pair("yy", vector2T),
                      });

    add_struct_fields(m_all,
                      vector3_fields,
                      {
                          // XYZ accessors
                          std::make_pair("x", floatT),
                          std::make_pair("y", floatT),
                          std::make_pair("z", floatT),
                          std::make_pair("xx", vector2T),
                          std::make_pair("yy", vector2T),
                          std::make_pair("zz", vector2T),
                          std::make_pair("xy", vector2T),
                          std::make_pair("yx", vector2T),
                          std::make_pair("yz", vector2T),
                          std::make_pair("zy", vector2T),
                          std::make_pair("xz", vector2T),
                          std::make_pair("zx", vector2T),
                          std::make_pair("xxx", vector3T),
                          std::make_pair("yyy", vector3T),
                          std::make_pair("zzz", vector3T),
                      });

    add_struct_fields(m_all,
                      vector4_fields,
                      {
                          // XYZW accessors
                          std::make_pair("x", floatT),
                          std::make_pair("y", floatT),
                          std::make_pair("z", floatT),
                          std::make_pair("w", floatT),
                          std::make_pair("xy", vector2T),
                          std::make_pair("xyz", vector3T),
                          std::make_pair("xxxx", vector4T),
                          std::make_pair("yyyy", vector4T),
                          std::make_pair("zzzz", vector4T),
                          std::make_pair("wwww", vector4T),
                      });

    add_system_value(sprite_image, naming::sprite_batch_image_param, *image_t);
    add_system_value(sprite_color, naming::sprite_batch_color_attrib, *vector4T);
    add_system_value(sprite_uv, naming::sprite_batch_uv_attrib, *vector2T);
}

BuiltInSymbols::BuiltInSymbols(BuiltInSymbols&&) noexcept = default;

BuiltInSymbols& BuiltInSymbols::operator=(BuiltInSymbols&&) noexcept = default; // NOLINT

BuiltInSymbols::~BuiltInSymbols() noexcept = default;

bool BuiltInSymbols::contains(const Decl& symbol) const
{
    return std::ranges::find(m_all, &symbol) != m_all.cend();
}

bool BuiltInSymbols::is_general_image_sampling_function(const Decl& symbol) const
{
    return is_non_mipmapped_image_sampling_function(symbol) ||
           is_mipmapped_image_sampling_function(symbol);
}

bool BuiltInSymbols::is_non_mipmapped_image_sampling_function(const Decl& symbol) const
{
    return &symbol == sample_image.get();
}

bool BuiltInSymbols::is_mipmapped_image_sampling_function(const Decl& symbol) const
{
    return &symbol == sample_level_image.get();
}

bool BuiltInSymbols::accepts_implicitly_cast_arguments(const FunctionDecl& function) const
{
    return is_some_vector_ctor(function);
}

bool BuiltInSymbols::is_float_ctor(const Decl& symbol) const
{
    return &symbol == float_ctor_int.get() || &symbol == float_ctor_uint.get();
}

bool BuiltInSymbols::is_int_ctor(const Decl& symbol) const
{
    return &symbol == int_ctor_float.get() || &symbol == int_ctor_uint.get();
}

bool BuiltInSymbols::is_uint_ctor(const Decl& symbol) const
{
    return &symbol == uint_ctor_float.get() || &symbol == uint_ctor_int.get();
}

bool BuiltInSymbols::is_some_vector_ctor(const Decl& symbol) const
{
    return is_vector2_ctor(symbol) || is_vector3_ctor(symbol) || is_vector4_ctor(symbol);
}

bool BuiltInSymbols::is_vector2_ctor(const Decl& symbol) const
{
    return &symbol == vector2_ctor_x_y.get() || &symbol == vector2_ctor_xy.get();
}

bool BuiltInSymbols::is_vector3_ctor(const Decl& symbol) const
{
    return &symbol == vector3_ctor_x_y_z.get() || &symbol == vector3_ctor_xy_z.get() ||
           &symbol == vector3_ctor_xyz.get();
}

bool BuiltInSymbols::is_vector4_ctor(const Decl& symbol) const
{
    return &symbol == vector4_ctor_x_y_z_w.get() || &symbol == vector4_ctor_xy_zw.get() ||
           &symbol == vector4_ctor_xy_z_w.get() || &symbol == vector4_ctor_xyz_w.get() ||
           &symbol == vector4_ctor_xyzw.get();
}

bool BuiltInSymbols::is_some_intrinsic_function(const Decl& symbol) const
{
    return isa<FunctionDecl>(symbol) && !is_some_vector_ctor(symbol) &&
           std::ranges::find(m_all, &symbol) != m_all.cend();
}

bool BuiltInSymbols::is_vector_field_access(const Decl& symbol) const
{
    const auto check = [&symbol](const auto& list) {
        const auto it = std::find_if(list.cbegin(), list.cend(), [&symbol](const auto& e) {
            return e.get() == &symbol;
        });

        return it != list.cend();
    };

    return check(vector2_fields) || check(vector3_fields) || check(vector4_fields);
}

void BuiltInSymbols::add_func(
    std::unique_ptr<FunctionDecl>&                                                 var,
    std::string_view                                                               func_name,
    std::initializer_list<std::pair<std::string_view, gsl::not_null<const Type*>>> param_descs,
    const Type&                                                                    return_type)
{
    // The variable must not be initialized yet.
    assert(!var);

    SmallVector<std::unique_ptr<FunctionParamDecl>, 4> params;

    for (const auto& [param_name, param_type] : param_descs)
    {
        params.push_back(
            std::make_unique<FunctionParamDecl>(SourceLocation::std, param_name, *param_type));
    }

    var = std::make_unique<FunctionDecl>(SourceLocation::std,
                                         func_name,
                                         std::move(params),
                                         std::move(return_type),
                                         nullptr);

    m_all.emplace_back(var.get());
}

void BuiltInSymbols::add_system_value(std::unique_ptr<Decl>& var,
                                      std::string_view       name,
                                      const Type&            type)
{
    var = std::make_unique<VarDecl>(name, type);
    m_all.emplace_back(var.get());
}
} // namespace cer::shadercompiler
