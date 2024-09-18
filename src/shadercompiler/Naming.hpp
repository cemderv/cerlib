// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <string_view>

namespace cer::shadercompiler::naming
{
static constexpr std::string_view forbidden_identifier_prefix{"cer_"};
static constexpr std::string_view shader_stage_input_param{"Input"};
static constexpr std::string_view shader_entry_point{"main"};

static constexpr std::string_view sprite_batch_image_param{"sprite_image"};
static constexpr std::string_view sprite_batch_color_attrib{"sprite_color"};
static constexpr std::string_view sprite_batch_uv_attrib{"sprite_uv"};

bool is_identifier_forbidden(std::string_view identifier);
} // namespace cer::shadercompiler::naming
