// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#define forbid_copy(class_name)                                                                    \
    class_name(const class_name&)     = delete;                                                    \
    void operator=(const class_name&) = delete

#define forbid_move(class_name)                                                                    \
    class_name(class_name&&) noexcept     = delete;                                                \
    void operator=(class_name&&) noexcept = delete

#define default_copy(class_name)                                                                   \
    class_name(const class_name&)                    = default;                                    \
    auto operator=(const class_name&) -> class_name& = default

#define default_move(class_name)                                                                   \
    class_name(class_name&&) noexcept                    = default;                                \
    auto operator=(class_name&&) noexcept -> class_name& = default

#define forbid_copy_and_move(class_name)                                                           \
    forbid_copy(class_name);                                                                       \
    forbid_move(class_name)
