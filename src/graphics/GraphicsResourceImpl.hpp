// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "util/Object.hpp"
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
    explicit GraphicsResourceImpl(GraphicsDevice& parent_device, GraphicsResourceType type);

  public:
    forbid_copy_and_move(GraphicsResourceImpl);

    ~GraphicsResourceImpl() noexcept override;

    auto parent_device() -> GraphicsDevice&;

    auto type() const -> GraphicsResourceType;

    auto name() const -> std::string_view;

    virtual void set_name(std::string_view name);

  private:
    GraphicsDevice&      m_parent_device;
    GraphicsResourceType m_resource_type;
    std::string          m_name;
};
} // namespace cer::details