// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Image.hpp"
#include "util/NonCopyable.hpp"
#include <functional>
#include <string>

class RenderingTestHelper
{
  public:
    using RenderFunction = std::function<void()>;

    RenderingTestHelper(uint32_t width, uint32_t height, const cer::Window& window);

    NON_COPYABLE_NON_MOVABLE(RenderingTestHelper);

    ~RenderingTestHelper() noexcept = default;

    void test_render(std::string_view test_name, const RenderFunction& function);

    void generate_reference_image(std::string_view test_name, const RenderFunction& function);

  private:
    auto get_reference_image_filename(std::string_view test_name) const -> std::string;

    cer::Image m_canvas;
};
