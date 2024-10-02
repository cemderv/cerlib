// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cerlib/GraphicsResource.hpp>
#include <cerlib/Image.hpp>
#include <cerlib/Matrix.hpp>
#include <cerlib/Vector2.hpp>
#include <cerlib/Vector3.hpp>
#include <cerlib/Vector4.hpp>
#include <span>
#include <string_view>

namespace cer
{
namespace details
{
class ShaderImpl;
}

/**
 * Represents a user-programmable pixel shader.
 * Used to customize the rendering of sprites.
 *
 * @ingroup Graphics
 */
class Shader final : public GraphicsResource
{
    CERLIB_DECLARE_DERIVED_OBJECT(GraphicsResource, Shader);

  public:
    /**
     * Creates a shader from source code.
     *
     * @param name The name of the shader. Used for tracking the object as well as error
     * reporting. Does not have to be related to the code.
     * @param source_code The code of the shader.
     * @param defines Reserved; currently has no effect.
     */
    explicit Shader(std::string_view                  name,
                    std::string_view                  source_code,
                    std::span<const std::string_view> defines = {});

    /** Sets the value of a float parameter. */
    void set_value(std::string_view name, float value);

    /** Sets the value of a signed 32-bit integer parameter. */
    void set_value(std::string_view name, int32_t value);

    /** Sets the value of a boolean parameter. */
    void set_value(std::string_view name, bool value);

    /** Sets the value of a 2D vector parameter. */
    void set_value(std::string_view name, Vector2 value);

    /** Sets the value of a 3D vector parameter. */
    void set_value(std::string_view name, Vector3 value);

    /** Sets the value of a 4D vector parameter. */
    void set_value(std::string_view name, Vector4 value);

    /** Sets the value of a matrix parameter. */
    void set_value(std::string_view name, const Matrix& value);

    /** Sets the value of a float array parameter. */
    void set_value(std::string_view name, std::span<const float> values, uint32_t offset = 0);

    /** Sets the value of a 32-bit integer array parameter. */
    void set_value(std::string_view name, std::span<const int32_t> values, uint32_t offset = 0);

    /** Sets the value of a 2D vector array parameter. */
    void set_value(std::string_view name, std::span<const Vector2> values, uint32_t offset = 0);

    /** Sets the value of a 3D vector array parameter. */
    void set_value(std::string_view name, std::span<const Vector3> values, uint32_t offset = 0);

    /** Sets the value of a 4D vector array parameter. */
    void set_value(std::string_view name, std::span<const Vector4> values, uint32_t offset = 0);

    /** Sets the value of a matrix array parameter. */
    void set_value(std::string_view name, std::span<const Matrix> values, uint32_t offset = 0);

    /**
     * Sets the value of an image parameter. The shader will store a reference to
     * the image until the shader is destroyed or the image is unset.
     *
     * @param name The name of the parameter to modify.
     * @param image The image to set.
     */
    void set_value(std::string_view name, const Image& image);

    /** Gets the float value of a parameter. */
    std::optional<float> float_value(std::string_view name) const;

    /** Gets the signed 32-bit integer value of a parameter. */
    std::optional<int32_t> int_value(std::string_view name) const;

    /** Gets the boolean value of a parameter. */
    std::optional<bool> bool_value(std::string_view name) const;

    /** Gets the 2D vector value of a parameter. */
    std::optional<Vector2> vector2_value(std::string_view name) const;

    /** Gets the 3D vector value of a parameter. */
    std::optional<Vector3> vector3_value(std::string_view name) const;

    /** Gets the 4D vector value of a parameter. */
    std::optional<Vector4> vector4_value(std::string_view name) const;

    /** Gets the matrix value of a parameter. */
    std::optional<Matrix> matrix_value(std::string_view name) const;

    /** Gets the image value of a parameter. */
    std::optional<Image> image_value(std::string_view name) const;

    /** Gets a value indicating whether the shader contains a parameter with a specific
     * name. */
    bool has_parameter(std::string_view name) const;

    /**
     * Creates a separate instance of the built-in grayscale shader.
     *
     * The shader provides the following parameters:
     *   - "saturation" (float): The color saturation of the sprite. Default value: 0.0.
     */
    static Shader create_grayscale();
};
} // namespace cer