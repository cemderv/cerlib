// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/GraphicsResource.hpp>

#include "GraphicsResourceImpl.hpp"
#include "util/Util.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(GraphicsResource)

GraphicsResourceType GraphicsResource::type() const
{
    DECLARE_THIS_IMPL;
    return impl->type();
}

std::string_view GraphicsResource::name() const
{
    DECLARE_THIS_IMPL;
    return impl->name();
}

void GraphicsResource::set_name(std::string_view name)
{
    DECLARE_THIS_IMPL;
    impl->set_name(name);
}
} // namespace cer