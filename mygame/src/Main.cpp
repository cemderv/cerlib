#include "MyGame.hpp"

// Implements the main function of the target platform. Do not remove!
#include <cerlib/Main.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  // Create & run our game.
  return cer::run_game<MyGame>();
}
