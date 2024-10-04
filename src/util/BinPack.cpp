// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "BinPack.hpp"

#include "cerlib/Math.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

namespace cer
{
static auto is_contained_in(const BinPack::Rect& a, const BinPack::Rect& b) -> bool
{
    return a.x >= b.x && a.y >= b.y && a.x + a.width <= b.x + b.width &&
           a.y + a.height <= b.y + b.height;
}

BinPack::BinPack() = default;

BinPack::BinPack(int32_t width, int32_t height)
    : m_bin_width(width)
    , m_bin_height(height)
{
    m_free_rectangles.emplace_back(0, 0, width, height);
}

auto BinPack::insert(int32_t width, int32_t height) -> std::optional<BinPack::Rect>
{
    // Unused in this function. We don't need to know the score after finding the
    // position.
    auto score1 = std::numeric_limits<int>::max();
    auto score2 = std::numeric_limits<int>::max();

    const auto new_node = find_position_for_new_node(width, height, score1, score2);

    if (new_node.has_value())
    {
        place_rect(*new_node);
    }

    return new_node;
}

void BinPack::insert(std::vector<Size>& rects, std::vector<Rect>& dst)
{
    dst.clear();

    while (!rects.empty())
    {
        auto best_score1     = std::numeric_limits<int>::max();
        auto best_score2     = std::numeric_limits<int>::max();
        auto best_rect_index = size_t(-1);
        auto best_node       = std::optional<Rect>{};

        for (size_t i = 0; i < rects.size(); ++i)
        {
            auto       score1   = 0;
            auto       score2   = 0;
            const auto new_node = score_rect(rects[i].width, rects[i].height, score1, score2);

            if (score1 < best_score1 || (score1 == best_score1 && score2 < best_score2))
            {
                best_score1     = score1;
                best_score2     = score2;
                best_node       = new_node;
                best_rect_index = i;
            }
        }

        if (best_rect_index == size_t(-1))
        {
            return;
        }

        place_rect(best_node.value());
        dst.push_back(best_node.value());
        rects.at(best_rect_index) = rects.back();
        rects.pop_back();
    }
}

void BinPack::place_rect(const Rect& node)
{
    for (size_t i = 0; i < m_free_rectangles.size(); /*left blank*/)
    {
        if (split_free_node(m_free_rectangles[i], node))
        {
            m_free_rectangles[i] = m_free_rectangles.back();
            m_free_rectangles.pop_back();
        }
        else
        {
            ++i;
        }
    }

    prune_free_list();

    m_used_rectangles.push_back(node);
}

auto BinPack::score_rect(int width, int height, int& score1, int& score2) const
    -> std::optional<Rect>
{
    score1 = std::numeric_limits<int>::max();
    score2 = std::numeric_limits<int>::max();

    const auto new_node = find_position_for_new_node(width, height, score1, score2);

    // Cannot fit the current rect.
    if (!new_node.has_value())
    {
        score1 = std::numeric_limits<int>::max();
        score2 = std::numeric_limits<int>::max();
    }

    return new_node;
}

/// Computes the ratio of used surface area.
auto BinPack::occupancy() const -> double
{
    auto used_surface_area = uint64_t(0);

    for (const auto& used_rectangle : m_used_rectangles)
    {
        used_surface_area += uint64_t(used_rectangle.width) * uint64_t(used_rectangle.height);
    }

    return double(used_surface_area) / (double(m_bin_width) * double(m_bin_height));
}

auto BinPack::find_position_for_new_node(int  width,
                                         int  height,
                                         int& best_area_fit,
                                         int& best_short_side_fit) const -> std::optional<Rect>
{
    auto best_node = std::optional<Rect>{};

    best_area_fit       = std::numeric_limits<int>::max();
    best_short_side_fit = std::numeric_limits<int>::max();

    for (const auto& free_rectangle : m_free_rectangles)
    {
        const auto area_fit = (free_rectangle.width * free_rectangle.height) - (width * height);

        // Try to place the rect in upright (non-flipped) orientation.
        if (free_rectangle.width >= width && free_rectangle.height >= height)
        {
            const auto leftover_horiz = abs(free_rectangle.width - width);
            const auto leftover_vert  = abs(free_rectangle.height - height);
            const auto short_side_fit = min(leftover_horiz, leftover_vert);

            if (area_fit < best_area_fit ||
                (area_fit == best_area_fit && short_side_fit < best_short_side_fit))
            {
                best_node           = Rect{free_rectangle.x, free_rectangle.y, width, height};
                best_short_side_fit = short_side_fit;
                best_area_fit       = area_fit;
            }
        }
    }

    return best_node;
}

auto BinPack::split_free_node(const Rect& free_node, const Rect& used_node) -> bool
{
    // Test with SAT if the rectangles even intersect.
    if (used_node.x >= free_node.x + free_node.width ||
        used_node.x + used_node.width <= free_node.x ||
        used_node.y >= free_node.y + free_node.height ||
        used_node.y + used_node.height <= free_node.y)
    {
        return false;
    }

    // We add up to four new free rectangles to the free rectangles list below.
    // None of these four newly added free rectangles can overlap any other three,
    // so keep a mark of them to avoid testing them against each other.
    m_new_free_rectangles_last_size = m_new_free_rectangles.size();

    if (used_node.x < free_node.x + free_node.width && used_node.x + used_node.width > free_node.x)
    {
        // New node at the top side of the used node.
        if (used_node.y > free_node.y && used_node.y < free_node.y + free_node.height)
        {
            auto new_node   = free_node;
            new_node.height = used_node.y - new_node.y;
            insert_new_free_rectangle(new_node);
        }

        // New node at the bottom side of the used node.
        if (used_node.y + used_node.height < free_node.y + free_node.height)
        {
            auto new_node   = free_node;
            new_node.y      = used_node.y + used_node.height;
            new_node.height = free_node.y + free_node.height - (used_node.y + used_node.height);
            insert_new_free_rectangle(new_node);
        }
    }

    if (used_node.y < free_node.y + free_node.height &&
        used_node.y + used_node.height > free_node.y)
    {
        // New node at the left side of the used node.
        if (used_node.x > free_node.x && used_node.x < free_node.x + free_node.width)
        {
            auto new_node  = free_node;
            new_node.width = used_node.x - new_node.x;
            insert_new_free_rectangle(new_node);
        }

        // New node at the right side of the used node.
        if (used_node.x + used_node.width < free_node.x + free_node.width)
        {
            auto new_node  = free_node;
            new_node.x     = used_node.x + used_node.width;
            new_node.width = free_node.x + free_node.width - (used_node.x + used_node.width);
            insert_new_free_rectangle(new_node);
        }
    }

    return true;
}

void BinPack::insert_new_free_rectangle(const Rect& new_free_rect)
{
    assert(new_free_rect.width > 0);
    assert(new_free_rect.height > 0);

    for (size_t i = 0; i < m_new_free_rectangles_last_size;)
    {
        // This new free rect is already accounted for?
        if (is_contained_in(new_free_rect, m_new_free_rectangles[i]))
        {
            return;
        }

        // Does this new free rect obsolete a previous new free rect?
        if (is_contained_in(m_new_free_rectangles[i], new_free_rect))
        {
            // Remove i'th new free rect, but do so by retaining the order
            // of the older vs newest free rectangles that we may still be placing
            // in calling function SplitFreeNode().
            m_new_free_rectangles[i] = m_new_free_rectangles[--m_new_free_rectangles_last_size];
            m_new_free_rectangles[m_new_free_rectangles_last_size] = m_new_free_rectangles.back();
            m_new_free_rectangles.pop_back();
        }
        else
        {
            ++i;
        }
    }

    m_new_free_rectangles.push_back(new_free_rect);
}

void BinPack::prune_free_list()
{
    // Test all newly introduced free rectangles against old free rectangles.
    for (const auto& rect : m_free_rectangles)
    {
        for (size_t j = 0; j < m_new_free_rectangles.size();)
        {
            if (is_contained_in(m_new_free_rectangles[j], rect))
            {
                m_new_free_rectangles[j] = m_new_free_rectangles.back();
                m_new_free_rectangles.pop_back();
            }
            else
            {
                // The old free rectangles can never be contained in any of the
                // new free rectangles (the new free rectangles keep shrinking
                // in size)
                assert(!is_contained_in(rect, m_new_free_rectangles[j]));
                ++j;
            }
        }
    }

    // Merge new and old free rectangles to the group of old free rectangles.
    m_free_rectangles.insert(m_free_rectangles.end(),
                             m_new_free_rectangles.begin(),
                             m_new_free_rectangles.end());

    m_new_free_rectangles.clear();
}

} // namespace cer
