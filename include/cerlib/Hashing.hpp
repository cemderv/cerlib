// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

// std::hash specializations of cerlib types

#pragma once

#include <cerlib/BlendState.hpp>
#include <cerlib/Circle.hpp>
#include <cerlib/Color.hpp>
#include <cerlib/Font.hpp>
#include <cerlib/GraphicsResource.hpp>
#include <cerlib/Image.hpp>
#include <cerlib/Matrix.hpp>
#include <cerlib/Rectangle.hpp>
#include <cerlib/Sampler.hpp>
#include <cerlib/Shader.hpp>
#include <cerlib/Sound.hpp>
#include <cerlib/SoundChannel.hpp>
#include <cerlib/SoundTypes.hpp>
#include <cerlib/Vector2.hpp>
#include <cerlib/Vector3.hpp>
#include <cerlib/Vector4.hpp>
#include <cerlib/Window.hpp>
#include <functional>

namespace cer::details
{
static void hash_combine(std::size_t&)
{
    // Nothing to do here.
}

template <typename T, typename... Rest>
static void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
    const std::hash<T> hasher{};
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
}
} // namespace cer::details

namespace std
{
template <>
struct hash<cer::Vector2>
{
    auto operator()(const cer::Vector2& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed, value.x, value.y);
        return seed;
    }
};

template <>
struct hash<cer::Vector3>
{
    auto operator()(const cer::Vector3& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed, value.x, value.y, value.z);
        return seed;
    }
};

template <>
struct hash<cer::Vector4>
{
    auto operator()(const cer::Vector4& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed, value.x, value.y, value.z);
        return seed;
    }
};

template <>
struct hash<cer::Matrix>
{
    auto operator()(const cer::Matrix& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed,
                                   value.m11,
                                   value.m12,
                                   value.m13,
                                   value.m14,
                                   value.m21,
                                   value.m22,
                                   value.m33,
                                   value.m44,
                                   value.m21,
                                   value.m22,
                                   value.m33,
                                   value.m44,
                                   value.m21,
                                   value.m22,
                                   value.m33,
                                   value.m44);
        return seed;
    }
};

template <>
struct hash<cer::Circle>
{
    auto operator()(const cer::Circle& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed, value.center, value.radius);
        return seed;
    }
};

template <>
struct hash<cer::Color>
{
    auto operator()(const cer::Color& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed, value.r, value.g, value.b, value.a);
        return seed;
    }
};

template <>
struct hash<cer::Font>
{
    auto operator()(const cer::Font& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::GraphicsResource>
{
    auto operator()(const cer::GraphicsResource& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::Image>
{
    auto operator()(const cer::Image& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::Rectangle>
{
    auto operator()(const cer::Rectangle& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed, value.x, value.y, value.width, value.height);
        return seed;
    }
};

template <>
struct hash<cer::Sampler>
{
    auto operator()(const cer::Sampler& value) const noexcept -> size_t
    {
        size_t seed{};
        cer::details::hash_combine(seed,
                                   int(value.filter),
                                   int(value.address_u),
                                   int(value.address_v),
                                   int(value.texture_comparison),
                                   int(value.border_color));
        return seed;
    }
};

template <>
struct hash<cer::Shader>
{
    auto operator()(const cer::Shader& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::Sound>
{
    auto operator()(const cer::Sound& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::SoundChannel>
{
    auto operator()(const cer::SoundChannel& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::Window>
{
    auto operator()(const cer::Window& value) const noexcept -> size_t
    {
        size_t seed{};
        hash_combine(seed, value.impl());
        return seed;
    }
};

template <>
struct hash<cer::BlendState>
{
    auto operator()(const cer::BlendState& value) const noexcept -> size_t
    {
        size_t seed{};

        cer::details::hash_combine(seed,
                                   value.blending_enabled,
                                   value.blend_factor,
                                   int(value.color_blend_function),
                                   int(value.color_src_blend),
                                   int(value.color_dst_blend),
                                   int(value.alpha_blend_function),
                                   int(value.alpha_src_blend),
                                   int(value.alpha_dst_blend),
                                   int(value.color_write_mask));

        return seed;
    }
};
} // namespace std
