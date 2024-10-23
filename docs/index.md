---
title: "cerlib: 2D game development library for modern C++"
icon: material/home-outline
decription: cerlib is a lightweight 2D game development library for modern C++.
hide:
  toc: true
---

# cerlib

![img](img/LogoWide.webp#only-light){ width="300" }
![img](img/LogoWideDark.webp#only-dark){ width="300" }

#

#### A lightweight library for easy 2D game development using C++ 20 and newer.

cerlib focuses on a simple design and at the same time offers you the possibility to design your game architecture your way.

<figure markdown="span">
    ![img](img/Cover.webp){ width="800" }
</figure>

## Features

---

:fontawesome-solid-dove:{.feature} **Open Source**

* _Permissive Apache 2.0 license with no royalties attached_

---

:material-linux:{.feature} **Cross-platform**

* _Runs on multiple platforms, including :fontawesome-brands-windows:, :material-apple:, :material-linux:, :material-android: and :simple-webassembly:_

---

:fontawesome-solid-gears:{.feature} **App framework**

* _Based on latest SDL3_

---

:material-image:{.feature} **Efficient sprite and UTF-8 text rendering**

* _Utilizing the platform's native graphics API and GPU_

---

:material-brush:{.feature} **Custom sprite shading**

* _Simple and safe shading language that transpiles to native shading languages_

---

:material-square-root:{.feature} **Integrated math library**

* _From colors to vectors to matrices_

---

:material-volume-medium:{.feature} **Audio playback**

* _Including faders and channels for playback control_

---

:material-package-variant:{.feature} **Content management system**

* _Images: .png, .jpg, .bmp, .dds, .hdr, .tga, .psd, .gif_
* _Fonts: .ttf, .otf_
* _Sounds: .wav, .mp3, .ogg, .flac_

---

:material-star-shooting:{.feature} **Particle systems**

* _Customizable behaviors via emitters, modifiers and shapes_

---

:material-view-grid-outline:{.feature} **First-class support for [ImGui](https://github.com/ocornut/imgui)**

* _Automatically available in your game, simply `#!cpp #include <imgui.h>` and use it_

---

:material-bug:{.feature} **Compatible with graphics debuggers such as [RenderDoc](https://renderdoc.org)**

* _For in-depth frame analysis_

---


## What it does **not** provide

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

```cpp title="Minimal Example" linenums="1"
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

For a detailed introduction, please visit [Getting Started](getting-started.md).

## Platform Support

| OS | Architecture | Graphics Backend |
|----|--------------|------------------|
| :fontawesome-brands-windows: Windows | x64, ARM64 | OpenGL ≥ 3.0 |
| :material-linux: Linux | x64, ARM64 | OpenGL ≥ 3.0 |
| :material-android: Android | All ABIs | OpenGL ES ≥ 3.0 |
| :simple-webassembly: Web | WebAssembly | WebGL 2 |
| :simple-apple: macOS | Intel, Apple Silicon | OpenGL ≥ 3.0 |
| :material-apple-ios: iOS, iPadOS, visionOS | ARM64 | Metal - [_In Development_](https://github.com/cemderv/cerlib/issues/3) |

## Supported compilers

cerlib has been tested and is confirmed to work with the following compilers:

| Compiler                                    | Required Version  |
|---------------------------------------------|-------------------|
| :fontawesome-brands-windows: MSVC           | 2022              |
| :fontawesome-solid-dragon: Clang, Clang-cl  | ≥ 16.0.6          |
| :simple-gnu: GCC                            | ≥ 11.4            |
| :material-apple: Apple Clang                | ≥ 15              |

## Contributing and Feedback

Please see [Contributing](contributing.md) for further details on how to contribute to cerlib.
