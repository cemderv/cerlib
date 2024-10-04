// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
/**
 * Defines how a texture's data is interpolated when it is sampled in a shader.
 *
 * @ingroup Graphics
 */
enum class ImageFilter
{
    /** Use linear interpolation */
    Linear = 1,

    /** Use point (nearest neighbor) sampling */
    Point,
};

/**
 * Defines how a texture's data is wrapped when it is sampled in a shader.
 *
 * @ingroup Graphics
 */
enum class ImageAddressMode
{
    /**
     * Repeat the texture pattern by tiling it at every integer boundary.
     *
     * For example, when the texture is sampled across UV coordinates [0.0 .. 2.5],
     * the texture is repeated 2.5 times.
     */
    Repeat = 1,

    /**
     * Clamp the texture coordinates to the range [0.0 .. 1.0].
     *
     * Coordinates outside of this range result in the texture's border colors.
     */
    ClampToEdgeTexels = 2,

    /**
     * Clamp the texture coordinates to the range [0.0 .. 1.0].
     *
     * Coordinates outside of this range result in the border color that is specified by
     * the texture's sampler.
     */
    ClampToSamplerBorderColor = 3,

    /**
     * Flip the texture at every integer boundary.
     */
    Mirror = 4,
};

/**
 * Defines the resulting color if a texture is sampled outside its borders.
 *
 * @ingroup Graphics
 */
enum class SamplerBorderColor
{
    TransparentBlack = 1, /**< Transparent black (0, 0, 0, 0) for values outside the edge. */
    OpaqueBlack      = 2, /**< Opaque black (0, 0, 0, 1) for values outside the edge. */
    OpaqueWhite      = 3, /**< Opaque white (1, 1, 1, 1) for values outside the edge. */
};

/**
 * Defines how two values (source and destination) are compared.
 *
 * @ingroup Graphics
 */
enum class Comparison
{
    Never        = 1, /**< The comparison never passes. */
    Less         = 2, /**< The comparison passes if source < destination. */
    Equal        = 3, /**< The comparison passes if source == destination. */
    LessEqual    = 4, /**< The comparison passes if source <= destination. */
    Greater      = 5, /**< The comparison passes if source > destination. */
    NotEqual     = 6, /**< The comparison passes if source != destination. */
    GreaterEqual = 7, /**< The comparison passes if source >= destination. */
    Always       = 8, /**< The comparison always passes. */
};

/**
 * Represents an image sampler.
 *
 * @ingroup Graphics
 */
struct Sampler
{
    static constexpr auto point_repeat() -> Sampler;

    static constexpr auto point_clamp() -> Sampler;

    static constexpr auto linear_repeat() -> Sampler;

    static constexpr auto linear_clamp() -> Sampler;

    /** Default comparison */
    auto operator==(const Sampler&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Sampler&) const -> bool = default;

    ImageFilter        filter             = ImageFilter::Linear;
    ImageAddressMode   address_u          = ImageAddressMode::ClampToEdgeTexels;
    ImageAddressMode   address_v          = ImageAddressMode::ClampToEdgeTexels;
    Comparison         texture_comparison = Comparison::Never;
    SamplerBorderColor border_color       = SamplerBorderColor::OpaqueBlack;
};

constexpr auto Sampler::point_repeat() -> Sampler
{
    return {
        .filter    = ImageFilter::Point,
        .address_u = ImageAddressMode::Repeat,
        .address_v = ImageAddressMode::Repeat,
    };
}

constexpr auto Sampler::point_clamp() -> Sampler
{
    return {
        .filter    = ImageFilter::Point,
        .address_u = ImageAddressMode::ClampToEdgeTexels,
        .address_v = ImageAddressMode::ClampToEdgeTexels,
    };
}

constexpr auto Sampler::linear_repeat() -> Sampler
{
    return {
        .filter    = ImageFilter::Linear,
        .address_u = ImageAddressMode::Repeat,
        .address_v = ImageAddressMode::Repeat,
    };
}

constexpr auto Sampler::linear_clamp() -> Sampler
{
    return {
        .filter    = ImageFilter::Linear,
        .address_u = ImageAddressMode::ClampToEdgeTexels,
        .address_v = ImageAddressMode::ClampToEdgeTexels,
    };
}
} // namespace cer