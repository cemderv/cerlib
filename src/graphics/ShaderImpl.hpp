// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "GraphicsResourceImpl.hpp"
#include "ShaderParameter.hpp"
#include "util/InternalError.hpp"
#include "util/SmallVector.hpp"
#include <span>
#include <unordered_set>

namespace cer
{
class Image;
}

namespace cer::details
{
class GraphicsDevice;

class ShaderImpl : public GraphicsResourceImpl
{
    friend GraphicsDevice;

  public:
    using ParameterList     = SmallVector<ShaderParameter, 8>;
    using ParameterPtrsList = SmallVector<ShaderParameter*, 8>;

  protected:
    explicit ShaderImpl(gsl::not_null<GraphicsDevice*> parent_device, ParameterList parameters);

  public:
    NON_COPYABLE_NON_MOVABLE(ShaderImpl);

    ~ShaderImpl() noexcept override;

    static std::string shader_parameter_type_string(ShaderParameterType type);

    static void verify_parameter_read(std::string_view    parameter_name,
                                      ShaderParameterType dst_type,
                                      ShaderParameterType src_type);

    static void verify_parameter_assignment(std::string_view    parameter_name,
                                            ShaderParameterType dst_type,
                                            ShaderParameterType src_type);

    template <typename T>
    std::optional<T> read_parameter_data(std::string_view name, ShaderParameterType type) const
    {
        if (const auto param = find_parameter(name))
        {
            verify_parameter_read(name, param->type, type);
            return *reinterpret_cast<const T*>(m_cbuffer_data.data() + param->offset);
        }

        return std::optional<T>();
    }

    void update_parameter_image(std::string_view name, const Image& image);

    template <typename T>
    void update_parameter_scalar(std::string_view name, ShaderParameterType type, const T& src_data)
    {
        verify_parameter_update_condition();

        if (const auto param = find_parameter(name))
        {
            verify_parameter_assignment(name, param->type, type);

            const auto dst_data = reinterpret_cast<T*>(m_cbuffer_data.data() + param->offset);

            if (*dst_data == src_data)
                return;

            *dst_data = src_data;

            m_dirty_scalar_parameters.insert(param);
        }
    }

    template <typename T>
    void update_parameter_scalar_array(std::string_view    name,
                                       ShaderParameterType type,
                                       std::span<const T>  src_data,
                                       uint32_t            offset)
    {
        verify_parameter_update_condition();

        if (const auto param = find_parameter(name))
        {
            verify_parameter_assignment(name, param->type, type);

            const auto src_count = src_data.size();

            if (src_count + offset > param->array_size)
            {
                if (offset > 0)
                {
                    CER_THROW_INVALID_ARG(
                        "The number of specified values and offset (= {}+{}) exceeds the "
                        "parameter's array size (= {}).",
                        src_count,
                        offset,
                        param->array_size);
                }

                CER_THROW_INVALID_ARG(
                    "The number of specified values (= {}) exceeds the parameter's "
                    "array size (= {}).",
                    src_count,
                    param->array_size);
            }

            constexpr auto increment_per_element = ShaderParameter::array_element_base_alignment;

            auto dst_data = m_cbuffer_data.data() + param->offset;
            dst_data += (offset * increment_per_element);

            for (uint32_t i = 0; i < src_count; ++i, dst_data += increment_per_element)
                *reinterpret_cast<T*>(dst_data) = src_data[i];

            m_dirty_scalar_parameters.insert(param);
        }
    }

    ShaderParameter* find_parameter(std::string_view name);

    const ShaderParameter* find_parameter(std::string_view name) const;

    const std::unordered_set<const ShaderParameter*>& dirty_scalar_parameters() const;

    void clear_dirty_scalar_parameters();

    const std::unordered_set<const ShaderParameter*>& dirty_image_parameters() const;

    void clear_dirty_image_parameters();

    const uint8_t* cbuffer_data() const;

    uint32_t cbuffer_size() const;

    std::span<const ShaderParameter> all_parameters() const;

    std::span<ShaderParameter* const> image_parameters() const;

  private:
    void verify_parameter_update_condition();

    void set_default_parameter_values();

    SmallVector<uint8_t, 512>                  m_cbuffer_data;
    uint32_t                                   m_c_buffer_size{};
    ParameterList                              m_parameters;
    ParameterPtrsList                          m_image_parameters;
    std::unordered_set<const ShaderParameter*> m_dirty_scalar_parameters;
    std::unordered_set<const ShaderParameter*> m_dirty_image_parameters;
    bool                                       m_is_in_use{};
};
} // namespace cer::details