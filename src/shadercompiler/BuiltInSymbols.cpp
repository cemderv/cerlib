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

using namespace std::string_view_literals;

// NOLINTBEGIN

#define ADD_FUNC_FOR_FLOAT_TO_VECTOR4(name)                                                        \
    add_func(name##_Float, #name, {{"value", float_t}}, float_t);                                  \
    add_func(name##_Vector2, #name, {{"value", vector2_t}}, vector2_t);                            \
    add_func(name##_Vector3, #name, {{"value", vector3_t}}, vector3_t);                            \
    add_func(name##_Vector4, #name, {{"value", vector4_t}}, vector4_t)

#define ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(name, argname1, argname2)                           \
    add_func(name##_Float, #name, {{argname1, float_t}, {argname2, float_t}}, float_t);            \
    add_func(name##_Vector2, #name, {{argname1, vector2_t}, {argname2, vector2_t}}, vector2_t);    \
    add_func(name##_Vector3, #name, {{argname1, vector3_t}, {argname2, vector3_t}}, vector3_t);    \
    add_func(name##_Vector4, #name, {{argname1, vector4_t}, {argname2, vector4_t}}, vector4_t)

#define ADD_FUNC_FOR_ALL_VECTORS(name)                                                             \
    add_func(name##_Vector2, #name, {{"value", vector2_t}}, vector2_t);                            \
    add_func(name##_Vector3, #name, {{"value", vector3_t}}, vector3_t);                            \
    add_func(name##_Vector4, #name, {{"value", vector4_t}}, vector4_t)

#define ADD_FUNC_FOR_ALL_VECTORS_TWO_ARGS(name, argname1, argname2)                                \
    add_func(name##_Vector2, #name, {{argname1, vector2_t}, {argname2, vector2_t}}, vector2_t);    \
    add_func(name##_Vector3, #name, {{argname1, vector3_t}, {argname2, vector3_t}}, vector3_t);    \
    add_func(name##_Vector4, #name, {{argname1, vector4_t}, {argname2, vector4_t}}, vector4_t)

#define ADD_FUNC_FOR_FLOAT_TO_MATRIX(name)                                                         \
    add_func(name##_Float, #name, {{"value", float_t}}, float_t);                                  \
    add_func(name##_Vector2, #name, {{"value", vector2_t}}, vector2_t);                            \
    add_func(name##_Vector3, #name, {{"value", vector3_t}}, vector3_t);                            \
    add_func(name##_Vector4, #name, {{"value", vector4_t}}, vector4_t);                            \
    add_func(name##_Matrix4x4, #name, {{"value", matrix_t}}, matrix_t)

#define ADD_FUNC_FOR_FLOAT_TO_MATRIX_BOOL(name)                                                    \
    add_func(name##_Float, #name, {{"value", float_t}}, bool_t);                                   \
    add_func(name##_Vector2, #name, {{"value", vector2_t}}, bool_t);                               \
    add_func(name##_Vector3, #name, {{"value", vector3_t}}, bool_t);                               \
    add_func(name##_Vector4, #name, {{"value", vector4_t}}, bool_t);                               \
    add_func(name##_Matrix4x4, #name, {{"value", matrix_t}}, bool_t)

// NOLINTEND

namespace cer::shadercompiler
{
static void add_struct_fields(
    RefList<Decl, 132>&                                                       all,
    List<std::unique_ptr<Decl>>&                                                    var,
    std::initializer_list<std::pair<std::string_view, std::reference_wrapper<const Type>>> fields)
{
    var.reserve(fields.size());

    for (const auto& [field_name, field_type] : fields)
    {
        var.push_back(
            std::make_unique<StructFieldDecl>(SourceLocation::std, field_name, field_type));

        all.emplace_back(*var.back());
    }
}

BuiltInSymbols::BuiltInSymbols()
{
    const auto& int_t     = IntType::instance();
    const auto& float_t   = FloatType::instance();
    const auto& vector2_t = Vector2Type::instance();
    const auto& vector3_t = Vector3Type::instance();
    const auto& vector4_t = Vector4Type::instance();
    const auto& matrix_t  = MatrixType::instance();
    const auto& image_t   = ImageType::instance();
    const auto& bool_t    = BoolType::instance();

    add_func(float_ctor_int, float_t.type_name(), {{"value", int_t}}, float_t);
    add_func(int_ctor_float, int_t.type_name(), {{"value", float_t}}, int_t);
    add_func(int_ctor_uint, int_t.type_name(), {{"value", float_t}}, int_t);

    // Vector2 ctors
    add_func(vector2_ctor_xy,
             vector2_t.type_name(),
             {
                 {"xy", float_t},
             },
             vector2_t);

    add_func(vector2_ctor_x_y,
             vector2_t.type_name(),
             {
                 {"x", float_t},
                 {"y", float_t},
             },
             vector2_t);

    // Vector3 ctors
    add_func(vector3_ctor_x_y_z,
             vector3_t.type_name(),
             {
                 {"x", float_t},
                 {"y", float_t},
                 {"z", float_t},
             },
             vector3_t);

    add_func(vector3_ctor_xy_z,
             vector3_t.type_name(),
             {
                 {"xy", vector2_t},
                 {"z", float_t},
             },
             vector3_t);

    add_func(vector3_ctor_xyz,
             vector3_t.type_name(),
             {
                 {"xyz", float_t},
             },
             vector3_t);

    // Vector4 ctors
    add_func(vector4_ctor_x_y_z_w,
             vector4_t.type_name(),
             {
                 {"x", float_t},
                 {"y", float_t},
                 {"z", float_t},
                 {"w", float_t},
             },
             vector4_t);

    add_func(vector4_ctor_xy_zw,
             vector4_t.type_name(),
             {
                 {"xy", vector2_t},
                 {"zw", vector2_t},
             },
             vector4_t);

    add_func(vector4_ctor_xy_z_w,
             vector4_t.type_name(),
             {
                 {"xy", vector2_t},
                 {"z", float_t},
                 {"w", float_t},
             },
             vector4_t);

    add_func(vector4_ctor_xyz_w,
             vector4_t.type_name(),
             {
                 {"xyz", vector3_t},
                 {"w", float_t},
             },
             vector4_t);

    add_func(vector4_ctor_xyzw,
             vector4_t.type_name(),
             {
                 {"xyzw", float_t},
             },
             vector4_t);

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
             {{"value", float_t}, {"start", float_t}, {"end", float_t}},
             float_t);

    add_func(clamp_Vector2,
             "clamp",
             {{"value", vector2_t}, {"start", vector2_t}, {"end", vector2_t}},
             vector2_t);

    add_func(clamp_Vector3,
             "clamp",
             {{"value", vector3_t}, {"start", vector3_t}, {"end", vector3_t}},
             vector3_t);

    add_func(clamp_Vector4,
             "clamp",
             {{"value", vector4_t}, {"start", vector4_t}, {"end", vector4_t}},
             vector4_t);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(cos);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(degrees);

    add_func(determinant_matrix, "determinant", {{"value", matrix_t}}, float_t);

    add_func(distance_Vector2, "distance", {{"lhs", vector2_t}, {"rhs", vector2_t}}, float_t);
    add_func(distance_Vector3, "distance", {{"lhs", vector3_t}, {"rhs", vector3_t}}, float_t);
    add_func(distance_Vector4, "distance", {{"lhs", vector4_t}, {"rhs", vector4_t}}, float_t);

    add_func(dot_Vector2, "dot", {{"lhs", vector2_t}, {"rhs", vector2_t}}, float_t);
    add_func(dot_Vector3, "dot", {{"lhs", vector3_t}, {"rhs", vector3_t}}, float_t);
    add_func(dot_Vector4, "dot", {{"lhs", vector4_t}, {"rhs", vector4_t}}, float_t);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(exp);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(exp2);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(floor);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(fmod, "x", "y");

    add_func(length_Vector2, "length", {{"value", vector2_t}}, float_t);
    add_func(length_Vector3, "length", {{"value", vector3_t}}, float_t);
    add_func(length_Vector4, "length", {{"value", vector4_t}}, float_t);

    add_func(lerp_Float, "lerp", {{"start", float_t}, {"stop", float_t}, {"t", float_t}}, float_t);
    add_func(lerp_Vector2,
             "lerp",
             {{"start", vector2_t}, {"stop", vector2_t}, {"t", float_t}},
             vector2_t);
    add_func(lerp_Vector3,
             "lerp",
             {{"start", vector3_t}, {"stop", vector3_t}, {"t", float_t}},
             vector3_t);
    add_func(lerp_Vector4,
             "lerp",
             {{"start", vector4_t}, {"stop", vector4_t}, {"t", float_t}},
             vector4_t);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(log);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(log2);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(max, "lhs", "rhs");
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(min, "lhs", "rhs");

    ADD_FUNC_FOR_ALL_VECTORS(normalize);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4_TWO_ARGS(pow, "x", "y");

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(radians);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(round);

    add_func(sample_image, "sample", {{"image", image_t}, {"coords", vector2_t}}, vector4_t);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(saturate);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(sign);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(sin);

    add_func(smoothstep_Float,
             "smoothstep",
             {{"min", float_t}, {"max", float_t}, {"value", float_t}},
             float_t);

    add_func(smoothstep_Vector2,
             "smoothstep",
             {{"min", vector2_t}, {"max", vector2_t}, {"value", vector2_t}},
             vector2_t);

    add_func(smoothstep_Vector3,
             "smoothstep",
             {{"min", vector3_t}, {"max", vector3_t}, {"value", vector3_t}},
             vector3_t);

    add_func(smoothstep_Vector4,
             "smoothstep",
             {{"min", vector4_t}, {"max", vector4_t}, {"value", vector4_t}},
             vector4_t);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(sqrt);
    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(tan);

    add_func(transpose_matrix, "transpose", {{"matrix", matrix_t}}, matrix_t);

    ADD_FUNC_FOR_FLOAT_TO_VECTOR4(trunc);

    add_struct_fields(m_all,
                      vector2_fields,
                      {
                          // XY accessors
                          {"x"sv, float_t},
                          {"y"sv, float_t},
                          {"xx"sv, vector2_t},
                          {"yy"sv, vector2_t},
                      });

    add_struct_fields(m_all,
                      vector3_fields,
                      {
                          // XYZ accessors
                          {"x"sv, float_t},
                          {"y"sv, float_t},
                          {"z"sv, float_t},
                          {"xx"sv, vector2_t},
                          {"yy"sv, vector2_t},
                          {"zz"sv, vector2_t},
                          {"xy"sv, vector2_t},
                          {"yx"sv, vector2_t},
                          {"yz"sv, vector2_t},
                          {"zy"sv, vector2_t},
                          {"xz"sv, vector2_t},
                          {"zx"sv, vector2_t},
                          {"xxx"sv, vector3_t},
                          {"yyy"sv, vector3_t},
                          {"zzz"sv, vector3_t},
                      });

    add_struct_fields(m_all,
                      vector4_fields,
                      {
                          // XYZW accessors
                          {"x"sv, float_t},
                          {"y"sv, float_t},
                          {"z"sv, float_t},
                          {"w"sv, float_t},
                          {"xy"sv, vector2_t},
                          {"xyz"sv, vector3_t},
                          {"xxxx"sv, vector4_t},
                          {"yyyy"sv, vector4_t},
                          {"zzzz"sv, vector4_t},
                          {"wwww"sv, vector4_t},
                      });

    add_system_value(sprite_image, naming::sprite_batch_image_param, image_t);
    add_system_value(sprite_color, naming::sprite_batch_color_attrib, vector4_t);
    add_system_value(sprite_uv, naming::sprite_batch_uv_attrib, vector2_t);
}

BuiltInSymbols::BuiltInSymbols(BuiltInSymbols&&) noexcept = default;

BuiltInSymbols& BuiltInSymbols::operator=(BuiltInSymbols&&) noexcept = default; // NOLINT

BuiltInSymbols::~BuiltInSymbols() noexcept = default;

auto BuiltInSymbols::contains(const Decl& symbol) const -> bool
{
    const auto it = std::ranges::find_if(m_all, [&symbol](const auto& e) {
        return &e.get() == &symbol;
    });

    return it != m_all.cend();
}

auto BuiltInSymbols::is_image_sampling_function(const Decl& symbol) const -> bool
{
    return &symbol == sample_image.get();
}

auto BuiltInSymbols::accepts_implicitly_cast_arguments(const FunctionDecl& function) const -> bool
{
    return is_some_vector_ctor(function);
}

auto BuiltInSymbols::is_float_ctor(const Decl& symbol) const -> bool
{
    return &symbol == float_ctor_int.get() || &symbol == float_ctor_uint.get();
}

auto BuiltInSymbols::is_int_ctor(const Decl& symbol) const -> bool
{
    return &symbol == int_ctor_float.get() || &symbol == int_ctor_uint.get();
}

auto BuiltInSymbols::is_uint_ctor(const Decl& symbol) const -> bool
{
    return &symbol == uint_ctor_float.get() || &symbol == uint_ctor_int.get();
}

auto BuiltInSymbols::is_some_vector_ctor(const Decl& symbol) const -> bool
{
    return is_vector2_ctor(symbol) || is_vector3_ctor(symbol) || is_vector4_ctor(symbol);
}

auto BuiltInSymbols::is_vector2_ctor(const Decl& symbol) const -> bool
{
    return &symbol == vector2_ctor_x_y.get() || &symbol == vector2_ctor_xy.get();
}

auto BuiltInSymbols::is_vector3_ctor(const Decl& symbol) const -> bool
{
    return &symbol == vector3_ctor_x_y_z.get() || &symbol == vector3_ctor_xy_z.get() ||
           &symbol == vector3_ctor_xyz.get();
}

auto BuiltInSymbols::is_vector4_ctor(const Decl& symbol) const -> bool
{
    return &symbol == vector4_ctor_x_y_z_w.get() || &symbol == vector4_ctor_xy_zw.get() ||
           &symbol == vector4_ctor_xy_z_w.get() || &symbol == vector4_ctor_xyz_w.get() ||
           &symbol == vector4_ctor_xyzw.get();
}

auto BuiltInSymbols::is_some_intrinsic_function(const Decl& symbol) const -> bool
{
    if (!isa<FunctionDecl>(symbol) || is_some_vector_ctor(symbol))
    {
        return false;
    }

    const auto it = std::ranges::find_if(m_all, [&symbol](const auto& e) {
        return &e.get() == &symbol;
    });

    return it != m_all.cend();
}

auto BuiltInSymbols::is_vector_field_access(const Decl& symbol) const -> bool
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
    std::unique_ptr<FunctionDecl>& var,
    std::string_view               func_name,
    std::initializer_list<std::pair<std::string_view, std::reference_wrapper<const Type>>>
                param_descs,
    const Type& return_type)
{
    // The variable must not be initialized yet.
    assert(!var);

    auto params = UniquePtrList<FunctionParamDecl, 4>{};

    for (const auto& [param_name, param_type] : param_descs)
    {
        params.push_back(
            std::make_unique<FunctionParamDecl>(SourceLocation::std, param_name, param_type));
    }

    var = std::make_unique<FunctionDecl>(SourceLocation::std,
                                         func_name,
                                         std::move(params),
                                         std::move(return_type),
                                         nullptr);

    m_all.emplace_back(*var);
}

void BuiltInSymbols::add_system_value(std::unique_ptr<Decl>& var,
                                      std::string_view       name,
                                      const Type&            type)
{
    var = std::make_unique<VarDecl>(name, type);
    m_all.emplace_back(*var);
}
} // namespace cer::shadercompiler
