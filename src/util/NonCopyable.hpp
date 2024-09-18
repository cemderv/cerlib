// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#define NON_COPYABLE(className)                                                                    \
    className(const className&)            = delete;                                               \
    className& operator=(const className&) = delete

#define NON_COPYABLE_NON_MOVABLE(className)                                                        \
    className(const className&)                = delete;                                           \
    className& operator=(const className&)     = delete;                                           \
    className(className&&) noexcept            = delete;                                           \
    className& operator=(className&&) noexcept = delete
