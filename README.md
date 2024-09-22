# cerlib

![cerlib-logo](misc/cerlib-logo-extrawide.svg)

cerlib is an easy-to-use 2D game library for C++.

- [Homepage](https://cerlib.org)
- [Getting Started](https://cerlib.org/getting-started)
- [API Reference](https://cerlib.org/docs/latest)

[![CMake Build](https://github.com/cemderv/cerlib/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/cemderv/cerlib/actions/workflows/cmake-multi-platform.yml)

## What it provides

- Cross-platform library focused on 2D game development
- App framework
- Content management system
    - Images: .png, .jpg, .bmp, .dds, .hdr, .tga, .psd, .gif
    - Generation of mipmaps
    - Fonts: .ttf, .otf
    - Sounds: .wav, .mp3, .ogg, .flac
- Efficient, GPU-based sprite and UTF8 text rendering
- Custom sprite shading
    - Simple & safe shading language that transpiles to native shading languages
- Integrated linear math library with first-class support
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
    window = cer::Window("My Game Window");
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
    cer::draw_sprite(image, {100, 200}, cer::Color::white());
  }

  cer::Window window;
  cer::Image image;
};

int main() {
  // Create & run our game.
  return cer::run_game<MyGame>();
}
```

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

## C++

The following describes how cerlib uses C++. The minimum required standard is C++ 20.

## Automatic Reference Counting

cerlib makes use of C++’s RAII idiom, meaning that object lifetimes are bound to their scopes.
Every cerlib object is automatically reference counted and the user does not have to release
them manually; they are released at their scope’s end. If the reference count reaches zero,
they are destroyed and freed. Every object is therefore constructed using its type’s constructor;
there are no factories or factory functions.

An example:

```cpp
auto SomeFunction()
{
  auto image = cer::Image(256, 128, ...);
 
  {
    // Reference count of 'image' is incremented (= 2).
    // Both 'image' and 'image2' refer to the same object:
    auto image2 = texture;
  }
  // 'image2' is destroyed, decrementing the reference count (= 1).
  
  return image; // 'image' is moved out of the function,
                // but still alive (reference count = 1).
}
 
auto OtherFunction()
{
  auto image = SomeFunction();
} // 'image' remains in scope, has its reference count
  // decremented (= 0) and is therefore destroyed.
```

Objects in cerlib can be thought of like `std::shared_ptr`, with the difference being that an object
carries its own reference count instead of referring to a control block (intrusive reference counting).

Every type of object is default-constructible, in which case they guarantee that no allocation and no
initialization happens. Additionally, every type of object is copy-constructible and move-constructible.
Move-construction and move-assignment prevents alteration of the reference count.
Objects can be tested for validity (i.e. do they hold a value?) in conditional expressions.

Example:

```cpp
auto image = Image(); // default construct
if (image) // = false
{
  // ...
}
 
texture = Image(...); // initialize a texture
if (image) // = true
{
  // ...
}
```

Objects additionally implement equality operators, which compare the internal pointers.

## Error Handling

cerlib uses C++ exceptions for error handling, though they should not be used for control flow.
Only guard against exceptions that you might expect to be thrown and are able to handle.
Exceptions are thrown by the library mostly when invalid arguments are passed to functions (`std::invalid_argument`)
or as a result of a failure in internal systems (`std::runtime_error`).
Building cerlib without exceptions is not supported.

## RTTI

cerlib uses RTTI and `dynamic_cast` mainly in the shader compiler code and some optional helper functions across the
library.
Building cerlib without RTTI is not supported.

## ABI Stability

cerlib does not guarantee any ABI stability across any major versions.
Your game should ensure that it uses a toolchain that is ABI-compatible with the
cerlib library you’re using.

This is usually not an issue when cerlib is part of your build, which is recommended.

## C++ Modules

Due to the [incomplete state](https://en.cppreference.com/w/cpp/compiler_support/20) of C++ 20 modules, cerlib is not
yet provided as a C++ module.
The support by compilers and CMake is being monitored however. We’re going to continually transition to
modules when they’re becoming widely supported by all compilers and target platforms that are relevant to cerlib.
