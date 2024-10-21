// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "GraphicsResourceImpl.hpp"
#include "ShaderParameter.hpp"
#include <cerlib/List.hpp>
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
    using ParameterList     = List<ShaderParameter, 8>;
    using ParameterPtrsList = List<ShaderParameter*, 8>;

  protected:
    explicit ShaderImpl(GraphicsDevice& parent_device, ParameterList parameters);

  public:
    forbid_copy_and_move(ShaderImpl);

    ~ShaderImpl() noexcept override;

    static auto shader_parameter_type_string(ShaderParameterType type) -> std::string;

    static void verify_parameter_read(std::string_view    parameter_name,
                                      ShaderParameterType dst_type,
                                      ShaderParameterType src_type);

    static void verify_parameter_assignment(std::string_view    parameter_name,
                                            ShaderParameterType dst_type,
                                            ShaderParameterType src_type);

    template <typename T>
    auto read_parameter_data(std::string_view name, ShaderParameterType type) const
        -> std::optional<T>
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
            {
                return;
            }

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
                    throw std::invalid_argument{fmt::format(
                        "The number of specified values and offset (= {}+{}) exceeds the "
                        "parameter's array size (= {}).",
                        src_count,
                        offset,
                        param->array_size)};
                }

                throw std::invalid_argument{
                    fmt::format("The number of specified values (= {}) exceeds the parameter's "
                                "array size (= {}).",
                                src_count,
                                param->array_size)};
            }

            constexpr auto increment_per_element = ShaderParameter::array_element_base_alignment;

            auto dst_data = m_cbuffer_data.data() + param->offset;
            dst_data += (offset * increment_per_element);

            for (uint32_t i = 0; i < src_count; ++i, dst_data += increment_per_element)
                *reinterpret_cast<T*>(dst_data) = src_data[i];

            m_dirty_scalar_parameters.insert(param);
        }
    }

    auto find_parameter(std::string_view name) -> ShaderParameter*;

    auto find_parameter(std::string_view name) const -> const ShaderParameter*;

    auto dirty_scalar_parameters() const -> const std::unordered_set<const ShaderParameter*>&;

    void clear_dirty_scalar_parameters();

    auto dirty_image_parameters() const -> const std::unordered_set<const ShaderParameter*>&;

    void clear_dirty_image_parameters();

    auto cbuffer_data() const -> const uint8_t*;

    auto cbuffer_size() const -> uint32_t;

    auto all_parameters() const -> std::span<const ShaderParameter>;

    auto image_parameters() const -> std::span<ShaderParameter* const>;

  private:
    void verify_parameter_update_condition();

    void set_default_parameter_values();

    List<uint8_t, 512>                         m_cbuffer_data;
    uint32_t                                   m_c_buffer_size{};
    ParameterList                              m_parameters;
    ParameterPtrsList                          m_image_parameters;
    std::unordered_set<const ShaderParameter*> m_dirty_scalar_parameters;
    std::unordered_set<const ShaderParameter*> m_dirty_image_parameters;
    bool                                       m_is_in_use{};
};
} // namespace cer::details