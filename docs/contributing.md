---
icon: material/account-group
---

# Contributing

Thanks for your interest in contributing to cerlib!

cerlib development happens on [GitHub](https://github.com/cemderv/cerlib).

The homepage is [https://cerlib.org](https://cerlib.org).

## How to build

cerlib requires:

- A C++ compiler that implements the C++ 20 standard or newer.
  - Please see [Home](index.md) for a list of supported compilers.
- [CMake](https://cerlib.org) 3.22 or newer

Clone the repository or your fork of it:

```bash
git clone https://github.com/cemderv/cerlib
cd cerlib
```

The easiest way to start development is to open the cerlib folder in any C++ editor or IDE that
supports a CMake workflow, such as:

* [CLion](https://www.jetbrains.com/clion/)
* [Visual Studio](https://visualstudio.microsoft.com/)
* [Visual Studio Code](https://code.visualstudio.com/) with C++ extensions and a C++ toolchain
    * See [Getting Started](getting-started.md)
* [Qt Creator](https://github.com/qt-creator/qt-creator)
* [Xcode](https://developer.apple.com/xcode/)

Alternatively, you can configure and build via the command line:

```bash
cmake --workflow --preset dev
```

!!! note
    CMake workflow presets require version 3.25 or newer.

If you have an older version that does not support workflow presets,
you can configure and build separately:

```bash
# Set up the project
cmake --preset dev

# Build the project
cmake --build --preset dev
```

Build files are stored in the `build` subdirectory.

### Testbed

The `applications` folder contains demo applications as well as a `testbed`.
You may use this testbed as a clean environment to test cerlib in.
Code changes in the testbed should not be committed, unless they are bug fixes or enhancements.

### Game template

The `mygame` folder contains a ready-to-use game template. This is intended to be used by the end-user
as a starting point for their game.

It is **not** part of the cerlib build. If you want to start development on the game template, open its
folder in your editor / IDE directly.

## I want to contribute non-code or have questions

If you want to report a bug or request a feature, [create a new issue](https://github.com/cemderv/cerlib/issues).

For other kinds of feedback, you may contact the developers directly via:

* [Discussions](https://github.com/cemderv/cerlib/discussions)
* [Email](mailto:cem@dervis.de) 

## C++

The following describes how cerlib uses C++. The minimum required standard is C++ 20.

## Coding Standards

cerlib uses static analysis tools such as Cppcheck and clang-tidy as part of its build.
This is automatically done using CMake.

Code changes that should be merged into cerlib must pass all checks.

Formatting is ensured using clang-format.

Continuous integration ensures that code changes are compatible with all advertised compilers.

## Automatic Reference Counting

cerlib makes use of C++'s RAII idiom, meaning that object lifetimes are bound to their scopes.
Every cerlib object is automatically reference counted and the user does not have to release
them manually; they are released at their scope’s end. If the reference count reaches zero,
they are destroyed and freed. Every object is therefore constructed using its type’s constructor;
there are no factories or factory functions.

An example:

```cpp
cer::Image some_function()
{
  cer::Image image{256, 128, ...};
 
  {
    // Reference count of 'image' is incremented (= 2).
    // Both 'image' and 'image2' refer to the same object:
    cer::Image image2 = texture;
  }
  // 'image2' is destroyed, decrementing the reference count (= 1).
  
  return image; // 'image' is moved out of the function,
                // but still alive (reference count = 1).
}
 
void other_function()
{
  cer::Image image = some_function();
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
cer::Image image; // default construct
if (image) // = false
{
  // ...
}
 
image = cer::Image{...}; // initialize an image
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

# C++ Modules

Due to the incomplete state of C++ 20 modules, cerlib is not yet provided as a C++ module.
The support by compilers and CMake is being monitored however.
Transition to modules is going to happen transitionally when they are becoming widely supported by all compilers and target platforms that are relevant to cerlib.
