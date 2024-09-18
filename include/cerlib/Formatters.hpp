// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

// std::formatter specializations for cerlib types

#pragma once

#include <cerlib/Color.hpp>
#include <cerlib/Image.hpp>
#include <cerlib/Matrix.hpp>
#include <cerlib/Rectangle.hpp>
#include <cerlib/Vector2.hpp>
#include <cerlib/Vector3.hpp>
#include <cerlib/Vector4.hpp>
#include <format>

template <>
struct std::formatter<cer::Color>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Color& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "[{}; {}; {}; {}]", value.r, value.g, value.b, value.a);
    }
};

template <>
struct std::formatter<cer::Vector2>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Vector2& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "[{}; {}]", value.x, value.y);
    }
};

template <>
struct std::formatter<cer::Vector3>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Vector3& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "[{}; {}; {}]", value.x, value.y, value.z);
    }
};

template <>
struct std::formatter<cer::Vector4>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Vector4& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "[{}; {}; {}; {}]", value.x, value.y, value.z, value.w);
    }
};

template <>
struct std::formatter<cer::Matrix>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Matrix& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              R"([
  {}; {}; {}; {}
  {}; {}; {}; {}
  {}; {}; {}; {}
  {}; {}; {}; {}
])",
                              value.m11,
                              value.m12,
                              value.m13,
                              value.m14,
                              value.m21,
                              value.m22,
                              value.m23,
                              value.m24,
                              value.m31,
                              value.m32,
                              value.m33,
                              value.m34,
                              value.m41,
                              value.m42,
                              value.m43,
                              value.m44);
    }
};

template <>
struct std::formatter<cer::ImageFormat>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(cer::ImageFormat value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", cer::image_format_name(value));
    }
};

template <>
struct std::formatter<cer::Image>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Image& value, FormatContext& ctx) const
    {
        if (value)
        {
            const auto name    = value.name();
            const auto mipmaps = value.mipmap_count();

            return std::format_to(ctx.out(),
                                  "['{}'; {}x{}; {}; {} {}]",
                                  name.empty() ? "<unnamed>" : name,
                                  value.width(),
                                  value.height(),
                                  value.format(),
                                  mipmaps,
                                  mipmaps > 1 ? "mipmaps" : "mipmap");
        }
        else
        {
            return std::format_to(ctx.out(), "<none>");
        }
    }
};

template <>
struct std::formatter<cer::Rectangle>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const cer::Rectangle& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "[{}; {}; {}x{}]",
                              value.x,
                              value.y,
                              value.width,
                              value.height);
    }
};