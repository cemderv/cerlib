// cerlib Game Template
// For a detailed introduction, please visit
// https://cerlib.org/getting-started.html

#pragma once

#include <cerlib.hpp>

struct MyGame : cer::Game
{
    void load_content() override
    {
        // Load the game's initial assets:
        // ...
    }

    bool update([[maybe_unused]] const cer::GameTime& time) override
    {
        // Update game logic. Use the time parameter as a reference point for
        // how much time has passed since the last update:
        // ...

        // Return true to keep the game running. Returning false will quit the game.
        return true;
    }

    void draw([[maybe_unused]] const cer::Window& window) override
    {
        // Draw game content into window:
        // ...
    }

    // Create the game's main window.
    cer::Window window = cer::Window{"My Game Window"};
};
