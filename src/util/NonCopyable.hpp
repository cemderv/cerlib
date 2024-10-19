// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#define NON_COPYABLE(class_name)                                                                   \
    class_name(const class_name&)     = delete;                                                    \
    void operator=(const class_name&) = delete

#define NON_COPYABLE_NON_MOVABLE(class_name)                                                       \
    class_name(const class_name&)         = delete;                                                \
    void operator=(const class_name&)     = delete;                                                \
    class_name(class_name&&) noexcept     = delete;                                                \
    void operator=(class_name&&) noexcept = delete

#define forbid_copy(class_name)                                                                    \
    class_name(const class_name&)     = delete;                                                    \
    void operator=(const class_name&) = delete

#define default_move(class_name)                                                                   \
    class_name(class_name&&) noexcept                    = default;                                \
    auto operator=(class_name&&) noexcept -> class_name& = default
