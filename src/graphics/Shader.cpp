// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "GraphicsDevice.hpp"
#include "game/GameImpl.hpp"
#include "util/Util.hpp"
#include <cerlib/Shader.hpp>

#include "GrayscaleShader.shd.hpp"

#define DECLARE_SHADER_IMPL                                                                        \
    const auto impl = static_cast<details::ShaderImpl*>(this->impl());                             \
    VERIFY_IMPL_ACCESS

namespace cer
{
CERLIB_IMPLEMENT_DERIVED_OBJECT(GraphicsResource, Shader)

Shader::Shader(std::string_view                  name,
               std::string_view                  source_code,
               std::span<const std::string_view> defines)
{
    LOAD_DEVICE_IMPL;
    set_impl(*this, device_impl.demand_create_shader(name, source_code, defines).get());
}

void Shader::set_value(std::string_view name, float value)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Float, value);
}

void Shader::set_value(std::string_view name, int32_t value)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Int, value);
}

auto Shader::set_value(std::string_view name, bool value) -> void
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Bool, value);
}

void Shader::set_value(std::string_view name, Vector2 value)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Vector2, value);
}

void Shader::set_value(std::string_view name, Vector3 value)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Vector3, value);
}

void Shader::set_value(std::string_view name, Vector4 value)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Vector4, value);
}

void Shader::set_value(std::string_view name, const Matrix& value)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar(name, details::ShaderParameterType::Matrix, value);
}

void Shader::set_value(std::string_view name, std::span<const float> values, uint32_t offset)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar_array(name,
                                        details::ShaderParameterType::FloatArray,
                                        values,
                                        offset);
}

void Shader::set_value(std::string_view name, std::span<const int32_t> values, uint32_t offset)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar_array(name,
                                        details::ShaderParameterType::IntArray,
                                        values,
                                        offset);
}

void Shader::set_value(std::string_view name, std::span<const Vector2> values, uint32_t offset)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar_array(name,
                                        details::ShaderParameterType::Vector2Array,
                                        values,
                                        offset);
}

void Shader::set_value(std::string_view name, std::span<const Vector3> values, uint32_t offset)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar_array(name,
                                        details::ShaderParameterType::Vector3Array,
                                        values,
                                        offset);
}

void Shader::set_value(std::string_view name, std::span<const Vector4> values, uint32_t offset)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar_array(name,
                                        details::ShaderParameterType::Vector4Array,
                                        values,
                                        offset);
}

void Shader::set_value(std::string_view name, std::span<const Matrix> values, uint32_t offset)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_scalar_array(name,
                                        details::ShaderParameterType::MatrixArray,
                                        values,
                                        offset);
}

void Shader::set_value(std::string_view name, const Image& image)
{
    DECLARE_SHADER_IMPL;
    impl->update_parameter_image(name, image);
}

std::optional<float> Shader::float_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<float>(name, details::ShaderParameterType::Float);
}

std::optional<int32_t> Shader::int_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<int32_t>(name, details::ShaderParameterType::Int);
}

std::optional<bool> Shader::bool_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<bool>(name, details::ShaderParameterType::Bool);
}

std::optional<Vector2> Shader::vector2_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<Vector2>(name, details::ShaderParameterType::Vector2);
}

std::optional<Vector3> Shader::vector3_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<Vector3>(name, details::ShaderParameterType::Vector3);
}

std::optional<Vector4> Shader::vector4_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<Vector4>(name, details::ShaderParameterType::Vector4);
}

std::optional<Matrix> Shader::matrix_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<Matrix>(name, details::ShaderParameterType::Matrix);
}

std::optional<Image> Shader::image_value(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->read_parameter_data<Image>(name, details::ShaderParameterType::Image);
}

bool Shader::has_parameter(std::string_view name) const
{
    DECLARE_SHADER_IMPL;
    return impl->find_parameter(name) != nullptr;
}

Shader Shader::create_grayscale()
{
    return Shader{"cerlib_GrayscaleShader", GrayscaleShader_shd_string_view()};
}
} // namespace cer
