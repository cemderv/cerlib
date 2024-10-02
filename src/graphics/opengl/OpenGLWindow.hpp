// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "game/WindowImpl.hpp"

namespace cer::details
{
class OpenGLWindow final : public WindowImpl
{
  public:
    explicit OpenGLWindow(std::string_view        title,
                          uint32_t                id,
                          std::optional<int32_t>  position_x,
                          std::optional<int32_t>  position_y,
                          std::optional<uint32_t> width,
                          std::optional<uint32_t> height,
                          bool                    allow_high_dpi);

    NON_COPYABLE_NON_MOVABLE(OpenGLWindow);

    ~OpenGLWindow() noexcept override;

    void handle_resize_event() override;

    void make_context_current();

    SDL_GLContext sdl_gl_context() const;

  private:
    SDL_GLContext m_gl_context{};
};
} // namespace cer::details
