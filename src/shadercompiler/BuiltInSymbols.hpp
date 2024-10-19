// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/Decl.hpp"
#include "util/NonCopyable.hpp"
#include "util/small_vector.hpp"
#include <gsl/pointers>
#include <memory>
#include <span>
#include <vector>

#define DECLARE_FUNC_FOR_ALL_VECTORS(name)                                                         \
    bool is_##name##_function(const Decl& symbol) const                                            \
    {                                                                                              \
        return &symbol == name##_Vector2.get() || &symbol == name##_Vector3.get() ||               \
               &symbol == name##_Vector4.get();                                                    \
    }                                                                                              \
    std::unique_ptr<FunctionDecl> name##_Vector2;                                                  \
    std::unique_ptr<FunctionDecl> name##_Vector3;                                                  \
    std::unique_ptr<FunctionDecl> name##_Vector4


#define DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(name)                                                    \
    bool is_##name##_function(const Decl& symbol) const                                            \
    {                                                                                              \
        return &symbol == name##_Float.get() || &symbol == name##_Vector2.get() ||                 \
               &symbol == name##_Vector3.get() || &symbol == name##_Vector4.get();                 \
    }                                                                                              \
    std::unique_ptr<FunctionDecl> name##_Float;                                                    \
    std::unique_ptr<FunctionDecl> name##_Vector2;                                                  \
    std::unique_ptr<FunctionDecl> name##_Vector3;                                                  \
    std::unique_ptr<FunctionDecl> name##_Vector4


#define DECLARE_FUNC_FOR_FLOAT_TO_MAT(name)                                                        \
    bool is_##name##_function(const Decl& symbol) const                                            \
    {                                                                                              \
        return &symbol == name##_Float.get() || &symbol == name##_Vector2.get() ||                 \
               &symbol == name##_Vector3.get() || &symbol == name##_Vector4.get() ||               \
               &symbol == name##_Matrix4x4.get();                                                  \
    }                                                                                              \
    std::unique_ptr<FunctionDecl> name##_Float;                                                    \
    std::unique_ptr<FunctionDecl> name##_Vector2;                                                  \
    std::unique_ptr<FunctionDecl> name##_Vector3;                                                  \
    std::unique_ptr<FunctionDecl> name##_Vector4;                                                  \
    std::unique_ptr<FunctionDecl> name##_Matrix4x4


namespace cer::shadercompiler
{
class Decl;
class FunctionDecl;
class Scope;
class Type;
class StructDecl;

/**
 * Represents symbols that are built into the shading language and implicitly
 * available.
 */
class BuiltInSymbols final
{
  public:
    BuiltInSymbols();

    NON_COPYABLE(BuiltInSymbols);

    BuiltInSymbols(BuiltInSymbols&&) noexcept;

    auto operator=(BuiltInSymbols&&) noexcept -> BuiltInSymbols&;

    ~BuiltInSymbols() noexcept;

    auto contains(const Decl& symbol) const -> bool;

    auto is_image_sampling_function(const Decl& symbol) const -> bool;

    auto accepts_implicitly_cast_arguments(const FunctionDecl& function) const -> bool;

    auto is_float_ctor(const Decl& symbol) const -> bool;

    auto is_int_ctor(const Decl& symbol) const -> bool;

    auto is_uint_ctor(const Decl& symbol) const -> bool;

    auto is_some_vector_ctor(const Decl& symbol) const -> bool;

    auto is_vector2_ctor(const Decl& symbol) const -> bool;

    auto is_vector3_ctor(const Decl& symbol) const -> bool;

    auto is_vector4_ctor(const Decl& symbol) const -> bool;

    auto is_some_intrinsic_function(const Decl& symbol) const -> bool;

    auto is_vector_field_access(const Decl& symbol) const -> bool;

    // clang-tidy is complaining about public member variables, which is fine.
    // In this case, we know what we're doing and are treating these variables as
    // read-only.
    // TODO: We can introduce and modify macros to declare a private member and its public
    // accessor for us.

    // NOLINTBEGIN

    std::unique_ptr<FunctionDecl> float_ctor_int;
    std::unique_ptr<FunctionDecl> float_ctor_uint;
    std::unique_ptr<FunctionDecl> int_ctor_float;
    std::unique_ptr<FunctionDecl> int_ctor_uint;
    std::unique_ptr<FunctionDecl> uint_ctor_float;
    std::unique_ptr<FunctionDecl> uint_ctor_int;

    std::unique_ptr<FunctionDecl> vector2_ctor_x_y;
    std::unique_ptr<FunctionDecl> vector2_ctor_xy;

    std::unique_ptr<FunctionDecl> vector3_ctor_x_y_z;
    std::unique_ptr<FunctionDecl> vector3_ctor_xy_z;
    std::unique_ptr<FunctionDecl> vector3_ctor_xyz;

    std::unique_ptr<FunctionDecl> vector4_ctor_x_y_z_w;
    std::unique_ptr<FunctionDecl> vector4_ctor_xy_zw;
    std::unique_ptr<FunctionDecl> vector4_ctor_xy_z_w;
    std::unique_ptr<FunctionDecl> vector4_ctor_xyz_w;
    std::unique_ptr<FunctionDecl> vector4_ctor_xyzw;

    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(abs);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(acos);
    DECLARE_FUNC_FOR_FLOAT_TO_MAT(all);
    DECLARE_FUNC_FOR_FLOAT_TO_MAT(any);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(asin);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(atan);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(atan2);
    DECLARE_FUNC_FOR_FLOAT_TO_MAT(ceil);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(clamp);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(cos);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(degrees);

    std::unique_ptr<FunctionDecl> determinant_matrix;

    DECLARE_FUNC_FOR_ALL_VECTORS(distance);
    DECLARE_FUNC_FOR_ALL_VECTORS(dot);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(exp);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(exp2);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(floor);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(fmod);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(frac);

    DECLARE_FUNC_FOR_ALL_VECTORS(length);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(lerp);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(log);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(log2);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(max);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(min);
    DECLARE_FUNC_FOR_ALL_VECTORS(normalize);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(pow);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(radians);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(round);

    std::unique_ptr<FunctionDecl> sample_image;

    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(saturate);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(sign);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(sin);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(smoothstep);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(sqrt);
    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(tan);

    std::unique_ptr<FunctionDecl> transpose_matrix;

    DECLARE_FUNC_FOR_FLOAT_TO_VECTOR4(trunc);

    std::vector<std::unique_ptr<Decl>> vector2_fields;
    std::vector<std::unique_ptr<Decl>> vector3_fields;
    std::vector<std::unique_ptr<Decl>> vector4_fields;

    std::unique_ptr<Decl> sprite_image;
    std::unique_ptr<Decl> sprite_color;
    std::unique_ptr<Decl> sprite_uv;

    // NOLINTEND

    auto all_decls() const -> std::span<const gsl::not_null<Decl*>>
    {
        return m_all;
    }

  private:
    void add_func(
        std::unique_ptr<FunctionDecl>&                                                 var,
        std::string_view                                                               func_name,
        std::initializer_list<std::pair<std::string_view, gsl::not_null<const Type*>>> param_descs,
        const Type&                                                                    return_type);

    void add_system_value(std::unique_ptr<Decl>& var, std::string_view name, const Type& type);

    gch::small_vector<gsl::not_null<Decl*>, 132> m_all;
};
} // namespace cer::shadercompiler
