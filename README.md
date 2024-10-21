# cerlib

<img src="https://github.com/cemderv/cerlib/blob/main/misc/cerlib-logo-startpage.png?raw=true" width="300">

#### cerlib is an easy-to-use 2D game library for C++.

- [Homepage](https://cerlib.org)
- [Getting Started](https://cerlib.org/getting-started)
- [Online Demo](https://cerlib.org/demo)

<img src="https://github.com/cemderv/cerlib/blob/main/misc/cerlib-cover.webp?raw=true" width="600">

[![CMake Build](https://github.com/cemderv/cerlib/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/cemderv/cerlib/actions/workflows/cmake-multi-platform.yml)

## What it provides

- Simple and lightweight cross-platform library focused on 2D game development
- App framework
  - Based on SDL3
- Content management system
    - Images: .png, .jpg, .bmp, .dds, .hdr, .tga, .psd, .gif
    - Fonts: .ttf, .otf
    - Sounds: .wav, .mp3, .ogg, .flac
- Efficient, GPU-based sprite and UTF8 text rendering
- Custom sprite shading
    - Simple & safe shading language that transpiles to native shading languages
- Integrated linear math library with first-class support
- Simple audio playback including faders and channels for playback control
- First-class support for [ImGui](https://github.com/ocornut/imgui)
    - Automatically available in your game, simply `#include <imgui.h>` and use it
- Compatibility with graphics debuggers such as [RenderDoc](https://renderdoc.org)

## What it does not provide

cerlib does **not** provide

- A full-fledged 2D game engine experience
- A level editor
- An entity-component-system or any kind of scene representation
- A physics library
- Networking functionality (candidate for future versions)
- 3D rendering (candidate for future versions)

In short, cerlib lets you define your game architecture however you desire.
It does not impose a strict model on the game's code.
It provides every basic audiovisual aspect necessary to develop a 2D game.

## Example

```cpp
#include <cerlib.hpp>
#include <cerlib/Main.hpp>

struct MyGame : cer::Game {
  MyGame() {
    window = cer::Window{"My Game Window"};
  }

  void load_content() override {
    image = cer::load_image("MyImage.png");
  }

  bool update(const cer::GameTime& time) override {
    // Update game logic. Use the time parameter as a reference point for
    // how much time has passed since the last update:
    // ...
    return true;
  }

  void draw(const cer::Window& window) override {
    // Draw game content into 'window':
    // ...
    cer::draw_sprite(image, {100, 200}, cer::white);
  }

  cer::Window window;
  cer::Image image;
};

int main() {
  // Create & run our game.
  return cer::run_game<MyGame>();
}
```

## Quickstart

This is for those who want to get started quickly.
It assumes that you have a working C++ 20 toolchain and editor / IDE already.

First, clone the repository:

```bash
git clone https://github.com/cemderv/cerlib
cd cerlib
```

The easiest way to start is to open the `mygame` folder in the editor / IDE of your choice. **It must support CMake-based projects**. You should then be able to select a preset such as `debug` and directly hit play and run your game.

#### Building from the command line

```bash
cd mygame
cmake --workflow --preset debug
```

For a detailed introduction and tutorials, please visit the [Getting Started](https://cerlib.org/getting-started) page.

## Platform support

|          | OpenGL    | OpenGL ES / WebGL | Metal   |
|----------|-----------|-------------------|---------|
| Windows  | ✅ (≥ 3.0) |                   |         |
| Linux    | ✅ (≥ 3.0) |                   |         |
| Android  |           | ✅ (≥ ES 3.0)      |         |
| Web      |           | ✅ (WebGL 2)       |         |
| macOS    | ✅ (≥ 3.0) |                   | Planned |
| iOS      |           |                   | Planned |
| visionOS |           |                   | Planned |

## Architectures

|        | Windows | macOS | Linux |
|--------|---------|-------|-------|
| x64    | ✅       | ✅     | ✅     |
| x86    | ✅       |       | ✅     |
| ARM64  | ✅       | ✅     | ✅     | 
| RISC-V |         |       | ✅     |

## Supported compilers

cerlib has been tested and is confirmed to work with the following compilers:

- MSVC 2022
- Clang ≥ 16.0.6
- GCC ≥ 13.3
- Apple Clang ≥ 15

## Contributing and Feedback

Please see [Contributing](CONTRIBUTING.md) for further details on how to contribute to cerlib.
