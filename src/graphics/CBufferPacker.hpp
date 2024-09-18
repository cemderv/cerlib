// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "ShaderImpl.hpp"

namespace cer::details
{
class CBufferPacker final
{
  public:
    struct Result
    {
        uint32_t cbuffer_size{};
    };

    static Result pack_parameters(ShaderImpl::ParameterList& parameters,
                                  uint32_t                   cbuffer_alignment,
                                  bool                       take_max_of_alignment_and_size);
};
} // namespace cer::details
