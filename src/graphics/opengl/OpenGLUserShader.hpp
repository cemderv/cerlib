// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "graphics/ShaderImpl.hpp"

namespace cer::details
{
class OpenGLUserShader final : public ShaderImpl
{
  public:
    explicit OpenGLUserShader(GraphicsDevice&  parent_device,
                              std::string_view glsl_code,
                              ParameterList    parameters);

    GLuint gl_handle{};
};
} // namespace cer::details