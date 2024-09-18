// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

namespace cer
{
/**
 * Defines a modifier key on a keyboard, such as the shift and control keys.
 * Values of this type can be bitwise combined to represent multiple states.
 *
 * @ingroup Input
 */
enum class KeyModifier
{
    None         = 0, /** */
    LeftShift    = 1 << 0, /** */
    RightShift   = 1 << 1, /** */
    LeftControl  = 1 << 2, /** */
    RightControl = 1 << 3, /** */
    LeftAlt      = 1 << 4, /** */
    RightAlt     = 1 << 5, /** */
    Num          = 1 << 6, /** */
    Caps         = 1 << 7, /** */
};
} // namespace cer