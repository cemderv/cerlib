// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
/**
 * Defines which RGBA channels of the render target can be written to when
 * drawing.
 *
 * Values can be combined to allow for writes to more than one channel.
 *
 * @ingroup Graphics
 */
enum class ColorWriteMask
{
    None  = 0, /**< Color writes are disabled for all channels. */
    Red   = 1, /**< Allow writes to the red channel. */
    Green = 2, /**< Allow writes to the green channel. */
    Blue  = 4, /**< Allow writes to the blue channel. */
    Alpha = 8, /**< Allow writes to the alpha channel. */
    All   = Red | Green | Blue | Alpha, /**< Allow writes to all RGBA channels. */
};

static auto operator&(ColorWriteMask lhs, ColorWriteMask rhs) -> ColorWriteMask
{
    return ColorWriteMask(int(lhs) & int(rhs));
}

static auto operator|(ColorWriteMask lhs, ColorWriteMask rhs) -> ColorWriteMask
{
    return ColorWriteMask(int(lhs) | int(rhs));
}

/**
 * Defines how a source color is combined with a destination color.
 * The source color is the resulting color of a pixel shader.
 * The destination color is the color that is already present in a render target.
 *
 * @ingroup Graphics
 */
enum class BlendFunction
{
    /**
     * Result = (SourceColor * SourceBlend) + (DestinationColor * DestinationBlend)
     */
    Add,

    /**
     * Result = (SourceColor * SourceBlend) - (DestinationColor * DestinationBlend)
     */
    Subtract,

    /**
     * Result = (DestinationColor * DestinationBlend) - (SourceColor * SourceBlend)
     */
    ReverseSubtract,

    /**
     * Result = min( (SourceColor * SourceBlend), (DestinationColor * DestinationBlend) )
     */
    Min,

    /**
     * Result = max( (SourceColor * SourceBlend), (DestinationColor * DestinationBlend) )
     */
    Max,
};

/**
 * Defines various color blending factors.
 *
 * @ingroup Graphics
 */
enum class Blend
{
    /** Result = (SourceColor * SourceBlend) + (DestinationColor * DestinationBlend) */
    One,

    /** Each component is multiplied by zero. */
    Zero,

    /** Each component is multiplied by the source color. */
    SourceColor,

    /** Each component is multiplied by the inverse of the source color. */
    InverseSourceColor,

    /** Each component is multiplied by the alpha value of the source color. */
    SourceAlpha,

    /** Each component is multiplied by the inverse alpha value of the source color. */
    InverseSourceAlpha,

    /** Each component is multiplied by the destination color. */
    DestColor,

    /** Each component is multiplied by the inverse of the destination color. */
    InverseDestColor,

    /** Each component is multiplied by the inverse of the destination color. */
    DestAlpha,

    /** Each component is multiplied by the inverse alpha value of the destination color. */
    InverseDestAlpha,

    /** Each component is multiplied by the color specified using `cer::set_blend_factor()`. */
    BlendFactor,

    /** Each component is multiplied by the inverse of the color specified using
     * cer::set_blend_factor(). */
    InverseBlendFactor,

    /**
     * Each component is multiplied by the greater value between the alpha value of the
     * source color and the inverse alpha value of the source color.
     */
    SourceAlphaSaturation,
};

/**
 * Represents a state that describes how a source pixel is blended with a
 * destination pixel to form a final output color.
 *
 * The source color is the color that is returned from a shader (i.e. sprite color).
 * The destination color is the color that is already stored in the render target (i.e.
 * canvas or window surface).
 *
 * @ingroup Graphics
 */
struct BlendState
{
    /**
     * Gets a blend state with alpha-blending disabled.
     * The source color overwrites the destination color.
     */
    static constexpr auto opaque() -> BlendState;

    /**
     * Gets a blend state with alpha-blending enabled.
     * The state assumes that the RGB channels have been premultiplied with the alpha
     * channel.
     */
    static constexpr auto alpha_blend() -> BlendState;

    /**
     * A blend state with alpha-blending enabled.
     * The state assumes that the RGB channels haven't been premultiplied with the alpha
     * channel.
     */
    static constexpr auto non_premultiplied() -> BlendState;

    /**
     * Gets a blend state with alpha-blending enabled.
     * The source color is added onto the destination color.
     */
    static constexpr auto additive() -> BlendState;

    /** Default comparison */
    auto operator==(const BlendState&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const BlendState&) const -> bool = default;

    /** If true, the state allows alpha blending. */
    bool blending_enabled = false;

    /** */
    Color blend_factor = white;

    /** */
    BlendFunction color_blend_function = BlendFunction::Add;

    /** */
    Blend color_src_blend = Blend::One;

    /** */
    Blend color_dst_blend = Blend::Zero;

    /** */
    BlendFunction alpha_blend_function = BlendFunction::Add;

    /** */
    Blend alpha_src_blend = Blend::One;

    /** */
    Blend alpha_dst_blend = Blend::Zero;

    /** */
    ColorWriteMask color_write_mask = ColorWriteMask::All;
};

constexpr auto BlendState::opaque() -> BlendState
{
    return {
        .blending_enabled = false,
        .blend_factor     = {1, 1, 1, 1},
        .color_src_blend  = Blend::One,
        .color_dst_blend  = Blend::Zero,
        .alpha_src_blend  = Blend::One,
        .alpha_dst_blend  = Blend::Zero,
    };
};

constexpr auto BlendState::alpha_blend() -> BlendState
{
    return {
        .blending_enabled = true,
        .blend_factor     = {1, 1, 1, 1},
        .color_src_blend  = Blend::One,
        .color_dst_blend  = Blend::InverseSourceAlpha,
        .alpha_src_blend  = Blend::One,
        .alpha_dst_blend  = Blend::InverseSourceAlpha,
    };
};

constexpr auto BlendState::non_premultiplied() -> BlendState
{
    return {
        .blending_enabled = true,
        .blend_factor     = Color{1, 1, 1, 1},
        .color_src_blend  = Blend::SourceAlpha,
        .color_dst_blend  = Blend::InverseSourceAlpha,
        .alpha_src_blend  = Blend::SourceAlpha,
        .alpha_dst_blend  = Blend::InverseSourceAlpha,
    };
};

constexpr auto BlendState::additive() -> BlendState
{
    return {
        .blending_enabled = true,
        .blend_factor     = Color{1, 1, 1, 1},
        .color_src_blend  = Blend::SourceAlpha,
        .color_dst_blend  = Blend::One,
        .alpha_src_blend  = Blend::SourceAlpha,
        .alpha_dst_blend  = Blend::One,
    };
};
} // namespace cer