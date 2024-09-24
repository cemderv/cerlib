#pragma once

#include <cerlib.hpp>

class MyGame : public cer::Game {
public:
  MyGame() {
    window = cer::Window{"My Game Window"};
  }

  void load_content() override {
  }

  bool update([[maybe_unused]] const cer::GameTime& time) override {
    // Update game logic. Use the time parameter as a reference point for
    // how much time has passed since the last update:
    // ...

    return true;
  }

  void draw([[maybe_unused]] const cer::Window& window) override {
    // Draw game content into window:
    // ...
  }

  cer::Window window;
};
