# cerlib

<img src="https://github.com/cemderv/cerlib/blob/main/misc/cerlib-logo-startpage.png?raw=true" width="300">

#### A lightweight library for 2D game development using modern C++.

- [Homepage](https://cerlib.org)
- [Getting Started](https://cerlib.org/getting-started.html)
- [Online Demo](https://cerlib.org/platformer-demo.html)

<img src="https://github.com/cemderv/cerlib/blob/main/misc/cerlib-cover.webp?raw=true" width="600">

[![CMake Build](https://github.com/cemderv/cerlib/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/cemderv/cerlib/actions/workflows/cmake-multi-platform.yml)

## Features 

- Open Source
  - Permissive Apache 2.0 license with no royalties attached
- Cross-platform
- App framework
  - Based on latest SDL3
- Efficient, GPU-based sprite and UTF-8 text rendering
- Custom sprite shading
    - Simple & safe shading language that transpiles to native shading languages
- Integrated math library
- Audio playback
- Content management system
  - Images: .png, .jpg, .bmp, .dds, .hdr, .tga, .psd, .gif
  - Fonts: .ttf, .otf
  - Sounds: .wav, .mp3, .ogg, .flac
- Particle systems
- First-class support for [ImGui](https://github.com/ocornut/imgui)
  - Automatically available in your game, simply `#include <imgui.h>` and use it
- Compatible with graphics debuggers such as [RenderDoc](https://renderdoc.org)

## What it does not provide

cerlib does **not** provide

- A full-fledged 2D game engine
- A level editor
- An entity-component-system or any kind of scene representation
- A physics API
- A networking API (candidate for future versions)
- 3D rendering (candidate for future versions)

---

In short, cerlib lets you define your game architecture however you desire.
It does not impose a strict model on the game's code.
It provides every audiovisual aspect necessary to comfortably make a 2D game.

---

## Minimal Example

```cpp
#include <cerlib.hpp>
#include <cerlib/Main.hpp>

using namespace cer;

struct MyGame : Game
{
    void load_content() override
    {
      image = Image{"MyImage.png"};
    }

    bool update(const GameTime& time) override
    {
      // Update game logic. Use the time parameter as a reference point for
      // how much time has passed since the last update:
      // ...
      return true;
    }

    void draw(const Window& window) override
    {
      // Draw game content into 'window':
      // ...
      draw_sprite(image, {100, 200}, white);
    }

    Window window = Window{"My Game Window"};
    Image image;
};

int main() {
  // Create and run your game.
  return run_game<MyGame>();
}
```

## Getting Started

For a detailed introduction, please visit [Getting Started](https://cerlib.org/getting-started.html).

## Platform Support

| OS                    | Architecture           | Graphics Backend                                                       |
|-----------------------|------------------------|------------------------------------------------------------------------|
| Windows               | x64, ARM64             | OpenGL ≥ 3.0                                                           |
| Linux                 | x64, ARM64             | OpenGL ≥ 3.0                                                           |
| Android               | All ABIs               | OpenGL ES ≥ 3.0                                                        |
| Web                   | WebAssembly            | WebGL 2                                                                |
| macOS                 | Intel, Apple Silicon   | OpenGL ≥ 3.0                                                           |
| iOS, iPadOS, visionOS | ARM64                  | Metal - [In Development](https://github.com/cemderv/cerlib/issues/3)   |

## Supported Compilers

cerlib has been tested and is confirmed to work with the following compilers:

| Compiler         | Required Version  |
|------------------|-------------------|
| MSVC             | 2022              |
| Clang, Clang-cl  | ≥ 16.0.6          |
| GCC              | ≥ 11.4            |
| Apple Clang      | ≥ 15              |

## Contributing and Feedback

Please see [Contributing](https://cerlib.org/contributing.html) for further details on how to contribute to cerlib.
