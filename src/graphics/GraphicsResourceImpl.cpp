// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "GraphicsResourceImpl.hpp"

#include "GraphicsDevice.hpp"
#include "contentmanagement/ContentManager.hpp"

namespace cer::details
{
GraphicsResourceImpl::GraphicsResourceImpl(gsl::not_null<GraphicsDevice*> parent_device,
                                           GraphicsResourceType           type)
    : m_parent_device(parent_device)
    , m_resource_type(type)
{
    m_parent_device->notify_resource_created(this);
}

GraphicsResourceImpl::~GraphicsResourceImpl() noexcept
{
    m_parent_device->notify_resource_destroyed(this);
}

auto GraphicsResourceImpl::name() const -> std::string_view
{
    return m_name;
}

void GraphicsResourceImpl::set_name(std::string_view name)
{
    m_name = name;
}

auto GraphicsResourceImpl::parent_device() -> GraphicsDevice&
{
    return *m_parent_device;
}

auto GraphicsResourceImpl::type() const -> GraphicsResourceType
{
    return m_resource_type;
}
} // namespace cer::details