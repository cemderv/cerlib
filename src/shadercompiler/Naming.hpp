// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <string_view>

namespace cer::shadercompiler::naming
{
static constexpr auto forbidden_identifier_prefix = std::string_view{"cer_"};
static constexpr auto shader_stage_input_param    = std::string_view{"Input"};
static constexpr auto shader_entry_point          = std::string_view{"main"};
static constexpr auto sprite_batch_image_param    = std::string_view{"sprite_image"};
static constexpr auto sprite_batch_color_attrib   = std::string_view{"sprite_color"};
static constexpr auto sprite_batch_uv_attrib      = std::string_view{"sprite_uv"};

auto is_identifier_forbidden(std::string_view identifier) -> bool;
} // namespace cer::shadercompiler::naming
