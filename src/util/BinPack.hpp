// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Rectangle.hpp"

#include <optional>
#include <vector>

namespace cer
{
class BinPack final
{
  public:
    struct Size
    {
        int32_t width{};
        int32_t height{};
    };

    struct Rect
    {
        explicit Rect(int32_t x, int32_t y, int32_t width, int32_t height)
            : x(x)
            , y(y)
            , width(width)
            , height(height)
        {
        }

        auto to_rectangle() const -> Rectangle
        {
            return {static_cast<float>(x),
                    static_cast<float>(y),
                    static_cast<float>(width),
                    static_cast<float>(height)};
        }

        int32_t x{};
        int32_t y{};
        int32_t width{};
        int32_t height{};
    };

    BinPack();

    explicit BinPack(int32_t width, int32_t height);

    void insert(std::vector<Size>& rects, std::vector<Rect>& dst);

    std::optional<Rect> insert(int32_t width, int32_t height);

    double occupancy() const;

  private:
    std::optional<Rect> score_rect(int width, int height, int& score1, int& score2) const;

    void place_rect(const Rect& node);

    std::optional<Rect> find_position_for_new_node(int  width,
                                                   int  height,
                                                   int& bestAreaFitFit,
                                                   int& best_short_side_fit) const;

    void insert_new_free_rectangle(const Rect& newFreeRect);

    bool split_free_node(const Rect& free_node, const Rect& used_node);

    void prune_free_list();

    int32_t m_bin_width{};
    int32_t m_bin_height{};

    size_t            m_new_free_rectangles_last_size{};
    std::vector<Rect> m_new_free_rectangles;

    std::vector<Rect> m_used_rectangles;
    std::vector<Rect> m_free_rectangles;
};
} // namespace cer
