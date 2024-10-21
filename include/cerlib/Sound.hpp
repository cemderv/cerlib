// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>
#include <span>

namespace cer
{
namespace details
{
class SoundImpl;
}

/**
 * Represents a sound, ready for playback.
 * Sounds can be played using one of the `cer::play_sound()` functions.
 *
 * @ingroup Audio
 */
class Sound
{
    CERLIB_DECLARE_OBJECT(Sound);

  public:
    /**
     * Creates a sound from memory that represents decodable audio data, such as
     * `.wav` or `.mp3`. After the sound is created, the data may be released.
     *
     * @param data The data to load the sound from. The sound will create its own copy of the data.
     */
    explicit Sound(std::span<const std::byte> data);

    /**
     * Lazily loads a Sound object from the storage.
     *
     * @param asset_name The name of the sound in the asset storage.
     *
     * @throw std::runtime_error If the asset does not exist or could not be read or
     * loaded.
     */
    explicit Sound(std::string_view asset_name);

    /** Stops playing the sound and all of its derived channels. */
    void stop();
};
} // namespace cer
