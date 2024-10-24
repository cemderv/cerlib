// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/String.hpp>
#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
namespace details
{
class GraphicsResourceImpl;
}

/**
 * Defines the type of a graphics resource.
 *
 * @ingroup Graphics
 */
enum class GraphicsResourceType
{
    /** The resource represents an image object. */
    Image = 1,

    /** The resource represents a shader object. */
    Shader = 2,
};

/**
 * Represents the base of all graphics resources.
 *
 * Graphics resources are always owned by the user via automatic reference counting.
 * When a resource is set to be used by the library, for example when calling
 * set_shader(), the library holds a reference until it is done using it or when
 * another resource is set using the respective call.
 *
 * @ingroup Graphics
 */
class GraphicsResource
{
    CERLIB_DECLARE_OBJECT(GraphicsResource);

  public:
    /** Gets the type of the resource. */
    auto type() const -> GraphicsResourceType;

    /** Gets the debuggable name of the resource. */
    auto name() const -> std::string_view;

    /**
     * Sets the debuggable name of the resource.
     * The name additionally appears in graphics debuggers.
     *
     * @param name The debuggable name to assign to the resource.
     */
    void set_name(std::string_view name);
};
} // namespace cer
