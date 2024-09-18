// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "util/Object.hpp"
#include <gsl/pointers>
#include <string>

namespace cer
{
enum class GraphicsResourceType;
}

namespace cer::details
{
class GraphicsDevice;

class GraphicsResourceImpl : public Object, public Asset
{
  protected:
    explicit GraphicsResourceImpl(gsl::not_null<GraphicsDevice*> parent_device,
                                  GraphicsResourceType           type);

  public:
    NON_COPYABLE_NON_MOVABLE(GraphicsResourceImpl);

    ~GraphicsResourceImpl() noexcept override;

    GraphicsDevice& parent_device();

    GraphicsResourceType type() const;

    std::string_view name() const;

    virtual void set_name(std::string_view name);

  private:
    gsl::not_null<GraphicsDevice*> m_parent_device;
    GraphicsResourceType           m_resource_type;
    std::string                    m_name;
};
} // namespace cer::details