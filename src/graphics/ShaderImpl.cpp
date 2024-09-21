// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "ShaderImpl.hpp"
#include "CBufferPacker.hpp"
#include "GraphicsDevice.hpp"
#include "cerlib/GraphicsResource.hpp"
#include "cerlib/Logging.hpp"
#include "shadercompiler/Type.hpp"
#include "util/InternalError.hpp"
#include "util/Util.hpp"

namespace cer::details
{
std::string ShaderImpl::shader_parameter_type_string(ShaderParameterType type)
{
    using namespace shadercompiler;

    switch (type)
    {
        case ShaderParameterType::Float: return std::string(FloatType::instance().type_name());
        case ShaderParameterType::Int: return std::string(IntType::instance().type_name());
        case ShaderParameterType::Bool: return std::string(BoolType::instance().type_name());
        case ShaderParameterType::Vector2: return std::string(Vector2Type::instance().type_name());
        case ShaderParameterType::Vector3: return std::string(Vector3Type::instance().type_name());
        case ShaderParameterType::Vector4: return std::string(Vector4Type::instance().type_name());
        case ShaderParameterType::Matrix: return std::string(MatrixType::instance().type_name());
        case ShaderParameterType::Image: return std::string(ImageType::instance().type_name());
        case ShaderParameterType::FloatArray:
            return std::format("{}[]", FloatType::instance().type_name());
        case ShaderParameterType::IntArray:
            return std::format("{}[]", IntType::instance().type_name());
        case ShaderParameterType::BoolArray:
            return std::format("{}[]", BoolType::instance().type_name());
        case ShaderParameterType::Vector2Array:
            return std::format("{}[]", Vector2Type::instance().type_name());
        case ShaderParameterType::Vector3Array:
            return std::format("{}[]", Vector3Type::instance().type_name());
        case ShaderParameterType::Vector4Array:
            return std::format("{}[]", Vector4Type::instance().type_name());
        case ShaderParameterType::MatrixArray:
            return std::format("{}[]", MatrixType::instance().type_name());
    }

    return {};
}

ShaderImpl::ShaderImpl(gsl::not_null<GraphicsDevice*> parent_device, ParameterList parameters)
    : GraphicsResourceImpl(parent_device, GraphicsResourceType::Shader)
    , m_parameters(std::move(parameters))
{
    constexpr uint16_t cbuffer_size_alignment               = 16;
    constexpr bool     take_max_of_param_size_and_alignment = true;

    const CBufferPacker::Result pack_result =
        CBufferPacker::pack_parameters(m_parameters,
                                       cbuffer_size_alignment,
                                       take_max_of_param_size_and_alignment);

    m_cbuffer_data.resize(pack_result.cbuffer_size);
    m_c_buffer_size = pack_result.cbuffer_size;

    // Because we use binary search to look up parameters, sort them here once.
    std::ranges::sort(m_parameters, [](const ShaderParameter& lhs, const ShaderParameter& rhs) {
        return lhs.name < rhs.name;
    });

    for (ShaderParameter& param : m_parameters)
    {
        if (param.is_image)
        {
            m_image_parameters.push_back(&param);
            m_dirty_image_parameters.insert(&param);
        }
        else
        {
            m_dirty_scalar_parameters.insert(&param);
        }
    }

    set_default_parameter_values();
}

ShaderImpl::~ShaderImpl() noexcept
{
    log_verbose("~ShaderImpl({})", name());
    parent_device().notify_user_shader_destroyed(this);
}

void ShaderImpl::verify_parameter_read(std::string_view    parameter_name,
                                       ShaderParameterType dst_type,
                                       ShaderParameterType src_type)
{
    if (dst_type != src_type)
    {
        CER_THROW_LOGIC_ERROR("Attempting to read value of parameter '{}' (type '{}') as "
                              "a value of type '{}'.",
                              parameter_name,
                              shader_parameter_type_string(src_type),
                              shader_parameter_type_string(dst_type));
    }
}

void ShaderImpl::verify_parameter_assignment(std::string_view    parameter_name,
                                             ShaderParameterType dst_type,
                                             ShaderParameterType src_type)
{
    if (dst_type != src_type)
    {
        CER_THROW_LOGIC_ERROR("Attempting to set value of parameter '{}' (type '{}') to "
                              "a value of type '{}'.",
                              parameter_name,
                              shader_parameter_type_string(dst_type),
                              shader_parameter_type_string(src_type));
    }
}

void ShaderImpl::update_parameter_image(std::string_view name, const Image& image)
{
    verify_parameter_update_condition();

    if (ShaderParameter* param = find_parameter(name))
    {
        if (!param->is_image)
        {
            CER_THROW_LOGIC_ERROR(
                "Attempting to set value of parameter '{}' (type '{}') to an image.",
                name,
                shader_parameter_type_string(param->type));
        }

        if (param->image != image)
        {
            param->image = image;
            m_dirty_image_parameters.insert(param);
        }
    }
}

ShaderParameter* ShaderImpl::find_parameter(std::string_view name)
{
    const auto it = binary_find(m_parameters.begin(), m_parameters.end(), name);
    return it != m_parameters.end() ? &*it : nullptr;
}

const ShaderParameter* ShaderImpl::find_parameter(std::string_view name) const
{
    const auto it = binary_find(m_parameters.begin(), m_parameters.end(), name);
    return it != m_parameters.end() ? &*it : nullptr;
}

const std::unordered_set<const ShaderParameter*>& ShaderImpl::dirty_scalar_parameters() const
{
    return m_dirty_scalar_parameters;
}

void ShaderImpl::clear_dirty_scalar_parameters()
{
    m_dirty_scalar_parameters.clear();
}

const std::unordered_set<const ShaderParameter*>& ShaderImpl::dirty_image_parameters() const
{
    return m_dirty_image_parameters;
}

void ShaderImpl::clear_dirty_image_parameters()
{
    m_dirty_image_parameters.clear();
}

const uint8_t* ShaderImpl::cbuffer_data() const
{
    return m_cbuffer_data.data();
}

uint32_t ShaderImpl::cbuffer_size() const
{
    return m_c_buffer_size;
}

std::span<const ShaderParameter> ShaderImpl::all_parameters() const
{
    return m_parameters;
}

std::span<ShaderParameter* const> ShaderImpl::image_parameters() const
{
    return m_image_parameters;
}

void ShaderImpl::verify_parameter_update_condition()
{
    // Currently, we don't allow updating parameter values while a shader is in use.
    if (m_is_in_use)
    {
        CER_THROW_RUNTIME_ERROR_STR(
            "Shader parameters may not be updated while the shader is in use. Please unset "
            "the "
            "shader first, or update the parameters before setting the shader as active.");
    }
}

void ShaderImpl::set_default_parameter_values()
{
    for (ShaderParameter& param : m_parameters)
    {
        switch (param.type)
        {
            case ShaderParameterType::Float: {
                // Special case here: in the shader compiler, "floats" are stored as
                // doubles. We have to convert those values correctly, otherwise we get a
                // bad_any_cast exception.
                const float value =
                    param.default_value.has_value()
                        ? static_cast<float>(std::any_cast<double>(param.default_value))
                        : 0.0f;

                update_parameter_scalar(param.name, param.type, value);
                break;
            }
            case ShaderParameterType::Int:
                update_parameter_scalar(param.name,
                                        param.type,
                                        param.default_value.has_value()
                                            ? std::any_cast<int32_t>(param.default_value)
                                            : 0);
                break;
            case ShaderParameterType::Bool:
                update_parameter_scalar(param.name,
                                        param.type,
                                        param.default_value.has_value()
                                            ? std::any_cast<bool>(param.default_value)
                                            : false);
                break;
            case ShaderParameterType::Vector2:
                update_parameter_scalar(param.name,
                                        param.type,
                                        param.default_value.has_value()
                                            ? std::any_cast<Vector2>(param.default_value)
                                            : Vector2());
                break;
            case ShaderParameterType::Vector3:
                update_parameter_scalar(param.name,
                                        param.type,
                                        param.default_value.has_value()
                                            ? std::any_cast<Vector3>(param.default_value)
                                            : Vector3());
                break;
            case ShaderParameterType::Vector4:
                update_parameter_scalar(param.name,
                                        param.type,
                                        param.default_value.has_value()
                                            ? std::any_cast<Vector4>(param.default_value)
                                            : Vector4());
                break;
            case ShaderParameterType::Matrix:
                update_parameter_scalar(param.name,
                                        param.type,
                                        param.default_value.has_value()
                                            ? std::any_cast<Matrix>(param.default_value)
                                            : Matrix());
                break;
            case ShaderParameterType::Image: {
                break;
            }
            default: break;
        }
    }
}
} // namespace cer::details