// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <string_view>

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
 * setVertexBuffer(), the library holds a reference until it is done using it or when
 * another resource is set using the respective call.
 *
 * @ingroup Graphics
 */
class CERLIB_API GraphicsResource
{
    CERLIB_DECLARE_OBJECT(GraphicsResource);

  public:
    /** Gets the type of the resource. */
    GraphicsResourceType type() const;

    /** Gets the debuggable name of the resource. */
    std::string_view name() const;

    /**
     * Sets the debuggable name of the resource.
     * The name additionally appears in graphics debuggers.
     *
     * @param name The debuggable name to assign to the resource.
     */
    void set_name(std::string_view name);
};
} // namespace cer
